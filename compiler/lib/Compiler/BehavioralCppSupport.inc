// SPDX-License-Identifier: BSD-3-Clause
// Compiler-owned adapters emitted into the structured GFIR C++ support header.
constexpr llvm::StringLiteral behavioralValueSupport = R"cpp(#include <cstddef>

namespace pyc_behavior {

using gfsim::Bits;
using gfsim::FourState;
using gfsim::table;
using gfsim::wire;

template <unsigned Out, unsigned In>
Bits<Out> sliceWords(const Bits<In> &value, unsigned low) noexcept {
  Bits<Out> result{};
  const unsigned first = low / 64;
  const unsigned shift = low % 64;
  for (unsigned i = 0; i < Bits<Out>::kWords; ++i) {
    auto word = value.word(first + i) >> shift;
    if (shift)
      word |= value.word(first + i + 1) << (64 - shift);
    result.setWord(i, word);
  }
  return result;
}

template <unsigned Out, unsigned In>
FourState<Out> slice(const FourState<In> &value, unsigned low) noexcept {
  if constexpr (Out > In) {
    return FourState<Out>::unknown();
  } else if constexpr (Out == In) {
    return low == 0 ? value : FourState<Out>::unknown();
  } else if constexpr (In <= 64) {
    return gfsim::extract<Out>(value, low);
  } else {
    if (low > In - Out)
      return FourState<Out>::unknown();
    return FourState<Out>::fromMasks(sliceWords<Out>(value.value(), low),
                                     sliceWords<Out>(value.knownMask(), low),
                                     sliceWords<Out>(value.zMask(), low));
  }
}

template <unsigned... Width>
auto concatenate(const FourState<Width> &...fields) noexcept {
  constexpr unsigned total = (Width + ...);
  if constexpr (sizeof...(Width) == 2 && total <= 64) {
    return gfsim::concat(fields...);
  } else {
    auto result = FourState<total>::unknown();
    unsigned low = total;
    (result.assignSlice(low -= Width, fields), ...);
    return result;
  }
}

template <class T> const auto &pack(const wire<T> &value) noexcept {
  return value.packed();
}

template <class T, std::size_t... Extents>
auto pack(const wire<table<T, Extents...>> &rows) noexcept {
  using Table = wire<table<T, Extents...>>;
  if constexpr (Table::width <= 64) {
    return rows.packed();
  } else {
    auto result = Table::packed_type::unknown();
    constexpr unsigned width = wire<T>::width;
    for (std::size_t i = 0; i < Table::size; ++i)
      result.assignSlice((Table::size - 1 - i) * width, pack(rows.element(i)));
    return result;
  }
}

template <class Wire>
Wire unpack(const typename Wire::packed_type &value) noexcept {
  if constexpr (requires { typename Wire::element_type; } && Wire::width > 64) {
    using Lane = wire<typename Wire::element_type>;
    Wire result;
    for (std::size_t i = 0; i < Wire::size; ++i)
      result.element(i) =
          unpack<Lane>(slice<Lane::width>(value, (Wire::size - 1 - i) * Lane::width));
    return result;
  } else {
    return Wire::fromPacked(value);
  }
}

} // namespace pyc_behavior
)cpp";

constexpr llvm::StringLiteral behavioralCppSupport = R"cpp(#include <gfsim/byte_mem.h>
#include <gfsim/fifo.h>
#include <gfsim/sync_mem.h>

#include <array>
#include <bit>
#include <cstdint>
#include <optional>

