#include "pycircuit_system.hpp"
#include <cstdlib>
void require(bool value) {
  if (!value)
    std::abort();
}
int main() {
  top dut("field-feedback");
  dut.Build();
  dut.pyc_696e707574 = gfsim::wire<gfsim::Bits<8>>::known(gfsim::Bits<8>{0x96});
  dut.Work();
  require(dut.pair.isFullyKnown() &&
          dut.pair.packed().value() == gfsim::Bits<16>{0x9696});
  require(dut.a.isFullyKnown() && dut.a.value() == gfsim::Bits<8>{0x96});
  require(dut.b.isFullyKnown() && dut.b.value() == gfsim::Bits<8>{0x96});
  dut.pyc_696e707574 =
      gfsim::wire<gfsim::Bits<8>>::fromPacked(gfsim::FourState<8>::fromMasks(
          gfsim::Bits<8>{0xa5}, gfsim::Bits<8>{0xf0}, gfsim::Bits<8>{0x08}));
  dut.Work();
  require(dut.pair.packed().knownMask() == gfsim::Bits<16>{0xf0f0});
  require(dut.pair.packed().zMask() == gfsim::Bits<16>{0x0808});
  require((dut.pair.packed().value() & gfsim::Bits<16>{0xf0f0}) ==
          gfsim::Bits<16>{0xa0a0});
  require(dut.a.packed().knownMask() == gfsim::Bits<8>{0xf0} &&
          dut.b.packed().knownMask() == gfsim::Bits<8>{0xf0});
  require(dut.a.packed().zMask() == gfsim::Bits<8>{0x08} &&
          dut.b.packed().zMask() == gfsim::Bits<8>{0x08});
}
