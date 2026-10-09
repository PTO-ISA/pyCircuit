#include "pycircuit_system.hpp"
#include <cstdlib>

int main() {
  top dut("interleave"); dut.Build();
  for (unsigned a : {0u, 1u, 42u, 85u, 127u, 128u, 255u}) {
    dut.a = gfsim::wire<gfsim::Bits<8>>::known(gfsim::Bits<8>{a});
    dut.Work();
    if (!dut.q.isFullyKnown() || dut.q.value() != gfsim::Bits<8>{255u - a})
      std::abort();
    dut.Xfer();
  }
}
