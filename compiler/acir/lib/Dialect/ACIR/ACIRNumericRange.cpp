#include "ACIRNumericRange.h"

#include "ACIRSourceContracts.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/IR/Location.h"
#include "llvm/ADT/APInt.h"
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
  DictionaryAttr target;
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
    return emit() << "range proof requires its source-owned FileLineColLoc";
  if (!symbol || failed(detail::verifyOccurrence(origin, emit)) || !site ||
      !definition || definition.getValue() != symbol.getValue() || !expansion ||
      !expansion.empty())
    return emit() << "range proof origin must name the direct enclosing module";
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
    return emit() << "checked to_bits requires a closed integer domain";
  APSInt lower = mathValue(lowerAttr), upper = mathValue(upperAttr);
  bool isUnsigned = interpretation.getValue() == "unsigned";
  if (compare(lower, upper) >= 0 || (isUnsigned && lower.isNegative()) ||
      (!isUnsigned && !lower.isNegative()))
    return emit() << "checked to_bits domain interval/sign is invalid";
  APSInt maximum = upper;
  --maximum;
  unsigned minimum = 0;
  if (isUnsigned) {
    if (maximum.isNegative())
      return emit() << "unsigned checked domain crosses below zero";
    minimum = std::max(1u, maximum.getActiveBits());
  } else {
    for (unsigned width = 1; width <= 64; ++width)
      if (fitsSigned(lower, width) && fitsSigned(maximum, width)) {
        minimum = width;
        break;
      }
  }
  if (!minimum || minimum != storage.getWidth())
    return emit() << "checked to_bits domain storage is not canonical";
  return Domain{attr, lower, upper, storage, isUnsigned};
}

FailureOr<Node> node(Attribute raw, StringRef opcode, unsigned operands,
                     Operation *owner, bool boundary) {
  auto emit = [&] { return owner->emitOpError(); };
  auto dictionary = dyn_cast<DictionaryAttr>(raw);
  auto id =
      dictionary ? dictionary.getAs<DictionaryAttr>("id") : DictionaryAttr();
  auto operation =
      dictionary ? dictionary.getAs<StringAttr>("operator") : StringAttr();
  auto refs =
      dictionary ? dictionary.getAs<ArrayAttr>("operands") : ArrayAttr();
  auto target = dictionary ? dictionary.getAs<DictionaryAttr>("target")
                           : DictionaryAttr();
  auto targetKind = target ? target.getAs<StringAttr>("kind") : StringAttr();
  bool validTarget = boundary
                         ? target && target.size() == 2 && targetKind &&
                               targetKind.getValue() == "integer_boundary" &&
                               target.getAs<DictionaryAttr>("domain")
                         : target && target.size() == 1 && targetKind &&
                               targetKind.getValue() == "none";
  if (!dictionary || dictionary.size() != 4 || !id || !operation ||
      operation.getValue() != opcode || !refs || refs.size() != operands ||
      !validTarget || failed(detail::verifyValueID(id, emit)))
    return emit() << "checked to_bits NumericNode has invalid closed shape";
  return Node{id, refs, target};
}

bool reference(Attribute raw, StringRef kind, unsigned index,
               Operation *owner) {
  auto ref = dyn_cast<DictionaryAttr>(raw);
  auto actualKind = ref ? ref.getAs<StringAttr>("kind") : StringAttr();
  auto actualIndex = ref ? ref.getAs<IntegerAttr>("index") : IntegerAttr();
  if (!ref || ref.size() != 2 || !actualKind || actualKind.getValue() != kind ||
      !actualIndex)
    return false;
  auto decoded = detail::decodeU32(actualIndex, "to_bits NumericRef index",
                                   [&] { return owner->emitOpError(); });
  return succeeded(decoded) && *decoded == index;
}

ValueBindingOp binding(RuleOp rule, DictionaryAttr id, Operation *owner) {
  ValueBindingOp found;
  for (auto candidate : rule.getBody().front().getOps<ValueBindingOp>())
    if (candidate.getIdAttr() == id) {
      if (found) {
        owner->emitOpError() << "checked to_bits ValueID is bound twice";
        return {};
      }
      found = candidate;
    }
  if (!found)
    owner->emitOpError() << "checked to_bits ValueID has no binding";
  return found;
}

