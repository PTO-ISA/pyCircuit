// SPDX-License-Identifier: BSD-3-Clause
#pragma once

#include "gfsim/wire.h"

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace gfsim {

enum class QueueReadyPolicy { LocalOccupancy, DownstreamPop };

// One queue lane. Outputs read committed state; Work only prepares a
// transaction for the existing collection_storage completion/Discard/Xfer
// barriers.
template <class T, std::size_t Depth, QueueReadyPolicy Policy,
          std::uint64_t AvailabilityLatency>
struct fifo_kernel {
  static_assert(Depth > 0, "FIFO depth must be positive");
  static_assert(AvailabilityLatency > 0, "FIFO latency must be positive");
  static_assert(Policy == QueueReadyPolicy::LocalOccupancy ||
                    Policy == QueueReadyPolicy::DownstreamPop,
                "unsupported FIFO ready policy");

  struct EmptyTiming {};
  struct DelayedTiming {
    std::array<std::uint64_t, Depth> deadline{};
    std::uint64_t tick = 0;
    std::size_t mature_ptr = 0, eligible_count = 0;
  };
  using Timing =
      std::conditional_t<(AvailabilityLatency > 1), DelayedTiming, EmptyTiming>;
  static constexpr unsigned TimestampWidth =
      std::bit_width(AvailabilityLatency - 1);

  struct Current {
    std::array<wire<T>, Depth> storage;
    std::size_t rd = 0, wr = 0, count = 0;
    bool initialized = false, clock = false;
    [[no_unique_address]] Timing timing;
  };
  struct Pending {
    wire<T> token;
    bool valid = false, clock = false, reset = false;
    bool push = false, pop = false;
    bool advance_time = false, mature = false;
  };
  struct Inputs {
    const wire<Bits<1>> &clk, &rst, &in_valid;
    const wire<T> &in_data;
    const wire<Bits<1>> &out_ready;
  };
  struct Outputs {
    wire<Bits<1>> &in_ready, &out_valid;
    wire<T> &out_data;
  };

  static constexpr std::uint64_t wrapTick(std::uint64_t value) noexcept {
    if constexpr (TimestampWidth == 64)
      return value;
    else
      return value & ((std::uint64_t{1} << TimestampWidth) - 1);
  }

  static std::size_t eligibleCount(const Current &current) noexcept {
    if constexpr (AvailabilityLatency == 1)
      return current.count;
    else
      return current.timing.eligible_count;
  }

  static wire<Bits<1>> readValid(const Current &current) noexcept {
    if (!current.initialized)
      return wire<Bits<1>>::unknown();
    return wire<Bits<1>>::known(Bits<1>{eligibleCount(current) != 0});
  }

  static wire<T> readData(const Current &current) noexcept {
    if (!current.initialized)
      return wire<T>::unknown();
    if (eligibleCount(current) != 0)
      return current.storage[current.rd];
    using Packed = typename wire<T>::packed_type;
    return wire<T>::fromPacked(Packed::known(Bits<wire<T>::width>{}));
  }

  static wire<Bits<1>> readReady(const Current &current,
                                 const wire<Bits<1>> &outReady) noexcept {
    if (!current.initialized)
      return wire<Bits<1>>::unknown();
    const auto capacity = FourState<1>::known(Bits<1>{current.count < Depth});
    if constexpr (Policy == QueueReadyPolicy::LocalOccupancy)
      return wire<Bits<1>>::fromPacked(capacity);
    else
      return wire<Bits<1>>::fromPacked(bit_or(
          capacity, bit_and(readValid(current).packed(), outReady.packed())));
  }

  static void discard(const Current &current, Pending &pending) noexcept {
    pending.valid = false;
    pending.reset = false;
    pending.push = false;
    pending.pop = false;
    pending.advance_time = false;
    pending.mature = false;
    pending.clock = current.clock;
  }

  static void work(const Current &current, Inputs inputs, Pending &pending,
                   Outputs) {
    discard(current, pending);
    try {
      if (!inputs.clk.isFullyKnown())
        throw FourStateViolation("fifo clock must be known");
      pending.clock = inputs.clk.value().toBool();
      pending.valid = true;
      if (current.clock || !pending.clock)
        return;
      if (!inputs.rst.isFullyKnown())
        throw FourStateViolation("fifo reset must be known at posedge");
      if (inputs.rst.value().toBool()) {
        pending.reset = true;
        return;
      }
      const auto pop =
          bit_and(readValid(current).packed(), inputs.out_ready.packed());
      const auto push = bit_and(inputs.in_valid.packed(),
                                readReady(current, inputs.out_ready).packed());
      if (!push.isFullyKnown() || !pop.isFullyKnown())
        throw FourStateViolation("fifo effective transfers must be known");
      pending.push = push.value().toBool();
      pending.pop = pop.value().toBool();
      if (pending.push)
        pending.token = inputs.in_data;
      if constexpr (AvailabilityLatency > 1) {
        if (current.initialized) {
          const auto &timing = current.timing;
          const auto nextTick = wrapTick(timing.tick + std::uint64_t{1});
          pending.advance_time = true;
          // Only the delayed suffix uses deadlines. Eligibility persists
          // through arbitrary timestamp wraps and backpressure.
          pending.mature = current.count > timing.eligible_count &&
                           timing.deadline[timing.mature_ptr] == nextTick;
        }
      }
    } catch (...) {
      discard(current, pending);
      throw;
    }
  }

  static void xfer(Current &current, Pending &pending, Outputs) noexcept {
    if (!pending.valid)
      return;
    current.clock = pending.clock;
    if (pending.reset) {
      current.initialized = true;
      current.rd = current.wr = current.count = 0;
      if constexpr (AvailabilityLatency > 1) {
        auto &timing = current.timing;
        timing.tick = 0;
        timing.mature_ptr = timing.eligible_count = 0;
      }
    } else if (current.initialized) {
      if constexpr (AvailabilityLatency > 1) {
        auto &timing = current.timing;
        if (pending.advance_time)
          timing.tick = wrapTick(timing.tick + std::uint64_t{1});
        if (pending.mature)
          timing.mature_ptr =
              timing.mature_ptr == Depth - 1 ? 0 : timing.mature_ptr + 1;
        // Pop was decided from old eligibility, so same-edge maturity
        // cannot enable a pop. Opposite changes cancel without underflow.
        if (pending.mature != pending.pop) {
          if (pending.mature)
            ++timing.eligible_count;
          else
            --timing.eligible_count;
        }
        if (pending.push)
          timing.deadline[current.wr] =
              wrapTick(timing.tick + (AvailabilityLatency - 1));
      }
      if (pending.push) {
        current.storage[current.wr] = pending.token;
        current.wr = current.wr == Depth - 1 ? 0 : current.wr + 1;
      }
      if (pending.pop)
        current.rd = current.rd == Depth - 1 ? 0 : current.rd + 1;
      if (pending.push != pending.pop) {
        if (pending.push)
          ++current.count;
        else
          --current.count;
      }
    }
    discard(current, pending);
  }

  static void reset(const Current &current, Inputs, Pending &pending) noexcept {
    discard(current, pending);
    pending.valid = true;
    pending.reset = true;
    pending.clock = false;
  }
};

} // namespace gfsim
