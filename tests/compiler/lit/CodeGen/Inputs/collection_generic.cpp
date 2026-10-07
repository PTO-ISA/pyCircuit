#include "pycircuit_system.hpp"
#include <cstdlib>
int main() {
  top dut("generic");
  dut.Build();
  using Signal = gfsim::wire<gfsim::Bits<8>>;
  dut.x.element(0) = Signal::known(gfsim::Bits<8>{9});
  dut.x.element(1) = Signal::fromPacked(gfsim::FourState<8>::highImpedance());
  dut.Work();
  static_assert(std::remove_cvref_t<decltype(dut.out)>::size == 8);
  for (unsigned i = 0; i < 4; ++i) {
    if (!dut.out.element(i).isFullyKnown() ||
        dut.out.element(i).value() != gfsim::Bits<8>{9})
      std::abort();
    if (dut.out.element(i + 4).packed().zMask() != gfsim::Bits<8>::ones())
      std::abort();
  }
}
