#include "SourceValueSemantics.h"

#include "llvm/ADT/APSInt.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/Support/ErrorHandling.h"

#include <algorithm>
#include <limits>

using namespace mlir;

namespace acir::ac::detail {
namespace {

std::optional<ValueOpcodeInfo> opcodeInfo(ValueOpcode opcode) {
  using C = ValueKindConstraint;
  using K = ValueKind;
  using E = ValueEvaluation;
  switch (opcode) {
  case ValueOpcode::Neg:
  case ValueOpcode::Invert:
    return ValueOpcodeInfo{
        1, {C::Integer, C::BooleanOrInteger}, K::Integer, E::Strict, false};
  case ValueOpcode::Not:
    return ValueOpcodeInfo{
        1, {C::Boolean, C::BooleanOrInteger}, K::Boolean, E::Strict, false};
  case ValueOpcode::ToInt:
    return ValueOpcodeInfo{1,
                           {C::BooleanOrInteger, C::BooleanOrInteger},
                           K::Integer,
                           E::Strict,
                           false};
  case ValueOpcode::Add:
  case ValueOpcode::Sub:
  case ValueOpcode::Mul:
  case ValueOpcode::AndBits:
  case ValueOpcode::OrBits:
  case ValueOpcode::XorBits:
    return ValueOpcodeInfo{
        2, {C::Integer, C::Integer}, K::Integer, E::Strict, false};
  case ValueOpcode::FloorDiv:
  case ValueOpcode::Mod:
  case ValueOpcode::Shl:
  case ValueOpcode::Shr:
    return ValueOpcodeInfo{
        2, {C::Integer, C::Integer}, K::Integer, E::Strict, true};
  case ValueOpcode::Eq:
  case ValueOpcode::Ne:
  case ValueOpcode::Lt:
  case ValueOpcode::Le:
  case ValueOpcode::Gt:
  case ValueOpcode::Ge:
    return ValueOpcodeInfo{
        2, {C::Integer, C::Integer}, K::Boolean, E::Strict, false};
  case ValueOpcode::AndBool:
    return ValueOpcodeInfo{
        2, {C::Boolean, C::Boolean}, K::Boolean, E::AndShortCircuit, false};
  case ValueOpcode::OrBool:
    return ValueOpcodeInfo{
        2, {C::Boolean, C::Boolean}, K::Boolean, E::OrShortCircuit, false};
  }
  return std::nullopt;
}

FailureOr<unsigned> checkedWidth(uint64_t width, EmitError error) {
  if (!width || width > std::numeric_limits<unsigned>::max())
    return error()
           << "mathematical value precision exceeds compiler APInt capacity";
  return static_cast<unsigned>(width);
}

uint64_t signedWidth(const llvm::APSInt &value) {
  if (value.isNegative())
    return value.getSignificantBits();
  return static_cast<uint64_t>(value.getActiveBits()) + 1;
}

llvm::APInt signedBits(const llvm::APSInt &value, unsigned width) {
  // Trimming redundant high bits is safe because callers first compute the
  // mathematical signed precision, then extend to the operation's full width.
  return value.isUnsigned() ? value.zextOrTrunc(width)
                            : value.sextOrTrunc(width);
}

Attribute integer(MLIRContext *context, llvm::APInt bits) {
  return MathIntAttr::get(context,
                          llvm::APSInt(std::move(bits), /*isUnsigned=*/false));
}

FailureOr<llvm::APSInt> decodeInteger(MathIntAttr value, EmitError error) {
  if (failed(parseMathIntAttr(value.getContext(), value.getCanonicalValue(),
                              error)))
    return failure();
  return llvm::APSInt(value.getCanonicalValue());
}

FailureOr<Attribute> shift(ValueOpcode opcode, const llvm::APSInt &value,
                           const llvm::APSInt &count, MLIRContext *context,
                           EmitError error) {
  if (count.isNegative())
    return error() << "mathematical shift count must be nonnegative";
  // Negative counts still diagnose for zero; nonnegative huge left shifts of
  // zero require no allocation or narrowing of the count.
  if (value.isZero())
    return Attribute(MathIntAttr::get(context, llvm::APSInt::get(0)));
  uint64_t precision = signedWidth(value);
  if (opcode == ValueOpcode::Shr) {
    auto limit = llvm::APSInt::getUnsigned(precision);
    if (llvm::APSInt::compareValues(count, limit) >= 0)
      return Attribute(MathIntAttr::get(
          context, llvm::APSInt::get(value.isNegative() ? -1 : 0)));
    auto width = checkedWidth(precision, error);
    if (failed(width))
      return failure();
    // Only now is the count proved smaller than a representable APInt width.
    unsigned amount = static_cast<unsigned>(count.getZExtValue());
    return integer(context, signedBits(value, *width).ashr(amount));
  }
  auto baseWidth = checkedWidth(precision, error);
  if (failed(baseWidth))
    return failure();
  uint64_t available = std::numeric_limits<unsigned>::max() - precision;
  if (llvm::APSInt::compareValues(count, llvm::APSInt::getUnsigned(available)) >
      0)
    return error()
           << "mathematical left shift result exceeds compiler APInt capacity";
  uint64_t amount = count.getZExtValue();
  auto width = checkedWidth(precision + amount, error);
  if (failed(width))
    return failure();
  return integer(context,
                 signedBits(value, *width).shl(static_cast<unsigned>(amount)));
}

FailureOr<Attribute> integerOperation(ValueOpcode opcode,
                                      ArrayRef<llvm::APSInt> values,
                                      MLIRContext *context, EmitError error) {
  const llvm::APSInt &lhs = values.front();
  uint64_t precision = signedWidth(lhs);
  if (values.size() == 2 &&
      (opcode == ValueOpcode::Shl || opcode == ValueOpcode::Shr))
    return shift(opcode, lhs, values[1], context, error);
  if (values.size() == 2)
    precision = std::max(precision, signedWidth(values[1]));
  if (opcode == ValueOpcode::Neg || opcode == ValueOpcode::Add ||
      opcode == ValueOpcode::Sub || opcode == ValueOpcode::FloorDiv ||
      opcode == ValueOpcode::Mod)
    ++precision;
  if (opcode == ValueOpcode::Mul)
    precision = signedWidth(lhs) + signedWidth(values[1]);
  auto width = checkedWidth(precision, error);
  if (failed(width))
    return failure();
  llvm::APInt a = signedBits(lhs, *width);
  if (opcode == ValueOpcode::Neg)
    return integer(context, -a);
  if (opcode == ValueOpcode::Invert)
    return integer(context, ~a);
  llvm::APInt b = signedBits(values[1], *width);
  switch (opcode) {
  case ValueOpcode::Add:
    return integer(context, a + b);
  case ValueOpcode::Sub:
    return integer(context, a - b);
  case ValueOpcode::Mul:
    return integer(context, a * b);
  case ValueOpcode::AndBits:
    return integer(context, a & b);
  case ValueOpcode::OrBits:
    return integer(context, a | b);
  case ValueOpcode::XorBits:
    return integer(context, a ^ b);
  case ValueOpcode::Eq:
    return Attribute(BoolAttr::get(context, a == b));
  case ValueOpcode::Ne:
    return Attribute(BoolAttr::get(context, a != b));
  case ValueOpcode::Lt:
    return Attribute(BoolAttr::get(context, a.slt(b)));
  case ValueOpcode::Le:
    return Attribute(BoolAttr::get(context, a.sle(b)));
  case ValueOpcode::Gt:
    return Attribute(BoolAttr::get(context, a.sgt(b)));
  case ValueOpcode::Ge:
    return Attribute(BoolAttr::get(context, a.sge(b)));
  case ValueOpcode::FloorDiv:
  case ValueOpcode::Mod: {
    if (b.isZero())
      return error()
             << "mathematical division/remainder divisor must be nonzero";
    llvm::APInt quotient(*width, 0), remainder(*width, 0);
    llvm::APInt::sdivrem(a, b, quotient, remainder);
    if (!remainder.isZero() && a.isNegative() != b.isNegative()) {
      quotient -= llvm::APInt(*width, 1);
      remainder += b;
    }
    return integer(context, opcode == ValueOpcode::FloorDiv
                                ? std::move(quotient)
                                : std::move(remainder));
  }
  default:
    return error() << "invalid mathematical integer value opcode";
  }
}
} // namespace

std::optional<ValueOpcode> parseValueOpcode(StringRef spelling) {
  return llvm::StringSwitch<std::optional<ValueOpcode>>(spelling)
      .Case("neg", ValueOpcode::Neg)
      .Case("invert", ValueOpcode::Invert)
      .Case("not", ValueOpcode::Not)
      .Case("to_int", ValueOpcode::ToInt)
      .Case("add", ValueOpcode::Add)
      .Case("sub", ValueOpcode::Sub)
      .Case("mul", ValueOpcode::Mul)
      .Case("floordiv", ValueOpcode::FloorDiv)
      .Case("mod", ValueOpcode::Mod)
      .Case("and_bits", ValueOpcode::AndBits)
      .Case("or_bits", ValueOpcode::OrBits)
      .Case("xor_bits", ValueOpcode::XorBits)
      .Case("shl", ValueOpcode::Shl)
      .Case("shr", ValueOpcode::Shr)
      .Case("eq", ValueOpcode::Eq)
      .Case("ne", ValueOpcode::Ne)
      .Case("lt", ValueOpcode::Lt)
      .Case("le", ValueOpcode::Le)
      .Case("gt", ValueOpcode::Gt)
      .Case("ge", ValueOpcode::Ge)
      .Case("and_bool", ValueOpcode::AndBool)
      .Case("or_bool", ValueOpcode::OrBool)
      .Default(std::nullopt);
}

ValueOpcodeInfo getValueOpcodeInfo(ValueOpcode opcode) {
  auto info = opcodeInfo(opcode);
  if (!info)
    llvm_unreachable(
        "ValueOpcode metadata requires a member of the closed enum");
  return *info;
}

FailureOr<Attribute> evaluateValue(ValueOpcode opcode,
                                   ArrayRef<Attribute> operands,
                                   EmitError error) {
  auto info = opcodeInfo(opcode);
  if (!info)
    return error() << "unknown mathematical value opcode";
  if (operands.size() != info->arity)
    return error() << "value opcode requires " << info->arity << " operands";
  SmallVector<llvm::APSInt> integers;
  for (auto [index, operand] : llvm::enumerate(operands)) {
    auto boolean = dyn_cast_or_null<BoolAttr>(operand);
    auto math = dyn_cast_or_null<MathIntAttr>(operand);
    auto constraint = info->operands[index];
    if ((!boolean && !math) ||
        (constraint == ValueKindConstraint::Boolean && !boolean) ||
        (constraint == ValueKindConstraint::Integer && !math))
      return error() << "value operand " << index
                     << " has incompatible bool/integer kind";
    if (math) {
      auto decoded = decodeInteger(math, error);
      if (failed(decoded))
        return failure();
      integers.push_back(std::move(*decoded));
    }
  }
  MLIRContext *context = operands.front().getContext();
  if (opcode == ValueOpcode::ToInt) {
    if (auto value = dyn_cast<BoolAttr>(operands.front()))
      return Attribute(MathIntAttr::get(
          context, llvm::APSInt::get(value.getValue() ? 1 : 0)));
    return operands.front();
  }
  if (opcode == ValueOpcode::Not)
    return Attribute(
        BoolAttr::get(context, !cast<BoolAttr>(operands.front()).getValue()));
  if (opcode == ValueOpcode::AndBool || opcode == ValueOpcode::OrBool) {
    bool lhs = cast<BoolAttr>(operands[0]).getValue();
    bool rhs = cast<BoolAttr>(operands[1]).getValue();
    return Attribute(BoolAttr::get(
        context, opcode == ValueOpcode::AndBool ? lhs && rhs : lhs || rhs));
  }
  return integerOperation(opcode, integers, context, error);
}

} // namespace acir::ac::detail
