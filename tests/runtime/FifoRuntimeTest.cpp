#include "gfsim/collection.h"
#include "gfsim/dff.h"
#include "gfsim/fifo.h"
#include "gtest/gtest.h"

#include <array>
#include <atomic>
#include <cstdlib>
#include <deque>
#include <limits>
#include <new>
#include <type_traits>

namespace {
std::atomic<bool> trackAllocations{false};
std::atomic<std::size_t> allocationCount{0};
struct HeaderA {
  gfsim::Bits<13> tag;
  gfsim::Bits<1> flag;
  HeaderA() noexcept : tag(37), flag(1) {}
};
struct RecordA {
  HeaderA header;
  gfsim::Bits<65> data;
  RecordA() noexcept : data(7) {}
};
struct HeaderB {
  gfsim::Bits<130> address;
  gfsim::Bits<13> size;
};
struct RecordB {
  gfsim::Bits<1> valid;
  HeaderB descriptor;
};
} // namespace
void *operator new(std::size_t size) {
  if (trackAllocations.load(std::memory_order_relaxed))
    allocationCount.fetch_add(1, std::memory_order_relaxed);
  if (auto *p = std::malloc(size))
    return p;
  throw std::bad_alloc();
}
void *operator new[](std::size_t size) { return ::operator new(size); }
void operator delete(void *p) noexcept { std::free(p); }
void operator delete(void *p, std::size_t) noexcept { std::free(p); }
void operator delete[](void *p) noexcept { std::free(p); }
void operator delete[](void *p, std::size_t) noexcept { std::free(p); }
namespace gfsim {
template <>
struct hardware_traits<HeaderA>
    : hardware_struct_traits<HeaderA, &HeaderA::tag, &HeaderA::flag> {};
template <>
struct hardware_traits<RecordA>
    : hardware_struct_traits<RecordA, &RecordA::header, &RecordA::data> {};
template <>
struct hardware_traits<HeaderB>
    : hardware_struct_traits<HeaderB, &HeaderB::address, &HeaderB::size> {};
template <>
struct hardware_traits<RecordB>
    : hardware_struct_traits<RecordB, &RecordB::valid, &RecordB::descriptor> {};
} // namespace gfsim
namespace {
using Control = gfsim::wire<gfsim::Bits<1>>;
using Policy = gfsim::QueueReadyPolicy;
using Table130 = gfsim::table<gfsim::Bits<130>, 3>;
using TableRecord = gfsim::table<RecordA, 3>;
Control control(unsigned value) {
  return Control::known(gfsim::Bits<1>{value});
}
Control unknownControl(bool z, unsigned latent) {
  using FS = gfsim::FourState<1>;
  return Control::fromPacked(z ? FS::highImpedance(gfsim::Bits<1>{latent})
                               : FS::unknown(gfsim::Bits<1>{latent}));
}
template <class T> gfsim::wire<T> zeroToken() {
  using Wire = gfsim::wire<T>;
  return Wire::fromPacked(
      Wire::packed_type::known(gfsim::Bits<Wire::width>{0}));
}
// Input construction is independent of payload layout and FIFO implementation.
// Keep latent value bits behind X/Z; transport must compare all three planes.
template <class T>
gfsim::wire<T> token(unsigned serial, unsigned maskMode = 0) {
  constexpr auto W = gfsim::wire<T>::width;
  gfsim::Bits<W> value, known, z;
  for (unsigned bit = 0; bit != W; ++bit) {
    const auto mask = std::uint64_t{1} << (bit % 64);
    const unsigned state = (bit + serial) % 4;
    if (((bit * 7 + serial * 3) % 11) < 5)
      value.setWord(bit / 64, value.word(bit / 64) | mask);
    if (maskMode == 0 || (maskMode == 1 && state < 2))
      known.setWord(bit / 64, known.word(bit / 64) | mask);
    if (maskMode == 2 || (maskMode == 1 && state == 2))
      z.setWord(bit / 64, z.word(bit / 64) | mask);
  }
  using FS = typename gfsim::wire<T>::packed_type;
  return gfsim::wire<T>::fromPacked(FS::fromMasks(value, known, z));
}
template <class T>
void expectPlanes(const gfsim::wire<T> &actual,
                  const gfsim::wire<T> &expected) {
  EXPECT_EQ(actual.packed().value(), expected.packed().value());
  EXPECT_EQ(actual.packed().knownMask(), expected.packed().knownMask());
  EXPECT_EQ(actual.packed().zMask(), expected.packed().zMask());
}
template <class Kernel>
void expectCurrent(const typename Kernel::Current &a,
                   const typename Kernel::Current &b) {
  EXPECT_EQ(a.initialized, b.initialized);
  EXPECT_EQ(a.clock, b.clock);
  EXPECT_EQ(a.rd, b.rd);
  EXPECT_EQ(a.wr, b.wr);
  EXPECT_EQ(a.count, b.count);
  if constexpr (Kernel::TimestampWidth != 0) {
    EXPECT_EQ(a.timing.tick, b.timing.tick);
    EXPECT_EQ(a.timing.mature_ptr, b.timing.mature_ptr);
    EXPECT_EQ(a.timing.eligible_count, b.timing.eligible_count);
    EXPECT_EQ(a.timing.deadline, b.timing.deadline);
  }
  for (std::size_t i = 0; i != a.storage.size(); ++i)
    expectPlanes(a.storage[i], b.storage[i]);
}
template <class Kernel>
void expectPending(const typename Kernel::Pending &a,
                   const typename Kernel::Pending &b) {
  EXPECT_EQ(a.valid, b.valid);
  EXPECT_EQ(a.clock, b.clock);
  EXPECT_EQ(a.reset, b.reset);
  EXPECT_EQ(a.push, b.push);
  EXPECT_EQ(a.pop, b.pop);
  EXPECT_EQ(a.advance_time, b.advance_time);
  EXPECT_EQ(a.mature, b.mature);
  expectPlanes(a.token, b.token);
}

template <class T, std::size_t Depth, Policy ReadyPolicy,
          std::uint64_t Latency = 1>
struct QueueFixture {
  using Kernel = gfsim::fifo_kernel<T, Depth, ReadyPolicy, Latency>;
  typename Kernel::Current current;
  typename Kernel::Pending pending;
  Control clk = control(0), rst = control(0), valid = control(0),
          take = control(0);
  gfsim::wire<T> data = token<T>(1), outputData = token<T>(99, 1);
  Control outputReady = unknownControl(true, 1),
          outputValid = unknownControl(false, 1);
  typename Kernel::Inputs inputs() const {
    return {clk, rst, valid, data, take};
  }
  typename Kernel::Outputs outputs() {
    return {outputReady, outputValid, outputData};
  }
  void publish() {
    outputReady = Kernel::readReady(current, take);
    outputValid = Kernel::readValid(current);
    outputData = Kernel::readData(current);
  }
  void hostReset() {
    const auto before = current;
    const auto readyBefore = outputReady, validBefore = outputValid;
    const auto dataBefore = outputData;
    Kernel::reset(current, inputs(), pending);
    expectCurrent<Kernel>(current, before);
    expectPlanes(outputReady, readyBefore);
    expectPlanes(outputValid, validBefore);
    expectPlanes(outputData, dataBefore);
    Kernel::xfer(current, pending, outputs());
    expectPlanes(outputReady, readyBefore);
    expectPlanes(outputValid, validBefore);
    expectPlanes(outputData, dataBefore);
  }
  void sample(unsigned clock) {
    clk = control(clock);
    publish();
    Kernel::work(current, inputs(), pending, outputs());
    Kernel::xfer(current, pending, outputs());
  }
};

// Golden state is token order plus edge history, with no rd/wr/count mirror.
template <class T, std::size_t Depth, Policy ReadyPolicy> void exerciseDeque() {
  QueueFixture<T, Depth, ReadyPolicy> queue;
  using Kernel = typename decltype(queue)::Kernel;
  std::deque<gfsim::wire<T>> expected;
  bool previousClock = false;
  queue.hostReset();
  unsigned pushes = 0, pops = 0;
  auto observe = [&] {
    const bool pop = !expected.empty() && queue.take.value().toBool();
    const bool ready = expected.size() < Depth ||
                       (ReadyPolicy == Policy::DownstreamPop && pop);
    expectPlanes(Kernel::readReady(queue.current, queue.take), control(ready));
    expectPlanes(Kernel::readValid(queue.current), control(!expected.empty()));
    expectPlanes(Kernel::readData(queue.current),
                 expected.empty() ? zeroToken<T>() : expected.front());
  };
  auto step = [&](bool clock, bool reset, bool valid, bool take,
                  unsigned serial, unsigned maskMode = 0) {
    queue.clk = control(clock);
    queue.rst = control(reset);
    queue.valid = control(valid);
    queue.take = control(take);
    queue.data = token<T>(serial, maskMode);
    observe();
    queue.publish();
    const auto oldCurrent = queue.current;
    const auto oldReady = queue.outputReady, oldValid = queue.outputValid;
    const auto oldData = queue.outputData;
    const bool edge = !previousClock && clock;
    const bool pop = edge && !reset && !expected.empty() && take;
    const bool push = edge && !reset && valid &&
                      (expected.size() < Depth ||
                       (ReadyPolicy == Policy::DownstreamPop && pop));
    Kernel::work(queue.current, queue.inputs(), queue.pending, queue.outputs());
    expectCurrent<Kernel>(queue.current, oldCurrent);
    expectPlanes(queue.outputReady, oldReady);
    expectPlanes(queue.outputValid, oldValid);
    expectPlanes(queue.outputData, oldData);
    Kernel::xfer(queue.current, queue.pending, queue.outputs());
    expectPlanes(queue.outputReady, oldReady);
    expectPlanes(queue.outputValid, oldValid);
    expectPlanes(queue.outputData, oldData);
    if (edge && reset)
      expected.clear();
    else {
      if (pop) {
        expected.pop_front();
        ++pops;
      }
      if (push) {
        expected.push_back(queue.data);
        ++pushes;
      }
    }
    previousClock = clock;
    observe();
    EXPECT_FALSE(queue.pending.valid);
  };
  // Fill then request a full replacement; the old head is sampled before Xfer.
  for (unsigned i = 0; i != Depth; ++i) {
    step(false, false, true, false, i, i % 4);
    step(true, false, true, false, i, i % 4);
  }
  ASSERT_EQ(expected.size(), Depth);
  step(false, false, true, true, 100, 1);
  step(true, false, true, true, 100, 1);
  EXPECT_EQ(expected.size(),
            ReadyPolicy == Policy::LocalOccupancy ? Depth - 1 : Depth);
  // Alternating requests, held high/low, resets, all-X/Z and latent bits force
  // arbitrary-depth wrap, backpressure and old-state availability repeatedly.
  for (unsigned i = 0; i != 96; ++i) {
    const bool valid = i % 5 != 2, take = i % 7 != 3;
    step(false, false, valid, take, 120 + i, i % 4);
    step(false, false, !valid, !take, 240 + i, (i + 1) % 4);
    step(true, i == 39, valid, take, 120 + i, i % 4);
    step(true, false, true, true, 400 + i, (i + 2) % 4);
  }
  for (unsigned i = 0; i != Depth + 1; ++i) {
    step(false, false, false, true, 0);
    step(true, false, false, true, 0);
  }
  EXPECT_TRUE(expected.empty());
  EXPECT_GT(pushes, Depth * 4);
  EXPECT_GT(pops, Depth * 4);
}
template <class T> void depthPolicyMatrix() {
  exerciseDeque<T, 1, Policy::LocalOccupancy>();
  exerciseDeque<T, 1, Policy::DownstreamPop>();
  exerciseDeque<T, 2, Policy::LocalOccupancy>();
  exerciseDeque<T, 2, Policy::DownstreamPop>();
  exerciseDeque<T, 3, Policy::LocalOccupancy>();
  exerciseDeque<T, 3, Policy::DownstreamPop>();
  exerciseDeque<T, 5, Policy::LocalOccupancy>();
  exerciseDeque<T, 5, Policy::DownstreamPop>();
}
TEST(FifoRuntimeTest, Bits1DequeAllDepthsBothPolicies) {
  depthPolicyMatrix<gfsim::Bits<1>>();
}
TEST(FifoRuntimeTest, Bits13DequeAllDepthsBothPolicies) {
  depthPolicyMatrix<gfsim::Bits<13>>();
}
TEST(FifoRuntimeTest, Bits65DequeAllDepthsBothPolicies) {
  depthPolicyMatrix<gfsim::Bits<65>>();
}
TEST(FifoRuntimeTest, Bits130DequeAllDepthsBothPolicies) {
  depthPolicyMatrix<gfsim::Bits<130>>();
}
TEST(FifoRuntimeTest, NestedRecordADequeAllDepthsBothPolicies) {
  depthPolicyMatrix<RecordA>();
}
TEST(FifoRuntimeTest, UnrelatedNestedRecordBDequeAllDepthsBothPolicies) {
  depthPolicyMatrix<RecordB>();
}
TEST(FifoRuntimeTest, TableThreeU130DequeAllDepthsBothPolicies) {
  depthPolicyMatrix<Table130>();
}
TEST(FifoRuntimeTest, TableThreeNestedRecordDequeAllDepthsBothPolicies) {
  depthPolicyMatrix<TableRecord>();
}

template <Policy P> void coldLifecycle() {
  QueueFixture<RecordA, 3, P> q;
  using Kernel = typename decltype(q)::Kernel;
  q.current.storage[0] = token<RecordA>(41, 1);
  auto expectCold = [&] {
    expectPlanes(Kernel::readValid(q.current), Control::unknown());
    expectPlanes(Kernel::readReady(q.current, q.take), Control::unknown());
    expectPlanes(Kernel::readData(q.current), gfsim::wire<RecordA>::unknown());
    EXPECT_FALSE(q.current.initialized);
  };
  expectCold();
  EXPECT_NE(gfsim::hardware_traits<RecordA>::pack(RecordA{}),
            gfsim::Bits<79>{0});
  // Synchronous reset alone can establish authority from cold state, masking
  // unknown transfers; a staged reset still leaves old outputs untouched.
  QueueFixture<RecordA, 3, P> synchronous;
  synchronous.clk = control(1);
  synchronous.rst = control(1);
  synchronous.valid = unknownControl(false, 1);
  synchronous.take = unknownControl(true, 1);
  const auto staleOutput = synchronous.outputData;
  Kernel::work(synchronous.current, synchronous.inputs(), synchronous.pending,
               synchronous.outputs());
  EXPECT_FALSE(synchronous.current.initialized);
  Kernel::xfer(synchronous.current, synchronous.pending, synchronous.outputs());
  EXPECT_TRUE(synchronous.current.initialized);
  expectPlanes(Kernel::readData(synchronous.current), zeroToken<RecordA>());
  expectPlanes(synchronous.outputData, staleOutput);
  // A known-zero cold transaction changes clock history only, never authority.
  const auto before = q.current;
  q.clk = control(1);
  q.valid = control(0);
  q.take = control(0);
  Kernel::work(q.current, q.inputs(), q.pending, q.outputs());
  EXPECT_TRUE(q.pending.valid);
  EXPECT_FALSE(q.pending.push);
  EXPECT_FALSE(q.pending.pop);
  expectCurrent<Kernel>(q.current, before);
  Kernel::xfer(q.current, q.pending, q.outputs());
  EXPECT_TRUE(q.current.clock);
  EXPECT_FALSE(q.current.initialized);
  expectPlanes(q.current.storage[0], before.storage[0]);
  expectCold();
  // Host reset ignores every input and stages only metadata; abandoning it
  // leaves cold state and clock history intact and does not clear payload RAM.
  q.clk = unknownControl(true, 1);
  q.rst = unknownControl(false, 1);
  q.valid = unknownControl(false, 0);
  q.take = unknownControl(true, 0);
  const auto cold = q.current;
  const auto staleToken = token<RecordA>(87, 2);
  q.pending.token = staleToken;
  const auto oldReady = q.outputReady, oldValid = q.outputValid;
  const auto oldData = q.outputData;
  Kernel::reset(q.current, q.inputs(), q.pending);
  EXPECT_TRUE(q.pending.valid);
  EXPECT_TRUE(q.pending.reset);
  EXPECT_FALSE(q.pending.clock);
  expectCurrent<Kernel>(q.current, cold);
  expectCold();
  Kernel::discard(q.current, q.pending);
  EXPECT_FALSE(q.pending.valid);
  EXPECT_FALSE(q.pending.reset);
  EXPECT_FALSE(q.pending.push);
  EXPECT_FALSE(q.pending.pop);
  EXPECT_EQ(q.pending.clock, q.current.clock);
  expectPlanes(q.pending.token, staleToken);
  Kernel::xfer(q.current, q.pending, q.outputs());
  expectCurrent<Kernel>(q.current, cold);
  q.hostReset();
  EXPECT_TRUE(q.current.initialized);
  EXPECT_FALSE(q.current.clock);
  expectPlanes(Kernel::readValid(q.current), control(0));
  expectPlanes(Kernel::readReady(q.current, q.take), control(1));
  expectPlanes(Kernel::readData(q.current), zeroToken<RecordA>());
  expectPlanes(q.current.storage[0], before.storage[0]);
  expectPlanes(q.outputReady, oldReady);
  expectPlanes(q.outputValid, oldValid);
  expectPlanes(q.outputData, oldData);
  expectPlanes(q.pending.token, staleToken);
}
TEST(FifoRuntimeTest, ColdAuthorityHostResetAndDiscardBothPolicies) {
  coldLifecycle<Policy::LocalOccupancy>();
  coldLifecycle<Policy::DownstreamPop>();
}

template <Policy P> void pureReads() {
  QueueFixture<Table130, 3, P> q;
  using Kernel = typename decltype(q)::Kernel;
  q.hostReset();
  q.valid = control(1);
  q.take = control(0);
  for (unsigned i = 0; i != 3; ++i) {
    q.data = token<Table130>(17 + i, i + 1);
    q.sample(0);
    q.sample(1);
  }
  q.clk = control(0);
  Kernel::work(q.current, q.inputs(), q.pending, q.outputs());
  const auto beforeCurrent = q.current;
  const auto beforePending = q.pending;
  const auto head = Kernel::readData(q.current);
  for (unsigned latent : {0u, 1u}) {
    for (unsigned kind = 0; kind != 4; ++kind) {
      q.take = kind < 2 ? control(kind) : unknownControl(kind == 3, latent);
      const auto ready = Kernel::readReady(q.current, q.take);
      const auto expectedReady =
          P == Policy::LocalOccupancy ? control(0)
          : q.take.isFullyKnown()
              ? q.take
              : Control::fromPacked(
                    gfsim::FourState<1>::unknown(q.take.packed().value()));
      expectPlanes(ready, expectedReady);
      expectPlanes(Kernel::readValid(q.current), control(1));
      expectPlanes(Kernel::readData(q.current), head);
      expectCurrent<Kernel>(q.current, beforeCurrent);
      expectPending<Kernel>(q.pending, beforePending);
    }
  }
  // Outputs are borrowed destinations, not authority: arbitrary stale pins
  // cannot alter acceptance or the next committed head.
  q.outputReady = control(1);
  q.outputValid = control(0);
  q.outputData = token<Table130>(999);
  q.clk = control(0);
  Kernel::work(q.current, q.inputs(), q.pending, q.outputs());
  Kernel::xfer(q.current, q.pending, q.outputs());
  q.valid = control(0);
  q.take = control(1);
  q.clk = control(1);
  Kernel::work(q.current, q.inputs(), q.pending, q.outputs());
  Kernel::xfer(q.current, q.pending, q.outputs());
  expectPlanes(Kernel::readData(q.current), token<Table130>(18, 2));
  expectPlanes(q.outputReady, control(1));
  expectPlanes(q.outputValid, control(0));
  expectPlanes(q.outputData, token<Table130>(999));
}
TEST(FifoRuntimeTest, PureReadsHaveNoStatePendingOrOutputDependency) {
  pureReads<Policy::LocalOccupancy>();
  pureReads<Policy::DownstreamPop>();
}

template <Policy P> void effectiveControls() {
  using Q = QueueFixture<gfsim::Bits<13>, 3, P>;
  using Kernel = typename Q::Kernel;
  for (bool z : {false, true})
    for (unsigned latent : {0u, 1u}) {
      const auto unknown = unknownControl(z, latent);
      Q cold;
      cold.clk = control(1);
      cold.valid = unknown;
      cold.take = control(0);
      const auto coldBefore = cold.current;
      EXPECT_THROW(Kernel::work(cold.current, cold.inputs(), cold.pending,
                                cold.outputs()),
                   gfsim::FourStateViolation);
      EXPECT_FALSE(cold.pending.valid);
      EXPECT_EQ(cold.pending.clock, cold.current.clock);
      Kernel::xfer(cold.current, cold.pending, cold.outputs());
      expectCurrent<Kernel>(cold.current, coldBefore);
      cold.valid = control(0);
      cold.take = unknown;
      EXPECT_THROW(Kernel::work(cold.current, cold.inputs(), cold.pending,
                                cold.outputs()),
                   gfsim::FourStateViolation);
      // Nonrising Work masks unknown reset and all handshakes, even while cold.
      cold.clk = control(0);
      cold.rst = unknown;
      EXPECT_NO_THROW(Kernel::work(cold.current, cold.inputs(), cold.pending,
                                   cold.outputs()));
      Kernel::xfer(cold.current, cold.pending, cold.outputs());
      EXPECT_FALSE(cold.current.initialized);
      Q empty;
      empty.hostReset();
      empty.valid = control(0);
      empty.take = unknown;
      EXPECT_NO_THROW(empty.sample(1));
      expectPlanes(Kernel::readValid(empty.current), control(0));
      empty.rst = unknown;
      empty.valid = unknown;
      EXPECT_NO_THROW(
          empty.sample(1)); // Repeated high ignores unknown controls.
      expectPlanes(Kernel::readValid(empty.current), control(0));
      // Compare both known resolutions of the masked empty out_ready.
      for (unsigned take : {0u, 1u}) {
        Q resolved;
        resolved.hostReset();
        resolved.valid = control(0);
        resolved.take = control(take);
        resolved.sample(1);
        expectCurrent<Kernel>(empty.current, resolved.current);
      }
      Q interior;
      interior.hostReset();
      interior.valid = unknown;
      interior.clk = control(1);
      const auto before = interior.current;
      EXPECT_THROW(Kernel::work(interior.current, interior.inputs(),
                                interior.pending, interior.outputs()),
                   gfsim::FourStateViolation);
      EXPECT_FALSE(interior.pending.valid);
      EXPECT_FALSE(interior.pending.push);
      EXPECT_FALSE(interior.pending.pop);
      EXPECT_EQ(interior.pending.clock, before.clock);
      Kernel::xfer(interior.current, interior.pending, interior.outputs());
      expectCurrent<Kernel>(interior.current, before);
      // Same-edge retry must commit; a failed proposal cannot consume the edge.
      interior.valid = control(1);
      interior.data = token<gfsim::Bits<13>>(123, 1);
      EXPECT_NO_THROW(Kernel::work(interior.current, interior.inputs(),
                                   interior.pending, interior.outputs()));
      Kernel::xfer(interior.current, interior.pending, interior.outputs());
      expectPlanes(Kernel::readData(interior.current), interior.data);
      interior.sample(0);
      interior.valid = control(0);
      interior.take = unknown;
      interior.clk = control(1);
      EXPECT_THROW(Kernel::work(interior.current, interior.inputs(),
                                interior.pending, interior.outputs()),
                   gfsim::FourStateViolation);
      // Unknown reset at an edge rejects; reset1 dominates unknown transfers.
      interior.take = control(0);
      interior.rst = unknown;
      EXPECT_THROW(Kernel::work(interior.current, interior.inputs(),
                                interior.pending, interior.outputs()),
                   gfsim::FourStateViolation);
      interior.rst = control(1);
      interior.valid = unknown;
      interior.take = unknown;
      EXPECT_NO_THROW(Kernel::work(interior.current, interior.inputs(),
                                   interior.pending, interior.outputs()));
      Kernel::xfer(interior.current, interior.pending, interior.outputs());
      expectPlanes(Kernel::readValid(interior.current), control(0));
      expectPlanes(Kernel::readData(interior.current),
                   zeroToken<gfsim::Bits<13>>());
      // Full capacity masks unknown valid when no pop is requested, for both
      // policies, identically to its independent known-zero/one resolutions.
      Q full;
      full.hostReset();
      full.valid = control(1);
      for (unsigned i = 0; i != 3; ++i) {
        full.data = token<gfsim::Bits<13>>(i);
        full.sample(0);
        full.sample(1);
      }
      full.sample(0);
      const auto fullBefore = full.current;
      full.valid = unknown;
      full.take = control(0);
      full.clk = control(1);
      EXPECT_NO_THROW(Kernel::work(full.current, full.inputs(), full.pending,
                                   full.outputs()));
      Kernel::xfer(full.current, full.pending, full.outputs());
      for (unsigned valid : {0u, 1u}) {
        Q resolved;
        resolved.current = fullBefore;
        resolved.valid = control(valid);
        resolved.take = control(0);
        resolved.clk = control(1);
        Kernel::work(resolved.current, resolved.inputs(), resolved.pending,
                     resolved.outputs());
        Kernel::xfer(resolved.current, resolved.pending, resolved.outputs());
        expectCurrent<Kernel>(full.current, resolved.current);
      }
      full.sample(0);
      full.take = control(1);
      full.clk = control(1);
      if constexpr (P == Policy::DownstreamPop) {
        EXPECT_THROW(Kernel::work(full.current, full.inputs(), full.pending,
                                  full.outputs()),
                     gfsim::FourStateViolation);
      } else {
        EXPECT_NO_THROW(Kernel::work(full.current, full.inputs(), full.pending,
                                     full.outputs()));
        Kernel::xfer(full.current, full.pending, full.outputs());
        expectPlanes(Kernel::readData(full.current), token<gfsim::Bits<13>>(1));
      }
      // Unknown clocks reject even when reset is known high.
      full.clk = unknown;
      full.rst = control(1);
      const auto state = full.current;
      EXPECT_THROW(Kernel::work(full.current, full.inputs(), full.pending,
                                full.outputs()),
                   gfsim::FourStateViolation);
      Kernel::xfer(full.current, full.pending, full.outputs());
      expectCurrent<Kernel>(full.current, state);
    }
}
TEST(FifoRuntimeTest, MaskedEffectiveUnknownResetAndClockBothPolicies) {
  effectiveControls<Policy::LocalOccupancy>();
  effectiveControls<Policy::DownstreamPop>();
}

template <std::size_t D, Policy P> void throughput(unsigned expectedPushes) {
  QueueFixture<gfsim::Bits<65>, D, P> q;
  using Kernel = typename decltype(q)::Kernel;
  q.hostReset();
  q.valid = control(1);
  q.take = control(1);
  unsigned pushes = 0, pops = 0;
  for (unsigned cycle = 0; cycle != 12; ++cycle) {
    q.sample(0);
    pushes += Kernel::readReady(q.current, q.take).value().toBool();
    pops += Kernel::readValid(q.current).value().toBool();
    q.data = token<gfsim::Bits<65>>(cycle);
    q.sample(1);
  }
  EXPECT_EQ(pushes, expectedPushes);
  EXPECT_EQ(pops, P == Policy::DownstreamPop || D > 1 ? 11u : 6u);
}
TEST(FifoRuntimeTest, DepthOneHalfRateAndLargerDepthFullThroughput) {
  throughput<1, Policy::LocalOccupancy>(6);
  throughput<1, Policy::DownstreamPop>(12);
  throughput<2, Policy::LocalOccupancy>(12);
  throughput<3, Policy::LocalOccupancy>(12);
  throughput<5, Policy::LocalOccupancy>(12);
}

TEST(FifoRuntimeTest,
     PreparedPushAndResetDiscardKeepStateAndAllowSameEdgeRetry) {
  QueueFixture<TableRecord, 5, Policy::LocalOccupancy> q;
  using Kernel = typename decltype(q)::Kernel;
  q.hostReset();
  q.valid = control(1);
  q.clk = control(1);
  q.data = token<TableRecord>(59, 1);
  const auto before = q.current;
  q.publish();
  const auto output = q.outputData;
  Kernel::work(q.current, q.inputs(), q.pending, q.outputs());
  const auto preparedToken = q.pending.token;
  EXPECT_TRUE(q.pending.push);
  Kernel::discard(q.current, q.pending);
  EXPECT_FALSE(q.pending.valid);
  EXPECT_FALSE(q.pending.push);
  EXPECT_FALSE(q.pending.pop);
  EXPECT_FALSE(q.pending.reset);
  EXPECT_EQ(q.pending.clock, before.clock);
  expectPlanes(q.pending.token, preparedToken);
  Kernel::xfer(q.current, q.pending, q.outputs());
  expectCurrent<Kernel>(q.current, before);
  expectPlanes(q.outputData, output);
  Kernel::work(q.current, q.inputs(), q.pending, q.outputs());
  Kernel::xfer(q.current, q.pending, q.outputs());
  expectPlanes(Kernel::readData(q.current), q.data);
  q.sample(0);
  const auto nonempty = q.current;
  Kernel::reset(q.current, q.inputs(), q.pending);
  Kernel::discard(q.current, q.pending);
  Kernel::xfer(q.current, q.pending, q.outputs());
  expectCurrent<Kernel>(q.current, nonempty);
  // A committed host reset empties metadata but must leave every slot intact.
  q.hostReset();
  expectPlanes(Kernel::readData(q.current), zeroToken<TableRecord>());
  for (std::size_t i = 0; i != nonempty.storage.size(); ++i)
    expectPlanes(q.current.storage[i], nonempty.storage[i]);
}

template <std::size_t D, Policy P, std::uint64_t Latency = 1>
struct QueueFamily {
  using Kernel = gfsim::fifo_kernel<Table130, D, P, Latency>;
  gfsim::collection_storage<Kernel> state{2};
  std::array<Control, 2> clocks{control(0), control(0)},
      resets{control(0), control(0)}, valid{control(1), control(1)},
      take{control(0), control(0)}, ready{}, available{};
  std::array<gfsim::wire<Table130>, 2> data{token<Table130>(11, 1),
                                            token<Table130>(28, 2)},
      output{};
  typename Kernel::Inputs inputs(std::size_t i) const {
    return {clocks[i], resets[i], valid[i], data[i], take[i]};
  }
  typename Kernel::Outputs outputs(std::size_t i) {
    return {ready[i], available[i], output[i]};
  }
  void publish() {
    for (std::size_t i = 0; i != 2; ++i) {
      ready[i] = Kernel::readReady(state.current(i), take[i]);
      available[i] = Kernel::readValid(state.current(i));
      output[i] = Kernel::readData(state.current(i));
    }
  }
  void work() {
    publish();
    for (std::size_t i = 0; i != 2; ++i)
      state.work(i, inputs(i), outputs(i));
  }
  void xfer() {
    state.xfer([&](std::size_t i) noexcept { return outputs(i); });
  }
  void reset() {
    state.reset([&](std::size_t i) noexcept { return inputs(i); });
    xfer();
  }
};
template <std::size_t D, Policy P> void familyAtomicity() {
  QueueFamily<D, P> family;
  using Kernel = typename decltype(family)::Kernel;
  family.reset();
  family.clocks.fill(control(1));
  family.valid[1] = unknownControl(true, 1);
  const auto lane0 = family.state.current(0), lane1 = family.state.current(1);
  EXPECT_THROW(family.work(), gfsim::FourStateViolation);
  family.xfer();
  expectCurrent<Kernel>(family.state.current(0), lane0);
  expectCurrent<Kernel>(family.state.current(1), lane1);
  family.valid[1] = control(1);
  family.work();
  family.xfer();
  expectPlanes(Kernel::readData(family.state.current(0)), family.data[0]);
  expectPlanes(Kernel::readData(family.state.current(1)), family.data[1]);
  // Partial family preparation never commits even when the one lane succeeds.
  family.clocks.fill(control(0));
  family.work();
  family.xfer();
  const auto committed0 = family.state.current(0),
             committed1 = family.state.current(1);
  family.clocks.fill(control(1));
  family.state.work(0, family.inputs(0), family.outputs(0));
  family.xfer();
  expectCurrent<Kernel>(family.state.current(0), committed0);
  expectCurrent<Kernel>(family.state.current(1), committed1);

  using Register = gfsim::dffe_kernel<gfsim::Bits<13>>;
  typename Register::Current reg;
  typename Register::Pending regPending;
  Control regClock = control(0), regReset = control(0), regEnable = control(1);
  auto regData = token<gfsim::Bits<13>>(91),
       regInit = zeroToken<gfsim::Bits<13>>(), regOutput = regInit;
  auto regInputs = [&] {
    return typename Register::Inputs{regClock, regReset, regEnable, regData,
                                     regInit};
  };
  Register::reset(reg, regInputs(), regPending);
  Register::xfer(reg, regPending, {regOutput});
  const auto oldRegister = reg.q;
  // The existing whole-owner discard barrier is explicit here. A failed
  // later sibling causes no Xfer; all queue/register proposals are discarded.
  const auto firstTokens = family.data;
  family.data = {token<Table130>(102, 1), token<Table130>(207, 3)};
  regClock = control(1);
  regEnable = unknownControl(false, 1);
  family.work();
  EXPECT_THROW(Register::work(reg, regInputs(), regPending, {regOutput}),
               gfsim::FourStateViolation);
  family.state.discard();
  Register::discard(reg, regPending);
  family.xfer();
  Register::xfer(reg, regPending, {regOutput});
  expectCurrent<Kernel>(family.state.current(0), committed0);
  expectCurrent<Kernel>(family.state.current(1), committed1);
  EXPECT_FALSE(reg.clock);
  expectPlanes(reg.q, oldRegister);
  // Retrying exactly the same high edge commits each lane and the register.
  regEnable = control(1);
  family.work();
  Register::work(reg, regInputs(), regPending, {regOutput});
  family.xfer();
  Register::xfer(reg, regPending, {regOutput});
  EXPECT_TRUE(reg.clock);
  expectPlanes(reg.q, regData);
  EXPECT_EQ(family.state.current(0).count, 2u);
  EXPECT_EQ(family.state.current(1).count, 2u);
  expectPlanes(Kernel::readData(family.state.current(0)), firstTokens[0]);
  expectPlanes(Kernel::readData(family.state.current(1)), firstTokens[1]);
  // Pop the first tokens: no lane/table-element or latent-plane mixing.
  family.valid.fill(control(0));
  family.take.fill(control(1));
  family.clocks.fill(control(0));
  family.work();
  family.xfer();
  family.clocks.fill(control(1));
  family.work();
  family.xfer();
  expectPlanes(Kernel::readData(family.state.current(0)), family.data[0]);
  expectPlanes(Kernel::readData(family.state.current(1)), family.data[1]);
}
TEST(FifoRuntimeTest, TwoTableTokenLanesLateSiblingDiscardAndSameEdgeRetry) {
  familyAtomicity<3, Policy::LocalOccupancy>();
  familyAtomicity<3, Policy::DownstreamPop>();
  familyAtomicity<5, Policy::LocalOccupancy>();
  familyAtomicity<5, Policy::DownstreamPop>();
}

TEST(FifoRuntimeTest,
     AcceptedHotWorkPureReadTransferDiscardAndResetAllocateNothing) {
  using Kernel = gfsim::fifo_kernel<TableRecord, 5, Policy::DownstreamPop, 1>;
  typename Kernel::Current current;
  typename Kernel::Pending pending;
  Control clk = control(0), rst = control(0), valid = control(1),
          take = control(1), ready, available;
  auto data = token<TableRecord>(111, 1), output = zeroToken<TableRecord>();
  typename Kernel::Inputs inputs{clk, rst, valid, data, take};
  typename Kernel::Outputs outputs{ready, available, output};
  Kernel::reset(current, inputs, pending);
  Kernel::xfer(current, pending, outputs);
  QueueFamily<3, Policy::LocalOccupancy> family;
  family.reset();
  const auto before = allocationCount.load(std::memory_order_relaxed);
  trackAllocations.store(true, std::memory_order_relaxed);
  for (unsigned i = 0; i != 256; ++i) {
    clk = control(i % 2);
    ready = Kernel::readReady(current, take);
    available = Kernel::readValid(current);
    output = Kernel::readData(current);
    Kernel::work(current, inputs, pending, outputs);
    if (i % 17 == 0)
      Kernel::discard(current, pending);
    Kernel::xfer(current, pending, outputs);
    if (i % 41 == 0) {
      Kernel::reset(current, inputs, pending);
      Kernel::xfer(current, pending, outputs);
    }
    family.clocks.fill(control(i % 2));
    family.work();
    family.xfer();
    if (i % 43 == 0)
      family.reset();
  }
  trackAllocations.store(false, std::memory_order_relaxed);
  EXPECT_EQ(allocationCount.load(std::memory_order_relaxed), before);
}

using Edge = std::array<std::uint64_t, 2>; // {high, low}

constexpr Edge addEdge(Edge edge, std::uint64_t increment) {
  const auto oldLow = edge[1];
  edge[1] += increment;
  edge[0] += edge[1] < oldLow;
  return edge;
}

TEST(FifoRuntimeTest, PortableAbsoluteEdgeArithmeticCarryOrderingAndReset) {
  constexpr auto max = std::numeric_limits<std::uint64_t>::max();
  EXPECT_EQ(addEdge(Edge{0, max}, 1), (Edge{1, 0}));
  EXPECT_EQ(addEdge(Edge{0, max}, max - 1), (Edge{1, max - 2}));
  EXPECT_LT((Edge{0, max}), (Edge{1, 0}));
  EXPECT_EQ((Edge{1, max - 2}), (Edge{1, max - 2}));

  Edge edge{1, 7};
  edge = Edge{0, 0};
  EXPECT_EQ(edge, (Edge{0, 0}));
}

// The oracle uses absolute two-word committed-edge numbers, not wrapped
// timestamps or implementation pointers. Runs assert the bounded epoch domain;
// adding any u64 latency therefore remains exact, including UINT64_MAX.
template <class T, std::size_t D, Policy P, std::uint64_t L>
struct AvailabilityOracle {
  struct Birth {
    gfsim::wire<T> value;
    Edge maturity;
  };
  QueueFixture<T, D, P, L> queue;
  std::deque<Birth> births;
  Edge edges{0, 0};
  bool previousClock = false;
  unsigned pushes = 0, pops = 0, replacements = 0, popMaturePush = 0;

