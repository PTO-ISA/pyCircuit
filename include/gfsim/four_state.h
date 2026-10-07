// SPDX-License-Identifier: BSD-3-Clause
// Reused hardware value representation from the existing primitive library.
#pragma once

#include "gfsim/bits.h"

#include <stdexcept>

namespace gfsim {

class FourStateViolation final : public std::runtime_error {
public:
  using std::runtime_error::runtime_error;
};

template <unsigned Width> class FourState final {
public:
  using Mask = Bits<Width>;

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
  bool invariantHolds() const { return (knownMask_ & zMask_) == Mask{0}; }

  // Update only the words touched by a verified field assignment. This is a
  // value-buffer operation, never a write to a leaf's committed state.
  template <unsigned N>
  void assignSlice(unsigned low, const FourState<N> &source) noexcept {
    static_assert(N <= Width);
    if (low > Width - N)
      return;
    unsigned position = 0;
    while (position < N) {
      unsigned destinationBit = low + position;
      unsigned destinationWord = destinationBit / 64;
      unsigned destinationOffset = destinationBit % 64;
      unsigned count = N - position;
      if (count > 64 - destinationOffset)
        count = 64 - destinationOffset;
      const std::uint64_t bits =
          count == 64 ? ~std::uint64_t{0} : (std::uint64_t{1} << count) - 1;
      const std::uint64_t mask = bits << destinationOffset;
      auto update = [&](Mask &target, const auto &input) {
        unsigned sourceOffset = position % 64;
        auto word = input.word(position / 64) >> sourceOffset;
        if (sourceOffset && sourceOffset + count > 64)
          word |= input.word(position / 64 + 1) << (64 - sourceOffset);
        target.setWord(destinationWord,
                       (target.word(destinationWord) & ~mask) |
                           ((word & bits) << destinationOffset));
      };
      update(value_, source.value());
      update(knownMask_, source.knownMask());
      update(zMask_, source.zMask());
      position += count;
    }
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
      throw std::invalid_argument("four-state masks require (known & z) == 0");
  }

  Mask value_{};
  Mask knownMask_{};
  Mask zMask_{};
};

// Finite hardware operators. They preserve knownness/Z where Verilog does,
// and never use unchecked host shifts, arithmetic conversions or division.
// Arithmetic widths/truncation are chosen by verified ACIR, not by these
// helpers.
template <unsigned W> FourState<W> bit_not(const FourState<W> &value) noexcept {
  return FourState<W>::fromMasks(~value.value(), value.knownMask(), Bits<W>{0});
}

template <unsigned W>
FourState<W> bit_and(const FourState<W> &lhs,
                     const FourState<W> &rhs) noexcept {
  const auto zero =
      (lhs.knownMask() & ~lhs.value()) | (rhs.knownMask() & ~rhs.value());
  const auto one =
      lhs.knownMask() & lhs.value() & rhs.knownMask() & rhs.value();
  return FourState<W>::fromMasks(lhs.value() & rhs.value(), zero | one,
                                 Bits<W>{0});
}

template <unsigned W>
FourState<W> bit_or(const FourState<W> &lhs, const FourState<W> &rhs) noexcept {
  const auto one =
      (lhs.knownMask() & lhs.value()) | (rhs.knownMask() & rhs.value());
  const auto zero =
      lhs.knownMask() & ~lhs.value() & rhs.knownMask() & ~rhs.value();
  return FourState<W>::fromMasks(lhs.value() | rhs.value(), zero | one,
                                 Bits<W>{0});
}

template <unsigned W>
FourState<W> bit_xor(const FourState<W> &lhs,
                     const FourState<W> &rhs) noexcept {
  return FourState<W>::fromMasks(lhs.value() ^ rhs.value(),
                                 lhs.knownMask() & rhs.knownMask(), Bits<W>{0});
}

template <unsigned W>
FourState<W> add(const FourState<W> &lhs, const FourState<W> &rhs) noexcept {
  if (!lhs.isFullyKnown() || !rhs.isFullyKnown())
    return FourState<W>::unknown();
  return FourState<W>::known(lhs.value() + rhs.value());
}

template <unsigned W>
FourState<W> sub(const FourState<W> &lhs, const FourState<W> &rhs) noexcept {
  if (!lhs.isFullyKnown() || !rhs.isFullyKnown())
    return FourState<W>::unknown();
  return FourState<W>::known(lhs.value() - rhs.value());
}

template <unsigned W>
FourState<W> mul(const FourState<W> &lhs, const FourState<W> &rhs) noexcept {
  if (!lhs.isFullyKnown() || !rhs.isFullyKnown())
    return FourState<W>::unknown();
  return FourState<W>::known(lhs.value() * rhs.value());
}

template <unsigned W>
FourState<W> udiv(const FourState<W> &lhs, const FourState<W> &rhs) noexcept {
  if (!lhs.isFullyKnown() || !rhs.isFullyKnown() || rhs.value() == Bits<W>{0})
    return FourState<W>::unknown();
  return FourState<W>::known(udiv<W>(lhs.value(), rhs.value()));
}

template <unsigned W>
FourState<W> urem(const FourState<W> &lhs, const FourState<W> &rhs) noexcept {
  if (!lhs.isFullyKnown() || !rhs.isFullyKnown() || rhs.value() == Bits<W>{0})
    return FourState<W>::unknown();
  return FourState<W>::known(urem<W>(lhs.value(), rhs.value()));
}

enum class ComparePredicate { EQ, NE, ULT, ULE, UGT, UGE, SLT, SLE, SGT, SGE };

template <ComparePredicate Predicate, unsigned W>
FourState<1> compare(const FourState<W> &lhs,
                     const FourState<W> &rhs) noexcept {
  if constexpr (Predicate == ComparePredicate::EQ ||
                Predicate == ComparePredicate::NE) {
    const auto difference =
        (lhs.value() ^ rhs.value()) & lhs.knownMask() & rhs.knownMask();
    if (difference != Bits<W>{0})
      return FourState<1>::known(
          Bits<1>{Predicate == ComparePredicate::NE ? 1u : 0u});
    if (!lhs.isFullyKnown() || !rhs.isFullyKnown())
      return FourState<1>::unknown();
    return FourState<1>::known(
        Bits<1>{Predicate == ComparePredicate::EQ ? 1u : 0u});
  }
  if (!lhs.isFullyKnown() || !rhs.isFullyKnown())
    return FourState<1>::unknown();
  bool result = false;
  if constexpr (Predicate == ComparePredicate::ULT)
    result = lhs.value() < rhs.value();
  else if constexpr (Predicate == ComparePredicate::ULE)
    result = lhs.value() <= rhs.value();
  else if constexpr (Predicate == ComparePredicate::UGT)
    result = lhs.value() > rhs.value();
  else if constexpr (Predicate == ComparePredicate::UGE)
    result = lhs.value() >= rhs.value();
  else if constexpr (Predicate == ComparePredicate::SLT)
    result = slt(lhs.value(), rhs.value());
  else if constexpr (Predicate == ComparePredicate::SLE)
    result = !slt(rhs.value(), lhs.value());
  else if constexpr (Predicate == ComparePredicate::SGT)
    result = slt(rhs.value(), lhs.value());
  else if constexpr (Predicate == ComparePredicate::SGE)
    result = !slt(lhs.value(), rhs.value());
  return FourState<1>::known(Bits<1>{result ? 1u : 0u});
}

template <unsigned W>
FourState<W> select(const FourState<1> &condition, const FourState<W> &whenTrue,
                    const FourState<W> &whenFalse) noexcept {
  if (condition.isFullyKnown())
    return condition.value().toBool() ? whenTrue : whenFalse;
  const auto commonKnown = whenTrue.knownMask() & whenFalse.knownMask() &
                           ~(whenTrue.value() ^ whenFalse.value());
  const auto commonZ = whenTrue.zMask() & whenFalse.zMask();
  return FourState<W>::fromMasks(whenTrue.value(), commonKnown, commonZ);
}

template <unsigned Out, unsigned In>
FourState<Out> zero_extend(const FourState<In> &value) noexcept {
  const auto inputMask = zext<Out, In>(Bits<In>::ones());
  return FourState<Out>::fromMasks(zext<Out, In>(value.value()),
                                   zext<Out, In>(value.knownMask()) |
                                       ~inputMask,
                                   zext<Out, In>(value.zMask()));
}

template <unsigned Out, unsigned In>
FourState<Out> truncate(const FourState<In> &value) noexcept {
  return FourState<Out>::fromMasks(trunc<Out, In>(value.value()),
                                   trunc<Out, In>(value.knownMask()),
                                   trunc<Out, In>(value.zMask()));
}

template <unsigned Out, unsigned In>
FourState<Out> sign_extend(const FourState<In> &value) noexcept {
  return FourState<Out>::fromMasks(sext<Out, In>(value.value()),
                                   sext<Out, In>(value.knownMask()),
                                   sext<Out, In>(value.zMask()));
}

template <unsigned Out, unsigned In>
FourState<Out> extract(const FourState<In> &value, unsigned low) noexcept {
  // Invalid static slices are rejected by ACIR. Keep the runtime safe even for
  // a direct caller: no out-of-range host access or arithmetic wraparound.
  if (Out > In || low > In - Out)
    return FourState<Out>::unknown();
  return FourState<Out>::fromMasks(extract<Out, In>(value.value(), low),
                                   extract<Out, In>(value.knownMask(), low),
                                   extract<Out, In>(value.zMask(), low));
}

template <unsigned High, unsigned Low>
FourState<High + Low> concat(const FourState<High> &high,
                             const FourState<Low> &low) noexcept {
  return FourState<High + Low>::fromMasks(
      concat(high.value(), low.value()),
      concat(high.knownMask(), low.knownMask()),
      concat(high.zMask(), low.zMask()));
}

template <unsigned W, unsigned AmountWidth>
unsigned boundedShift(const FourState<AmountWidth> &amount) noexcept {
  for (unsigned word = 1; word < Bits<AmountWidth>::kWords; ++word)
    if (amount.value().word(word) != 0)
      return W;
  if (amount.value().value() >= W)
    return W;
  return static_cast<unsigned>(amount.value().value());
}

template <unsigned W, unsigned AmountWidth>
FourState<W> shl(const FourState<W> &value,
                 const FourState<AmountWidth> &amount) noexcept {
  if (!amount.isFullyKnown())
    return FourState<W>::unknown();
  const unsigned count = boundedShift<W>(amount);
  const auto fill = ~shl<W>(Bits<W>::ones(), count);
  return FourState<W>::fromMasks(shl<W>(value.value(), count),
                                 shl<W>(value.knownMask(), count) | fill,
                                 shl<W>(value.zMask(), count));
}

template <unsigned W, unsigned AmountWidth>
FourState<W> lshr(const FourState<W> &value,
                  const FourState<AmountWidth> &amount) noexcept {
  if (!amount.isFullyKnown())
    return FourState<W>::unknown();
  const unsigned count = boundedShift<W>(amount);
  const auto fill = ~lshr<W>(Bits<W>::ones(), count);
  return FourState<W>::fromMasks(lshr<W>(value.value(), count),
                                 lshr<W>(value.knownMask(), count) | fill,
                                 lshr<W>(value.zMask(), count));
}

template <unsigned W, unsigned AmountWidth>
FourState<W> ashr(const FourState<W> &value,
                  const FourState<AmountWidth> &amount) noexcept {
  if (!amount.isFullyKnown())
    return FourState<W>::unknown();
  const unsigned count = boundedShift<W>(amount);
  return FourState<W>::fromMasks(ashr<W>(value.value(), count),
                                 ashr<W>(value.knownMask(), count),
                                 ashr<W>(value.zMask(), count));
}

} // namespace gfsim
