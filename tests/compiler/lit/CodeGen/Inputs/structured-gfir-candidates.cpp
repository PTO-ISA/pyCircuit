#include "pycircuit_system.hpp"
#include <cstdlib>
#include <iostream>

auto bit(bool value) {
  return gfsim::wire<gfsim::Bits<1>>::known(gfsim::Bits<1>{value});
}

void expect(const pyc_root &root, unsigned total, unsigned snapshot) {
  const auto check = [](const auto &port, unsigned expected) {
    return port.isFullyKnown() && port.value() == gfsim::Bits<8>{expected};
  };
  if (!check(root.total, total) || !check(root.snapshot, snapshot)) {
    std::cerr << "Expected committed total " << total << " and prior total "
              << snapshot << '\n';
    std::abort();
  }
}

int main() {
  gfsim::WorkExecutor executor;
  pyc_root root("candidates", &executor);
  root.pyc_7079635f636c6b = bit(false);
  root.pyc_7079635f727374 = bit(false);
  root.Build();
  root.Reset();
  root.Xfer();

  const auto sample = [&](bool clock, bool reset, unsigned total, unsigned old) {
    root.pyc_7079635f636c6b = bit(clock);
    root.pyc_7079635f727374 = bit(reset);
    root.Work();
    expect(root, total, old);
    root.Xfer();
  };

  sample(false, false, 0, 0);
  root.pyc_7079635f636c6b = bit(true);
  root.Work();
  expect(root, 0, 0);
  root.DiscardNext();
  root.Xfer();

  // A discarded edge must be retryable. Both nested calls share one
  // candidate; the independent Rule sees the committed value.
  sample(true, false, 0, 0);
  sample(true, false, 2, 0);
  sample(false, false, 2, 0);
  sample(true, false, 2, 0);
  sample(true, false, 4, 2);

  sample(false, false, 4, 2);
  sample(true, true, 4, 2);
  sample(true, false, 0, 0);
}
