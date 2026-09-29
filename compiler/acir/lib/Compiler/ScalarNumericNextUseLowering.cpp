#include "ScalarNumericLoweringDetail.h"

#include "Dialect/ACIR/ACIRNumericNextUse.h"
#include "acir/Dialect/ACIR/ACIRAttributes.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "llvm/ADT/DenseSet.h"

using namespace mlir;

namespace acir::compiler {
namespace {

APSInt mathValue(ac::MathIntAttr attr) {
  StringRef text = attr.getCanonicalValue();
  bool negative = text.consume_front("-");
  unsigned width = std::max(2u, unsigned(text.size() * 4 + 2));
  APInt bits(width, text, 10);
  if (negative)
    bits = -bits;
  return APSInt(std::move(bits), !negative);
}

DictionaryAttr singletonDomain(Builder &builder, ac::MathIntAttr value) {
  APSInt lower = mathValue(value);
  APSInt upper = lower.extend(lower.getBitWidth() + 1);
  ++upper;
  unsigned width = std::max(1u, lower.getActiveBits());
  return builder.getDictionaryAttr({
      builder.getNamedAttr("kind", builder.getStringAttr("integer")),
      builder.getNamedAttr("storage",
                           TypeAttr::get(builder.getIntegerType(width))),
      builder.getNamedAttr("lower",
                           ac::MathIntAttr::get(builder.getContext(), lower)),
      builder.getNamedAttr("upper",
                           ac::MathIntAttr::get(builder.getContext(), upper)),
      builder.getNamedAttr("interpretation", builder.getStringAttr("unsigned")),
  });
}

ac::ValueBindingOp bind(OpBuilder &builder, Location location, Value value,
                        Value valid, Value path, DictionaryAttr id,
                        DictionaryAttr domain) {
  OperationState state(location, ac::ValueBindingOp::getOperationName());
  state.addOperands({value, valid, path});
  state.addAttribute("id", id);
  state.addAttribute("domain", domain);
  return cast<ac::ValueBindingOp>(builder.create(state));
}

ac::NumericProofOp proof(OpBuilder &builder, Location location, StringRef mode,
                         ArrayRef<Value> operands, ArrayRef<int32_t> segments,
                         DictionaryAttr resultDomain, ArrayAttr inputIDs,
                         ArrayAttr inputDomains, DictionaryAttr resultID,
                         ArrayAttr obligations, ArrayAttr checks,
                         DictionaryAttr origin, IntegerAttr width = {}) {
  OperationState state(location, ac::NumericProofOp::getOperationName());
  state.addOperands(operands);
  state.addAttribute("operand_segment_sizes",
                     builder.getDenseI32ArrayAttr(segments));
  state.addAttribute("mode", builder.getStringAttr(mode));
  state.addAttribute("result_domain", resultDomain);
  state.addAttribute("input_ids", inputIDs);
  state.addAttribute("input_domains", inputDomains);
  state.addAttribute("result_id", resultID);
  state.addAttribute("obligations", obligations);
  state.addAttribute("checks", checks);
  state.addAttribute("origin", origin);
  if (width)
    state.addAttribute("width", width);
  return cast<ac::NumericProofOp>(builder.create(state));
}

} // namespace

LogicalResult lowerNumericNextUseRule(ModuleOp unit, ac::RuleOp rule) {
  if (!unit || !rule || rule->getParentOfType<ModuleOp>() != unit ||
      failed(ac::verifyNumericNextUseClosure(rule)))
    return failure();
  Block &body = rule.getBody().front();
  SmallVector<ac::SourceReadOp> reads(body.getOps<ac::SourceReadOp>());
  SmallVector<ac::MathFromBitsOp> from(body.getOps<ac::MathFromBitsOp>());
  SmallVector<ac::MathConstantOp> constants(body.getOps<ac::MathConstantOp>());
  SmallVector<ac::MathBinaryOp> binaries(body.getOps<ac::MathBinaryOp>());
  SmallVector<ac::MathToBitsOp> boundaries(body.getOps<ac::MathToBitsOp>());
  SmallVector<ac::SourceUseOp> sourceUses(body.getOps<ac::SourceUseOp>());
  SmallVector<ac::SourceExpectOp> expects(body.getOps<ac::SourceExpectOp>());
  if (reads.size() != 1 || from.size() != 1 || constants.size() != 2 ||
      binaries.size() != 2 || boundaries.size() != 1 ||
      sourceUses.size() != 1 || expects.size() != 1)
    return rule.emitOpError() << "U1 lowering received a changed source packet";

  Builder attrs(unit.getContext());
  ArrayAttr required = rule->getAttrOfType<ArrayAttr>("ac.required_numeric");
  auto nodeID = [&](unsigned index) {
    return cast<DictionaryAttr>(required[index]).getAs<DictionaryAttr>("id");
  };
  auto inputDomain =
      cast<DictionaryAttr>(rule->getAttrOfType<ArrayAttr>("ac.input_types")[0]);
  auto resultDomain = boundaries[0].getDomainAttr();
  auto oneDomain = singletonDomain(
      attrs, constants[0]->getAttrOfType<ac::MathIntAttr>("value"));
  auto maskDomain = singletonDomain(
      attrs, constants[1]->getAttrOfType<ac::MathIntAttr>("value"));
  auto oneType =
      cast<IntegerType>(oneDomain.getAs<TypeAttr>("storage").getValue());
  auto maskType =
      cast<IntegerType>(maskDomain.getAs<TypeAttr>("storage").getValue());
  auto resultType =
      cast<IntegerType>(resultDomain.getAs<TypeAttr>("storage").getValue());
  DictionaryAttr inputID = attrs.getDictionaryAttr({
      attrs.getNamedAttr("origin",
                         reads[0]->getAttrOfType<DictionaryAttr>("ac.origin")),
      attrs.getNamedAttr("slot", attrs.getI32IntegerAttr(0)),
  });
  auto requiredUse = cast<DictionaryAttr>(
      rule->getAttrOfType<ArrayAttr>("ac.required_uses")[0]);
  auto useID = requiredUse.getAs<DictionaryAttr>("id");
  auto checkID = expects[0]->getAttrOfType<DictionaryAttr>("ac.check_id");
  auto checkBinding = attrs.getDictionaryAttr({
      attrs.getNamedAttr("id", checkID),
      attrs.getNamedAttr("owner", nodeID(5)),
      attrs.getNamedAttr("kind", attrs.getStringAttr("range")),
      attrs.getNamedAttr("operand_ordinal", attrs.getI32IntegerAttr(0)),
  });

  auto yield = cast<ac::YieldOp>(body.getTerminator());
  OpBuilder builder(expects[0]);
  Value one = arith::ConstantOp::create(builder, boundaries[0].getLoc(),
                                        builder.getBoolAttr(true));
  auto finiteOne = arith::ConstantOp::create(
      builder, constants[0].getLoc(),
      IntegerAttr::get(oneType, APInt(oneType.getWidth(), 1)));
  auto finiteMask = arith::ConstantOp::create(
      builder, constants[1].getLoc(),
      IntegerAttr::get(maskType, APInt(maskType.getWidth(), 255)));
  Value addRhs = finiteOne;
  if (oneType != resultType)
    addRhs = arith::ExtUIOp::create(builder, constants[0].getLoc(), resultType,
                                    finiteOne);
  auto add = arith::AddIOp::create(builder, binaries[0].getLoc(), resultType,
                                   reads[0].getResult(), addRhs);
  Value maskRhs = finiteMask;
  if (maskType != resultType)
    maskRhs = arith::ExtUIOp::create(builder, constants[1].getLoc(), resultType,
                                     finiteMask);
  auto masked =
      arith::AndIOp::create(builder, binaries[1].getLoc(), add, maskRhs);

  auto inputBinding = bind(builder, reads[0].getLoc(), reads[0].getResult(),
                           one, one, inputID, inputDomain);
  (void)bind(builder, from[0].getLoc(), reads[0].getResult(), one, one,
             nodeID(0), inputDomain);
  (void)bind(builder, constants[0].getLoc(), finiteOne, one, one, nodeID(1),
             oneDomain);
  (void)bind(builder, constants[1].getLoc(), finiteMask, one, one, nodeID(3),
             maskDomain);
  auto maskBinding = bind(builder, binaries[1].getLoc(), masked, one, one,
                          nodeID(4), resultDomain);
  auto boundaryBinding = bind(builder, boundaries[0].getLoc(), masked, one, one,
                              nodeID(5), resultDomain);

  auto lowObligations = attrs.getArrayAttr(required.getValue().take_front(5));
  (void)proof(builder, binaries[1].getLoc(), "low_bits",
              {one, inputBinding.getValue(), one, masked, one},
              {1, 1, 1, 1, 1, 0, 0}, resultDomain,
              attrs.getArrayAttr({inputID}), attrs.getArrayAttr({inputDomain}),
              nodeID(4), lowObligations, attrs.getArrayAttr({}),
              binaries[1]->getAttrOfType<DictionaryAttr>("ac.origin"),
              attrs.getI32IntegerAttr(8));
  auto boundaryProof = proof(
      builder, boundaries[0].getLoc(), "exact",
      {one, maskBinding.getValue(), one, boundaryBinding.getValue(), one, one,
       one},
      {1, 1, 1, 1, 1, 1, 1}, resultDomain, attrs.getArrayAttr({nodeID(4)}),
      attrs.getArrayAttr({resultDomain}), nodeID(5),
      attrs.getArrayAttr({required[5]}), attrs.getArrayAttr({checkBinding}),
      boundaries[0]->getAttrOfType<DictionaryAttr>("ac.origin"));
  (void)boundaryProof;

  expects[0].getConditionMutable().assign(one);
  expects[0].getPathMutable().assign(one);
  OperationState useState(sourceUses[0].getLoc(),
                          ac::ValueUseOp::getOperationName());
  useState.addOperands({boundaryBinding.getValue(), boundaryBinding.getValid(),
                        sourceUses[0].getPath()});
  useState.addAttribute("id", useID);
  useState.addAttribute("source", nodeID(5));
  auto use = cast<ac::ValueUseOp>(builder.create(useState));
  Value enabled = arith::AndIOp::create(builder, sourceUses[0].getLoc(),
                                        use.getValid(), use.getPath());
  yield.getValuesMutable().assign({use.getValue(), enabled});

  llvm::DenseSet<Operation *> controls;
  for (auto binary : binaries)
    for (Value control :
         {binary.getPath(), binary.getLhsValid(), binary.getRhsValid()})
      if (auto producer = control.getDefiningOp<arith::ConstantOp>())
        controls.insert(producer);
  if (auto producer =
          boundaries[0].getPath().getDefiningOp<arith::ConstantOp>())
    controls.insert(producer);
  sourceUses[0].erase();
  boundaries[0].erase();
  for (auto binary : llvm::reverse(binaries))
    binary.erase();
  for (auto constant : constants)
    constant.erase();
  from[0].erase();
  for (Operation *control : controls)
    if (control != one.getDefiningOp() && control->getNumResults() == 1 &&
        control->getResult(0).use_empty())
      control->erase();
  return ac::verifyNumericNextUseClosure(rule);
}

} // namespace acir::compiler
