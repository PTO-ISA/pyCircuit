#include "gfsim/dff.h"
#include "gfsim/SimSystem.h"
#include "gtest/gtest.h"

#include <type_traits>

namespace {
struct Payload { gfsim::Bits<5> tag; gfsim::Bits<70> data; };
}
namespace gfsim {
template <> struct hardware_traits<Payload>
    : hardware_struct_traits<Payload, &Payload::tag, &Payload::data> {};
}
namespace {
template <unsigned W> auto known(unsigned long long value) {
  return gfsim::wire<gfsim::Bits<W>>::known(gfsim::Bits<W>{value});
}
template <class Leaf> void fallingEdge(Leaf &reg) {
  reg.clk = known<1>(0); reg.Work(); reg.Xfer();
}
TEST(RegRuntimeTest, ConcreteEmptyModuleAndAbstractObject) {
  static_assert(std::is_abstract_v<gfsim::SimObj>);
  static_assert(!std::is_abstract_v<gfsim::SimModule>);
  gfsim::SimModule empty("empty");
  empty.Build(); empty.Work(); empty.Xfer(); empty.DiscardNext();
  empty.Reset(); empty.ReportStat();
  EXPECT_FALSE(empty.HasWork());
}
TEST(RegRuntimeTest, DffeOldQHoldDiscardAndClockedReset) {
  gfsim::dffe<gfsim::Bits<9>> reg("reg");
  static_assert(noexcept(reg.Xfer()));
  reg.init = known<9>(3); reg.Reset();
  EXPECT_FALSE(reg.q.isFullyKnown());
  reg.Xfer(); EXPECT_EQ(reg.q.value().value(), 3u);
  reg.clk = known<1>(1); reg.rst = known<1>(0);
  reg.en = known<1>(1); reg.d = known<9>(257);
  reg.Work(); EXPECT_EQ(reg.q.value().value(), 3u);
  reg.DiscardNext(); reg.Xfer(); EXPECT_EQ(reg.q.value().value(), 3u);
  // Discard cancels clock history as well, so the same edge can be retried.
  reg.Work(); reg.Xfer(); EXPECT_EQ(reg.q.value().value(), 257u);
  fallingEdge(reg); reg.clk = known<1>(1); reg.en = known<1>(0);
  reg.d = known<9>(11); reg.Work(); reg.Xfer();
  EXPECT_EQ(reg.q.value().value(), 257u);
  // An empty transfer cannot replay the disabled D value.
  reg.Xfer(); EXPECT_EQ(reg.q.value().value(), 257u);
  fallingEdge(reg); reg.clk = known<1>(1); reg.rst = known<1>(1);
  reg.en = gfsim::wire<gfsim::Bits<1>>::unknown();
  reg.Work(); EXPECT_EQ(reg.q.value().value(), 257u);
  reg.Xfer(); EXPECT_EQ(reg.q.value().value(), 3u);
  reg.Reset(); reg.DiscardNext(); reg.Xfer();
  EXPECT_EQ(reg.q.value().value(), 3u);
}
TEST(RegRuntimeTest, StructLayoutAndIndependentLeafInstancesAbove64Bits) {
  Payload initial{gfsim::Bits<5>{18}, gfsim::shl(gfsim::Bits<70>{1}, 68)};
  Payload update{gfsim::Bits<5>{7}, gfsim::Bits<70>{53}};
  const auto packed = gfsim::hardware_traits<Payload>::pack(initial);
  // First declared field is MSB. Values below bit 70 belong only to data.
  EXPECT_TRUE(packed.bit(74)); EXPECT_TRUE(packed.bit(71));
  EXPECT_TRUE(packed.bit(68)); EXPECT_FALSE(packed.bit(70));
  const auto decoded = gfsim::hardware_traits<Payload>::unpack(packed);
  EXPECT_EQ(decoded.tag, initial.tag); EXPECT_EQ(decoded.data, initial.data);
  gfsim::dff<Payload> left("left"), right("right");
  for (auto *leaf : {&left, &right}) {
    leaf->init = gfsim::wire<Payload>::known(initial);
    leaf->Reset(); leaf->Xfer(); leaf->clk = known<1>(1);
    leaf->rst = known<1>(0);
  }
  left.d = gfsim::wire<Payload>::known(update);
  right.d = gfsim::wire<Payload>::known(initial);
  left.Work(); right.Work();
  EXPECT_EQ(left.q.value().tag, initial.tag);
  left.Xfer(); right.DiscardNext(); right.Xfer();
  EXPECT_EQ(left.q.value().tag, update.tag);
  EXPECT_EQ(left.q.value().data, update.data);
  EXPECT_EQ(right.q.value().tag, initial.tag);
  EXPECT_EQ(right.q.value().data, initial.data);
}
TEST(RegRuntimeTest, FourStatePayloadCopiesAndUnknownControlsFailClosed) {
  using FS = gfsim::FourState<9>;
  gfsim::dffe<gfsim::Bits<9>> reg("xz");
  reg.init = known<9>(0); reg.Reset(); reg.Xfer();
  reg.clk = known<1>(1); reg.rst = known<1>(0); reg.en = known<1>(1);
  // One known bit and one Z bit; all other bits remain X.
  reg.d = gfsim::wire<gfsim::Bits<9>>::fromPacked(
      FS::fromMasks(gfsim::Bits<9>{1}, gfsim::Bits<9>{1}, gfsim::Bits<9>{4}));
  reg.Work(); reg.Xfer();
  EXPECT_EQ(reg.q.packed().value(), gfsim::Bits<9>{1});
  EXPECT_EQ(reg.q.packed().knownMask(), gfsim::Bits<9>{1});
  EXPECT_EQ(reg.q.packed().zMask(), gfsim::Bits<9>{4});
  fallingEdge(reg); reg.clk = known<1>(1);
  reg.en = gfsim::wire<gfsim::Bits<1>>::unknown();
  EXPECT_THROW(reg.Work(), gfsim::FourStateViolation);
  reg.DiscardNext(); reg.Xfer();
  EXPECT_EQ(reg.q.packed().knownMask(), gfsim::Bits<9>{1});
}
class NestedLeaves final : public gfsim::SimModule {
public:
  NestedLeaves() : SimModule("root"), left("root.left"), right("root.right") {}
  gfsim::dffe<gfsim::Bits<9>> left, right;
  bool injectFailure = false;
  unsigned observed = 0;
  void Work() override {
    observed = static_cast<unsigned>(left.q.value().value());
    for (auto *leaf : {&left, &right}) {
      leaf->clk = known<1>(1); leaf->rst = known<1>(0);
      leaf->en = known<1>(1); leaf->d = known<9>(observed + 4);
      leaf->Work();
    }
    if (injectFailure) throw std::runtime_error("after both leaf proposals");
  }
  void Xfer() noexcept override { left.Xfer(); right.Xfer(); }
  void DiscardNext() noexcept override { left.DiscardNext(); right.DiscardNext(); }
  void Reset() noexcept override {
    left.init = known<9>(5); right.init = known<9>(11);
    left.Reset(); right.Reset();
  }
  bool HasWork() const noexcept override { return true; }
};
class LeafSystem final : public gfsim::SimSystem {
public: bool accepted = true;
private: bool Precheck() noexcept override { return accepted; }
};
TEST(RegRuntimeTest, NestedLeavesNeverCommitWhenWorkOrPrecheckFails) {
  for (bool workFailure : {false, true}) {
    NestedLeaves root; LeafSystem system;
    ASSERT_TRUE(system.AddModule(root)); system.Build(); system.Reset();
    root.injectFailure = workFailure; system.accepted = workFailure;
    EXPECT_EQ(system.Step(), gfsim::SimStepResult::Failed);
    EXPECT_EQ(system.cycle(), 0u); EXPECT_EQ(root.observed, 5u);
    EXPECT_EQ(root.left.q.value(), gfsim::Bits<9>{5});
    EXPECT_EQ(root.right.q.value(), gfsim::Bits<9>{11});
    // Even an accidental later transfer cannot publish discarded proposals.
    root.Xfer();
    EXPECT_EQ(root.left.q.value(), gfsim::Bits<9>{5});
    EXPECT_EQ(root.right.q.value(), gfsim::Bits<9>{11});
    root.injectFailure = false; system.accepted = true; system.Reset();
    ASSERT_EQ(system.Step(), gfsim::SimStepResult::Running);
    EXPECT_EQ(root.left.q.value(), gfsim::Bits<9>{9});
    EXPECT_EQ(root.right.q.value(), gfsim::Bits<9>{9});
  }
}

}

