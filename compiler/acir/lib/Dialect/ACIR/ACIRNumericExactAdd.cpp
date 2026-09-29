#include "ACIRNumericExactAdd.h"

#include "ACIRSourceContracts.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "llvm/ADT/APInt.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallString.h"

#include <algorithm>

using namespace mlir;

namespace acir::ac {
namespace {

InFlightDiagnostic error(Operation *operation) {
  return operation->emitOpError();
}

APSInt parseMathValue(MathIntAttr value) {
  StringRef spelling = value.getCanonicalValue();
  const bool negative = spelling.consume_front("-");
  const unsigned width = static_cast<unsigned>(spelling.size() * 4 + 1);
  APInt bits(width, spelling, /*radix=*/10);
  if (negative)
    bits = -bits;
  return APSInt(std::move(bits), /*isUnsigned=*/!negative);
}

struct IntegerDomainValue {
  DictionaryAttr domain;
  APSInt mathematicalValue;
  Type storage;
  bool isUnsigned = false;
};

FailureOr<IntegerDomainValue> singletonDomain(DictionaryAttr domain,
                                              Operation *owner) {
  auto emit = [&] { return owner->emitOpError(); };
  if (failed(detail::verifyLogicalTypeStructure(domain, emit)))
    return failure();
  auto kind = domain.getAs<StringAttr>("kind");
  auto storage = domain.getAs<TypeAttr>("storage");
  auto lower = domain.getAs<MathIntAttr>("lower");
  auto upper = domain.getAs<MathIntAttr>("upper");
  auto interpretation = domain.getAs<StringAttr>("interpretation");
  auto integer =
      storage ? dyn_cast<IntegerType>(storage.getValue()) : IntegerType();
  if (!kind || kind.getValue() != "integer" || domain.size() != 5 || !storage ||
      !integer || !integer.isSignless() || !lower || !upper ||
      !interpretation ||
      (interpretation.getValue() != "signed" &&
       interpretation.getValue() != "unsigned"))
    return emit() << "constant-add proof requires closed integer domains";
  APSInt value = parseMathValue(lower);
  APSInt upperValue = parseMathValue(upper);
  APSInt expectedUpper = value.extend(value.getBitWidth() + 1);
  ++expectedUpper;
  if (APSInt::compareValues(upperValue, expectedUpper) != 0)
    return emit() << "constant-add domains must be singleton [c,c+1)";
  const bool isUnsigned = interpretation.getValue() == "unsigned";
  if ((isUnsigned && value.isNegative()) ||
      (!isUnsigned && !value.isNegative()))
    return emit()
           << "singleton value sign must match its integer interpretation";
  unsigned minimumWidth = std::max(1u, isUnsigned ? value.getActiveBits()
                                                  : value.getSignificantBits());
  if (integer.getWidth() != minimumWidth || integer.getWidth() > 64)
    return emit() << "constant-add domains require minimum signless i1..i64 "
                     "storage";
  return IntegerDomainValue{domain, value, storage.getValue(), isUnsigned};
}

FailureOr<APSInt> actualConstantValue(Value value,
                                      const IntegerDomainValue &domain,
                                      Operation *owner) {
  auto emit = [&] { return owner->emitOpError(); };
  auto constant = value.getDefiningOp<arith::ConstantOp>();
  auto integer =
      constant ? dyn_cast<IntegerAttr>(constant.getValue()) : IntegerAttr();
  auto storage = dyn_cast<IntegerType>(value.getType());
  if (!constant || !integer || !storage || !storage.isSignless() ||
      value.getType() != domain.storage)
    return emit() << "constant-add input binding must name an exact arith "
                     "integer constant";
  APSInt result(integer.getValue(), domain.isUnsigned);
  if (APSInt::compareValues(result, domain.mathematicalValue) != 0)
    return emit() << "constant-add bit pattern disagrees with mathematical "
                     "domain value";
  return result;
}

ValueBindingOp findBinding(RuleOp rule, DictionaryAttr id, Operation *owner) {
  ValueBindingOp result;
  for (ValueBindingOp binding : rule.getBody().front().getOps<ValueBindingOp>())
    if (binding.getIdAttr() == id) {
      if (result) {
        owner->emitOpError() << "constant-add ValueID has duplicate bindings";
        return {};
      }
      result = binding;
    }
  if (!result)
    owner->emitOpError() << "constant-add ValueID has no ValueBinding";
  return result;
}

bool isNoneTarget(DictionaryAttr target, MLIRContext *context) {
  return target && target.size() == 1 &&
         target.getAs<StringAttr>("kind") == StringAttr::get(context, "none");
}

struct ExtendedOperandMatch {
  Operation *extension = nullptr;
};

std::optional<ExtendedOperandMatch>
matchExtendedOperand(Value actual, Value constantValue, unsigned resultWidth,
                     const APSInt &mathematicalValue) {
  auto sourceType = dyn_cast<IntegerType>(constantValue.getType());
  auto actualType = dyn_cast<IntegerType>(actual.getType());
  if (!sourceType || !actualType || actualType.getWidth() != resultWidth ||
      sourceType.getWidth() > resultWidth)
    return std::nullopt;
  if (sourceType.getWidth() == resultWidth)
    return actual == constantValue
               ? std::optional<ExtendedOperandMatch>(ExtendedOperandMatch{})
               : std::nullopt;
  if (!mathematicalValue.isUnsigned()) {
    auto extension = actual.getDefiningOp<arith::ExtSIOp>();
    if (!extension || extension.getIn() != constantValue)
      return std::nullopt;
    return ExtendedOperandMatch{extension.getOperation()};
  }
  auto extension = actual.getDefiningOp<arith::ExtUIOp>();
  if (!extension || extension.getIn() != constantValue)
    return std::nullopt;
  return ExtendedOperandMatch{extension.getOperation()};
}

bool isTrueI1(Value value) {
  auto constant = value.getDefiningOp<arith::ConstantOp>();
  auto integer =
      constant ? dyn_cast<IntegerAttr>(constant.getValue()) : IntegerAttr();
  return integer && integer.getType().isInteger(1) &&
         integer.getValue().isOne();
}

} // namespace

LogicalResult verifyExactConstantAddWitness(RuleOp rule,
                                            ValueBindingOp resultBinding,
                                            NumericProofOp proof) {
  auto emit = [&] { return proof.emitOpError(); };
  auto obligations = proof.getObligationsAttr();
  if (!obligations || obligations.size() != 3)
    return emit() << "B1a requires exactly constant A, constant B, add nodes";
  auto nodeA = dyn_cast<DictionaryAttr>(obligations[0]);
  auto nodeB = dyn_cast<DictionaryAttr>(obligations[1]);
  auto nodeSum = dyn_cast<DictionaryAttr>(obligations[2]);
  auto idA = nodeA ? nodeA.getAs<DictionaryAttr>("id") : DictionaryAttr();
  auto idB = nodeB ? nodeB.getAs<DictionaryAttr>("id") : DictionaryAttr();
  auto idSum = nodeSum ? nodeSum.getAs<DictionaryAttr>("id") : DictionaryAttr();
  auto opA = nodeA ? nodeA.getAs<StringAttr>("operator") : StringAttr();
  auto opB = nodeB ? nodeB.getAs<StringAttr>("operator") : StringAttr();
  auto opSum = nodeSum ? nodeSum.getAs<StringAttr>("operator") : StringAttr();
  auto refsA = nodeA ? nodeA.getAs<ArrayAttr>("operands") : ArrayAttr();
  auto refsB = nodeB ? nodeB.getAs<ArrayAttr>("operands") : ArrayAttr();
  auto refsSum = nodeSum ? nodeSum.getAs<ArrayAttr>("operands") : ArrayAttr();
  auto targetA =
      nodeA ? nodeA.getAs<DictionaryAttr>("target") : DictionaryAttr();
  auto targetB =
      nodeB ? nodeB.getAs<DictionaryAttr>("target") : DictionaryAttr();
  auto targetSum =
      nodeSum ? nodeSum.getAs<DictionaryAttr>("target") : DictionaryAttr();
  if (!nodeA || !nodeB || !nodeSum || nodeA.size() != 4 || nodeB.size() != 4 ||
      nodeSum.size() != 4 || !idA || !idB || !idSum || idA == idB ||
      idA == idSum || idB == idSum || !opA || !opB || !opSum ||
      opA.getValue() != "constant" || opB.getValue() != "constant" ||
      opSum.getValue() != "add" || !refsA || refsA.size() != 1 || !refsB ||
      refsB.size() != 1 || !refsSum || refsSum.size() != 2 ||
      !isNoneTarget(targetA, proof.getContext()) ||
      !isNoneTarget(targetB, proof.getContext()) ||
      !isNoneTarget(targetSum, proof.getContext()) ||
      proof.getResultIdAttr() != idSum || resultBinding.getIdAttr() != idSum)
    return emit() << "B1a recipe must be distinct constant A/B followed by "
                     "add(node0,node1)";

  auto refA = dyn_cast<DictionaryAttr>(refsA[0]);
  auto refB = dyn_cast<DictionaryAttr>(refsB[0]);
  auto refSumA = dyn_cast<DictionaryAttr>(refsSum[0]);
  auto refSumB = dyn_cast<DictionaryAttr>(refsSum[1]);
  if (!refA || refA.size() != 2 ||
      refA.getAs<StringAttr>("kind") !=
          StringAttr::get(proof.getContext(), "constant") ||
      !refA.getAs<MathIntAttr>("value") || !refB || refB.size() != 2 ||
      refB.getAs<StringAttr>("kind") !=
          StringAttr::get(proof.getContext(), "constant") ||
      !refB.getAs<MathIntAttr>("value") || !refSumA || refSumA.size() != 2 ||
      refSumA.getAs<StringAttr>("kind") !=
          StringAttr::get(proof.getContext(), "node") ||
      !refSumA.getAs<IntegerAttr>("index") || !refSumB || refSumB.size() != 2 ||
      refSumB.getAs<StringAttr>("kind") !=
          StringAttr::get(proof.getContext(), "node") ||
      !refSumB.getAs<IntegerAttr>("index"))
    return emit() << "B1a constant/add NumericRefs do not match node order";
  auto refIndexA = detail::decodeU32(refSumA.getAs<IntegerAttr>("index"),
                                     "B1a lhs NumericRef index", emit);
  auto refIndexB = detail::decodeU32(refSumB.getAs<IntegerAttr>("index"),
                                     "B1a rhs NumericRef index", emit);
  if (failed(refIndexA) || failed(refIndexB) || *refIndexA != 0 ||
      *refIndexB != 1)
    return emit() << "B1a add refs must be node indices 0 and 1";

  auto bindingA = findBinding(rule, idA, proof);
  auto bindingB = findBinding(rule, idB, proof);
  if (!bindingA || !bindingB)
    return failure();
  auto domainA =
      singletonDomain(bindingA.getDomainAttr(), proof.getOperation());
  auto domainB =
      singletonDomain(bindingB.getDomainAttr(), proof.getOperation());
  auto domainSum =
      singletonDomain(resultBinding.getDomainAttr(), proof.getOperation());
  if (failed(domainA) || failed(domainB) || failed(domainSum))
    return failure();
  APSInt valueA = parseMathValue(refA.getAs<MathIntAttr>("value"));
  APSInt valueB = parseMathValue(refB.getAs<MathIntAttr>("value"));
  if (APSInt::compareValues(valueA, domainA->mathematicalValue) != 0 ||
      APSInt::compareValues(valueB, domainB->mathematicalValue) != 0)
    return emit()
           << "B1a constant recipe values disagree with singleton domains";
  const unsigned sumWidth =
      std::max(valueA.getBitWidth(), valueB.getBitWidth()) + 1;
  APInt lhsBits = valueA.extend(sumWidth);
  APInt rhsBits = valueB.extend(sumWidth);
  APInt expectedBits = lhsBits + rhsBits;
  APSInt expectedSum(expectedBits, /*isUnsigned=*/!expectedBits.isNegative());
  if (APSInt::compareValues(expectedSum, domainSum->mathematicalValue) != 0)
    return emit()
           << "B1a sum domain does not equal exact mathematical addition";

  unsigned widthA = cast<IntegerType>(domainA->storage).getWidth();
  unsigned widthB = cast<IntegerType>(domainB->storage).getWidth();
  unsigned resultWidth = cast<IntegerType>(domainSum->storage).getWidth();
  if (resultWidth < widthA || resultWidth < widthB)
    return emit() << "B1a rejects result narrowing";
  auto constantA = bindingA.getValue().getDefiningOp<arith::ConstantOp>();
  auto constantB = bindingB.getValue().getDefiningOp<arith::ConstantOp>();
  auto sum = resultBinding.getValue().getDefiningOp<arith::AddIOp>();
  if (!constantA || !constantB || !sum ||
      sum.getOverflowFlags() != arith::IntegerOverflowFlags::none ||
      sum.getResult().getType() != domainSum->storage)
    return emit()
           << "B1a actual SSA must be constants and one flag-free arith.addi";
  auto actualA =
      actualConstantValue(bindingA.getValue(), *domainA, proof.getOperation());
  auto actualB =
      actualConstantValue(bindingB.getValue(), *domainB, proof.getOperation());
  if (failed(actualA) || failed(actualB))
    return failure();

  llvm::SmallPtrSet<Operation *, 8> expectedOps;
  expectedOps.insert(constantA.getOperation());
  expectedOps.insert(constantB.getOperation());
  expectedOps.insert(sum.getOperation());
  auto matchA = matchExtendedOperand(sum.getLhs(), bindingA.getValue(),
                                     resultWidth, *actualA);
  auto matchB = matchExtendedOperand(sum.getRhs(), bindingB.getValue(),
                                     resultWidth, *actualB);
  if (!matchA || !matchB) {
    matchA = matchExtendedOperand(sum.getRhs(), bindingA.getValue(),
                                  resultWidth, *actualA);
    matchB = matchExtendedOperand(sum.getLhs(), bindingB.getValue(),
                                  resultWidth, *actualB);
  }
  if (!matchA || !matchB)
    return emit() << "B1a addi operands must be the approved input extensions";
  if (matchA->extension)
    expectedOps.insert(matchA->extension);
  if (matchB->extension)
    expectedOps.insert(matchB->extension);

  if (proof->getOperand(1) != sum.getResult() ||
      resultBinding.getValue() != sum.getResult() ||
      proof.getResultDomainAttr() != resultBinding.getDomainAttr() ||
      proof->getOperand(0) != resultBinding.getPath() ||
      proof->getOperand(2) != resultBinding.getValid())
    return emit()
           << "B1a proof result/domain/controls must equal final binding";
  for (ValueBindingOp binding : {bindingA, bindingB, resultBinding}) {
    auto pathConstant = binding.getPath().getDefiningOp<arith::ConstantOp>();
    auto validConstant = binding.getValid().getDefiningOp<arith::ConstantOp>();
    auto dataProducer = binding.getValue().getDefiningOp();
    if (!isTrueI1(binding.getPath()) || !isTrueI1(binding.getValid()) ||
        !pathConstant || !validConstant || !dataProducer ||
        pathConstant->getBlock() != proof->getBlock() ||
        validConstant->getBlock() != proof->getBlock() ||
        dataProducer->getBlock() != proof->getBlock() ||
        !pathConstant->isBeforeInBlock(binding) ||
        !validConstant->isBeforeInBlock(binding) ||
        !dataProducer->isBeforeInBlock(binding) ||
        !binding->isBeforeInBlock(proof))
      return emit() << "B1a bindings require ordered literal-true controls";
    expectedOps.insert(pathConstant.getOperation());
    expectedOps.insert(validConstant.getOperation());
  }

  for (ValueBindingOp binding : rule.getBody().front().getOps<ValueBindingOp>())
    if (binding.getIdAttr() != idA && binding.getIdAttr() != idB &&
        binding.getIdAttr() != idSum)
      return emit() << "B1a rule contains an extra ValueBinding";
  for (Operation &operation : proof->getBlock()->getOperations()) {
    if (isa<arith::ConstantOp, arith::ExtUIOp, arith::ExtSIOp, arith::AddIOp,
            ValueBindingOp, NumericProofOp, YieldOp>(operation)) {
      if (isa<arith::ConstantOp>(operation) &&
          !expectedOps.contains(&operation))
        return emit() << "B1a rule contains an unowned arithmetic constant";
      if ((isa<arith::ExtUIOp, arith::ExtSIOp>(operation) ||
           isa<arith::AddIOp>(operation)) &&
          !expectedOps.contains(&operation))
        return emit() << "B1a rule contains an extra arithmetic operation";
      continue;
    }
    return emit() << "B1a rule contains an unsupported operation";
  }
  return success();
}

} // namespace acir::ac
