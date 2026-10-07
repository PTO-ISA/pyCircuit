#include "pycircuit/Dialect/ACIR/ACIROps.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"

#include "llvm/ADT/STLExtras.h"

#include <limits>
#include <optional>

using namespace mlir;

namespace acir::ac {
namespace {

std::optional<llvm::APSInt> literalInteger(StaticExprAttr expression) {
  if (!expression)
    return std::nullopt;
  DictionaryAttr tree = expression.getTree();
  auto kind = tree ? tree.getAs<StringAttr>("kind") : StringAttr();
  auto value = tree ? tree.getAs<DictionaryAttr>("value") : DictionaryAttr();
  auto valueKind = value ? value.getAs<StringAttr>("kind") : StringAttr();
  auto integer = value ? value.getAs<MathIntAttr>("value") : MathIntAttr();
  if (!kind || kind.getValue() != "literal" || !valueKind ||
      valueKind.getValue() != "integer" || !integer)
    return std::nullopt;
  return llvm::APSInt(integer.getCanonicalValue());
}

bool isPositive(const llvm::APSInt &value) {
  return !value.isNegative() && !value.isZero();
}

LogicalResult requireSameWidth(Operation *operation, BitsType lhs, BitsType rhs,
                               BitsType result) {
  if (!areEquivalentHardwareTypes(lhs, rhs) ||
      !areEquivalentHardwareTypes(lhs, result))
    return operation->emitOpError()
           << "operands and result must have the same symbolic width";
  return success();
}

bool isClosed(StringRef value, ArrayRef<StringRef> choices) {
  return llvm::is_contained(choices, value);
}

LogicalResult requireOneBit(BitsType type, Operation *operation,
                            StringRef description) {
  auto width = literalInteger(type.getWidth());
  if (!width || *width != 1)
    return operation->emitOpError()
           << description << " requires canonical literal width one";
  return success();
}

} // namespace

LogicalResult
BitsType::verify(llvm::function_ref<InFlightDiagnostic()> emitError,
                 StaticExprAttr width) {
  if (!width || failed(StaticExprAttr::verify(emitError, width.getTree())))
    return emitError() << "bits width requires one valid StaticExpr";
  if (auto value = literalInteger(width); value && !isPositive(*value))
    return emitError() << "bits width must be positive";
  return success();
}

LogicalResult BitsConstantOp::verify() {
  if (failed(StaticExprAttr::verify([&] { return emitOpError(); },
                                    getValue().getTree())))
    return failure();
  auto value = literalInteger(getValue());
  auto width = literalInteger(getResult().getType().getWidth());
  if (value && value->isNegative())
    return emitOpError() << "bits constant must be nonnegative";
  if (value && width && width->getActiveBits() <= 64 &&
      value->getActiveBits() > width->getZExtValue())
    return emitOpError() << "bits constant does not fit its result width";
  return success();
}

LogicalResult BitsUnaryOp::verify() {
  if (getOpcode() != "not")
    return emitOpError() << "opcode must be 'not'";
  if (!areEquivalentHardwareTypes(getInput().getType(), getResult().getType()))
    return emitOpError()
           << "unary operand and result must have the same symbolic width";
  return success();
}

LogicalResult BitsBinaryOp::verify() {
  if (!isClosed(getOpcode(), {"add", "sub", "mul", "udiv", "urem", "and", "or",
                              "xor", "shl", "lshr", "ashr"}))
    return emitOpError() << "unknown closed bit-vector binary opcode";
  return requireSameWidth(*this, getLhs().getType(), getRhs().getType(),
                          getResult().getType());
}

LogicalResult BitsCompareOp::verify() {
  if (!isClosed(getPredicate(), {"eq", "ne", "ult", "ule", "ugt", "uge", "slt",
                                 "sle", "sgt", "sge"}))
    return emitOpError() << "unknown closed bit-vector comparison predicate";
  if (!areEquivalentHardwareTypes(getLhs().getType(), getRhs().getType()))
    return emitOpError() << "comparison operands must have the same width";
  return requireOneBit(getResult().getType(), *this, "comparison result");
}

LogicalResult BitsSelectOp::verify() {
  if (failed(
          requireOneBit(getCondition().getType(), *this, "select condition")))
    return failure();
  return requireSameWidth(*this, getTrueValue().getType(),
                          getFalseValue().getType(), getResult().getType());
}

LogicalResult BitsConcatOp::verify() {
  if (getInputs().empty())
    return emitOpError() << "concat requires at least one input";
  uint64_t total = 0;
  bool allLiteral = true;
  for (Value input : getInputs()) {
    auto width = literalInteger(cast<BitsType>(input.getType()).getWidth());
    if (!width || width->isNegative() || width->getActiveBits() > 64 ||
        total > std::numeric_limits<uint64_t>::max() - width->getZExtValue()) {
      allLiteral = false;
      break;
    }
    total += width->getZExtValue();
  }
  auto resultWidth = literalInteger(getResult().getType().getWidth());
  if (allLiteral && resultWidth &&
      (resultWidth->isNegative() || resultWidth->getActiveBits() > 64 ||
       resultWidth->getZExtValue() != total))
    return emitOpError() << "concat result width must equal input width sum";
  return success();
}

LogicalResult BitsExtractOp::verify() {
  if (failed(StaticExprAttr::verify([&] { return emitOpError(); },
                                    getLow().getTree())))
    return failure();
  auto low = literalInteger(getLow());
  auto inputWidth = literalInteger(getInput().getType().getWidth());
  auto resultWidth = literalInteger(getResult().getType().getWidth());
  if (low && low->isNegative())
    return emitOpError() << "extract low bound must be nonnegative";
  if (low && inputWidth && resultWidth && low->getActiveBits() <= 64 &&
      inputWidth->getActiveBits() <= 64 && resultWidth->getActiveBits() <= 64 &&
      (resultWidth->isNegative() ||
       low->getZExtValue() > inputWidth->getZExtValue() ||
       resultWidth->getZExtValue() >
           inputWidth->getZExtValue() - low->getZExtValue()))
    return emitOpError() << "extract slice exceeds its input width";
  return success();
}

LogicalResult BitsResizeOp::verify() {
  if (!isClosed(getMode(), {"trunc", "zext", "sext"}))
    return emitOpError() << "resize mode must be trunc, zext, or sext";
  auto inputWidth = literalInteger(getInput().getType().getWidth());
  auto resultWidth = literalInteger(getResult().getType().getWidth());
  if (!inputWidth || !resultWidth)
    return success();
  if ((getMode() == "trunc" && *resultWidth >= *inputWidth) ||
      (getMode() != "trunc" && *resultWidth <= *inputWidth))
    return emitOpError() << "trunc must narrow and extension modes must widen";
  return success();
}

} // namespace acir::ac
