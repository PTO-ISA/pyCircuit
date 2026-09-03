#ifndef GFSIM_REGISTER_H
#define GFSIM_REGISTER_H

#include "gfsim/core.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace gfsim {

/// Scalar architectural register. Indexed devices use `RegFile`.
template <typename T> class Register {
public:
  T load() const { return value_; }
  void store(T value) { value_ = value; }
  void reset() { value_ = T{}; }

private:
  T value_{};
};

/// Fixed-size register file. Index 0 is wired to zero.
template <typename T, std::size_t N> class RegFile {
public:
  static_assert(N > 0, "RegFile requires a positive entry count");

  T read(std::uint32_t index) const {
    index %= static_cast<std::uint32_t>(N);
    if (index == 0)
      return T{};
    return regs_[index];
  }

  void write(std::uint32_t index, T value) {
    index %= static_cast<std::uint32_t>(N);
    if (index != 0)
      regs_[index] = value;
  }

  void reset() { regs_.fill(T{}); }

private:
  std::array<T, N> regs_{};
};

} // namespace gfsim

#endif // GFSIM_REGISTER_H
