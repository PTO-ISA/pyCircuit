#include "pycircuit_system.hpp"
#include <cstdlib>
int main() {
  top dut("closed_static"); dut.Work();
  static_assert(decltype(dut.q)::width == 8);
  if (!dut.q.isFullyKnown() || dut.q.value() != gfsim::Bits<8>{165})
    std::abort();
}
