#include "CheckGraph.h"
#include "Dialect/ACIR/ACIRNumericComposition.h"
#include "Dialect/ACIR/ACIRNumericNextUse.h"

#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"

using namespace mlir;

namespace acir::compiler {
namespace {

DictionaryAttr checkSite(DictionaryAttr identity) {
  return identity ? identity.getAs<DictionaryAttr>("check") : DictionaryAttr();
}

int compareBindings(const CheckBinding &left, const CheckBinding &right) {
  int owner =
      ac::detail::compareClosedSourceStructure(left.ownerRef, right.ownerRef);
  if (owner)
    return owner;
  int registration = ac::detail::compareClosedSourceStructure(
      left.registration, right.registration);
  if (registration)
    return registration;
  return ac::detail::compareClosedSourceStructure(checkSite(left.checkID),
                                                  checkSite(right.checkID));
}

DictionaryAttr requiredCheck(Builder &builder, ac::SourceExpectOp expect) {
  return builder.getDictionaryAttr({
      builder.getNamedAttr(
          "id", expect->getAttrOfType<DictionaryAttr>("ac.check_id")),
      builder.getNamedAttr("kind", expect.getKindAttr()),
      builder.getNamedAttr("location",
                           expect->getAttrOfType<DictionaryAttr>("location")),
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

LogicalResult verifySupportedNumericChecks(ac::RuleOp rule,
                                           ac::detail::EmitError emitError) {
  if (!hasNumericInventory(rule))
    return success();
  if (ac::hasNumericCompositionContract(rule)) {
    if (failed(ac::verifyNumericCompositionClosure(rule)))
      return emitError()
             << "source check analysis requires verified numeric composition closure";
    return success();
  }
  if (!ac::hasNumericNextUseContract(rule) ||
      failed(ac::verifyNumericNextUseClosure(rule)))
    return emitError()
           << "source check analysis supports only verified U1 numeric "
              "next-state rules";
  return success();
}

LogicalResult validateActual(InstanceView &owner, ac::RuleOp rule,
                             ac::SourceExpectOp expect, size_t requiredIndex,
                             ac::detail::EmitError emitError) {
  if (!expect || !rule ||
      rule->getParentOfType<ac::ModuleOp>() != owner.module ||
      rule->getBlock() != &owner.module.getBody().front() ||
      rule.getBody().getBlocks().size() != 1 ||
      expect->getParentOfType<ac::RuleOp>() != rule ||
      expect->getBlock() != &rule.getBody().front())
    return emitError() << "check must be direct in its registered owner rule";
  if (failed(verifySupportedNumericChecks(rule, emitError)))
    return failure();

  auto identity = expect->getAttrOfType<DictionaryAttr>("ac.check_id");
  auto registration = identity ? identity.getAs<DictionaryAttr>("registration")
                               : DictionaryAttr();
  auto site = checkSite(identity);
  auto obligation =
      identity
          ? ac::detail::decodeU64(identity.getAs<IntegerAttr>("obligation"),
                                  "CheckID obligation", emitError)
          : FailureOr<uint64_t>(failure());
  if (!identity || failed(ac::detail::verifyCheckID(identity, emitError)) ||
      failed(obligation) || *obligation != requiredIndex || !site ||
      registration != rule.getRegistrationAttr())
    return emitError()
           << "check identity does not match its ordered rule scope";

  auto required = rule->getAttrOfType<ArrayAttr>("ac.required_checks");
  Builder builder(rule.getContext());
  if (!required || requiredIndex >= required.size() ||
      required[requiredIndex] != requiredCheck(builder, expect))
    return emitError() << "check does not match its ordered required entry";
  auto location = expect->getAttrOfType<DictionaryAttr>("location");
  if (failed(ac::detail::verifySourceSpan(location, emitError)))
    return failure();
  if (ac::hasNumericCompositionContract(rule)) {
    if ((expect.getKind() != "assert" && expect.getKind() != "range") ||
        !expect.getCondition().getType().isInteger(1) ||
        !expect.getPath().getType().isInteger(1) ||
        failed(ac::verifyNumericCompositionExpect(expect)))
      return emitError()
             << "compositional source check must use verified bool condition and live path";
  } else if (ac::hasNumericNextUseContract(rule)) {
    if (expect.getKind() != "range" ||
        !expect.getCondition().getType().isInteger(1) ||
        !expect.getPath().getType().isInteger(1))
      return emitError() << "U1 range check condition/path must be bool SSA";
  } else {
    if (expect.getKind() != "assert")
      return emitError() << "source check kind must be assert";
    auto conditionRead =
        expect.getCondition().getDefiningOp<ac::SourceReadOp>();
    auto current = conditionRead
                       ? dyn_cast<BlockArgument>(conditionRead.getCurrent())
                       : BlockArgument();
    if (!conditionRead ||
        conditionRead->getBlock() != &rule.getBody().front() || !current ||
        current.getOwner() != &rule.getBody().front() ||
        !expect.getCondition().getType().isInteger(1) ||
        !expect.getPath().getType().isInteger(1))
      return emitError()
             << "source check condition/path are not direct rule bool bindings";
  }
  return success();
}

struct ActualCheck {
  InstanceView *owner = nullptr;
  ac::RuleOp rule;
  ac::SourceExpectOp expect;
  size_t requiredIndex = 0;
};

} // namespace

FailureOr<CheckGraph> buildSourceCheckGraph(const ModuleGraph &modules,
                                            ac::detail::EmitError emitError) {
  if (!modules.root)
    return emitError() << "check graph requires a rooted ModuleGraph";
  CheckGraph graph;
  graph.modules = &modules;
  DenseSet<Attribute> identities;
  Builder builder(modules.root->module.getContext());

  for (InstanceView *owner : modules.postOrder())
    for (ac::RuleOp rule :
         owner->module.getBody().front().getOps<ac::RuleOp>()) {
      if (failed(verifySupportedNumericChecks(rule, emitError)))
        return failure();
      size_t requiredIndex = 0;
      for (ac::SourceExpectOp expect :
           rule.getBody().front().getOps<ac::SourceExpectOp>()) {
        if (failed(
                validateActual(*owner, rule, expect, requiredIndex, emitError)))
          return failure();
        CheckBinding binding;
        binding.requiredIndex = requiredIndex++;
        binding.owner = owner;
        binding.ownerRef = owner->owner;
        binding.rule = rule;
        binding.registration = rule.getRegistrationAttr();
        binding.expect = expect;
        binding.checkID = expect->getAttrOfType<DictionaryAttr>("ac.check_id");
        binding.kind = expect.getKindAttr();
        binding.location = expect->getAttrOfType<DictionaryAttr>("location");
        binding.condition = expect.getCondition();
        binding.path = expect.getPath();
        auto key = builder.getDictionaryAttr({
            builder.getNamedAttr("owner", binding.ownerRef),
            builder.getNamedAttr("id", binding.checkID),
        });
        if (!identities.insert(key).second)
          return emitError() << "one owner repeats a check identity";
        graph.bindings.push_back(std::move(binding));
      }
      auto required = rule->getAttrOfType<ArrayAttr>("ac.required_checks");
      if ((required || requiredIndex != 0) &&
          (!required || required.size() != requiredIndex))
        return emitError() << "rule checks do not close required checks";
    }

  llvm::sort(graph.bindings,
             [](const CheckBinding &left, const CheckBinding &right) {
               return compareBindings(left, right) < 0;
             });
  for (auto [ordinal, binding] : llvm::enumerate(graph.bindings)) {
    if (ordinal && compareBindings(graph.bindings[ordinal - 1], binding) == 0)
      return emitError() << "check structural identity is repeated";
    binding.stableOrdinal = static_cast<uint32_t>(ordinal);
  }
  return graph;
}

LogicalResult verifySourceChecks(const CheckGraph &graph,
                                 ac::detail::EmitError emitError) {
  if (!graph.modules || !graph.modules->root)
    return emitError() << "check graph requires a rooted ModuleGraph";
  SmallVector<ActualCheck> actual;
  for (InstanceView *owner : graph.modules->postOrder())
    for (ac::RuleOp rule :
         owner->module.getBody().front().getOps<ac::RuleOp>()) {
      if (failed(verifySupportedNumericChecks(rule, emitError)))
        return failure();
      size_t index = 0;
      for (ac::SourceExpectOp expect :
           rule.getBody().front().getOps<ac::SourceExpectOp>())
        actual.push_back({owner, rule, expect, index++});
      auto required = rule->getAttrOfType<ArrayAttr>("ac.required_checks");
      if ((required || index != 0) && (!required || required.size() != index))
        return emitError() << "actual checks do not close required checks";
    }
  if (actual.size() != graph.bindings.size())
    return emitError() << "check graph differs from actual check count";

  llvm::sort(actual, [](const ActualCheck &left, const ActualCheck &right) {
    ac::RuleOp leftRule = left.rule;
    ac::RuleOp rightRule = right.rule;
    CheckBinding lhs;
    lhs.ownerRef = left.owner->owner;
    lhs.registration = leftRule.getRegistrationAttr();
    lhs.checkID = left.expect->getAttrOfType<DictionaryAttr>("ac.check_id");
    CheckBinding rhs;
    rhs.ownerRef = right.owner->owner;
    rhs.registration = rightRule.getRegistrationAttr();
    rhs.checkID = right.expect->getAttrOfType<DictionaryAttr>("ac.check_id");
    return compareBindings(lhs, rhs) < 0;
  });

  DenseSet<Attribute> identities;
  Builder builder(graph.modules->root->module.getContext());
  for (size_t ordinal = 0; ordinal < actual.size(); ++ordinal) {
    const ActualCheck &current = actual[ordinal];
    const CheckBinding &binding = graph.bindings[ordinal];
    ac::RuleOp currentRule = current.rule;
    ac::SourceExpectOp currentExpect = current.expect;
    if (failed(validateActual(*current.owner, current.rule, current.expect,
                              current.requiredIndex, emitError)))
      return failure();
    auto identity =
        current.expect->getAttrOfType<DictionaryAttr>("ac.check_id");
    auto key = builder.getDictionaryAttr({
        builder.getNamedAttr("owner", current.owner->owner),
        builder.getNamedAttr("id", identity),
    });
    if (!identities.insert(key).second || binding.stableOrdinal != ordinal ||
        binding.requiredIndex != current.requiredIndex ||
        binding.owner != current.owner ||
        binding.ownerRef != current.owner->owner ||
        binding.rule != current.rule || binding.expect != current.expect ||
        binding.registration != currentRule.getRegistrationAttr() ||
        binding.checkID != identity ||
        binding.kind != currentExpect.getKindAttr() ||
        binding.location !=
            currentExpect->getAttrOfType<DictionaryAttr>("location") ||
        binding.condition != currentExpect.getCondition() ||
        binding.path != currentExpect.getPath())
      return emitError() << "check binding differs from actual source check";
  }
  return success();
}

} // namespace acir::compiler
