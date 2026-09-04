#ifndef GFSIM_BITS_H
#define GFSIM_BITS_H

#include <concepts>
#include <cstdint>
#include <type_traits>

namespace gfsim {

/// Exact-width unsigned circuit value. Every producing operation truncates
/// modulo 2^Width, independent of C++ integer-promotion rules.
template <unsigned Width> class UInt {
  static_assert(Width > 0 && Width <= 64,
                "gfsim::UInt width must be in [1, 64]");

public:
  using storage_type = std::uint64_t;
  static constexpr unsigned width = Width;

  constexpr UInt() = default;

  template <typename T>
    requires(std::is_integral_v<T>)
  constexpr UInt(T value) : value_(static_cast<storage_type>(value) & mask()) {}

  constexpr storage_type value() const { return value_; }
  template <typename T>
    requires(std::is_integral_v<T>)
  constexpr explicit operator T() const {
    return static_cast<T>(value_);
  }
  constexpr explicit(Width != 1) operator bool() const { return value_ != 0; }

  friend constexpr UInt operator+(UInt lhs, UInt rhs) {
    return UInt(lhs.value_ + rhs.value_);
  }
  friend constexpr UInt operator-(UInt lhs, UInt rhs) {
    return UInt(lhs.value_ - rhs.value_);
  }
  friend constexpr UInt operator*(UInt lhs, UInt rhs) {
    return UInt(lhs.value_ * rhs.value_);
  }
  friend constexpr UInt operator&(UInt lhs, UInt rhs) {
    return UInt(lhs.value_ & rhs.value_);
  }
  friend constexpr UInt operator|(UInt lhs, UInt rhs) {
    return UInt(lhs.value_ | rhs.value_);
  }
  friend constexpr UInt operator^(UInt lhs, UInt rhs) {
    return UInt(lhs.value_ ^ rhs.value_);
  }
  friend constexpr UInt operator~(UInt value) { return UInt(~value.value_); }
  friend constexpr UInt operator<<(UInt lhs, UInt rhs) {
    return rhs.value_ >= Width ? UInt{} : UInt(lhs.value_ << rhs.value_);
  }
  friend constexpr UInt operator>>(UInt lhs, UInt rhs) {
    return rhs.value_ >= Width ? UInt{} : UInt(lhs.value_ >> rhs.value_);
  }

  template <std::integral T> friend constexpr UInt operator+(UInt lhs, T rhs) {
    return lhs + UInt(rhs);
  }
  template <std::integral T> friend constexpr UInt operator-(UInt lhs, T rhs) {
    return lhs - UInt(rhs);
  }
  template <std::integral T> friend constexpr UInt operator*(UInt lhs, T rhs) {
    return lhs * UInt(rhs);
  }
  template <std::integral T> friend constexpr UInt operator&(UInt lhs, T rhs) {
    return lhs & UInt(rhs);
  }
  template <std::integral T> friend constexpr UInt operator|(UInt lhs, T rhs) {
    return lhs | UInt(rhs);
  }
  template <std::integral T> friend constexpr UInt operator^(UInt lhs, T rhs) {
    return lhs ^ UInt(rhs);
  }
  template <std::integral T> friend constexpr UInt operator<<(UInt lhs, T rhs) {
    if constexpr (std::signed_integral<T>)
      if (rhs < 0)
        return UInt{};
    const storage_type amount = static_cast<storage_type>(rhs);
    return amount >= Width ? UInt{} : UInt(lhs.value_ << amount);
  }
  template <std::integral T> friend constexpr UInt operator>>(UInt lhs, T rhs) {
    if constexpr (std::signed_integral<T>)
      if (rhs < 0)
        return UInt{};
    const storage_type amount = static_cast<storage_type>(rhs);
    return amount >= Width ? UInt{} : UInt(lhs.value_ >> amount);
  }

  friend constexpr bool operator==(UInt, UInt) = default;
  friend constexpr bool operator<(UInt lhs, UInt rhs) {
    return lhs.value_ < rhs.value_;
  }
  friend constexpr bool operator>(UInt lhs, UInt rhs) { return rhs < lhs; }
  friend constexpr bool operator<=(UInt lhs, UInt rhs) { return !(rhs < lhs); }
  friend constexpr bool operator>=(UInt lhs, UInt rhs) { return !(lhs < rhs); }

  template <std::integral T> friend constexpr bool operator==(UInt lhs, T rhs) {
    return lhs == UInt(rhs);
  }
  template <std::integral T> friend constexpr bool operator<(UInt lhs, T rhs) {
    return lhs < UInt(rhs);
  }
  template <std::integral T> friend constexpr bool operator>(UInt lhs, T rhs) {
    return lhs > UInt(rhs);
  }
  template <std::integral T> friend constexpr bool operator<=(UInt lhs, T rhs) {
    return lhs <= UInt(rhs);
  }
  template <std::integral T> friend constexpr bool operator>=(UInt lhs, T rhs) {
    return lhs >= UInt(rhs);
  }

private:
  static constexpr storage_type mask() {
    if constexpr (Width == 64)
      return ~storage_type{0};
    else
      return (storage_type{1} << Width) - 1;
  }

  storage_type value_ = 0;
};

template <typename T> struct IsUInt : std::false_type {};
template <unsigned Width> struct IsUInt<UInt<Width>> : std::true_type {};

template <typename T>
concept IntegralLike = std::integral<T> || IsUInt<std::remove_cv_t<T>>::value;

template <typename T>
concept UnsignedIntegralLike =
    std::unsigned_integral<T> || IsUInt<std::remove_cv_t<T>>::value;

} // namespace gfsim

#endif // GFSIM_BITS_H
