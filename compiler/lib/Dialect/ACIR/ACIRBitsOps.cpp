// Bits operation verifiers (B2 step-2 design r3 ACIRBitsOps row, r9 §5):
// local bodies keep the opcode/predicate grammar and the literal-width
// checks; the symbolic-width equivalence fallback moved verbatim to the
// staged BitsBoundChecks TU and runs from the source-unit walk; the
// resolved dispatch keeps its bound-width checks (subsuming the closed
// cases with actual bindings).
#include "pycircuit/Dialect/ACIR/ACIROps.h"
#include "ACIRSourceContracts.h"
#include "IR/StateDeclarationContracts.h"

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

// Verify bodies below this point come from the pre-split ACIRBitsOps.cpp
// (recreated in full, calling the local helpers above).

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

LogicalResult BitsConstant4Op::verify() {
  StringRef pattern = getPattern();
  if (pattern.empty() || !llvm::all_of(pattern, [](char bit) {
        return bit == '0' || bit == '1' || bit == 'x' || bit == 'z';
      }))
    return emitOpError()
           << "pattern requires nonempty canonical lowercase 0/1/x/z";
  auto width = literalInteger(getResult().getType().getWidth());
  if (!width || width->isNegative() || width->getActiveBits() > 64 ||
      width->getZExtValue() != pattern.size())
    return emitOpError()
           << "pattern length must equal the positive literal result width";
  return success();
}

LogicalResult BitsUnaryOp::verify() {
  if (getOpcode() != "not")
    return emitOpError() << "opcode must be 'not'";
  return success();
}

LogicalResult BitsBinaryOp::verify() {
  if (!isClosed(getOpcode(), {"add", "sub", "mul", "udiv", "urem", "and", "or",
                              "xor", "shl", "lshr", "ashr"}))
    return emitOpError() << "unknown closed bit-vector binary opcode";
  return success();
}

LogicalResult BitsCompareOp::verify() {
  if (!isClosed(getPredicate(), {"eq", "ne", "case_eq", "ult", "ule", "ugt",
                                 "uge", "slt", "sle", "sgt", "sge"}))
    return emitOpError() << "unknown closed bit-vector comparison predicate";
  return requireOneBit(getResult().getType(), *this, "comparison result");
}

LogicalResult BitsSelectOp::verify() {
  return requireOneBit(getCondition().getType(), *this, "select condition");
}

LogicalResult ValueRequireOp::verify() {
  auto error = [&] { return emitOpError(); };
  auto package = (*this)->getParentOfType<mlir::ModuleOp>();
  if (auto stage = package ? package->getAttrOfType<StringAttr>("ac.stage") : StringAttr();
      stage && stage.getValue() == "source")
    return emitOpError() << "is compiler-owned final availability evidence";
  if (!isa<RuleOp, ModuleOp>((*this)->getParentOp()))
    return emitOpError() << "requires an immediate hardware Module or Rule";
  if (failed(requireOneBit(getAvailability().getType(), *this,
                           "required availability")) ||
      failed(requireOneBit(getPath().getType(), *this, "required use path")) ||
      failed(requireOneBit(getResult().getType(), *this, "required use result")) ||
      failed(detail::verifyOccurrence(getOccurrence(), error)) ||
      failed(detail::verifySourceSpan(getSourceLocation(), error)))
    return failure();
  return success();
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
