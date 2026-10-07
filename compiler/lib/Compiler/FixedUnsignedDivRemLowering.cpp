#include "FixedUnsignedDivRemLowering.h"

#include "Dialect/ACIR/ACIRSourceContracts.h"
#include "mlir/IR/Diagnostics.h"

#include <limits>

using namespace mlir;
namespace acir::compiler {
namespace {

ac::StaticExprAttr integerExpression(OpBuilder &builder,
                                     const llvm::APSInt &integer,
                                     const NumericLoweringSite &site) {
  return ac::StaticExprAttr::get(
      builder.getContext(),
      builder.getDictionaryAttr(
          {builder.getNamedAttr("kind", builder.getStringAttr("literal")),
           builder.getNamedAttr(
               "value",
               builder.getDictionaryAttr(
                   {builder.getNamedAttr("kind", builder.getStringAttr("integer")),
                    builder.getNamedAttr("value", ac::MathIntAttr::get(
                                                      builder.getContext(),
                                                      integer))})),
           builder.getNamedAttr("origin", site.origin),
           builder.getNamedAttr("location", site.sourceSpan)}));
}

Value operation(OpBuilder &builder, const NumericLoweringSite &site,
                StringRef name, ValueRange operands, Type type,
                NamedAttribute attribute) {
  OperationState state(site.location, name);
  state.addOperands(operands);
  state.addTypes(type);
  state.addAttribute(attribute.getName(), attribute.getValue());
  return builder.create(state)->getResult(0);
}

ModuleOp packageOf(Operation *operation) {
  if (!operation)
    return {};
  if (auto package = dyn_cast<ModuleOp>(operation))
    return package;
  return operation->getParentOfType<ModuleOp>();
}

} // namespace

FailureOr<Value> lowerFixedUnsignedDivRem(
    OpBuilder &builder, ac::HardwareAnalysis &analysis,
    const NumericLoweringSite &site, const NumericValue &numerator,
    bool authoritativeUnsigned, const llvm::APSInt &divisor,
    FixedUnsignedDivRemResult resultKind) {
  auto error = [&] { return mlir::emitError(site.location); };
  auto *context = builder.getContext();
  if (site.location.getContext() != context ||
      (site.origin && site.origin.getContext() != context) ||
      (site.sourceSpan && site.sourceSpan.getContext() != context) ||
      !analysis.getPackage() ||
      analysis.getPackage().getContext() != context)
    return error() << "unsigned division/remainder context mismatch";
  if (failed(ac::detail::verifyOccurrence(site.origin, error)) ||
      failed(ac::detail::verifySourceSpan(site.sourceSpan, error)))
    return failure();
  auto *block = builder.getInsertionBlock();
  auto *owner = block ? block->getParentOp() : nullptr;
  if (!owner || packageOf(owner) != analysis.getPackage())
    return error() << "unsigned division/remainder requires a valid insertion "
                      "point";
  if (resultKind != FixedUnsignedDivRemResult::Quotient &&
      resultKind != FixedUnsignedDivRemResult::Remainder)
    return error() << "unsigned division/remainder has an invalid result mode";
  if (!numerator.value || !authoritativeUnsigned ||
      numerator.sourceKind == ac::detail::ValueKind::Boolean ||
      !isa<ac::BitsType>(numerator.value.getType()))
    return error() << "unsigned division/remainder requires authoritative "
                      "unsigned bits";
  if (numerator.value.getType().getContext() != context)
    return error() << "unsigned division/remainder context mismatch";
  Operation *producer = numerator.value.getDefiningOp();
  if (auto argument = dyn_cast<BlockArgument>(numerator.value))
    producer = argument.getOwner()->getParentOp();
  if (packageOf(producer) != analysis.getPackage())
    return error() << "unsigned division/remainder context mismatch";

  auto width = analysis.getPackedWidth(numerator.value.getType(), {}, owner);
  if (failed(width))
    return failure();
  if (!*width)
    return error() << "unsigned division/remainder requires a positive "
                      "resolved input width";
  if (divisor.isNegative() || divisor.isZero())
    return error() << "unsigned division/remainder divisor must be positive";
  if (divisor.getActiveBits() > *width)
    return error() << "unsigned division/remainder divisor must fit input width";

  // Prove every APInt size before allocating the reciprocal's power of two.
  constexpr uint64_t capacity = std::numeric_limits<unsigned>::max();
  uint64_t logarithm = divisor.getActiveBits() - divisor.isPowerOf2();
  if (*width > capacity || logarithm > capacity - *width)
    return error() << "unsigned division/remainder geometry exceeds "
                      "compiler representation capacity";
  uint64_t shift = *width + logarithm;
  if (shift == capacity || *width > capacity - shift)
    return error() << "unsigned division/remainder geometry exceeds "
                      "compiler representation capacity";
  unsigned productWidth = static_cast<unsigned>(*width + shift);
  unsigned low = static_cast<unsigned>(shift);
  llvm::APInt power(productWidth, 0);
  power.setBit(low);
  llvm::APInt denominator = divisor.zextOrTrunc(productWidth);
  llvm::APInt reciprocal, residual;
  llvm::APInt::udivrem(power, denominator, reciprocal, residual);
  if (!residual.isZero()) {
    bool overflow;
    reciprocal = reciprocal.uadd_ov(llvm::APInt(productWidth, 1), overflow);
    if (overflow)
      return error() << "unsigned division/remainder reciprocal exceeds its "
                        "proven representation";
  }
  if (reciprocal.ugt(power))
    return error() << "unsigned division/remainder reciprocal exceeds its "
                      "proven representation";

  auto wideType = ac::BitsType::get(
      context, integerExpression(builder,
                                 llvm::APSInt::getUnsigned(productWidth), site));
  auto multiplier = integerExpression(
      builder, llvm::APSInt(std::move(reciprocal), true), site);
  auto offset = integerExpression(builder, llvm::APSInt::getUnsigned(low), site);
  auto divisorValue = integerExpression(builder, divisor, site);
  auto originalType = numerator.value.getType();

  // Insertion starts only after all semantic and representation proofs succeed.
  auto wide = operation(
      builder, site, ac::BitsResizeOp::getOperationName(), numerator.value,
      wideType, builder.getNamedAttr("mode", builder.getStringAttr("zext")));
  auto constant = operation(
      builder, site, ac::BitsConstantOp::getOperationName(), {}, wideType,
      builder.getNamedAttr("value", multiplier));
  auto product = operation(
      builder, site, ac::BitsBinaryOp::getOperationName(), {wide, constant},
      wideType, builder.getNamedAttr("opcode", builder.getStringAttr("mul")));
  auto quotient = operation(
      builder, site, ac::BitsExtractOp::getOperationName(), product, originalType,
      builder.getNamedAttr("low", offset));
  if (resultKind == FixedUnsignedDivRemResult::Quotient)
    return quotient;
  auto originalDivisor = operation(
      builder, site, ac::BitsConstantOp::getOperationName(), {}, originalType,
      builder.getNamedAttr("value", divisorValue));
  auto scaled = operation(
      builder, site, ac::BitsBinaryOp::getOperationName(),
      {quotient, originalDivisor}, originalType,
      builder.getNamedAttr("opcode", builder.getStringAttr("mul")));
  return operation(
      builder, site, ac::BitsBinaryOp::getOperationName(),
      {numerator.value, scaled}, originalType,
      builder.getNamedAttr("opcode", builder.getStringAttr("sub")));
}

} // namespace acir::compiler
