// SPDX-License-Identifier: BSD-3-Clause
#pragma once

#include "gfsim/byte_mem.h"
#include "gfsim/dff.h"
#include "gfsim/fifo.h"
#include "gfsim/sync_mem.h"

#include <array>
#include <bit>
#include <cstdint>
#include <optional>
#include <tuple>

namespace gfsim::behavior {

using Level = wire<Bits<1>>;

inline Level level(bool value) noexcept {
  return Level::known(Bits<1>{value});
}

inline bool isOne(const Level &value) noexcept {
  return value.isFullyKnown() && value.value().toBool();
}

// Behavioral owners query Current directly and prepare all transfers after the
// caller has resolved both sides. Only the enclosing sample may commit them.
template <class T, std::size_t Depth, QueueReadyPolicy Policy, std::uint64_t Latency>
class Fifo {
  using Kernel = fifo_kernel<T, Depth, Policy, Latency>;

public:
  Level ready(const Level &recipient) const noexcept {
    return Kernel::readReady(current_, recipient);
  }
  Level valid() const noexcept { return Kernel::readValid(current_); }
  wire<T> data() const noexcept { return Kernel::readData(current_); }

  void prepare(const Level &clock, const Level &reset, const Level &offered,
               const wire<T> &value, const Level &recipient) {
    Kernel::work(current_, {clock, reset, offered, value, recipient}, pending_,
                 outputs());
  }
  void reset() noexcept {
    Kernel::reset(current_, {Level{}, Level{}, Level{}, wire<T>{}, Level{}},
                  pending_);
  }
  void commit() noexcept { Kernel::xfer(current_, pending_, outputs()); }
  void discard() noexcept { Kernel::discard(current_, pending_); }

private:
  typename Kernel::Outputs outputs() noexcept {
    return {unusedReady_, unusedValid_, unusedData_};
  }
  typename Kernel::Current current_;
  typename Kernel::Pending pending_;
  Level unusedReady_, unusedValid_;
  wire<T> unusedData_;
};

template <class T, unsigned AddressWidth, std::size_t Depth, std::size_t Ports>
class SyncMemory {
  using Kernel = sync_mem_kernel<T, AddressWidth, Depth, Ports>;
  using Address = wire<Bits<AddressWidth>>;
  using Mask = wire<Bits<Kernel::STRB_WIDTH>>;

public:
  const wire<T> &result(unsigned port) const noexcept { return current_.q[port]; }

  void prepare(const Level &clock, const Level &reset,
               const std::array<Level, Ports> &readEnabled,
               const std::array<Address, Ports> &readAddress,
               const Level &writeEnabled, const Address &writeAddress,
               const wire<T> &data, const Mask &mask) {
    typename Kernel::Inputs inputs{clock, reset, {}, {}, writeEnabled,
                                  writeAddress, data, mask};
    for (std::size_t port = 0; port < Ports; ++port) {
      inputs.ren[port] = &readEnabled[port];
      inputs.raddr[port] = &readAddress[port];
    }
    Kernel::work(current_, inputs, pending_, {});
  }
  void reset() noexcept {
    Kernel::reset(current_, {Level{}, Level{}, {}, {}, Level{}, Address{},
                             wire<T>{}, Mask{}}, pending_);
  }
  void commit() noexcept { Kernel::xfer(current_, pending_, {}); }
  void discard() noexcept { Kernel::discard(current_, pending_); }

private:
  typename Kernel::Current current_;
  typename Kernel::Pending pending_;
};

template <class T, unsigned AddressWidth, std::size_t Depth>
class ByteMemory {
  using Kernel = byte_mem_kernel<T, AddressWidth, Depth>;
  using Address = wire<Bits<AddressWidth>>;
  using Mask = wire<Bits<Kernel::STRB_WIDTH>>;

public:
  wire<T> read(const Address &address) const noexcept {
    wire<T> data;
    Kernel::read(current_, address, {data});
    return data;
  }
  void prepare(const Level &clock, const Level &reset, const Address &address,
               const Level &writeEnabled, const Address &writeAddress,
               const wire<T> &data, const Mask &mask) {
    Kernel::prepare(current_, {clock, reset, address, writeEnabled, writeAddress,
                               data, mask}, pending_);
  }
  void reset() noexcept {
    Kernel::reset(current_, {Level{}, Level{}, Address{}, Level{}, Address{},
                             wire<T>{}, Mask{}}, pending_);
  }
  void commit() noexcept {
    wire<T> unused;
    Kernel::xfer(current_, pending_, {unused});
  }
  void discard() noexcept { Kernel::discard(current_, pending_); }

private:
  typename Kernel::Current current_;
  typename Kernel::Pending pending_;
};

// All logical state in a verified domain shares committed edge history.
// Clock-only samples still prepare their history for the enclosing Xfer.
class SampledDomain {
public:
  SampledClockEvaluation evaluate(const Level &clock,
                                  const Level &reset) const noexcept {
    return evaluateSampledClock(clock_, clock, reset);
  }

