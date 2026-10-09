#include "pycircuit_system.hpp"
#include <cstdlib>
int main() {
  top dut("wide_constant"); dut.Work();
  static_assert(decltype(dut.q)::width == 130);
  if (!dut.q.isFullyKnown() || dut.q.value().word(0) != 123 ||
      dut.q.value().word(1) != 0 || dut.q.value().word(2) != 2)
    std::abort();
}
