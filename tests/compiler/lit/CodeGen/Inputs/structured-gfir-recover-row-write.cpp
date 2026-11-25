#include "pycircuit_system.hpp"

#include <cstdlib>

template <unsigned Width> auto known(unsigned value) {
  using Bits = gfsim::Bits<Width>;
  return gfsim::wire<Bits>::known(Bits{value});
}

int main() {
  using Bit = gfsim::Bits<1>;
  using Word = gfsim::Bits<24>;
  using Flag = gfsim::FourState<1>;
  gfsim::WorkExecutor executor{1};
  pyc_root root{"row_write", &executor};
  root.Build();
  const auto check = [&](Flag outer, Flag inner, Flag index, unsigned value, unsigned mask) {
    root.Reset();
    root.Xfer();
    root.outer = decltype(root.outer)::fromPacked(outer);
    root.inner = decltype(root.inner)::fromPacked(inner);
    root.index = decltype(root.index)::fromPacked(index);
    root.data = known<8>(0xa5);
    root.tag = known<8>(0x37);
    root.pyc_7079635f727374 = known<1>(0);
    for (bool clock : {false, true, false}) {
      root.pyc_7079635f636c6b = known<1>(clock);
      root.Work();
      root.Xfer();
    }
    const auto &state = root.old.packed();
    if (state.knownMask() != Word{mask} || state.zMask() != Word{0} ||
        (state.value() & state.knownMask()) != Word{value & mask})
      std::abort();
  };
  const auto zero = Flag::known(Bit{0});
  const auto one = Flag::known(Bit{1});
  const auto x = Flag::unknown();
  const auto z = Flag::highImpedance();
  check(zero, x, z, 0xa0a037, 0xffffff);
  check(one, one, one, 0xa0a537, 0xffffff);
  check(x, one, z, 0xa0a037, 0xfafaff);
  check(z, one, one, 0xa0a037, 0xfffaff);
}
