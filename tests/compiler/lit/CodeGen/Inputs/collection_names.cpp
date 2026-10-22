#include "pycircuit_system.hpp"
#include <cstdlib>
int main() {
  top dut("names");
  dut.Build();
  using Signal = gfsim::wire<gfsim::Bits<8>>;
  dut.PYC_COUNT = Signal::known(gfsim::Bits<8>{1});
  dut.pyc_696d706c656d656e746174696f6e5f = Signal::known(gfsim::Bits<8>{2});
  dut.pyc_6e5f5f30 = Signal::known(gfsim::Bits<8>{4});
  dut.Work();
  if (!dut.a.isFullyKnown() || dut.a.value() != gfsim::Bits<8>{1} ||
      !dut.b.isFullyKnown() || dut.b.value() != gfsim::Bits<8>{2} ||
      !dut.c.isFullyKnown() || dut.c.value() != gfsim::Bits<8>{4} ||
      !dut.q.isFullyKnown() || dut.q.value() != gfsim::Bits<8>{7})
    std::abort();
}
