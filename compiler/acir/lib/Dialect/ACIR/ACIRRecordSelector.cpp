#include "ACIRRecordSelector.h"
#include "ACIRSourceContracts.h"
#include "llvm/ADT/STLExtras.h"

#include <iterator>

using namespace mlir;

namespace acir::ac::record_detail {
namespace {

bool noAttrs(Operation *operation) {
  return operation->getDiscardableAttrDictionary().empty();
}

bool oneUse(Value value) {
  return std::distance(value.use_begin(), value.use_end()) == 1;
}

} // namespace

bool isTrueI1(Value value) {
  auto constant = value.getDefiningOp<arith::ConstantOp>();
  auto integer =
      constant ? dyn_cast<IntegerAttr>(constant.getValue()) : IntegerAttr();
  return integer && integer.getType().isInteger(1) &&
         integer.getValue().isOne();
}

bool isFalseI1(Value value) {
  auto constant = value.getDefiningOp<arith::ConstantOp>();
  auto integer =
      constant ? dyn_cast<IntegerAttr>(constant.getValue()) : IntegerAttr();
  return integer && integer.getType().isInteger(1) &&
         integer.getValue().isZero();
}

LogicalResult verifyRecordLeafType(DictionaryAttr logical, Type type,
                                   Operation *owner) {
  auto emit = [&] { return owner->emitOpError(); };
  if (!logical || failed(detail::verifyLogicalTypeStructure(logical, emit)))
    return failure();
  auto kind = logical.getAs<StringAttr>("kind");
  if (kind.getValue() == "bool") {
    auto boolean = dyn_cast<IntegerType>(type);
    return boolean && boolean.isSignless() && boolean.getWidth() == 1
               ? success()
               : emit() << "record bool leaf must use signless i1";
  }
  auto storage = logical.getAs<TypeAttr>("storage");
  auto integer =
      storage ? dyn_cast<IntegerType>(storage.getValue()) : IntegerType();
  if (kind.getValue() != "integer" || !integer || !integer.isSignless() ||
      integer.getWidth() < 1 || integer.getWidth() > 64 ||
      type != storage.getValue())
    return emit() << "S2A record integer leaf must be signless i1 through i64";
  return success();
}

bool isSafeUsePath(Value value) {
  return value.getType().isInteger(1) && (isTrueI1(value) || isFalseI1(value));
}

FailureOr<FinalRecordSelector> verifyFinalRecordSelector(
    RuleOp rule, Value sourceData, Value usePath, Value useValid,
    Type recordType, ArrayRef<SelectorField> fields,
    ArrayRef<StructGetOp> sourceGets, StructCreateOp sourceCreate,
    arith::AndIOp fieldValidAnd) {
  if (!rule)
    return failure();
  auto emit = [&] { return rule.emitOpError(); };
  auto yield =
      rule && rule.getBody().getBlocks().size() == 1
          ? dyn_cast_or_null<YieldOp>(rule.getBody().front().getTerminator())
          : YieldOp();
  if (fields.size() != 2 || sourceGets.size() != 2 || !sourceData || !usePath ||
      !useValid || sourceData.getType() != recordType || !sourceCreate ||
      !yield || yield.getValues().size() != 2)
    return emit() << "S2A selector inputs are incomplete";

  auto enable = yield.getValues()[1].getDefiningOp<arith::AndIOp>();
  if (!enable || !noAttrs(enable) ||
      !((enable.getLhs() == usePath && enable.getRhs() == useValid) ||
        (enable.getLhs() == useValid && enable.getRhs() == usePath)))
    return emit() << "S2A selector enable must be use.path AND use.valid";

  StructGetOp leaves[2];
  for (StructGetOp op : rule.getBody().front().getOps<StructGetOp>()) {
    if (llvm::is_contained(sourceGets, op))
      continue;
    auto field = op.getFieldAttr();
    unsigned ordinal =
        field && field.getValue() == fields[0].name.getValue()   ? 0
        : field && field.getValue() == fields[1].name.getValue() ? 1
                                                                 : 2;
    if (ordinal > 1 || leaves[ordinal] || !noAttrs(op) ||
        op->getNumRegions() != 0 || op.getValue() != sourceData ||
        op.getResult().getType() != fields[ordinal].physical)
      return op.emitOpError() << "synthetic selector get is malformed";
    leaves[ordinal] = op;
  }
  if (!leaves[0] || !leaves[1])
    return emit() << "S2A selector needs one get per record field";

  arith::SelectOp selects[2];
  arith::ConstantOp zeros[2];
  for (arith::SelectOp select :
       rule.getBody().front().getOps<arith::SelectOp>()) {
    unsigned ordinal = select.getTrueValue() == leaves[0].getResult()   ? 0
                       : select.getTrueValue() == leaves[1].getResult() ? 1
                                                                        : 2;
    if (ordinal > 1 || selects[ordinal] || !noAttrs(select) ||
        select.getCondition() != enable.getResult() ||
        select.getType() != fields[ordinal].physical)
      return select.emitOpError() << "S2A scalar selector is malformed";
    auto zero = select.getFalseValue().getDefiningOp<arith::ConstantOp>();
    auto integer =
        zero ? dyn_cast<IntegerAttr>(zero.getValue()) : IntegerAttr();
    if (!zero || !integer ||
        zero.getResult().getType() != fields[ordinal].physical ||
        !integer.getValue().isZero() || zero->getAttrs().size() != 1 ||
        !zero->hasAttr("value"))
      return select.emitOpError()
             << "S2A selector false value must be its typed zero leaf";
    selects[ordinal] = select;
    zeros[ordinal] = zero;
  }
  if (!selects[0] || !selects[1] || zeros[0] == zeros[1])
    return emit() << "S2A selector requires two distinct zero leaves";

  auto selected = yield.getValues()[0].getDefiningOp<StructCreateOp>();
  if (!yield || !selected || !noAttrs(selected) ||
      selected->getNumRegions() != 0 ||
      selected == sourceData.getDefiningOp() ||
      selected.getValues().size() != 2 ||
      selected.getValues()[0] != selects[0].getResult() ||
      selected.getValues()[1] != selects[1].getResult() ||
      selected.getResult().getType() != recordType)
    return emit() << "S2A aggregate selector create is malformed";

  Block &body = rule.getBody().front();
  if (llvm::any_of(body,
                   [](Operation &operation) {
                     return !isa<arith::ConstantOp, arith::AndIOp,
                                 arith::SelectOp, StructGetOp, StructCreateOp,
                                 ValueBindingOp, ValueUseOp, YieldOp>(
                         operation);
                   }) ||
      llvm::any_of(body.getOps<arith::AndIOp>(),
                   [&](arith::AndIOp op) {
                     return op != enable && op != fieldValidAnd;
                   }) ||
      llvm::range_size(body.getOps<arith::AndIOp>()) !=
          (fieldValidAnd ? 2u : 1u) ||
      llvm::range_size(body.getOps<arith::SelectOp>()) != 2 ||
      llvm::range_size(body.getOps<StructGetOp>()) != 4 ||
      llvm::range_size(body.getOps<StructCreateOp>()) != 2 ||
      llvm::range_size(body.getOps<ValueUseOp>()) != 1 ||
      llvm::range_size(body.getOps<ValueBindingOp>()) != 4)
    return emit() << "S2A record body contains an extra or missing operation";

  for (arith::ConstantOp constant : body.getOps<arith::ConstantOp>()) {
    auto integer = dyn_cast<IntegerAttr>(constant.getValue());
    bool zeroLeaf = llvm::is_contained(zeros, constant);
    bool booleanControl = integer && integer.getType().isInteger(1);
    if (!integer || constant->getAttrs().size() != 1 ||
        !constant->hasAttr("value") || (!zeroLeaf && !booleanControl) ||
        (zeroLeaf && !oneUse(constant.getResult())) ||
        (!zeroLeaf && constant.use_empty()))
      return constant.emitOpError()
             << "S2A constant is outside the exact selector/control set";
    if (!zeroLeaf)
      for (OpOperand &operand : constant.getResult().getUses()) {
        Operation *user = operand.getOwner();
        bool control =
            (isa<ValueBindingOp>(user) && (operand.getOperandNumber() == 1 ||
                                           operand.getOperandNumber() == 2)) ||
            (isa<ValueUseOp>(user) && operand.getOperandNumber() == 2) ||
            isa<arith::AndIOp>(user);
        if (!control)
          return constant.emitOpError()
                 << "S2A boolean literal escapes control operands";
      }
  }

  auto firstGet = sourceGets[0];
  auto secondGet = sourceGets[1];
  if (!oneUse(leaves[0].getResult()) || !oneUse(leaves[1].getResult()) ||
      !oneUse(zeros[0].getResult()) || !oneUse(zeros[1].getResult()) ||
      !oneUse(selects[0].getResult()) || !oneUse(selects[1].getResult()) ||
      !oneUse(selected.getResult()) ||
      std::distance(enable.getResult().use_begin(),
                    enable.getResult().use_end()) != 3 ||
      std::distance(body.getArgument(0).use_begin(),
                    body.getArgument(0).use_end()) != 3 ||
      std::distance(firstGet.getResult().use_begin(),
                    firstGet.getResult().use_end()) != 2 ||
      std::distance(secondGet.getResult().use_begin(),
                    secondGet.getResult().use_end()) != 2 ||
      std::distance(sourceCreate.getResult().use_begin(),
                    sourceCreate.getResult().use_end()) != 4 ||
      (fieldValidAnd &&
       std::distance(fieldValidAnd.getResult().use_begin(),
                     fieldValidAnd.getResult().use_end()) != 3) ||
      yield.getValues()[0] != selected.getResult() ||
      yield.getValues()[1] != enable.getResult())
    return emit() << "S2A selector contains an orphan or unauthorized use";
  return FinalRecordSelector{enable, selected};
}

} // namespace acir::ac::record_detail
