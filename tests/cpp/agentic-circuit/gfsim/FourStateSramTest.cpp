#include "pyc_four_state.hpp"
#include "pyc_sync_mem.hpp"

#include "gtest/gtest.h"

namespace pyc::cpp {
namespace {

template <typename Memory>
void risingEdge(Memory &memory, Wire<1> &clock) {
  clock = Wire<1>{0};
  memory.tick_compute();
  memory.tick_commit();
  clock = Wire<1>{1};
  memory.tick_compute();
  memory.tick_commit();
}

TEST(FourStateTest, ExactParityComparesValueKnownAndZMasks) {
  auto knownA = FourState<8>::known(Wire<8>{0x5a});
  auto knownB = FourState<8>::known(Wire<8>{0x5a});
  auto unknownA = FourState<8>::unknown(Wire<8>{0x00});
  auto unknownB = FourState<8>::unknown(Wire<8>{0xff});
  auto highZ = FourState<8>::highImpedance(Wire<8>{0xff});

  EXPECT_TRUE(knownA.parityEquivalent(knownB));
  EXPECT_TRUE(unknownA.parityEquivalent(unknownB));
  EXPECT_TRUE(highZ.parityEquivalent(FourState<8>::highImpedance()));
  EXPECT_FALSE(knownA.parityEquivalent(unknownA));
  EXPECT_FALSE(unknownA.parityEquivalent(highZ));
  EXPECT_THROW(FourState<8>::fromMasks(Wire<8>{0}, Wire<8>{1}, Wire<8>{1}),
               std::invalid_argument);
}

TEST(SyncMemFourStateTest, EnforcesOneCycleLiveWindowAndOldDataRead) {
  Wire<1> clk{0};
  Wire<1> rst{0};
  Wire<1> ren{0};
  Wire<2> raddr{0};
  Wire<8> rdata{0};
  Wire<1> wvalid{0};
  Wire<2> waddr{0};
  Wire<8> wdata{0};
  Wire<1> wstrb{1};
  pyc_sync_mem<2, 8, 4, 1> memory(clk, rst, ren, raddr, rdata, wvalid,
                                  waddr, wdata, wstrb);

  EXPECT_FALSE(memory.rdataState().isFullyKnown());
  memory.pokeEntry(1, 0x11);
  ren = Wire<1>{1};
  raddr = Wire<2>{1};
  wvalid = Wire<1>{1};
  waddr = Wire<2>{1};
  wdata = Wire<8>{0x22};
  risingEdge(memory, clk);
  EXPECT_TRUE(memory.rdataLive());
  EXPECT_TRUE(memory.rdataState().isFullyKnown());
  EXPECT_EQ(rdata, Wire<8>{0x11});
  EXPECT_EQ(memory.peekEntry(1), 0x22u);

  wvalid = Wire<1>{0};
  ren = Wire<1>{0};
  risingEdge(memory, clk);
  EXPECT_FALSE(memory.rdataLive());
  EXPECT_FALSE(memory.rdataState().isFullyKnown());

  ren = Wire<1>{1};
  risingEdge(memory, clk);
  EXPECT_TRUE(memory.rdataLive());
  EXPECT_EQ(rdata, Wire<8>{0x22});
  risingEdge(memory, clk);
  EXPECT_TRUE(memory.rdataLive());

  rst = Wire<1>{1};
  risingEdge(memory, clk);
  EXPECT_FALSE(memory.rdataLive());
  EXPECT_FALSE(memory.rdataState().isFullyKnown());
}

TEST(SyncMemFourStateTest, KnownnessIsGatedByActiveControls) {
  Wire<1> clk{0};
  Wire<1> rst{0};
  Wire<1> ren{0};
  Wire<2> raddr{0};
  Wire<8> rdata{0};
  Wire<1> wvalid{0};
  Wire<2> waddr{0};
  Wire<8> wdata{0};
  Wire<1> wstrb{0};
  pyc_sync_mem<2, 8, 4, 1> memory(clk, rst, ren, raddr, rdata, wvalid,
                                  waddr, wdata, wstrb);

  memory.setVerificationInputs(
      FourState<1>::known(Wire<1>{0}), FourState<1>::known(Wire<1>{0}),
      FourState<2>::unknown(), FourState<1>::known(Wire<1>{0}),
      FourState<2>::unknown(), FourState<8>::unknown(),
      FourState<1>::unknown());
  EXPECT_NO_THROW(risingEdge(memory, clk));

  memory.setVerificationInputs(
      FourState<1>::known(Wire<1>{0}), FourState<1>::known(Wire<1>{1}),
      FourState<2>::unknown(), FourState<1>::known(Wire<1>{0}),
      FourState<2>::unknown(), FourState<8>::unknown(),
      FourState<1>::unknown());
  EXPECT_THROW(risingEdge(memory, clk), FourStateViolation);

  ren = Wire<1>{0};
  wvalid = Wire<1>{1};
  memory.setVerificationInputs(
      FourState<1>::known(Wire<1>{0}), FourState<1>::known(Wire<1>{0}),
      FourState<2>::unknown(), FourState<1>::known(Wire<1>{1}),
      FourState<2>::known(Wire<2>{0}), FourState<8>::unknown(),
      FourState<1>::known(Wire<1>{1}));
  EXPECT_THROW(risingEdge(memory, clk), FourStateViolation);
}

TEST(SyncMemFourStateTest, DualPortOutputsExpireIndependently) {
  Wire<1> clk{0};
  Wire<1> rst{0};
  Wire<1> ren0{1};
  Wire<2> raddr0{0};
  Wire<8> rdata0{0};
  Wire<1> ren1{1};
  Wire<2> raddr1{1};
  Wire<8> rdata1{0};
  Wire<1> wvalid{0};
  Wire<2> waddr{0};
  Wire<8> wdata{0};
  Wire<1> wstrb{0};
  pyc_sync_mem_dp<2, 8, 4, 1> memory(
      clk, rst, ren0, raddr0, rdata0, ren1, raddr1, rdata1, wvalid, waddr,
      wdata, wstrb);
  memory.pokeEntry(0, 0x33);
  memory.pokeEntry(1, 0x44);

  risingEdge(memory, clk);
  EXPECT_TRUE(memory.rdataLive(0));
  EXPECT_TRUE(memory.rdataLive(1));
  EXPECT_EQ(rdata0, Wire<8>{0x33});
  EXPECT_EQ(rdata1, Wire<8>{0x44});

  ren0 = Wire<1>{0};
  ren1 = Wire<1>{1};
  risingEdge(memory, clk);
  EXPECT_FALSE(memory.rdataLive(0));
  EXPECT_TRUE(memory.rdataLive(1));
}

} // namespace
} // namespace pyc::cpp
