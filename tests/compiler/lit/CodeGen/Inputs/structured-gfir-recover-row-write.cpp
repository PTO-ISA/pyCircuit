#include "pycircuit_system.hpp"

#include <cstdlib>

template <unsigned Width> auto known(unsigned value) {
  using Bits = gfsim::Bits<Width>;
  return gfsim::wire<Bits>::known(Bits{value});
}

void expect(const pyc_root &root, unsigned value, unsigned mask, unsigned z = 0) {
  using Wide = gfsim::Bits<128>;
  using Low = gfsim::Bits<24>;
  const Wide sentinel{0xfedcba9876543210ULL, 0x0123456789abcdefULL};
  const auto &state = root.old.packed();
  // Raw unknown bits follow the true candidate, as well as retaining exact
  // known/Z masks. The RTL oracle checks their observable four-state symbols.
  if (state.value() != gfsim::concat(sentinel, Low{value}) ||
      state.knownMask() != gfsim::concat(Wide::ones(), Low{mask}) ||
      state.zMask() != gfsim::concat(Wide{0}, Low{z}))
    std::abort();
}

int main() {
  using Bit = gfsim::Bits<1>;
  using Flag = gfsim::FourState<1>;
  using Byte = gfsim::FourState<8>;
  gfsim::WorkExecutor executor{1};
  pyc_root root{"row_write", &executor};
  root.Build();
  const auto check = [&](Flag outer, Flag inner, Flag index, unsigned value,
                         unsigned mask, unsigned z = 0,
                         Byte data = Byte::known(gfsim::Bits<8>{0xa5}),
                         Byte tag = Byte::known(gfsim::Bits<8>{0x37})) {
    root.Reset();
    root.Xfer();
    root.outer = decltype(root.outer)::fromPacked(outer);
    root.inner = decltype(root.inner)::fromPacked(inner);
    root.index = decltype(root.index)::fromPacked(index);
    root.data = decltype(root.data)::fromPacked(data);
    root.tag = decltype(root.tag)::fromPacked(tag);
    root.pyc_7079635f727374 = known<1>(0);
    for (bool clock : {false, true}) {
      root.pyc_7079635f636c6b = known<1>(clock);
      root.Work();
      expect(root, 0xa0a0a0, 0xffffff);
      root.Xfer();
    }
    root.pyc_7079635f636c6b = known<1>(0);
    root.Work();
    expect(root, value, mask, z);
    root.Xfer();
  };
  const auto zero = Flag::known(Bit{0});
  const auto one = Flag::known(Bit{1});
  const auto x = Flag::unknown();
  const auto z = Flag::highImpedance();
  check(zero, x, z, 0xa0a037, 0xffffff);
  check(one, one, one, 0xa0a537, 0xffffff);
  check(one, one, x, 0xa5a537, 0xfafaff);
  check(x, one, z, 0xa5a537, 0xfafaff);
  check(z, one, one, 0xa0a537, 0xfffaff);
  check(x, z, zero, 0xa5a037, 0xfaffff);
  check(one, one, zero, 0xa5a037, 0xf0ff00, 0x0a00ff,
        Byte::fromMasks(gfsim::Bits<8>{0xa5}, gfsim::Bits<8>{0xf0},
                        gfsim::Bits<8>{0x0a}),
        Byte::highImpedance(gfsim::Bits<8>{0x37}));

  // Discard publishes neither the indexed write nor its direct sibling, and
  // does not consume the rising edge. The same-high retry commits both.
  check(one, one, one, 0xa0a537, 0xffffff);
  root.index = known<1>(0); root.data = known<8>(0x3c); root.tag = known<8>(0x88);
  root.pyc_7079635f636c6b = known<1>(1);
  root.Work(); expect(root, 0xa0a537, 0xffffff);
  root.DiscardNext(); root.Xfer();
  root.Work(); expect(root, 0xa0a537, 0xffffff); root.Xfer();
  root.pyc_7079635f636c6b = known<1>(0);
  root.Work(); expect(root, 0x3ca588, 0xffffff); root.Xfer();

  root.pyc_7079635f727374 = known<1>(1);
  root.pyc_7079635f636c6b = known<1>(1);
  root.Work(); root.Xfer();
  root.pyc_7079635f727374 = known<1>(0);
  root.pyc_7079635f636c6b = known<1>(0);
  root.Work(); expect(root, 0xa0a0a0, 0xffffff); root.Xfer();
}