  void prepare(const SampledClockEvaluation &sample) {
    detail::checkDffeClock(sample);
    nextClock_ = sample.nextClock;
    writesState_ = sample.rising;
    valid_ = true;
  }

  void reset() noexcept {
    nextClock_ = false;
    writesState_ = true;
    valid_ = true;
  }

  bool commit() noexcept {
    if (!valid_)
      return false;
    clock_ = nextClock_;
    valid_ = false;
    return writesState_;
  }

  void discard() noexcept { valid_ = false; }

private:
  bool clock_ = false;
  bool nextClock_ = false;
  bool writesState_ = false;
  bool valid_ = false;
};

// A value proposal is read only when the domain commits a sample with writes.
// Every such sample replaces all of its owner's proposals, including holds.
template <class T> class State {
public:
  const wire<T> &value() const noexcept { return current_; }

  void prepare(const Level &enabled, const std::optional<wire<T>> &candidate) {
    if (detail::dffeEnabled(enabled))
      pending_ = candidate;
    else
      hold();
  }

  void hold() noexcept { pending_.reset(); }
  void reset(const wire<T> &initial) noexcept { pending_ = initial; }

  void commit() noexcept {
    if (pending_)
      current_ = *pending_;
  }

private:
  wire<T> current_;
  std::optional<wire<T>> pending_;
};

template <std::size_t Size, class T>
wire<table<T, Size>> splat(const wire<T> &element) noexcept {
  wire<table<T, Size>> result;
  for (std::size_t row = 0; row < Size; ++row)
    result.element(row) = element;
  return result;
}

template <std::size_t Size, unsigned IndexWidth>
FourState<1> inRange(const wire<Bits<IndexWidth>> &index) noexcept {
  constexpr auto extentWidth = std::bit_width(Size);
  constexpr unsigned width = IndexWidth > extentWidth ? IndexWidth : extentWidth;
  return compare<ComparePredicate::ULT>(
      zero_extend<width>(index.packed()), FourState<width>::known(Bits<width>{Size}));
}

template <class T, std::size_t Size, unsigned IndexWidth>
std::tuple<wire<T>, Level> lookup(const wire<table<T, Size>> &rows,
                                 const wire<Bits<IndexWidth>> &index) noexcept {
  auto range = Level::fromPacked(inRange<Size>(index));
  if (isOne(range))
    return {rows.element(index.value().value()), range};
  return {wire<T>::unknown(), range};
}

template <class T, std::size_t Size, unsigned IndexWidth>
void replace(wire<table<T, Size>> &rows, const wire<Bits<IndexWidth>> &index,
             const wire<T> &value) noexcept {
  if (index.isFullyKnown()) {
    if (isOne(Level::fromPacked(inRange<Size>(index))))
      rows.element(index.value().value()) = value;
    return;
  }
  constexpr auto extentWidth = std::bit_width(Size);
  constexpr unsigned width = IndexWidth > extentWidth ? IndexWidth : extentWidth;
  const auto key = zero_extend<width>(index.packed());
  for (std::size_t row = 0; row < Size; ++row) {
    const auto equal = compare<ComparePredicate::EQ>(
        key, FourState<width>::known(Bits<width>{row}));
    auto &old = rows.element(row);
    old = wire<T>::fromPacked(select(equal, value.packed(), old.packed()));
  }
}

} // namespace gfsim::behavior
