#include "pycircuit_system.hpp"

#include <cstdlib>

int main() {
  pyc_root root{"select_group"};
  using Word = gfsim::Bits<24>;
  using Bit = gfsim::Bits<1>;
  auto sample = [&](const gfsim::FourState<1> &pick, unsigned value, unsigned known) {
    root.pick = decltype(root.pick)::fromPacked(pick);
    root.Work();
    const auto &actual = root.chosen.packed();
    if ((actual.value() & actual.knownMask()) != Word{value & known} ||
        actual.knownMask() != Word{known} || actual.zMask() != Word{0x0000ff})
      std::abort();
  };
  root.yes = decltype(root.yes)::fromPacked(
      gfsim::FourState<24>::fromMasks(Word{0xa50100}, Word{0xffff00}, Word{0x0000ff}));
  root.no = decltype(root.no)::fromPacked(
      gfsim::FourState<24>::fromMasks(Word{0xa58100}, Word{0xffff00}, Word{0x0000ff}));
  sample(gfsim::FourState<1>::known(Bit{1}), 0xa50100, 0xffff00);
  sample(gfsim::FourState<1>::known(Bit{0}), 0xa58100, 0xffff00);
  sample(gfsim::FourState<1>::unknown(Bit{1}), 0xa50100, 0xff7f00);
  sample(gfsim::FourState<1>::highImpedance(), 0xa50100, 0xff7f00);
}
