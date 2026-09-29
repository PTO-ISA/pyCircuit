#include "ScalarNumericLowering.h"
#include "ScalarNumericLoweringDetail.h"

#include "Dialect/ACIR/ACIRNumericProof.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/IR/Verifier.h"
#include "llvm/ADT/APSInt.h"

#include <algorithm>

using namespace mlir;

namespace acir::compiler {
namespace {

APSInt mathValue(ac::MathIntAttr value) {
  return APSInt(value.getCanonicalValue());
}

APInt extendNumeric(const APSInt &value, unsigned width) {
  return value.isUnsigned() ? value.zextOrTrunc(width)
                            : value.sextOrTrunc(width);
}

int compareMath(const APSInt &lhs, const APSInt &rhs) {
  unsigned width = std::max(lhs.getBitWidth(), rhs.getBitWidth()) + 1;
  APInt left = extendNumeric(lhs, width);
  APInt right = extendNumeric(rhs, width);
  if (left == right)
    return 0;
  return left.slt(right) ? -1 : 1;
}

APSInt addMath(const APSInt &lhs, const APSInt &rhs) {
  unsigned width = std::max(lhs.getBitWidth(), rhs.getBitWidth()) + 1;
  APInt sum = extendNumeric(lhs, width) + extendNumeric(rhs, width);
  bool negative =
      lhs.isNegative() && rhs.isNegative()
          ? true
          : lhs.isNegative() != rhs.isNegative() && sum.isNegative();
  return APSInt(std::move(sum), /*isUnsigned=*/!negative);
}

APSInt powerOfTwo(unsigned exponent, bool negative = false) {
  APInt bits(exponent + 2, 0);
  bits.setBit(exponent);
  if (negative)
    bits = -bits;
  return APSInt(std::move(bits), /*isUnsigned=*/!negative);
}

FailureOr<unsigned> minimumDomainWidth(const APSInt &lower, const APSInt &upper,
                                       Operation *owner) {
  auto emit = [&] { return owner->emitOpError(); };
  if (compareMath(lower, upper) >= 0)
    return emit() << "numeric interval must satisfy lower < upper";
  bool nonNegative = !lower.isNegative();
  for (unsigned width = 1; width <= 64; ++width) {
    if (compareMath(upper, powerOfTwo(nonNegative ? width : width - 1)) > 0)
      continue;
    if (!nonNegative && compareMath(lower, powerOfTwo(width - 1, true)) < 0)
      continue;
    return width;
  }
  return emit() << "numeric interval requires unsupported i65 storage";
}

DictionaryAttr makeIntegerDomain(Builder &builder, const APSInt &lower,
                                 const APSInt &upper, Operation *owner) {
  auto width = minimumDomainWidth(lower, upper, owner);
  if (failed(width))
    return {};
  auto domain = builder.getDictionaryAttr({
      builder.getNamedAttr("kind", builder.getStringAttr("integer")),
      builder.getNamedAttr("storage",
                           TypeAttr::get(builder.getIntegerType(*width))),
      builder.getNamedAttr("lower",
                           ac::MathIntAttr::get(builder.getContext(), lower)),
      builder.getNamedAttr("upper",
                           ac::MathIntAttr::get(builder.getContext(), upper)),
      builder.getNamedAttr(
          "interpretation",
          builder.getStringAttr(lower.isNegative() ? "signed" : "unsigned")),
  });
  if (failed(ac::detail::verifyLogicalTypeStructure(
          domain, [&] { return owner->emitOpError(); })))
    return {};
  return domain;
}

DictionaryAttr inputValueID(Builder &builder, DictionaryAttr origin) {
  return builder.getDictionaryAttr({
      builder.getNamedAttr("origin", origin),
      builder.getNamedAttr("slot", builder.getI32IntegerAttr(0)),
  });
}

struct LoweringPlan {
  DictionaryAttr inputDomain;
  DictionaryAttr constantDomain;
  DictionaryAttr resultDomain;
  IntegerType constantType;
  IntegerType resultType;
  IntegerAttr constantBits;
};

FailureOr<LoweringPlan> makePlan(ModuleOp unit,
                                 detail::RuleInventory &inventory) {
  auto emit = [&] { return inventory.add.emitOpError(); };
  DictionaryAttr inputDomain = inventory.fromBits.getDomainAttr();
  auto inputLowerAttr = inputDomain.getAs<ac::MathIntAttr>("lower");
  auto inputUpperAttr = inputDomain.getAs<ac::MathIntAttr>("upper");
  auto storage = inputDomain.getAs<TypeAttr>("storage");
  auto constantAttr =
      inventory.constant->getAttrOfType<ac::MathIntAttr>("value");
  if (!inputLowerAttr || !inputUpperAttr || !storage || !constantAttr)
    return emit() << "N0-C1 interval operands are incomplete";
  APSInt inputLower = mathValue(inputLowerAttr);
  APSInt inputUpper = mathValue(inputUpperAttr);
  APSInt constantValue = mathValue(constantAttr);
  APSInt constantUpper = addMath(constantValue, APSInt("1"));
  APSInt resultLower = addMath(inputLower, constantValue);
  APSInt resultUpper = addMath(inputUpper, constantValue);
  Builder builder(unit.getContext());
  DictionaryAttr constantDomain = makeIntegerDomain(
      builder, constantValue, constantUpper, inventory.constant);
  DictionaryAttr resultDomain =
      makeIntegerDomain(builder, resultLower, resultUpper, inventory.add);
  if (!constantDomain || !resultDomain)
    return failure();
  auto constantType = dyn_cast<IntegerType>(
      constantDomain.getAs<TypeAttr>("storage").getValue());
  auto resultType =
      dyn_cast<IntegerType>(resultDomain.getAs<TypeAttr>("storage").getValue());
  auto inputType = dyn_cast<IntegerType>(storage.getValue());
  if (!constantType || !resultType || !inputType ||
      resultType.getWidth() < inputType.getWidth() ||
      resultType.getWidth() < constantType.getWidth())
    return emit() << "N0-C1 exact result rejects narrowing and i65";
  APInt bits = constantValue.isUnsigned()
                   ? constantValue.zextOrTrunc(constantType.getWidth())
                   : constantValue.sextOrTrunc(constantType.getWidth());
  return LoweringPlan{inputDomain,  constantDomain,
                      resultDomain, constantType,
                      resultType,   IntegerAttr::get(constantType, bits)};
}

ac::ValueBindingOp createBinding(OpBuilder &builder, Location location,
                                 Value value, Value valid, Value path,
                                 DictionaryAttr id, DictionaryAttr domain) {
  OperationState state(location, ac::ValueBindingOp::getOperationName());
  state.addOperands({value, valid, path});
  state.addAttribute("id", id);
  state.addAttribute("domain", domain);
  return cast<ac::ValueBindingOp>(builder.create(state));
}

Value extendValue(OpBuilder &builder, Location location, Value value,
                  IntegerType resultType, StringRef interpretation) {
  auto sourceType = cast<IntegerType>(value.getType());
  if (sourceType.getWidth() == resultType.getWidth())
    return value;
  if (interpretation == "signed")
    return arith::ExtSIOp::create(builder, location, resultType, value);
  return arith::ExtUIOp::create(builder, location, resultType, value);
}

LogicalResult lowerRule(ModuleOp unit, detail::RuleInventory &inventory) {
  auto plan = makePlan(unit, inventory);
  if (failed(plan))
    return failure();
  ac::RuleOp rule = inventory.rule;
  Builder attrBuilder(unit.getContext());
  DictionaryAttr inputID = inputValueID(
      attrBuilder, inventory.read->getAttrOfType<DictionaryAttr>("ac.origin"));
  ArrayAttr required = rule->getAttrOfType<ArrayAttr>("ac.required_numeric");
  auto fromID = cast<DictionaryAttr>(required[0]).getAs<DictionaryAttr>("id");
  auto constantID =
      cast<DictionaryAttr>(required[1]).getAs<DictionaryAttr>("id");
  auto addID = cast<DictionaryAttr>(required[2]).getAs<DictionaryAttr>("id");

  Block &body = rule.getBody().front();
  auto yield = cast<ac::YieldOp>(body.getTerminator());
  OpBuilder builder(yield);
  Location location = inventory.add.getLoc();
  Value one = inventory.add.getPath();
  SmallVector<Operation *> controls;
  DenseSet<Operation *> seenControls;
  for (Value control : {inventory.add.getPath(), inventory.add.getLhsValid(),
                        inventory.add.getRhsValid()}) {
    auto constant = control.getDefiningOp<arith::ConstantOp>();
    if (constant && seenControls.insert(constant.getOperation()).second)
      controls.push_back(constant.getOperation());
  }
  auto constant = arith::ConstantOp::create(
      builder, inventory.constant.getLoc(), plan->constantBits);
  Value input = inventory.read.getResult();
  Value extendedInput = extendValue(
      builder, location, input, plan->resultType,
      plan->inputDomain.getAs<StringAttr>("interpretation").getValue());
  Value extendedConstant = extendValue(
      builder, inventory.constant.getLoc(), constant.getResult(),
      plan->resultType,
      plan->constantDomain.getAs<StringAttr>("interpretation").getValue());
  auto sum = arith::AddIOp::create(builder, location, plan->resultType,
                                   extendedInput, extendedConstant);
  (void)createBinding(builder, location, input, one, one, inputID,
                      plan->inputDomain);
  (void)createBinding(builder, location, input, one, one, fromID,
                      plan->inputDomain);
  (void)createBinding(builder, inventory.constant.getLoc(),
                      constant.getResult(), one, one, constantID,
                      plan->constantDomain);
  (void)createBinding(builder, location, sum.getResult(), one, one, addID,
                      plan->resultDomain);

  OperationState proofState(location, ac::NumericProofOp::getOperationName());
  proofState.addOperands({one, input, one, sum.getResult(), one});
  proofState.addAttribute("operand_segment_sizes",
                          builder.getDenseI32ArrayAttr({1, 1, 1, 1, 1, 0, 0}));
  proofState.addAttribute("mode", builder.getStringAttr("exact"));
  proofState.addAttribute("result_domain", plan->resultDomain);
  proofState.addAttribute("input_ids", builder.getArrayAttr({inputID}));
  proofState.addAttribute("input_domains",
                          builder.getArrayAttr({plan->inputDomain}));
  proofState.addAttribute("result_id", addID);
  proofState.addAttribute("obligations", required);
  proofState.addAttribute("checks", builder.getArrayAttr({}));
  proofState.addAttribute(
      "origin", inventory.add->getAttrOfType<DictionaryAttr>("ac.origin"));
  builder.create(proofState);

  inventory.add.erase();
  inventory.constant.erase();
  inventory.fromBits.erase();
  Operation *retainedControl = one.getDefiningOp();
  for (Operation *control : controls)
    if (control != retainedControl && control->getResult(0).use_empty())
      control->erase();
  return success();
}

LogicalResult lowerClone(ModuleOp clone) {
  auto emit = [&] { return clone.emitError(); };
  auto unitInventory = detail::inspectUnit(clone, emit);
  if (failed(unitInventory) ||
      unitInventory->kind != detail::UnitInventoryKind::Source)
    return failure();
  for (ac::RuleOp rule : unitInventory->sourceRules) {
    auto inventory = detail::inspectRule(rule);
    if (failed(inventory))
      return failure();
    if (inventory->kind == detail::RuleInventoryKind::ExactInputScalar) {
      if (failed(lowerExactInputScalarRule(clone, rule)))
        return failure();
      continue;
    }
    if (inventory->kind == detail::RuleInventoryKind::InputMask) {
      if (failed(lowerInputMaskRule(clone, rule)))
        return failure();
      continue;
    }
    if (inventory->kind == detail::RuleInventoryKind::CheckedToBits) {
      if (failed(lowerCheckedToBitsRule(clone, rule)))
        return failure();
      continue;
    }
    if (inventory->kind == detail::RuleInventoryKind::NumericNextUse) {
      if (failed(lowerNumericNextUseRule(clone, rule)))
        return failure();
      continue;
    }
    if (inventory->kind == detail::RuleInventoryKind::Composition) {
      if (failed(lowerNumericCompositionRule(clone, rule)))
        return failure();
      continue;
    }
    if (failed(lowerRule(clone, *inventory)))
      return failure();
  }
  if (failed(verify(clone)))
    return clone.emitError() << "N0-C1 lowered clone does not verify";
  auto closed = detail::inspectUnit(clone, emit);
  if (failed(closed) || closed->kind != detail::UnitInventoryKind::Lowered)
    return clone.emitError() << "N0-C1 clone did not reach lowered witnesses";
  return success();
}

} // namespace

LogicalResult lowerExactInputAddTransactional(ModuleOp unit) {
  if (!unit)
    return failure();
  auto emit = [&] { return unit.emitError(); };
  if (failed(verify(unit)))
    return emit() << "N0-C1 input unit does not verify";
  auto inventory = detail::inspectUnit(unit, emit);
  if (failed(inventory))
    return failure();
  if (inventory->kind != detail::UnitInventoryKind::Source)
    return success();

  OwningOpRef<ModuleOp> clone(cast<ModuleOp>(unit->clone()));
  if (failed(lowerClone(*clone)))
    return emit() << "N0-C1 transactional clone lowering failed verification";

  unit->setAttrs((*clone)->getAttrs());
  unit.getBodyRegion().takeBody((*clone).getBodyRegion());
  return success();
}

} // namespace acir::compiler
