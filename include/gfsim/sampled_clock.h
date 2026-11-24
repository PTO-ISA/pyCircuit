// SPDX-License-Identifier: BSD-3-Clause
#pragma once

#include "gfsim/wire.h"

namespace gfsim {

enum class ResetSemantics { Strict, IfOne };

// Root-owned interpretation shared by sampled checks and primitive inputs.
// Checking knownness first makes hidden value bits under X/Z irrelevant.
inline FourState<1>
interpretReset(const FourState<1> &raw,
               ResetSemantics semantics = ResetSemantics::Strict) noexcept {
  if (semantics == ResetSemantics::Strict)
    return raw;
  return FourState<1>::known(
      Bits<1>(raw.isFullyKnown() && raw.value().toBool() ? 1 : 0));
}
inline wire<Bits<1>>
interpretReset(const wire<Bits<1>> &raw,
               ResetSemantics semantics = ResetSemantics::Strict) noexcept {
  return wire<Bits<1>>::fromPacked(interpretReset(raw.packed(), semantics));
}

enum class SampledClockFault { None, UnknownClock, UnknownReset };

struct SampledClockEvaluation {
  bool nextClock;
  bool rising;
  bool resetActive;
  SampledClockFault fault;
};

// Evaluate borrowed pins without changing committed or pending history. A fault
// retains the previous clock; callers must not commit a failed sample.
inline SampledClockEvaluation evaluateSampledClock(
    bool previous, const wire<Bits<1>> &clock, const wire<Bits<1>> &rawReset,
    ResetSemantics semantics = ResetSemantics::Strict) noexcept {
  if (!clock.isFullyKnown())
    return {previous, false, false, SampledClockFault::UnknownClock};
  const bool next = clock.value().toBool();
  if (previous || !next)
    return {next, false, false, SampledClockFault::None};
  const auto reset = interpretReset(rawReset, semantics);
  if (!reset.isFullyKnown())
    return {previous, true, false, SampledClockFault::UnknownReset};
  return {next, true, reset.value().toBool(), SampledClockFault::None};
}

// A generated System owns its input-domain history even when its design
// has no storage or resource consuming that domain.
class SystemSampledClock {
public:
  void prepare(const wire<Bits<1>> &clock, const wire<Bits<1>> &reset,
               ResetSemantics semantics) {
    const auto sample = evaluateSampledClock(clock_, clock, reset, semantics);
    if (sample.fault == SampledClockFault::UnknownClock)
      throw FourStateViolation("System clock must be known");
    if (sample.fault == SampledClockFault::UnknownReset)
      throw FourStateViolation("System reset must be known at posedge");
    nextClock_ = sample.nextClock;
  }
  void reset() noexcept { nextClock_ = false; }
  void discard() noexcept { nextClock_ = clock_; }
  void commit() noexcept { clock_ = nextClock_; }

private:
  bool clock_ = false;
  bool nextClock_ = false;
};

} // namespace gfsim
