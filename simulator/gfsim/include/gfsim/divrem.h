#ifndef GFSIM_DIVREM_H
#define GFSIM_DIVREM_H

#include "gfsim/bits.h"

#include <cstdint>
#include <limits>

namespace gfsim {

struct DivRemResult {
  UInt<64> quotient{};
  UInt<64> remainder{};
};

constexpr std::int64_t signExtend32(std::uint64_t value) {
  const std::uint32_t low = static_cast<std::uint32_t>(value);
  return static_cast<std::int64_t>(static_cast<std::int32_t>(low));
}

constexpr UInt<64> signExtend32Bits(std::uint64_t value) {
  return UInt<64>{static_cast<std::uint64_t>(signExtend32(value))};
}

constexpr DivRemResult divrem(UInt<64> lhs, UInt<64> rhs, bool signed_mode,
                              bool word_mode) {
  if (word_mode) {
    if (signed_mode) {
      const std::int64_t dividend = signExtend32(lhs.value());
      const std::int64_t divisor = signExtend32(rhs.value());
      if (divisor == 0)
        return {UInt<64>{0}, signExtend32Bits(lhs.value())};
      if (dividend == std::numeric_limits<std::int32_t>::min() && divisor == -1)
        return {signExtend32Bits(0x80000000ULL), UInt<64>{0}};
      return {signExtend32Bits(static_cast<std::uint32_t>(dividend / divisor)),
              signExtend32Bits(static_cast<std::uint32_t>(dividend % divisor))};
    }
    const std::uint64_t dividend = lhs.value() & 0xffffffffULL;
    const std::uint64_t divisor = rhs.value() & 0xffffffffULL;
    if (divisor == 0)
      return {UInt<64>{0}, signExtend32Bits(dividend)};
    return {signExtend32Bits(static_cast<std::uint32_t>(dividend / divisor)),
            signExtend32Bits(static_cast<std::uint32_t>(dividend % divisor))};
  }

  if (signed_mode) {
    const std::int64_t dividend = lhs.signedValue();
    const std::int64_t divisor = rhs.signedValue();
    if (divisor == 0)
      return {UInt<64>{0}, lhs};
    if (dividend == std::numeric_limits<std::int64_t>::min() && divisor == -1)
      return {lhs, UInt<64>{0}};
    return {UInt<64>{dividend / divisor}, UInt<64>{dividend % divisor}};
  }
  const std::uint64_t dividend = lhs.value();
  const std::uint64_t divisor = rhs.value();
  if (divisor == 0)
    return {UInt<64>{0}, lhs};
  return {UInt<64>{dividend / divisor}, UInt<64>{dividend % divisor}};
}

} // namespace gfsim

#endif // GFSIM_DIVREM_H
