#include "FinalUses.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "llvm/ADT/DenseMap.h"

using namespace mlir;
namespace acir::compiler {
namespace {
bool trueI1(Value value) {
  auto constant = value.getDefiningOp<arith::ConstantOp>();
  auto attr =
      constant ? dyn_cast<IntegerAttr>(constant.getValue()) : IntegerAttr();
  return attr && attr.getType().isInteger(1) && attr.getValue().isOne();
}
} // namespace

LogicalResult retainGenericSourceUses(ac::RuleOp rule,
                                      DenseMap<Value, Value> &replacements) {
  auto requiredNumeric = rule->getAttrOfType<ArrayAttr>("ac.required_numeric");
  if (requiredNumeric && !requiredNumeric.empty())
    return success();
  auto &body = rule.getBody().front();
  SmallVector<ac::SourceUseOp> uses(body.getOps<ac::SourceUseOp>());
  if (uses.empty())
    return success();
  auto emit = [&] { return rule.emitOpError(); };
  auto outputs = rule->getAttrOfType<ArrayAttr>("ac.output_bindings");
  auto types = rule->getAttrOfType<ArrayAttr>("ac.output_types");
  auto yield = dyn_cast<ac::YieldOp>(body.getTerminator());
  if (!outputs || !types || outputs.size() != types.size() ||
      outputs.size() != uses.size() || !yield ||
      yield.getValues().size() != 2 * uses.size() ||
      !body.getOps<ac::ValueUseOp>().empty() ||
      !body.getOps<ac::ValueBindingOp>().empty())
    return emit()
           << "generic use retention requires one source assignment per target";
  DenseMap<Attribute, ac::ValueBindingOp> values;
  SmallVector<Attribute> required, yieldBindings(outputs.size());
  Builder attrs(rule.getContext());
  auto module = rule->getParentOfType<ac::ModuleOp>();
  auto key = attrs.getDictionaryAttr(
      {attrs.getNamedAttr(
           "definition",
           FlatSymbolRefAttr::get(rule.getContext(), module.getSymName())),
       attrs.getNamedAttr("arguments", attrs.getArrayAttr({}))});
  rule->setAttr(
      "ac.proof_scope",
      attrs.getDictionaryAttr(
          {attrs.getNamedAttr("specialization", key),
           attrs.getNamedAttr("registration", rule.getRegistrationAttr())}));
  for (auto use : uses) {
    auto state = use.getTargetAttr().getAs<DictionaryAttr>("state");
    auto position = llvm::find(outputs.getValue(), state);
    if (position == outputs.end() || !trueI1(use.getValid()))
      return emit() << "generic use retention requires exact target and valid "
                       "pure value";
    size_t ordinal = position - outputs.begin();
    if (yieldBindings[ordinal] ||
        yield.getValues()[2 * ordinal] != use.getData() ||
        yield.getValues()[2 * ordinal + 1] != use.getEnabled())
      return emit()
             << "generic source assignment is not the exact target yield";
    if (!use.getValue().getDefiningOp<ac::SourceReadOp>() &&
        !use.getValue().getDefiningOp<arith::ConstantOp>())
      return emit()
             << "generic use retention admits direct current or literal values";
    auto domain = dyn_cast<DictionaryAttr>(types[ordinal]);
    auto source = use.getSourceAttr();
    OpBuilder builder(use);
    auto found = values.find(source);
    if (found != values.end()) {
      if (found->second.getValue() != use.getValue() ||
          found->second.getDomainAttr() != domain)
        return emit() << "shared source ValueID disagrees with its actual "
                         "value/domain";
      // All admitted pure reads/literals have a proven-true validity. Reuse
      // that same true SSA for shared source identities and their yields.
      Value oldValid = use.getValid();
      Value canonicalValid = found->second.getValid();
      if (oldValid != canonicalValid) {
        replacements.try_emplace(oldValid, canonicalValid);
        oldValid.replaceAllUsesWith(canonicalValid);
      }
    } else {
      OperationState binding(use.getValue().getLoc(),
                             ac::ValueBindingOp::getOperationName());
      binding.addOperands({use.getValue(), use.getValid(), use.getValid()});
      binding.addAttribute("id", source);
      binding.addAttribute("domain", domain);
      values.try_emplace(source,
                         cast<ac::ValueBindingOp>(builder.create(binding)));
    }
    OperationState marker(use.getLoc(), ac::ValueUseOp::getOperationName());
    marker.addOperands({use.getValue(), use.getValid(), use.getPath()});
    marker.addAttribute("id", use.getIdAttr());
    marker.addAttribute("source", source);
    builder.create(marker);
    required.push_back(attrs.getDictionaryAttr(
        {attrs.getNamedAttr("id", use.getIdAttr()),
         attrs.getNamedAttr("value", source),
         attrs.getNamedAttr("target", use.getTargetAttr())}));
    auto contribution = attrs.getDictionaryAttr(
        {attrs.getNamedAttr("use", use.getIdAttr()),
         attrs.getNamedAttr("selection_ordinal", attrs.getUnitAttr())});
    yieldBindings[ordinal] = attrs.getDictionaryAttr(
        {attrs.getNamedAttr("data_operand",
                            attrs.getI32IntegerAttr(2 * ordinal)),
         attrs.getNamedAttr("enable_operand",
                            attrs.getI32IntegerAttr(2 * ordinal + 1)),
         attrs.getNamedAttr("target", state),
         attrs.getNamedAttr("contributions",
                            attrs.getArrayAttr({contribution}))});
  }
  rule->setAttr("ac.required_uses", attrs.getArrayAttr(required));
  rule->setAttr("ac.yield_bindings", attrs.getArrayAttr(yieldBindings));
  return success();
}
} // namespace acir::compiler
