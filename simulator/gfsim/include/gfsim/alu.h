#ifndef GFSIM_ALU_H
#define GFSIM_ALU_H

#include "gfsim/bits.h"

#include <cstdint>

namespace gfsim {

constexpr UInt<64> signExtend32(std::uint32_t value) {
  const std::uint64_t bits = value;
  return UInt<64>{(value & 0x80000000U) ? bits | 0xffffffff00000000ULL : bits};
}

template <unsigned LhsWidth, unsigned RhsWidth>
constexpr UInt<64> addw(UInt<LhsWidth> lhs, UInt<RhsWidth> rhs) {
  return signExtend32(static_cast<std::uint32_t>(lhs.value()) +
                      static_cast<std::uint32_t>(rhs.value()));
}

template <unsigned LhsWidth, unsigned RhsWidth>
constexpr UInt<64> subw(UInt<LhsWidth> lhs, UInt<RhsWidth> rhs) {
  return signExtend32(static_cast<std::uint32_t>(lhs.value()) -
                      static_cast<std::uint32_t>(rhs.value()));
}

template <unsigned LhsWidth, unsigned RhsWidth>
constexpr UInt<64> andw(UInt<LhsWidth> lhs, UInt<RhsWidth> rhs) {
  return signExtend32(static_cast<std::uint32_t>(lhs.value()) &
                      static_cast<std::uint32_t>(rhs.value()));
}

template <unsigned LhsWidth, unsigned RhsWidth>
constexpr UInt<64> orw(UInt<LhsWidth> lhs, UInt<RhsWidth> rhs) {
  return signExtend32(static_cast<std::uint32_t>(lhs.value()) |
                      static_cast<std::uint32_t>(rhs.value()));
}

template <unsigned LhsWidth, unsigned RhsWidth>
constexpr UInt<64> xorw(UInt<LhsWidth> lhs, UInt<RhsWidth> rhs) {
  return signExtend32(static_cast<std::uint32_t>(lhs.value()) ^
                      static_cast<std::uint32_t>(rhs.value()));
}

constexpr UInt<64> sll(UInt<64> lhs, UInt<64> rhs) {
  return UInt<64>{lhs.value() << (rhs.value() & 63U)};
}

constexpr UInt<64> srl(UInt<64> lhs, UInt<64> rhs) {
  return UInt<64>{lhs.value() >> (rhs.value() & 63U)};
}

constexpr UInt<64> sra(UInt<64> lhs, UInt<64> rhs) {
  return lhs.arithmeticShiftRight(UInt<64>{rhs.value() & 63U});
}

constexpr UInt<64> sllw(UInt<64> lhs, UInt<64> rhs) {
  const std::uint32_t value = static_cast<std::uint32_t>(lhs.value());
  return signExtend32(value << (rhs.value() & 31U));
}

constexpr UInt<64> srlw(UInt<64> lhs, UInt<64> rhs) {
  const std::uint32_t value = static_cast<std::uint32_t>(lhs.value());
  return signExtend32(value >> (rhs.value() & 31U));
}

constexpr UInt<64> sraw(UInt<64> lhs, UInt<64> rhs) {
  const std::uint32_t low = static_cast<std::uint32_t>(lhs.value());
  const std::uint64_t extended =
      (low & 0x80000000U)
          ? static_cast<std::uint64_t>(low) | 0xffffffff00000000ULL
          : static_cast<std::uint64_t>(low);
  return UInt<64>{extended}.arithmeticShiftRight(UInt<64>{rhs.value() & 31U});
}

constexpr UInt<64> smin(UInt<64> lhs, UInt<64> rhs) {
  return signedValue(lhs) < signedValue(rhs) ? lhs : rhs;
}

constexpr UInt<64> umin(UInt<64> lhs, UInt<64> rhs) {
  return lhs.value() < rhs.value() ? lhs : rhs;
}

constexpr UInt<64> smax(UInt<64> lhs, UInt<64> rhs) {
  return signedValue(lhs) > signedValue(rhs) ? lhs : rhs;
}

constexpr UInt<64> umax(UInt<64> lhs, UInt<64> rhs) {
  return lhs.value() > rhs.value() ? lhs : rhs;
}

constexpr UInt<64> mulw(UInt<64> lhs, UInt<64> rhs) {
  return signExtend32(static_cast<std::uint32_t>(lhs.value() * rhs.value()));
}

template <unsigned AuxWidth>
constexpr UInt<64> madd(UInt<64> lhs, UInt<64> rhs, UInt<AuxWidth> aux) {
  return UInt<64>{lhs.value() * rhs.value() + aux.value()};
}

template <unsigned AuxWidth>
constexpr UInt<64> maddw(UInt<64> lhs, UInt<64> rhs, UInt<AuxWidth> aux) {
  const std::uint32_t product =
      static_cast<std::uint32_t>(lhs.value() * rhs.value());
  const std::uint32_t addend = static_cast<std::uint32_t>(aux.value());
  return signExtend32(product + addend);
}

template <unsigned LhsWidth, unsigned RhsWidth, unsigned AuxWidth>
constexpr UInt<64> msub(UInt<LhsWidth> lhs, UInt<RhsWidth> rhs,
                        UInt<AuxWidth> aux) {
  return UInt<64>{aux.value() - lhs.value() * rhs.value()};
}

template <unsigned Width>
constexpr UInt<64> sextLow(UInt<64> value, UInt<Width> fieldWidth) {
  const std::uint64_t width = fieldWidth.value();
  if (width == 0)
    return UInt<64>{};
  if (width >= 64)
    return value;
  const std::uint64_t mask = (std::uint64_t{1} << width) - 1;
  const std::uint64_t low = value.value() & mask;
  return UInt<64>{(low & (std::uint64_t{1} << (width - 1))) ? low | ~mask
                                                            : low};
}

template <unsigned Width>
constexpr UInt<64> zextLow(UInt<64> value, UInt<Width> fieldWidth) {
  const std::uint64_t width = fieldWidth.value();
  if (width == 0)
    return UInt<64>{};
  if (width >= 64)
    return value;
  return UInt<64>{value.value() & ((std::uint64_t{1} << width) - 1)};
}

constexpr UInt<64> csel(UInt<1> predicate, UInt<64> lhs, UInt<64> rhs,
                        UInt<1> negateFalse) {
  if (predicate.value() != 0)
    return lhs;
  return negateFalse.value() != 0 ? UInt<64>{0U - rhs.value()} : rhs;
}

} // namespace gfsim

#endif // GFSIM_ALU_H
