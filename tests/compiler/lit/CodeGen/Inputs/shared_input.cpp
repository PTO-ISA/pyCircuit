#include "pycircuit_system.hpp"
#include <cstdlib>
int main() {
  top dut("shared"); dut.Build();
  for (unsigned value : {1u, 11u, 128u, 201u, 255u}) {
    dut.x = gfsim::wire<gfsim::Bits<8>>::known(gfsim::Bits<8>{value});
    dut.Work();
    if (!dut.q.isFullyKnown() || dut.q.value() != gfsim::Bits<8>{(2 * value) % 256})
      std::abort();
  }
}
