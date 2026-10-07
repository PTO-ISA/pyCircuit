#include "gfsim/byte_mem.h"
#include "gfsim/collection.h"
#include "gfsim/dff.h"
#include "gfsim/sync_mem.h"
#include "gtest/gtest.h"
#include <array>
#include <atomic>
#include <cstdlib>
#include <new>
#include <type_traits>

namespace {
std::atomic<bool> trackAllocations{false};
std::atomic<std::size_t> allocationCount{0};
struct Entry {
  gfsim::Bits<3> tag;
  gfsim::Bits<5> body;
};
} // namespace
void *operator new(std::size_t size) {
  if (trackAllocations.load(std::memory_order_relaxed))
    ++allocationCount;
  if (auto *p = std::malloc(size))
    return p;
  throw std::bad_alloc();
}
void operator delete(void *p) noexcept { std::free(p); }
void operator delete(void *p, std::size_t) noexcept { std::free(p); }
namespace gfsim {
template <>
struct hardware_traits<Entry>
    : hardware_struct_traits<Entry, &Entry::tag, &Entry::body> {};
} // namespace gfsim
namespace {
template <unsigned W> auto known(uint64_t value) {
  return gfsim::wire<gfsim::Bits<W>>::known(gfsim::Bits<W>{value});
}
using Control = gfsim::wire<gfsim::Bits<1>>;
using Data = gfsim::wire<gfsim::Bits<9>>;
struct RegisterFamily {
  using Kernel = gfsim::dffe_kernel<gfsim::Bits<9>>;
  gfsim::collection_storage<Kernel> state{3};
  std::array<Control, 3> clk{known<1>(0), known<1>(0), known<1>(0)},
      rst{known<1>(0), known<1>(0), known<1>(0)},
      en{known<1>(1), known<1>(1), known<1>(1)};
  std::array<Data, 3> d{known<9>(11), known<9>(22), known<9>(33)},
      init{known<9>(3), known<9>(7), known<9>(19)}, q{};
  Kernel::Inputs inputs(std::size_t i) const {
    return {clk[i], rst[i], en[i], d[i], init[i]};
  }
  void transfer() {
    state.xfer([&](std::size_t i) noexcept { return Kernel::Outputs{q[i]}; });
  }
  void reset() {
    state.reset([&](std::size_t i) noexcept { return inputs(i); });
    transfer();
  }
  void work() {
    for (std::size_t i = 0; i < 3; ++i)
      state.work(i, inputs(i), {q[i]});
  }
  void edge(bool high) {
    clk.fill(known<1>(high));
    work();
    transfer();
  }
};
TEST(CollectionRuntimeTest, NestedShapeRowMajorAndAllFourStatePlanes) {
  using Payload = gfsim::table<gfsim::table<gfsim::Bits<4>, 3>, 2>;
  static_assert(Payload::rank == 2 && Payload::size == 6);
  static_assert(Payload::shape[0] == 2 && Payload::shape[1] == 3);
  static_assert(std::is_same_v<Payload::element_type, gfsim::Bits<4>>);
  using FS = gfsim::FourState<24>;
  auto packed =
      FS::fromMasks(gfsim::Bits<24>{0x123456}, gfsim::Bits<24>{0xfff0f0},
                    gfsim::Bits<24>{0x000a0a});
  auto wire = gfsim::wire<Payload>::fromPacked(packed);
  EXPECT_EQ(wire.element(0).value(), gfsim::Bits<4>{1});
  EXPECT_EQ(wire.element(4).value(), gfsim::Bits<4>{5});
  EXPECT_EQ(wire.element(3).packed().zMask(), gfsim::Bits<4>{0xa});
  EXPECT_EQ(wire.element(5).packed().knownMask(), gfsim::Bits<4>{0});
  EXPECT_EQ(wire.packed().value(), gfsim::Bits<24>{0x123456});
  EXPECT_EQ(wire.packed().knownMask(), gfsim::Bits<24>{0xfff0f0});
  EXPECT_EQ(wire.packed().zMask(), gfsim::Bits<24>{0x000a0a});
  EXPECT_FALSE(wire.isFullyKnown());
  EXPECT_EQ(&wire.element(4), wire.data() + 4);
}
TEST(CollectionRuntimeTest, StructTableUsesDeclaredFieldsAndElementZeroIsMsb) {
  using Payload = gfsim::table<Entry, 2>;
  Payload values;
  values[0] = {gfsim::Bits<3>{5}, gfsim::Bits<5>{17}};
  values[1] = {gfsim::Bits<3>{2}, gfsim::Bits<5>{3}};
  auto packed = gfsim::hardware_traits<Payload>::pack(values);
  EXPECT_EQ(packed, gfsim::Bits<16>{0xb143});
  auto decoded =
      gfsim::hardware_traits<Payload>::unpack(gfsim::Bits<16>{0xe522});
  EXPECT_EQ(decoded[0].tag, gfsim::Bits<3>{7});
  EXPECT_EQ(decoded[0].body, gfsim::Bits<5>{5});
  EXPECT_EQ(decoded[1].tag, gfsim::Bits<3>{1});
  EXPECT_EQ(decoded[1].body, gfsim::Bits<5>{2});
}
TEST(CollectionRuntimeTest, DffeIndependentHoldClockedResetAndOldQ) {
  RegisterFamily family;
  family.reset();
  family.en[1] = known<1>(0);
  family.rst[2] = known<1>(1);
  family.en[2] = Control::unknown();
  family.clk.fill(known<1>(1));
  family.work();
  EXPECT_EQ(family.q[0].value(), gfsim::Bits<9>{3});
  EXPECT_EQ(family.state.current(0).q.value(), gfsim::Bits<9>{3});
  family.transfer();
  EXPECT_EQ(family.q[0].value(), gfsim::Bits<9>{11});
  EXPECT_EQ(family.q[1].value(), gfsim::Bits<9>{7});
  EXPECT_EQ(family.q[2].value(), gfsim::Bits<9>{19});
  family.transfer();
  EXPECT_EQ(family.q[1].value(), gfsim::Bits<9>{7});
}
TEST(CollectionRuntimeTest, FailedLaterLaneDiscardsAllDataAndPendingClocks) {
  RegisterFamily family;
  family.reset();
  family.clk.fill(known<1>(1));
  family.en[2] = Control::unknown();
  EXPECT_THROW(family.work(), gfsim::FourStateViolation);
  family.transfer();
  const unsigned initial[]{3, 7, 19};
  for (std::size_t i = 0; i < 3; ++i) {
    EXPECT_EQ(family.q[i].value(), gfsim::Bits<9>{initial[i]});
    EXPECT_FALSE(family.state.current(i).clock);
  }
  family.en[2] = known<1>(1);
  family.work();
  family.transfer();
  const unsigned changed[]{11, 22, 33};
  for (std::size_t i = 0; i < 3; ++i)
    EXPECT_EQ(family.q[i].value(), gfsim::Bits<9>{changed[i]});
}
TEST(CollectionRuntimeTest, PartialWorkAndExplicitDiscardCannotCommit) {
  RegisterFamily family;
  family.reset();
  family.clk.fill(known<1>(1));
  family.state.work(0, family.inputs(0), {family.q[0]});
  family.transfer();
  EXPECT_EQ(family.q[0].value(), gfsim::Bits<9>{3});
  family.work();
  family.state.discard();
  family.transfer();
  EXPECT_EQ(family.q[2].value(), gfsim::Bits<9>{19});
  family.work();
  family.transfer();
  EXPECT_EQ(family.q[2].value(), gfsim::Bits<9>{33});
}
TEST(CollectionRuntimeTest, SameKernelOwnersDoNotShareState) {
  RegisterFamily left, right;
  left.reset();
  right.reset();
  left.edge(true);
  right.d[0] = known<9>(201);
  right.edge(true);
  EXPECT_EQ(left.q[0].value(), gfsim::Bits<9>{11});
  EXPECT_EQ(right.q[0].value(), gfsim::Bits<9>{201});
  EXPECT_NE(&left.state.current(0), &right.state.current(0));
}
TEST(CollectionRuntimeTest, DffRingUsesOneSnapshotAndPreservesXz) {
  using K = gfsim::dff_kernel<gfsim::Bits<4>>;
  gfsim::collection_storage<K> state(3);
  auto zero = known<1>(0), one = known<1>(1);
  std::array<gfsim::wire<gfsim::Bits<4>>, 3> initial{known<4>(2), known<4>(5),
                                                     known<4>(9)},
      q, d;
  state.reset([&](std::size_t i) noexcept {
    return K::Inputs{zero, zero, initial[i], initial[i]};
  });
  state.xfer([&](std::size_t i) noexcept { return K::Outputs{q[i]}; });
  for (std::size_t i = 0; i < 3; ++i)
    d[i] = q[(i + 1) % 3];
  for (std::size_t i = 0; i < 3; ++i)
    state.work(i, {one, zero, d[i], initial[i]}, {q[i]});
  EXPECT_EQ(q[0].value(), gfsim::Bits<4>{2});
  state.xfer([&](std::size_t i) noexcept { return K::Outputs{q[i]}; });
  const unsigned rotated[]{5, 9, 2};
  for (std::size_t i = 0; i < 3; ++i)
    EXPECT_EQ(q[i].value(), gfsim::Bits<4>{rotated[i]});
  for (std::size_t i = 0; i < 3; ++i)
    state.work(i, {zero, zero, d[i], initial[i]}, {q[i]});
  state.xfer([&](std::size_t i) noexcept { return K::Outputs{q[i]}; });
  d[1] = gfsim::wire<gfsim::Bits<4>>::fromPacked(gfsim::FourState<4>::fromMasks(
      gfsim::Bits<4>{1}, gfsim::Bits<4>{3}, gfsim::Bits<4>{4}));
  for (std::size_t i = 0; i < 3; ++i)
    state.work(i, {one, zero, d[i], initial[i]}, {q[i]});
  state.xfer([&](std::size_t i) noexcept { return K::Outputs{q[i]}; });
  EXPECT_EQ(q[1].packed().knownMask(), gfsim::Bits<4>{3});
  EXPECT_EQ(q[1].packed().zMask(), gfsim::Bits<4>{4});
}
TEST(CollectionRuntimeTest, BulkWorkTransferAndBorrowDoNotAllocate) {
  using K = gfsim::dff_kernel<gfsim::Bits<9>>;
  static_assert(
      !std::is_base_of_v<gfsim::SimModule, gfsim::collection_storage<K>>);
  static_assert(!std::is_polymorphic_v<K::Current> &&
                !std::is_polymorphic_v<K::Pending>);
  gfsim::collection_storage<K> state(64);
  gfsim::wire<gfsim::table<gfsim::Bits<9>, 64>> q;
  auto zero = known<1>(0), one = known<1>(1);
  auto init = known<9>(7), data = known<9>(401);
  const auto *borrowed = q.data();
  allocationCount = 0;
  trackAllocations = true;
  state.reset(
      [&](std::size_t) noexcept { return K::Inputs{zero, zero, data, init}; });
  state.xfer([&](std::size_t i) noexcept { return K::Outputs{q.element(i)}; });
  for (std::size_t i = 0; i < 64; ++i)
    state.work(i, {one, zero, data, init}, {q.element(i)});
  state.xfer([&](std::size_t i) noexcept { return K::Outputs{q.element(i)}; });
  state.discard();
  trackAllocations = false;
  EXPECT_EQ(allocationCount.load(), 0u);
  EXPECT_EQ(borrowed, q.data());
  EXPECT_EQ(q.element(63).value(), gfsim::Bits<9>{401});
}
template <std::size_t Ports> struct MemoryFamily {
  using K = gfsim::sync_mem_kernel<gfsim::Bits<16>, 3, 5, Ports>;
  struct Pins {
    Control clk = known<1>(0), rst = known<1>(0), wvalid = known<1>(0);
    std::array<Control, Ports> ren;
    std::array<gfsim::wire<gfsim::Bits<3>>, Ports> raddr;
    gfsim::wire<gfsim::Bits<3>> waddr = known<3>(1);
    gfsim::wire<gfsim::Bits<16>> wdata = known<16>(0);
    gfsim::wire<gfsim::Bits<2>> wstrb = known<2>(3);
    Pins() {
      ren.fill(known<1>(0));
      raddr.fill(known<3>(1));
    }
  };
  gfsim::collection_storage<K> state{2};
  std::array<Pins, 2> pins;
  std::array<std::array<gfsim::wire<gfsim::Bits<16>>, Ports>, 2> q;
  typename K::Inputs inputs(std::size_t i) const {
    auto &p = pins[i];
    typename K::Inputs result{p.clk,    p.rst,   {},      {},
                              p.wvalid, p.waddr, p.wdata, p.wstrb};
    for (std::size_t j = 0; j < Ports; ++j) {
      result.ren[j] = &p.ren[j];
      result.raddr[j] = &p.raddr[j];
    }
    return result;
  }
  typename K::Outputs outputs(std::size_t i) {
    typename K::Outputs result{};
    for (std::size_t j = 0; j < Ports; ++j)
      result.rdata[j] = &q[i][j];
    return result;
  }
  void work() {
    for (std::size_t i = 0; i < 2; ++i)
      state.work(i, inputs(i), outputs(i));
  }
  void transfer() {
    state.xfer([&](std::size_t i) noexcept { return outputs(i); });
  }
  void edge(bool high) {
    for (auto &p : pins)
      p.clk = known<1>(high);
    work();
    transfer();
  }
  void reset() {
    state.reset([&](std::size_t i) noexcept { return inputs(i); });
    transfer();
  }
};
TEST(CollectionRuntimeTest,
     SyncMemoriesIndependentOldDataStrobesAndTwoIdleExpiry) {
  MemoryFamily<1> family;
  family.reset();
  family.pins[0].wdata = known<16>(0x1234);
  family.pins[1].wdata = known<16>(0x5678);
  for (auto &p : family.pins) {
    p.wvalid = known<1>(1);
    p.ren[0] = known<1>(1);
  }
  family.edge(true);
  EXPECT_EQ(family.q[0][0].value(), gfsim::Bits<16>{0});
  EXPECT_EQ(family.state.current(0).memory[1], gfsim::Bits<16>{0x1234});
  EXPECT_EQ(family.state.current(1).memory[1], gfsim::Bits<16>{0x5678});
  family.edge(false);
  family.pins[0].wdata = known<16>(0xabcd);
  family.pins[1].wdata = known<16>(0x9bef);
  for (auto &p : family.pins)
    p.wstrb = known<2>(1);
  family.edge(true);
  EXPECT_EQ(family.q[0][0].value(), gfsim::Bits<16>{0x1234});
  EXPECT_EQ(family.q[1][0].value(), gfsim::Bits<16>{0x5678});
  EXPECT_EQ(family.state.current(0).memory[1], gfsim::Bits<16>{0x12cd});
  EXPECT_EQ(family.state.current(1).memory[1], gfsim::Bits<16>{0x56ef});
  for (auto &p : family.pins) {
    p.wvalid = known<1>(0);
    p.ren[0] = known<1>(0);
  }
  family.edge(false);
  family.edge(true);
  EXPECT_TRUE(family.q[0][0].isFullyKnown());
  family.edge(false);
  family.edge(true);
  EXPECT_FALSE(family.q[0][0].isFullyKnown());
  family.reset();
  EXPECT_EQ(family.state.current(1).memory[1], gfsim::Bits<16>{0x56ef});
  EXPECT_FALSE(family.q[1][0].isFullyKnown());
  EXPECT_FALSE(family.state.current(0).clock);
}
TEST(CollectionRuntimeTest, SyncMemoryFailureCancelsWritesAndCanRetrySameEdge) {
  MemoryFamily<1> family;
  family.reset();
  family.pins[0].wdata = known<16>(301);
  family.pins[1].wdata = known<16>(707);
  for (auto &p : family.pins) {
    p.clk = known<1>(1);
    p.wvalid = known<1>(1);
  }
  family.pins[1].wstrb = gfsim::wire<gfsim::Bits<2>>::unknown();
  EXPECT_THROW(family.work(), gfsim::FourStateViolation);
  family.transfer();
  EXPECT_EQ(family.state.current(0).memory[1], gfsim::Bits<16>{0});
  EXPECT_EQ(family.state.current(1).memory[1], gfsim::Bits<16>{0});
  EXPECT_FALSE(family.state.current(0).clock);
  family.pins[1].wstrb = known<2>(3);
  family.work();
  family.transfer();
  EXPECT_EQ(family.state.current(0).memory[1], gfsim::Bits<16>{301});
  EXPECT_EQ(family.state.current(1).memory[1], gfsim::Bits<16>{707});
}
TEST(CollectionRuntimeTest, DualPortReadOutputsAndStorageRemainPerLane) {
  MemoryFamily<2> family;
  family.reset();
  for (std::size_t i = 0; i < 2; ++i) {
    auto &p = family.pins[i];
    p.wvalid = known<1>(1);
    p.waddr = known<3>(0);
    p.wdata = known<16>(i ? 29 : 11);
  }
  family.edge(true);
  family.edge(false);
  for (std::size_t i = 0; i < 2; ++i) {
    auto &p = family.pins[i];
    p.waddr = known<3>(2);
    p.wdata = known<16>(i ? 47 : 31);
  }
  family.edge(true);
  family.edge(false);
  for (auto &p : family.pins) {
    p.wvalid = known<1>(0);
    p.ren.fill(known<1>(1));
    p.raddr[0] = known<3>(0);
    p.raddr[1] = known<3>(2);
  }
  family.edge(true);
  EXPECT_EQ(family.q[0][0].value(), gfsim::Bits<16>{11});
  EXPECT_EQ(family.q[0][1].value(), gfsim::Bits<16>{31});
  EXPECT_EQ(family.q[1][0].value(), gfsim::Bits<16>{29});
  EXPECT_EQ(family.q[1][1].value(), gfsim::Bits<16>{47});
}
TEST(CollectionRuntimeTest, ByteMemoryAsyncReadsUseOldDataAndDiscardAllLanes) {
  using K = gfsim::byte_mem_kernel<gfsim::Bits<16>, 3, 5>;
  gfsim::collection_storage<K> state(2);
  auto zero = known<1>(0), one = known<1>(1);
  auto address = known<3>(2);
  auto strobe = known<2>(3);
  std::array<gfsim::wire<gfsim::Bits<16>>, 2> data{known<16>(0x1234),
                                                   known<16>(0x5678)},
      q;
  auto inputs = [&](std::size_t i, const Control &clk) {
    return K::Inputs{clk, zero, address, one, address, data[i], strobe};
  };
  for (std::size_t i = 0; i < 2; ++i)
    state.work(i, inputs(i, one), {q[i]});
  EXPECT_EQ(q[0].value(), gfsim::Bits<16>{0});
  state.xfer([&](std::size_t i) noexcept { return K::Outputs{q[i]}; });
  EXPECT_EQ(state.current(0).memory[2], 0x34);
  EXPECT_EQ(state.current(1).memory[3], 0x56);
  for (std::size_t i = 0; i < 2; ++i)
    state.work(i, inputs(i, zero), {q[i]});
  EXPECT_EQ(q[0].value(), gfsim::Bits<16>{0x1234});
  EXPECT_EQ(q[1].value(), gfsim::Bits<16>{0x5678});
  state.xfer([&](std::size_t i) noexcept { return K::Outputs{q[i]}; });
  data[0] = known<16>(0xbeef);
  data[1] = known<16>(0x1122);
  strobe = known<2>(2);
  state.work(0, inputs(0, one), {q[0]});
  auto badClock = Control::unknown();
  EXPECT_THROW(state.work(1, inputs(1, badClock), {q[1]}),
               gfsim::FourStateViolation);
  state.xfer([&](std::size_t i) noexcept { return K::Outputs{q[i]}; });
  EXPECT_EQ(state.current(0).memory[3], 0x12);
  EXPECT_EQ(state.current(1).memory[3], 0x56);
  for (std::size_t i = 0; i < 2; ++i)
    state.work(i, inputs(i, one), {q[i]});
  state.xfer([&](std::size_t i) noexcept { return K::Outputs{q[i]}; });
  EXPECT_EQ(state.current(0).memory[2], 0x34);
  EXPECT_EQ(state.current(0).memory[3], 0xbe);
  EXPECT_EQ(state.current(1).memory[2], 0x78);
  EXPECT_EQ(state.current(1).memory[3], 0x11);
  state.reset([&](std::size_t i) noexcept { return inputs(i, zero); });
  state.xfer([&](std::size_t i) noexcept { return K::Outputs{q[i]}; });
  EXPECT_EQ(state.current(0).memory[3], 0xbe);
  EXPECT_FALSE(state.current(0).clock);
}
TEST(CollectionRuntimeTest, PureByteReadDoesNotChangeStorageClockOrPending) {
  using K = gfsim::byte_mem_kernel<gfsim::Bits<16>, 3, 5>;
  K::Current current;
  current.memory[2] = 0x34;
  current.memory[3] = 0x12;
  current.clock = true;
  K::Pending pending;
  pending.valid = true;
  pending.clock = false;
  pending.address = 1;
  pending.bytes[0] = 0x91;
  pending.lanes[0] = true;
  auto storage = current.memory;
  auto q = known<16>(0xffff);
  K::read(current, known<3>(2), {q});
  EXPECT_EQ(q.value(), gfsim::Bits<16>{0x1234});
  K::read(current, gfsim::wire<gfsim::Bits<3>>::unknown(), {q});
  EXPECT_FALSE(q.isFullyKnown());
  K::read(current, known<3>(7), {q});
  EXPECT_EQ(q.value(), gfsim::Bits<16>{0});
  EXPECT_EQ(current.memory, storage);
  EXPECT_TRUE(current.clock);
  EXPECT_TRUE(pending.valid);
  EXPECT_FALSE(pending.clock);
  EXPECT_EQ(pending.address, 1u);
  EXPECT_EQ(pending.bytes[0], 0x91);
  EXPECT_TRUE(pending.lanes[0]);
}
TEST(CollectionRuntimeTest, AssignSliceCrossesWordsAndPreservesAllOtherPlanes) {
  using Wide = gfsim::Bits<130>;
  using Part = gfsim::Bits<70>;
  auto original = gfsim::FourState<130>::known(
      Wide{0x123456789abcdef0ULL, 0xfedcba9876543210ULL, 3ULL});
  auto source = gfsim::FourState<70>::fromMasks(
      Part{0x8877665544332211ULL, 0x21ULL},
      Part{0xffffffffffffff7fULL, 0x3eULL}, Part{0x80ULL, 1ULL});
  auto destination = gfsim::wire<Wide>::fromPacked(original);
  destination.assignSlice(59, source);
  for (unsigned bit = 0; bit < 130; ++bit) {
    bool selected = bit >= 59 && bit < 129;
    unsigned offset = selected ? bit - 59 : bit;
    EXPECT_EQ(destination.packed().value().bit(bit),
              selected ? source.value().bit(offset)
                       : original.value().bit(offset));
    EXPECT_EQ(destination.packed().knownMask().bit(bit),
              selected ? source.knownMask().bit(offset)
                       : original.knownMask().bit(offset));
    EXPECT_EQ(destination.packed().zMask().bit(bit),
              selected ? source.zMask().bit(offset)
                       : original.zMask().bit(offset));
  }
  EXPECT_EQ(source.value(), (Part{0x8877665544332211ULL, 0x21ULL}));
  EXPECT_EQ(source.zMask(), (Part{0x80ULL, 1ULL}));
}
} // namespace