bool boolConstant(Value value, bool expected) {
  auto op = value.getDefiningOp<arith::ConstantOp>();
  auto attr = op ? dyn_cast<IntegerAttr>(op.getValue()) : IntegerAttr();
  return attr && attr.getType().isInteger(1) &&
         attr.getValue() == APInt(1, expected);
}

bool matchesAnd(Value value, Value lhs, Value rhs) {
  auto op = value.getDefiningOp<arith::AndIOp>();
  return op && op.getType().isInteger(1) && op.getLhs() == lhs &&
         op.getRhs() == rhs;
}

bool matchesSourceValue(Value actual, Value source, bool isUnsigned) {
  if (actual == source)
    return true;
  if (isUnsigned) {
    auto extension = actual.getDefiningOp<arith::ExtUIOp>();
    return extension && extension.getIn() == source;
  }
  auto extension = actual.getDefiningOp<arith::ExtSIOp>();
  return extension && extension.getIn() == source;
}

bool actualEndpoint(Value value, const APSInt &endpoint) {
  bool isUnsigned = !endpoint.isNegative();
  if (auto extension = value.getDefiningOp<arith::ExtUIOp>()) {
    if (!isUnsigned)
      return false;
    value = extension.getIn();
  } else if (auto extension = value.getDefiningOp<arith::ExtSIOp>()) {
    if (isUnsigned)
      return false;
    value = extension.getIn();
  }
  auto constant = value.getDefiningOp<arith::ConstantOp>();
  auto attr =
      constant ? dyn_cast<IntegerAttr>(constant.getValue()) : IntegerAttr();
  if (!attr)
    return false;
  APSInt decoded(attr.getValue(), isUnsigned);
  return compare(decoded, endpoint) == 0;
}

bool matchesBound(Value condition, Value value, const APSInt &endpoint,
                  const Domain &input, bool lower) {
  bool alwaysTrue = lower ? compare(input.lower, endpoint) >= 0
                          : compare(input.upper, endpoint) <= 0;
  bool alwaysFalse = lower ? compare(input.upper, endpoint) <= 0
                           : compare(input.lower, endpoint) >= 0;
  if (alwaysTrue || alwaysFalse)
    return boolConstant(condition, alwaysTrue);
  auto cmp = condition.getDefiningOp<arith::CmpIOp>();
  auto predicate = lower ? (input.isUnsigned ? arith::CmpIPredicate::ule
                                             : arith::CmpIPredicate::sle)
                         : (input.isUnsigned ? arith::CmpIPredicate::ult
                                             : arith::CmpIPredicate::slt);
  return cmp && cmp.getPredicate() == predicate &&
         matchesSourceValue(lower ? cmp.getRhs() : cmp.getLhs(), value,
                            input.isUnsigned) &&
         actualEndpoint(lower ? cmp.getLhs() : cmp.getRhs(), endpoint);
}

bool approvedPath(Value path, RuleOp rule) {
  if (boolConstant(path, true) || boolConstant(path, false))
    return true;
  auto read = path.getDefiningOp<SourceReadOp>();
  auto current =
      read ? dyn_cast<BlockArgument>(read.getCurrent()) : BlockArgument();
  auto inputTypes = rule->getAttrOfType<ArrayAttr>("ac.input_types");
  auto logical =
      current && inputTypes && current.getArgNumber() < inputTypes.size()
          ? dyn_cast<DictionaryAttr>(inputTypes[current.getArgNumber()])
          : DictionaryAttr();
  return read && read->getBlock() == &rule.getBody().front() && current &&
         current.getOwner() == &rule.getBody().front() &&
         read.getResult().getType().isInteger(1) && logical &&
         logical.getAs<StringAttr>("kind") ==
             StringAttr::get(rule.getContext(), "bool");
}

bool matchesConversion(Value converted, Value source, const Domain &input,
                       const Domain &result) {
  if (converted.getType() != result.storage)
    return false;
  unsigned from = input.storage.getWidth(), to = result.storage.getWidth();
  if (from == to)
    return converted == source;
  if (from < to) {
    if (input.isUnsigned) {
      auto ext = converted.getDefiningOp<arith::ExtUIOp>();
      return ext && ext.getIn() == source;
    }
    auto ext = converted.getDefiningOp<arith::ExtSIOp>();
    return ext && ext.getIn() == source;
  }
  auto trunc = converted.getDefiningOp<arith::TruncIOp>();
  return trunc && trunc.getIn() == source;
}

