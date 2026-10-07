#include "gfsim/byte_mem.h"
#include "gfsim/sync_mem.h"
#include "gtest/gtest.h"

namespace {
struct MemoryPayload { gfsim::Bits<5> tag; gfsim::Bits<8> byte; };
}
namespace gfsim {
template <> struct hardware_traits<MemoryPayload>
    : hardware_struct_traits<MemoryPayload, &MemoryPayload::tag, &MemoryPayload::byte> {};
}
namespace {
template <unsigned W> auto known(unsigned long long value) {
  return gfsim::wire<gfsim::Bits<W>>::known(gfsim::Bits<W>{value});
}
template <class Mem> void fallingEdge(Mem &mem) {
  mem.clk = known<1>(0); mem.Work(); mem.Xfer();
}
TEST(TypedMemoryTest, SyncReadWriteCollisionOldDataPartialStrobeAndDiscard) {
  gfsim::sync_mem<gfsim::Bits<13>, 4, 7> mem("mem");
  mem.pokeEntry(3, gfsim::Bits<13>{0x1234});
  mem.Reset(); mem.Xfer(); EXPECT_FALSE(mem.rdata.isFullyKnown());
  mem.clk = known<1>(1); mem.rst = known<1>(0);
  mem.ren = known<1>(1); mem.raddr = known<4>(3);
  mem.wvalid = known<1>(1); mem.waddr = known<4>(3);
  mem.wdata = known<13>(0x1abc); mem.wstrb = known<2>(1);
  mem.Work(); EXPECT_EQ(mem.peekEntry(3), gfsim::Bits<13>{0x1234});
  EXPECT_FALSE(mem.rdata.isFullyKnown());
  mem.Xfer(); EXPECT_EQ(mem.rdata.value(), gfsim::Bits<13>{0x1234});
  EXPECT_EQ(mem.peekEntry(3), gfsim::Bits<13>{0x12bc});
  fallingEdge(mem); mem.clk = known<1>(1); mem.wstrb = known<2>(2);
  mem.Work(); mem.DiscardNext(); mem.Xfer();
  EXPECT_EQ(mem.peekEntry(3), gfsim::Bits<13>{0x12bc});
  mem.Work(); mem.Xfer();
  EXPECT_EQ(mem.rdata.value(), gfsim::Bits<13>{0x12bc});
  EXPECT_EQ(mem.peekEntry(3), gfsim::Bits<13>{0x1abc});
  fallingEdge(mem); mem.clk = known<1>(1); mem.ren = known<1>(0);
  mem.wvalid = known<1>(0); mem.Work(); mem.Xfer();
  EXPECT_TRUE(mem.rdata.isFullyKnown());
  fallingEdge(mem); mem.clk = known<1>(1); mem.Work(); mem.Xfer();
  EXPECT_FALSE(mem.rdata.isFullyKnown());
}
TEST(TypedMemoryTest, SyncBoundsUnknownControlsAndResetKeepContents) {
  gfsim::sync_mem<gfsim::Bits<8>, 70, 3> mem("wide_address");
  mem.pokeEntry(1, gfsim::Bits<8>{91}); mem.Reset(); mem.Xfer();
  mem.clk = known<1>(1); mem.rst = known<1>(0); mem.ren = known<1>(1);
  mem.raddr = gfsim::wire<gfsim::Bits<70>>::known(gfsim::shl(gfsim::Bits<70>{1}, 69));
  mem.wvalid = known<1>(0); mem.Work(); mem.Xfer();
  EXPECT_EQ(mem.rdata.value(), gfsim::Bits<8>{0});
  fallingEdge(mem); mem.clk = known<1>(1);
  mem.wvalid = gfsim::wire<gfsim::Bits<1>>::unknown();
  EXPECT_THROW(mem.Work(), gfsim::FourStateViolation);
  mem.DiscardNext(); mem.Xfer(); EXPECT_EQ(mem.peekEntry(1), gfsim::Bits<8>{91});
  mem.Reset(); mem.Xfer(); EXPECT_FALSE(mem.rdata.isFullyKnown());
  EXPECT_EQ(mem.peekEntry(1), gfsim::Bits<8>{91});
}
TEST(TypedMemoryTest, ByteReadIsCurrentStateAndWritesCommitOnlyAtXfer) {
  gfsim::byte_mem<gfsim::Bits<16>, 5, 9> mem("bytes");
  mem.pokeByte(2, 0x34); mem.pokeByte(3, 0x12); mem.Reset(); mem.Xfer();
  mem.raddr = known<5>(2); mem.clk = known<1>(1); mem.rst = known<1>(0);
  mem.wvalid = known<1>(1); mem.waddr = known<5>(2);
  mem.wdata = known<16>(0xabcd); mem.wstrb = known<2>(2);
  mem.Work(); EXPECT_EQ(mem.rdata.value(), gfsim::Bits<16>{0x1234});
  EXPECT_EQ(mem.peekByte(3), 0x12); mem.Xfer();
  EXPECT_EQ(mem.peekByte(2), 0x34); EXPECT_EQ(mem.peekByte(3), 0xab);
  EXPECT_EQ(mem.rdata.value(), gfsim::Bits<16>{0x1234});
  fallingEdge(mem); EXPECT_EQ(mem.rdata.value(), gfsim::Bits<16>{0xab34});
  mem.clk = known<1>(1); mem.wstrb = known<2>(1);
  mem.Work(); mem.DiscardNext(); mem.Xfer(); EXPECT_EQ(mem.peekByte(2), 0x34);
  mem.raddr = known<5>(8); mem.clk = known<1>(0); mem.Work(); mem.Xfer();
  EXPECT_EQ(mem.rdata.value(), gfsim::Bits<16>{0});
}
TEST(TypedMemoryTest, DualPortReadsShareOldStorageAndIndependentOutputs) {
  gfsim::sync_mem_dp<gfsim::Bits<8>, 3, 5> mem("dual");
  mem.pokeEntry(0, gfsim::Bits<8>{7}); mem.pokeEntry(1, gfsim::Bits<8>{19});
  mem.Reset(); mem.Xfer(); mem.clk = known<1>(1); mem.rst = known<1>(0);
  mem.ren0 = known<1>(1); mem.ren1 = known<1>(1);
  mem.raddr0 = known<3>(0); mem.raddr1 = known<3>(1);
  mem.wvalid = known<1>(1); mem.waddr = known<3>(0);
  mem.wdata = known<8>(101); mem.wstrb = known<1>(1);
  mem.Work(); mem.Xfer();
  EXPECT_EQ(mem.rdata0.value(), gfsim::Bits<8>{7});
  EXPECT_EQ(mem.rdata1.value(), gfsim::Bits<8>{19});
  EXPECT_EQ(mem.peekEntry(0), gfsim::Bits<8>{101});
}
TEST(TypedMemoryTest, StructPayloadStrobesUseDeclaredPackedLayout) {
  gfsim::sync_mem<MemoryPayload, 3, 4> mem("typed");
  const MemoryPayload initial{gfsim::Bits<5>{18}, gfsim::Bits<8>{0x34}};
  const MemoryPayload replacement{gfsim::Bits<5>{26}, gfsim::Bits<8>{0xbc}};
  mem.pokeEntry(2, initial); mem.Reset(); mem.Xfer();
  mem.clk = known<1>(1); mem.rst = known<1>(0);
  mem.ren = known<1>(1); mem.raddr = known<3>(2);
  mem.wvalid = known<1>(1); mem.waddr = known<3>(2);
  mem.wdata = gfsim::wire<MemoryPayload>::known(replacement);
  mem.wstrb = known<2>(1); mem.Work(); mem.Xfer();
  EXPECT_EQ(mem.rdata.value().tag, initial.tag);
  EXPECT_EQ(mem.rdata.value().byte, initial.byte);
  EXPECT_EQ(mem.peekEntry(2).tag, initial.tag);
  EXPECT_EQ(mem.peekEntry(2).byte, replacement.byte);
  fallingEdge(mem); mem.clk = known<1>(1); mem.wstrb = known<2>(2);
  mem.Work(); mem.Xfer();
  EXPECT_EQ(mem.rdata.value().tag, initial.tag);
  EXPECT_EQ(mem.rdata.value().byte, replacement.byte);
  EXPECT_EQ(mem.peekEntry(2).tag, replacement.tag);
  EXPECT_EQ(mem.peekEntry(2).byte, replacement.byte);
}

}
