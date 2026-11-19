// SPDX-License-Identifier: BSD-3-Clause
#pragma once

#include "gfsim/wire.h"

namespace gfsim {

enum class ResetSemantics { Strict, IfOne };
enum class RegisterEdge { Posedge, Negedge };
enum class RegisterResetKind { None, Sync, Async };
enum class RegisterResetPolarity { High, Low };

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
  // The selected update edge; the legacy field name is retained for callers
  // using the original positive-edge contract.
  bool rising;
  bool resetActive;
  SampledClockFault fault;
};

// Evaluate borrowed pins without changing committed or pending history. A fault
// retains the previous clock; callers must not commit a failed sample.
inline SampledClockEvaluation evaluateSampledClock(
    bool previous, const wire<Bits<1>> &clock, const wire<Bits<1>> &rawReset,
    ResetSemantics semantics = ResetSemantics::Strict,
    RegisterEdge edge = RegisterEdge::Posedge,
    RegisterResetKind resetKind = RegisterResetKind::Sync,
    RegisterResetPolarity polarity = RegisterResetPolarity::High) noexcept {
  if (!clock.isFullyKnown())
    return {previous, false, false, SampledClockFault::UnknownClock};
  const bool next = clock.value().toBool();
  const bool selectedEdge =
      edge == RegisterEdge::Posedge ? (!previous && next) : (previous && !next);
  if (resetKind == RegisterResetKind::None)
    return {next, selectedEdge, false, SampledClockFault::None};
  if (!selectedEdge && resetKind != RegisterResetKind::Async)
    return {next, false, false, SampledClockFault::None};
  // Interpret if_one after polarity normalization: an unknown active-low pin
  // is inactive under if_one, rather than becoming an asserted reset.
  const auto normalized =
      polarity == RegisterResetPolarity::Low
          ? wire<Bits<1>>::fromPacked(bit_not(rawReset.packed()))
          : rawReset;
  const auto reset = interpretReset(normalized, semantics);
  if (!reset.isFullyKnown())
    return {previous, selectedEdge, false, SampledClockFault::UnknownReset};
  return {next, selectedEdge, reset.value().toBool(), SampledClockFault::None};
}

} // namespace gfsim
