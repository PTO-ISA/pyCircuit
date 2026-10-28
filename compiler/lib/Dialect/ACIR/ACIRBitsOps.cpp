#include "pycircuit/Dialect/ACIR/ACIROps.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "ACIRSourceContracts.h"
#include "HardwareSourceCheckPreflight.h"

#include "llvm/ADT/PointerUnion.h"
#include "llvm/ADT/STLExtras.h"

#include <limits>
#include <optional>
#include <thread>

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

// Invocation results can have canonical widths while their callers retain a
// closed authored expression. Prepay the graph before using the shared evaluator.
LogicalResult prepayClosedWidth(
    llvm::PointerUnion<Attribute, Type> node, Operation *site,
    detail::SourceCheckBudget &budget, unsigned depth = 0,
    uint64_t repetitions = 1) {
  using Budget = detail::SourceCheckBudget;
  if (!node) return success();
  if (depth > 128)
    return emitError(UnknownLoc::get(site->getContext()))
           << "closed width comparison depth exceeded";
  uint64_t visits = Budget::multiply(repetitions, 4 * uint64_t(depth + 1));
  auto pay = [&](uint64_t units) {
    return budget.debit(Budget::multiply(visits, units), site);
  };
  if (failed(pay(1))) return failure();
  bool valid = true;
  auto attribute = [&](Attribute value) {
    if (valid && value && failed(prepayClosedWidth(
                             value, site, budget, depth + 1, repetitions)))
      valid = false;
  };
  auto type = [&](Type value) {
    if (valid && value && failed(prepayClosedWidth(
                             value, site, budget, depth + 1, repetitions)))
      valid = false;
  };
  if (auto value = dyn_cast<Attribute>(node)) {
    uint64_t bytes = 0, slots = 0, slotBytes = 0;
    if (auto string = dyn_cast<StringAttr>(value)) bytes = string.getValue().size();
    if (auto integer = dyn_cast<MathIntAttr>(value))
      bytes = integer.getCanonicalValue().size();
    if (auto integer = dyn_cast<IntegerAttr>(value))
      bytes = (uint64_t(integer.getValue().getBitWidth()) + 7) / 8;
    if (auto dictionary = dyn_cast<DictionaryAttr>(value)) {
      slots = dictionary.size();
      slotBytes = sizeof(NamedAttribute);
    }
    if (auto array = dyn_cast<ArrayAttr>(value)) {
      slots = array.size();
      slotBytes = sizeof(Attribute);
    }
    if (failed(pay(Budget::add(bytes, Budget::add(
                     Budget::multiply(slots, slots + 1),
                     Budget::bytes(Budget::multiply(slots + 1, slotBytes)))))))
      return failure();
    if (auto expression = dyn_cast<StaticExprAttr>(value)) {
      auto kind = expression.getTree().getAs<StringAttr>("kind");
      // type_width re-enters packed layout/type resolution. Pay its repeated
      // substitution/probe/evaluation at every nesting, without resetting cost.
      if (kind && kind.getValue() == "type_width")
        repetitions = Budget::multiply(repetitions, 8);
    }
    value.walkImmediateSubElements(attribute, type);
  } else {
    cast<Type>(node).walkImmediateSubElements(attribute, type);
  }
  return success(valid);
}

FailureOr<bool> equivalentWidths(Operation *site, BitsType lhs, BitsType rhs) {
  if (areEquivalentHardwareTypes(lhs, rhs)) return true;
  detail::SourceCheckBudget budget(HardwareSourceCheckLimits{});
  budget.setPhase("closed Bits width comparison");
  detail::SourceCheckBudget::RawDiagnosticScope rawDiagnostics(budget);
  if (failed(prepayClosedWidth(Type(lhs), site, budget)) ||
      failed(prepayClosedWidth(Type(rhs), site, budget)))
    return failure();
  // No package scan, defaults or instance facts belong in this fallback.
  // Unresolved/nominal-dependent widths retain the symbolic comparison above.
  HardwareAnalysis hardware(mlir::ModuleOp{});
  const auto checkingThread = std::this_thread::get_id();
  ScopedDiagnosticHandler quiet(site->getContext(), [&](Diagnostic &) {
    return success(std::this_thread::get_id() == checkingThread);
  });
  auto left = hardware.evaluateStatic(lhs.getWidth(), {}, site);
  if (failed(left)) return false;
  auto right = hardware.evaluateStatic(rhs.getWidth(), {}, site);
  if (failed(right)) return false;
  auto leftInteger = dyn_cast<MathIntAttr>(*left);
  auto rightInteger = dyn_cast<MathIntAttr>(*right);
  if (!leftInteger || !rightInteger) return false;
  llvm::APSInt leftWidth(leftInteger.getCanonicalValue());
  llvm::APSInt rightWidth(rightInteger.getCanonicalValue());
  return isPositive(leftWidth) && isPositive(rightWidth) &&
         leftWidth.getActiveBits() <= 64 && rightWidth.getActiveBits() <= 64 &&
         leftWidth.getZExtValue() == rightWidth.getZExtValue();
}

LogicalResult requireSameWidth(Operation *operation, BitsType lhs, BitsType rhs,
                               BitsType result) {
  auto operands = equivalentWidths(operation, lhs, rhs);
  if (failed(operands)) return failure();
  auto output = equivalentWidths(operation, lhs, result);
  if (failed(output)) return failure();
  if (!*operands || !*output)
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
  auto sameWidth = equivalentWidths(*this, getInput().getType(), getResult().getType());
  if (failed(sameWidth)) return failure();
  if (!*sameWidth)
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
  if (!isClosed(getPredicate(), {"eq", "ne", "case_eq", "ult", "ule", "ugt",
                                 "uge", "slt", "sle", "sgt", "sge"}))
    return emitOpError() << "unknown closed bit-vector comparison predicate";
  auto sameWidth = equivalentWidths(*this, getLhs().getType(), getRhs().getType());
  if (failed(sameWidth)) return failure();
  if (!*sameWidth)
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
