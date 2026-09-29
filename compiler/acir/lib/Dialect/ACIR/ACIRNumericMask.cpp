#include "ACIRNumericMask.h"

#include "ACIRSourceContracts.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/IR/Location.h"
#include "llvm/ADT/APInt.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/SmallPtrSet.h"

#include <algorithm>

using namespace mlir;

namespace acir::ac {
namespace {

struct Domain {
  DictionaryAttr attr;
  APSInt lower;
  APSInt upper;
  IntegerType storage;
  bool isUnsigned;
};

struct Node {
  DictionaryAttr id;
  ArrayAttr operands;
};

LogicalResult verifyProofOrigin(NumericProofOp proof) {
  auto emit = [&] { return proof.emitOpError(); };
  auto file = proof->getParentOfType<mlir::ModuleOp>();
  auto fileOwner = file ? file->getAttrOfType<DictionaryAttr>("ac.source_owner")
                        : DictionaryAttr();
  auto ownerPath =
      fileOwner ? fileOwner.getAs<StringAttr>("path") : StringAttr();
  auto hardware = proof->getParentOfType<ModuleOp>();
  auto moduleOwner =
      hardware ? hardware->getAttrOfType<DictionaryAttr>("ac.source_owner")
               : DictionaryAttr();
  auto symbol =
      hardware ? hardware->getAttrOfType<StringAttr>("sym_name") : StringAttr();
  auto location = dyn_cast<FileLineColLoc>(proof->getLoc());
  auto origin = proof.getOriginAttr();
  auto site = origin ? origin.getAs<DictionaryAttr>("site") : DictionaryAttr();
  auto definition =
      site ? site.getAs<FlatSymbolRefAttr>("definition") : FlatSymbolRefAttr();
  auto expansion = origin ? origin.getAs<ArrayAttr>("expansion") : ArrayAttr();
  if (!fileOwner || failed(detail::verifySourceOwner(fileOwner, emit)) ||
      moduleOwner != fileOwner || !ownerPath || !location ||
      location.getFilename() != ownerPath.getValue())
    return emit() << "mask proof requires its source-owned FileLineColLoc";
  if (!symbol || failed(detail::verifyOccurrence(origin, emit)) || !site ||
      !definition || definition.getValue() != symbol.getValue() || !expansion ||
      !expansion.empty())
    return emit() << "mask proof origin must be a direct enclosing module "
                     "Site with empty expansion";
  return success();
}

APSInt mathValue(MathIntAttr attr) {
  StringRef text = attr.getCanonicalValue();
  bool negative = text.consume_front("-");
  unsigned width = std::max(2u, unsigned(text.size() * 4 + 2));
  APInt bits(width, text, 10);
  if (negative)
    bits = -bits;
  return APSInt(std::move(bits), !negative);
}

APInt extend(const APSInt &value, unsigned width) {
  return value.isUnsigned() ? value.zextOrTrunc(width)
                            : value.sextOrTrunc(width);
}

int compare(const APSInt &lhs, const APSInt &rhs) {
  unsigned width = std::max(lhs.getBitWidth(), rhs.getBitWidth()) + 1;
  APInt left = extend(lhs, width), right = extend(rhs, width);
  if (left == right)
    return 0;
  return left.slt(right) ? -1 : 1;
}

bool fitsSigned(const APInt &value, unsigned width) {
  return value.sextOrTrunc(width).sextOrTrunc(value.getBitWidth()) == value;
}

FailureOr<Domain> domain(DictionaryAttr attr, Operation *owner) {
  auto emit = [&] { return owner->emitOpError(); };
  if (!attr || failed(detail::verifyLogicalTypeStructure(attr, emit)))
    return failure();
  auto kind = attr.getAs<StringAttr>("kind");
  auto storageAttr = attr.getAs<TypeAttr>("storage");
  auto lowerAttr = attr.getAs<MathIntAttr>("lower");
  auto upperAttr = attr.getAs<MathIntAttr>("upper");
  auto interpretation = attr.getAs<StringAttr>("interpretation");
  auto storage = storageAttr ? dyn_cast<IntegerType>(storageAttr.getValue())
                             : IntegerType();
  if (attr.size() != 5 || !kind || kind.getValue() != "integer" || !storage ||
      !storage.isSignless() || storage.getWidth() == 0 ||
      storage.getWidth() > 64 || !lowerAttr || !upperAttr || !interpretation ||
      (interpretation.getValue() != "signed" &&
       interpretation.getValue() != "unsigned"))
    return emit() << "mask witness requires a closed integer domain";
  APSInt lower = mathValue(lowerAttr), upper = mathValue(upperAttr);
  bool isUnsigned = interpretation.getValue() == "unsigned";
  if (compare(lower, upper) >= 0 || (isUnsigned && lower.isNegative()) ||
      (!isUnsigned && !lower.isNegative()))
    return emit() << "mask witness domain has invalid interval/sign";
  APSInt maximum = upper;
  --maximum;
  unsigned minimum = 0;
  if (isUnsigned) {
    if (maximum.isNegative())
      return emit() << "unsigned mask domain crosses below zero";
    minimum = std::max(1u, maximum.getActiveBits());
  } else {
    for (unsigned width = 1; width <= 64; ++width)
      if (fitsSigned(lower, width) && fitsSigned(maximum, width)) {
        minimum = width;
        break;
      }
  }
  if (!minimum || minimum != storage.getWidth())
    return emit() << "mask witness domain storage is not canonical";
  return Domain{attr, lower, upper, storage, isUnsigned};
}

FailureOr<Node> node(Attribute raw, StringRef opcode, unsigned operandCount,
                     Operation *owner) {
  auto emit = [&] { return owner->emitOpError(); };
  auto dictionary = dyn_cast<DictionaryAttr>(raw);
  auto id =
      dictionary ? dictionary.getAs<DictionaryAttr>("id") : DictionaryAttr();
  auto operation =
      dictionary ? dictionary.getAs<StringAttr>("operator") : StringAttr();
  auto operands =
      dictionary ? dictionary.getAs<ArrayAttr>("operands") : ArrayAttr();
  auto target = dictionary ? dictionary.getAs<DictionaryAttr>("target")
                           : DictionaryAttr();
  if (!dictionary || dictionary.size() != 4 || !id || !operation ||
      operation.getValue() != opcode || !operands ||
      operands.size() != operandCount || !target || target.size() != 1 ||
      target.getAs<StringAttr>("kind") !=
          StringAttr::get(owner->getContext(), "none") ||
      failed(detail::verifyValueID(id, emit)))
    return emit() << "mask NumericNode has unsupported closed shape";
  return Node{id, operands};
}

bool reference(Attribute raw, StringRef kind, unsigned index,
               Operation *owner) {
  auto ref = dyn_cast<DictionaryAttr>(raw);
  auto actualKind = ref ? ref.getAs<StringAttr>("kind") : StringAttr();
  auto actualIndex = ref ? ref.getAs<IntegerAttr>("index") : IntegerAttr();
  if (!ref || ref.size() != 2 || !actualKind || actualKind.getValue() != kind ||
      !actualIndex)
    return false;
  auto decoded = detail::decodeU32(actualIndex, "mask NumericRef index",
                                   [&] { return owner->emitOpError(); });
  return succeeded(decoded) && *decoded == index;
}

FailureOr<APSInt> constantReference(Node node, Operation *owner) {
  auto ref = dyn_cast<DictionaryAttr>(node.operands[0]);
  auto value = ref ? ref.getAs<MathIntAttr>("value") : MathIntAttr();
  if (!ref || ref.size() != 2 ||
      ref.getAs<StringAttr>("kind") !=
          StringAttr::get(owner->getContext(), "constant") ||
      !value)
    return owner->emitOpError() << "mask constant node has invalid reference";
  return mathValue(value);
}

ValueBindingOp binding(RuleOp rule, DictionaryAttr id, Operation *owner) {
  ValueBindingOp found;
  for (auto candidate : rule.getBody().front().getOps<ValueBindingOp>())
    if (candidate.getIdAttr() == id) {
      if (found) {
        owner->emitOpError() << "mask ValueID has duplicate bindings";
        return {};
      }
      found = candidate;
    }
  if (!found)
    owner->emitOpError() << "mask ValueID has no binding";
  return found;
}

bool trueI1(Value value) {
  auto op = value.getDefiningOp<arith::ConstantOp>();
  auto attr = op ? dyn_cast<IntegerAttr>(op.getValue()) : IntegerAttr();
  return attr && attr.getType().isInteger(1) && attr.getValue().isOne();
}

bool singleton(const Domain &actual, const APSInt &value) {
  APSInt upper = value.extend(value.getBitWidth() + 1);
  ++upper;
  return compare(actual.lower, value) == 0 && compare(actual.upper, upper) == 0;
}

bool actualConstant(Value value, const Domain &actual,
                    const APSInt &mathematical) {
  auto op = value.getDefiningOp<arith::ConstantOp>();
  auto attr = op ? dyn_cast<IntegerAttr>(op.getValue()) : IntegerAttr();
  if (!attr || value.getType() != actual.storage)
    return false;
  APSInt decoded(attr.getValue(), actual.isUnsigned);
  return compare(decoded, mathematical) == 0;
}

bool fullResultDomain(const Domain &actual, unsigned width) {
  APSInt zero(APInt(width + 2, 0), true);
  APInt bits(width + 2, 0);
  bits.setBit(width);
  APSInt upper(std::move(bits), true);
  return actual.isUnsigned && actual.storage.getWidth() == width &&
         compare(actual.lower, zero) == 0 && compare(actual.upper, upper) == 0;
}

bool matchesExtension(Value value, Value source, unsigned width,
                      bool isUnsigned, Operation *&extension) {
  auto sourceType = dyn_cast<IntegerType>(source.getType());
  auto type = dyn_cast<IntegerType>(value.getType());
  if (!sourceType || !type || type.getWidth() != width ||
      sourceType.getWidth() > width)
    return false;
  if (sourceType.getWidth() == width) {
    extension = nullptr;
    return value == source;
  }
  extension = value.getDefiningOp();
  return isUnsigned ? bool(dyn_cast_or_null<arith::ExtUIOp>(extension)) &&
                          cast<arith::ExtUIOp>(extension).getIn() == source
                    : bool(dyn_cast_or_null<arith::ExtSIOp>(extension)) &&
                          cast<arith::ExtSIOp>(extension).getIn() == source;
}

bool matchesModuloFit(Value value, Value source, unsigned width) {
  auto sourceType = dyn_cast<IntegerType>(source.getType());
  auto type = dyn_cast<IntegerType>(value.getType());
  if (!sourceType || !type || type.getWidth() != width)
    return false;
  if (sourceType.getWidth() == width)
    return value == source;
  if (sourceType.getWidth() < width) {
    auto ext = value.getDefiningOp<arith::ExtUIOp>();
    return ext && ext.getIn() == source;
  }
  auto trunc = value.getDefiningOp<arith::TruncIOp>();
  return trunc && trunc.getIn() == source;
}

LogicalResult commonEnvelope(RuleOp rule, ValueBindingOp result,
                             NumericProofOp proof, unsigned nodes,
                             StringRef mode) {
  auto emit = [&] { return proof.emitOpError(); };
  if (failed(verifyProofOrigin(proof)))
    return failure();
  auto obligations = proof.getObligationsAttr();
  auto inputIDs = proof.getInputIdsAttr();
  auto inputDomains = proof.getInputDomainsAttr();
  auto checks = proof.getChecksAttr();
  auto requiredChecks = rule->getAttrOfType<ArrayAttr>("ac.required_checks");
  auto observations =
      rule->getAttrOfType<ArrayAttr>("ac.required_observations");
  if (!obligations || obligations.size() != nodes || !inputIDs ||
      inputIDs.size() != 1 || !inputDomains || inputDomains.size() != 1 ||
      !checks || !checks.empty() || !proof.getModeAttr() ||
      proof.getModeAttr().getValue() != mode ||
      proof.getResultIdAttr() != result.getIdAttr() ||
      proof.getResultIdAttr().getAs<DictionaryAttr>("origin") !=
          proof.getOriginAttr() ||
      proof.getResultDomainAttr() != result.getDomainAttr() ||
      rule.getInputs().size() != 1 || !rule.getTargets().empty())
    return emit() << "mask proof envelope is not canonical";
  if ((requiredChecks && !requiredChecks.empty()) ||
      (observations && !observations.empty()))
    return emit() << "mask proof does not admit checks or observations";
  if (failed(detail::verifyOccurrence(proof.getOriginAttr(), emit)))
    return failure();
  return success();
}

LogicalResult verifyInput(RuleOp rule, NumericProofOp proof, Node from,
                          ValueBindingOp input, ValueBindingOp converted,
                          Domain &inputDomain, SourceReadOp &read) {
  auto emit = [&] { return proof.emitOpError(); };
  auto inputIDs = proof.getInputIdsAttr();
  auto inputID = dyn_cast<DictionaryAttr>(inputIDs[0]);
  auto ruleDomains = rule->getAttrOfType<ArrayAttr>("ac.input_types");
  auto ruleBindings = rule->getAttrOfType<ArrayAttr>("ac.input_bindings");
  if (!inputID || inputID == from.id ||
      !reference(from.operands[0], "input", 0, proof) || !ruleDomains ||
      ruleDomains.size() != 1 || !ruleBindings || ruleBindings.size() != 1 ||
      proof.getInputDomainsAttr()[0] != ruleDomains[0] ||
      failed(detail::verifyStateRef(dyn_cast<DictionaryAttr>(ruleBindings[0]),
                                    emit)))
    return emit() << "mask proof input/from_bits identities are invalid";
  auto parsed = domain(dyn_cast<DictionaryAttr>(ruleDomains[0]), proof);
  if (failed(parsed))
    return failure();
  inputDomain = *parsed;
  SmallVector<SourceReadOp> reads(
      rule.getBody().front().getOps<SourceReadOp>());
  if (reads.size() != 1)
    return emit() << "mask witness requires exactly one SourceRead";
  read = reads.front();
  auto current = dyn_cast<BlockArgument>(read.getCurrent());
  auto handle = dyn_cast<RegType>(rule.getInputs()[0].getType());
  auto slot = detail::decodeU32(inputID.getAs<IntegerAttr>("slot"),
                                "mask input slot", emit);
  if (failed(slot) || *slot != 0 || !current ||
      current.getOwner() != &rule.getBody().front() ||
      current.getArgNumber() != 0 || !handle ||
      handle.getElementType() != inputDomain.storage ||
      read.getResult().getType() != inputDomain.storage ||
      failed(detail::verifyOccurrence(
          read->getAttrOfType<DictionaryAttr>("ac.origin"), emit)) ||
      inputID.getAs<DictionaryAttr>("origin") !=
          read->getAttrOfType<DictionaryAttr>("ac.origin") ||
      input.getIdAttr() != inputID || converted.getIdAttr() != from.id ||
      input.getValue() != read.getResult() ||
      converted.getValue() != read.getResult() ||
      input.getDomainAttr() != inputDomain.attr ||
      converted.getDomainAttr() != inputDomain.attr)
    return emit() << "I/F bindings must name the authoritative SourceRead";
  return success();
}

LogicalResult verifyControlsAndProof(ValueBindingOp input,
                                     ValueBindingOp result,
                                     NumericProofOp proof) {
  if (!trueI1(input.getPath()) || !trueI1(input.getValid()) ||
      !trueI1(result.getPath()) || !trueI1(result.getValid()) ||
      proof->getOperand(0) != result.getPath() ||
      proof->getOperand(1) != input.getValue() ||
      proof->getOperand(2) != input.getValid() ||
      proof->getOperand(3) != result.getValue() ||
      proof->getOperand(4) != result.getValid())
    return proof.emitOpError() << "mask proof operands do not match I/S";
  return success();
}

LogicalResult closedRule(RuleOp rule, NumericProofOp proof,
                         ArrayRef<Operation *> arithmetic) {
  llvm::SmallPtrSet<Operation *, 32> expected;
  expected.insert(arithmetic.begin(), arithmetic.end());
  for (auto binding : rule.getBody().front().getOps<ValueBindingOp>()) {
    if (!trueI1(binding.getPath()) || !trueI1(binding.getValid()))
      return proof.emitOpError()
             << "mask bindings require literal-true path/valid";
    expected.insert(binding);
    expected.insert(binding.getPath().getDefiningOp());
    expected.insert(binding.getValid().getDefiningOp());
  }
  expected.insert(proof);
  expected.insert(rule.getBody().front().getTerminator());
  for (Operation &operation : rule.getBody().front().getOperations())
    if (!expected.contains(&operation))
      return proof.emitOpError() << "mask witness rule has an extra operation";
  return success();
}

} // namespace

LogicalResult verifyExactInputConstantMaskWitness(RuleOp rule,
                                                  ValueBindingOp result,
                                                  NumericProofOp proof) {
  if (failed(commonEnvelope(rule, result, proof, 3, "exact")) ||
      proof->hasAttr("width"))
    return failure();
  auto obligations = proof.getObligationsAttr();
  auto from = node(obligations[0], "from_bits", 1, proof);
  auto mask = node(obligations[1], "constant", 1, proof);
  auto bitAnd = node(obligations[2], "and_bits", 2, proof);
  if (failed(from) || failed(mask) || failed(bitAnd) ||
      !reference(bitAnd->operands[0], "node", 0, proof) ||
      !reference(bitAnd->operands[1], "node", 1, proof))
    return failure();
  auto maskValue = constantReference(*mask, proof);
  if (failed(maskValue) || maskValue->isNegative())
    return proof.emitOpError() << "exact mask must be nonnegative";
  auto input = binding(
      rule, dyn_cast<DictionaryAttr>(proof.getInputIdsAttr()[0]), proof);
  auto converted = binding(rule, from->id, proof);
  auto maskBinding = binding(rule, mask->id, proof);
  if (!input || !converted || !maskBinding || result.getIdAttr() != bitAnd->id)
    return failure();
  if (input.getIdAttr() == converted.getIdAttr() ||
      input.getIdAttr() == maskBinding.getIdAttr() ||
      input.getIdAttr() == result.getIdAttr() ||
      converted.getIdAttr() == maskBinding.getIdAttr() ||
      converted.getIdAttr() == result.getIdAttr() ||
      maskBinding.getIdAttr() == result.getIdAttr())
    return proof.emitOpError() << "exact mask ValueIDs must be distinct";
  Domain inputDomain, maskDomain, resultDomain;
  SourceReadOp read;
  auto parsedMask = domain(maskBinding.getDomainAttr(), proof);
  auto parsedResult = domain(result.getDomainAttr(), proof);
  if (failed(parsedMask) || failed(parsedResult) ||
      failed(
          verifyInput(rule, proof, *from, input, converted, inputDomain, read)))
    return failure();
  maskDomain = *parsedMask;
  resultDomain = *parsedResult;
  APSInt zero(APInt(maskValue->getBitWidth() + 1, 0), true);
  APSInt upper = *maskValue;
  ++upper;
  if (!maskDomain.isUnsigned || !singleton(maskDomain, *maskValue) ||
      !actualConstant(maskBinding.getValue(), maskDomain, *maskValue) ||
      !resultDomain.isUnsigned || compare(resultDomain.lower, zero) != 0 ||
      compare(resultDomain.upper, upper) != 0)
    return proof.emitOpError() << "exact mask domains must be C=[M,M+1), "
                                  "S=[0,M+1)";
  auto andi = result.getValue().getDefiningOp<arith::AndIOp>();
  auto resultType = dyn_cast<IntegerType>(result.getValue().getType());
  Operation *narrow = nullptr;
  if (!andi) {
    auto trunc = result.getValue().getDefiningOp<arith::TruncIOp>();
    if (!trunc)
      return proof.emitOpError() << "exact mask result must be andi/trunci";
    narrow = trunc;
    andi = trunc.getIn().getDefiningOp<arith::AndIOp>();
  }
  auto opType = andi ? dyn_cast<IntegerType>(andi.getType()) : IntegerType();
  Operation *inputExt = nullptr, *maskExt = nullptr;
  if (!andi || !resultType || resultType != resultDomain.storage || !opType ||
      opType.getWidth() < resultType.getWidth() ||
      !matchesExtension(andi.getLhs(), read.getResult(), opType.getWidth(),
                        inputDomain.isUnsigned, inputExt) ||
      !matchesExtension(andi.getRhs(), maskBinding.getValue(),
                        opType.getWidth(), true, maskExt) ||
      (narrow && cast<arith::TruncIOp>(narrow).getType() != resultType))
    return proof.emitOpError() << "exact mask SSA/extension chain is invalid";
  if (failed(verifyControlsAndProof(input, result, proof)))
    return failure();
  if (std::distance(rule.getBody().front().getOps<ValueBindingOp>().begin(),
                    rule.getBody().front().getOps<ValueBindingOp>().end()) != 4)
    return proof.emitOpError()
           << "exact mask requires only I/F/Cmask/S bindings";
  SmallVector<Operation *> expected{
      read, maskBinding.getValue().getDefiningOp(), andi};
  if (inputExt)
    expected.push_back(inputExt);
  if (maskExt)
    expected.push_back(maskExt);
  if (narrow)
    expected.push_back(narrow);
  return closedRule(rule, proof, expected);
}

LogicalResult verifyLowBitsInputMaskWitness(RuleOp rule, ValueBindingOp result,
                                            NumericProofOp proof) {
  if (failed(commonEnvelope(rule, result, proof, 5, "low_bits")))
    return failure();
  auto widthAttr = proof->getAttrOfType<IntegerAttr>("width");
  auto widthDecoded = detail::decodeU32(widthAttr, "low_bits width",
                                        [&] { return proof.emitOpError(); });
  if (failed(widthDecoded) || *widthDecoded == 0 || *widthDecoded > 64)
    return proof.emitOpError() << "low_bits width must be in 1..64";
  unsigned width = *widthDecoded;
  auto obligations = proof.getObligationsAttr();
  auto from = node(obligations[0], "from_bits", 1, proof);
  auto constant = node(obligations[1], "constant", 1, proof);
  auto operationAttr = dyn_cast<DictionaryAttr>(obligations[2]);
  auto opcode = operationAttr ? operationAttr.getAs<StringAttr>("operator")
                              : StringAttr();
  auto arithmetic =
      opcode && (opcode.getValue() == "add" || opcode.getValue() == "sub")
          ? node(obligations[2], opcode.getValue(), 2, proof)
          : FailureOr<Node>(failure());
  auto mask = node(obligations[3], "constant", 1, proof);
  auto bitAnd = node(obligations[4], "and_bits", 2, proof);
  if (failed(from) || failed(constant) || failed(arithmetic) || failed(mask) ||
      failed(bitAnd) || !reference(from->operands[0], "input", 0, proof) ||
      !reference(arithmetic->operands[0], "node", 0, proof) ||
      !reference(arithmetic->operands[1], "node", 1, proof) ||
      !reference(bitAnd->operands[0], "node", 2, proof) ||
      !reference(bitAnd->operands[1], "node", 3, proof))
    return proof.emitOpError() << "low_bits obligation graph is not canonical";
  auto k = constantReference(*constant, proof);
  auto maskValue = constantReference(*mask, proof);
  APInt full(width + 1, 0);
  full.setLowBits(width);
  APSInt expectedMask(std::move(full), true);
  if (failed(k) || failed(maskValue) || k->isNegative() ||
      compare(*maskValue, expectedMask) != 0)
    return proof.emitOpError() << "low_bits requires k>=0 and mask=2^w-1";
  auto input = binding(
      rule, dyn_cast<DictionaryAttr>(proof.getInputIdsAttr()[0]), proof);
  auto converted = binding(rule, from->id, proof);
  auto kBinding = binding(rule, constant->id, proof);
  auto maskBinding = binding(rule, mask->id, proof);
  if (!input || !converted || !kBinding || !maskBinding ||
      result.getIdAttr() != bitAnd->id)
    return failure();
  llvm::SmallDenseSet<Attribute, 8> ids;
  for (DictionaryAttr id :
       {input.getIdAttr(), converted.getIdAttr(), kBinding.getIdAttr(),
        arithmetic->id, maskBinding.getIdAttr(), result.getIdAttr()})
    if (!ids.insert(id).second)
      return proof.emitOpError() << "low_bits ValueIDs must be distinct";
  Domain inputDomain;
  SourceReadOp read;
  auto resultDomain = domain(result.getDomainAttr(), proof);
  auto kDomain = domain(kBinding.getDomainAttr(), proof);
  auto maskDomain = domain(maskBinding.getDomainAttr(), proof);
  if (failed(resultDomain) || failed(kDomain) || failed(maskDomain) ||
      failed(verifyInput(rule, proof, *from, input, converted, inputDomain,
                         read)) ||
      !inputDomain.isUnsigned || !fullResultDomain(*resultDomain, width))
    return proof.emitOpError() << "low_bits requires a nonnegative input and "
                                  "canonical [0,2^w) result";
  APInt reduced = extend(*k, width).trunc(width);
  APSInt reducedK(reduced, true);
  if (!singleton(*kDomain, reducedK) || !singleton(*maskDomain, expectedMask) ||
      !actualConstant(kBinding.getValue(), *kDomain, reducedK) ||
      !actualConstant(maskBinding.getValue(), *maskDomain, expectedMask))
    return proof.emitOpError()
           << "low_bits constants must be reduced/canonical";
  auto andi = result.getValue().getDefiningOp<arith::AndIOp>();
  auto arithmeticOp = andi ? andi.getLhs().getDefiningOp() : nullptr;
  bool rightMask = andi && andi.getRhs() == maskBinding.getValue();
  if (!rightMask && andi) {
    arithmeticOp = andi.getRhs().getDefiningOp();
    rightMask = andi.getLhs() == maskBinding.getValue();
  }
  bool correctArithmetic = opcode.getValue() == "add"
                               ? isa_and_nonnull<arith::AddIOp>(arithmeticOp)
                               : isa_and_nonnull<arith::SubIOp>(arithmeticOp);
  if (!andi || !rightMask || !correctArithmetic ||
      andi.getType() != resultDomain->storage ||
      arithmeticOp->getOperand(0).getType() != resultDomain->storage ||
      !matchesModuloFit(arithmeticOp->getOperand(0), read.getResult(), width) ||
      !matchesModuloFit(arithmeticOp->getOperand(1), kBinding.getValue(),
                        width))
    return proof.emitOpError()
           << "low_bits actual modulo arithmetic is invalid";
  if (failed(verifyControlsAndProof(input, result, proof)))
    return failure();
  SmallVector<ValueBindingOp> bindings(
      rule.getBody().front().getOps<ValueBindingOp>());
  if (bindings.size() != 5 ||
      llvm::any_of(bindings, [&](ValueBindingOp candidate) {
        return candidate.getIdAttr() == arithmetic->id;
      }))
    return proof.emitOpError()
           << "low_bits retains the wide obligation without "
              "an intermediate binding";
  SmallVector<Operation *> expected{read, kBinding.getValue().getDefiningOp(),
                                    maskBinding.getValue().getDefiningOp(),
                                    arithmeticOp, andi};
  if (arithmeticOp->getOperand(0) != read.getResult())
    expected.push_back(arithmeticOp->getOperand(0).getDefiningOp());
  if (arithmeticOp->getOperand(1) != kBinding.getValue())
    expected.push_back(arithmeticOp->getOperand(1).getDefiningOp());
  return closedRule(rule, proof, expected);
}

} // namespace acir::ac
