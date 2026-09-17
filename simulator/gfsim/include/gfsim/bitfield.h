#ifndef GFSIM_BITFIELD_H
#define GFSIM_BITFIELD_H

#include "gfsim/bits.h"

#include <cstdint>

namespace gfsim {

constexpr std::uint64_t bitfieldMask(std::uint64_t width) {
  return width >= 64 ? ~std::uint64_t{0}
                     : (width == 0 ? 0 : (std::uint64_t{1} << width) - 1);
}

constexpr std::uint64_t rotateRight64(std::uint64_t value,
                                      std::uint64_t offset) {
  offset &= 63U;
  return offset == 0 ? value : (value >> offset) | (value << (64 - offset));
}

constexpr std::uint64_t rotateLeft64(std::uint64_t value,
                                     std::uint64_t offset) {
  offset &= 63U;
  return offset == 0 ? value : (value << offset) | (value >> (64 - offset));
}

template <unsigned WidthWidth, unsigned OffsetWidth>
constexpr UInt<64> bitfieldExtract(UInt<64> value, UInt<WidthWidth> width,
                                   UInt<OffsetWidth> offset,
                                   bool signedResult) {
  const std::uint64_t fieldWidth = width.value();
  if (fieldWidth == 0 || fieldWidth > 64)
    return UInt<64>{};
  const std::uint64_t result =
      rotateRight64(value.value(), offset.value()) & bitfieldMask(fieldWidth);
  if (!signedResult || fieldWidth == 64 ||
      (result & (std::uint64_t{1} << (fieldWidth - 1))) == 0)
    return UInt<64>{result};
  return UInt<64>{result | ~bitfieldMask(fieldWidth)};
}

template <unsigned WidthWidth, unsigned OffsetWidth>
constexpr UInt<64> bitfieldPopcount(UInt<64> value, UInt<WidthWidth> width,
                                    UInt<OffsetWidth> offset) {
  const std::uint64_t field =
      bitfieldExtract(value, width, offset, false).value();
  return UInt<64>{static_cast<std::uint64_t>(std::popcount(field) & 0x7f)};
}

template <unsigned WidthWidth, unsigned OffsetWidth>
constexpr UInt<64> bitfieldClz(UInt<64> value, UInt<WidthWidth> width,
                               UInt<OffsetWidth> offset) {
  const std::uint64_t fieldWidth = width.value();
  if (fieldWidth == 0 || fieldWidth > 64)
    return UInt<64>{};
  const std::uint64_t field =
      bitfieldExtract(value, width, offset, false).value();
  if (field == 0)
    return UInt<64>{fieldWidth};
  std::uint64_t count = 0;
  for (std::uint64_t bit = fieldWidth;
       bit != 0 && ((field >> (bit - 1)) & 1U) == 0; --bit)
    ++count;
  return UInt<64>{count};
}

template <unsigned WidthWidth, unsigned OffsetWidth>
constexpr UInt<64> bitfieldCtz(UInt<64> value, UInt<WidthWidth> width,
                               UInt<OffsetWidth> offset) {
  const std::uint64_t fieldWidth = width.value();
  if (fieldWidth == 0 || fieldWidth > 64)
    return UInt<64>{};
  const std::uint64_t field =
      bitfieldExtract(value, width, offset, false).value();
  if (field == 0)
    return UInt<64>{fieldWidth};
  std::uint64_t count = 0;
  while (((field >> count) & 1U) == 0)
    ++count;
  return UInt<64>{count};
}

template <unsigned WidthWidth, unsigned OffsetWidth>
constexpr UInt<64> bitfieldClear(UInt<64> value, UInt<WidthWidth> width,
                                 UInt<OffsetWidth> offset) {
  const std::uint64_t fieldWidth = width.value();
  if (fieldWidth == 0 || fieldWidth > 64)
    return value;
  const std::uint64_t rotated =
      rotateRight64(value.value(), offset.value()) & ~bitfieldMask(fieldWidth);
  return UInt<64>{rotateLeft64(rotated, offset.value())};
}

template <unsigned WidthWidth, unsigned OffsetWidth>
constexpr UInt<64> bitfieldSet(UInt<64> value, UInt<WidthWidth> width,
                               UInt<OffsetWidth> offset) {
  const std::uint64_t fieldWidth = width.value();
  if (fieldWidth == 0 || fieldWidth > 64)
    return value;
  const std::uint64_t rotated =
      rotateRight64(value.value(), offset.value()) | bitfieldMask(fieldWidth);
  return UInt<64>{rotateLeft64(rotated, offset.value())};
}

template <unsigned WidthWidth, unsigned OffsetWidth>
constexpr UInt<64> bitfieldReverseBytes(UInt<64> value, UInt<WidthWidth> width,
                                        UInt<OffsetWidth> offset) {
  const std::uint64_t fieldWidth = width.value();
  if (fieldWidth == 0 || fieldWidth > 64 || (fieldWidth % 8) != 0)
    return UInt<64>{};
  const std::uint64_t field =
      bitfieldExtract(value, width, offset, false).value();
  std::uint64_t result = 0;
  for (std::uint64_t index = 0; index < fieldWidth / 8; ++index)
    result |= ((field >> (index * 8)) & 0xffU)
              << ((fieldWidth / 8 - index - 1) * 8);
  return UInt<64>{result};
}

template <unsigned SourceWidth, unsigned WidthWidth, unsigned OffsetWidth>
constexpr UInt<64> bitfieldInsert(UInt<64> value, UInt<SourceWidth> source,
                                  UInt<WidthWidth> width,
                                  UInt<OffsetWidth> offset) {
  const std::uint64_t fieldWidth = width.value();
  if (fieldWidth == 0 || fieldWidth > 64)
    return value;
  std::uint64_t result = value.value();
  for (std::uint64_t bit = 0; bit < fieldWidth; ++bit) {
    const std::uint64_t destination = (offset.value() + bit) & 63U;
    const std::uint64_t mask = std::uint64_t{1} << destination;
    result =
        (result & ~mask) | (((source.value() >> bit) & 1U) != 0 ? mask : 0);
  }
  return UInt<64>{result};
}

} // namespace gfsim

#endif // GFSIM_BITFIELD_H