  AvailabilityOracle() { queue.hostReset(); }
  bool eligible() const {
    return !births.empty() && births.front().maturity <= edges;
  }
  bool ready(bool take) const {
    return births.size() < D ||
           (P == Policy::DownstreamPop && eligible() && take);
  }
  void observe() {
    using Kernel = typename decltype(queue)::Kernel;
    expectPlanes(Kernel::readReady(queue.current, queue.take),
                 control(ready(queue.take.value().toBool())));
    expectPlanes(Kernel::readValid(queue.current), control(eligible()));
    expectPlanes(Kernel::readData(queue.current),
                 eligible() ? births.front().value : zeroToken<T>());
    EXPECT_EQ(queue.current.count, births.size());
  }
  void sample(bool clock, bool reset, bool valid, bool take, unsigned serial,
              unsigned mask = 1, bool abandon = false) {
    using Kernel = typename decltype(queue)::Kernel;
    queue.clk = control(clock);
    queue.rst = control(reset);
    queue.valid = control(valid);
    queue.take = control(take);
    queue.data = token<T>(serial, mask);
    observe();
    queue.publish();
    const auto current = queue.current;
    const auto oldReady = queue.outputReady, oldValid = queue.outputValid;
    const auto oldData = queue.outputData;
    const bool edge = !previousClock && clock;
    const bool pop = edge && !reset && eligible() && take;
    const bool push = edge && !reset && ready(take) && valid;
    const bool full = births.size() == D;
    bool matures = false;
    if (edge && !reset)
      for (const auto &birth : births)
        matures |= birth.maturity == addEdge(edges, 1);
    Kernel::work(queue.current, queue.inputs(), queue.pending, queue.outputs());
    expectCurrent<Kernel>(queue.current, current);
    if (abandon)
      Kernel::discard(queue.current, queue.pending);
    Kernel::xfer(queue.current, queue.pending, queue.outputs());
    expectPlanes(queue.outputReady, oldReady);
    expectPlanes(queue.outputValid, oldValid);
    expectPlanes(queue.outputData, oldData);
    if (abandon) {
      expectCurrent<Kernel>(queue.current, current);
    } else {
      previousClock = clock;
      if (edge && reset) {
        births.clear();
        edges = Edge{0, 0};
      } else if (edge) {
        ASSERT_TRUE(edges <
                    (Edge{0, std::numeric_limits<std::uint64_t>::max()}));
        edges = addEdge(edges, 1);
        if (pop) {
          births.pop_front();
          ++pops;
        }
        if (push) {
          births.push_back({queue.data, addEdge(edges, L - 1)});
          ++pushes;
        }
        replacements += pop && push && full;
        popMaturePush += pop && push && matures;
      }
    }
    observe();
    EXPECT_FALSE(queue.pending.valid);
    EXPECT_FALSE(queue.pending.advance_time);
    EXPECT_FALSE(queue.pending.mature);
  }
  void rise(bool valid, bool take, unsigned serial, unsigned mask = 1) {
    sample(false, false, valid, take, serial, mask);
    sample(true, false, valid, take, serial, mask);
  }
};

template <class T, std::size_t D, Policy P, std::uint64_t L>
void delayedDeque() {
  SCOPED_TRACE(::testing::Message() << "depth=" << D << " latency=" << L
                                    << " policy=" << static_cast<int>(P));
  AvailabilityOracle<T, D, P, L> oracle;
  // Birth E0 reserves capacity immediately. Pop requests at the maturity edge
  // must still see old ineligible state; earliest pop is EL.
  oracle.rise(true, true, 10, 1);
  ASSERT_EQ(oracle.births.size(), 1u);
  for (std::uint64_t age = 1; age < L; ++age) {
    oracle.rise(false, true, 0);
    EXPECT_EQ(oracle.pops, 0u);
    EXPECT_EQ(oracle.births.size(), 1u);
  }
  oracle.rise(false, true, 0);
  EXPECT_EQ(oracle.pops, 1u);
  EXPECT_TRUE(oracle.births.empty());
  // Empty wraps must not revive stale deadlines. Blocked mature heads persist
  // through many complete timestamp revolutions, including a delayed suffix.
  for (unsigned i = 0; i != D; ++i)
    oracle.rise(true, false, 20 + i, i % 4);
  for (unsigned i = 0; i != 8 * L + D; ++i)
    oracle.rise(false, false, 0);
  ASSERT_EQ(oracle.births.size(), D);
  EXPECT_TRUE(oracle.eligible());
  for (unsigned i = 0; i != 256; ++i) {
    const bool valid = i % 5 != 1, take = i % 7 < 5;
    oracle.sample(false, false, valid, take, i + 100, i % 4);
    oracle.sample(false, false, !valid, !take, i + 400, (i + 1) % 4);
    // A discarded rising proposal must not age or consume the physical edge.
    if (i % 17 == 0)
      oracle.sample(true, false, valid, take, i + 100, i % 4, true);
    oracle.sample(true, i == 91, valid, take, i + 100, i % 4);
    oracle.sample(true, false, true, true, i + 700, (i + 2) % 4);
  }
  // Continuous traffic forces simultaneous old-head pop, suffix maturity and
  // a fresh push where D permits, plus D1 eligible-slot replacement/reuse.
  for (unsigned i = 0; i != 64; ++i)
    oracle.rise(true, true, i + 900, i % 4);
  for (unsigned i = 0; i != D + L + 1; ++i)
    oracle.rise(false, true, 0);
  EXPECT_TRUE(oracle.births.empty());
  EXPECT_GT(oracle.pushes, 10u);
  if constexpr (D == 1 && P == Policy::DownstreamPop)
    EXPECT_GT(oracle.replacements, 0u);
  if constexpr (L > 1 && (D > L || (D == L && P == Policy::DownstreamPop)))
    EXPECT_GT(oracle.popMaturePush, 0u);
}

template <class T, std::uint64_t L> void delayedDepthPolicyMatrix() {
  delayedDeque<T, 1, Policy::LocalOccupancy, L>();
  delayedDeque<T, 1, Policy::DownstreamPop, L>();
  delayedDeque<T, 2, Policy::LocalOccupancy, L>();
  delayedDeque<T, 2, Policy::DownstreamPop, L>();
  delayedDeque<T, 3, Policy::LocalOccupancy, L>();
  delayedDeque<T, 3, Policy::DownstreamPop, L>();
  delayedDeque<T, 5, Policy::LocalOccupancy, L>();
  delayedDeque<T, 5, Policy::DownstreamPop, L>();
}
TEST(FifoRuntimeTest, AbsoluteAvailabilityEdgesDepthPolicyLatencyBoundaries) {
  delayedDepthPolicyMatrix<gfsim::Bits<13>, 1>();
  delayedDepthPolicyMatrix<gfsim::Bits<13>, 2>();
  delayedDepthPolicyMatrix<gfsim::Bits<13>, 3>();
  delayedDepthPolicyMatrix<gfsim::Bits<13>, 4>();
  delayedDepthPolicyMatrix<gfsim::Bits<13>, 5>();
  delayedDepthPolicyMatrix<gfsim::Bits<13>, 8>();
  delayedDepthPolicyMatrix<gfsim::Bits<13>, 9>();
  delayedDepthPolicyMatrix<gfsim::Bits<13>, 16>();
  delayedDepthPolicyMatrix<gfsim::Bits<13>, 17>();
}
TEST(FifoRuntimeTest, DelayedCompleteRawPlanesWidthsRecordsAndTableTokens) {
  delayedDepthPolicyMatrix<gfsim::Bits<1>, 3>();
  delayedDepthPolicyMatrix<gfsim::Bits<65>, 3>();
  delayedDepthPolicyMatrix<gfsim::Bits<130>, 3>();
  delayedDepthPolicyMatrix<RecordA, 3>();
  delayedDepthPolicyMatrix<RecordB, 3>();
  delayedDepthPolicyMatrix<Table130, 3>();
  delayedDepthPolicyMatrix<TableRecord, 3>();
}

template <Policy P> void delayedControlsResetReads() {
  using Q = QueueFixture<RecordA, 3, P, 5>;
  using Kernel = typename Q::Kernel;
  Q q;
  expectPlanes(Kernel::readReady(q.current, q.take), Control::unknown());
  expectPlanes(Kernel::readValid(q.current), Control::unknown());
  expectPlanes(Kernel::readData(q.current), gfsim::wire<RecordA>::unknown());
  q.hostReset();
  q.valid = control(1);
  for (unsigned i = 0; i != 3; ++i) {
    q.data = token<RecordA>(i + 21, i % 4);
    q.sample(0);
    q.sample(1);
  }
  ASSERT_EQ(q.current.count, 3u);
  expectPlanes(Kernel::readData(q.current), zeroToken<RecordA>());
  // Full waiting capacity masks both unknown transfers, even on rising edges.
  // None of these nonedges or pure reads may bring a birth closer to maturity.
  for (bool z : {false, true})
    for (unsigned latent : {0u, 1u}) {
      q.valid = unknownControl(z, latent);
      q.take = unknownControl(!z, latent);
      const auto before = q.current;
      const auto pending = q.pending;
      for (unsigned i = 0; i != 32; ++i) {
        expectPlanes(Kernel::readReady(q.current, q.take), control(0));
        expectPlanes(Kernel::readValid(q.current), control(0));
        expectPlanes(Kernel::readData(q.current), zeroToken<RecordA>());
      }
      expectCurrent<Kernel>(q.current, before);
      expectPending<Kernel>(q.pending, pending);
      q.sample(1); // Held high throughout this loop.
      expectCurrent<Kernel>(q.current, before);
    }
  const auto beforeFalling = q.current;
  q.sample(0); // Falling.
  q.sample(0); // Held low.
  auto nonedge = beforeFalling;
  nonedge.clock = false;
  expectCurrent<Kernel>(q.current, nonedge);
  EXPECT_NO_THROW(q.sample(1)); // E3: still ineligible.
  expectPlanes(Kernel::readValid(q.current), control(0));
  q.sample(0);
  EXPECT_NO_THROW(q.sample(1)); // E4 Xfer matures the first birth.
  expectPlanes(Kernel::readData(q.current), token<RecordA>(21, 0));
  q.sample(0);
  const auto maturityBefore = q.current;
  q.clk = control(1);
  EXPECT_THROW(Kernel::work(q.current, q.inputs(), q.pending, q.outputs()),
               gfsim::FourStateViolation);
  EXPECT_FALSE(q.pending.advance_time);
  EXPECT_FALSE(q.pending.mature);
  Kernel::xfer(q.current, q.pending, q.outputs());
  expectCurrent<Kernel>(q.current, maturityBefore);
  // Same physical edge retry ages the delayed suffix once, with no pop.
  q.valid = control(0);
  q.take = control(0);
  q.sample(1);
  q.sample(0);
  const auto beforeReset = q.current;
  q.clk = unknownControl(true, 1);
  q.rst = unknownControl(false, 0);
  Kernel::reset(q.current, q.inputs(), q.pending);
  EXPECT_FALSE(q.pending.clock);
  Kernel::discard(q.current, q.pending);
  Kernel::xfer(q.current, q.pending, q.outputs());
  expectCurrent<Kernel>(q.current, beforeReset);
  q.hostReset();
  EXPECT_FALSE(q.current.clock);
  expectPlanes(Kernel::readData(q.current), zeroToken<RecordA>());
  for (std::size_t i = 0; i != 3; ++i)
    expectPlanes(q.current.storage[i], beforeReset.storage[i]);
  q.clk = control(1);
  q.rst = control(1);
  q.valid = unknownControl(true, 1);
  q.take = unknownControl(false, 1);
  EXPECT_NO_THROW(q.sample(1)); // Synchronous reset masks unknown transfers.
  q.rst = control(0);
  q.valid = control(0);
  q.take = control(1);
  for (unsigned i = 0; i != 48; ++i) {
    q.sample(0);
    q.sample(1);
    expectPlanes(Kernel::readValid(q.current), control(0));
    expectPlanes(Kernel::readData(q.current), zeroToken<RecordA>());
  }
  // Stale deadlines cannot accelerate a fresh birth after mixed reset.
  q.valid = control(1);
  q.take = control(0);
  q.data = token<RecordA>(87, 2);
  q.sample(0);
  q.sample(1);
  q.valid = control(0);
  for (unsigned age = 1; age != 5; ++age) {
    q.sample(0);
    q.sample(1);
    expectPlanes(Kernel::readValid(q.current), control(age == 4));
  }
  expectPlanes(Kernel::readData(q.current), token<RecordA>(87, 2));
}
TEST(FifoRuntimeTest,
     DelayedUnknownControlsPureReadsMixedResetAndStaleMetadata) {
  delayedControlsResetReads<Policy::LocalOccupancy>();
  delayedControlsResetReads<Policy::DownstreamPop>();
}

template <Policy P> void delayedFamilyFailureAndIndependentClocks() {
  QueueFamily<3, P, 3> family;
  using Kernel = typename decltype(family)::Kernel;
  family.reset();
  auto step = [&] {
    family.work();
    family.xfer();
  };
  family.clocks.fill(control(1));
  step(); // Both birth E0.
  family.valid.fill(control(0));
  family.clocks.fill(control(0));
  step();
  family.clocks[0] = control(1);
  step(); // Only lane zero advances to E1.
  family.clocks[0] = control(0);
  step();
  const auto before0 = family.state.current(0),
             before1 = family.state.current(1);
  family.clocks.fill(control(1));
  family.valid[1] = unknownControl(true, 0); // Late owner rejects a push.
  EXPECT_THROW(family.work(), gfsim::FourStateViolation);
  family.xfer();
  expectCurrent<Kernel>(family.state.current(0), before0);
  expectCurrent<Kernel>(family.state.current(1), before1);
  expectPlanes(Kernel::readValid(family.state.current(0)), control(0));
  family.valid[1] = control(0);
  step(); // Retry lane zero E2 maturity, lane one only E1.
  expectPlanes(Kernel::readValid(family.state.current(0)), control(1));
  expectPlanes(Kernel::readValid(family.state.current(1)), control(0));
  expectPlanes(Kernel::readData(family.state.current(0)), family.data[0]);
  expectPlanes(Kernel::readData(family.state.current(1)),
               zeroToken<Table130>());
  family.clocks[1] = control(0);
  step();
  // A different storage sibling fails exactly when lane one would mature.
  using Register = gfsim::dffe_kernel<gfsim::Bits<13>>;
  typename Register::Current reg;
  typename Register::Pending pending;
  Control clk = control(1), rst = control(0), en = unknownControl(false, 1);
  auto data = token<gfsim::Bits<13>>(17), init = zeroToken<gfsim::Bits<13>>();
  auto out = init;
  typename Register::Inputs inputs{clk, rst, en, data, init};
  Register::reset(reg, inputs, pending);
  Register::xfer(reg, pending, {out});
  const auto beforeRegister = reg;
  const auto lane0 = family.state.current(0), lane1 = family.state.current(1);
  family.clocks[1] = control(1);
  family.work();
  EXPECT_THROW(Register::work(reg, inputs, pending, {out}),
               gfsim::FourStateViolation);
  family.state.discard();
  Register::discard(reg, pending);
  family.xfer();
  Register::xfer(reg, pending, {out});
  expectCurrent<Kernel>(family.state.current(0), lane0);
  expectCurrent<Kernel>(family.state.current(1), lane1);
  expectPlanes(reg.q, beforeRegister.q);
  EXPECT_EQ(reg.clock, beforeRegister.clock);
  en = control(1);
  family.work();
  Register::work(reg, inputs, pending, {out});
  family.xfer();
  Register::xfer(reg, pending, {out});
  expectPlanes(Kernel::readData(family.state.current(1)), family.data[1]);
  expectPlanes(reg.q, data);
}
TEST(FifoRuntimeTest,
     DelayedMaturityLateSiblingZeroCommitRetryIndependentClocks) {
  delayedFamilyFailureAndIndependentClocks<Policy::LocalOccupancy>();
  delayedFamilyFailureAndIndependentClocks<Policy::DownstreamPop>();
}

template <std::uint64_t L> void hugeLatencyEarlyOnly() {
  AvailabilityOracle<gfsim::Bits<65>, 2, Policy::DownstreamPop, L> oracle;
  oracle.rise(true, false, 31, 1);
  oracle.rise(true, false, 32, 2);
  for (unsigned i = 0; i != 32; ++i)
    oracle.rise(true, true, i + 100, 1);
  EXPECT_EQ(oracle.pushes, 2u);
  EXPECT_EQ(oracle.pops, 0u);
  oracle.sample(false, false, false, false, 0);
  oracle.sample(true, true, false, false, 0);
  EXPECT_TRUE(oracle.births.empty());
  for (unsigned i = 0; i != 8; ++i)
    oracle.rise(false, true, 0);
}
TEST(FifoRuntimeTest, HugeLatencyBoundedEarlyStateAndResetNoHugeWaitClaim) {
  hugeLatencyEarlyOnly<(std::uint64_t{1} << 63)>();
  hugeLatencyEarlyOnly<(std::uint64_t{1} << 63) + 1>();
  hugeLatencyEarlyOnly<std::numeric_limits<std::uint64_t>::max()>();
}

TEST(FifoRuntimeTest,
     LabelledTimestampArithmeticProbeK64NearWrapReachableState) {
  using Q = QueueFixture<gfsim::Bits<13>, 2, Policy::DownstreamPop,
                         std::numeric_limits<std::uint64_t>::max()>;
  using Kernel = Q::Kernel;
  // This is an arithmetic/state-invariant probe, not a claim that the enormous
  // wait was executed. One waiting token with one edge remaining is reachable
  // by accepting it L-2 edges earlier; a matured predecessor may have popped.
  EXPECT_EQ(Kernel::TimestampWidth, 64u);
  const auto max = std::numeric_limits<std::uint64_t>::max();
  EXPECT_EQ(Kernel::wrapTick(max), max);
  EXPECT_EQ(Kernel::wrapTick(std::uint64_t(max + std::uint64_t{1})), 0u);
  Q q;
  q.hostReset();
  q.current.count = 1;
  q.current.wr = 1;
  q.current.storage[0] = token<gfsim::Bits<13>>(51, 1);
  q.current.timing.tick = max;
  q.current.timing.deadline[0] = 0;
  q.clk = control(1);
  q.take = control(1);
  q.valid = control(1);
  q.data = token<gfsim::Bits<13>>(52, 2);
  const auto before = q.current;
  Kernel::work(q.current, q.inputs(), q.pending, q.outputs());
  EXPECT_TRUE(q.pending.mature);
  EXPECT_FALSE(q.pending.pop);
  Kernel::discard(q.current, q.pending);
  Kernel::xfer(q.current, q.pending, q.outputs());
  expectCurrent<Kernel>(q.current, before);
  Kernel::work(q.current, q.inputs(), q.pending, q.outputs());
  Kernel::xfer(q.current, q.pending, q.outputs());
  EXPECT_EQ(q.current.timing.tick, 0u);
  EXPECT_EQ(q.current.timing.deadline[1], max - 1);
  EXPECT_EQ(q.current.timing.mature_ptr, 1u);
  EXPECT_EQ(Kernel::eligibleCount(q.current), 1u);
  expectPlanes(Kernel::readData(q.current), token<gfsim::Bits<13>>(51, 1));
  q.valid = control(0);
  q.sample(0);
  q.sample(1);
  EXPECT_EQ(q.current.count, 1u);
  expectPlanes(Kernel::readValid(q.current), control(0));
  expectPlanes(Kernel::readData(q.current), zeroToken<gfsim::Bits<13>>());
}

template <class T, std::size_t D> void layoutProbe() {
  using L1 = gfsim::fifo_kernel<T, D, Policy::LocalOccupancy, 1>;
  using Delayed = gfsim::fifo_kernel<T, D, Policy::DownstreamPop, 3>;
  using Huge = gfsim::fifo_kernel<T, D, Policy::DownstreamPop,
                                  std::numeric_limits<std::uint64_t>::max()>;
  const auto tokenBytes = sizeof(gfsim::wire<T>);
  // Bound relationships, not incidental offsets/padding or exact object sizes.
  struct LatencyOneBaseline {
    std::array<gfsim::wire<T>, D> storage;
    std::size_t rd, wr, count;
    bool initialized, clock;
  };
  EXPECT_TRUE((std::is_empty_v<typename L1::Timing>));
  EXPECT_EQ(sizeof(typename L1::Current), sizeof(LatencyOneBaseline));
  EXPECT_LE(sizeof(typename L1::Current), D * tokenBytes + 32);
  EXPECT_LE(sizeof(typename Delayed::Current),
            sizeof(typename L1::Current) + 8 * D + 24);
  EXPECT_LE(sizeof(typename Huge::Current),
            sizeof(typename L1::Current) + 8 * D + 24);
  EXPECT_LE(sizeof(typename Delayed::Pending), tokenBytes + 8);
  EXPECT_LE(sizeof(typename L1::Pending), tokenBytes + 8);
  // Full conservative source envelope: input/output and two cached payload
  // wires, six control wires, and <=8 completion bytes per family lane.
  const auto laneBytes = sizeof(typename Delayed::Current) +
                         sizeof(typename Delayed::Pending) + 4 * tokenBytes +
                         6 * sizeof(Control) + 8;
  const auto bound = tokenBytes * (D + 5) + 8 * D + 216;
  EXPECT_LE(laneBytes, bound);
  for (std::size_t family : {1u, 2u, 7u})
    EXPECT_LE(family * laneBytes + sizeof(gfsim::collection_storage<Delayed>),
              family * bound + 128);
}
TEST(FifoRuntimeTest, LabelledActualLayoutProbeConservativeLaneFamilyBudget) {
  layoutProbe<gfsim::Bits<1>, 1>();
  layoutProbe<gfsim::Bits<13>, 2>();
  layoutProbe<gfsim::Bits<65>, 3>();
  layoutProbe<gfsim::Bits<130>, 5>();
  layoutProbe<RecordA, 3>();
  layoutProbe<RecordB, 5>();
  layoutProbe<Table130, 3>();
  layoutProbe<TableRecord, 5>();
}

TEST(FifoRuntimeTest, DelayedHotWorkReadsTransferDiscardResetAllocateNothing) {
  QueueFixture<TableRecord, 5, Policy::DownstreamPop, 3> q;
  using Kernel = typename decltype(q)::Kernel;
  q.hostReset();
  q.valid = control(1);
  q.take = control(1);
  const auto before = allocationCount.load(std::memory_order_relaxed);
  trackAllocations.store(true, std::memory_order_relaxed);
  for (unsigned i = 0; i != 512; ++i) {
    q.clk = control(i % 2);
    const auto ready = Kernel::readReady(q.current, q.take);
    const auto available = Kernel::readValid(q.current);
    const auto output = Kernel::readData(q.current);
    (void)ready;
    (void)available;
    (void)output;
    Kernel::work(q.current, q.inputs(), q.pending, q.outputs());
    if (i % 17 == 0)
      Kernel::discard(q.current, q.pending);
    Kernel::xfer(q.current, q.pending, q.outputs());
    if (i % 41 == 0) {
      Kernel::reset(q.current, q.inputs(), q.pending);
      Kernel::xfer(q.current, q.pending, q.outputs());
    }
  }
  trackAllocations.store(false, std::memory_order_relaxed);
  EXPECT_EQ(allocationCount.load(std::memory_order_relaxed), before);
}
} // namespace
