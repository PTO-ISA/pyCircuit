#include "Dialect/ACIR/ACIRNumericExactScalar.h"

#include "Dialect/ACIR/ACIRSourceContracts.h"
#include "acir/Dialect/ACIR/ACIROps.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "llvm/ADT/APSInt.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/StringSwitch.h"

#include <algorithm>

using namespace mlir;

namespace acir::compiler {
namespace {

struct SourceScalar {
  ac::SourceReadOp read;
  ac::MathFromBitsOp fromBits;
  ac::MathConstantOp constant;
  Operation *operation;
  StringRef opcode;
  DictionaryAttr inputID;
  DictionaryAttr fromID;
  DictionaryAttr constantID;
  DictionaryAttr resultID;
  DictionaryAttr inputDomain;
  APSInt constantValue;
  Value path;
};

struct DomainPlan {
  DictionaryAttr attr;
  IntegerType storage;
  bool isUnsigned;
};

APSInt mathValue(ac::MathIntAttr value) {
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

FailureOr<DomainPlan> integerDomain(Builder &builder, const APSInt &lower,
                                    const APSInt &upper, Operation *owner) {
  auto emit = [&] { return owner->emitOpError(); };
  if (compare(lower, upper) >= 0)
    return emit() << "scalar interval must be nonempty";
  bool isUnsigned = !lower.isNegative();
  APSInt maximum = upper;
  --maximum;
  unsigned width = 0;
  if (isUnsigned) {
    if (maximum.isNegative())
      return emit() << "unsigned scalar interval crosses below zero";
    width = std::max(1u, maximum.getActiveBits());
  } else {
    for (unsigned candidate = 1; candidate <= 64; ++candidate)
      if (fitsSigned(lower, candidate) && fitsSigned(maximum, candidate)) {
        width = candidate;
        break;
      }
  }
  if (!width || width > 64)
    return emit() << "scalar interval requires unsupported storage above i64";
  auto storage = builder.getIntegerType(width);
  auto domain = builder.getDictionaryAttr({
      builder.getNamedAttr("kind", builder.getStringAttr("integer")),
      builder.getNamedAttr("storage", TypeAttr::get(storage)),
      builder.getNamedAttr("lower",
                           ac::MathIntAttr::get(builder.getContext(), lower)),
      builder.getNamedAttr("upper",
                           ac::MathIntAttr::get(builder.getContext(), upper)),
      builder.getNamedAttr(
          "interpretation",
          builder.getStringAttr(isUnsigned ? "unsigned" : "signed")),
  });
  if (failed(ac::detail::verifyLogicalTypeStructure(domain, emit)))
    return failure();
  return DomainPlan{domain, storage, isUnsigned};
}

DictionaryAttr boolDomain(Builder &builder) {
  return builder.getDictionaryAttr({
      builder.getNamedAttr("kind", builder.getStringAttr("bool")),
  });
}

bool trueConstant(Value value) {
  auto constant = value.getDefiningOp<arith::ConstantOp>();
  auto integer =
      constant ? dyn_cast<IntegerAttr>(constant.getValue()) : IntegerAttr();
  return integer && integer.getType().isInteger(1) &&
         integer.getValue().isOne();
}

FailureOr<DictionaryAttr> recipeNode(Attribute raw, StringRef opcode,
                                     unsigned operands, Operation *owner) {
  auto emit = [&] { return owner->emitOpError(); };
  auto node = dyn_cast<DictionaryAttr>(raw);
  auto id = node ? node.getAs<DictionaryAttr>("id") : DictionaryAttr();
  auto operation = node ? node.getAs<StringAttr>("operator") : StringAttr();
  auto refs = node ? node.getAs<ArrayAttr>("operands") : ArrayAttr();
  auto target = node ? node.getAs<DictionaryAttr>("target") : DictionaryAttr();
  if (!node || node.size() != 4 || !id || !operation ||
      operation.getValue() != opcode || !refs || refs.size() != operands ||
      !target || target.size() != 1 ||
      target.getAs<StringAttr>("kind") !=
          StringAttr::get(owner->getContext(), "none") ||
      failed(ac::detail::verifyValueID(id, emit)))
    return emit() << "scalar source NumericNode has unsupported shape";
  return node;
}

bool exactRef(Attribute raw, StringRef kind, unsigned index, Operation *owner) {
  auto ref = dyn_cast<DictionaryAttr>(raw);
  auto actualKind = ref ? ref.getAs<StringAttr>("kind") : StringAttr();
  auto actualIndex = ref ? ref.getAs<IntegerAttr>("index") : IntegerAttr();
  if (!ref || ref.size() != 2 || !actualKind || actualKind.getValue() != kind ||
      !actualIndex)
    return false;
  auto decoded = ac::detail::decodeU32(actualIndex, "NumericRef index",
                                       [&] { return owner->emitOpError(); });
  return succeeded(decoded) && *decoded == index;
}

FailureOr<SourceScalar> inspectSource(ac::RuleOp rule) {
  auto emit = [&] { return rule.emitOpError(); };
  if (rule.getBody().getBlocks().size() != 1 || rule.getInputs().size() != 1 ||
      !rule.getTargets().empty())
    return emit() << "scalar lowering requires one input-only rule block";
  Block &body = rule.getBody().front();
  SmallVector<ac::SourceReadOp> reads(body.getOps<ac::SourceReadOp>());
  SmallVector<ac::MathFromBitsOp> fromBits(body.getOps<ac::MathFromBitsOp>());
  SmallVector<ac::MathConstantOp> constants(body.getOps<ac::MathConstantOp>());
  SmallVector<ac::MathBinaryOp> binaries(body.getOps<ac::MathBinaryOp>());
  SmallVector<ac::MathCompareOp> compares(body.getOps<ac::MathCompareOp>());
  if (reads.size() != 1 || fromBits.size() != 1 || constants.size() != 1 ||
      binaries.size() + compares.size() != 1 ||
      !body.getOps<ac::ValueBindingOp>().empty() ||
      !body.getOps<ac::NumericProofOp>().empty())
    return emit() << "scalar lowering requires exactly read/F/C/operation";
  Operation *operation = binaries.empty() ? compares.front().getOperation()
                                          : binaries.front().getOperation();
  StringAttr operationAttr =
      binaries.empty() ? compares.front().getPredicateAttr()
                       : operation->getAttrOfType<StringAttr>("operator");
  if (!operationAttr ||
      !llvm::StringSwitch<bool>(operationAttr.getValue())
           .Cases({"sub", "eq", "ne", "lt", "le", "gt", "ge"}, true)
           .Default(false))
    return emit() << "scalar lowering found an unsupported operator";
  StringRef opcode = operationAttr.getValue();
  if ((opcode == "sub") != !binaries.empty())
    return emit() << "scalar operator kind does not match its source op";
  Value path = operation->getOperand(0);
  Value lhs = operation->getOperand(1);
  Value lhsValid = operation->getOperand(2);
  Value rhs = operation->getOperand(3);
  Value rhsValid = operation->getOperand(4);
  if (fromBits.front().getValue() != reads.front().getResult() ||
      lhs != fromBits.front().getResult() ||
      rhs != constants.front().getResult() || !trueConstant(path) ||
      !trueConstant(lhsValid) || !trueConstant(rhsValid) ||
      !llvm::hasSingleElement(reads.front().getResult().getUses()) ||
      !llvm::hasSingleElement(fromBits.front().getResult().getUses()) ||
      !llvm::hasSingleElement(constants.front().getResult().getUses()) ||
      !operation->getResult(0).use_empty() ||
      !operation->getResult(1).use_empty())
    return emit() << "scalar source SSA/control graph is not exact";
  auto required = rule->getAttrOfType<ArrayAttr>("ac.required_numeric");
  auto inputTypes = rule->getAttrOfType<ArrayAttr>("ac.input_types");
  auto inputBindings = rule->getAttrOfType<ArrayAttr>("ac.input_bindings");
  auto outputTypes = rule->getAttrOfType<ArrayAttr>("ac.output_types");
  auto outputBindings = rule->getAttrOfType<ArrayAttr>("ac.output_bindings");
  auto requiredChecks = rule->getAttrOfType<ArrayAttr>("ac.required_checks");
  auto requiredObservations =
      rule->getAttrOfType<ArrayAttr>("ac.required_observations");
  if (!required || required.size() != 3 ||
      !rule->getAttrOfType<DictionaryAttr>("ac.proof_scope") || !inputTypes ||
      inputTypes.size() != 1 || !inputBindings || inputBindings.size() != 1 ||
      !outputTypes || !outputTypes.empty() || !outputBindings ||
      !outputBindings.empty() || (requiredChecks && !requiredChecks.empty()) ||
      (requiredObservations && !requiredObservations.empty()) ||
      failed(ac::detail::verifyStateRef(
          dyn_cast<DictionaryAttr>(inputBindings[0]), emit)))
    return emit() << "scalar source rule has no closed recipe/proof scope";
  auto fromNode = recipeNode(required[0], "from_bits", 1, rule);
  auto constantNode = recipeNode(required[1], "constant", 1, rule);
  auto resultNode = recipeNode(required[2], opcode, 2, rule);
  if (failed(fromNode) || failed(constantNode) || failed(resultNode))
    return failure();
  auto fromOperands = (*fromNode).getAs<ArrayAttr>("operands");
  auto constantOperands = (*constantNode).getAs<ArrayAttr>("operands");
  auto resultOperands = (*resultNode).getAs<ArrayAttr>("operands");
  auto constantRef = dyn_cast<DictionaryAttr>(constantOperands[0]);
  auto constantValue = constantRef ? constantRef.getAs<ac::MathIntAttr>("value")
                                   : ac::MathIntAttr();
  if (!exactRef(fromOperands[0], "input", 0, rule) || !constantRef ||
      constantRef.size() != 2 || !constantValue ||
      constantRef.getAs<StringAttr>("kind") !=
          StringAttr::get(rule.getContext(), "constant") ||
      constantValue != constants.front().getValueAttr() ||
      !exactRef(resultOperands[0], "node", 0, rule) ||
      !exactRef(resultOperands[1], "node", 1, rule))
    return emit() << "scalar source recipe refs do not match F/C operation";
  Builder builder(rule.getContext());
  auto inputID = builder.getDictionaryAttr({
      builder.getNamedAttr(
          "origin", reads.front()->getAttrOfType<DictionaryAttr>("ac.origin")),
      builder.getNamedAttr("slot", builder.getI32IntegerAttr(0)),
  });
  auto fromID = (*fromNode).getAs<DictionaryAttr>("id");
  auto constantID = (*constantNode).getAs<DictionaryAttr>("id");
  auto resultID = (*resultNode).getAs<DictionaryAttr>("id");
  if (fromID.getAs<DictionaryAttr>("origin") !=
          fromBits.front()->getAttrOfType<DictionaryAttr>("ac.origin") ||
      constantID.getAs<DictionaryAttr>("origin") !=
          constants.front()->getAttrOfType<DictionaryAttr>("ac.origin") ||
      resultID.getAs<DictionaryAttr>("origin") !=
          operation->getAttrOfType<DictionaryAttr>("ac.origin") ||
      inputID == fromID || inputID == constantID || inputID == resultID ||
      fromID == constantID || fromID == resultID || constantID == resultID)
    return emit() << "scalar I/F/C/S ValueIDs must be distinct";
  auto inputDomain = fromBits.front().getDomainAttr();
  auto inputStorage = inputDomain.getAs<TypeAttr>("storage");
  auto handleType = dyn_cast<ac::RegType>(rule.getInputs()[0].getType());
  auto current = dyn_cast<BlockArgument>(reads.front().getCurrent());
  if (inputTypes[0] != inputDomain || !inputStorage || !handleType ||
      handleType.getElementType() != inputStorage.getValue() || !current ||
      current.getOwner() != &body || current.getArgNumber() != 0 ||
      reads.front().getResult().getType() != inputStorage.getValue())
    return emit() << "scalar SourceRead is not the authoritative real input";
  for (Operation &nested : body.getOperations()) {
    if (isa<ac::SourceReadOp, ac::MathFromBitsOp, ac::MathConstantOp,
            ac::MathBinaryOp, ac::MathCompareOp, ac::YieldOp>(nested))
      continue;
    if (auto control = dyn_cast<arith::ConstantOp>(nested)) {
      if (!trueConstant(control.getResult()) ||
          llvm::any_of(control.getResult().getUsers(),
                       [&](Operation *user) { return user != operation; }))
        return emit()
               << "scalar source permits only operation control constants";
      continue;
    }
    return emit() << "scalar source rule contains an extra operation";
  }
  return SourceScalar{reads.front(),
                      fromBits.front(),
                      constants.front(),
                      operation,
                      opcode,
                      inputID,
                      fromID,
                      constantID,
                      resultID,
                      inputDomain,
                      mathValue(constantValue),
                      path};
}

ac::ValueBindingOp bind(OpBuilder &builder, Location location, Value value,
                        Value one, DictionaryAttr id, DictionaryAttr domain) {
  OperationState state(location, ac::ValueBindingOp::getOperationName());
  state.addOperands({value, one, one});
  state.addAttribute("id", id);
  state.addAttribute("domain", domain);
  return cast<ac::ValueBindingOp>(builder.create(state));
}

Value extendValue(OpBuilder &builder, Location location, Value value,
                  unsigned width, bool isUnsigned) {
  auto source = cast<IntegerType>(value.getType());
  auto target = builder.getIntegerType(width);
  if (source.getWidth() == width)
    return value;
  return isUnsigned
             ? Value(arith::ExtUIOp::create(builder, location, target, value))
             : Value(arith::ExtSIOp::create(builder, location, target, value));
}

arith::CmpIPredicate predicate(StringRef opcode, bool isSigned) {
  if (opcode == "eq")
    return arith::CmpIPredicate::eq;
  if (opcode == "ne")
    return arith::CmpIPredicate::ne;
  return llvm::StringSwitch<arith::CmpIPredicate>(opcode)
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

LogicalResult lowerExactInputScalarRule(ModuleOp unit, ac::RuleOp rule) {
  auto source = inspectSource(rule);
  if (failed(source))
    return failure();
  Builder attributes(unit.getContext());
  auto constantUpper = source->constantValue;
  ++constantUpper;
  auto constantDomain = integerDomain(attributes, source->constantValue,
                                      constantUpper, source->constant);
  if (failed(constantDomain))
    return failure();
  auto inputStorage = source->inputDomain.getAs<TypeAttr>("storage");
  auto inputInterpretation =
      source->inputDomain.getAs<StringAttr>("interpretation");
  auto inputLower = source->inputDomain.getAs<ac::MathIntAttr>("lower");
  auto inputUpper = source->inputDomain.getAs<ac::MathIntAttr>("upper");
  auto inputType = inputStorage ? dyn_cast<IntegerType>(inputStorage.getValue())
                                : IntegerType();
  if (!inputType || !inputInterpretation || !inputLower || !inputUpper)
    return rule.emitOpError() << "scalar input domain is incomplete";
  bool inputUnsigned = inputInterpretation.getValue() == "unsigned";
  APSInt lower = mathValue(inputLower);
  APSInt upper = mathValue(inputUpper);
  DictionaryAttr resultDomain;
  unsigned operationWidth = 0;
  const bool isSub = source->opcode == "sub";
  if (isSub) {
    auto planned = integerDomain(
        attributes, subtract(lower, source->constantValue),
        subtract(upper, source->constantValue), source->operation);
    if (failed(planned) || planned->storage.getWidth() < inputType.getWidth() ||
        planned->storage.getWidth() < constantDomain->storage.getWidth())
      return rule.emitOpError()
             << "exact subtraction requires nonnarrowing i64 storage";
    resultDomain = planned->attr;
    operationWidth = planned->storage.getWidth();
  } else {
    resultDomain = boolDomain(attributes);
    bool workSigned = !inputUnsigned || !constantDomain->isUnsigned;
    operationWidth =
        std::max(inputType.getWidth() + (workSigned && inputUnsigned),
                 constantDomain->storage.getWidth() +
                     (workSigned && constantDomain->isUnsigned));
    if (operationWidth > 64)
      return rule.emitOpError()
             << "exact comparison requires unsupported common width above i64";
  }

  Block &body = rule.getBody().front();
  OpBuilder builder(body.getTerminator());
  APInt constantBits = source->constantValue.isUnsigned()
                           ? source->constantValue.zextOrTrunc(
                                 constantDomain->storage.getWidth())
                           : source->constantValue.sextOrTrunc(
                                 constantDomain->storage.getWidth());
  auto constant = arith::ConstantOp::create(
      builder, source->constant.getLoc(),
      IntegerAttr::get(constantDomain->storage, constantBits));
  Value lhs =
      extendValue(builder, source->operation->getLoc(),
                  source->read.getResult(), operationWidth, inputUnsigned);
  Value rhs =
      extendValue(builder, source->constant.getLoc(), constant.getResult(),
                  operationWidth, constantDomain->isUnsigned);
  Value result;
  if (isSub)
    result =
        arith::SubIOp::create(builder, source->operation->getLoc(), lhs, rhs);
  else
    result = arith::CmpIOp::create(
        builder, source->operation->getLoc(),
        predicate(source->opcode,
                  !inputUnsigned || !constantDomain->isUnsigned),
        lhs, rhs);
  auto inputBinding =
      bind(builder, source->read.getLoc(), source->read.getResult(),
           source->path, source->inputID, source->inputDomain);
  (void)bind(builder, source->fromBits.getLoc(), source->read.getResult(),
             source->path, source->fromID, source->inputDomain);
  (void)bind(builder, source->constant.getLoc(), constant.getResult(),
             source->path, source->constantID, constantDomain->attr);
  auto resultBinding = bind(builder, source->operation->getLoc(), result,
                            source->path, source->resultID, resultDomain);
  OperationState proofState(source->operation->getLoc(),
                            ac::NumericProofOp::getOperationName());
  proofState.addOperands({source->path, inputBinding.getValue(), source->path,
                          result, source->path});
  proofState.addAttribute("operand_segment_sizes",
                          builder.getDenseI32ArrayAttr({1, 1, 1, 1, 1, 0, 0}));
  proofState.addAttribute("mode", builder.getStringAttr("exact"));
  proofState.addAttribute("result_domain", resultDomain);
  proofState.addAttribute("input_ids", builder.getArrayAttr({source->inputID}));
  proofState.addAttribute("input_domains",
                          builder.getArrayAttr({source->inputDomain}));
  proofState.addAttribute("result_id", source->resultID);
  proofState.addAttribute(
      "obligations", rule->getAttrOfType<ArrayAttr>("ac.required_numeric"));
  proofState.addAttribute("checks", builder.getArrayAttr({}));
  proofState.addAttribute(
      "origin", source->operation->getAttrOfType<DictionaryAttr>("ac.origin"));
  auto proof = cast<ac::NumericProofOp>(builder.create(proofState));

  DenseSet<Operation *> controls;
  for (Value control :
       {source->operation->getOperand(0), source->operation->getOperand(2),
        source->operation->getOperand(4)})
    controls.insert(control.getDefiningOp());
  source->operation->erase();
  source->constant.erase();
  source->fromBits.erase();
  Operation *retained = source->path.getDefiningOp();
  for (Operation *control : controls)
    if (control != retained && control->getResult(0).use_empty())
      control->erase();
  return isSub ? ac::verifyExactInputConstantSubWitness(rule, resultBinding,
                                                        proof)
               : ac::verifyExactInputConstantCompareWitness(rule, resultBinding,
                                                            proof);
}

} // namespace acir::compiler