namespace pyc_behavior {

using Level = wire<Bits<1>>;

template <unsigned W>
bool sameMasks(const FourState<W> &left, const FourState<W> &right) noexcept {
  return left.value() == right.value() && left.knownMask() == right.knownMask() &&
         left.zMask() == right.zMask();
}

inline Level level(bool value) noexcept {
  return Level::known(Bits<1>{value});
}

inline bool isOne(const Level &value) noexcept {
  return value.isFullyKnown() && value.value().toBool();
}

// Behavioral owners query Current directly and prepare all transfers after the
// caller has resolved both sides. Only the enclosing sample may commit them.
template <class T, std::size_t Depth, gfsim::QueueReadyPolicy Policy,
          std::uint64_t Latency>
class Fifo {
  using Kernel = gfsim::fifo_kernel<T, Depth, Policy, Latency>;

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
  using Kernel = gfsim::sync_mem_kernel<T, AddressWidth, Depth, Ports>;
  using Address = wire<Bits<AddressWidth>>;
  using Mask = wire<Bits<Kernel::STRB_WIDTH>>;

public:
  const wire<T> &result(unsigned port) const noexcept { return responses_[port]; }
  Level live(unsigned port) const noexcept { return level(current_.live[port]); }

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
    Kernel::work(current_, inputs, pending_, outputs());
  }
  void reset() noexcept {
    Kernel::reset(current_, {Level{}, Level{}, {}, {}, Level{}, Address{},
                             wire<T>{}, Mask{}}, pending_);
  }
  void commit() noexcept { Kernel::xfer(current_, pending_, outputs()); }
  void discard() noexcept { Kernel::discard(current_, pending_); }

private:
  typename Kernel::Outputs outputs() noexcept {
    typename Kernel::Outputs pins{};
    for (std::size_t port = 0; port < Ports; ++port)
      pins.rdata[port] = &responses_[port];
    return pins;
  }

  typename Kernel::Current current_;
  typename Kernel::Pending pending_;
  std::array<wire<T>, Ports> responses_;
};

template <class T, unsigned AddressWidth, std::size_t Depth>
class ByteMemory {
  using Kernel = gfsim::byte_mem_kernel<T, AddressWidth, Depth>;
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
    wire<T> unused;
    Kernel::work(current_, {clock, reset, address, writeEnabled, writeAddress,
                            data, mask}, pending_, {unused});
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
  gfsim::SampledClockEvaluation evaluate(const Level &clock,
                                         const Level &reset) const noexcept {
    return gfsim::evaluateSampledClock(clock_, clock, reset);
  }

  void prepare(const gfsim::SampledClockEvaluation &sample) {
    if (sample.fault == gfsim::SampledClockFault::UnknownClock)
      throw gfsim::FourStateViolation("dffe clock must be known");
    if (sample.fault == gfsim::SampledClockFault::UnknownReset)
      throw gfsim::FourStateViolation("dffe reset must be known at posedge");
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
    if (!enabled.isFullyKnown())
      throw gfsim::FourStateViolation("dffe enable must be known at posedge");
    if (isOne(enabled))
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
  return gfsim::compare<gfsim::ComparePredicate::ULT>(
      gfsim::zero_extend<width>(index.packed()), FourState<width>::known(Bits<width>{Size}));
}

template <class T, std::size_t Size, unsigned IndexWidth>
std::tuple<wire<T>, Level> lookup(const wire<table<T, Size>> &rows,
                                 const wire<Bits<IndexWidth>> &index) noexcept {
  auto range = Level::fromPacked(inRange<Size>(index));
  if (isOne(range))
    return {rows.element(index.value().value()), range};
  return {wire<T>::unknown(), range};
}

template <class Table, unsigned Width, unsigned IndexWidth>
auto lookupPacked(const FourState<Width> &value, unsigned low,
                  const wire<Bits<IndexWidth>> &index) noexcept {
  using Lane = wire<typename Table::element_type>;
  auto range = Level::fromPacked(inRange<Table::size>(index));
  if (!isOne(range))
    return std::tuple{Lane::unknown(), range};
  unsigned offset = (Table::size - 1 - index.value().value()) * Lane::width;
  return std::tuple{unpack<Lane>(slice<Lane::width>(value, low + offset)), range};
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
  const auto key = gfsim::zero_extend<width>(index.packed());
  for (std::size_t row = 0; row < Size; ++row) {
    const auto equal = gfsim::compare<gfsim::ComparePredicate::EQ>(
        key, FourState<width>::known(Bits<width>{row}));
    auto &old = rows.element(row);
    old = wire<T>::fromPacked(gfsim::select(equal, value.packed(), old.packed()));
  }
}

} // namespace pyc_behavior
)cpp";
