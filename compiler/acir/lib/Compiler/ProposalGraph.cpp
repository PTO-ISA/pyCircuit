#include "ProposalGraph.h"
#include "CheckGraph.h"
#include "Dialect/ACIR/ACIRNumericComposition.h"
#include "Dialect/ACIR/ACIRNumericNextUse.h"

#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/IR/BuiltinOps.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"

#include <set>

using namespace mlir;

namespace acir::compiler {
namespace {

enum class GuardKind { False, True, SSA, Complex };

struct GuardView {
  GuardKind kind = GuardKind::Complex;
  Value value;
};

std::optional<bool> constantI1(Value value) {
  auto constant = value.getDefiningOp<arith::ConstantOp>();
  auto attribute =
      constant ? dyn_cast<IntegerAttr>(constant.getValue()) : IntegerAttr();
  if (!attribute || !attribute.getType().isInteger(1))
    return std::nullopt;
  return attribute.getValue().isOne();
}

GuardView effectiveGuard(const ProposalContribution &contribution) {
  std::optional<bool> valid = constantI1(contribution.valid);
  std::optional<bool> path = constantI1(contribution.path);
  if ((valid && !*valid) || (path && !*path))
    return {GuardKind::False, {}};
  if (valid && *valid && path && *path)
    return {GuardKind::True, {}};
  if (valid && *valid)
    return {GuardKind::SSA, contribution.path};
  if (path && *path)
    return {GuardKind::SSA, contribution.valid};
  return {GuardKind::Complex, {}};
}

bool isComplement(Value candidate, Value other) {
  auto xorOp = candidate.getDefiningOp<arith::XOrIOp>();
  if (!xorOp)
    return false;
  auto lhs = constantI1(xorOp.getLhs());
  auto rhs = constantI1(xorOp.getRhs());
  return (lhs && *lhs && xorOp.getRhs() == other) ||
         (rhs && *rhs && xorOp.getLhs() == other);
}

ProposalConflictKind classifyConflict(const ProposalContribution &left,
                                      const ProposalContribution &right) {
  GuardView lhs = effectiveGuard(left);
  GuardView rhs = effectiveGuard(right);
  if (lhs.kind == GuardKind::False || rhs.kind == GuardKind::False)
    return ProposalConflictKind::MutuallyExclusive;
  if (lhs.kind == GuardKind::True && rhs.kind == GuardKind::True)
    return ProposalConflictKind::GuaranteedOverlap;
  if (lhs.kind == GuardKind::SSA && rhs.kind == GuardKind::SSA) {
    if (lhs.value == rhs.value)
      return ProposalConflictKind::GuaranteedOverlap;
    if (isComplement(lhs.value, rhs.value) ||
        isComplement(rhs.value, lhs.value))
      return ProposalConflictKind::MutuallyExclusive;
  }
  return ProposalConflictKind::PossibleOverlap;
}

DictionaryAttr formalState(Builder &builder, DictionaryAttr port) {
  return builder.getDictionaryAttr({
      builder.getNamedAttr("kind", builder.getStringAttr("formal")),
      builder.getNamedAttr("parameter", port.get("parameter")),
      builder.getNamedAttr("ordinal", port.get("ordinal")),
  });
}

DictionaryAttr ownedState(Builder &builder, ac::RegOp reg) {
  return builder.getDictionaryAttr({
      builder.getNamedAttr("kind", builder.getStringAttr("owned")),
      builder.getNamedAttr("declaration", reg->getAttr("ac.declaration")),
      builder.getNamedAttr("element", reg->getAttr("ac.element")),
  });
}

bool hasNumericInventory(ac::RuleOp rule) {
  if (auto required = rule->getAttrOfType<ArrayAttr>("ac.required_numeric");
      required && !required.empty())
    return true;
  for (Operation &operation : rule.getBody().front()) {
    StringRef name = operation.getName().getStringRef();
    if (name.starts_with("ac.math.") || name == "ac.numeric.proof" ||
        name == "ac.value.binding" || name == "ac.value.use" ||
        operation.hasAttr("ac.check_template"))
      return true;
  }
  return false;
}

LogicalResult verifySupportedNumericRule(ac::RuleOp rule,
                                         ac::detail::EmitError emitError) {
  if (!hasNumericInventory(rule))
    return success();
  if (ac::hasNumericCompositionContract(rule)) {
    if (failed(ac::verifyNumericCompositionClosure(rule)))
      return emitError()
             << "proposal graph requires verified numeric composition closure";
    return success();
  }
  if (!ac::hasNumericNextUseContract(rule) ||
      failed(ac::verifyNumericNextUseClosure(rule)))
    return emitError() << "proposal graph supports only verified U1 numeric "
                          "next-state rules";
  return success();
}

DictionaryAttr requiredUseTarget(ac::RuleOp rule, ac::ValueUseOp use) {
  auto required = rule->getAttrOfType<ArrayAttr>("ac.required_uses");
  auto entry = required && required.size() == 1
                   ? dyn_cast<DictionaryAttr>(required[0])
                   : DictionaryAttr();
  if (!entry || entry.getAs<DictionaryAttr>("id") != use.getIdAttr() ||
      entry.getAs<DictionaryAttr>("value") != use.getSourceAttr())
    return {};
  return entry.getAs<DictionaryAttr>("target");
}

DictionaryAttr compositionUseTarget(ac::RuleOp rule, ac::ValueUseOp use) {
  auto required = rule->getAttrOfType<ArrayAttr>("ac.required_uses");
  if (!required)
    return {};
  for (Attribute raw : required) {
    auto entry = dyn_cast<DictionaryAttr>(raw);
    if (entry && entry.getAs<DictionaryAttr>("id") == use.getIdAttr() &&
        entry.getAs<DictionaryAttr>("value") == use.getSourceAttr())
      return entry.getAs<DictionaryAttr>("target");
  }
  return {};
}

} // namespace

FailureOr<ProposalGraph>
buildSourceProposalGraph(const ModuleGraph &modules,
                         ac::detail::EmitError emitError) {
  return buildSourceProposalGraph(modules, nullptr, emitError);
}

FailureOr<ProposalGraph>
buildSourceProposalGraph(const ModuleGraph &modules, const CheckGraph *checks,
                         ac::detail::EmitError emitError) {
  if (!modules.root)
    return emitError() << "proposal graph requires a rooted ModuleGraph";
  ProposalGraph graph;
  graph.modules = &modules;
  graph.checks = checks;
  graph.globalPermitAlways = !checks || checks->bindings.empty();
  DenseMap<Attribute, size_t> stateIndex;
  Builder builder(modules.root->module.getContext());

  auto addState = [&](DictionaryAttr id,
                      DictionaryAttr logical) -> LogicalResult {
    auto [position, inserted] = stateIndex.try_emplace(id, graph.states.size());
    if (inserted) {
      StateProposals state;
      state.stateID = id;
      state.logicalType = logical;
      state.commit.stateID = id;
      state.commit.permitAlways = graph.globalPermitAlways;
      graph.states.push_back(std::move(state));
    } else if (graph.states[position->second].logicalType != logical) {
      return emitError()
             << "proposal StateID has inconsistent exact LogicalTypes";
    }
    return success();
  };

  for (InstanceView *view : modules.postOrder()) {
    // Resolve the compiler stage once per view. The stage selects which logical
    // attribute the regs carry: final designs have ac.logical_element stripped
    // by final materialization, so their regs carry ac.logical_type only.
    auto unit = view->module->getParentOfType<ModuleOp>();
    auto stage =
        unit ? unit->getAttrOfType<StringAttr>("ac.stage") : StringAttr();
    if (!stage || (stage.getValue() != "source" && stage.getValue() != "final"))
      return emitError() << "module has an unsupported compiler stage '"
                         << (stage ? stage.getValue()
                                   : llvm::StringRef("<missing>"))
                         << "'";
    bool finalStage = stage.getValue() == "final";
    for (ac::RegOp reg : view->module.getBody().front().getOps<ac::RegOp>()) {
      auto relative = ownedState(builder, reg);
      auto found = view->ownedStates.find(relative);
      auto logical = reg->getAttrOfType<DictionaryAttr>(
          finalStage ? "ac.logical_type" : "ac.logical_element");
      if (found == view->ownedStates.end())
        return failure();
      if (!logical)
        return emitError()
               << "proposal reg has no stage-matching logical type '"
               << (finalStage ? "ac.logical_type" : "ac.logical_element")
               << "'";
      if (failed(addState(found->second, logical)))
        return failure();
    }
    auto ports = view->module->getAttrOfType<ArrayAttr>("ac.ports");
    for (Attribute raw : ports) {
      auto port = cast<DictionaryAttr>(raw);
      auto alias = view->formalAliases.find(formalState(builder, port));
      if (alias == view->formalAliases.end() ||
          failed(addState(alias->second, port.getAs<DictionaryAttr>("type"))))
        return failure();
    }

    for (ac::RuleOp rule :
         view->module.getBody().front().getOps<ac::RuleOp>()) {
      auto bindings = rule->getAttrOfType<ArrayAttr>("ac.output_bindings");
      auto types = rule->getAttrOfType<ArrayAttr>("ac.output_types");
      if (!bindings || !types || bindings.size() != rule.getTargets().size() ||
          types.size() != rule.getTargets().size() ||
          rule.getBody().getBlocks().size() != 1)
        return emitError() << "proposal rule output contract is not closed";
      Block &body = rule.getBody().front();
      if (failed(verifySupportedNumericRule(rule, emitError)))
        return failure();
      if (ac::hasNumericCompositionContract(rule)) {
        auto yield = dyn_cast<ac::YieldOp>(body.getTerminator());
        if (!yield || yield.getValues().size() != 2 * rule.getTargets().size())
          return emitError() << "compositional proposal requires one exact "
                                "yield pair per output";
        SmallVector<SmallVector<ProposalUseFact>> factsPerOutput(
            rule.getTargets().size());
        auto outputIndex = [&](DictionaryAttr target) -> FailureOr<size_t> {
          auto kind = target.getAs<StringAttr>("kind");
          auto state = target.getAs<DictionaryAttr>("state");
          if (!kind || kind.getValue() != "next_scalar" || !state)
            return emitError()
                   << "compositional proposal target is not next_scalar";
          std::optional<size_t> found;
          for (auto [index, binding] : llvm::enumerate(bindings)) {
            if (binding != state)
              continue;
            if (found)
              return emitError()
                     << "compositional target matches repeated rule outputs";
            found = index;
          }
          if (!found)
            return emitError() << "compositional target has no rule output";
          return *found;
        };
        for (ac::SourceUseOp use : body.getOps<ac::SourceUseOp>()) {
          auto index = outputIndex(use.getTarget());
          if (failed(index))
            return failure();
          ProposalUseFact fact;
          fact.useID = use.getId();
          fact.sourceID = use.getSource();
          fact.value = use.getValue();
          fact.valid = use.getValid();
          fact.path = use.getPath();
          fact.use = use;
          factsPerOutput[*index].push_back(std::move(fact));
        }
        for (ac::ValueUseOp use : body.getOps<ac::ValueUseOp>()) {
          DictionaryAttr target = compositionUseTarget(rule, use);
          if (!target)
            return emitError()
                   << "compositional ValueUse has no exact RequiredUse target";
          auto index = outputIndex(target);
          if (failed(index))
            return failure();
          ProposalUseFact fact;
          fact.useID = use.getIdAttr();
          fact.sourceID = use.getSourceAttr();
          fact.value = use.getValue();
          fact.valid = use.getValid();
          fact.path = use.getPath();
          fact.valueUse = use;
          factsPerOutput[*index].push_back(std::move(fact));
        }
        for (auto [index, facts] : llvm::enumerate(factsPerOutput)) {
          if (facts.empty())
            continue;
          auto target = dyn_cast<DictionaryAttr>(bindings[index]);
          auto id = modules.resolveState(*view, target, emitError);
          auto logical = dyn_cast<DictionaryAttr>(types[index]);
          if (failed(id) || !logical || failed(addState(*id, logical)))
            return failure();

          ProposalContribution contribution;
          contribution.stateID = *id;
          contribution.logicalType = logical;
          contribution.data = yield.getValues()[2 * index];
          contribution.enabled = yield.getValues()[2 * index + 1];
          // The composed E is the rule-level guard. Individual branch guards
          // remain available in sourceFacts for independent final verification.
          contribution.valid = contribution.enabled;
          contribution.path = contribution.enabled;
          contribution.owner = view;
          contribution.rule = rule;
          contribution.child = view->placement;
          contribution.sourceFacts = facts;
          contribution.composition = true;
          if (facts.size() == 1) {
            const ProposalUseFact &fact = facts.front();
            contribution.value = fact.value;
            contribution.valid = fact.valid;
            contribution.path = fact.path;
            contribution.useID = fact.useID;
            contribution.sourceID = fact.sourceID;
            contribution.use = fact.use;
            contribution.valueUse = fact.valueUse;
          }
          size_t contributionIndex = graph.contributions.size();
          graph.contributions.push_back(std::move(contribution));
          graph.states[stateIndex.lookup(*id)].contributions.push_back(
              contributionIndex);
        }
        continue;
      }
      auto addContribution =
          [&](DictionaryAttr target, Value data, Value enabled, Value value,
              Value valid, Value path, DictionaryAttr useID,
              DictionaryAttr sourceID, ac::SourceUseOp sourceUse,
              ac::ValueUseOp valueUse) -> LogicalResult {
        auto kind = target.getAs<StringAttr>("kind");
        auto state = target.getAs<DictionaryAttr>("state");
        if (!kind || kind.getValue() != "next_scalar" || !state)
          return emitError()
                 << "proposal graph supports next_scalar SourceUse only";
        std::optional<size_t> output;
        for (auto [index, binding] : llvm::enumerate(bindings))
          if (binding == state) {
            if (output)
              return emitError()
                     << "SourceUse target matches repeated rule outputs";
            output = index;
          }
        if (!output)
          return emitError() << "SourceUse target has no rule output";
        auto id = modules.resolveState(*view, state, emitError);
        auto logical = dyn_cast<DictionaryAttr>(types[*output]);
        if (failed(id) || !logical || failed(addState(*id, logical)))
          return failure();

        ProposalContribution contribution;
        contribution.stateID = *id;
        contribution.logicalType = logical;
        contribution.data = data;
        contribution.enabled = enabled;
        contribution.value = value;
        contribution.valid = valid;
        contribution.path = path;
        contribution.useID = useID;
        contribution.sourceID = sourceID;
        contribution.owner = view;
        contribution.rule = rule;
        contribution.use = sourceUse;
        contribution.valueUse = valueUse;
        contribution.child = view->placement;
        size_t contributionIndex = graph.contributions.size();
        graph.contributions.push_back(std::move(contribution));
        graph.states[stateIndex.lookup(*id)].contributions.push_back(
            contributionIndex);
        return success();
      };
      for (ac::SourceUseOp use : body.getOps<ac::SourceUseOp>())
        if (failed(addContribution(use.getTarget(), use.getData(),
                                   use.getEnabled(), use.getValue(),
                                   use.getValid(), use.getPath(), use.getId(),
                                   use.getSource(), use, {})))
          return failure();
      for (ac::ValueUseOp use : body.getOps<ac::ValueUseOp>()) {
        auto yield = dyn_cast<ac::YieldOp>(body.getTerminator());
        DictionaryAttr target = requiredUseTarget(rule, use);
        if (!yield || yield.getValues().size() != 2 || !target ||
            failed(addContribution(target, yield.getValues()[0],
                                   yield.getValues()[1], use.getValue(),
                                   use.getValid(), use.getPath(), use.getId(),
                                   use.getSource(), {}, use)))
          return failure();
      }
    }
  }

  for (StateProposals &state : graph.states) {
    SmallVector<size_t> active;
    for (size_t index : state.contributions)
      if (effectiveGuard(graph.contributions[index]).kind != GuardKind::False)
        active.push_back(index);
    state.commit.contributions = active;
    if (active.empty()) {
      state.commit.kind = CommitPairKind::Hold;
      continue;
    }
    if (active.size() == 1) {
      const ProposalContribution &only = graph.contributions[active.front()];
      state.commit.kind = CommitPairKind::Forward;
      state.commit.data = only.data;
      state.commit.enable = only.enabled;
      continue;
    }
    bool exclusive = true;
    for (size_t left = 0; left < active.size(); ++left)
      for (size_t right = left + 1; right < active.size(); ++right) {
        ProposalConflict conflict;
        conflict.left = active[left];
        conflict.right = active[right];
        conflict.kind = classifyConflict(graph.contributions[active[left]],
                                         graph.contributions[active[right]]);
        exclusive &= conflict.kind == ProposalConflictKind::MutuallyExclusive;
        state.conflicts.push_back(conflict);
      }
    if (exclusive)
      state.commit.kind = CommitPairKind::ExclusiveMerge;
  }
  llvm::sort(graph.states,
             [](const StateProposals &left, const StateProposals &right) {
               return ac::detail::compareClosedSourceStructure(
                          left.stateID, right.stateID) < 0;
             });
  return graph;
}

LogicalResult verifySourceProposals(const ProposalGraph &graph,
                                    ac::detail::EmitError emitError) {
  if (!graph.modules || !graph.modules->root)
    return emitError() << "proposal graph requires a rooted ModuleGraph";

  bool hasSourceChecks = false;
  DenseSet<Operation *> checkedModules;
  for (InstanceView *view : graph.modules->views) {
    Operation *module = view ? view->module.getOperation() : nullptr;
    if (!module || !checkedModules.insert(module).second)
      continue;
    module->walk([&](Operation *operation) {
      StringRef name = operation->getName().getStringRef();
      hasSourceChecks |=
          name == "ac.expect" || operation->hasAttr("ac.check_id");
      if (auto values =
              operation->getAttrOfType<ArrayAttr>("ac.required_checks"))
        hasSourceChecks |= !values.empty();
    });
    for (ac::RuleOp rule : view->module.getBody().front().getOps<ac::RuleOp>())
      if (failed(verifySupportedNumericRule(rule, emitError)))
        return failure();
  }
  if (graph.checks) {
    if (graph.checks->modules != graph.modules ||
        failed(verifySourceChecks(*graph.checks, emitError)))
      return emitError() << "proposal graph has an invalid CheckGraph";
  } else if (hasSourceChecks) {
    return emitError()
           << "proposal graph requires a CheckGraph for source checks";
  }
  bool expectedPermitAlways = !graph.checks || graph.checks->bindings.empty();
  if (graph.globalPermitAlways != expectedPermitAlways)
    return emitError() << "proposal global permit is not derived from checks";

  DenseSet<const InstanceView *> owners;
  for (InstanceView *view : graph.modules->views)
    if (!view || !owners.insert(view).second)
      return emitError() << "proposal ModuleGraph contains a repeated view";

  DenseSet<Attribute> states;
  SmallVector<unsigned> membership(graph.contributions.size(), 0);
  std::set<std::pair<InstanceView *, Operation *>> admittedCompositionFacts;
  DenseSet<Attribute> admittedCompositionOutputs;
  DenseSet<Operation *> verifiedCompositionRules;
  for (const StateProposals &state : graph.states) {
    if (failed(ac::detail::verifyStateID(state.stateID, emitError)) ||
        failed(ac::detail::verifyLogicalTypeStructure(state.logicalType,
                                                      emitError)) ||
        !states.insert(state.stateID).second)
      return failure();
    DenseSet<size_t> local;
    for (size_t index : state.contributions) {
      if (index >= graph.contributions.size())
        return emitError() << "proposal contribution index is out of range";
      if (!local.insert(index).second)
        return emitError() << "StateID repeats one proposal contribution";
      ++membership[index];
      const ProposalContribution &contribution = graph.contributions[index];
      if (contribution.stateID != state.stateID ||
          contribution.logicalType != state.logicalType)
        return emitError()
               << "proposal contribution belongs to a different StateID";
    }
  }
  for (unsigned count : membership)
    if (count != 1)
      return emitError()
             << "every proposal contribution must belong to one StateID";

  for (const ProposalContribution &contribution : graph.contributions) {
    ac::RuleOp actualRule = contribution.rule;
    if (contribution.composition) {
      if (!contribution.owner || !owners.contains(contribution.owner) ||
          !actualRule || !ac::hasNumericCompositionContract(actualRule) ||
          (!verifiedCompositionRules.contains(actualRule) &&
           failed(ac::verifyNumericCompositionClosure(actualRule))) ||
          contribution.owner->placement != contribution.child ||
          actualRule->getParentOfType<ac::ModuleOp>() !=
              contribution.owner->module ||
          actualRule->getBlock() !=
              &contribution.owner->module.getBody().front() ||
          contribution.sourceFacts.empty())
        return emitError() << "compositional proposal lacks native closure or "
                              "row authority";
      verifiedCompositionRules.insert(actualRule);
      auto yield =
          dyn_cast<ac::YieldOp>(actualRule.getBody().front().getTerminator());
      auto bindings =
          actualRule->getAttrOfType<ArrayAttr>("ac.output_bindings");
      auto types = actualRule->getAttrOfType<ArrayAttr>("ac.output_types");
      std::optional<size_t> output;
      if (!yield || !bindings || !types ||
          bindings.size() != actualRule.getTargets().size() ||
          types.size() != actualRule.getTargets().size() ||
          yield.getValues().size() != 2 * actualRule.getTargets().size())
        return emitError() << "compositional proposal yield is not closed";
      for (auto [index, binding] : llvm::enumerate(bindings)) {
        auto resolved = dyn_cast<DictionaryAttr>(binding);
        auto id = resolved ? graph.modules->resolveState(*contribution.owner,
                                                         resolved, emitError)
                           : FailureOr<DictionaryAttr>(failure());
        if (succeeded(id) && *id == contribution.stateID) {
          if (output)
            return emitError()
                   << "compositional proposal matches repeated output StateID";
          output = index;
        }
      }
      if (!output)
        return emitError()
               << "compositional proposal StateID has no unique output";
      Builder identityBuilder(actualRule.getContext());
      auto identity = identityBuilder.getDictionaryAttr({
          identityBuilder.getNamedAttr("owner", contribution.owner->owner),
          identityBuilder.getNamedAttr("registration",
                                       actualRule.getRegistrationAttr()),
          identityBuilder.getNamedAttr("target", bindings[*output]),
      });
      if (!admittedCompositionOutputs.insert(identity).second ||
          contribution.logicalType != types[*output] ||
          contribution.data != yield.getValues()[2 * *output] ||
          contribution.enabled != yield.getValues()[2 * *output + 1] ||
          contribution.valid != (contribution.sourceFacts.size() == 1
                                     ? contribution.sourceFacts.front().valid
                                     : contribution.enabled) ||
          contribution.path != (contribution.sourceFacts.size() == 1
                                    ? contribution.sourceFacts.front().path
                                    : contribution.enabled))
        return emitError()
               << "compositional proposal differs from its unique output yield";
      if (contribution.sourceFacts.size() == 1) {
        const ProposalUseFact &fact = contribution.sourceFacts.front();
        if (contribution.useID != fact.useID ||
            contribution.sourceID != fact.sourceID ||
            contribution.value != fact.value || contribution.use != fact.use ||
            contribution.valueUse != fact.valueUse)
          return emitError()
                 << "singleton compositional proposal lost scalar row aliases";
      } else if (contribution.useID || contribution.sourceID ||
                 contribution.value || contribution.use ||
                 contribution.valueUse) {
        return emitError() << "multi-row compositional proposal retained "
                              "ambiguous scalar authority";
      }
      for (const ProposalUseFact &fact : contribution.sourceFacts) {
        ac::SourceUseOp sourceUse = fact.use;
        ac::ValueUseOp valueUse = fact.valueUse;
        Operation *operation =
            sourceUse ? sourceUse.getOperation() : valueUse.getOperation();
        if (!operation || (sourceUse && valueUse) ||
            operation->getParentOfType<ac::RuleOp>() != actualRule ||
            operation->getBlock() != &actualRule.getBody().front() ||
            !admittedCompositionFacts.emplace(contribution.owner, operation)
                 .second)
          return emitError() << "compositional proposal row is missing, "
                                "repeated or foreign";
        DictionaryAttr target =
            sourceUse ? sourceUse.getTarget()
                      : compositionUseTarget(actualRule, valueUse);
        auto kind = target.getAs<StringAttr>("kind");
        auto relative = target.getAs<DictionaryAttr>("state");
        auto resolved = relative ? graph.modules->resolveState(
                                       *contribution.owner, relative, emitError)
                                 : FailureOr<DictionaryAttr>(failure());
        Value value = sourceUse ? sourceUse.getValue() : valueUse.getValue();
        Value valid = sourceUse ? sourceUse.getValid() : valueUse.getValid();
        Value path = sourceUse ? sourceUse.getPath() : valueUse.getPath();
        DictionaryAttr useID =
            sourceUse ? sourceUse.getId() : valueUse.getIdAttr();
        DictionaryAttr sourceID =
            sourceUse ? sourceUse.getSource() : valueUse.getSourceAttr();
        if (!kind || kind.getValue() != "next_scalar" || !relative ||
            failed(resolved) || *resolved != contribution.stateID ||
            relative != bindings[*output] || fact.useID != useID ||
            fact.sourceID != sourceID || fact.value != value ||
            fact.valid != valid || fact.path != path ||
            failed(ac::detail::verifyUseID(fact.useID, emitError)) ||
            failed(ac::detail::verifyValueID(fact.sourceID, emitError)))
          return emitError() << "compositional proposal row differs from "
                                "actual source/value use";
      }
      continue;
    }
    ac::SourceUseOp actualUse = contribution.use;
    ac::ValueUseOp actualValueUse = contribution.valueUse;
    Operation *actualOperation =
        actualUse ? actualUse.getOperation() : actualValueUse.getOperation();
    if (!contribution.owner || !owners.contains(contribution.owner) ||
        !actualRule || !actualOperation || (actualUse && actualValueUse) ||
        contribution.child != contribution.owner->placement ||
        actualRule->getParentOfType<ac::ModuleOp>() !=
            contribution.owner->module ||
        actualRule->getBlock() !=
            &contribution.owner->module.getBody().front() ||
        actualOperation->getParentOfType<ac::RuleOp>() != actualRule ||
        actualOperation->getBlock() != &actualRule.getBody().front())
      return emitError()
             << "proposal contribution owner/rule/use relationship is forged";

    auto target = actualUse ? actualUse.getTarget()
                            : requiredUseTarget(actualRule, actualValueUse);
    auto kind = target.getAs<StringAttr>("kind");
    auto relative = target.getAs<DictionaryAttr>("state");
    auto resolved = [&]() -> FailureOr<DictionaryAttr> {
      if (!relative)
        return failure();
      return graph.modules->resolveState(*contribution.owner, relative,
                                         emitError);
    }();
    auto bindings = actualRule->getAttrOfType<ArrayAttr>("ac.output_bindings");
    auto types = actualRule->getAttrOfType<ArrayAttr>("ac.output_types");
    std::optional<size_t> output;
    if (bindings)
      for (auto [index, binding] : llvm::enumerate(bindings))
        if (binding == relative) {
          if (output)
            return emitError()
                   << "proposal target matches repeated rule outputs";
          output = index;
        }
    auto logical = output && types && *output < types.size()
                       ? dyn_cast<DictionaryAttr>(types[*output])
                       : DictionaryAttr();
    auto actualYield =
        dyn_cast<ac::YieldOp>(actualRule.getBody().front().getTerminator());
    if (actualValueUse && (!actualYield || actualYield.getValues().size() != 2))
      return emitError() << "U1 proposal has no exact finite yield";
    Value actualData =
        actualUse ? actualUse.getData() : actualYield.getValues()[0];
    Value actualEnabled =
        actualUse ? actualUse.getEnabled() : actualYield.getValues()[1];
    Value actualValue =
        actualUse ? actualUse.getValue() : actualValueUse.getValue();
    Value actualValid =
        actualUse ? actualUse.getValid() : actualValueUse.getValid();
    Value actualPath =
        actualUse ? actualUse.getPath() : actualValueUse.getPath();
    DictionaryAttr actualUseID =
        actualUse ? actualUse.getId() : actualValueUse.getId();
    DictionaryAttr actualSourceID =
        actualUse ? actualUse.getSource() : actualValueUse.getSource();
    if (!kind || kind.getValue() != "next_scalar" || !relative ||
        failed(resolved) || *resolved != contribution.stateID || !logical ||
        logical != contribution.logicalType ||
        contribution.useID != actualUseID ||
        contribution.sourceID != actualSourceID ||
        contribution.data != actualData ||
        contribution.enabled != actualEnabled ||
        contribution.value != actualValue ||
        contribution.valid != actualValid || contribution.path != actualPath ||
        failed(ac::detail::verifyUseID(contribution.useID, emitError)) ||
        failed(ac::detail::verifyValueID(contribution.sourceID, emitError)))
      return emitError() << "proposal contribution differs from its actual "
                            "source/value use";
  }

  size_t expectedCompositionFacts = 0;
  for (InstanceView *view : graph.modules->views) {
    if (!view)
      continue;
    for (ac::RuleOp rule :
         view->module.getBody().front().getOps<ac::RuleOp>()) {
      if (!ac::hasNumericCompositionContract(rule))
        continue;
      if (failed(ac::verifyNumericCompositionClosure(rule)))
        return emitError()
               << "proposal graph contains an invalid numeric composition rule";
      expectedCompositionFacts += std::distance(
          rule.getBody().front().getOps<ac::SourceUseOp>().begin(),
          rule.getBody().front().getOps<ac::SourceUseOp>().end());
      expectedCompositionFacts +=
          std::distance(rule.getBody().front().getOps<ac::ValueUseOp>().begin(),
                        rule.getBody().front().getOps<ac::ValueUseOp>().end());
    }
  }
  if (admittedCompositionFacts.size() != expectedCompositionFacts)
    return emitError()
           << "proposal graph does not retain every compositional source row";

  for (const StateProposals &state : graph.states) {
    std::set<size_t> stateMembers(state.contributions.begin(),
                                  state.contributions.end());
    std::set<size_t> active;
    for (size_t index : state.contributions)
      if (effectiveGuard(graph.contributions[index]).kind != GuardKind::False)
        active.insert(index);

    std::set<std::pair<size_t, size_t>> expectedPairs;
    for (auto left = active.begin(); left != active.end(); ++left)
      for (auto right = std::next(left); right != active.end(); ++right)
        expectedPairs.emplace(*left, *right);
    std::set<std::pair<size_t, size_t>> actualPairs;
    for (const ProposalConflict &conflict : state.conflicts) {
      if (conflict.left >= graph.contributions.size() ||
          conflict.right >= graph.contributions.size() ||
          conflict.left == conflict.right || !active.contains(conflict.left) ||
          !active.contains(conflict.right))
        return emitError()
               << "proposal conflict references an invalid or foreign writer";
      size_t left = std::min(conflict.left, conflict.right);
      size_t right = std::max(conflict.left, conflict.right);
      if (!actualPairs.emplace(left, right).second ||
          conflict.kind != classifyConflict(graph.contributions[left],
                                            graph.contributions[right]))
        return emitError()
               << "proposal conflict proof does not match actual guards";
    }
    if (actualPairs != expectedPairs)
      return emitError()
             << "proposal conflicts do not cover every active writer pair";
    for (const ProposalConflict &conflict : state.conflicts)
      if (conflict.kind != ProposalConflictKind::MutuallyExclusive)
        return emitError()
               << (conflict.kind == ProposalConflictKind::GuaranteedOverlap
                       ? "StateID has necessarily overlapping writers"
                       : "StateID needs an unavailable precommit overlap "
                         "check");

    if (state.commit.permitAlways != expectedPermitAlways ||
        state.commit.stateID != state.stateID)
      return emitError() << "proposal commit permit is not derived from checks";
    std::set<size_t> committed;
    for (size_t index : state.commit.contributions) {
      if (index >= graph.contributions.size() ||
          !stateMembers.contains(index) || !active.contains(index) ||
          !committed.insert(index).second)
        return emitError()
               << "commit references an invalid, inactive or foreign writer";
    }
    if (committed != active)
      return emitError() << "commit contributors differ from active writers";

    if (state.commit.kind == CommitPairKind::Forward) {
      if (active.size() != 1)
        return emitError() << "forward commit requires one active contribution";
      size_t index = *active.begin();
      const ProposalContribution &only = graph.contributions[index];
      if (state.commit.data != only.data || state.commit.enable != only.enabled)
        return emitError() << "forward commit does not preserve actual D/E";
    } else if (state.commit.kind == CommitPairKind::ExclusiveMerge) {
      if (active.size() < 2 || state.commit.data || state.commit.enable)
        return emitError()
               << "exclusive merge must remain a complete private recipe";
      for (const ProposalConflict &conflict : state.conflicts)
        if (conflict.kind != ProposalConflictKind::MutuallyExclusive)
          return emitError()
                 << "exclusive merge contains a non-exclusive writer pair";
    } else if (!active.empty() || state.commit.data || state.commit.enable) {
      return emitError() << "hold commit contains an active writer";
    }
  }
  return success();
}

} // namespace acir::compiler