LogicalResult verifyGuardedConversion(Value value, Value source, Value guard,
                                      Value demand, Value safety,
                                      const Domain &input, const Domain &result,
                                      NumericProofOp proof,
                                      llvm::SmallPtrSetImpl<Operation *> &ops) {
  auto branch = value.getDefiningOp<scf::IfOp>();
  if (!branch || branch.getCondition() != guard ||
      !matchesAnd(guard, demand, safety) || branch.getNumResults() != 1 ||
      branch.getThenRegion().getBlocks().size() != 1 ||
      branch.getElseRegion().getBlocks().size() != 1)
    return proof.emitOpError()
           << "checked conversion requires one guarded scf.if";
  Block &thenBlock = branch.getThenRegion().front();
  Block &elseBlock = branch.getElseRegion().front();
  SmallVector<scf::YieldOp> thenYields(thenBlock.getOps<scf::YieldOp>());
  SmallVector<scf::YieldOp> elseYields(elseBlock.getOps<scf::YieldOp>());
  if (thenYields.size() != 1 || elseYields.size() != 1)
    return proof.emitOpError()
           << "checked conversion branches require exactly one yield";
  scf::YieldOp thenYield = thenYields.front();
  scf::YieldOp elseYield = elseYields.front();
  if (thenYield.getNumOperands() != 1 || elseYield.getNumOperands() != 1)
    return proof.emitOpError()
           << "checked conversion yields require exactly one value";
  if (!matchesConversion(thenYield.getOperand(0), source, input, result))
    return proof.emitOpError() << "checked conversion safe branch is invalid";
  Operation *conversion = thenYield.getOperand(0) == source
                              ? nullptr
                              : thenYield.getOperand(0).getDefiningOp();
  if (conversion && (conversion->getBlock() != &thenBlock ||
                     !conversion->isBeforeInBlock(thenYield)))
    return proof.emitOpError()
           << "checked conversion must be inside the safe branch";
  auto zero = elseYield.getOperand(0).getDefiningOp<arith::ConstantOp>();
  auto zeroAttr = zero ? dyn_cast<IntegerAttr>(zero.getValue()) : IntegerAttr();
  if (!zeroAttr || zeroAttr.getType() != result.storage ||
      !zeroAttr.getValue().isZero())
    return proof.emitOpError()
           << "checked conversion requires zero placeholder";
  if (zero->getBlock() != &elseBlock || !zero->isBeforeInBlock(elseYield))
    return proof.emitOpError()
           << "checked conversion zero must be inside the fallback branch";
  for (Operation &operation : thenBlock.getOperations())
    if (&operation != conversion && &operation != thenYield.getOperation())
      return proof.emitOpError()
             << "checked conversion safe branch has an extra operation";
  for (Operation &operation : elseBlock.getOperations())
    if (&operation != zero.getOperation() &&
        &operation != elseYield.getOperation())
      return proof.emitOpError()
             << "checked conversion fallback branch has an extra operation";
  ops.insert(branch);
  if (Operation *condition = branch.getCondition().getDefiningOp())
    ops.insert(condition);
  ops.insert(thenYield);
  ops.insert(elseYield);
  ops.insert(zero);
  if (conversion)
    ops.insert(conversion);
  return success();
}

} // namespace