// These value tests share the existing Runtime test target. The new arithmetic
// overload must guard host division and preserve its full-result X convention.
TEST(RegRuntimeArithmeticTest, UnsignedDivRemExhaustiveEightBitValues) {
  using B = gfsim::Bits<8>;
  using F = gfsim::FourState<8>;
  for (unsigned n = 0; n != 256; ++n)
    for (unsigned d = 0; d != 256; ++d) {
      auto q = gfsim::udiv(F::known(B{n}), F::known(B{d}));
      auto r = gfsim::urem(F::known(B{n}), F::known(B{d}));
      if (!d) {
        ASSERT_EQ(q.knownMask(), B{0});
        ASSERT_EQ(r.knownMask(), B{0});
        ASSERT_EQ(q.zMask(), B{0});
        ASSERT_EQ(r.zMask(), B{0});
      } else {
        ASSERT_TRUE(q.isFullyKnown() && r.isFullyKnown());
        ASSERT_EQ(q.value().value(), n / d);
        ASSERT_EQ(r.value().value(), n % d);
      }
    }
  // The existing low-level two-state helper has a separate legacy convention.
  EXPECT_EQ(gfsim::urem(B{17}, B{0}), B{0});
}

template <unsigned W> void unknownDivRemInputs() {
  using B = gfsim::Bits<W>;
  using F = gfsim::FourState<W>;
  for (unsigned bit = 0; bit != W; ++bit) {
    B mask{0}; mask.setWord(bit / 64, std::uint64_t{1} << (bit % 64));
    for (bool z : {false, true}) {
      auto partial = F::fromMasks(B::ones(), ~mask, z ? mask : B{0});
      for (auto pair : {std::pair{partial, F::known(B{3})},
                        std::pair{partial, F::known(B{0})},
                        std::pair{F::known(B{7}), partial},
                        std::pair{F::known(B{0}), partial},
                        std::pair{partial, partial}}) {
        for (auto result : {gfsim::udiv(pair.first, pair.second),
                             gfsim::urem(pair.first, pair.second)}) {
          ASSERT_TRUE(result.invariantHolds());
          EXPECT_EQ(result.knownMask(), B{0});
          EXPECT_EQ(result.zMask(), B{0});
        }
      }
    }
  }
}

