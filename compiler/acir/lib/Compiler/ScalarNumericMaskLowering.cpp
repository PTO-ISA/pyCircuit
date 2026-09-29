#include "Dialect/ACIR/ACIRNumericMask.h"

#include "Dialect/ACIR/ACIRSourceContracts.h"
#include "acir/Dialect/ACIR/ACIROps.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "llvm/ADT/APSInt.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"

#include <algorithm>

using namespace mlir;

namespace acir::compiler {
namespace {

struct SourceMask {
  ac::SourceReadOp read;
  ac::MathFromBitsOp from;
  SmallVector<ac::MathConstantOp> constants;
  SmallVector<ac::MathBinaryOp> binaries;
  SmallVector<DictionaryAttr> ids;
  DictionaryAttr inputID;
  DictionaryAttr inputDomain;
  SmallVector<APSInt> values;
  bool lowBits;
  bool subtract;
  unsigned width;
  Value one;
};

struct DomainPlan {
  DictionaryAttr attr;
  IntegerType storage;
  bool isUnsigned;
};

APSInt mathValue(ac::MathIntAttr attr) {
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

DomainPlan unsignedDomain(Builder &builder, const APSInt &lower,
                          const APSInt &upper) {
  APSInt maximum = upper;
  --maximum;
  unsigned width = std::max(1u, maximum.getActiveBits());
  auto storage = builder.getIntegerType(width);
  auto attr = builder.getDictionaryAttr({
      builder.getNamedAttr("kind", builder.getStringAttr("integer")),
      builder.getNamedAttr("storage", TypeAttr::get(storage)),
      builder.getNamedAttr("lower",
                           ac::MathIntAttr::get(builder.getContext(), lower)),
      builder.getNamedAttr("upper",
                           ac::MathIntAttr::get(builder.getContext(), upper)),
      builder.getNamedAttr("interpretation", builder.getStringAttr("unsigned")),
  });
  return {attr, storage, true};
}

bool trueI1(Value value) {
  auto op = value.getDefiningOp<arith::ConstantOp>();
  auto attr = op ? dyn_cast<IntegerAttr>(op.getValue()) : IntegerAttr();
  return attr && attr.getType().isInteger(1) && attr.getValue().isOne();
}

FailureOr<DictionaryAttr> recipeNode(Attribute raw, StringRef opcode,
                                     unsigned operands, Operation *owner) {
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
      failed(
          ac::detail::verifyValueID(id, [&] { return owner->emitOpError(); })))
    return owner->emitOpError() << "mask source recipe node is invalid";
  return node;
}

bool ref(Attribute raw, StringRef kind, unsigned index, Operation *owner) {
  auto dictionary = dyn_cast<DictionaryAttr>(raw);
  auto actualKind =
      dictionary ? dictionary.getAs<StringAttr>("kind") : StringAttr();
  auto actualIndex =
      dictionary ? dictionary.getAs<IntegerAttr>("index") : IntegerAttr();
  if (!dictionary || dictionary.size() != 2 || !actualKind ||
      actualKind.getValue() != kind || !actualIndex)
    return false;
  auto decoded = ac::detail::decodeU32(actualIndex, "mask source ref",
                                       [&] { return owner->emitOpError(); });
  return succeeded(decoded) && *decoded == index;
}

FailureOr<APSInt> constantValue(DictionaryAttr node, Operation *owner) {
  auto operands = node.getAs<ArrayAttr>("operands");
  auto reference = operands && operands.size() == 1
                       ? dyn_cast<DictionaryAttr>(operands[0])
                       : DictionaryAttr();
  auto value =
      reference ? reference.getAs<ac::MathIntAttr>("value") : ac::MathIntAttr();
  if (!reference || reference.size() != 2 ||
      reference.getAs<StringAttr>("kind") !=
          StringAttr::get(owner->getContext(), "constant") ||
      !value)
    return owner->emitOpError() << "mask source constant ref is invalid";
  return mathValue(value);
}

FailureOr<SourceMask> inspect(ac::RuleOp rule) {
  auto emit = [&] { return rule.emitOpError(); };
  if (rule.getBody().getBlocks().size() != 1 || rule.getInputs().size() != 1 ||
      !rule.getTargets().empty())
    return emit() << "mask lowering requires one input-only rule block";
  Block &body = rule.getBody().front();
  SmallVector<ac::SourceReadOp> reads(body.getOps<ac::SourceReadOp>());
  SmallVector<ac::MathFromBitsOp> from(body.getOps<ac::MathFromBitsOp>());
  SmallVector<ac::MathConstantOp> constants(body.getOps<ac::MathConstantOp>());
  SmallVector<ac::MathBinaryOp> binaries(body.getOps<ac::MathBinaryOp>());
  auto required = rule->getAttrOfType<ArrayAttr>("ac.required_numeric");
  bool exact = constants.size() == 1 && binaries.size() == 1 && required &&
               required.size() == 3;
  bool low = constants.size() == 2 && binaries.size() == 2 && required &&
             required.size() == 5;
  if (reads.size() != 1 || from.size() != 1 || (!exact && !low) ||
      !body.getOps<ac::ValueBindingOp>().empty() ||
      !body.getOps<ac::NumericProofOp>().empty())
    return emit() << "mask source inventory is not D2a or D2b";
  auto fromNode = recipeNode(required[0], "from_bits", 1, rule);
  auto constant0 = recipeNode(required[1], "constant", 1, rule);
  if (failed(fromNode) || failed(constant0))
    return failure();
  SmallVector<DictionaryAttr> nodes{*fromNode, *constant0};
  SmallVector<APSInt> values;
  auto firstValue = constantValue(*constant0, rule);
  if (failed(firstValue) || firstValue->isNegative())
    return emit() << "mask source constants must be nonnegative";
  values.push_back(*firstValue);
  bool subtract = false;
  unsigned width = 0;
  if (exact) {
    auto andNode = recipeNode(required[2], "and_bits", 2, rule);
    auto actualOpcode = binaries[0]->getAttrOfType<StringAttr>("operator");
    if (failed(andNode) || !actualOpcode ||
        actualOpcode.getValue() != "and_bits" ||
        !ref((*andNode).getAs<ArrayAttr>("operands")[0], "node", 0, rule) ||
        !ref((*andNode).getAs<ArrayAttr>("operands")[1], "node", 1, rule))
      return emit() << "D2a requires ordered and_bits(F,M)";
    nodes.push_back(*andNode);
  } else {
    auto arithmeticAttr = dyn_cast<DictionaryAttr>(required[2]);
    auto opcode = arithmeticAttr ? arithmeticAttr.getAs<StringAttr>("operator")
                                 : StringAttr();
    if (!opcode || (opcode.getValue() != "add" && opcode.getValue() != "sub"))
      return emit() << "D2b arithmetic must be add or sub";
    subtract = opcode.getValue() == "sub";
    auto arithmetic = recipeNode(required[2], opcode.getValue(), 2, rule);
    auto maskNode = recipeNode(required[3], "constant", 1, rule);
    auto andNode = recipeNode(required[4], "and_bits", 2, rule);
    if (failed(arithmetic) || failed(maskNode) || failed(andNode))
      return failure();
    auto maskValue = constantValue(*maskNode, rule);
    if (failed(maskValue) || maskValue->isNegative())
      return emit() << "D2b mask must be nonnegative";
    width = maskValue->getActiveBits();
    APInt full(width + 1, 0);
    full.setLowBits(width);
    APSInt expected(std::move(full), true);
    if (width == 0 || width > 64 || compare(*maskValue, expected) != 0 ||
        !ref((*arithmetic).getAs<ArrayAttr>("operands")[0], "node", 0, rule) ||
        !ref((*arithmetic).getAs<ArrayAttr>("operands")[1], "node", 1, rule) ||
        !ref((*andNode).getAs<ArrayAttr>("operands")[0], "node", 2, rule) ||
        !ref((*andNode).getAs<ArrayAttr>("operands")[1], "node", 3, rule))
      return emit() << "D2b mask/width or recipe references are invalid";
    nodes.append({*arithmetic, *maskNode, *andNode});
    values.push_back(*maskValue);
  }
  for (auto [index, constant] : llvm::enumerate(constants)) {
    auto operands =
        nodes[low ? (index ? 3 : 1) : 1].getAs<ArrayAttr>("operands");
    auto reference =
        operands ? dyn_cast<DictionaryAttr>(operands[0]) : DictionaryAttr();
    if (!reference ||
        constant.getValueAttr() != reference.getAs<ac::MathIntAttr>("value"))
      return emit() << "mask MathConstant disagrees with its recipe";
  }
  auto final = binaries.back();
  if (from[0].getValue() != reads[0].getResult() ||
      binaries.front().getLhs() != from[0].getResult() ||
      binaries.front().getRhs() != constants[0].getResult() ||
      !trueI1(final.getPath()) || !trueI1(final.getLhsValid()) ||
      !trueI1(final.getRhsValid()) || !final.getResult().use_empty() ||
      !final.getValid().use_empty())
    return emit() << "mask source SSA/control graph is not closed";
  for (auto binary : binaries)
    if (!trueI1(binary.getPath()) || !trueI1(binary.getLhsValid()) ||
        !trueI1(binary.getRhsValid()))
      return emit() << "mask source requires literal-true controls";
  if (!llvm::hasSingleElement(reads[0].getResult().getUses()) ||
      !llvm::hasSingleElement(from[0].getResult().getUses()) ||
      llvm::any_of(constants, [](ac::MathConstantOp constant) {
        return !llvm::hasSingleElement(constant.getResult().getUses());
      }))
    return emit() << "mask source leaves have extra SSA uses";
  if (low && (binaries.back().getLhs() != binaries.front().getResult() ||
              binaries.back().getRhs() != constants.back().getResult() ||
              !llvm::hasSingleElement(binaries.front().getResult().getUses())))
    return emit() << "D2b intermediate must feed only final and_bits";
  auto inputTypes = rule->getAttrOfType<ArrayAttr>("ac.input_types");
  auto inputBindings = rule->getAttrOfType<ArrayAttr>("ac.input_bindings");
  auto outputTypes = rule->getAttrOfType<ArrayAttr>("ac.output_types");
  auto outputBindings = rule->getAttrOfType<ArrayAttr>("ac.output_bindings");
  auto checks = rule->getAttrOfType<ArrayAttr>("ac.required_checks");
  auto observations =
      rule->getAttrOfType<ArrayAttr>("ac.required_observations");
  if (!inputTypes || inputTypes.size() != 1 || !inputBindings ||
      inputBindings.size() != 1 || !outputTypes || !outputTypes.empty() ||
      !outputBindings || !outputBindings.empty() ||
      (checks && !checks.empty()) || (observations && !observations.empty()) ||
      !rule->getAttrOfType<DictionaryAttr>("ac.proof_scope") ||
      failed(ac::detail::verifyStateRef(
          dyn_cast<DictionaryAttr>(inputBindings[0]), emit)))
    return emit() << "mask source rule metadata is not closed";
  auto inputDomain = dyn_cast<DictionaryAttr>(inputTypes[0]);
  auto storage =
      inputDomain ? inputDomain.getAs<TypeAttr>("storage") : TypeAttr();
  auto interpretation = inputDomain
                            ? inputDomain.getAs<StringAttr>("interpretation")
                            : StringAttr();
  if (!inputDomain || inputDomain != from[0].getDomainAttr() || !storage ||
      !interpretation || (low && interpretation.getValue() != "unsigned") ||
      reads[0].getResult().getType() != storage.getValue())
    return emit() << "mask source input domain is invalid";
  Builder builder(rule.getContext());
  auto inputID = builder.getDictionaryAttr({
      builder.getNamedAttr(
          "origin", reads[0]->getAttrOfType<DictionaryAttr>("ac.origin")),
      builder.getNamedAttr("slot", builder.getI32IntegerAttr(0)),
  });
  SmallVector<DictionaryAttr> ids;
  for (DictionaryAttr node : nodes)
    ids.push_back(node.getAs<DictionaryAttr>("id"));
  if (llvm::any_of(ids, [&](DictionaryAttr id) { return id == inputID; }))
    return emit() << "mask source ValueIDs collide with input identity";
  DenseSet<Attribute> unique;
  for (DictionaryAttr id : ids)
    if (!unique.insert(id).second)
      return emit() << "mask source ValueIDs must be distinct";
  SmallVector<Operation *> producers{from[0].getOperation(),
                                     constants[0].getOperation()};
  if (low)
    producers.append({binaries[0].getOperation(), constants[1].getOperation(),
                      binaries[1].getOperation()});
  else
    producers.push_back(binaries[0].getOperation());
  for (auto [id, producer] : llvm::zip_equal(ids, producers))
    if (id.getAs<DictionaryAttr>("origin") !=
        producer->getAttrOfType<DictionaryAttr>("ac.origin"))
      return emit() << "mask ValueID origin does not match its producer";
  for (Operation &operation : body.getOperations()) {
    if (isa<ac::SourceReadOp, ac::MathFromBitsOp, ac::MathConstantOp,
            ac::MathBinaryOp, ac::YieldOp>(operation))
      continue;
    if (auto control = dyn_cast<arith::ConstantOp>(operation)) {
      if (!trueI1(control.getResult()))
        return emit() << "mask source has a non-true control constant";
      continue;
    }
    return emit() << "mask source contains an unsupported operation";
  }
  return SourceMask{reads[0], from[0],  constants,   binaries,
                    ids,      inputID,  inputDomain, values,
                    low,      subtract, width,       final.getPath()};
}

Value fit(OpBuilder &builder, Location location, Value value, unsigned width,
          bool isUnsigned) {
  unsigned source = cast<IntegerType>(value.getType()).getWidth();
  auto target = builder.getIntegerType(width);
  if (source == width)
    return value;
  if (source > width)
    return arith::TruncIOp::create(builder, location, target, value);
  return isUnsigned
             ? Value(arith::ExtUIOp::create(builder, location, target, value))
             : Value(arith::ExtSIOp::create(builder, location, target, value));
}

ac::ValueBindingOp bind(OpBuilder &builder, Location location, Value value,
                        Value one, DictionaryAttr id, DictionaryAttr domain) {
  OperationState state(location, ac::ValueBindingOp::getOperationName());
  state.addOperands({value, one, one});
  state.addAttribute("id", id);
  state.addAttribute("domain", domain);
  return cast<ac::ValueBindingOp>(builder.create(state));
}

} // namespace

LogicalResult lowerInputMaskRule(ModuleOp unit, ac::RuleOp rule) {
  auto source = inspect(rule);
  if (failed(source) || !unit ||
      rule->getParentOfType<mlir::ModuleOp>() != unit)
    return failure();
  Builder attrs(unit.getContext());
  auto storageAttr = source->inputDomain.getAs<TypeAttr>("storage");
  auto interpretationAttr =
      source->inputDomain.getAs<StringAttr>("interpretation");
  auto inputStorage = storageAttr
                          ? dyn_cast<IntegerType>(storageAttr.getValue())
                          : IntegerType();
  if (!interpretationAttr)
    return rule.emitOpError() << "mask input domain lacks interpretation";
  auto interpretation = interpretationAttr.getValue();
  bool inputUnsigned = interpretation == "unsigned";
  APSInt zero(APInt(66, 0), true);
  APSInt mask = source->lowBits ? source->values[1] : source->values[0];
  if (!inputStorage || inputStorage.getWidth() == 0 ||
      inputStorage.getWidth() > 64 || mask.getActiveBits() > 64)
    return rule.emitOpError()
           << "mask lowering requires finite i1..i64 storage";
  APSInt maskUpper = mask;
  ++maskUpper;
  auto maskDomain = unsignedDomain(attrs, mask, maskUpper);
  DomainPlan resultDomain;
  DomainPlan constantDomain;
  unsigned operationWidth;
  APSInt finiteConstant = source->values[0];
  if (source->lowBits) {
    APInt modulus(source->width + 1, 0);
    modulus.setBit(source->width);
    APSInt upper(std::move(modulus), true);
    resultDomain = unsignedDomain(attrs, zero, upper);
    APInt reduced = extend(finiteConstant, source->width).trunc(source->width);
    finiteConstant = APSInt(reduced, true);
    APSInt constantUpper =
        finiteConstant.extend(finiteConstant.getBitWidth() + 1);
    ++constantUpper;
    constantDomain = unsignedDomain(attrs, finiteConstant, constantUpper);
    operationWidth = source->width;
  } else {
    resultDomain = unsignedDomain(attrs, zero, maskUpper);
    constantDomain = maskDomain;
    operationWidth =
        std::max({inputStorage.getWidth(), maskDomain.storage.getWidth(),
                  resultDomain.storage.getWidth()});
  }
  Block &body = rule.getBody().front();
  OpBuilder builder(body.getTerminator());
  auto integerConstant = [&](Location location, const APSInt &value,
                             IntegerType type) {
    return arith::ConstantOp::create(
        builder, location,
        IntegerAttr::get(type, extend(value, type.getWidth())));
  };
  auto k = integerConstant(source->constants[0].getLoc(), finiteConstant,
                           constantDomain.storage);
  Value lhs = fit(builder, source->binaries[0].getLoc(),
                  source->read.getResult(), operationWidth, inputUnsigned);
  Value arithmetic = lhs;
  if (source->lowBits) {
    Value rhs =
        fit(builder, source->constants[0].getLoc(), k, operationWidth, true);
    arithmetic = source->subtract
                     ? Value(arith::SubIOp::create(
                           builder, source->binaries[0].getLoc(), lhs, rhs))
                     : Value(arith::AddIOp::create(
                           builder, source->binaries[0].getLoc(), lhs, rhs));
  }
  auto maskConstant = source->lowBits
                          ? integerConstant(source->constants.back().getLoc(),
                                            mask, maskDomain.storage)
                          : k;
  Value maskOperand = fit(builder, source->constants.back().getLoc(),
                          maskConstant, operationWidth, true);
  Value masked = arith::AndIOp::create(
      builder, source->binaries.back().getLoc(), arithmetic, maskOperand);
  Value result = fit(builder, source->binaries.back().getLoc(), masked,
                     resultDomain.storage.getWidth(), true);
  auto inputBinding =
      bind(builder, source->read.getLoc(), source->read.getResult(),
           source->one, source->inputID, source->inputDomain);
  (void)bind(builder, source->from.getLoc(), source->read.getResult(),
             source->one, source->ids[0], source->inputDomain);
  (void)bind(builder, source->constants[0].getLoc(), k, source->one,
             source->ids[1], constantDomain.attr);
  if (source->lowBits)
    (void)bind(builder, source->constants[1].getLoc(), maskConstant,
               source->one, source->ids[3], maskDomain.attr);
  auto resultBinding = bind(builder, source->binaries.back().getLoc(), result,
                            source->one, source->ids.back(), resultDomain.attr);
  OperationState state(source->binaries.back().getLoc(),
                       ac::NumericProofOp::getOperationName());
  state.addOperands(
      {source->one, inputBinding.getValue(), source->one, result, source->one});
  state.addAttribute("operand_segment_sizes",
                     builder.getDenseI32ArrayAttr({1, 1, 1, 1, 1, 0, 0}));
  state.addAttribute(
      "mode", builder.getStringAttr(source->lowBits ? "low_bits" : "exact"));
  if (source->lowBits)
    state.addAttribute("width", builder.getI32IntegerAttr(source->width));
  state.addAttribute("result_domain", resultDomain.attr);
  state.addAttribute("input_ids", builder.getArrayAttr({source->inputID}));
  state.addAttribute("input_domains",
                     builder.getArrayAttr({source->inputDomain}));
  state.addAttribute("result_id", source->ids.back());
  state.addAttribute("obligations",
                     rule->getAttrOfType<ArrayAttr>("ac.required_numeric"));
  state.addAttribute("checks", builder.getArrayAttr({}));
  state.addAttribute(
      "origin",
      source->binaries.back()->getAttrOfType<DictionaryAttr>("ac.origin"));
  auto proof = cast<ac::NumericProofOp>(builder.create(state));
  DenseSet<Operation *> controls;
  for (auto binary : source->binaries)
    for (Value control :
         {binary.getPath(), binary.getLhsValid(), binary.getRhsValid()})
      controls.insert(control.getDefiningOp());
  for (auto binary : llvm::reverse(source->binaries))
    binary.erase();
  for (auto constant : source->constants)
    constant.erase();
  source->from.erase();
  for (Operation *control : controls)
    if (control != source->one.getDefiningOp() &&
        control->getResult(0).use_empty())
      control->erase();
  return source->lowBits
             ? ac::verifyLowBitsInputMaskWitness(rule, resultBinding, proof)
             : ac::verifyExactInputConstantMaskWitness(rule, resultBinding,
                                                       proof);
}

} // namespace acir::compiler
