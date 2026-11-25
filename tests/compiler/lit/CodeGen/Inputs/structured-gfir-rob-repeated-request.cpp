#include "pycircuit_system.hpp"

#include <cstdlib>

template <unsigned Width> auto known(unsigned value) {
  using Bits = gfsim::Bits<Width>;
  return gfsim::wire<Bits>::known(Bits{value});
}

void require(bool ok) {
  if (!ok)
    std::abort();
}

void allocation(const pyc_root &root, unsigned index, unsigned count) {
  require(root.result.isFullyKnown());
  auto result = root.result.value();
  require(result.allocate_accepted.value() == 1);
  require(result.allocated_index.value() == index);
  require(result.count.value() == count);
}

int main() {
  gfsim::WorkExecutor executor{1};
  pyc_root root{"repeated-request", &executor};
  root.Build();
  root.Reset();
  root.Xfer();

  root.pyc_7079635f636c6b = known<1>(0);
  root.pyc_7079635f727374 = known<1>(0);
  root.allocate = known<1>(1);
  root.allocate_tag = known<8>(17);
  root.complete = known<1>(0);
  root.complete_index = known<2>(0);
  root.complete_tag = known<8>(0);
  root.complete_value = known<16>(0);
  root.retire = known<1>(0);
  root.flush = known<1>(0);
  root.Work();
  allocation(root, 0, 1);
  root.Xfer();

  // The same raw data bit now has an X mask. This edge must fail.
  using Bit = gfsim::Bits<1>;
  root.allocate = decltype(root.allocate)::fromPacked(gfsim::FourState<1>::unknown(Bit{1}));
  root.pyc_7079635f636c6b = known<1>(1);
  bool rejected = false;
  try {
    root.Work();
  } catch (const gfsim::FourStateViolation &) {
    rejected = true;
  }
  require(rejected);
  root.DiscardNext();
  root.Xfer();

  root.allocate = known<1>(1);
  root.Work();
  allocation(root, 0, 1);
  root.Xfer();

  // A successful transfer changes the committed state for the same request.
  root.pyc_7079635f636c6b = known<1>(0);
  root.Work();
  allocation(root, 1, 2);
  root.Xfer();
}