TEST(RegRuntimeArithmeticTest, UnknownsDominateArithmeticAtEveryBitPosition) {
  unknownDivRemInputs<1>(); unknownDivRemInputs<13>();
  unknownDivRemInputs<32>(); unknownDivRemInputs<64>();
  unknownDivRemInputs<65>(); unknownDivRemInputs<129>();
  unknownDivRemInputs<257>();
}

namespace {

// Two independent physical clock/reset pin pairs over the same Runtime register
// kernel. The host drives each lane's pins explicitly. Nothing here merges the
// lanes into one domain, schedules domains, or crosses between them: this is
// multi-domain *physical drive*, not clock-domain scheduling or CDC.
struct LaneDrive {
  bool clk = false;
  bool rst = false;
};
// Reference model for one lane: an edge commits only when THAT lane's own clock
// rises, and reset is sampled only at that same own rising edge.
struct LaneModel {
  unsigned q = 0;
  bool clock = false;
  bool tick(bool clk, bool rst) {
    const bool rising = !clock && clk;
    clock = clk;
    if (!rising)
      return false;
    q = rst ? 0u : (q + 1u) & 255u;
    return true;
  }
};
class TwoClockLanes final : public gfsim::SimModule {
public:
  TwoClockLanes() : SimModule("lanes"), a("lanes.a"), b("lanes.b") {}
  gfsim::dff<gfsim::Bits<8>> a, b;
  // Published counts. The emitted C++ publishes each lane's Q during Work,
  // before the system transfers state, so a sample observes old Q and the next
  // epoch sees the previous edge's committed result. These wires reproduce
  // that publication order rather than reading the leaf output after Xfer.
  gfsim::wire<gfsim::Bits<8>> aCount, bCount;
  LaneDrive driveA, driveB;
  void Work() override {
    for (auto *lane : {&a, &b})
      lane->init = known<8>(0);
    a.clk = known<1>(driveA.clk); a.rst = known<1>(driveA.rst);
    b.clk = known<1>(driveB.clk); b.rst = known<1>(driveB.rst);
    aCount = a.q; bCount = b.q;
    // Data is the next count derived from committed Q only, so no lane can
    // advance without its own clock edge.
    a.d = known<8>((a.q.value().value() + 1) & 255);
    b.d = known<8>((b.q.value().value() + 1) & 255);
    a.Work(); b.Work();
  }
  void Xfer() noexcept override { a.Xfer(); b.Xfer(); }
  void DiscardNext() noexcept override { a.DiscardNext(); b.DiscardNext(); }
  void Reset() noexcept override {
    for (auto *lane : {&a, &b}) { lane->init = known<8>(0); lane->Reset(); }
  }
  bool HasWork() const noexcept override { return true; }
};
class TwoClockSystem final : public gfsim::SimSystem {
public:
  bool accepted = true;
private:
  bool Precheck() noexcept override { return accepted; }
};
gfsim::wire<gfsim::Bits<1>> unknownBit(bool highZ) {
  return gfsim::wire<gfsim::Bits<1>>::fromPacked(gfsim::FourState<1>::fromMasks(
      gfsim::Bits<1>{0}, gfsim::Bits<1>{0}, gfsim::Bits<1>{highZ ? 1u : 0u}));
}
void lowerClock(gfsim::dff<gfsim::Bits<8>> &lane) {
  lane.clk = known<1>(0); lane.Work(); lane.Xfer();
}

} // namespace

