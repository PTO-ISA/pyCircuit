#include "ACIRNumericExactInputAdd.h"

#include "ACIRSourceContracts.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/IR/Location.h"
#include "llvm/ADT/APInt.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallPtrSet.h"

#include <algorithm>
#include <optional>

using namespace mlir;

namespace acir::ac {
namespace {

LogicalResult verifyProofOrigin(NumericProofOp proof) {
  auto emit = [&] { return proof.emitOpError(); };
  auto file = proof->getParentOfType<mlir::ModuleOp>();
  auto sourceOwner =
      file ? file->getAttrOfType<DictionaryAttr>("ac.source_owner")
           : DictionaryAttr();
  auto ownerPath =
      sourceOwner ? sourceOwner.getAs<StringAttr>("path") : StringAttr();
  auto module = proof->getParentOfType<ModuleOp>();
  auto moduleOwner =
      module ? module->getAttrOfType<DictionaryAttr>("ac.source_owner")
             : DictionaryAttr();
  auto symbol =
      module ? module->getAttrOfType<StringAttr>("sym_name") : StringAttr();
  auto location = dyn_cast<FileLineColLoc>(proof->getLoc());
  auto origin = proof.getOriginAttr();
  auto site = origin ? origin.getAs<DictionaryAttr>("site") : DictionaryAttr();
  auto definition =
      site ? site.getAs<FlatSymbolRefAttr>("definition") : FlatSymbolRefAttr();
  auto expansion = origin ? origin.getAs<ArrayAttr>("expansion") : ArrayAttr();
  if (!sourceOwner || failed(detail::verifySourceOwner(sourceOwner, emit)) ||
      moduleOwner != sourceOwner || !ownerPath || !location ||
      location.getFilename() != ownerPath.getValue())
    return emit() << "N0 proof origin requires a source-owned FileLineColLoc";
  if (!symbol || failed(detail::verifyOccurrence(origin, emit)) || !site ||
      !definition || definition.getValue() != symbol.getValue() || !expansion ||
      !expansion.empty())
    return emit() << "N0 proof origin must be a direct module definition Site";
  return success();
}

APSInt parseMathInt(MathIntAttr value) {
  StringRef spelling = value.getCanonicalValue();
  const bool negative = spelling.consume_front("-");
  const unsigned width =
      std::max(2u, static_cast<unsigned>(spelling.size() * 4 + 2));
  APInt parsed(width, spelling, /*radix=*/10);
  if (negative)
    parsed = -parsed;
  return APSInt(std::move(parsed), /*isUnsigned=*/!negative);
}

bool isTrueI1(Value value) {
  auto constant = value.getDefiningOp<arith::ConstantOp>();
  auto integer =
      constant ? dyn_cast<IntegerAttr>(constant.getValue()) : IntegerAttr();
  return integer && integer.getType().isInteger(1) &&
         integer.getValue().isOne();
}

Operation *i1Constant(Value value) {
  auto constant = value.getDefiningOp<arith::ConstantOp>();
  auto integer =
      constant ? dyn_cast<IntegerAttr>(constant.getValue()) : IntegerAttr();
  return integer && integer.getType().isInteger(1) ? constant.getOperation()
                                                   : nullptr;
}

bool fitsSignedWidth(const APInt &value, unsigned width) {
  return value.sextOrTrunc(width).sextOrTrunc(value.getBitWidth()) == value;
}

APInt extendNumericValue(const APSInt &value, unsigned width) {
  return value.isUnsigned() ? value.zextOrTrunc(width)
                            : value.sextOrTrunc(width);
}

int compareNumeric(const APSInt &lhs, const APSInt &rhs) {
  const unsigned width = std::max(lhs.getBitWidth(), rhs.getBitWidth()) + 1;
  APInt left = extendNumericValue(lhs, width);
  APInt right = extendNumericValue(rhs, width);
  if (left == right)
    return 0;
  return left.slt(right) ? -1 : 1;
}

FailureOr<unsigned> minimumIntervalWidth(const APSInt &lower,
                                         const APSInt &upper, bool isUnsigned,
                                         Operation *owner) {
  auto emit = [&] { return owner->emitOpError(); };
  if (compareNumeric(lower, upper) >= 0)
    return emit() << "integer interval must be nonempty [lower,upper)";
  APSInt maximum = upper;
  --maximum;
  if (isUnsigned) {
    if (lower.isNegative() || maximum.isNegative())
      return emit() << "unsigned interval cannot contain negative values";
    return std::max(1u, maximum.getActiveBits());
  }
  for (unsigned width = 1; width <= 64; ++width)
    if (fitsSignedWidth(lower, width) && fitsSignedWidth(maximum, width))
      return width;
  return emit() << "exact interval requires unsupported signed i65 storage";
}

struct IntegerDomain {
  DictionaryAttr attribute;
  APSInt lower;
  APSInt upper;
  Type storage;
  unsigned width;
  bool isUnsigned;
};

FailureOr<IntegerDomain> verifyIntegerDomain(DictionaryAttr domain,
                                             Operation *owner) {
  auto emit = [&] { return owner->emitOpError(); };
  if (failed(detail::verifyLogicalTypeStructure(domain, emit)))
    return failure();
  auto kind = domain.getAs<StringAttr>("kind");
  auto storage = domain.getAs<TypeAttr>("storage");
  auto lowerAttr = domain.getAs<MathIntAttr>("lower");
  auto upperAttr = domain.getAs<MathIntAttr>("upper");
  auto interpretation = domain.getAs<StringAttr>("interpretation");
  auto integer =
      storage ? dyn_cast<IntegerType>(storage.getValue()) : IntegerType();
  if (!kind || kind.getValue() != "integer" || domain.size() != 5 || !storage ||
      !integer || !integer.isSignless() || integer.getWidth() == 0 ||
      integer.getWidth() > 64 || !lowerAttr || !upperAttr || !interpretation ||
      (interpretation.getValue() != "signed" &&
       interpretation.getValue() != "unsigned"))
    return emit() << "N0-B1b requires a closed signless i1..i64 integer domain";

  APSInt lower = parseMathInt(lowerAttr);
  APSInt upper = parseMathInt(upperAttr);
  const bool isUnsigned = interpretation.getValue() == "unsigned";
  if ((lower.isNegative() && isUnsigned) ||
      (!lower.isNegative() && !isUnsigned))
    return emit()
           << "domain interpretation must match canonical lower-bound sign";
  auto minimumWidth = minimumIntervalWidth(lower, upper, isUnsigned, owner);
  if (failed(minimumWidth))
    return failure();
  if (*minimumWidth != integer.getWidth())
    return emit()
           << "integer interval storage must use its canonical minimum width";
  return IntegerDomain{domain,        lower,     upper, storage.getValue(),
                       *minimumWidth, isUnsigned};
}

FailureOr<APSInt> verifySingletonConstantDomain(DictionaryAttr domain,
                                                MathIntAttr recipeValue,
                                                Operation *owner) {
  auto verified = verifyIntegerDomain(domain, owner);
  if (failed(verified))
    return failure();
  APSInt value = parseMathInt(recipeValue);
  APSInt expectedUpper = value;
  ++expectedUpper;
  if (compareNumeric(verified->lower, value) != 0 ||
      compareNumeric(verified->upper, expectedUpper) != 0)
    return owner->emitOpError()
           << "constant domain and NumericRef must be singleton [c,c+1)";
  if ((verified->isUnsigned && value.isNegative()) ||
      (!verified->isUnsigned && !value.isNegative()))
    return owner->emitOpError()
           << "constant value sign must match its integer interpretation";
  return value;
}

APSInt addExact(const APSInt &lhs, const APSInt &rhs) {
  const unsigned width = std::max(lhs.getBitWidth(), rhs.getBitWidth()) + 1;
  APInt left = extendNumericValue(lhs, width);
  APInt right = extendNumericValue(rhs, width);
  APInt result = left + right;
  const bool isNegative =
      lhs.isNegative() && rhs.isNegative()
          ? true
          : lhs.isNegative() != rhs.isNegative() && result.isNegative();
  const bool isUnsigned = !isNegative;
  return APSInt(std::move(result), isUnsigned);
}

bool isNoneTarget(DictionaryAttr target, MLIRContext *context) {
  return target && target.size() == 1 &&
         target.getAs<StringAttr>("kind") == StringAttr::get(context, "none");
}

bool isReference(DictionaryAttr reference, StringRef kind, unsigned index,
                 MLIRContext *context, Operation *owner) {
  auto actualKind =
      reference ? reference.getAs<StringAttr>("kind") : StringAttr();
  auto actualIndex =
      reference ? reference.getAs<IntegerAttr>("index") : IntegerAttr();
  if (!reference || reference.size() != 2 || !actualKind ||
      actualKind.getValue() != kind || !actualIndex)
    return false;
  auto ordinal = detail::decodeU32(actualIndex, "N0 NumericRef index",
                                   [&] { return owner->emitOpError(); });
  return succeeded(ordinal) && *ordinal == index;
}

struct NodeView {
  DictionaryAttr id;
  StringRef operation;
  ArrayAttr operands;
  DictionaryAttr target;
};

FailureOr<NodeView> parseNode(Attribute raw, StringRef operation,
                              size_t operandCount, Operation *owner) {
  auto emit = [&] { return owner->emitOpError(); };
  auto node = dyn_cast<DictionaryAttr>(raw);
  auto id = node ? node.getAs<DictionaryAttr>("id") : DictionaryAttr();
  auto opcode = node ? node.getAs<StringAttr>("operator") : StringAttr();
  auto operands = node ? node.getAs<ArrayAttr>("operands") : ArrayAttr();
  auto target = node ? node.getAs<DictionaryAttr>("target") : DictionaryAttr();
  if (!node || node.size() != 4 || !id || !opcode ||
      opcode.getValue() != operation || !operands ||
      operands.size() != operandCount ||
      !isNoneTarget(target, owner->getContext()) ||
      failed(detail::verifyValueID(id, emit)))
    return emit() << "N0-B1b NumericNode has an unsupported closed shape";
  return NodeView{id, opcode.getValue(), operands, target};
}

ValueBindingOp findBinding(RuleOp rule, DictionaryAttr id, Operation *owner) {
  ValueBindingOp found;
  for (ValueBindingOp binding : rule.getBody().front().getOps<ValueBindingOp>())
    if (binding.getIdAttr() == id) {
      if (found) {
        owner->emitOpError() << "N0 ValueID has duplicate ValueBindings";
        return {};
      }
      found = binding;
    }
  if (!found)
    owner->emitOpError() << "N0 ValueID has no ValueBinding";
  return found;
}

bool matchesExtension(Value actual, Value source, unsigned resultWidth,
                      bool isUnsigned, Operation *&extension) {
  auto sourceType = dyn_cast<IntegerType>(source.getType());
  auto actualType = dyn_cast<IntegerType>(actual.getType());
  if (!sourceType || !actualType || actualType.getWidth() != resultWidth ||
      sourceType.getWidth() > resultWidth)
    return false;
  if (sourceType.getWidth() == resultWidth) {
    extension = nullptr;
    return actual == source;
  }
  if (isUnsigned) {
    auto ext = actual.getDefiningOp<arith::ExtUIOp>();
    if (!ext || ext.getIn() != source)
      return false;
    extension = ext.getOperation();
    return true;
  }
  auto ext = actual.getDefiningOp<arith::ExtSIOp>();
  if (!ext || ext.getIn() != source)
    return false;
  extension = ext.getOperation();
  return true;
}

LogicalResult verifyRealInputAuthority(RuleOp rule, Value handle,
                                       DictionaryAttr binding,
                                       const IntegerDomain &domain,
                                       Operation *owner) {
  auto emit = [&] { return owner->emitOpError(); };
  if (failed(detail::verifyStateRef(binding, emit)))
    return failure();
  auto module = rule->getParentOfType<ModuleOp>();
  auto file = owner->getParentOfType<mlir::ModuleOp>();
  auto moduleOwner =
      module ? module->getAttrOfType<DictionaryAttr>("ac.source_owner")
             : DictionaryAttr();
  auto fileOwner = file ? file->getAttrOfType<DictionaryAttr>("ac.source_owner")
                        : DictionaryAttr();
  auto handleType = dyn_cast<RegType>(handle.getType());
  if (!module || !file || !moduleOwner || moduleOwner != fileOwner ||
      failed(detail::verifySourceOwner(moduleOwner, emit)) || !handleType ||
      handleType.getElementType() != domain.storage)
    return emit()
           << "N0 input handle physical type must equal its logical domain";
  auto kind = binding.getAs<StringAttr>("kind");
  if (!kind)
    return emit() << "N0 input requires a real owned or formal StateRef";
  if (kind.getValue() == "owned") {
    auto state = handle.getDefiningOp<RegOp>();
    auto element = binding.getAs<ArrayAttr>("element");
    auto stateElement =
        state ? state->getAttrOfType<ArrayAttr>("ac.element") : ArrayAttr();
    auto shape =
        state ? state->getAttrOfType<ArrayAttr>("ac.shape") : ArrayAttr();
    if (!state || state->getParentOfType<ModuleOp>() != module || !element ||
        !element.empty() || !stateElement || !stateElement.empty() || !shape ||
        !shape.empty() ||
        binding.getAs<DictionaryAttr>("declaration") !=
            state->getAttrOfType<DictionaryAttr>("ac.declaration") ||
        domain.attribute !=
            state->getAttrOfType<DictionaryAttr>("ac.logical_element") ||
        state->getAttrOfType<DictionaryAttr>("ac.source_owner") !=
            moduleOwner ||
        failed(detail::verifyOccurrence(
            state->getAttrOfType<DictionaryAttr>("ac.declaration"), emit)))
      return emit() << "N0 owned input must be the real scalar source reg";
    return success();
  }
  if (kind.getValue() != "formal")
    return emit() << "N0 input StateRef kind must be owned or formal";
  auto formal = dyn_cast<BlockArgument>(handle);
  auto ports = module->getAttrOfType<ArrayAttr>("ac.ports");
  if (!formal || formal.getOwner() != &module.getBody().front() ||
      formal.getArgNumber() < 2 || !ports ||
      formal.getArgNumber() - 2 >= ports.size())
    return emit() << "N0 formal input must be a real module port";
  auto port = dyn_cast<DictionaryAttr>(ports[formal.getArgNumber() - 2]);
  auto role = port ? port.getAs<StringAttr>("role") : StringAttr();
  if (!port || port.size() != 6 || !role || role.getValue() != "current" ||
      port.getAs<StringAttr>("parameter") !=
          binding.getAs<StringAttr>("parameter") ||
      port.get("ordinal") != binding.get("ordinal") ||
      port.getAs<DictionaryAttr>("type") != domain.attribute ||
      failed(detail::verifyOccurrence(port.getAs<DictionaryAttr>("origin"),
                                      emit)) ||
      failed(detail::verifySourceSpan(port.getAs<DictionaryAttr>("location"),
                                      emit)) ||
      port.getAs<DictionaryAttr>("location").getAs<StringAttr>("path") !=
          moduleOwner.getAs<StringAttr>("path"))
    return emit() << "N0 formal input must match its current PortSlot";
  return success();
}

} // namespace

LogicalResult verifyExactInputConstantAddWitness(RuleOp rule,
                                                 ValueBindingOp resultBinding,
                                                 NumericProofOp proof) {
  auto emit = [&] { return proof.emitOpError(); };
  auto obligations = proof.getObligationsAttr();
  auto inputIDs = proof.getInputIdsAttr();
  auto inputDomains = proof.getInputDomainsAttr();
  auto checks = proof.getChecksAttr();
  if (!obligations || obligations.size() != 3 || !inputIDs ||
      inputIDs.size() != 1 || !inputDomains || inputDomains.size() != 1 ||
      !checks || !checks.empty())
    return emit() << "N0-B1b requires one input and exactly from_bits, "
                     "constant, add nodes";
  auto proofResultId = proof.getResultIdAttr();
  if (failed(verifyProofOrigin(proof)) || !proofResultId ||
      proofResultId.get("origin") != proof.getOriginAttr())
    return emit() << "N0 result ID origin must equal its direct proof origin";

  auto fromBitsNode = parseNode(obligations[0], "from_bits", 1, proof);
  auto constantNode = parseNode(obligations[1], "constant", 1, proof);
  auto addNode = parseNode(obligations[2], "add", 2, proof);
  if (failed(fromBitsNode) || failed(constantNode) || failed(addNode))
    return failure();
  NodeView fromBits = *fromBitsNode;
  NodeView constant = *constantNode;
  NodeView add = *addNode;
  DictionaryAttr idInput = dyn_cast<DictionaryAttr>(inputIDs[0]);
  DictionaryAttr idFromBits = fromBits.id;
  DictionaryAttr idConstant = constant.id;
  DictionaryAttr idAdd = add.id;
  if (!idInput ||
      failed(detail::verifyValueID(idInput,
                                   [&] { return proof.emitOpError(); })) ||
      idInput == idFromBits || idInput == idConstant || idInput == idAdd ||
      idFromBits == idConstant || idFromBits == idAdd || idConstant == idAdd ||
      proof.getResultIdAttr() != idAdd || resultBinding.getIdAttr() != idAdd)
    return emit() << "N0-B1b I/F/C/S ValueIDs must be valid and distinct";

  auto refInput = dyn_cast<DictionaryAttr>(fromBits.operands[0]);
  auto constantRef = dyn_cast<DictionaryAttr>(constant.operands[0]);
  auto addRef0 = dyn_cast<DictionaryAttr>(add.operands[0]);
  auto addRef1 = dyn_cast<DictionaryAttr>(add.operands[1]);
  auto recipeConstant =
      constantRef ? constantRef.getAs<MathIntAttr>("value") : MathIntAttr();
  if (!isReference(refInput, "input", 0, proof.getContext(), proof) ||
      !constantRef || constantRef.size() != 2 ||
      constantRef.getAs<StringAttr>("kind") !=
          StringAttr::get(proof.getContext(), "constant") ||
      !recipeConstant ||
      !isReference(addRef0, "node", 0, proof.getContext(), proof) ||
      !isReference(addRef1, "node", 1, proof.getContext(), proof))
    return emit() << "N0-B1b refs must be from_bits(input0), constant, "
                     "add(node0,node1)";

  auto inputDomainAttr = dyn_cast<DictionaryAttr>(inputDomains[0]);
  auto inputDomain = verifyIntegerDomain(inputDomainAttr, proof);
  auto constantBinding = findBinding(rule, idConstant, proof);
  auto fromBitsBinding = findBinding(rule, idFromBits, proof);
  auto inputBinding = findBinding(rule, idInput, proof);
  if (failed(inputDomain) || !inputBinding || !fromBitsBinding ||
      !constantBinding)
    return failure();
  auto constantDomain = verifySingletonConstantDomain(
      constantBinding.getDomainAttr(), recipeConstant, proof);
  auto constantDomainBinding =
      verifyIntegerDomain(constantBinding.getDomainAttr(), proof);
  auto resultDomain = verifyIntegerDomain(resultBinding.getDomainAttr(), proof);
  if (failed(constantDomain) || failed(constantDomainBinding) ||
      failed(resultDomain))
    return failure();

  auto ruleInputTypes = rule->getAttrOfType<ArrayAttr>("ac.input_types");
  auto ruleInputBindings = rule->getAttrOfType<ArrayAttr>("ac.input_bindings");
  auto requiredChecks = rule->getAttrOfType<ArrayAttr>("ac.required_checks");
  auto requiredObservations =
      rule->getAttrOfType<ArrayAttr>("ac.required_observations");
  if (rule.getInputs().size() != 1 || !rule.getTargets().empty() ||
      !ruleInputTypes || ruleInputTypes.size() != 1 || !ruleInputBindings ||
      ruleInputBindings.size() != 1 || ruleInputTypes[0] != inputDomainAttr ||
      proof.getInputDomainsAttr()[0] != inputDomainAttr ||
      (requiredChecks && !requiredChecks.empty()) ||
      (requiredObservations && !requiredObservations.empty()) ||
      failed(verifyRealInputAuthority(
          rule, rule.getInputs()[0],
          dyn_cast<DictionaryAttr>(ruleInputBindings[0]), *inputDomain, proof)))
    return emit() << "N0 proof must bind exactly one real scalar current input";

  Block &body = rule.getBody().front();
  auto reads = body.getOps<SourceReadOp>();
  if (std::distance(reads.begin(), reads.end()) != 1)
    return emit() << "N0-B1b requires exactly one source.read";
  SourceReadOp read = *reads.begin();
  auto current = dyn_cast<BlockArgument>(read.getCurrent());
  auto inputIDOrigin = idInput.getAs<DictionaryAttr>("origin");
  auto inputIDSlot = idInput.getAs<IntegerAttr>("slot");
  auto inputSlot = detail::decodeU32(inputIDSlot, "N0 input ValueID slot",
                                     [&] { return proof.emitOpError(); });
  if (read->getBlock() != &body || !current || current.getOwner() != &body ||
      current.getArgNumber() != 0 ||
      read.getResult().getType() != inputDomain->storage ||
      read.getCurrent().getType() != inputDomain->storage ||
      !read->isBeforeInBlock(fromBitsBinding) ||
      inputBinding.getValue() != read.getResult() ||
      fromBitsBinding.getValue() != read.getResult() ||
      inputBinding.getDomainAttr() != inputDomainAttr ||
      fromBitsBinding.getDomainAttr() != inputDomainAttr || !inputIDOrigin ||
      inputIDOrigin != read->getAttrOfType<DictionaryAttr>("ac.origin") ||
      failed(inputSlot) || *inputSlot != 0)
    return emit()
           << "I and F must bind the exact same real SourceRead SSA/domain";

  auto constantInteger =
      dyn_cast<IntegerType>(constantBinding.getValue().getType());
  auto constantOp =
      constantBinding.getValue().getDefiningOp<arith::ConstantOp>();
  auto constantAttr =
      constantOp ? dyn_cast<IntegerAttr>(constantOp.getValue()) : IntegerAttr();
  if (!constantOp || !constantInteger || !constantInteger.isSignless() ||
      constantBinding.getValue().getType() != constantBinding.getDomainAttr()
                                                  .getAs<TypeAttr>("storage")
                                                  .getValue() ||
      !constantAttr)
    return emit() << "N0 constant binding must name an exact integer constant";
  APSInt actualConstant(constantAttr.getValue(),
                        constantDomainBinding->isUnsigned);
  if (compareNumeric(actualConstant, *constantDomain) != 0)
    return emit() << "N0 constant bit pattern disagrees with its NumericRef";

  auto resultStorage = dyn_cast<IntegerType>(resultDomain->storage);
  if (!resultStorage)
    return emit() << "N0 result storage must be signless integer";
  APSInt expectedLower = addExact(inputDomain->lower, *constantDomain);
  APSInt expectedUpper = addExact(inputDomain->upper, *constantDomain);
  if (compareNumeric(resultDomain->lower, expectedLower) != 0 ||
      compareNumeric(resultDomain->upper, expectedUpper) != 0)
    return emit() << "N0 exact result interval must be [L+k,U+k)";
  if (resultDomain->width < inputDomain->width ||
      resultDomain->width < constantInteger.getWidth())
    return emit() << "N0 rejects result narrowing";

  auto sum = resultBinding.getValue().getDefiningOp<arith::AddIOp>();
  auto resultWidth = resultStorage.getWidth();
  if (!sum || sum.getOverflowFlags() != arith::IntegerOverflowFlags::none ||
      sum.getResult().getType() != resultDomain->storage ||
      resultBinding.getValue() != sum.getResult())
    return emit() << "N0 actual result must be one flag-free arith.addi";
  Operation *inputExtension = nullptr;
  Operation *constantExtension = nullptr;
  bool matched =
      matchesExtension(sum.getLhs(), read.getResult(), resultWidth,
                       inputDomain->isUnsigned, inputExtension) &&
      matchesExtension(sum.getRhs(), constantBinding.getValue(), resultWidth,
                       constantDomainBinding->isUnsigned, constantExtension);
  if (!matched) {
    inputExtension = nullptr;
    constantExtension = nullptr;
    matched =
        matchesExtension(sum.getRhs(), read.getResult(), resultWidth,
                         inputDomain->isUnsigned, inputExtension) &&
        matchesExtension(sum.getLhs(), constantBinding.getValue(), resultWidth,
                         constantDomainBinding->isUnsigned, constantExtension);
  }
  if (!matched)
    return emit() << "N0 addi operands must be the approved source/constant "
                     "extensions";
  if ((inputExtension && (inputExtension->getBlock() != &body ||
                          !inputExtension->isBeforeInBlock(sum))) ||
      (constantExtension && (constantExtension->getBlock() != &body ||
                             !constantExtension->isBeforeInBlock(sum))))
    return emit()
           << "N0 extensions must be ordered in the rule computation block";

  if (proof.getInputIdsAttr()[0] != idInput ||
      proof.getResultDomainAttr() != resultBinding.getDomainAttr() ||
      proof->getOperand(0) != resultBinding.getPath() ||
      proof->getOperand(1) != inputBinding.getValue() ||
      proof->getOperand(2) != inputBinding.getValid() ||
      proof->getOperand(3) != resultBinding.getValue() ||
      proof->getOperand(4) != resultBinding.getValid())
    return emit() << "N0 proof operands/domains must equal I and S bindings";

  llvm::SmallPtrSet<Operation *, 16> expected;
  expected.insert(read.getOperation());
  expected.insert(constantOp.getOperation());
  expected.insert(sum.getOperation());
  if (inputExtension)
    expected.insert(inputExtension);
  if (constantExtension)
    expected.insert(constantExtension);
  for (ValueBindingOp binding :
       {inputBinding, fromBitsBinding, constantBinding, resultBinding}) {
    auto path = i1Constant(binding.getPath());
    auto valid = i1Constant(binding.getValid());
    auto producer = binding.getValue().getDefiningOp();
    if (!isTrueI1(binding.getPath()) || !isTrueI1(binding.getValid()) ||
        !path || !valid || !producer || producer->getBlock() != &body ||
        path->getBlock() != &body || valid->getBlock() != &body ||
        !producer->isBeforeInBlock(binding) ||
        !path->isBeforeInBlock(binding) || !valid->isBeforeInBlock(binding) ||
        !binding->isBeforeInBlock(proof))
      return emit()
             << "N0 bindings require ordered literal-true path/valid controls";
    expected.insert(path);
    expected.insert(valid);
  }
  if (!inputBinding->isBeforeInBlock(fromBitsBinding) ||
      !fromBitsBinding->isBeforeInBlock(constantBinding) ||
      !constantBinding->isBeforeInBlock(resultBinding) ||
      resultBinding.getIdAttr() != idAdd || !isTrueI1(proof->getOperand(0)) ||
      !isTrueI1(proof->getOperand(2)) || !isTrueI1(proof->getOperand(4)))
    return emit()
           << "N0 I/F/C/S binding order and proof controls are not canonical";

  for (Operation &operation : body.getOperations()) {
    if (isa<arith::ConstantOp>(operation)) {
      if (!expected.contains(&operation))
        return emit() << "N0 rule contains an unowned arithmetic constant";
      continue;
    }
    if (isa<SourceReadOp, arith::ExtUIOp, arith::ExtSIOp, arith::AddIOp,
            ValueBindingOp, NumericProofOp, YieldOp>(operation)) {
      if ((isa<arith::ExtUIOp, arith::ExtSIOp, arith::AddIOp>(operation) &&
           !expected.contains(&operation)) ||
          (isa<NumericProofOp>(operation) &&
           &operation != proof.getOperation()))
        return emit() << "N0 rule contains an extra arithmetic/proof operation";
      continue;
    }
    return emit() << "N0 rule contains an unsupported operation";
  }
  return success();
}

} // namespace acir::ac
