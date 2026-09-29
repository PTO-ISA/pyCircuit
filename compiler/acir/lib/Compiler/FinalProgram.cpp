#include "FinalProgram.h"

#include "Dialect/ACIR/ACIRFinalUses.h"
#include "Dialect/ACIR/ACIRHardwareClosure.h"
#include "Dialect/ACIR/ACIRNumericNextUse.h"
#include "Dialect/ACIR/ACIRStaticEvaluation.h"
#include "FinalHardware.h"
#include "FinalHardwareProgram.h"
#include "FinalUses.h"

#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/Verifier.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/Support/raw_ostream.h"

using namespace mlir;

namespace acir::compiler {
LogicalResult
FinalProgram::verifyOwnedSourceClosure(ac::detail::EmitError emitError) const {
  if (units_.empty())
    return emitError() << "final program requires complete source link units";
  if (!registry_ || !modules_ || !checks_ || !proposals_ || !observations_)
    return emitError() << "final program is missing an owned analysis graph";
  if (!modules_->root || checks_->modules != modules_.get() ||
      proposals_->modules != modules_.get() ||
      proposals_->checks != checks_.get() ||
      observations_->modules != modules_.get())
    return emitError() << "final program graph pointers are not closed";

  ArrayRef<ModuleOp> supplied = registry_->suppliedHeaders();
  if (supplied.size() != units_.size())
    return emitError() << "final program authority registry is incomplete";
  for (auto [index, unit] : llvm::enumerate(units_)) {
    if (!unit.body || !unit.header || supplied[index] != unit.header)
      return emitError() << "final program source authority changed at unit["
                         << index << ']';
    auto bodyStage = unit.body->getAttrOfType<StringAttr>("ac.stage");
    auto headerStage = unit.header->getAttrOfType<StringAttr>("ac.stage");
    if (!bodyStage || bodyStage.getValue() != "source" || !headerStage ||
        headerStage.getValue() != "source" || failed(verify(unit.body)) ||
        failed(verify(unit.header)) ||
        failed(
            registry_->verifyBodySnapshots(unit.body, unit.header, emitError)))
      return emitError()
             << "final program source unit is no longer admitted at "
                "unit["
             << index << ']';
  }

  auto admitted = admitSourceLinkUnits(units_, emitError);
  if (failed(admitted))
    return emitError() << "final program source authority no longer closes";

  DenseSet<const InstanceView *> views;
  DenseSet<Attribute> ownerRefs;
  bool containsRoot = false;
  for (InstanceView *view : modules_->views) {
    if (!view || !views.insert(view).second || !view->module || !view->header ||
        !ownerRefs.insert(view->owner).second ||
        failed(ac::detail::verifyOwnerRef(view->owner, emitError)))
      return emitError() << "final program ModuleGraph has an invalid owner";
    containsRoot |= view == modules_->root;
    if (view->module->hasAttr("ac.stage") || view->header->hasAttr("ac.stage"))
      return emitError() << "final program graph operation spoofs a stage";
    bool ownsModule = false;
    bool ownsHeader = false;
    for (SourceLinkUnit unit : units_) {
      ownsModule |=
          view->module->getParentOfType<mlir::ModuleOp>() == unit.body;
      ownsHeader |=
          view->header->getParentOfType<mlir::ModuleOp>() == unit.header;
    }
    if (!ownsModule || !ownsHeader)
      return emitError()
             << "final program graph references a foreign source authority";
  }
  if (!containsRoot)
    return emitError() << "final program root is outside its ModuleGraph";
  return success();
}

struct FrozenRegControls {
  Value clock;
  Value reset;
  SmallVector<Value, 2> prefix;
  SmallVector<Type, 2> prefixTypes;
  Block *prefixBlock = nullptr;
  DictionaryAttr descriptor;
  StringAttr domain;
};

namespace {

bool containsSourceAttribute(Attribute attribute) {
  std::string text;
  llvm::raw_string_ostream(text) << attribute;
  return text.find("#ac.static_expr") != std::string::npos;
}

LogicalResult preflightMaterialization(const FinalProgram &program,
                                       ac::detail::EmitError emitError) {
  DenseSet<Operation *> definitions;
  for (InstanceView *view : program.modules().views) {
    if (!view || !view->module || !definitions.insert(view->module).second)
      continue;
    LogicalResult result = success();
    view->module->walk([&](Operation *operation) {
      if (failed(result))
        return;
      StringRef name = operation->getName().getStringRef();
      if (isa<ac::StructOp, mlir::func::FuncOp>(operation) ||
          name.starts_with("ac.math.") ||
          operation->hasAttr("ac.check_template")) {
        result = emitError()
                 << "final program contains an unsupported record, helper, "
                    "math or numeric-proof carrier";
        return;
      }
      for (Type type : operation->getOperandTypes())
        if (isa<ac::MathIntType>(type))
          result = emitError() << "final program contains runtime MathInt";
      for (Type type : operation->getResultTypes())
        if (isa<ac::MathIntType>(type))
          result = emitError() << "final program contains runtime MathInt";
    });
    if (failed(result))
      return failure();
    for (ac::RuleOp rule : view->module.getBody().front().getOps<ac::RuleOp>())
      if (!isSupportedFinalNumericRule(rule))
        return emitError()
               << "final program supports only verified U1 numeric rules";
    for (ac::RegOp reg : view->module.getBody().front().getOps<ac::RegOp>()) {
      auto shape = reg->getAttrOfType<ArrayAttr>("ac.shape");
      auto physical = dyn_cast<ac::RegType>(reg.getState().getType());
      auto element = physical ? dyn_cast<IntegerType>(physical.getElementType())
                              : IntegerType();
      if (!shape || !shape.empty() || !element || element.getWidth() > 64)
        return emitError() << "final program materializes scalar integer state "
                              "up to 64 bits";
    }
  }
  for (const StateProposals &state : program.proposals().states)
    if (state.commit.kind == CommitPairKind::ExclusiveMerge)
      return emitError()
             << "final program does not materialize ExclusiveMerge yet";
  return success();
}

LogicalResult preflightUnitCarriers(ArrayRef<SourceLinkUnit> units,
                                    ac::detail::EmitError emitError) {
  LogicalResult result = success();
  auto inspect = [&](ModuleOp unit) {
    unit->walk([&](Operation *operation) {
      if (failed(result))
        return;
      StringRef name = operation->getName().getStringRef();
      if (isa<ac::StructOp, mlir::func::FuncOp>(operation) ||
          name.starts_with("ac.math.") ||
          operation->hasAttr("ac.check_template")) {
        result = emitError()
                 << "final program cannot materialize record, helper, math or "
                    "numeric-proof source units";
        return;
      }
      for (Type type : operation->getOperandTypes())
        if (isa<ac::MathIntType>(type))
          result = emitError() << "final program contains runtime MathInt";
      for (Type type : operation->getResultTypes())
        if (isa<ac::MathIntType>(type))
          result = emitError() << "final program contains runtime MathInt";
    });
  };
  for (SourceLinkUnit unit : units) {
    inspect(unit.body);
    inspect(unit.header);
    unit.body->walk([&](ac::RuleOp rule) {
      if (succeeded(result) && !isSupportedFinalNumericRule(rule))
        result = emitError()
                 << "final source unit has an unsupported numeric rule";
    });
  }
  return result;
}

LogicalResult verifyProposalStorage(const ModuleGraph &modules,
                                    const ProposalGraph &proposals,
                                    ac::detail::EmitError emitError) {
  DenseMap<Attribute, std::pair<InstanceView *, ac::RegOp>> storage;
  DenseSet<const InstanceView *> validViews;
  for (InstanceView *view : modules.views) {
    if (!view || !view->module || view->module.getBody().empty())
      return emitError() << "final program has an incomplete state owner";
    validViews.insert(view);
    auto regs = view->module.getBody().front().getOps<ac::RegOp>();
    size_t index = 0;
    for (ac::RegOp reg : regs) {
      if (index >= view->ownedStateIDs.size())
        return emitError() << "final program owned state map does not match "
                              "ac.reg declarations";
      Attribute id = view->ownedStateIDs[index++];
      if (!storage.try_emplace(id, view, reg).second)
        return emitError() << "final program has duplicate owned state storage";
    }
    if (index != view->ownedStateIDs.size())
      return emitError() << "final program owned state map does not match "
                            "ac.reg declarations";
  }
  if (!modules.root)
    return emitError() << "final program has no selected system root";
  for (InstanceView *view : modules.views)
    for (const auto &alias : view->formalAliases)
      if (!storage.contains(alias.second))
        return view == modules.root
                   ? emitError() << "selected root has an unbound data formal "
                                    "or synthetic root StateID"
                   : emitError() << "formal state alias does not resolve to "
                                    "owned storage";
  for (const StateProposals &state : proposals.states) {
    auto found = storage.find(state.stateID);
    if (found == storage.end())
      return emitError()
             << "ProposalGraph StateID has no owned ac.reg declaration";
    auto owner = state.stateID.getAs<DictionaryAttr>("owner");
    if (!owner || owner != found->second.first->owner)
      return emitError()
             << "ProposalGraph StateID owner does not match owned storage";
    if (state.commit.stateID != state.stateID)
      return emitError()
             << "final program commit owner does not match proposal StateID";
    for (size_t contributionIndex : state.contributions) {
      if (contributionIndex >= proposals.contributions.size())
        return emitError()
               << "final program proposal contribution index is invalid";
      const ProposalContribution &contribution =
          proposals.contributions[contributionIndex];
      if (contribution.stateID != state.stateID || !contribution.owner ||
          !validViews.contains(contribution.owner))
        return emitError() << "final program proposal contribution target or "
                              "instance owner is invalid";
    }
  }
  return success();
}

FailureOr<Attribute> materializeInitial(ac::RegOp reg,
                                        ac::detail::EmitError emitError) {
  auto initial = reg->getAttrOfType<DictionaryAttr>("ac.initial_value");
  auto expression =
      initial && initial.getAs<StringAttr>("kind") &&
              initial.getAs<StringAttr>("kind").getValue() == "scalar"
          ? initial.getAs<ac::StaticExprAttr>("value")
          : ac::StaticExprAttr();
  if (!expression)
    return emitError() << "scalar state has no materializable initializer";
  auto evaluated =
      ac::detail::evaluateSourceStaticExpr(expression, reg, emitError);
  if (failed(evaluated))
    return failure();
  auto kind = evaluated->getAs<StringAttr>("kind");
  if (!kind)
    return emitError() << "evaluated initializer has no scalar kind";
  if (kind.getValue() == "bool") {
    auto value = evaluated->getAs<BoolAttr>("value");
    if (!value)
      return emitError() << "evaluated bool initializer is malformed";
    return Attribute(value);
  }
  if (kind.getValue() != "integer")
    return emitError() << "record/list initializer is not materialized";
  auto value = evaluated->getAs<ac::MathIntAttr>("value");
  auto physical = cast<ac::RegType>(reg.getState().getType()).getElementType();
  auto integer = dyn_cast<IntegerType>(physical);
  if (!value || !integer || integer.getWidth() > 64)
    return emitError() << "integer initializer exceeds bounded materialization";
  llvm::APSInt number(value.getCanonicalValue());
  if (number.isNegative() || number.getActiveBits() > integer.getWidth())
    return emitError() << "integer initializer does not fit its physical state";
  number = number.extOrTrunc(integer.getWidth());
  number.setIsUnsigned(true);
  return Attribute(IntegerAttr::get(integer, number));
}

DictionaryAttr formalState(Builder &builder, DictionaryAttr port) {
  return builder.getDictionaryAttr({
      builder.getNamedAttr("kind", builder.getStringAttr("formal")),
      builder.getNamedAttr("parameter", port.get("parameter")),
      builder.getNamedAttr("ordinal", port.get("ordinal")),
  });
}

FailureOr<FinalProgram::StateCarrierSnapshot>
freezeStateCarrier(const FinalProgram &program, DictionaryAttr stateID,
                   const DenseMap<Operation *, FrozenRegControls> &controls,
                   ac::detail::EmitError emitError) {
  size_t owners = 0;
  FinalProgram::StateCarrierSnapshot frozen;
  for (InstanceView *view : program.modules().views) {
    if (!view || !view->module)
      continue;
    auto regs = view->module.getBody().front().getOps<ac::RegOp>();
    size_t ownedIndex = 0;
    for (ac::RegOp reg : regs) {
      if (ownedIndex >= view->ownedStateIDs.size())
        return emitError() << "final program has an incomplete owned state map";
      DictionaryAttr ownedID = view->ownedStateIDs[ownedIndex++];
      if (ownedID != stateID)
        continue;
      ++owners;
      Value handle = reg.getState();
      auto physical = dyn_cast<ac::RegType>(handle.getType());
      auto integer = physical ? dyn_cast<IntegerType>(physical.getElementType())
                              : IntegerType();
      if (!integer || integer.getWidth() == 0 || integer.getWidth() > 64)
        return emitError()
               << "final program state carrier is not a bounded scalar integer";
      Attribute initializer = reg->getAttr("ac.initial_value");
      if (!initializer)
        initializer = IntegerAttr::get(integer, APInt(integer.getWidth(), 0));
      auto control = controls.find(reg.getOperation());
      if (control == controls.end())
        return emitError() << "final owned state has no frozen module controls";
      frozen.stateID = stateID;
      frozen.view = view;
      frozen.owner = view->owner;
      frozen.handle = handle;
      frozen.declaration = reg.getOperation();
      frozen.attributes =
          Builder(reg.getContext()).getDictionaryAttr(reg->getAttrs());
      frozen.physicalType = handle.getType();
      frozen.width = integer.getWidth();
      frozen.initializer = initializer;
      frozen.clock = control->second.clock;
      frozen.reset = control->second.reset;
      frozen.controlPrefix.append(control->second.prefix.begin(),
                                  control->second.prefix.end());
      frozen.controlPrefixTypes.append(control->second.prefixTypes.begin(),
                                       control->second.prefixTypes.end());
      frozen.controlPrefixBlock = control->second.prefixBlock;
      frozen.controlDescriptor = control->second.descriptor;
      frozen.domain = control->second.domain;
      break;
    }
  }
  if (owners != 1 || !frozen.declaration)
    return emitError() << "final program StateID must have exactly one owned "
                          "ac.reg storage carrier";
  return frozen;
}

FailureOr<SmallVector<FinalProgram::FinalInstanceSnapshot, 0>>
freezeSystemPlan(const FinalProgram &program, size_t &rootOrdinal,
                 ac::detail::EmitError emitError) {
  SmallVector<FinalProgram::FinalInstanceSnapshot, 0> plan;
  DenseMap<const InstanceView *, size_t> ordinal;
  for (auto [index, view] : llvm::enumerate(program.modules().views)) {
    if (!view || !ordinal.try_emplace(view, index).second)
      return emitError() << "final system plan has duplicate or null instances";
  }
  auto root = ordinal.find(program.modules().root);
  if (root == ordinal.end())
    return emitError() << "final system plan root has no unique ordinal";
  rootOrdinal = root->second;
  plan.resize(program.modules().views.size());
  DenseMap<const InstanceView *, size_t> childOccurrences;
  for (auto [index, view] : llvm::enumerate(program.modules().views)) {
    auto &entry = plan[index];
    entry.ordinal = index;
    entry.view = view;
    entry.owner = view->owner;
    entry.definition = view->definition;
    entry.staticArguments = view->staticArguments;
    entry.module = view->module;
    entry.placement = view->placement;
    if (view->parent) {
      auto parent = ordinal.find(view->parent);
      if (parent == ordinal.end())
        return emitError()
               << "final system plan parent is outside the instance set";
      entry.parentOrdinal = parent->second;
    }
    for (InstanceView *child : view->children) {
      auto found = ordinal.find(child);
      if (found == ordinal.end())
        return emitError()
               << "final system plan child is outside the instance set";
      if (child->parent != view ||
          !childOccurrences.try_emplace(child, 1).second)
        return emitError()
               << "final system plan parent/child links are inconsistent";
      entry.childOrdinals.push_back(found->second);
    }
    for (DictionaryAttr stateID : view->ownedStateIDs) {
      auto found = llvm::find_if(program.proposals().states,
                                 [&](const StateProposals &state) {
                                   return state.stateID == stateID;
                                 });
      if (found == program.proposals().states.end())
        continue;
      if (stateID.getAs<DictionaryAttr>("owner") != view->owner)
        return emitError() << "final system plan owned StateID has the wrong "
                              "instance owner";
      entry.ownedStateOrdinals.push_back(
          static_cast<size_t>(found - program.proposals().states.begin()));
    }
    for (auto [indexInModule, rule] :
         llvm::enumerate(view->module.getBody().front().getOps<ac::RuleOp>())) {
      (void)rule;
      entry.ruleOrdinals.push_back(indexInModule);
    }
  }
  for (InstanceView *view : program.modules().views)
    if ((view == program.modules().root && view->parent) ||
        (view != program.modules().root &&
         (!view->parent || childOccurrences.lookup(view) != 1)))
      return emitError() << "final system plan has a missing or invalid parent";
  for (auto [index, contribution] :
       llvm::enumerate(program.proposals().contributions)) {
    auto found = ordinal.find(contribution.owner);
    if (found == ordinal.end())
      return emitError() << "final system plan proposal has no instance owner";
    plan[found->second].proposalContributionOrdinals.push_back(index);
  }
  for (auto [index, state] : llvm::enumerate(program.proposals().states)) {
    auto owner = state.stateID.getAs<DictionaryAttr>("owner");
    auto path = owner ? owner.getAs<ArrayAttr>("instance_path") : ArrayAttr();
    InstanceView *matched = nullptr;
    for (InstanceView *view : program.modules().views)
      if (view && view->owner && view->owner.get("instance_path") == path) {
        if (matched)
          return emitError() << "final system plan StateID has ambiguous owner";
        matched = view;
      }
    auto found = ordinal.find(matched);
    if (found == ordinal.end())
      return emitError()
             << "final system plan StateID owner is outside the instance set";
    plan[found->second].proposalStateOrdinals.push_back(index);
  }
  for (auto [index, check] : llvm::enumerate(program.checks().bindings)) {
    auto found = ordinal.find(check.owner);
    if (found == ordinal.end())
      return emitError() << "final system plan check has no instance owner";
    plan[found->second].checkOrdinals.push_back(index);
  }
  for (auto [index, observation] :
       llvm::enumerate(program.observations().bindings)) {
    auto found = ordinal.find(observation.owner);
    if (found == ordinal.end())
      return emitError()
             << "final system plan observation has no instance owner";
    plan[found->second].observationOrdinals.push_back(index);
  }
  return plan;
}

LogicalResult
verifyFrozenStateCarrier(const FinalProgram::StateCarrierSnapshot &snapshot,
                         ac::detail::EmitError emitError) {
  InstanceView *view = snapshot.view;
  if (!snapshot.stateID || !view || !view->module ||
      view->owner != snapshot.owner)
    return emitError() << "EmitReady state carrier owner closure changed";
  if (!snapshot.formalPort) {
    auto reg = dyn_cast_or_null<ac::RegOp>(snapshot.declaration);
    if (!reg || reg.getState() != snapshot.handle ||
        snapshot.handle.getType() != snapshot.physicalType ||
        Builder(reg.getContext()).getDictionaryAttr(reg->getAttrs()) !=
            snapshot.attributes ||
        reg->getAttr("ac.initial_value") != snapshot.initializer)
      return emitError() << "EmitReady owned state carrier facts changed";
    if (snapshot.controlPrefix.size() != 2 ||
        snapshot.controlPrefixTypes.size() != 2 ||
        !snapshot.controlPrefixBlock ||
        view->module.getBody().getBlocks().size() != 1 ||
        &view->module.getBody().front() != snapshot.controlPrefixBlock ||
        snapshot.controlPrefixBlock->getNumArguments() < 2 ||
        snapshot.controlPrefixBlock->getArgument(0) !=
            snapshot.controlPrefix[0] ||
        snapshot.controlPrefixBlock->getArgument(1) !=
            snapshot.controlPrefix[1] ||
        snapshot.controlPrefixBlock->getArgument(0).getType() !=
            snapshot.controlPrefixTypes[0] ||
        snapshot.controlPrefixBlock->getArgument(1).getType() !=
            snapshot.controlPrefixTypes[1] ||
        !snapshot.controlPrefixTypes[0].isInteger(1) ||
        !snapshot.controlPrefixTypes[1].isInteger(1) ||
        snapshot.controlPrefix[0] != snapshot.clock ||
        snapshot.controlPrefix[1] != snapshot.reset ||
        reg.getClock() != snapshot.clock || reg.getReset() != snapshot.reset ||
        !reg.getClock().getType().isInteger(1) ||
        !reg.getReset().getType().isInteger(1) ||
        view->module->getAttr("ac.control_ports") !=
            snapshot.controlDescriptor ||
        reg->getAttr("ac.domain") != snapshot.domain || !snapshot.domain ||
        snapshot.domain.getValue() != "default")
      return emitError() << "EmitReady owned state control prefix changed";
    size_t ownedIndex = 0;
    bool foundState = false;
    for (ac::RegOp candidate :
         view->module.getBody().front().getOps<ac::RegOp>()) {
      if (ownedIndex >= view->ownedStateIDs.size())
        return emitError() << "EmitReady owned state map is incomplete";
      foundState |= candidate == reg &&
                    view->ownedStateIDs[ownedIndex] == snapshot.stateID;
      ++ownedIndex;
    }
    if (!foundState)
      return emitError() << "EmitReady owned StateID binding changed";
  } else
    return emitError() << "EmitReady StateID carrier cannot be a formal handle";
  auto physical = dyn_cast<ac::RegType>(snapshot.handle.getType());
  auto integer = physical ? dyn_cast<IntegerType>(physical.getElementType())
                          : IntegerType();
  if (!integer || snapshot.handle.getType() != snapshot.physicalType ||
      integer.getWidth() != snapshot.width || !snapshot.initializer ||
      !isa<IntegerAttr>(snapshot.initializer))
    return emitError()
           << "EmitReady state physical type or initializer is invalid";
  return success();
}

FailureOr<SmallVector<FinalProgram::StateAliasSnapshot>>
freezeStateAliases(const FinalProgram &program,
                   ac::detail::EmitError emitError) {
  SmallVector<FinalProgram::StateAliasSnapshot> aliases;
  for (InstanceView *view : program.modules().views) {
    if (!view || !view->module || view->module.getBody().empty())
      return emitError()
             << "final program cannot freeze an incomplete state alias view";
    size_t ownedIndex = 0;
    for (ac::RegOp reg : view->module.getBody().front().getOps<ac::RegOp>()) {
      if (ownedIndex >= view->ownedStateIDs.size())
        return emitError()
               << "final program has an incomplete owned state alias map";
      aliases.push_back(
          {view, reg.getState(), view->ownedStateIDs[ownedIndex++], {}});
    }
    if (ownedIndex != view->ownedStateIDs.size())
      return emitError() << "final program has excess owned state aliases";

    auto ports = view->module->getAttrOfType<ArrayAttr>("ac.ports");
    Block &body = view->module.getBody().front();
    if (!ports || ports.size() + 2 != body.getNumArguments())
      return emitError()
             << "final program has an incomplete formal state alias map";
    Builder builder(view->module.getContext());
    for (auto [index, rawPort] : llvm::enumerate(ports)) {
      auto port = dyn_cast<DictionaryAttr>(rawPort);
      if (!port)
        return emitError()
               << "final program has a malformed formal state alias";
      DictionaryAttr formal = formalState(builder, port);
      auto found = view->formalAliases.find(formal);
      if (found == view->formalAliases.end() || !found->second)
        return emitError()
               << "final program formal state handle has no StateID alias";
      aliases.push_back({view,
                         body.getArgument(static_cast<unsigned>(index + 2)),
                         found->second, formal});
    }
  }
  return aliases;
}

LogicalResult
verifyFrozenStateAliases(const FinalProgram &program,
                         ArrayRef<FinalProgram::StateAliasSnapshot> frozen,
                         ac::detail::EmitError emitError) {
  auto current = freezeStateAliases(program, emitError);
  if (failed(current))
    return failure();
  if (current->size() != frozen.size())
    return emitError() << "EmitReady state alias cardinality changed";
  for (auto [index, expected] : llvm::enumerate(frozen)) {
    const auto &actual = (*current)[index];
    if (actual.view != expected.view || actual.handle != expected.handle ||
        actual.stateID != expected.stateID ||
        actual.formalState != expected.formalState)
      return emitError() << "EmitReady state handle alias changed";
  }
  return success();
}

DictionaryAttr operationAttributes(Operation *operation) {
  Builder builder(operation->getContext());
  return builder.getDictionaryAttr(operation->getAttrs());
}

FinalProgram::OperationSnapshot
freezeOperation(Operation *operation, InstanceView *owner, ac::RuleOp rule) {
  FinalProgram::OperationSnapshot snapshot;
  snapshot.operation = operation;
  snapshot.owner = owner;
  snapshot.rule = rule;
  snapshot.name = operation->getName().getStringRef().str();
  snapshot.attributes = operationAttributes(operation);
  snapshot.operands.append(operation->operand_begin(),
                           operation->operand_end());
  snapshot.resultTypes.append(operation->getResultTypes().begin(),
                              operation->getResultTypes().end());
  snapshot.parent = operation->getParentOp();
  snapshot.block = operation->getBlock();
  return snapshot;
}

bool sameOperation(const FinalProgram::OperationSnapshot &expected) {
  Operation *operation = expected.operation;
  if (!operation || operation->getName().getStringRef() != expected.name ||
      operationAttributes(operation) != expected.attributes ||
      operation->getParentOp() != expected.parent ||
      operation->getNumOperands() != expected.operands.size() ||
      operation->getNumResults() != expected.resultTypes.size())
    return false;
  for (auto [index, operand] : llvm::enumerate(operation->getOperands()))
    if (operand != expected.operands[index])
      return false;
  for (auto [index, type] : llvm::enumerate(operation->getResultTypes()))
    if (type != expected.resultTypes[index])
      return false;
  return true;
}

LogicalResult
freezeExpression(Value value, InstanceView *owner, ac::RuleOp rule,
                 SmallVectorImpl<FinalProgram::OperationSnapshot> &out,
                 DenseSet<Operation *> &visited,
                 ac::detail::EmitError emitError) {
  if (!owner || !owner->module || !rule)
    return emitError() << "final expression has no rule or module owner";
  if (!value)
    return emitError() << "final expression contains an empty value";
  if (auto argument = dyn_cast<BlockArgument>(value)) {
    if (!rule || argument.getOwner() != &rule.getBody().front() ||
        argument.getArgNumber() >= rule.getInputs().size())
      return emitError()
             << "final expression block argument is not a frozen rule input";
    return success();
  }
  Operation *definition = value.getDefiningOp();
  if (!definition)
    return emitError() << "final expression has no supported definition";
  StringRef name = definition->getName().getStringRef();
  bool supported = name == "arith.constant" || name == "arith.andi" ||
                   name == "arith.xori" || name == "arith.extui" ||
                   name == "arith.addi" || name == "arith.ori" ||
                   name == "arith.cmpi" || name == "arith.trunci" ||
                   name == "arith.select";
  if (!supported)
    return emitError() << "final expression contains unsupported operation "
                       << name;
  if (auto add = dyn_cast<arith::AddIOp>(definition)) {
    auto type = dyn_cast<IntegerType>(add.getType());
    if (!type || type.getWidth() == 0 || type.getWidth() > 64 ||
        add.getOverflowFlags() != arith::IntegerOverflowFlags::none)
      return emitError()
             << "final expression addition must be flag-free i1..i64";
  }
  if (auto extension = dyn_cast<arith::ExtUIOp>(definition)) {
    auto input = dyn_cast<IntegerType>(extension.getIn().getType());
    auto result = dyn_cast<IntegerType>(extension.getType());
    if (!input || !result || input.getWidth() == 0 ||
        input.getWidth() >= result.getWidth() || result.getWidth() > 64)
      return emitError()
             << "final expression unsigned extension must widen within i64";
  }
  if (!visited.insert(definition).second)
    return success();
  if (definition->getParentOfType<ac::ModuleOp>() != owner->module ||
      (rule && definition->getParentOfType<ac::RuleOp>() != rule))
    return emitError()
           << "final expression operation has foreign rule or owner";
  out.push_back(freezeOperation(definition, owner, rule));
  for (Value operand : definition->getOperands())
    if (failed(freezeExpression(operand, owner, rule, out, visited, emitError)))
      return failure();
  return success();
}

Value resolveReplacement(Value value, const DenseMap<Value, Value> &replaced) {
  DenseSet<Value> seen;
  while (value && seen.insert(value).second) {
    auto found = replaced.find(value);
    if (found == replaced.end())
      break;
    value = found->second;
  }
  return value;
}

bool hasAnyFinalResidual(const FinalProgram &program) {
  DenseSet<Operation *> roots;
  bool residual = false;
  for (InstanceView *view : program.modules().views) {
    Operation *root =
        view && view->module
            ? view->module->getParentOfType<mlir::ModuleOp>().getOperation()
            : nullptr;
    if (!root || !roots.insert(root).second)
      continue;
    root->walk([&](Operation *operation) {
      bool retainedNumeric =
          isRetainedFinalNumericCarrier(program.numericRules(), operation);
      residual |= isa<ac::SourceReadOp, ac::SourceUseOp, ac::ModuleImportOp,
                      mlir::func::FuncOp>(operation);
      if (!retainedNumeric)
        for (NamedAttribute attribute : operation->getAttrs())
          residual |= containsSourceAttribute(attribute.getValue());
    });
  }
  return residual;
}

bool hasResidualSourceSemantics(const FinalProgram &program) {
  bool residual = false;
  DenseSet<Operation *> bodies;
  for (InstanceView *view : program.modules().views) {
    Operation *body =
        view ? view->module->getParentOfType<mlir::ModuleOp>().getOperation()
             : nullptr;
    if (!body || !bodies.insert(body).second)
      continue;
    body->walk([&](Operation *operation) {
      residual |= isa<ac::SourceReadOp, ac::SourceUseOp, ac::SourceExpectOp,
                      ac::SourceObserveOp>(operation);
    });
  }
  return residual;
}

bool isZeroRuleSourceClosure(const FinalProgram &program) {
  if (!program.modules().root || program.modules().views.empty() ||
      !program.checks().bindings.empty() ||
      !program.proposals().contributions.empty() ||
      !program.observations().bindings.empty())
    return false;

  for (InstanceView *view : program.modules().views) {
    if (!view || !view->module ||
        view->module.getBody().getBlocks().size() != 1)
      return false;
    for (ac::RuleOp rule :
         view->module.getBody().front().getOps<ac::RuleOp>()) {
      auto outputs = rule->getAttrOfType<ArrayAttr>("ac.output_bindings");
      auto outputTypes = rule->getAttrOfType<ArrayAttr>("ac.output_types");
      auto yield =
          rule.getBody().getBlocks().size() == 1
              ? dyn_cast<ac::YieldOp>(rule.getBody().front().getTerminator())
              : ac::YieldOp();
      if (!outputs || !outputs.empty() || !outputTypes ||
          !outputTypes.empty() || !rule.getTargets().empty() ||
          rule->getNumResults() != 0 || !yield || !yield.getValues().empty() ||
          std::distance(rule.getBody().front().begin(),
                        rule.getBody().front().end()) != 1)
        return false;
    }
  }
  for (const StateProposals &state : program.proposals().states)
    if (!state.contributions.empty() ||
        state.commit.kind != CommitPairKind::Hold || state.commit.data ||
        state.commit.enable || !state.commit.contributions.empty() ||
        !state.commit.permitAlways)
      return false;
  return program.proposals().globalPermitAlways;
}

FailureOr<DenseMap<Operation *, FrozenRegControls>>
collectFinalRegControls(const FinalProgram &program,
                        ac::detail::EmitError emitError) {
  DenseMap<Operation *, FrozenRegControls> result;
  DenseSet<Operation *> definitions;
  for (InstanceView *view : program.modules().views) {
    if (!view || !view->module || !definitions.insert(view->module).second)
      continue;
    Block &body = view->module.getBody().front();
    auto descriptor =
        view->module->getAttrOfType<DictionaryAttr>("ac.control_ports");
    if (!descriptor || body.getNumArguments() < 2 ||
        !body.getArgument(0).getType().isInteger(1) ||
        !body.getArgument(1).getType().isInteger(1))
      return emitError() << "final hardware controls are incomplete";
    for (ac::RegOp reg : body.getOps<ac::RegOp>()) {
      auto domain = reg->getAttrOfType<StringAttr>("ac.domain");
      if (!domain || domain.getValue() != "default" ||
          reg.getClock() != body.getArgument(0) ||
          reg.getReset() != body.getArgument(1))
        return emitError() << "final hardware reg controls are not canonical";
      FrozenRegControls controls;
      controls.clock = reg.getClock();
      controls.reset = reg.getReset();
      controls.prefix = {body.getArgument(0), body.getArgument(1)};
      controls.prefixTypes = {body.getArgument(0).getType(),
                              body.getArgument(1).getType()};
      controls.prefixBlock = &body;
      controls.descriptor = descriptor;
      controls.domain = domain;
      result.try_emplace(reg.getOperation(), std::move(controls));
    }
  }
  return result;
}

} // namespace

LogicalResult freezeEmitReadySnapshots(
    FinalProgram &program,
    const DenseMap<Operation *, FrozenRegControls> &frozenRegControls,
    ac::detail::EmitError emitError) {
  program.stateCarrierSnapshot_.clear();
  for (const StateProposals &state : program.proposals_->states) {
    auto carrier = freezeStateCarrier(program, state.stateID, frozenRegControls,
                                      emitError);
    if (failed(carrier))
      return failure();
    program.stateCarrierSnapshot_.push_back(std::move(*carrier));
  }
  auto stateAliases = freezeStateAliases(program, emitError);
  if (failed(stateAliases))
    return failure();
  program.stateAliasSnapshot_ = std::move(*stateAliases);

  program.moduleSnapshot_.clear();
  for (InstanceView *view : program.modules_->views)
    program.moduleSnapshot_.push_back(
        {view, view->owner, view->definition, view->staticArguments,
         view->module, view->placement, operationAttributes(view->module),
         view->module->getAttrOfType<ArrayAttr>("ac.ports"),
         view == program.modules_->root});
  program.proposalContributionSnapshot_.clear();
  for (const ProposalContribution &contribution :
       program.proposals_->contributions)
    program.proposalContributionSnapshot_.push_back(
        {contribution.stateID, contribution.logicalType, contribution.data,
         contribution.enabled, contribution.value, contribution.valid,
         contribution.path, contribution.useID, contribution.sourceID,
         contribution.owner, contribution.rule, contribution.child,
         contribution.sourceFacts, contribution.composition});

  program.commitSnapshot_.clear();
  for (const StateProposals &state : program.proposals_->states)
    program.commitSnapshot_.push_back({state.stateID, state.commit.kind,
                                       state.commit.data, state.commit.enable,
                                       state.commit.contributions,
                                       state.commit.permitAlways});
  program.checkSnapshot_.clear();
  for (const CheckBinding &check : program.checks_->bindings)
    program.checkSnapshot_.push_back(
        {check.stableOrdinal, check.requiredIndex, check.owner, check.ownerRef,
         check.rule, check.registration, check.checkID, check.kind,
         check.location, check.expect, operationAttributes(check.expect),
         check.condition, check.path});
  program.observationSnapshot_.clear();
  for (const ObservationBinding &observation : program.observations_->bindings)
    program.observationSnapshot_.push_back(
        {observation.stableOrdinal, observation.requiredIndex,
         observation.owner, observation.ownerRef, observation.rule,
         observation.registration, observation.observationID, observation.kind,
         observation.spec, observation.valueIDs, observation.valueConstraints,
         observation.observe, operationAttributes(observation.observe),
         observation.path, observation.values});
  program.ruleSnapshot_.clear();
  program.expressionSnapshot_.clear();
  program.placementSnapshot_.clear();
  auto resolveRuleState = [&](InstanceView *view, Value handle) {
    for (const FinalProgram::StateAliasSnapshot &alias :
         program.stateAliasSnapshot_)
      if (alias.view == view && alias.handle == handle)
        return alias.stateID;
    return DictionaryAttr();
  };
  for (InstanceView *view : program.modules_->views) {
    for (ac::InstanceOp placement :
         view->module.getBody().front().getOps<ac::InstanceOp>())
      program.placementSnapshot_.push_back(
          freezeOperation(placement, view, {}));
    for (ac::RuleOp rule :
         view->module.getBody().front().getOps<ac::RuleOp>()) {
      FinalProgram::RuleSnapshot frozen;
      frozen.owner = view;
      frozen.rule = rule;
      frozen.moduleEntryBlock = &view->module.getBody().front();
      frozen.ruleBodyBlock = &rule.getBody().front();
      frozen.inputs.append(rule.getInputs().begin(), rule.getInputs().end());
      frozen.targets.append(rule.getTargets().begin(), rule.getTargets().end());
      frozen.results.append(rule->getResults().begin(),
                            rule->getResults().end());
      for (Value value : frozen.inputs) {
        frozen.inputTypes.push_back(value.getType());
        frozen.inputStateIDs.push_back(resolveRuleState(view, value));
      }
      for (Value value : frozen.targets) {
        frozen.outputTypes.push_back(value.getType());
        frozen.outputStateIDs.push_back(resolveRuleState(view, value));
      }
      frozen.resultTypes.append(rule->getResultTypes().begin(),
                                rule->getResultTypes().end());
      Block &body = rule.getBody().front();
      frozen.blockArguments.append(body.args_begin(), body.args_end());
      for (BlockArgument argument : body.getArguments())
        frozen.blockArgumentTypes.push_back(argument.getType());
      if (frozen.inputStateIDs.size() != frozen.inputs.size() ||
          frozen.outputStateIDs.size() != frozen.targets.size() ||
          llvm::any_of(frozen.inputStateIDs,
                       [](DictionaryAttr id) { return !id; }) ||
          llvm::any_of(frozen.outputStateIDs,
                       [](DictionaryAttr id) { return !id; })) {
        for (auto [i, id] : llvm::enumerate(frozen.inputStateIDs))
          if (!id)
            return emitError()
                   << "final rule input StateID unresolved at index " << i;
        for (auto [i, id] : llvm::enumerate(frozen.outputStateIDs))
          if (!id)
            return emitError()
                   << "final rule output StateID unresolved at index " << i;
        return emitError()
               << "final rule signature has unresolved ordered StateIDs";
      }
      if (frozen.blockArguments.size() != frozen.inputs.size())
        return emitError()
               << "final rule block arguments do not match ordered inputs";
      for (size_t i = 0; i < frozen.inputs.size(); ++i) {
        auto reg = dyn_cast<ac::RegType>(frozen.inputTypes[i]);
        if (!reg || frozen.blockArgumentTypes[i] != reg.getElementType())
          return emitError() << "final rule block argument is not bound to its "
                                "ordered input";
      }
      program.ruleSnapshot_.push_back(std::move(frozen));
    }
  }
  auto freezeRoot = [&](Value value, InstanceView *owner, ac::RuleOp rule) {
    DenseSet<Operation *> visited;
    return freezeExpression(value, owner, rule, program.expressionSnapshot_,
                            visited, emitError);
  };
  for (const ProposalContribution &contribution :
       program.proposals_->contributions) {
    SmallVector<Value> roots{contribution.data, contribution.enabled};
    if (!contribution.composition) {
      roots.append({contribution.value, contribution.valid, contribution.path});
    } else {
      for (const auto &fact : contribution.sourceFacts)
        roots.append({fact.value, fact.valid, fact.path});
    }
    for (Value value : roots)
      if (failed(freezeRoot(value, contribution.owner, contribution.rule)))
        return emitError() << "final proposal expression is not closed";
  }
  for (const StateProposals &state : program.proposals_->states)
    if (state.commit.data || state.commit.enable) {
      if (state.commit.contributions.empty())
        return emitError() << "final commit expression has no rule owner";
      size_t contributionIndex = state.commit.contributions.front();
      if (contributionIndex >= program.proposals_->contributions.size())
        return emitError() << "final commit contribution index is invalid";
      const ProposalContribution &contribution =
          program.proposals_->contributions[contributionIndex];
      for (Value value : {state.commit.data, state.commit.enable})
        if (value &&
            failed(freezeRoot(value, contribution.owner, contribution.rule)))
          return emitError() << "final commit expression is not closed";
    }
  for (const CheckBinding &check : program.checks_->bindings)
    for (Value value : {check.condition, check.path})
      if (failed(freezeRoot(value, check.owner, check.rule)))
        return emitError() << "final check expression is not closed";
  for (const ObservationBinding &observation :
       program.observations_->bindings) {
    if (failed(
            freezeRoot(observation.path, observation.owner, observation.rule)))
      return emitError() << "final observation path is not closed";
    for (Value value : observation.values)
      if (failed(freezeRoot(value, observation.owner, observation.rule)))
        return emitError() << "final observation value is not closed";
  }
  auto instancePlan = freezeSystemPlan(
      program, program.rootInstanceOrdinalSnapshot_, emitError);
  if (failed(instancePlan))
    return emitError() << "final program system plan could not be frozen";
  program.instanceSnapshot_ = std::move(*instancePlan);
  program.postOrderInstanceOrdinalsSnapshot_.clear();
  for (InstanceView *view : program.modules_->postOrder()) {
    auto found =
        llvm::find_if(program.modules_->views, [&](InstanceView *candidate) {
          return candidate == view;
        });
    if (found == program.modules_->views.end())
      return emitError() << "final postorder contains a foreign instance";
    program.postOrderInstanceOrdinalsSnapshot_.push_back(
        static_cast<size_t>(found - program.modules_->views.begin()));
  }
  program.globalPermitSnapshot_ = program.proposals_->globalPermitAlways;

  for (FinalProgram::RuleSnapshot &snapshot : program.ruleSnapshot_) {
    snapshot.finalAttributes = operationAttributes(snapshot.rule);
  }
  program.unitSnapshot_.clear();
  for (SourceLinkUnit unit : program.units_) {
    if (failed(mlir::verify(unit.body)) || failed(mlir::verify(unit.header)))
      return emitError()
             << "materialized final source unit failed native verification";
    program.unitSnapshot_.push_back(
        {unit.body, operationAttributes(unit.body)});
    program.unitSnapshot_.push_back(
        {unit.header, operationAttributes(unit.header)});
  }
  return success();
}

FailureOr<FinalProgram> buildFinalProgram(ArrayRef<SourceLinkUnit> units,
                                          ac::detail::EmitError emitError) {
  if (units.empty())
    return emitError()
           << "final program requires at least one source link unit";
  auto registry = admitSourceLinkUnits(units, emitError);
  if (failed(registry))
    return emitError() << "final program source admission failed";

  FinalProgram program;
  program.units_.append(units.begin(), units.end());
  program.registry_ =
      std::make_unique<SourceHeaderRegistry>(std::move(*registry));
  auto modules =
      buildSourceModuleGraph(program.units_, *program.registry_, emitError);
  if (failed(modules))
    return emitError() << "final program ModuleGraph construction failed";
  program.modules_ = std::make_unique<ModuleGraph>(std::move(*modules));

  auto checks = buildSourceCheckGraph(*program.modules_, emitError);
  if (failed(checks) || failed(verifySourceChecks(*checks, emitError)))
    return emitError() << "final program CheckGraph construction failed";
  program.checks_ = std::make_unique<CheckGraph>(std::move(*checks));

  auto proposals = buildSourceProposalGraph(*program.modules_,
                                            program.checks_.get(), emitError);
  if (failed(proposals) || failed(verifySourceProposals(*proposals, emitError)))
    return emitError() << "final program ProposalGraph construction failed";
  program.proposals_ = std::make_unique<ProposalGraph>(std::move(*proposals));
  if (failed(verifyProposalStorage(*program.modules_, *program.proposals_,
                                   emitError)))
    return emitError() << "final program ProposalGraph storage closure failed";

  auto observations = buildSourceObservationGraph(*program.modules_, emitError);
  if (failed(observations) ||
      failed(verifySourceObservations(*observations, emitError)))
    return emitError() << "final program ObservationGraph construction failed";
  program.observations_ =
      std::make_unique<ObservationGraph>(std::move(*observations));
  program.state_ = FinalProgramState::AnalysisClosed;
  if (failed(verifyFinalProgram(program, emitError)))
    return failure();
  return std::move(program);
}

FailureOr<FinalProgram>
materializeFinalProgram(FinalProgram &&program,
                        ac::detail::EmitError emitError) {
  if (program.state_ != FinalProgramState::AnalysisClosed ||
      failed(verifyFinalProgram(program, emitError)) ||
      failed(preflightMaterialization(program, emitError)) ||
      failed(preflightUnitCarriers(program.units_, emitError)))
    return emitError()
           << "final program is not a materializable analysis closure";

  SmallVector<OwningOpRef<ModuleOp>> ownedUnits;
  SmallVector<SourceLinkUnit> clonedUnits;
  ownedUnits.reserve(2 * program.units_.size());
  clonedUnits.reserve(program.units_.size());
  for (SourceLinkUnit unit : program.units_) {
    auto body = OwningOpRef<ModuleOp>(cast<ModuleOp>(unit.body->clone()));
    auto header = OwningOpRef<ModuleOp>(cast<ModuleOp>(unit.header->clone()));
    clonedUnits.push_back({*body, *header});
    ownedUnits.push_back(std::move(body));
    ownedUnits.push_back(std::move(header));
  }
  auto cloned = buildFinalProgram(clonedUnits, emitError);
  if (failed(cloned) || failed(preflightMaterialization(*cloned, emitError)))
    return emitError()
           << "cloned final program does not preserve analysis closure";
  program = std::move(*cloned);
  program.ownedUnits_ = std::move(ownedUnits);

  auto numericEvidence = freezeFinalNumericEvidence(
      *program.modules_, *program.proposals_, *program.checks_, emitError);
  if (failed(numericEvidence))
    return failure();
  program.numericRuleSnapshot_ = std::move(*numericEvidence);

  DenseMap<Operation *, FrozenRegControls> frozenRegControls;
  for (InstanceView *view : program.modules_->views) {
    if (!view || !view->module || view->module.getBody().empty())
      return emitError() << "final module has no control prefix";
    Block &body = view->module.getBody().front();
    if (body.getNumArguments() < 2 ||
        !body.getArgument(0).getType().isInteger(1) ||
        !body.getArgument(1).getType().isInteger(1))
      return emitError()
             << "final module control prefix must be clock/reset i1";
    DictionaryAttr descriptor =
        view->module->getAttrOfType<DictionaryAttr>("ac.control_ports");
    auto clockIndex =
        descriptor ? descriptor.getAs<IntegerAttr>("clock") : IntegerAttr();
    auto resetIndex =
        descriptor ? descriptor.getAs<IntegerAttr>("reset") : IntegerAttr();
    if (!descriptor || descriptor.size() != 2 || !clockIndex || !resetIndex ||
        !clockIndex.getType().isInteger(32) ||
        !resetIndex.getType().isInteger(32) ||
        !clockIndex.getValue().isZero() || !resetIndex.getValue().isOne())
      return emitError()
             << "final module control descriptor must be clock=0/reset=1";
    for (ac::RegOp reg : body.getOps<ac::RegOp>()) {
      auto domain = reg->getAttrOfType<StringAttr>("ac.domain");
      if (reg.getClock() != body.getArgument(0) ||
          reg.getReset() != body.getArgument(1) ||
          !reg.getClock().getType().isInteger(1) ||
          !reg.getReset().getType().isInteger(1) || !domain ||
          domain.getValue() != "default")
        return emitError() << "owned state clock/reset must identity-forward "
                              "the module control prefix";
      FrozenRegControls controls;
      controls.clock = reg.getClock();
      controls.reset = reg.getReset();
      controls.prefix = {body.getArgument(0), body.getArgument(1)};
      controls.prefixTypes = {body.getArgument(0).getType(),
                              body.getArgument(1).getType()};
      controls.prefixBlock = &body;
      controls.descriptor = descriptor;
      controls.domain = domain;
      frozenRegControls.try_emplace(reg.getOperation(), std::move(controls));
    }
  }

  DenseMap<Value, Value> replaced;
  SmallVector<ac::SourceUseOp> uses;
  SmallVector<ac::SourceReadOp> reads;
  DenseSet<Operation *> definitions;
  for (InstanceView *view : program.modules_->views) {
    if (!definitions.insert(view->module).second)
      continue;
    for (ac::RegOp reg : view->module.getBody().front().getOps<ac::RegOp>()) {
      auto initial = materializeInitial(reg, emitError);
      if (failed(initial))
        return failure();
      auto logical = reg->getAttrOfType<DictionaryAttr>("ac.logical_element");
      if (!logical)
        return emitError() << "final reg has no canonical logical type";
      reg->setAttr("ac.logical_type", logical);
      reg->setAttr("ac.initial_value", *initial);
      for (StringRef name : {"ac.logical_element", "ac.shape"})
        reg->removeAttr(name);
    }
    for (ac::RuleOp rule :
         view->module.getBody().front().getOps<ac::RuleOp>()) {
      Block &body = rule.getBody().front();
      if (failed(retainGenericSourceUses(rule, replaced)))
        return emitError()
               << "final source-use authority could not be retained";
      for (ac::SourceUseOp use : body.getOps<ac::SourceUseOp>()) {
        OpBuilder builder(use);
        auto enable = arith::AndIOp::create(builder, use.getLoc(),
                                            use.getValid(), use.getPath());
        replaced.try_emplace(use.getData(), use.getValue());
        replaced.try_emplace(use.getEnabled(), enable.getResult());
        use.getData().replaceAllUsesWith(use.getValue());
        use.getEnabled().replaceAllUsesWith(enable.getResult());
        uses.push_back(use);
      }
      for (ac::SourceReadOp read : body.getOps<ac::SourceReadOp>()) {
        replaced.try_emplace(read.getResult(), read.getCurrent());
        read.getResult().replaceAllUsesWith(read.getCurrent());
        reads.push_back(read);
      }
      Builder builder(rule.getContext());
      for (ac::SourceObserveOp observe : body.getOps<ac::SourceObserveOp>()) {
        auto constraints =
            observe->getAttrOfType<ArrayAttr>("ac.value_constraints");
        SmallVector<Attribute> logicalTypes;
        if (!constraints || constraints.size() != observe.getValues().size())
          return emitError()
                 << "source observation constraints cannot be finalized";
        for (Attribute raw : constraints) {
          auto constraint = dyn_cast<DictionaryAttr>(raw);
          auto kind =
              constraint ? constraint.getAs<StringAttr>("kind") : StringAttr();
          auto logical = constraint ? constraint.getAs<DictionaryAttr>("type")
                                    : DictionaryAttr();
          if (!constraint || constraint.size() != 2 || !kind ||
              kind.getValue() != "logical" || !logical ||
              failed(
                  ac::detail::verifyLogicalTypeStructure(logical, emitError)))
            return emitError()
                   << "source observation has no finite final LogicalType";
          logicalTypes.push_back(logical);
        }
        observe->setAttr("ac.logical_types",
                         builder.getArrayAttr(logicalTypes));
        observe->removeAttr("ac.value_constraints");
      }
      if (!rule->hasAttr("ac.required_checks"))
        rule->setAttr("ac.required_checks", builder.getArrayAttr({}));
      if (!rule->hasAttr("ac.required_observations"))
        rule->setAttr("ac.required_observations", builder.getArrayAttr({}));
    }
  }
  for (SourceLinkUnit unit : program.units_) {
    unit.body->setAttr("ac.stage",
                       StringAttr::get(unit.body.getContext(), "final"));
    unit.header->setAttr("ac.stage",
                         StringAttr::get(unit.header.getContext(), "final"));
  }

  for (ProposalContribution &contribution : program.proposals_->contributions) {
    contribution.data = resolveReplacement(contribution.data, replaced);
    contribution.enabled = resolveReplacement(contribution.enabled, replaced);
    contribution.value = resolveReplacement(contribution.value, replaced);
    contribution.valid = resolveReplacement(contribution.valid, replaced);
    contribution.path = resolveReplacement(contribution.path, replaced);
    contribution.use = {};
    for (auto &fact : contribution.sourceFacts) {
      fact.value = resolveReplacement(fact.value, replaced);
      fact.valid = resolveReplacement(fact.valid, replaced);
      fact.path = resolveReplacement(fact.path, replaced);
      fact.use = {};
    }
  }
  for (StateProposals &state : program.proposals_->states) {
    state.commit.data = resolveReplacement(state.commit.data, replaced);
    state.commit.enable = resolveReplacement(state.commit.enable, replaced);
  }
  for (CheckBinding &check : program.checks_->bindings) {
    check.condition = resolveReplacement(check.condition, replaced);
    check.path = resolveReplacement(check.path, replaced);
  }
  for (ObservationBinding &observation : program.observations_->bindings) {
    observation.path = resolveReplacement(observation.path, replaced);
    for (Value &value : observation.values)
      value = resolveReplacement(value, replaced);
    auto logicalTypes =
        observation.observe->getAttrOfType<ArrayAttr>("ac.logical_types");
    SmallVector<Attribute> wrappers;
    if (!logicalTypes || logicalTypes.size() != observation.values.size())
      return emitError()
             << "final observation LogicalType inventory is incomplete";
    Builder builder(observation.observe.getContext());
    for (Attribute logical : logicalTypes)
      wrappers.push_back(builder.getDictionaryAttr({
          builder.getNamedAttr("kind", builder.getStringAttr("logical")),
          builder.getNamedAttr("type", logical),
      }));
    observation.valueConstraints = builder.getArrayAttr(wrappers);
  }
  rebaseFinalNumericEvidence(program.numericRuleSnapshot_, replaced);

  for (ac::SourceUseOp use : uses)
    use.erase();
  for (ac::SourceReadOp read : reads)
    read.erase();
  for (InstanceView *view : program.modules_->views)
    for (ac::RuleOp rule : view->module.getBody().front().getOps<ac::RuleOp>())
      for (auto constant : llvm::make_early_inc_range(
               rule.getBody().front().getOps<arith::ConstantOp>()))
        if (constant.getResult().use_empty())
          constant.erase();
  if (failed(verifyFinalNumericEvidence(program.numericRuleSnapshot_,
                                        *program.modules_, *program.proposals_,
                                        *program.checks_, emitError)))
    return emitError() << "final numeric evidence failed after cleanup";

  for (InstanceView *view : program.modules_->views)
    view->header = {};
  DenseSet<Operation *> sourceUnits;
  for (SourceLinkUnit unit : program.units_) {
    for (ModuleOp sourceUnit : {unit.body, unit.header}) {
      if (!sourceUnit || !sourceUnits.insert(sourceUnit).second)
        continue;
      SmallVector<ac::ModuleImportOp> imports;
      SmallVector<Operation *> declarations;
      sourceUnit->walk(
          [&](ac::ModuleImportOp import) { imports.push_back(import); });
      sourceUnit->walk([&](Operation *operation) {
        if (isa<ac::TypeAliasOp, ac::ConstantOp>(operation))
          declarations.push_back(operation);
      });
      for (ac::ModuleImportOp import : llvm::reverse(imports))
        import.erase();
      for (Operation *declaration : llvm::reverse(declarations))
        declaration->erase();
    }
  }
  program.registry_.reset();
  auto hardware = materializeFinalHardwarePackage(
      program.ownedUnits_, program.units_, *program.modules_, emitError);
  if (failed(hardware))
    return emitError() << "global final hardware materialization failed";
  program.finalHardware_ = std::move(*hardware);
  if (failed(ac::verifyFinalHardware(*program.finalHardware_)) ||
      failed(mlir::verify(*program.finalHardware_)))
    return emitError() << "global final hardware failed native verification";

  if (failed(freezeEmitReadySnapshots(program, frozenRegControls, emitError)))
    return failure();

  program.state_ = FinalProgramState::EmitReady;
  if (failed(verifyFinalProgram(program, emitError)))
    return failure();
  return std::move(program);
}

FailureOr<FinalProgram>
buildFinalProgramFromHardware(ModuleOp package,
                              ac::detail::EmitError emitError) {
  if (!package || failed(mlir::verify(package)) ||
      failed(ac::verifyFinalHardware(package)))
    return emitError()
           << "final program rehydration requires verified final hardware";
  FinalProgram program;
  program.finalHardware_ =
      OwningOpRef<ModuleOp>(cast<ModuleOp>(package->clone()));
  if (!program.finalHardware_ ||
      failed(mlir::verify(*program.finalHardware_)) ||
      failed(ac::verifyFinalHardware(*program.finalHardware_)))
    return emitError() << "cloned final hardware failed native verification";

  auto rebuilt =
      rebuildFinalHardwareProgram(*program.finalHardware_, emitError);
  if (failed(rebuilt))
    return emitError() << "final hardware program view reconstruction failed";
  program.modules_ = std::move(rebuilt->modules);
  program.checks_ = std::move(rebuilt->checks);
  program.proposals_ = std::move(rebuilt->proposals);
  program.observations_ = std::move(rebuilt->observations);

  auto numeric = freezeFinalNumericEvidenceFromHardware(
      *program.modules_, *program.proposals_, *program.checks_, emitError);
  if (failed(numeric))
    return emitError()
           << "final hardware numeric evidence reconstruction failed";
  program.numericRuleSnapshot_ = std::move(*numeric);
  if (failed(verifyFinalNumericEvidence(program.numericRuleSnapshot_,
                                        *program.modules_, *program.proposals_,
                                        *program.checks_, emitError)))
    return emitError() << "reconstructed final numeric evidence is invalid";

  auto controls = collectFinalRegControls(program, emitError);
  if (failed(controls) ||
      failed(freezeEmitReadySnapshots(program, *controls, emitError)))
    return emitError() << "final hardware snapshots could not be frozen";
  program.state_ = FinalProgramState::EmitReady;
  if (failed(verifyFinalProgram(program, emitError)))
    return failure();
  return std::move(program);
}

LogicalResult verifyFinalProgram(const FinalProgram &program,
                                 ac::detail::EmitError emitError) {
  if (program.state_ == FinalProgramState::EmitReady) {
    if (program.registry_ || !program.finalHardware_ || !program.modules_ ||
        !program.checks_ || !program.proposals_ || !program.observations_ ||
        !program.modules_->root ||
        program.checks_->modules != program.modules_.get() ||
        program.proposals_->modules != program.modules_.get() ||
        program.proposals_->checks != program.checks_.get() ||
        program.observations_->modules != program.modules_.get())
      return emitError()
             << "EmitReady final program graph pointers are not closed";
    if (failed(ac::verifyFinalHardware(*program.finalHardware_)) ||
        failed(mlir::verify(*program.finalHardware_)))
      return emitError()
             << "EmitReady global final hardware verification failed";
    for (SourceLinkUnit unit : program.units_)
      if (failed(mlir::verify(unit.body)) || failed(mlir::verify(unit.header)))
        return emitError()
               << "EmitReady source unit failed native verification";
    DenseSet<const InstanceView *> views;
    bool containsRoot = false;
    for (const InstanceView *view : program.modules_->views) {
      if (!view || !view->module || view->header ||
          !views.insert(view).second ||
          failed(ac::detail::verifyOwnerRef(view->owner, emitError)))
        return emitError() << "EmitReady ModuleGraph is incomplete";
      containsRoot |= view == program.modules_->root;
    }
    if (!containsRoot)
      return emitError() << "EmitReady root is outside its ModuleGraph";
    if (program.moduleSnapshot_.size() != program.modules_->views.size() ||
        program.proposalContributionSnapshot_.size() !=
            program.proposals_->contributions.size() ||
        program.stateCarrierSnapshot_.size() !=
            program.proposals_->states.size() ||
        program.instanceSnapshot_.size() != program.modules_->views.size() ||
        program.postOrderInstanceOrdinalsSnapshot_.size() !=
            program.modules_->views.size() ||
        program.commitSnapshot_.size() != program.proposals_->states.size() ||
        program.checkSnapshot_.size() != program.checks_->bindings.size() ||
        program.observationSnapshot_.size() !=
            program.observations_->bindings.size() ||
        program.unitSnapshot_.size() != 2 * program.units_.size() ||
        program.globalPermitSnapshot_ != program.proposals_->globalPermitAlways)
      return emitError() << "EmitReady closure cardinality or permit changed";
    if (failed(verifyFinalNumericEvidence(
            program.numericRuleSnapshot_, *program.modules_,
            *program.proposals_, *program.checks_, emitError)))
      return emitError() << "EmitReady numeric evidence closure changed";
    for (const FinalNumericRuleSnapshot &numeric :
         program.numericRuleSnapshot_) {
      if (numeric.composition)
        continue; // Global hardware verification checks every composed
                  // input/output binding.
      ac::RuleOp rule = numeric.rule;
      if (rule.getInputs().size() != 1 || rule.getTargets().size() != 1 ||
          program.resolveStateID(numeric.owner, rule.getInputs()[0]) !=
              numeric.stateID ||
          program.resolveStateID(numeric.owner, rule.getTargets()[0]) !=
              numeric.stateID)
        return emitError()
               << "EmitReady numeric current/next StateID authority changed";
    }
    for (auto [index, snapshot] : llvm::enumerate(program.moduleSnapshot_)) {
      InstanceView *view = program.modules_->views[index];
      if (!view || view != snapshot.view || view->owner != snapshot.owner ||
          view->definition != snapshot.definition ||
          view->staticArguments != snapshot.staticArguments ||
          view->module != snapshot.module ||
          view->placement != snapshot.placement || !view->module ||
          operationAttributes(view->module) != snapshot.attributes ||
          (view == program.modules_->root) != snapshot.root ||
          failed(ac::detail::verifyOwnerRef(view->owner, emitError)) ||
          view->module->hasAttr("ac.stage") ||
          view->module->getAttr("ac.ports") != snapshot.ports)
        return emitError() << "EmitReady module owner/root closure changed";
    }
    for (auto [index, snapshot] : llvm::enumerate(program.unitSnapshot_)) {
      SourceLinkUnit unit = program.units_[index / 2];
      ModuleOp actual = index % 2 == 0 ? unit.body : unit.header;
      if (!snapshot.unit || actual != snapshot.unit ||
          operationAttributes(actual) != snapshot.attributes)
        return emitError() << "EmitReady source-unit metadata changed";
    }
    for (const FinalProgram::OperationSnapshot &snapshot :
         program.placementSnapshot_) {
      bool linkedPlacement =
          snapshot.owner && snapshot.operation &&
          snapshot.operation->getParentOfType<ac::ModuleOp>() ==
              snapshot.owner->module &&
          snapshot.operation->getBlock() == snapshot.block &&
          snapshot.block == &snapshot.owner->module.getBody().front();
      if (!linkedPlacement || !snapshot.operation ||
          snapshot.operation->getName().getStringRef() != "ac.instance" ||
          !sameOperation(snapshot) ||
          snapshot.operation->getParentOfType<ac::ModuleOp>() !=
              snapshot.owner->module)
        return emitError() << "EmitReady placement operation closure changed";
    }
    for (const FinalProgram::RuleSnapshot &snapshot : program.ruleSnapshot_) {
      ac::RuleOp rule = snapshot.rule;
      if (!snapshot.owner || !rule ||
          rule->getParentOfType<ac::ModuleOp>() != snapshot.owner->module ||
          rule->getBlock() != snapshot.moduleEntryBlock ||
          snapshot.moduleEntryBlock !=
              &snapshot.owner->module.getBody().front() ||
          rule.getBody().getBlocks().size() != 1 ||
          &rule.getBody().front() != snapshot.ruleBodyBlock ||
          rule.getInputs().size() != snapshot.inputs.size() ||
          rule.getTargets().size() != snapshot.targets.size() ||
          rule->getNumResults() != snapshot.results.size() ||
          rule.getBody().front().getNumArguments() !=
              snapshot.blockArguments.size() ||
          operationAttributes(rule) != snapshot.finalAttributes ||
          snapshot.inputTypes.size() != snapshot.inputs.size() ||
          snapshot.outputTypes.size() != snapshot.targets.size())
        return emitError() << "EmitReady rule signature closure changed";
      for (size_t i = 0; i < snapshot.inputs.size(); ++i) {
        Value value = rule.getInputs()[i];
        if (value != snapshot.inputs[i] ||
            value.getType() != snapshot.inputTypes[i] ||
            program.resolveStateID(snapshot.owner, value) !=
                snapshot.inputStateIDs[i])
          return emitError() << "EmitReady ordered rule input/StateID changed";
      }
      for (size_t i = 0; i < snapshot.targets.size(); ++i) {
        Value value = rule.getTargets()[i];
        if (value != snapshot.targets[i] ||
            value.getType() != snapshot.outputTypes[i] ||
            program.resolveStateID(snapshot.owner, value) !=
                snapshot.outputStateIDs[i])
          return emitError() << "EmitReady ordered rule output/StateID changed";
      }
      for (size_t i = 0; i < snapshot.results.size(); ++i)
        if (rule->getResult(i) != snapshot.results[i] ||
            rule->getResult(i).getType() != snapshot.resultTypes[i])
          return emitError() << "EmitReady rule result target changed";
      for (size_t i = 0; i < snapshot.blockArguments.size(); ++i)
        if (rule.getBody().front().getArgument(i) !=
                snapshot.blockArguments[i] ||
            rule.getBody().front().getArgument(i).getType() !=
                snapshot.blockArgumentTypes[i] ||
            snapshot.blockArgumentTypes[i] !=
                cast<ac::RegType>(snapshot.inputTypes[i]).getElementType())
          return emitError() << "EmitReady rule block argument changed";
    }
    for (const FinalProgram::OperationSnapshot &snapshot :
         program.expressionSnapshot_) {
      ac::RuleOp frozenRule = snapshot.rule;
      if (!sameOperation(snapshot) || !snapshot.owner ||
          snapshot.operation->getParentOfType<ac::ModuleOp>() !=
              snapshot.owner->module ||
          !frozenRule ||
          snapshot.operation->getParentOfType<ac::RuleOp>() != frozenRule ||
          snapshot.operation->getBlock() != snapshot.block ||
          snapshot.block != &frozenRule.getBody().front())
        return emitError() << "EmitReady expression DAG operation changed";
    }
    auto verifyPartition = [&](size_t total, auto member, auto expectedOwner) {
      SmallVector<unsigned> counts(total, 0);
      for (const FinalProgram::FinalInstanceSnapshot &row :
           program.instanceSnapshot_)
        for (size_t index : member(row)) {
          if (index >= total || ++counts[index] != 1 ||
              expectedOwner(index) != row.view)
            return false;
        }
      return llvm::all_of(counts, [](unsigned count) { return count == 1; });
    };
    if (!verifyPartition(
            program.proposals_->contributions.size(),
            [](const FinalProgram::FinalInstanceSnapshot &row)
                -> ArrayRef<size_t> {
              return row.proposalContributionOrdinals;
            },
            [&](size_t i) {
              return program.proposals_->contributions[i].owner;
            }) ||
        !verifyPartition(
            program.proposals_->states.size(),
            [](const FinalProgram::FinalInstanceSnapshot &row)
                -> ArrayRef<size_t> { return row.proposalStateOrdinals; },
            [&](size_t i) -> InstanceView * {
              Attribute path = program.proposals_->states[i]
                                   .stateID.getAs<DictionaryAttr>("owner")
                                   .get("instance_path");
              for (InstanceView *view : program.modules_->views)
                if (view->owner.get("instance_path") == path)
                  return view;
              return nullptr;
            }) ||
        !verifyPartition(
            program.checks_->bindings.size(),
            [](const FinalProgram::FinalInstanceSnapshot &row)
                -> ArrayRef<size_t> { return row.checkOrdinals; },
            [&](size_t i) { return program.checks_->bindings[i].owner; }) ||
        !verifyPartition(
            program.observations_->bindings.size(),
            [](const FinalProgram::FinalInstanceSnapshot &row)
                -> ArrayRef<size_t> { return row.observationOrdinals; },
            [&](size_t i) { return program.observations_->bindings[i].owner; }))
      return emitError() << "EmitReady instance contribution partition is "
                            "incomplete or misowned";
    for (auto [index, snapshot] :
         llvm::enumerate(program.stateCarrierSnapshot_)) {
      if (snapshot.stateID != program.proposals_->states[index].stateID ||
          !snapshot.declaration || snapshot.formalPort ||
          failed(verifyFrozenStateCarrier(snapshot, emitError)))
        return emitError() << "EmitReady state carrier closure changed";
    }
    if (failed(verifyFrozenStateAliases(program, program.stateAliasSnapshot_,
                                        emitError)))
      return emitError() << "EmitReady state alias closure changed";
    if (failed(verifyProposalStorage(*program.modules_, *program.proposals_,
                                     emitError)))
      return emitError() << "EmitReady proposal storage closure changed";
    size_t currentRootOrdinal = 0;
    auto currentInstances =
        freezeSystemPlan(program, currentRootOrdinal, emitError);
    if (failed(currentInstances) ||
        currentRootOrdinal != program.rootInstanceOrdinalSnapshot_ ||
        currentInstances->size() != program.instanceSnapshot_.size())
      return emitError() << "EmitReady instance plan closure changed";
    for (auto [index, expected] : llvm::enumerate(program.instanceSnapshot_)) {
      const auto &actual = (*currentInstances)[index];
      if (actual.ordinal != expected.ordinal || actual.view != expected.view ||
          actual.owner != expected.owner ||
          actual.definition != expected.definition ||
          actual.staticArguments != expected.staticArguments ||
          actual.module != expected.module ||
          actual.placement != expected.placement ||
          actual.parentOrdinal != expected.parentOrdinal ||
          actual.childOrdinals != expected.childOrdinals ||
          actual.ownedStateOrdinals != expected.ownedStateOrdinals ||
          actual.ruleOrdinals != expected.ruleOrdinals ||
          actual.proposalContributionOrdinals !=
              expected.proposalContributionOrdinals ||
          actual.proposalStateOrdinals != expected.proposalStateOrdinals ||
          actual.checkOrdinals != expected.checkOrdinals ||
          actual.observationOrdinals != expected.observationOrdinals)
        return emitError() << "EmitReady instance plan row changed";
    }
    if (program.placementSnapshot_.size() + 1 != program.modules_->views.size())
      return emitError() << "EmitReady placement inventory is incomplete";
    SmallVector<size_t> currentPostOrder;
    for (InstanceView *view : program.modules_->postOrder()) {
      auto found = llvm::find(program.modules_->views, view);
      if (found == program.modules_->views.end())
        return emitError() << "EmitReady postorder contains a foreign instance";
      currentPostOrder.push_back(
          static_cast<size_t>(found - program.modules_->views.begin()));
    }
    if (currentPostOrder != program.postOrderInstanceOrdinalsSnapshot_)
      return emitError() << "EmitReady postorder instance ordinals changed";
    for (auto [index, snapshot] :
         llvm::enumerate(program.proposalContributionSnapshot_)) {
      const ProposalContribution &contribution =
          program.proposals_->contributions[index];
      if (contribution.stateID != snapshot.stateID ||
          contribution.logicalType != snapshot.logicalType ||
          contribution.data != snapshot.data ||
          contribution.enabled != snapshot.enabled ||
          contribution.value != snapshot.value ||
          contribution.valid != snapshot.valid ||
          contribution.path != snapshot.path ||
          contribution.useID != snapshot.useID ||
          contribution.sourceID != snapshot.sourceID ||
          contribution.owner != snapshot.owner ||
          contribution.rule != snapshot.rule ||
          contribution.child != snapshot.child ||
          contribution.composition != snapshot.composition ||
          contribution.sourceFacts.size() != snapshot.sourceFacts.size())
        return emitError() << "EmitReady proposal contribution closure changed";
      for (auto [i, fact] : llvm::enumerate(contribution.sourceFacts)) {
        const auto &saved = snapshot.sourceFacts[i];
        if (fact.useID != saved.useID || fact.sourceID != saved.sourceID ||
            fact.value != saved.value || fact.valid != saved.valid ||
            fact.path != saved.path || fact.use != saved.use ||
            fact.valueUse != saved.valueUse)
          return emitError() << "EmitReady original proposal use facts changed";
      }
    }
    for (auto [index, snapshot] : llvm::enumerate(program.commitSnapshot_)) {
      const StateProposals &state = program.proposals_->states[index];
      if (state.stateID != snapshot.stateID ||
          state.commit.stateID != snapshot.stateID ||
          state.commit.kind != snapshot.kind ||
          state.commit.data != snapshot.data ||
          state.commit.enable != snapshot.enable ||
          state.commit.contributions != snapshot.contributions ||
          state.commit.permitAlways != snapshot.permitAlways ||
          (state.commit.kind == CommitPairKind::Forward &&
           (!state.commit.data || !state.commit.enable)))
        return emitError() << "EmitReady commit closure changed";
    }
    DenseSet<Operation *> liveChecks;
    DenseSet<Operation *> liveObservations;
    DenseSet<Operation *> checkedDefinitions;
    for (InstanceView *view : program.modules_->views) {
      if (!view || !view->module ||
          !checkedDefinitions.insert(view->module).second)
        continue;
      for (ac::RuleOp rule :
           view->module.getBody().front().getOps<ac::RuleOp>()) {
        for (ac::SourceExpectOp expect :
             rule.getBody().front().getOps<ac::SourceExpectOp>())
          liveChecks.insert(expect);
        for (ac::SourceObserveOp observe :
             rule.getBody().front().getOps<ac::SourceObserveOp>())
          liveObservations.insert(observe);
      }
    }
    DenseSet<Operation *> expectedChecks;
    DenseSet<Operation *> expectedObservations;
    for (const FinalProgram::CheckSnapshot &snapshot : program.checkSnapshot_)
      expectedChecks.insert(snapshot.expect);
    for (const FinalProgram::ObservationSnapshot &snapshot :
         program.observationSnapshot_)
      expectedObservations.insert(snapshot.observe);
    if (liveChecks != expectedChecks ||
        liveObservations != expectedObservations)
      return emitError()
             << "EmitReady check/observation operation inventory changed";
    for (auto [index, snapshot] : llvm::enumerate(program.checkSnapshot_)) {
      const CheckBinding &check = program.checks_->bindings[index];
      if (check.stableOrdinal != snapshot.stableOrdinal ||
          check.requiredIndex != snapshot.requiredIndex ||
          check.owner != snapshot.owner ||
          check.ownerRef != snapshot.ownerRef || check.rule != snapshot.rule ||
          check.registration != snapshot.registration ||
          check.checkID != snapshot.checkID || check.kind != snapshot.kind ||
          check.location != snapshot.location ||
          check.expect != snapshot.expect || !check.expect ||
          !liveChecks.contains(check.expect) ||
          operationAttributes(check.expect) != snapshot.attributes ||
          check.condition != snapshot.condition || check.path != snapshot.path)
        return emitError() << "EmitReady check closure changed";
    }
    for (auto [index, snapshot] :
         llvm::enumerate(program.observationSnapshot_)) {
      const ObservationBinding &observation =
          program.observations_->bindings[index];
      if (observation.stableOrdinal != snapshot.stableOrdinal ||
          observation.requiredIndex != snapshot.requiredIndex ||
          observation.owner != snapshot.owner ||
          observation.ownerRef != snapshot.ownerRef ||
          observation.rule != snapshot.rule ||
          observation.registration != snapshot.registration ||
          observation.observationID != snapshot.observationID ||
          observation.kind != snapshot.kind ||
          observation.spec != snapshot.spec ||
          observation.valueIDs != snapshot.valueIDs ||
          observation.valueConstraints != snapshot.valueConstraints ||
          observation.observe != snapshot.observe || !observation.observe ||
          !liveObservations.contains(observation.observe) ||
          operationAttributes(observation.observe) != snapshot.attributes ||
          observation.path != snapshot.path ||
          observation.values != snapshot.values)
        return emitError() << "EmitReady observation closure changed";
    }
    bool residual = hasAnyFinalResidual(program);
    auto inspectResidual = [&](ModuleOp unit) {
      unit->walk([&](Operation *operation) {
        StringRef name = operation->getName().getStringRef();
        bool retainedNumeric = isRetainedFinalNumericCarrier(
            program.numericRuleSnapshot_, operation);
        auto rule = operation->getParentOfType<ac::RuleOp>();
        bool retainedUse = rule && ac::hasGenericFinalUses(rule) &&
                           isa<ac::ValueBindingOp, ac::ValueUseOp>(operation);
        residual |= isa<ac::SourceReadOp, ac::SourceUseOp, ac::ModuleImportOp,
                        ac::StructOp, mlir::func::FuncOp>(operation) ||
                    name.starts_with("ac.math.") ||
                    ((name == "ac.numeric.proof" ||
                      name == "ac.value.binding" || name == "ac.value.use") &&
                     !retainedNumeric && !retainedUse);
        for (Type type : operation->getOperandTypes())
          residual |= isa<ac::MathIntType>(type);
        for (Type type : operation->getResultTypes())
          residual |= isa<ac::MathIntType>(type);
        if (!retainedNumeric)
          for (NamedAttribute attribute : operation->getAttrs())
            residual |= containsSourceAttribute(attribute.getValue());
      });
    };
    for (SourceLinkUnit unit : program.units_) {
      inspectResidual(unit.body);
      inspectResidual(unit.header);
    }
    if (residual)
      return emitError()
             << "EmitReady program contains a residual source carrier";
    return success();
  }
  if (program.state_ != FinalProgramState::AnalysisClosed ||
      program.isEmitReady())
    return emitError()
           << "final program has no verified EmitReady construction path";
  if (failed(program.verifyOwnedSourceClosure(emitError)))
    return failure();
  if (failed(verifySourceChecks(*program.checks_, emitError)) ||
      failed(verifySourceProposals(*program.proposals_, emitError)) ||
      failed(verifySourceObservations(*program.observations_, emitError)))
    return emitError() << "final program analysis graph verification failed";
  if (failed(verifyProposalStorage(*program.modules_, *program.proposals_,
                                   emitError)))
    return emitError() << "final program state ownership closure failed";
  size_t rootOrdinal = 0;
  if (failed(freezeSystemPlan(program, rootOrdinal, emitError)))
    return emitError() << "final program instance/system plan is inconsistent";
  if (!hasResidualSourceSemantics(program) && !isZeroRuleSourceClosure(program))
    return emitError()
           << "AnalysisClosed final program requires residual source semantics";
  return success();
}

} // namespace acir::compiler