// Two physical clock domains through the Runtime API/runner: independent edges,
// independent reset, state isolation and per-lane real wraparound. A sample is
// the value committed by the previous epoch's own edges, so the oracle reads
// the model before ticking it.
TEST(RegRuntimeTest, TwoClockLanesDriveIndependentEdgesResetIsolationAndWrap) {
  TwoClockLanes lanes;
  TwoClockSystem system;
  ASSERT_TRUE(system.AddModule(lanes));
  system.Build();
  system.Reset();
  ASSERT_EQ(system.cycle(), 0u);
  // Public Reset initializes both leaves from init 0 without any clock edge.
  EXPECT_EQ(lanes.a.q.value(), gfsim::Bits<8>{0});
  EXPECT_EQ(lanes.b.q.value(), gfsim::Bits<8>{0});

  LaneModel modelA, modelB;
  unsigned edgesA = 0, edgesB = 0, wrapsA = 0, wrapsB = 0;
  unsigned resetEdgesA = 0, resetEdgesB = 0;
  auto frame = [&](LaneDrive da, LaneDrive db, const char *why) {
    const unsigned expectedA = modelA.q, expectedB = modelB.q;
    lanes.driveA = da;
    lanes.driveB = db;
    ASSERT_EQ(system.Step(), gfsim::SimStepResult::Running) << why;
    ASSERT_TRUE(lanes.a.q.isFullyKnown() && lanes.b.q.isFullyKnown()) << why;
    EXPECT_EQ(lanes.aCount.value(), gfsim::Bits<8>{expectedA}) << why;
    EXPECT_EQ(lanes.bCount.value(), gfsim::Bits<8>{expectedB}) << why;
    // A wrap counts only an own rising edge with the own reset low; a reset
    // edge is counted separately so no wrap can be a disguised reset.
    if (modelA.tick(da.clk, da.rst)) {
      ++edgesA;
      if (da.rst) ++resetEdgesA; else if (modelA.q == 0) ++wrapsA;
    }
    if (modelB.tick(db.clk, db.rst)) {
      ++edgesB;
      if (db.rst) ++resetEdgesB; else if (modelB.q == 0) ++wrapsB;
    }
  };
  const LaneDrive low{false, false};

  // Each lane reaches known zero through its own reset edge only.
  frame({true, true}, {true, true}, "own-edge reset");
  frame(low, low, "both resets deasserted");
  // Independent edges and state isolation: three A edges move only A.
  for (unsigned i = 0; i != 3; ++i) {
    frame({true, false}, low, "A own edge");
    frame(low, low, "A falling half");
  }
  EXPECT_EQ(lanes.aCount.value(), gfsim::Bits<8>{3});
  EXPECT_EQ(lanes.bCount.value(), gfsim::Bits<8>{0});
  // Then two B edges move only B while A holds the different value 3.
  for (unsigned i = 0; i != 2; ++i) {
    frame(low, {true, false}, "B own edge");
    frame(low, low, "B falling half");
  }
  EXPECT_EQ(lanes.aCount.value(), gfsim::Bits<8>{3});
  EXPECT_EQ(lanes.bCount.value(), gfsim::Bits<8>{2});
  // A held reset needs its own rising edge. Raising rst_a while clk_a is
  // already high, and holding it over the falling half, must leave A alone;
  // only the following A rise may reset it. B gets no edge in this block.
  frame({true, false}, low, "A own edge before the held reset");
  frame({true, true}, low, "rst_a with clk_a already high");
  frame({false, true}, low, "A falls with rst_a still high");
  frame({true, true}, low, "A own reset edge");
  frame(low, low, "A reset deasserted");
  EXPECT_EQ(lanes.aCount.value(), gfsim::Bits<8>{0});
  // Independent reset: A's own reset leaves B's different value undisturbed.
  EXPECT_EQ(lanes.bCount.value(), gfsim::Bits<8>{2});
  // The mirror case: B's own reset leaves A's nonzero state undisturbed.
  frame({true, false}, low, "A own edge to a distinct value");
  frame(low, low, "A falling half");
  EXPECT_EQ(lanes.aCount.value(), gfsim::Bits<8>{1});
  frame(low, {true, true}, "B own reset edge");
  frame(low, low, "B reset deasserted");
  EXPECT_EQ(lanes.aCount.value(), gfsim::Bits<8>{1});
  EXPECT_EQ(lanes.bCount.value(), gfsim::Bits<8>{0});
  // Simultaneous edges keep two distinct per-lane states at all times.
  for (unsigned i = 0; i != 6; ++i) {
    frame({true, false}, low, "A offset edge");
    frame(low, low, "A falling half");
  }
  for (unsigned i = 0; i != 3; ++i) {
    frame(low, {true, false}, "B offset edge");
    frame(low, low, "B falling half");
  }
  EXPECT_EQ(lanes.aCount.value(), gfsim::Bits<8>{7});
  EXPECT_EQ(lanes.bCount.value(), gfsim::Bits<8>{3});
  for (unsigned i = 0; i != 40; ++i) {
    frame({true, false}, {true, false}, "simultaneous edges");
    frame(low, low, "simultaneous falling half");
  }
  EXPECT_EQ(lanes.aCount.value(), gfsim::Bits<8>{47});
  EXPECT_EQ(lanes.bCount.value(), gfsim::Bits<8>{43});
  // Per-lane real wraparound. 256 genuine rising edges on one lane's own clock
  // with its own reset low reach 255 -> 0 exactly once and return to the start
  // value, while the other lane does not move at all.
  {
    const unsigned heldB = modelB.q;
    const unsigned beforeEdges = edgesA, beforeWraps = wrapsA;
    const unsigned beforeResets = resetEdgesA;
    for (unsigned edge = 0; edge != 256; ++edge) {
      frame({true, false}, low, "A wrap sweep edge");
      frame(low, low, "A wrap sweep falling half");
      ASSERT_EQ(lanes.bCount.value(), gfsim::Bits<8>{heldB})
          << "B must hold for the whole A wrap sweep";
    }
    EXPECT_EQ(edgesA - beforeEdges, 256u);
    EXPECT_EQ(resetEdgesA, beforeResets);
    EXPECT_EQ(wrapsA - beforeWraps, 1u);
    EXPECT_EQ(lanes.aCount.value(), gfsim::Bits<8>{47});
    EXPECT_EQ(lanes.bCount.value(), gfsim::Bits<8>{43});
  }
  {
    const unsigned heldA = modelA.q;
    const unsigned beforeEdges = edgesB, beforeWraps = wrapsB;
    const unsigned beforeResets = resetEdgesB;
    for (unsigned edge = 0; edge != 256; ++edge) {
      frame(low, {true, false}, "B wrap sweep edge");
      frame(low, low, "B wrap sweep falling half");
      ASSERT_EQ(lanes.aCount.value(), gfsim::Bits<8>{heldA})
          << "A must hold for the whole B wrap sweep";
    }
    EXPECT_EQ(edgesB - beforeEdges, 256u);
    EXPECT_EQ(resetEdgesB, beforeResets);
    EXPECT_EQ(wrapsB - beforeWraps, 1u);
    EXPECT_EQ(lanes.aCount.value(), gfsim::Bits<8>{47});
    EXPECT_EQ(lanes.bCount.value(), gfsim::Bits<8>{43});
  }
  // Exactly two own reset edges per lane and exactly one non-reset wrap each.
  EXPECT_EQ(resetEdgesA, 2u);
  EXPECT_EQ(resetEdgesB, 2u);
  EXPECT_EQ(wrapsA, 1u);
  EXPECT_EQ(wrapsB, 1u);
  EXPECT_EQ(system.cycle(), 1143u);
}

