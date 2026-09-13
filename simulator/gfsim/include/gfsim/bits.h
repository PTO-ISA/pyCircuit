#ifndef GFSIM_BITS_H
#define GFSIM_BITS_H

#include "gfsim/packet.h"

#include <array>
#include <bit>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace gfsim {

/// Exact-width unsigned circuit value. Bit operations truncate to Width;
/// scalar arithmetic and signed interpretation are available only through 64
/// bits, where they remain independent of C++ integer-promotion rules.
template <unsigned Width> class UInt {
  static_assert(Width > 0 && Width <= (1u << 16),
                "gfsim::UInt width must be in [1, 65536]");

public:
  using storage_type = std::uint64_t;
  static constexpr unsigned width = Width;
  static constexpr size_t word_count = Width / 64 + (Width % 64 != 0);
  using word_array_type = std::array<storage_type, word_count>;

  constexpr UInt() = default;

  template <typename T>
    requires(std::is_integral_v<T>)
  constexpr UInt(T value) {
    if constexpr (std::is_signed_v<T>)
      if (value < 0)
        words_.fill(~storage_type{0});
    words_[0] = static_cast<storage_type>(value);
    normalize();
  }

  constexpr explicit UInt(word_array_type words) : words_(words) {
    normalize();
  }

  constexpr storage_type value() const
    requires(Width <= 64)
  {
    return words_[0];
  }
  constexpr storage_type word(size_t index) const {
    return index < word_count ? words_[index] : storage_type{0};
  }
  constexpr bool bit(size_t index) const {
    return index < Width && ((words_[index / 64] >> (index % 64)) & 1) != 0;
  }
  constexpr bool operator[](size_t index) const { return bit(index); }

  template <typename T>
    requires(std::is_integral_v<T> && Width <= 64)
  constexpr explicit operator T() const {
    return static_cast<T>(words_[0]);
  }
  constexpr explicit(Width != 1) operator bool() const {
    for (storage_type word : words_)
      if (word != 0)
        return true;
    return false;
  }

  friend constexpr UInt operator+(UInt lhs, UInt rhs)
    requires(Width <= 64)
  {
    return UInt(lhs.words_[0] + rhs.words_[0]);
  }
  friend constexpr UInt operator-(UInt lhs, UInt rhs)
    requires(Width <= 64)
  {
    return UInt(lhs.words_[0] - rhs.words_[0]);
  }
  friend constexpr UInt operator*(UInt lhs, UInt rhs)
    requires(Width <= 64)
  {
    return UInt(lhs.words_[0] * rhs.words_[0]);
  }
  friend constexpr UInt operator/(UInt lhs, UInt rhs)
    requires(Width <= 64)
  {
    return rhs.words_[0] == 0 ? UInt{} : UInt(lhs.words_[0] / rhs.words_[0]);
  }
  friend constexpr UInt operator%(UInt lhs, UInt rhs)
    requires(Width <= 64)
  {
    return rhs.words_[0] == 0 ? UInt{} : UInt(lhs.words_[0] % rhs.words_[0]);
  }
  friend constexpr UInt operator&(UInt lhs, UInt rhs) {
    word_array_type words{};
    for (size_t index = 0; index < word_count; ++index)
      words[index] = lhs.words_[index] & rhs.words_[index];
    return UInt(words);
  }
  friend constexpr UInt operator|(UInt lhs, UInt rhs) {
    word_array_type words{};
    for (size_t index = 0; index < word_count; ++index)
      words[index] = lhs.words_[index] | rhs.words_[index];
    return UInt(words);
  }
  friend constexpr UInt operator^(UInt lhs, UInt rhs) {
    word_array_type words{};
    for (size_t index = 0; index < word_count; ++index)
      words[index] = lhs.words_[index] ^ rhs.words_[index];
    return UInt(words);
  }
  friend constexpr UInt operator~(UInt value) {
    word_array_type words{};
    for (size_t index = 0; index < word_count; ++index)
      words[index] = ~value.words_[index];
    return UInt(words);
  }
  friend constexpr UInt operator<<(UInt lhs, UInt rhs)
    requires(Width <= 64)
  {
    return rhs.words_[0] >= Width ? UInt{}
                                  : UInt(lhs.words_[0] << rhs.words_[0]);
  }
  friend constexpr UInt operator>>(UInt lhs, UInt rhs)
    requires(Width <= 64)
  {
    return rhs.words_[0] >= Width ? UInt{}
                                  : UInt(lhs.words_[0] >> rhs.words_[0]);
  }

  constexpr std::int64_t signedValue() const
    requires(Width <= 64)
  {
    if constexpr (Width == 64)
      return std::bit_cast<std::int64_t>(words_[0]);
    if ((words_[0] & (storage_type{1} << (Width - 1))) == 0)
      return static_cast<std::int64_t>(words_[0]);
    const storage_type magnitude = ((~words_[0]) & lowMask()) + 1;
    return -static_cast<std::int64_t>(magnitude);
  }

  constexpr UInt arithmeticShiftRight(UInt rhs) const
    requires(Width <= 64)
  {
    if (rhs.words_[0] == 0)
      return *this;
    const bool negative = (words_[0] & (storage_type{1} << (Width - 1))) != 0;
    if (rhs.words_[0] >= Width)
      return negative ? UInt(lowMask()) : UInt{};
    storage_type shifted = words_[0] >> rhs.words_[0];
    if (negative)
      shifted |= lowMask() ^ ((storage_type{1} << (Width - rhs.words_[0])) - 1);
    return UInt(shifted);
  }

  template <std::integral T>
  friend constexpr UInt operator+(UInt lhs, T rhs)
    requires(Width <= 64)
  {
    return lhs + UInt(rhs);
  }
  template <std::integral T>
  friend constexpr UInt operator-(UInt lhs, T rhs)
    requires(Width <= 64)
  {
    return lhs - UInt(rhs);
  }
  template <std::integral T>
  friend constexpr UInt operator*(UInt lhs, T rhs)
    requires(Width <= 64)
  {
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
  template <std::integral T>
  friend constexpr UInt operator<<(UInt lhs, T rhs)
    requires(Width <= 64)
  {
    if constexpr (std::signed_integral<T>)
      if (rhs < 0)
        return UInt{};
    const storage_type amount = static_cast<storage_type>(rhs);
    return amount >= Width ? UInt{} : UInt(lhs.words_[0] << amount);
  }
  template <std::integral T>
  friend constexpr UInt operator>>(UInt lhs, T rhs)
    requires(Width <= 64)
  {
    if constexpr (std::signed_integral<T>)
      if (rhs < 0)
        return UInt{};
    const storage_type amount = static_cast<storage_type>(rhs);
    return amount >= Width ? UInt{} : UInt(lhs.words_[0] >> amount);
  }

  friend constexpr bool operator==(UInt, UInt) = default;
  friend constexpr bool operator<(UInt lhs, UInt rhs)
    requires(Width <= 64)
  {
    return lhs.words_[0] < rhs.words_[0];
  }
  friend constexpr bool operator>(UInt lhs, UInt rhs)
    requires(Width <= 64)
  {
    return rhs < lhs;
  }
  friend constexpr bool operator<=(UInt lhs, UInt rhs)
    requires(Width <= 64)
  {
    return !(rhs < lhs);
  }
  friend constexpr bool operator>=(UInt lhs, UInt rhs)
    requires(Width <= 64)
  {
    return !(lhs < rhs);
  }

  template <std::integral T> friend constexpr bool operator==(UInt lhs, T rhs) {
    return lhs == UInt(rhs);
  }
  template <std::integral T>
  friend constexpr bool operator<(UInt lhs, T rhs)
    requires(Width <= 64)
  {
    return lhs < UInt(rhs);
  }
  template <std::integral T>
  friend constexpr bool operator>(UInt lhs, T rhs)
    requires(Width <= 64)
  {
    return lhs > UInt(rhs);
  }
  template <std::integral T>
  friend constexpr bool operator<=(UInt lhs, T rhs)
    requires(Width <= 64)
  {
    return lhs <= UInt(rhs);
  }
  template <std::integral T>
  friend constexpr bool operator>=(UInt lhs, T rhs)
    requires(Width <= 64)
  {
    return lhs >= UInt(rhs);
  }

private:
  static constexpr storage_type lowMask() {
    if constexpr (Width >= 64)
      return ~storage_type{0};
    else
      return (storage_type{1} << Width) - 1;
  }

  constexpr void normalize() {
    if constexpr (Width % 64 != 0)
      words_.back() &= (storage_type{1} << (Width % 64)) - 1;
  }

  word_array_type words_{};
};

template <typename T> struct IsUInt : std::false_type {};
template <unsigned Width> struct IsUInt<UInt<Width>> : std::true_type {};
template <typename T> struct IsScalarUInt : std::false_type {};
template <unsigned Width>
struct IsScalarUInt<UInt<Width>> : std::bool_constant<(Width <= 64)> {};

template <typename T>
concept IntegralLike =
    std::integral<T> || IsScalarUInt<std::remove_cv_t<T>>::value;

template <typename T>
concept UnsignedIntegralLike =
    std::unsigned_integral<T> || IsScalarUInt<std::remove_cv_t<T>>::value;

template <unsigned Width>
  requires(Width <= 64)
constexpr std::int64_t signedValue(UInt<Width> value) {
  return value.signedValue();
}

template <std::integral T> constexpr std::int64_t signedValue(T value) {
  return static_cast<std::int64_t>(value);
}

template <unsigned ResultWidth, unsigned InputWidth>
constexpr UInt<ResultWidth> bitExtract(UInt<InputWidth> value, size_t lsb) {
  static_assert(ResultWidth > 0 && ResultWidth <= InputWidth);
  if (lsb > InputWidth || ResultWidth > InputWidth - lsb)
    return UInt<ResultWidth>{};
  typename UInt<ResultWidth>::word_array_type words{};
  for (size_t bit = 0; bit < ResultWidth; ++bit)
    if (value.bit(lsb + bit))
      words[bit / 64] |= std::uint64_t{1} << (bit % 64);
  return UInt<ResultWidth>{words};
}

template <unsigned... Widths>
  requires(sizeof...(Widths) > 0)
constexpr UInt<(Widths + ...)> bitConcat(UInt<Widths>... values) {
  constexpr unsigned ResultWidth = (Widths + ...);
  typename UInt<ResultWidth>::word_array_type words{};
  size_t cursor = ResultWidth;
  auto append = [&]<unsigned PartWidth>(UInt<PartWidth> value) {
    cursor -= PartWidth;
    for (size_t bit = 0; bit < PartWidth; ++bit)
      if (value.bit(bit))
        words[(cursor + bit) / 64] |= std::uint64_t{1} << ((cursor + bit) % 64);
  };
  (append(values), ...);
  return UInt<ResultWidth>{words};
}

template <unsigned BaseWidth, unsigned ValueWidth>
constexpr UInt<BaseWidth> bitInsert(UInt<BaseWidth> base,
                                    UInt<ValueWidth> value, size_t lsb) {
  static_assert(ValueWidth <= BaseWidth);
  if (lsb > BaseWidth || ValueWidth > BaseWidth - lsb)
    return base;
  typename UInt<BaseWidth>::word_array_type words{};
  for (size_t word = 0; word < UInt<BaseWidth>::word_count; ++word)
    words[word] = base.word(word);
  for (size_t bit = 0; bit < ValueWidth; ++bit) {
    const size_t target = lsb + bit;
    const std::uint64_t mask = std::uint64_t{1} << (target % 64);
    if (value.bit(bit))
      words[target / 64] |= mask;
    else
      words[target / 64] &= ~mask;
  }
  return UInt<BaseWidth>{words};
}

template <unsigned Width> struct PacketTraits<UInt<Width>> {
  static constexpr bool isPacket = false;
  static constexpr std::string_view schema = {};
  static constexpr size_t serializedSize = Width / 8 + (Width % 8 != 0);
  static constexpr size_t maximumSerializedSize = serializedSize;
  static constexpr size_t alignment = 1;
  static constexpr PacketEndianness endianness = PacketEndianness::Little;
  static constexpr std::array<PacketField, 0> fields{};
  static constexpr std::optional<std::string_view> routingField = std::nullopt;
  static constexpr std::optional<std::string_view> correlationField =
      std::nullopt;
};

} // namespace gfsim

#endif // GFSIM_BITS_H
