// SPDX-License-Identifier: BSD-3-Clause
#pragma once

#include "gfsim/wire.h"
#include <cstddef>
#include <limits>

namespace gfsim::detail {

// Preserve all address bits before converting to a host index. High nonzero
// bits yield an out-of-bounds index rather than aliasing low addresses.
template <unsigned Width>
constexpr std::size_t memory_index(Bits<Width> addr) noexcept {
  constexpr unsigned hostBits = sizeof(std::size_t) * 8u;
  if constexpr (Width <= hostBits)
    return static_cast<std::size_t>(addr.value());
  for (unsigned bit = hostBits; bit < Width; ++bit)
    if (addr.bit(bit))
      return std::numeric_limits<std::size_t>::max();
  std::size_t result = 0;
  for (unsigned bit = 0; bit < hostBits; ++bit)
    if (addr.bit(bit))
      result |= std::size_t{1} << bit;
  return result;
}

template <unsigned Width, unsigned StrobeWidth>
constexpr Bits<Width> memory_strobe(Bits<Width> oldValue, Bits<Width> newValue,
                                    Bits<StrobeWidth> strobe) noexcept {
  Bits<Width> mask{};
  for (unsigned lane = 0; lane < StrobeWidth; ++lane) {
    if (!strobe.bit(lane))
      continue;
    const unsigned low = lane * 8;
    for (unsigned bit = 0; bit < 8 && bit < Width - low; ++bit)
      mask = mask | shl(Bits<Width>{1}, low + bit);
  }
  return (oldValue & ~mask) | (newValue & mask);
}

} // namespace gfsim::detail