LogicalResult verifyCheckedToBitsWitness(RuleOp rule,
                                         ValueBindingOp resultBinding,
                                         NumericProofOp proof) {
  auto emit = [&] { return proof.emitOpError(); };
  if (failed(verifyProofOrigin(proof)))
    return failure();
  auto obligations = proof.getObligationsAttr();
  auto checks = proof.getChecksAttr();
  auto inputIDs = proof.getInputIdsAttr();
  auto inputDomains = proof.getInputDomainsAttr();
  if (!obligations || obligations.size() != 2 || !checks ||
      checks.size() != 1 || !inputIDs || inputIDs.size() != 1 ||
      !inputDomains || inputDomains.size() != 1 || !proof.getModeAttr() ||
      proof.getModeAttr().getValue() != "exact" || proof->hasAttr("width") ||
      proof->getNumOperands() != 7 ||
      proof.getResultIdAttr() != resultBinding.getIdAttr() ||
      proof.getResultIdAttr().getAs<DictionaryAttr>("origin") !=
          proof.getOriginAttr() ||
      proof.getResultDomainAttr() != resultBinding.getDomainAttr())
    return emit() << "checked to_bits proof envelope is not canonical";
  auto from = node(obligations[0], "from_bits", 1, proof, false);
  auto convert = node(obligations[1], "to_bits", 1, proof, true);
  if (failed(from) || failed(convert) ||
      !reference(from->operands[0], "input", 0, proof) ||
      !reference(convert->operands[0], "node", 0, proof) ||
      convert->target.getAs<DictionaryAttr>("domain") !=
          resultBinding.getDomainAttr())
    return failure();
  auto inputID = dyn_cast<DictionaryAttr>(inputIDs[0]);
  auto inputBinding = binding(rule, inputID, proof);
  auto fromBinding = binding(rule, from->id, proof);
  if (!inputID || !inputBinding || !fromBinding ||
      resultBinding.getIdAttr() != convert->id || inputID == from->id ||
      inputID == convert->id || from->id == convert->id)
    return emit() << "checked to_bits I/F/S identities are invalid";
  SmallVector<SourceReadOp> reads(
      rule.getBody().front().getOps<SourceReadOp>());
  SourceReadOp integerRead;
  for (auto read : reads)
    if (read->getAttrOfType<DictionaryAttr>("ac.origin") ==
        inputID.getAs<DictionaryAttr>("origin")) {
      if (integerRead)
        return emit() << "checked to_bits has multiple integer SourceReads";
      integerRead = read;
    }
  auto inputDomain = domain(dyn_cast<DictionaryAttr>(inputDomains[0]), proof);
  auto resultDomain = domain(resultBinding.getDomainAttr(), proof);
  auto slot = detail::decodeU32(inputID.getAs<IntegerAttr>("slot"),
                                "checked input slot", emit);
  if (!integerRead || failed(inputDomain) || failed(resultDomain) ||
      failed(slot) || *slot != 0 ||
      inputID.getAs<DictionaryAttr>("origin") !=
          integerRead->getAttrOfType<DictionaryAttr>("ac.origin") ||
      inputBinding.getValue() != integerRead.getResult() ||
      fromBinding.getValue() != integerRead.getResult() ||
      inputBinding.getValid() != proof->getOperand(2) ||
      fromBinding.getValid() != proof->getOperand(2) ||
      inputBinding.getPath() != fromBinding.getPath() ||
      inputBinding.getDomainAttr() != inputDomain->attr ||
      fromBinding.getDomainAttr() != inputDomain->attr)
    return emit() << "checked to_bits input authority is invalid";
  SmallVector<SourceExpectOp> expects(
      rule.getBody().front().getOps<SourceExpectOp>());
  if (expects.size() != 1 || expects[0].getKind() != "range")
    return emit() << "checked to_bits requires exactly one range expect";
  SourceExpectOp expect = expects[0];
  auto check = dyn_cast<DictionaryAttr>(checks[0]);
  auto checkID = check ? check.getAs<DictionaryAttr>("id") : DictionaryAttr();
  auto owner = check ? check.getAs<DictionaryAttr>("owner") : DictionaryAttr();
  auto kind = check ? check.getAs<StringAttr>("kind") : StringAttr();
  auto operand =
      check ? check.getAs<IntegerAttr>("operand_ordinal") : IntegerAttr();
  auto ordinal = detail::decodeU32(operand, "checked operand ordinal", emit);
  auto registration = checkID ? checkID.getAs<DictionaryAttr>("registration")
                              : DictionaryAttr();
  auto occurrence =
      checkID ? checkID.getAs<DictionaryAttr>("check") : DictionaryAttr();
  auto obligation =
      checkID ? checkID.getAs<IntegerAttr>("obligation") : IntegerAttr();
  if (!check || check.size() != 4 || !checkID ||
      failed(detail::verifyCheckID(checkID, emit)) || owner != convert->id ||
      !kind || kind.getValue() != "range" || failed(ordinal) || *ordinal != 0 ||
      registration != rule.getRegistrationAttr() ||
      expect->getAttrOfType<DictionaryAttr>("ac.check_id") != checkID ||
      !occurrence || !obligation)
    return emit() << "checked to_bits CheckBinding/template linkage is invalid";
  Builder builder(rule.getContext());
  auto required = builder.getDictionaryAttr({
      builder.getNamedAttr("id", checkID),
      builder.getNamedAttr("kind", kind),
      builder.getNamedAttr("location",
                           expect->getAttrOfType<DictionaryAttr>("location")),
  });
  auto requiredChecks = rule->getAttrOfType<ArrayAttr>("ac.required_checks");
  if (!requiredChecks || requiredChecks.size() != 1 ||
      requiredChecks[0] != required)
    return emit() << "checked to_bits required check is not one-to-one";
  Value proofPath = proof->getOperand(0), inputValid = proof->getOperand(2);
  Value inputPath = inputBinding.getPath();
  Value safety = proof->getOperand(5), demand = proof->getOperand(6);
  Value resultValid = proof->getOperand(4);
  if (!approvedPath(inputPath, rule) || proofPath != demand ||
      !matchesAnd(demand, inputPath, inputValid) ||
      !matchesAnd(resultValid, demand, safety) ||
      expect.getCondition() != safety || expect.getPath() != demand ||
      resultBinding.getPath() != demand ||
      resultBinding.getValid() != resultValid ||
      proof->getOperand(1) != inputBinding.getValue() ||
      proof->getOperand(3) != resultBinding.getValue())
    return emit() << "checked to_bits path/safety SSA is inconsistent";
  auto safetyAnd = safety.getDefiningOp<arith::AndIOp>();
  if (!safetyAnd ||
      !matchesBound(safetyAnd.getLhs(), integerRead.getResult(),
                    resultDomain->lower, *inputDomain, true) ||
      !matchesBound(safetyAnd.getRhs(), integerRead.getResult(),
                    resultDomain->upper, *inputDomain, false))
    return emit() << "checked to_bits range predicate is not A<=v&&v<B";
  llvm::SmallPtrSet<Operation *, 32> expected;
  expected.insert(integerRead);
  expected.insert(inputBinding);
  expected.insert(fromBinding);
  expected.insert(resultBinding);
  expected.insert(proof);
  expected.insert(expect);
  expected.insert(demand.getDefiningOp());
  expected.insert(safetyAnd);
  expected.insert(resultValid.getDefiningOp());
  expected.insert(rule.getBody().front().getTerminator());
  for (Value bound : {safetyAnd.getLhs(), safetyAnd.getRhs()}) {
    expected.insert(bound.getDefiningOp());
    if (auto cmp = bound.getDefiningOp<arith::CmpIOp>())
      for (Value operand : cmp.getOperands()) {
        Operation *producer = operand.getDefiningOp();
        if (producer)
          expected.insert(producer);
        if (producer && (producer->getName().getStringRef() == "arith.extui" ||
                         producer->getName().getStringRef() == "arith.extsi"))
          if (Operation *source = producer->getOperand(0).getDefiningOp())
            expected.insert(source);
      }
  }
  if (inputPath.getDefiningOp())
    expected.insert(inputPath.getDefiningOp());
  if (inputValid.getDefiningOp())
    expected.insert(inputValid.getDefiningOp());
  if (failed(verifyGuardedConversion(
          resultBinding.getValue(), integerRead.getResult(), resultValid,
          demand, safety, *inputDomain, *resultDomain, proof, expected)))
    return failure();
  if (std::distance(rule.getBody().front().getOps<ValueBindingOp>().begin(),
                    rule.getBody().front().getOps<ValueBindingOp>().end()) != 3)
    return emit() << "checked to_bits requires only I/F/S bindings";
  for (Operation &operation : rule.getBody().front().getOperations())
    if (!expected.contains(&operation))
      return emit() << "checked to_bits rule has an extra operation/check: "
                    << operation.getName();
  return success();
}

} // namespace acir::ac
