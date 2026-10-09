#include "NumericLowering.h"

#include "mlir/IR/Diagnostics.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"

#include <algorithm>
#include <limits>
#include <thread>

using namespace mlir;
namespace acir::compiler {
namespace {
using ac::detail::ValueKind;
using ac::detail::ValueOpcode;
using llvm::APInt;
using llvm::APSInt;

auto diagnostic(const NumericLoweringSite &site) {
  return [location = site.location] { return mlir::emitError(location); };
}
LogicalResult verifySite(const NumericLoweringSite &site) {
  auto error = diagnostic(site);
  return success(
      succeeded(ac::detail::verifyOccurrence(site.origin, error)) &&
      succeeded(ac::detail::verifySourceSpan(site.sourceSpan, error)));
}
FailureOr<APSInt> calculate(ValueOpcode opcode, const APSInt &lhs,
                            const APSInt &rhs,
                            const NumericLoweringSite &site) {
  auto *ctx = site.location.getContext();
  SmallVector<Attribute> operands{ac::MathIntAttr::get(ctx, lhs),
                                  ac::MathIntAttr::get(ctx, rhs)};
  auto value = ac::detail::evaluateValue(opcode, operands, diagnostic(site));
  if (failed(value))
    return failure();
  auto integer = dyn_cast<ac::MathIntAttr>(*value);
  if (!integer)
    return diagnostic(site)() << "integer operation did not produce an integer";
  return APSInt(integer.getCanonicalValue());
}
FailureOr<APSInt> knownInteger(ac::StaticExprAttr expression, Operation *site,
                               const NumericLoweringSite &where) {
  auto tree = expression.getTree();
  auto kind = tree.getAs<StringAttr>("kind");
  if (kind && kind.getValue() == "literal") {
    auto value = tree.getAs<DictionaryAttr>("value");
    auto integer =
        value ? value.getAs<ac::MathIntAttr>("value") : ac::MathIntAttr();
    if (integer)
      return APSInt(integer.getCanonicalValue());
  }
  auto package =
      site ? site->getParentOfType<mlir::ModuleOp>() : mlir::ModuleOp();
  if (package) {
    ac::HardwareAnalysis analysis(package);
    if (analysis.isStaticEvaluable(expression, {})) {
      auto value = analysis.evaluateStatic(expression, {}, site);
      if (failed(value))
        return failure();
      if (auto integer = dyn_cast<ac::MathIntAttr>(*value))
        return APSInt(integer.getCanonicalValue());
    }
  }
  return diagnostic(where)()
         << "integer lowering requires a proven static integer";
}
FailureOr<unsigned> widthOf(ac::BitsType type, Operation *operation,
                            const NumericLoweringSite &site) {
  if (!type)
    return diagnostic(site)() << "integer lowering requires a bits value";
  auto width = knownInteger(type.getWidth(), operation, site);
  if (failed(width))
    return failure();
  if (width->isNegative() || width->isZero() ||
      width->getActiveBits() > std::numeric_limits<unsigned>::digits)
    return diagnostic(site)()
           << "integer width exceeds compiler representation capacity";
  return static_cast<unsigned>(width->getZExtValue());
}
uint64_t signedWidth(const APSInt &value) {
  return value.isNegative() ? value.getSignificantBits()
                            : uint64_t(value.getActiveBits()) + 1;
}
Operation *valueSite(Value value) {
  if (auto argument = dyn_cast<BlockArgument>(value))
    return argument.getOwner()->getParentOp();
  return value.getDefiningOp();
}
FailureOr<unsigned> intervalWidth(const IntegerInterval &interval,
                                  const NumericLoweringSite &site) {
  if (APSInt::compareValues(interval.lower, interval.upper) >= 0)
    return diagnostic(site)() << "integer interval must be nonempty";
  auto maximum =
      calculate(ValueOpcode::Sub, interval.upper, APSInt::getUnsigned(1), site);
  if (failed(maximum))
    return failure();
  uint64_t width =
      interval.lower.isNegative()
          ? std::max(signedWidth(interval.lower), signedWidth(*maximum))
          : std::max(1u, maximum->getActiveBits());
  if (width > std::numeric_limits<unsigned>::max())
    return diagnostic(site)()
           << "integer interval exceeds compiler representation capacity";
  return static_cast<unsigned>(width);
}
FailureOr<unsigned> checkValue(const NumericValue &value,
                               const NumericLoweringSite &site) {
  if (!value.value || value.sourceKind != ValueKind::Integer || !value.interval)
    return diagnostic(site)() << "mathematical arithmetic requires known "
                                 "Integer kind and finite interval";
  auto width = widthOf(dyn_cast<ac::BitsType>(value.value.getType()),
                       valueSite(value.value), site);
  auto required = intervalWidth(*value.interval, site);
  if (failed(width) || failed(required))
    return failure();
  if (*required > *width)
    return diagnostic(site)()
           << "integer interval does not fit its SSA representation";
  return *width;
}
ac::StaticExprAttr integerExpression(OpBuilder &builder, const APSInt &integer,
                                     const NumericLoweringSite &site) {
  auto value = builder.getDictionaryAttr(
      {builder.getNamedAttr("kind", builder.getStringAttr("integer")),
       builder.getNamedAttr(
           "value", ac::MathIntAttr::get(builder.getContext(), integer))});
  auto tree = builder.getDictionaryAttr(
      {builder.getNamedAttr("kind", builder.getStringAttr("literal")),
       builder.getNamedAttr("value", value),
       builder.getNamedAttr("origin", site.origin),
       builder.getNamedAttr("location", site.sourceSpan)});
  return ac::StaticExprAttr::get(builder.getContext(), tree);
}
ac::BitsType bitsType(OpBuilder &builder, unsigned width,
                      const NumericLoweringSite &site) {
  return ac::BitsType::get(
      builder.getContext(),
      integerExpression(builder, APSInt::getUnsigned(width), site));
}
Value operation(OpBuilder &builder, Location location, StringRef name,
                ValueRange operands, Type type, StringRef attribute,
                StringRef spelling) {
  OperationState state(location, name);
  state.addOperands(operands);
  state.addTypes(type);
  state.addAttribute(attribute, builder.getStringAttr(spelling));
  return builder.create(state)->getResult(0);
}
Value widen(OpBuilder &builder, const NumericLoweringSite &site,
            const NumericValue &value, unsigned current, unsigned width) {
  if (current == width)
    return value.value;
  return operation(builder, site.location, ac::BitsResizeOp::getOperationName(),
                   value.value, bitsType(builder, width, site), "mode",
                   value.interval->lower.isNegative() ? "sext" : "zext");
}
std::optional<APSInt> literalMask(const NumericValue &value,
                                  const NumericLoweringSite &site) {
  auto constant = value.value.getDefiningOp<ac::BitsConstantOp>();
  if (!constant)
    return std::nullopt;
  auto integer = knownInteger(constant.getValue(), constant, site);
  if (failed(integer))
    return std::nullopt;
  auto width = widthOf(constant.getResult().getType(), constant, site);
  if (failed(width) || integer->isNegative() ||
      integer->getActiveBits() > *width)
    return std::nullopt;
  if (value.interval->lower.isNegative())
    *integer = APSInt(integer->zextOrTrunc(*width), false);
  if (APSInt::compareValues(*integer, value.interval->lower) < 0 ||
      APSInt::compareValues(*integer, value.interval->upper) >= 0)
    return std::nullopt;
  return *integer;
}
} // namespace

std::optional<IntegerInterval> refineUnsignedSelectInterval(
    const NumericLoweringSite &site, StringRef predicate, bool selectedWhenTrue,
    const std::optional<IntegerInterval> &original, uint64_t literalWidth,
    const APSInt &comparisonConstant, const APSInt &fallbackConstant) {
  if (!literalWidth || comparisonConstant.isNegative() ||
      fallbackConstant.isNegative() ||
      comparisonConstant.getActiveBits() > literalWidth ||
      fallbackConstant.getActiveBits() > literalWidth)
    return std::nullopt;
  if (original &&
      (original->lower.isNegative() || original->upper.isNegative() ||
       APSInt::compareValues(original->lower, original->upper) >= 0 ||
       !(original->upper.getActiveBits() <= literalWidth ||
         (original->upper.isPowerOf2() &&
          original->upper.getActiveBits() - 1 <= literalWidth))))
    return std::nullopt;
  if (predicate != "ult" && predicate != "ule" && predicate != "ugt" &&
      predicate != "uge" && predicate != "eq")
    return std::nullopt;
  if (!selectedWhenTrue) {
    if (predicate == "eq")
      return std::nullopt;
    predicate = predicate == "ult"   ? "uge"
                : predicate == "ule" ? "ugt"
                : predicate == "ugt" ? "ule"
                                     : "ult";
  }
  auto increment = [&](const APSInt &value) -> std::optional<APSInt> {
    // Match evaluateValue(Add)'s signed precision plus carry, without a
    // carrier-sized extension. Only this speculative calculation is silenced.
    uint64_t precision =
        std::max(uint64_t(value.getActiveBits()) + 1, uint64_t(2));
    if (precision >= std::numeric_limits<unsigned>::max())
      return std::nullopt;
    const auto probingThread = std::this_thread::get_id();
    ScopedDiagnosticHandler suppress(site.location.getContext(), [&](Diagnostic &) {
      return success(std::this_thread::get_id() == probingThread);
    });
    auto result = calculate(ValueOpcode::Add, value, APSInt::getUnsigned(1), site);
    return succeeded(result) ? std::optional<APSInt>(*result) : std::nullopt;
  };
  APSInt lower = original ? original->lower : APSInt::getUnsigned(0);
  std::optional<APSInt> upper =
      original ? std::optional<APSInt>(original->upper) : std::nullopt;
  if (predicate == "ule" || predicate == "ugt" || predicate == "eq") {
    auto next = increment(comparisonConstant);
    if (!next)
      return std::nullopt;
    if (predicate == "ugt") {
      if (APSInt::compareValues(lower, *next) < 0)
        lower = *next;
    } else if (!upper || APSInt::compareValues(*next, *upper) < 0)
      upper = *next;
  }
  if (predicate == "ult") {
    if (!upper || APSInt::compareValues(comparisonConstant, *upper) < 0)
      upper = comparisonConstant;
  } else if (predicate == "uge" || predicate == "eq") {
    if (APSInt::compareValues(lower, comparisonConstant) < 0)
      lower = comparisonConstant;
  }
  // A lower-only restriction cannot represent an otherwise implicit carrier.
  // An empty abstract arm never permits changing four-state branch behavior.
  if (!upper || APSInt::compareValues(lower, *upper) >= 0)
    return std::nullopt;
  auto fallbackUpper = increment(fallbackConstant);
  if (!fallbackUpper)
    return std::nullopt;
  return IntegerInterval{
      APSInt::compareValues(lower, fallbackConstant) < 0 ? lower
                                                       : fallbackConstant,
      APSInt::compareValues(*upper, *fallbackUpper) > 0 ? *upper : *fallbackUpper};
}

FailureOr<NumericValue> lowerExactIntegerBinary(OpBuilder &builder,
                                                const NumericLoweringSite &site,
                                                ValueOpcode opcode,
                                                const NumericValue &lhs,
                                                const NumericValue &rhs) {
  if (failed(verifySite(site)))
    return failure();
  if (opcode != ValueOpcode::Add && opcode != ValueOpcode::Sub &&
      opcode != ValueOpcode::Mul && opcode != ValueOpcode::AndBits)
    return diagnostic(site)() << "unsupported exact integer binary operation";
  auto leftWidth = checkValue(lhs, site), rightWidth = checkValue(rhs, site);
  if (failed(leftWidth) || failed(rightWidth))
    return failure();
  IntegerInterval result;
  StringRef spelling;
  if (opcode == ValueOpcode::AndBits) {
    auto mask = literalMask(rhs, site);
    if (!mask || mask->isNegative())
      mask = literalMask(lhs, site);
    if (!mask || mask->isNegative())
      return diagnostic(site)() << "mathematical AND requires an actual known "
                                   "nonnegative integer constant mask";
    auto upper =
        calculate(ValueOpcode::Add, *mask, APSInt::getUnsigned(1), site);
    if (failed(upper))
      return failure();
    result = {APSInt::getUnsigned(0), *upper};
    spelling = "and";
  } else if (opcode == ValueOpcode::Mul) {
    auto leftMax = calculate(ValueOpcode::Sub, lhs.interval->upper,
                             APSInt::getUnsigned(1), site);
    auto rightMax = calculate(ValueOpcode::Sub, rhs.interval->upper,
                              APSInt::getUnsigned(1), site);
    if (failed(leftMax) || failed(rightMax))
      return failure();
    SmallVector<APSInt> products;
    for (const APSInt &left : {lhs.interval->lower, *leftMax})
      for (const APSInt &right : {rhs.interval->lower, *rightMax}) {
        auto product = calculate(ValueOpcode::Mul, left, right, site);
        if (failed(product))
          return failure();
        products.push_back(*product);
      }
    auto compare = [](const APSInt &a, const APSInt &b) {
      return APSInt::compareValues(a, b) < 0;
    };
    result.lower = *std::min_element(products.begin(), products.end(), compare);
    auto maximum = *std::max_element(products.begin(), products.end(), compare);
    auto upper =
        calculate(ValueOpcode::Add, maximum, APSInt::getUnsigned(1), site);
    if (failed(upper))
      return failure();
    result.upper = *upper;
    spelling = "mul";
  } else {
    auto lower = calculate(opcode, lhs.interval->lower,
                           opcode == ValueOpcode::Add ? rhs.interval->lower
                                                      : rhs.interval->upper,
                           site);
    auto upper = calculate(opcode, lhs.interval->upper,
                           opcode == ValueOpcode::Add ? rhs.interval->upper
                                                      : rhs.interval->lower,
                           site);
    if (failed(lower) || failed(upper))
      return failure();
    auto adjusted = calculate(opcode == ValueOpcode::Add ? ValueOpcode::Sub
                                                         : ValueOpcode::Add,
                              opcode == ValueOpcode::Add ? *upper : *lower,
                              APSInt::getUnsigned(1), site);
    if (failed(adjusted))
      return failure();
    result = opcode == ValueOpcode::Add ? IntegerInterval{*lower, *adjusted}
                                        : IntegerInterval{*adjusted, *upper};
    spelling = opcode == ValueOpcode::Add ? "add" : "sub";
  }
  auto resultWidth = intervalWidth(result, site);
  if (failed(resultWidth))
    return failure();
  bool signedOperands =
      lhs.interval->lower.isNegative() || rhs.interval->lower.isNegative();
  uint64_t width = std::max<uint64_t>(
      *resultWidth,
      std::max(uint64_t(*leftWidth) +
                   (signedOperands && !lhs.interval->lower.isNegative()),
               uint64_t(*rightWidth) +
                   (signedOperands && !rhs.interval->lower.isNegative())));
  if (width > std::numeric_limits<unsigned>::max())
    return diagnostic(site)()
           << "integer operation exceeds compiler representation capacity";
  unsigned fullWidth = static_cast<unsigned>(width);
  auto fullType = bitsType(builder, fullWidth, site);
  if ((*leftWidth == fullWidth &&
       !ac::areEquivalentHardwareTypes(lhs.value.getType(), fullType)) ||
      (*rightWidth == fullWidth &&
       !ac::areEquivalentHardwareTypes(rhs.value.getType(), fullType)))
    return diagnostic(site)() << "integer representation proof requires "
                                 "normalized equal-width types";
  Value left = widen(builder, site, lhs, *leftWidth, fullWidth);
  Value right = widen(builder, site, rhs, *rightWidth, fullWidth);
  Value value = operation(
      builder, site.location, ac::BitsBinaryOp::getOperationName(),
      {left, right}, bitsType(builder, fullWidth, site), "opcode", spelling);
  return NumericValue{value, ValueKind::Integer, std::move(result)};
}

FailureOr<NumericValue> lowerExactIntegerSelect(OpBuilder &builder,
                                                const NumericLoweringSite &site,
                                                const NumericValue &condition,
                                                const NumericValue &whenTrue,
                                                const NumericValue &whenFalse) {
  if (failed(verifySite(site)))
    return failure();
  if (!condition.value || condition.sourceKind != ValueKind::Boolean)
    return diagnostic(site)()
           << "integer select requires authoritative Boolean condition kind";
  if (!ac::areEquivalentHardwareTypes(condition.value.getType(),
                                      bitsType(builder, 1, site)))
    return diagnostic(site)()
           << "integer select condition requires canonical one-bit Bits";
  auto trueWidth = checkValue(whenTrue, site);
  auto falseWidth = checkValue(whenFalse, site);
  if (failed(trueWidth) || failed(falseWidth))
    return failure();
  IntegerInterval result{APSInt::compareValues(whenTrue.interval->lower,
                                               whenFalse.interval->lower) < 0
                             ? whenTrue.interval->lower
                             : whenFalse.interval->lower,
                         APSInt::compareValues(whenTrue.interval->upper,
                                               whenFalse.interval->upper) > 0
                             ? whenTrue.interval->upper
                             : whenFalse.interval->upper};
  auto required = intervalWidth(result, site);
  if (failed(required))
    return failure();
  bool signedBranches = whenTrue.interval->lower.isNegative() ||
                        whenFalse.interval->lower.isNegative();
  uint64_t width = std::max<uint64_t>(
      *required,
      std::max(
          uint64_t(*trueWidth) +
              (signedBranches && !whenTrue.interval->lower.isNegative()),
          uint64_t(*falseWidth) +
              (signedBranches && !whenFalse.interval->lower.isNegative())));
  if (width > std::numeric_limits<unsigned>::max())
    return diagnostic(site)()
           << "integer select exceeds compiler representation capacity";
  unsigned fullWidth = static_cast<unsigned>(width);
  auto fullType = bitsType(builder, fullWidth, site);
  if ((*trueWidth == fullWidth &&
       !ac::areEquivalentHardwareTypes(whenTrue.value.getType(), fullType)) ||
      (*falseWidth == fullWidth &&
       !ac::areEquivalentHardwareTypes(whenFalse.value.getType(), fullType)))
    return diagnostic(site)() << "integer select representation proof requires "
                                 "normalized equal-width types";
  Value yes = widen(builder, site, whenTrue, *trueWidth, fullWidth);
  Value no = widen(builder, site, whenFalse, *falseWidth, fullWidth);
  OperationState state(site.location, ac::BitsSelectOp::getOperationName());
  state.addOperands({condition.value, yes, no});
  state.addTypes(fullType);
  Value value = builder.create(state)->getResult(0);
  return NumericValue{value, ValueKind::Integer, std::move(result)};
}

FailureOr<NumericValue>
lowerExactIntegerShiftRight(OpBuilder &builder, const NumericLoweringSite &site,
                            const NumericValue &input, const APSInt &count) {
  if (failed(verifySite(site)))
    return failure();
  if (count.isNegative())
    return diagnostic(site)()
           << "integer right-shift count must be nonnegative";
  auto inputWidth = checkValue(input, site);
  if (failed(inputWidth))
    return failure();
  if (input.interval->lower.isNegative())
    return diagnostic(site)() << "integer right shift requires a proven "
                                 "nonnegative input interval";

  auto lower = calculate(ValueOpcode::Shr, input.interval->lower, count, site);
  auto maximum = calculate(ValueOpcode::Sub, input.interval->upper,
                           APSInt::getUnsigned(1), site);
  if (failed(lower) || failed(maximum))
    return failure();
  auto shiftedMaximum = calculate(ValueOpcode::Shr, *maximum, count, site);
  if (failed(shiftedMaximum))
    return failure();
  auto upper = calculate(ValueOpcode::Add, *shiftedMaximum,
                         APSInt::getUnsigned(1), site);
  if (failed(upper))
    return failure();
  IntegerInterval interval{*lower, *upper};
  auto requiredWidth = intervalWidth(interval, site);
  if (failed(requiredWidth))
    return failure();

  // Compare arbitrary-precision counts before narrowing. The actual SSA width,
  // not its interval's minimum width, decides which X/Z bits survive.
  bool overshift =
      APSInt::compareValues(count, APSInt::getUnsigned(*inputWidth)) >= 0;
  unsigned amount = overshift ? 0 : static_cast<unsigned>(count.getZExtValue());
  unsigned resultWidth = overshift ? 1 : *inputWidth - amount;
  if (*requiredWidth > resultWidth)
    return diagnostic(site)()
           << "integer right-shift interval does not fit its result width";

  // All proofs precede insertion. Even a zero shift gets a fresh SSA result so
  // mathematical provenance cannot change admission of other input uses.
  Value result;
  if (overshift) {
    OperationState state(site.location, ac::BitsConstantOp::getOperationName());
    state.addTypes(bitsType(builder, resultWidth, site));
    state.addAttribute(
        "value", integerExpression(builder, APSInt::getUnsigned(0), site));
    result = builder.create(state)->getResult(0);
  } else {
    OperationState state(site.location, ac::BitsExtractOp::getOperationName());
    state.addOperands(input.value);
    state.addTypes(amount == 0 ? input.value.getType()
                               : bitsType(builder, resultWidth, site));
    state.addAttribute("low", integerExpression(builder, count, site));
    result = builder.create(state)->getResult(0);
  }
  return NumericValue{result, ValueKind::Integer, std::move(interval)};
}

FailureOr<Value> lowerExactIntegerBoundary(OpBuilder &builder,
                                           const NumericLoweringSite &site,
                                           const NumericValue &value,
                                           ac::BitsType destination) {
  if (failed(verifySite(site)))
    return failure();
  auto sourceWidth = checkValue(value, site);
  auto targetWidth = widthOf(
      destination, value.value ? valueSite(value.value) : nullptr, site);
  if (failed(sourceWidth) || failed(targetWidth))
    return failure();
  if (*targetWidth == std::numeric_limits<unsigned>::max())
    return diagnostic(site)()
           << "integer boundary exceeds compiler representation capacity";
  APInt power(*targetWidth + 1, 0);
  power.setBit(*targetWidth);
  APSInt limit(std::move(power), true);
  if (value.interval->lower.isNegative() ||
      APSInt::compareValues(value.interval->upper, limit) > 0)
    return diagnostic(site)()
           << "integer interval is not proven within destination bounds";
  if (*sourceWidth == *targetWidth) {
    if (!ac::areEquivalentHardwareTypes(value.value.getType(), destination))
      return diagnostic(site)() << "integer boundary representation proof "
                                   "requires normalized equal-width types";
    return value.value;
  }
  return operation(builder, site.location, ac::BitsResizeOp::getOperationName(),
                   value.value, destination, "mode",
                   *sourceWidth > *targetWidth ? "trunc" : "zext");
}
} // namespace acir::compiler
