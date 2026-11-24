// SPDX-License-Identifier: BSD-3-Clause
#pragma once

#include "gfsim/dff.h"
#include "gfsim/fifo.h"

#include <bit>
#include <cstdint>
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

template <class T> class State {
  using Kernel = dffe_kernel<T>;

public:
  const wire<T> &value() const noexcept { return current_.q; }

  void prepare(const Level &clock, const Level &reset, const Level &enabled,
               const wire<T> &candidate, const wire<T> &initial) {
    Kernel::work(current_, {clock, reset, enabled, candidate, initial}, pending_,
                 {current_.q});
  }
  void reset(const wire<T> &initial) noexcept {
    Kernel::reset(current_, {Level{}, Level{}, Level{}, current_.q, initial},
                  pending_);
  }
  void commit() noexcept { Kernel::xfer(current_, pending_, {current_.q}); }
  void discard() noexcept { Kernel::discard(current_, pending_); }

private:
  typename Kernel::Current current_;
  typename Kernel::Pending pending_;
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
