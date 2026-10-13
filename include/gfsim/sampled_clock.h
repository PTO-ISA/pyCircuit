// SPDX-License-Identifier: BSD-3-Clause
#pragma once

#include "gfsim/wire.h"

namespace gfsim {

enum class SampledClockFault { None, UnknownClock, UnknownReset };

struct SampledClockEvaluation {
  bool nextClock;
  bool rising;
  bool resetActive;
  SampledClockFault fault;
};

// Evaluate borrowed pins without changing committed or pending history. A fault
// retains the previous clock; callers must not commit a failed sample.
inline SampledClockEvaluation
evaluateSampledClock(bool previous, const wire<Bits<1>> &clock,
                     const wire<Bits<1>> &reset) noexcept {
  if (!clock.isFullyKnown())
    return {previous, false, false, SampledClockFault::UnknownClock};
  const bool next = clock.value().toBool();
  if (previous || !next)
    return {next, false, false, SampledClockFault::None};
  if (!reset.isFullyKnown())
    return {previous, true, false, SampledClockFault::UnknownReset};
  return {next, true, reset.value().toBool(), SampledClockFault::None};
}

} // namespace gfsim
