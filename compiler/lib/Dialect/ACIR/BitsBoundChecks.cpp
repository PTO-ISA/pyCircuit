// Staged symbolic Bits width equivalence (B2 step-2 design r3 ACIRBitsOps
// row): the native verifiers keep the literal-width grammar; the symbolic
// fallback moved here verbatim and runs from the source-unit walk. The
// resolved dispatch keeps its own bound-width checks, which subsume the
// closed cases. No package scan, defaults or instance facts belong in this
// fallback, matching the pre-split semantics.
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "pycircuit/Dialect/ACIR/ACIROps.h"
#include "Dialect/ACIR/SourceCheckBudget.h"

#include <thread>

using namespace mlir;

namespace acir::ac {
namespace {
bool isPositive(const llvm::APSInt &value) { return !value.isNegative() && !value.isZero(); }
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
} // namespace

LogicalResult HardwareAnalysis::verifyBitsSymbolicWidths(
    Operation *operation) const {
  if (auto unary = dyn_cast<BitsUnaryOp>(operation)) {
    auto sameWidth = equivalentWidths(unary, unary.getInput().getType(),
                                      unary.getResult().getType());
    if (failed(sameWidth)) return failure();
    if (!*sameWidth)
      return unary.emitOpError()
          << "unary operand and result must have the same symbolic width";
    return success();
  }
  if (auto binary = dyn_cast<BitsBinaryOp>(operation))
    return requireSameWidth(binary, binary.getLhs().getType(),
                            binary.getRhs().getType(),
                            binary.getResult().getType());
  if (auto compare = dyn_cast<BitsCompareOp>(operation)) {
    auto sameWidth = equivalentWidths(compare, compare.getLhs().getType(),
                                      compare.getRhs().getType());
    if (failed(sameWidth)) return failure();
    if (!*sameWidth)
      return compare.emitOpError()
          << "comparison operands must have the same width";
    return success();
  }
  if (auto select = dyn_cast<BitsSelectOp>(operation))
    return requireSameWidth(select, select.getTrueValue().getType(),
                            select.getFalseValue().getType(),
                            select.getResult().getType());
  return operation->emitOpError()
         << "operation is not a symbolic-width bits check";
}
} // namespace acir::ac