// The same two lanes at the leaf API: an unknown control on one lane neither
// contaminates the other lane nor publishes state, and each lane keeps its own
// clock history across a discarded epoch.
TEST(RegRuntimeTest, TwoClockLanesKeepUnknownOwnControlsIndependentAndFailClosed) {
  gfsim::dff<gfsim::Bits<8>> a("a"), b("b");
  a.init = known<8>(0); b.init = known<8>(0);
  a.Reset(); b.Reset(); a.Xfer(); b.Xfer();
  a.clk = known<1>(1); a.rst = known<1>(0); a.d = known<8>(7);
  b.clk = known<1>(1); b.rst = known<1>(0); b.d = known<8>(9);
  a.Work(); b.Work(); a.Xfer(); b.Xfer();
  EXPECT_EQ(a.q.value(), gfsim::Bits<8>{7});
  EXPECT_EQ(b.q.value(), gfsim::Bits<8>{9});
  lowerClock(a); lowerClock(b);
  unsigned aValue = 7;
  for (bool z : {false, true}) {
    // An X/Z reset on a lane WITHOUT its own rising edge is irrelevant: that
    // lane holds its own value, and the other lane still commits on its edge.
    a.clk = known<1>(0); a.rst = unknownBit(z); a.Work();
    b.clk = known<1>(1); b.rst = known<1>(0); b.d = known<8>(3); b.Work();
    a.Xfer(); b.Xfer();
    EXPECT_EQ(a.q.value(), gfsim::Bits<8>{aValue}) << z;
    EXPECT_EQ(b.q.value(), gfsim::Bits<8>{3}) << z;
    lowerClock(b);
    // The mirror assignment, X and Z included.
    aValue = 200;
    a.clk = known<1>(1); a.rst = known<1>(0); a.d = known<8>(aValue); a.Work();
    b.clk = known<1>(0); b.rst = unknownBit(z); b.Work();
    a.Xfer(); b.Xfer();
    EXPECT_EQ(a.q.value(), gfsim::Bits<8>{aValue}) << z;
    EXPECT_EQ(b.q.value(), gfsim::Bits<8>{3}) << z;
    lowerClock(a);
  }
  // An X or Z reset at a lane's OWN rising edge fails closed and publishes
  // nothing, while the other lane keeps its committed state.
  a.clk = known<1>(1); a.rst = unknownBit(false); a.d = known<8>(11);
  EXPECT_THROW(a.Work(), gfsim::FourStateViolation);
  a.DiscardNext(); a.Xfer();
  EXPECT_EQ(a.q.value(), gfsim::Bits<8>{200});
  EXPECT_EQ(b.q.value(), gfsim::Bits<8>{3});
  a.rst = unknownBit(true); a.d = known<8>(12);
  EXPECT_THROW(a.Work(), gfsim::FourStateViolation);
  a.DiscardNext(); a.Xfer();
  EXPECT_EQ(a.q.value(), gfsim::Bits<8>{200});
  // An unknown own clock fails closed too, at either polarity.
  for (bool z : {false, true}) {
    a.clk = unknownBit(z); a.rst = known<1>(0); a.d = known<8>(13);
    EXPECT_THROW(a.Work(), gfsim::FourStateViolation);
    a.DiscardNext(); a.Xfer();
    EXPECT_EQ(a.q.value(), gfsim::Bits<8>{200}) << z;
    EXPECT_EQ(b.q.value(), gfsim::Bits<8>{3}) << z;
  }
  // Discard cancels clock history as well, so the same own edge is retryable.
  a.clk = known<1>(1); a.rst = known<1>(0); a.d = known<8>(201);
  a.Work(); a.Xfer();
  EXPECT_EQ(a.q.value(), gfsim::Bits<8>{201});
  EXPECT_EQ(b.q.value(), gfsim::Bits<8>{3});
  lowerClock(a); lowerClock(b);
}
