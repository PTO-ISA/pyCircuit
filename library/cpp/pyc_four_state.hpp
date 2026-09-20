#pragma once

#include "pyc_bits.hpp"

#include <stdexcept>

namespace pyc::cpp {

class FourStateViolation final : public std::runtime_error {
public:
  using std::runtime_error::runtime_error;
};

template <unsigned Width>
class FourState final {
public:
  using Mask = Wire<Width>;

  static FourState known(Mask value) {
    return FourState(value, Mask::ones(), Mask{0});
  }
  static FourState unknown(Mask value = Mask{0}) {
    return FourState(value, Mask{0}, Mask{0});
  }
  static FourState highImpedance(Mask value = Mask{0}) {
    return FourState(value, Mask{0}, Mask::ones());
  }
  static FourState fromMasks(Mask value, Mask knownMask, Mask zMask) {
    return FourState(value, knownMask, zMask);
  }

  const Mask &value() const { return value_; }
  const Mask &knownMask() const { return knownMask_; }
  const Mask &zMask() const { return zMask_; }

  bool isFullyKnown() const {
    return knownMask_ == Mask::ones() && zMask_ == Mask{0};
  }
  bool invariantHolds() const {
    return (knownMask_ & zMask_) == Mask{0};
  }

  bool parityEquivalent(const FourState &other) const {
    if (!invariantHolds() || !other.invariantHolds() ||
        knownMask_ != other.knownMask_ || zMask_ != other.zMask_)
      return false;
    return ((value_ ^ other.value_) & knownMask_) == Mask{0};
  }

private:
  FourState(Mask value, Mask knownMask, Mask zMask)
      : value_(value), knownMask_(knownMask), zMask_(zMask) {
    if (!invariantHolds())
      throw std::invalid_argument(
          "four-state masks require (known & z) == 0");
  }

  Mask value_{};
  Mask knownMask_{};
  Mask zMask_{};
};

} // namespace pyc::cpp
