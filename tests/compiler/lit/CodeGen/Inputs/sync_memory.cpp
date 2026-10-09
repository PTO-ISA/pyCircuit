#include "pycircuit_system.hpp"
#include <cstdlib>
template <unsigned W> auto known(unsigned value) {
  return gfsim::wire<gfsim::Bits<W>>::known(gfsim::Bits<W>{value});
}
void require(bool value) { if (!value) std::abort(); }
void falling(top &dut) { dut.clk = known<1>(0); dut.Work(); dut.Xfer(); }
void rising(top &dut) { dut.clk = known<1>(1); dut.Work(); dut.Xfer(); dut.Work(); }
void expect(top &dut, unsigned value) {
  require(dut.rdata.isFullyKnown() && dut.rdata.value() == gfsim::Bits<13>{value});
}
int main() {
  top dut("memory"); dut.Build();
  dut.clk = known<1>(0); dut.rst = known<1>(0);
  dut.ren = known<1>(1); dut.raddr = known<4>(3);
  dut.wvalid = known<1>(1); dut.waddr = known<4>(3);
  dut.wdata = known<13>(0x1abc); dut.wstrb = known<2>(1);
  dut.Reset(); dut.Xfer(); rising(dut); expect(dut, 0);
  falling(dut); dut.wvalid = known<1>(0); rising(dut); expect(dut, 0xbc);
  falling(dut); dut.wvalid = known<1>(1); dut.wstrb = known<2>(2);
  rising(dut); expect(dut, 0xbc);
  falling(dut); dut.wvalid = known<1>(0); rising(dut); expect(dut, 0x1abc);
  falling(dut); dut.clk = known<1>(1); dut.wvalid = known<1>(1);
  dut.wdata = known<13>(0); dut.wstrb = known<2>(3);
  dut.Work(); dut.DiscardNext(); dut.Xfer();
  dut.wvalid = known<1>(0); rising(dut); expect(dut, 0x1abc);
  falling(dut); dut.raddr = known<4>(14); rising(dut); expect(dut, 0);
}
