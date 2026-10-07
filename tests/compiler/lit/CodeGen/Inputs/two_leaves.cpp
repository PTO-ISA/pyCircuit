#include "pycircuit_system.hpp"
#include <cstdlib>

template <unsigned W> auto known(unsigned value) {
  return gfsim::wire<gfsim::Bits<W>>::known(gfsim::Bits<W>{value});
}
void require(bool condition) { if (!condition) std::abort(); }
void expect(const top &dut, unsigned a, unsigned b) {
  require(dut.qa.isFullyKnown() && dut.qb.isFullyKnown());
  require(dut.qa.value() == gfsim::Bits<8>{a});
  require(dut.qb.value() == gfsim::Bits<8>{b});
}
int main() {
  top dut("test"); dut.Build();
  dut.init = known<8>(3); dut.clk = known<1>(0);
  dut.rst = known<1>(0); dut.en = known<1>(1); dut.d = known<8>(0x96);
  dut.Reset(); dut.Xfer(); dut.Work(); expect(dut, 3, 3);
  dut.clk = known<1>(1); dut.Work(); expect(dut, 3, 3);
  dut.Xfer(); dut.Work(); expect(dut, 0x96, 0x69);
  dut.clk = known<1>(0); dut.Work(); dut.Xfer();
  dut.clk = known<1>(1); dut.en = known<1>(0); dut.d = known<8>(11);
  dut.Work(); dut.Xfer(); dut.Work(); expect(dut, 0x96, 0x69);
  dut.clk = known<1>(0); dut.Work(); dut.Xfer();
  dut.clk = known<1>(1); dut.en = known<1>(1); dut.d = known<8>(23);
  dut.Work(); dut.DiscardNext(); dut.Xfer(); dut.Work(); expect(dut, 0x96, 0x69);
  dut.Xfer(); dut.Work(); expect(dut, 23, 232);
  dut.clk = known<1>(0); dut.Work(); dut.Xfer();
  dut.clk = known<1>(1); dut.rst = known<1>(1);
  dut.Work(); dut.Xfer(); dut.Work(); expect(dut, 3, 3);
}
