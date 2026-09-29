#include "FinalComposedNumeric.h"
#include "Dialect/ACIR/ACIRNumericComposition.h"
#include "llvm/ADT/STLExtras.h"

using namespace mlir;
namespace acir::compiler {
namespace {
DictionaryAttr attributes(Operation *op) {
  return DictionaryAttr::get(op->getContext(), op->getAttrs());
}
bool retained(Operation *op) {
  return isa<ac::ValueBindingOp, ac::NumericProofOp, ac::ValueUseOp,
             ac::YieldOp>(op);
}
} // namespace

FailureOr<FinalNumericRuleSnapshot>
freezeComposedNumeric(InstanceView &owner, ac::RuleOp rule,
                      ac::detail::EmitError error) {
  if (!ac::hasNumericCompositionContract(rule) ||
      failed(ac::verifyNumericCompositionClosure(rule)))
    return error() << "composed numeric rule is not independently verified";
  FinalNumericRuleSnapshot snapshot;
  snapshot.composition = true;
  snapshot.owner = &owner;
  snapshot.rule = rule;
  snapshot.recipe = rule->getAttrOfType<ArrayAttr>("ac.required_numeric");
  snapshot.requiredUses = rule->getAttrOfType<ArrayAttr>("ac.required_uses");
  snapshot.yieldBindings = rule->getAttrOfType<ArrayAttr>("ac.yield_bindings");
  snapshot.proofScope = rule->getAttrOfType<DictionaryAttr>("ac.proof_scope");
  for (Operation &op : rule.getBody().front()) {
    if (!retained(&op))
      continue;
    FinalNumericCarrierSnapshot carrier;
    carrier.operation = &op;
    carrier.attributes = attributes(&op);
    carrier.operands.append(op.operand_begin(), op.operand_end());
    carrier.results.append(op.getResultTypes().begin(),
                           op.getResultTypes().end());
    snapshot.composedCarriers.push_back(std::move(carrier));
  }
  return snapshot;
}

LogicalResult verifyComposedNumeric(const FinalNumericRuleSnapshot &snapshot,
                                    const ModuleGraph &modules,
                                    ac::detail::EmitError error) {
  if (!snapshot.owner || !snapshot.rule || !snapshot.composition ||
      !llvm::is_contained(modules.views, snapshot.owner))
    return error() << "composed final numeric owner is missing";
  ac::RuleOp rule = snapshot.rule;
  if (rule->getParentOfType<ac::ModuleOp>() != snapshot.owner->module ||
      rule->getAttrOfType<ArrayAttr>("ac.required_numeric") !=
          snapshot.recipe ||
      rule->getAttrOfType<ArrayAttr>("ac.required_uses") !=
          snapshot.requiredUses ||
      rule->getAttrOfType<ArrayAttr>("ac.yield_bindings") !=
          snapshot.yieldBindings ||
      rule->getAttrOfType<DictionaryAttr>("ac.proof_scope") !=
          snapshot.proofScope)
    return error() << "composed final numeric authority changed";
  SmallVector<Operation *> live;
  for (Operation &op : rule.getBody().front())
    if (retained(&op))
      live.push_back(&op);
  if (live.size() != snapshot.composedCarriers.size())
    return error() << "composed final evidence inventory changed";
  for (auto [index, saved] : llvm::enumerate(snapshot.composedCarriers)) {
    // Compare live membership before dereferencing a potentially removed op.
    if (live[index] != saved.operation)
      return error() << "composed final evidence identity or order changed";
    Operation *op = live[index];
    if (attributes(op) != saved.attributes ||
        !llvm::equal(op->getOperands(), saved.operands) ||
        !llvm::equal(op->getResultTypes(), saved.results))
      return error()
             << "composed final evidence operands or attributes changed";
  }
  return ac::verifyNumericCompositionClosure(rule);
}
} // namespace acir::compiler
