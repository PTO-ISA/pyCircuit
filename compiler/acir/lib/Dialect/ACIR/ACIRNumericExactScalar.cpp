#include "ACIRNumericExactScalar.h"

#include "ACIRSourceContracts.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "llvm/ADT/APSInt.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/StringSwitch.h"

#include <algorithm>

using namespace mlir;

namespace acir::ac {
namespace {

struct IntegerDomain {
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

struct ScalarWitness {
  SourceReadOp read;
  ValueBindingOp input;
  ValueBindingOp fromBits;
  ValueBindingOp constant;
  Node operation;
  IntegerDomain inputDomain;
  IntegerDomain constantDomain;
  APSInt constantValue;
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
    return emit()
           << "exact scalar proof requires its source-owned FileLineColLoc";
  if (!symbol || failed(detail::verifyOccurrence(origin, emit)) || !site ||
      !definition || definition.getValue() != symbol.getValue() || !expansion ||
      !expansion.empty())
    return emit() << "exact scalar proof origin must be a direct enclosing "
                     "module Site with empty expansion";
  return success();
}

APSInt parseMathInt(MathIntAttr value) {
  StringRef text = value.getCanonicalValue();
  bool negative = text.consume_front("-");
  unsigned width = std::max(2u, static_cast<unsigned>(text.size() * 4 + 2));
  APInt bits(width, text, 10);
  if (negative)
    bits = -bits;
  return APSInt(std::move(bits), /*isUnsigned=*/!negative);
}

APInt extend(const APSInt &value, unsigned width) {
  return value.isUnsigned() ? value.zextOrTrunc(width)
                            : value.sextOrTrunc(width);
}

int compare(const APSInt &lhs, const APSInt &rhs) {
  unsigned width = std::max(lhs.getBitWidth(), rhs.getBitWidth()) + 1;
  APInt left = extend(lhs, width);
  APInt right = extend(rhs, width);
  if (left == right)
    return 0;
  return left.slt(right) ? -1 : 1;
}

APSInt subtract(const APSInt &lhs, const APSInt &rhs) {
  unsigned width = std::max(lhs.getBitWidth(), rhs.getBitWidth()) + 1;
  APInt result = extend(lhs, width) - extend(rhs, width);
  bool negative = result.isNegative();
  return APSInt(std::move(result), /*isUnsigned=*/!negative);
}

bool fitsSigned(const APInt &value, unsigned width) {
  return value.sextOrTrunc(width).sextOrTrunc(value.getBitWidth()) == value;
}

FailureOr<IntegerDomain> integerDomain(DictionaryAttr domain,
                                       Operation *owner) {
  auto emit = [&] { return owner->emitOpError(); };
  if (failed(detail::verifyLogicalTypeStructure(domain, emit)))
    return failure();
  auto kind = domain.getAs<StringAttr>("kind");
  auto storageAttr = domain.getAs<TypeAttr>("storage");
  auto lowerAttr = domain.getAs<MathIntAttr>("lower");
  auto upperAttr = domain.getAs<MathIntAttr>("upper");
  auto interpretation = domain.getAs<StringAttr>("interpretation");
  auto storage = storageAttr ? dyn_cast<IntegerType>(storageAttr.getValue())
                             : IntegerType();
  if (!kind || kind.getValue() != "integer" || domain.size() != 5 || !storage ||
      !storage.isSignless() || storage.getWidth() == 0 ||
      storage.getWidth() > 64 || !lowerAttr || !upperAttr || !interpretation)
    return emit() << "exact scalar witness requires a closed i1..i64 domain";
  APSInt lower = parseMathInt(lowerAttr);
  APSInt upper = parseMathInt(upperAttr);
  bool isUnsigned = interpretation.getValue() == "unsigned";
  if ((!isUnsigned && interpretation.getValue() != "signed") ||
      compare(lower, upper) >= 0 || (isUnsigned && lower.isNegative()) ||
      (!isUnsigned && !lower.isNegative()))
    return emit() << "integer domain interpretation or interval is invalid";
  APSInt maximum = upper;
  --maximum;
  unsigned minimum = 0;
  if (isUnsigned) {
    if (maximum.isNegative())
      return emit() << "unsigned interval cannot contain negative values";
    minimum = std::max(1u, maximum.getActiveBits());
  } else {
    for (unsigned width = 1; width <= 64; ++width)
      if (fitsSigned(lower, width) && fitsSigned(maximum, width)) {
        minimum = width;
        break;
      }
  }
  if (!minimum || minimum != storage.getWidth())
    return emit() << "integer domain storage is not its canonical width";
  return IntegerDomain{domain, lower, upper, storage, isUnsigned};
}

bool isTrue(Value value) {
  auto op = value.getDefiningOp<arith::ConstantOp>();
  auto attr = op ? dyn_cast<IntegerAttr>(op.getValue()) : IntegerAttr();
  return attr && attr.getType().isInteger(1) && attr.getValue().isOne();
}

FailureOr<Node> node(Attribute raw, StringRef opcode, unsigned operands,
                     Operation *owner) {
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
  if (!dictionary || dictionary.size() != 4 || !id || !operation ||
      operation.getValue() != opcode || !refs || refs.size() != operands ||
      !target || target.size() != 1 ||
      target.getAs<StringAttr>("kind") !=
          StringAttr::get(owner->getContext(), "none") ||
      failed(detail::verifyValueID(id, emit)))
    return emit() << "exact scalar NumericNode has an unsupported shape";
  return Node{id, refs};
}

bool reference(Attribute raw, StringRef kind, unsigned index,
               Operation *owner) {
  auto ref = dyn_cast<DictionaryAttr>(raw);
  auto actualKind = ref ? ref.getAs<StringAttr>("kind") : StringAttr();
  auto actualIndex = ref ? ref.getAs<IntegerAttr>("index") : IntegerAttr();
  if (!ref || ref.size() != 2 || !actualKind || actualKind.getValue() != kind ||
      !actualIndex)
    return false;
  auto value = detail::decodeU32(actualIndex, "NumericRef index",
                                 [&] { return owner->emitOpError(); });
  return succeeded(value) && *value == index;
}

ValueBindingOp binding(RuleOp rule, DictionaryAttr id, Operation *owner) {
  ValueBindingOp found;
  for (ValueBindingOp candidate :
       rule.getBody().front().getOps<ValueBindingOp>())
    if (candidate.getIdAttr() == id) {
      if (found) {
        owner->emitOpError() << "ValueID has duplicate bindings";
        return {};
      }
      found = candidate;
    }
  if (!found)
    owner->emitOpError() << "ValueID has no binding";
  return found;
}

bool matchesExtension(Value actual, Value source, unsigned width,
                      bool isUnsigned, Operation *&extension) {
  auto sourceType = dyn_cast<IntegerType>(source.getType());
  auto actualType = dyn_cast<IntegerType>(actual.getType());
  if (!sourceType || !actualType || actualType.getWidth() != width ||
      sourceType.getWidth() > width)
    return false;
  if (sourceType.getWidth() == width) {
    extension = nullptr;
    return actual == source;
  }
  Operation *candidate = nullptr;
  if (isUnsigned) {
    if (auto op = actual.getDefiningOp<arith::ExtUIOp>())
      candidate = op.getOperation();
  } else if (auto op = actual.getDefiningOp<arith::ExtSIOp>()) {
    candidate = op.getOperation();
  }
  if (!candidate || candidate->getOperand(0) != source)
    return false;
  extension = candidate;
  return true;
}

FailureOr<ScalarWitness> verifyFoundation(RuleOp rule,
                                          ValueBindingOp resultBinding,
                                          NumericProofOp proof,
                                          StringRef opcode) {
  auto emit = [&] { return proof.emitOpError(); };
  if (failed(verifyProofOrigin(proof)))
    return failure();
  ArrayAttr obligations = proof.getObligationsAttr();
  ArrayAttr inputIDs = proof.getInputIdsAttr();
  ArrayAttr inputDomains = proof.getInputDomainsAttr();
  auto requiredChecks = rule->getAttrOfType<ArrayAttr>("ac.required_checks");
  auto requiredObservations =
      rule->getAttrOfType<ArrayAttr>("ac.required_observations");
  if (!obligations || obligations.size() != 3 || !inputIDs ||
      inputIDs.size() != 1 || !inputDomains || inputDomains.size() != 1 ||
      !proof.getChecksAttr().empty() ||
      (requiredChecks && !requiredChecks.empty()) ||
      (requiredObservations && !requiredObservations.empty()) ||
      rule.getBody().getBlocks().size() != 1 || rule.getInputs().size() != 1 ||
      !rule.getTargets().empty())
    return emit() << "exact scalar witness requires one input and three nodes";
  auto from = node(obligations[0], "from_bits", 1, proof);
  auto constant = node(obligations[1], "constant", 1, proof);
  auto operation = node(obligations[2], opcode, 2, proof);
  if (failed(from) || failed(constant) || failed(operation))
    return failure();
  auto inputID = dyn_cast<DictionaryAttr>(inputIDs[0]);
  if (!inputID || failed(detail::verifyValueID(inputID, emit)) ||
      inputID == from->id || inputID == constant->id ||
      inputID == operation->id || from->id == constant->id ||
      from->id == operation->id || constant->id == operation->id ||
      resultBinding.getIdAttr() != operation->id ||
      proof.getResultIdAttr() != operation->id ||
      !reference(from->operands[0], "input", 0, proof) ||
      !reference(operation->operands[0], "node", 0, proof) ||
      !reference(operation->operands[1], "node", 1, proof))
    return emit() << "I/F/C/S IDs and references are not exact";
  for (DictionaryAttr id : {inputID, from->id, constant->id, operation->id})
    if (failed(
            detail::verifyOccurrence(id.getAs<DictionaryAttr>("origin"), emit)))
      return failure();
  if (operation->id.getAs<DictionaryAttr>("origin") != proof.getOriginAttr())
    return emit() << "S ValueID origin must equal the proof origin";
  auto constantRef = dyn_cast<DictionaryAttr>(constant->operands[0]);
  auto recipeValue =
      constantRef ? constantRef.getAs<MathIntAttr>("value") : MathIntAttr();
  if (!constantRef || constantRef.size() != 2 || !recipeValue ||
      constantRef.getAs<StringAttr>("kind") !=
          StringAttr::get(proof.getContext(), "constant"))
    return emit() << "constant node requires one exact MathInt reference";

  auto inputDomain =
      integerDomain(dyn_cast<DictionaryAttr>(inputDomains[0]), proof);
  auto inputBinding = binding(rule, inputID, proof);
  auto fromBinding = binding(rule, from->id, proof);
  auto constantBinding = binding(rule, constant->id, proof);
  if (failed(inputDomain) || !inputBinding || !fromBinding || !constantBinding)
    return failure();
  SmallVector<ValueBindingOp> allBindings(
      rule.getBody().front().getOps<ValueBindingOp>());
  SmallVector<NumericProofOp> allProofs(
      rule.getBody().front().getOps<NumericProofOp>());
  if (allBindings.size() != 4 || allProofs.size() != 1 ||
      allProofs.front() != proof)
    return emit()
           << "exact scalar witness requires exactly I/F/C/S and one proof";
  auto constantDomain = integerDomain(constantBinding.getDomainAttr(), proof);
  if (failed(constantDomain))
    return failure();
  APSInt constantValue = parseMathInt(recipeValue);
  APSInt expectedUpper = constantValue;
  ++expectedUpper;
  if (compare(constantDomain->lower, constantValue) != 0 ||
      compare(constantDomain->upper, expectedUpper) != 0)
    return emit() << "constant binding domain must be singleton [c,c+1)";

  Block &body = rule.getBody().front();
  SmallVector<SourceReadOp> reads(body.getOps<SourceReadOp>());
  auto ruleTypes = rule->getAttrOfType<ArrayAttr>("ac.input_types");
  auto ruleBindings = rule->getAttrOfType<ArrayAttr>("ac.input_bindings");
  if (reads.size() != 1 || !ruleTypes || ruleTypes.size() != 1 ||
      !ruleBindings || ruleBindings.size() != 1 ||
      ruleTypes[0] != inputDomain->attr ||
      failed(detail::verifyStateRef(dyn_cast<DictionaryAttr>(ruleBindings[0]),
                                    emit)))
    return emit() << "exact scalar witness requires one authoritative input";
  SourceReadOp read = reads.front();
  auto current = dyn_cast<BlockArgument>(read.getCurrent());
  auto inputSlot = detail::decodeU32(inputID.getAs<IntegerAttr>("slot"),
                                     "input ValueID slot", emit);
  if (!current || current.getOwner() != &body || current.getArgNumber() != 0 ||
      read.getResult().getType() != inputDomain->storage ||
      read.getCurrent().getType() != inputDomain->storage ||
      inputBinding.getValue() != read.getResult() ||
      fromBinding.getValue() != read.getResult() ||
      inputBinding.getDomainAttr() != inputDomain->attr ||
      fromBinding.getDomainAttr() != inputDomain->attr || failed(inputSlot) ||
      *inputSlot != 0 ||
      inputID.getAs<DictionaryAttr>("origin") !=
          read->getAttrOfType<DictionaryAttr>("ac.origin"))
    return emit() << "I/F must bind the same real SourceRead SSA and domain";
  auto constantOp =
      constantBinding.getValue().getDefiningOp<arith::ConstantOp>();
  auto constantAttr =
      constantOp ? dyn_cast<IntegerAttr>(constantOp.getValue()) : IntegerAttr();
  if (!constantAttr ||
      constantBinding.getValue().getType() != constantDomain->storage ||
      compare(APSInt(constantAttr.getValue(), constantDomain->isUnsigned),
              constantValue) != 0)
    return emit()
           << "constant SSA does not match its arbitrary-precision value";
  if (proof->getOperand(0) != resultBinding.getPath() ||
      proof->getOperand(1) != inputBinding.getValue() ||
      proof->getOperand(2) != inputBinding.getValid() ||
      proof->getOperand(3) != resultBinding.getValue() ||
      proof->getOperand(4) != resultBinding.getValid() ||
      !isTrue(proof->getOperand(0)) || !isTrue(proof->getOperand(2)) ||
      !isTrue(proof->getOperand(4)))
    return emit() << "proof operands must equal literal-valid I/S bindings";
  if (proof.getResultDomainAttr() != resultBinding.getDomainAttr() ||
      !inputBinding->isBeforeInBlock(fromBinding) ||
      !fromBinding->isBeforeInBlock(constantBinding) ||
      !constantBinding->isBeforeInBlock(resultBinding) ||
      !resultBinding->isBeforeInBlock(proof))
    return emit() << "I/F/C/S domains and canonical order are not exact";
  return ScalarWitness{
      read,       inputBinding, fromBinding,     constantBinding,
      *operation, *inputDomain, *constantDomain, constantValue};
}

LogicalResult verifyClosedBody(RuleOp rule, NumericProofOp proof,
                               ArrayRef<Operation *> arithmetic) {
  llvm::SmallPtrSet<Operation *, 16> expected;
  for (Operation *operation : arithmetic)
    if (operation)
      expected.insert(operation);
  for (ValueBindingOp binding :
       rule.getBody().front().getOps<ValueBindingOp>()) {
    if (!isTrue(binding.getPath()) || !isTrue(binding.getValid()) ||
        !binding->isBeforeInBlock(proof))
      return proof.emitOpError()
             << "all scalar bindings require ordered literal-true controls";
    expected.insert(binding.getPath().getDefiningOp());
    expected.insert(binding.getValid().getDefiningOp());
  }
  for (Operation &operation : rule.getBody().front().getOperations()) {
    if (isa<SourceReadOp, ValueBindingOp, YieldOp>(operation))
      continue;
    if (isa<NumericProofOp>(operation)) {
      if (&operation != proof.getOperation())
        return proof.emitOpError() << "scalar rule contains another proof";
      continue;
    }
    if (isa<arith::ConstantOp, arith::ExtUIOp, arith::ExtSIOp, arith::SubIOp,
            arith::CmpIOp>(operation) &&
        expected.contains(&operation))
      continue;
    return proof.emitOpError() << "scalar rule contains an extra operation";
  }
  return success();
}

arith::CmpIPredicate predicate(StringRef name, bool isSigned) {
  if (name == "eq")
    return arith::CmpIPredicate::eq;
  if (name == "ne")
    return arith::CmpIPredicate::ne;
  return llvm::StringSwitch<arith::CmpIPredicate>(name)
      .Case("lt",
            isSigned ? arith::CmpIPredicate::slt : arith::CmpIPredicate::ult)
      .Case("le",
            isSigned ? arith::CmpIPredicate::sle : arith::CmpIPredicate::ule)
      .Case("gt",
            isSigned ? arith::CmpIPredicate::sgt : arith::CmpIPredicate::ugt)
      .Case("ge",
            isSigned ? arith::CmpIPredicate::sge : arith::CmpIPredicate::uge);
}

} // namespace

LogicalResult verifyExactInputConstantSubWitness(RuleOp rule,
                                                 ValueBindingOp resultBinding,
                                                 NumericProofOp proof) {
  auto witness = verifyFoundation(rule, resultBinding, proof, "sub");
  if (failed(witness))
    return failure();
  auto resultDomain = integerDomain(resultBinding.getDomainAttr(), proof);
  if (failed(resultDomain))
    return failure();
  APSInt expectedLower =
      subtract(witness->inputDomain.lower, witness->constantValue);
  APSInt expectedUpper =
      subtract(witness->inputDomain.upper, witness->constantValue);
  if (compare(resultDomain->lower, expectedLower) != 0 ||
      compare(resultDomain->upper, expectedUpper) != 0)
    return proof.emitOpError() << "sub result interval must be [L-c,U-c)";
  if (resultDomain->storage.getWidth() <
          witness->inputDomain.storage.getWidth() ||
      resultDomain->storage.getWidth() <
          witness->constantDomain.storage.getWidth())
    return proof.emitOpError() << "sub result storage must be nonnarrowing";
  auto sub = resultBinding.getValue().getDefiningOp<arith::SubIOp>();
  if (!sub || sub.getOverflowFlags() != arith::IntegerOverflowFlags::none ||
      sub.getType() != resultDomain->storage)
    return proof.emitOpError() << "sub result must be one flag-free arith.subi";
  Operation *inputExt = nullptr;
  Operation *constantExt = nullptr;
  unsigned width = resultDomain->storage.getWidth();
  if (!matchesExtension(sub.getLhs(), witness->read.getResult(), width,
                        witness->inputDomain.isUnsigned, inputExt) ||
      !matchesExtension(sub.getRhs(), witness->constant.getValue(), width,
                        witness->constantDomain.isUnsigned, constantExt))
    return proof.emitOpError() << "subi must preserve ordered F-C operands";
  return verifyClosedBody(rule, proof,
                          {witness->read,
                           witness->constant.getValue().getDefiningOp(),
                           inputExt, constantExt, sub});
}

LogicalResult verifyExactInputConstantCompareWitness(
    RuleOp rule, ValueBindingOp resultBinding, NumericProofOp proof) {
  auto obligations = proof.getObligationsAttr();
  auto compareNode = obligations && obligations.size() == 3
                         ? dyn_cast<DictionaryAttr>(obligations[2])
                         : DictionaryAttr();
  auto opcode =
      compareNode ? compareNode.getAs<StringAttr>("operator") : StringAttr();
  if (!opcode || !llvm::StringSwitch<bool>(opcode.getValue())
                      .Cases({"eq", "ne", "lt", "le", "gt", "ge"}, true)
                      .Default(false))
    return proof.emitOpError() << "comparison witness has unknown predicate";
  auto witness =
      verifyFoundation(rule, resultBinding, proof, opcode.getValue());
  if (failed(witness))
    return failure();
  auto resultDomain = resultBinding.getDomainAttr();
  auto kind = resultDomain.getAs<StringAttr>("kind");
  if (!kind || kind.getValue() != "bool" || resultDomain.size() != 1 ||
      !resultBinding.getValue().getType().isInteger(1))
    return proof.emitOpError() << "comparison result domain must be bool i1";
  auto cmp = resultBinding.getValue().getDefiningOp<arith::CmpIOp>();
  bool workSigned =
      !witness->inputDomain.isUnsigned || !witness->constantDomain.isUnsigned;
  unsigned inputWidth = witness->inputDomain.storage.getWidth() +
                        (workSigned && witness->inputDomain.isUnsigned);
  unsigned constantWidth = witness->constantDomain.storage.getWidth() +
                           (workSigned && witness->constantDomain.isUnsigned);
  unsigned width = std::max(inputWidth, constantWidth);
  Operation *inputExt = nullptr;
  Operation *constantExt = nullptr;
  if (width > 64 || !cmp ||
      cmp.getPredicate() != predicate(opcode.getValue(), workSigned) ||
      !matchesExtension(cmp.getLhs(), witness->read.getResult(), width,
                        witness->inputDomain.isUnsigned, inputExt) ||
      !matchesExtension(cmp.getRhs(), witness->constant.getValue(), width,
                        witness->constantDomain.isUnsigned, constantExt))
    return proof.emitOpError()
           << "cmpi must use sign-aware common extensions and exact predicate";
  return verifyClosedBody(rule, proof,
                          {witness->read,
                           witness->constant.getValue().getDefiningOp(),
                           inputExt, constantExt, cmp});
}

} // namespace acir::ac
