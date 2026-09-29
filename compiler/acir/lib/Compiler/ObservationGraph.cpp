#include "ObservationGraph.h"
#include "Compiler/RuleInventory.h"
#include "Dialect/ACIR/ACIRNumericComposition.h"
#include "Dialect/ACIR/ACIRNumericNextUse.h"

#include "mlir/Dialect/Arith/IR/Arith.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"

using namespace mlir;

namespace acir::compiler {
namespace {

DictionaryAttr observationSite(DictionaryAttr identity) {
  return identity ? identity.getAs<DictionaryAttr>("site") : DictionaryAttr();
}

int compareBindings(const ObservationBinding &left,
                    const ObservationBinding &right) {
  int owner =
      ac::detail::compareClosedSourceStructure(left.ownerRef, right.ownerRef);
  if (owner)
    return owner;
  int registration = ac::detail::compareClosedSourceStructure(
      left.registration, right.registration);
  if (registration)
    return registration;
  return ac::detail::compareClosedSourceStructure(
      observationSite(left.observationID),
      observationSite(right.observationID));
}

DictionaryAttr requiredObservation(Builder &builder,
                                   ac::SourceObserveOp observe) {
  return builder.getDictionaryAttr({
      builder.getNamedAttr(
          "id", observe->getAttrOfType<DictionaryAttr>("ac.observation_id")),
      builder.getNamedAttr("kind", observe.getKindAttr()),
      builder.getNamedAttr("spec", observe.getSpecAttr()),
      builder.getNamedAttr("values",
                           observe->getAttrOfType<ArrayAttr>("ac.value_ids")),
  });
}

bool isFinalObservation(ac::SourceObserveOp observe) {
  auto unit = observe->getParentOfType<mlir::ModuleOp>();
  auto stage =
      unit ? unit->getAttrOfType<StringAttr>("ac.stage") : StringAttr();
  return stage && stage.getValue() == "final";
}

ArrayAttr internalValueConstraints(Builder &builder,
                                   ac::SourceObserveOp observe) {
  if (!isFinalObservation(observe))
    return observe->getAttrOfType<ArrayAttr>("ac.value_constraints");
  auto logicalTypes = observe->getAttrOfType<ArrayAttr>("ac.logical_types");
  if (!logicalTypes || observe->hasAttr("ac.value_constraints"))
    return {};
  SmallVector<Attribute> result;
  for (Attribute raw : logicalTypes) {
    auto logical = dyn_cast<DictionaryAttr>(raw);
    if (!logical)
      return {};
    result.push_back(builder.getDictionaryAttr({
        builder.getNamedAttr("kind", builder.getStringAttr("logical")),
        builder.getNamedAttr("type", logical),
    }));
  }
  return builder.getArrayAttr(result);
}

LogicalResult rejectUnsupportedNumericProofs(ac::RuleOp rule,
                                             ac::detail::EmitError emitError) {
  // Generic provenance carriers (ac.value.binding / ac.value.use) are
  // synthesized for ordinary direct-current and literal assignments too, so only
  // an explicit numeric obligation may gate this closure. The shared classifier
  // is the single definition of that test.
  if (!ruleHasNumericObligation(rule))
    return success();
  if (ac::hasNumericCompositionContract(rule)) {
    if (failed(ac::verifyNumericCompositionClosure(rule)))
      return emitError() << "source observation requires verified numeric "
                            "composition closure";
    return success();
  }
  Block &body = rule.getBody().front();
  if (!ac::hasNumericNextUseContract(rule) ||
      !body.getOps<ac::SourceObserveOp>().empty() ||
      failed(ac::verifyNumericNextUseClosure(rule)))
    return emitError()
           << "source observation analysis supports only verified U1 "
              "rules without observations";
  return success();
}

LogicalResult validateActual(InstanceView &owner, ac::RuleOp rule,
                             ac::SourceObserveOp observe, size_t requiredIndex,
                             ac::detail::EmitError emitError) {
  if (!observe || !rule ||
      rule->getParentOfType<ac::ModuleOp>() != owner.module ||
      rule->getBlock() != &owner.module.getBody().front() ||
      rule.getBody().getBlocks().size() != 1 ||
      observe->getParentOfType<ac::RuleOp>() != rule ||
      observe->getBlock() != &rule.getBody().front())
    return emitError()
           << "observation must be direct in its registered owner rule";
  if (failed(rejectUnsupportedNumericProofs(rule, emitError)))
    return failure();
  if (ac::hasNumericCompositionContract(rule) &&
      failed(ac::verifyNumericCompositionObserve(observe)))
    return failure();

  auto identity = observe->getAttrOfType<DictionaryAttr>("ac.observation_id");
  auto registration = identity ? identity.getAs<DictionaryAttr>("registration")
                               : DictionaryAttr();
  auto site = observationSite(identity);
  if (!identity || identity.size() != 2 ||
      failed(ac::detail::verifyOccurrence(registration, emitError)) ||
      failed(ac::detail::verifyOccurrence(site, emitError)) ||
      registration != rule.getRegistrationAttr())
    return emitError() << "observation identity does not match its rule";

  auto required = rule->getAttrOfType<ArrayAttr>("ac.required_observations");
  Builder builder(rule.getContext());
  if (!required || requiredIndex >= required.size() ||
      required[requiredIndex] != requiredObservation(builder, observe))
    return emitError()
           << "observation does not match its ordered required entry";

  auto valueIDs = observe->getAttrOfType<ArrayAttr>("ac.value_ids");
  bool final = isFinalObservation(observe);
  auto constraints = internalValueConstraints(builder, observe);
  auto inputTypes = rule->getAttrOfType<ArrayAttr>("ac.input_types");
  if (!valueIDs || !constraints || !inputTypes ||
      valueIDs.size() != observe.getValues().size() ||
      constraints.size() != observe.getValues().size())
    return emitError() << "observation value bindings are not closed";
  for (auto [index, value] : llvm::enumerate(observe.getValues())) {
    auto valueID = dyn_cast<DictionaryAttr>(valueIDs[index]);
    auto constraint = dyn_cast<DictionaryAttr>(constraints[index]);
    auto constraintKind =
        constraint ? constraint.getAs<StringAttr>("kind") : StringAttr();
    auto logical = constraint ? constraint.getAs<DictionaryAttr>("type")
                              : DictionaryAttr();
    auto read = value.getDefiningOp<ac::SourceReadOp>();
    auto constant = value.getDefiningOp<arith::ConstantOp>();
    auto current =
        read ? dyn_cast<BlockArgument>(read.getCurrent()) : BlockArgument();
    auto slot =
        valueID ? ac::detail::decodeU32(valueID.getAs<IntegerAttr>("slot"),
                                        "observation ValueID slot", emitError)
                : FailureOr<uint32_t>(failure());
    bool directConstant =
        ac::hasNumericCompositionContract(rule) && constant && valueID &&
        valueID.getAs<DictionaryAttr>("origin") ==
            constant->getAttrOfType<DictionaryAttr>("ac.origin") &&
        logical && logical.getAs<TypeAttr>("storage") &&
        logical.getAs<TypeAttr>("storage").getValue() == value.getType();
    auto direct = dyn_cast<BlockArgument>(value);
    bool directFinal = final && direct &&
                       direct.getOwner() == &rule.getBody().front() &&
                       direct.getArgNumber() < inputTypes.size() &&
                       inputTypes[direct.getArgNumber()] == logical;
    if (!valueID || failed(ac::detail::verifyValueID(valueID, emitError)) ||
        failed(slot) || *slot != 0 || !constraint || constraint.size() != 2 ||
        !constraintKind || constraintKind.getValue() != "logical" || !logical ||
        failed(ac::detail::verifyLogicalTypeStructure(logical, emitError)) ||
        (!directConstant && !directFinal &&
         (!read || read->getBlock() != &rule.getBody().front() || !current ||
          current.getOwner() != &rule.getBody().front() ||
          current.getArgNumber() >= inputTypes.size() ||
          inputTypes[current.getArgNumber()] != logical ||
          valueID.getAs<DictionaryAttr>("origin") !=
              read->getAttrOfType<DictionaryAttr>("ac.origin"))))
      return emitError()
             << "observation value is not its actual SourceRead binding";
  }
  return success();
}

struct ActualObservation {
  InstanceView *owner = nullptr;
  ac::RuleOp rule;
  ac::SourceObserveOp observe;
  size_t requiredIndex = 0;
};

} // namespace

FailureOr<ObservationGraph>
buildSourceObservationGraph(const ModuleGraph &modules,
                            ac::detail::EmitError emitError) {
  if (!modules.root)
    return emitError() << "observation graph requires a rooted ModuleGraph";
  ObservationGraph graph;
  graph.modules = &modules;
  DenseSet<Attribute> identities;
  Builder builder(modules.root->module.getContext());

  for (InstanceView *owner : modules.postOrder())
    for (ac::RuleOp rule :
         owner->module.getBody().front().getOps<ac::RuleOp>()) {
      if (failed(rejectUnsupportedNumericProofs(rule, emitError)))
        return failure();
      size_t requiredIndex = 0;
      for (ac::SourceObserveOp observe :
           rule.getBody().front().getOps<ac::SourceObserveOp>()) {
        if (failed(validateActual(*owner, rule, observe, requiredIndex,
                                  emitError)))
          return failure();
        ObservationBinding binding;
        binding.requiredIndex = requiredIndex++;
        binding.owner = owner;
        binding.ownerRef = owner->owner;
        binding.rule = rule;
        binding.registration = rule.getRegistrationAttr();
        binding.observationID =
            observe->getAttrOfType<DictionaryAttr>("ac.observation_id");
        binding.kind = observe.getKindAttr();
        binding.spec = observe.getSpecAttr();
        binding.valueIDs = observe->getAttrOfType<ArrayAttr>("ac.value_ids");
        binding.valueConstraints = internalValueConstraints(builder, observe);
        binding.path = observe.getPath();
        llvm::append_range(binding.values, observe.getValues());
        binding.observe = observe;
        auto key = builder.getDictionaryAttr({
            builder.getNamedAttr("owner", binding.ownerRef),
            builder.getNamedAttr("id", binding.observationID),
        });
        if (!identities.insert(key).second)
          return emitError() << "one owner repeats an observation identity";
        graph.bindings.push_back(std::move(binding));
      }
      auto required =
          rule->getAttrOfType<ArrayAttr>("ac.required_observations");
      if ((required || requiredIndex != 0) &&
          (!required || required.size() != requiredIndex))
        return emitError()
               << "rule observations do not close required observations";
    }

  llvm::sort(graph.bindings, [](const ObservationBinding &left,
                                const ObservationBinding &right) {
    return compareBindings(left, right) < 0;
  });
  for (auto [ordinal, binding] : llvm::enumerate(graph.bindings)) {
    if (ordinal && compareBindings(graph.bindings[ordinal - 1], binding) == 0)
      return emitError() << "observation structural identity is repeated";
    binding.stableOrdinal = static_cast<uint32_t>(ordinal);
  }
  return graph;
}

LogicalResult verifySourceObservations(const ObservationGraph &graph,
                                       ac::detail::EmitError emitError) {
  if (!graph.modules || !graph.modules->root)
    return emitError() << "observation graph requires a rooted ModuleGraph";
  SmallVector<ActualObservation> actual;
  for (InstanceView *owner : graph.modules->postOrder())
    for (ac::RuleOp rule :
         owner->module.getBody().front().getOps<ac::RuleOp>()) {
      if (failed(rejectUnsupportedNumericProofs(rule, emitError)))
        return failure();
      size_t index = 0;
      for (ac::SourceObserveOp observe :
           rule.getBody().front().getOps<ac::SourceObserveOp>())
        actual.push_back({owner, rule, observe, index++});
      auto required =
          rule->getAttrOfType<ArrayAttr>("ac.required_observations");
      if ((required || index != 0) && (!required || required.size() != index))
        return emitError()
               << "actual observations do not close required observations";
    }
  if (actual.size() != graph.bindings.size())
    return emitError()
           << "observation graph differs from actual observation count";

  llvm::sort(actual, [](const ActualObservation &left,
                        const ActualObservation &right) {
    ac::RuleOp leftRule = left.rule;
    ac::RuleOp rightRule = right.rule;
    ObservationBinding lhs;
    lhs.ownerRef = left.owner->owner;
    lhs.registration = leftRule.getRegistrationAttr();
    lhs.observationID =
        left.observe->getAttrOfType<DictionaryAttr>("ac.observation_id");
    ObservationBinding rhs;
    rhs.ownerRef = right.owner->owner;
    rhs.registration = rightRule.getRegistrationAttr();
    rhs.observationID =
        right.observe->getAttrOfType<DictionaryAttr>("ac.observation_id");
    return compareBindings(lhs, rhs) < 0;
  });

  DenseSet<Attribute> identities;
  Builder builder(graph.modules->root->module.getContext());
  for (size_t ordinal = 0; ordinal < actual.size(); ++ordinal) {
    const ActualObservation &current = actual[ordinal];
    const ObservationBinding &binding = graph.bindings[ordinal];
    ac::RuleOp currentRule = current.rule;
    if (failed(validateActual(*current.owner, current.rule, current.observe,
                              current.requiredIndex, emitError)))
      return failure();
    ac::SourceObserveOp observe = current.observe;
    auto identity = observe->getAttrOfType<DictionaryAttr>("ac.observation_id");
    auto key = builder.getDictionaryAttr({
        builder.getNamedAttr("owner", current.owner->owner),
        builder.getNamedAttr("id", identity),
    });
    if (!identities.insert(key).second || binding.stableOrdinal != ordinal ||
        binding.requiredIndex != current.requiredIndex ||
        binding.owner != current.owner ||
        binding.ownerRef != current.owner->owner ||
        binding.rule != current.rule || binding.observe != observe ||
        binding.registration != currentRule.getRegistrationAttr() ||
        binding.observationID != identity ||
        binding.kind != observe.getKindAttr() ||
        binding.spec != observe.getSpecAttr() ||
        binding.valueIDs != observe->getAttrOfType<ArrayAttr>("ac.value_ids") ||
        binding.valueConstraints !=
            internalValueConstraints(builder, observe) ||
        binding.path != observe.getPath() ||
        !llvm::equal(binding.values, observe.getValues()))
      return emitError()
             << "observation binding differs from actual source observation";
  }
  return success();
}

} // namespace acir::compiler
