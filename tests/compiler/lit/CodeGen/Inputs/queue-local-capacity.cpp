#include "pycircuit_system.hpp"
#include <cstdlib>
#include <iostream>
#include <source_location>
static void require(bool ok, std::source_location at = std::source_location::current()) {
  if (!ok) { std::cerr << "local capacity oracle at " << at.line() << '\n'; std::abort(); }
}
template<unsigned W> auto known(unsigned value) {
  return gfsim::wire<gfsim::Bits<W>>::known(gfsim::Bits<W>{value});
}
int main() {
  for (unsigned workers : {1u, 2u}) {
    gfsim::WorkExecutor pool(workers);
    pyc_root root("capacity", &pool);
    root.Build();
    root.clk = root.rst = root.push = root.take = known<1>(0);
    root.data = known<13>(0);
    auto cold = [&] {
      require(!root.ready.isFullyKnown() && !root.available.isFullyKnown() &&
              !root.head.isFullyKnown() && !root.capacity.isFullyKnown());
      require(root.capacity.packed().zMask() == gfsim::Bits<1>{0});
    };
    auto check = [&](bool ready, bool valid, unsigned data, bool capacity) {
      require(root.ready.isFullyKnown() && root.ready.value() == gfsim::Bits<1>{ready});
      require(root.available.isFullyKnown() && root.available.value() == gfsim::Bits<1>{valid});
      require(root.head.isFullyKnown() && root.head.value() == gfsim::Bits<13>{data});
      require(root.capacity.isFullyKnown() && root.capacity.value() == gfsim::Bits<1>{capacity});
    };
    root.Work(); cold(); root.Xfer(); root.Work(); cold(); root.Xfer();
    root.Reset(); root.DiscardNext(); root.Xfer(); root.Work(); cold(); root.Xfer();
    root.Reset(); root.Xfer();
    auto row = [&](bool clk, bool rst, bool push, bool take, unsigned data,
                   bool ready, bool valid, unsigned head, bool capacity,
                   bool commit = true) {
      root.clk=known<1>(clk); root.rst=known<1>(rst);
      root.push=known<1>(push); root.take=known<1>(take); root.data=known<13>(data);
      root.Work(); check(ready,valid,head,capacity);
      if (!commit) root.DiscardNext();
      root.Xfer();
    };
    row(0,0,0,0,0,1,0,0,1);
    row(1,0,1,0,37,1,0,0,1); // committed empty -> full but not eligible
    row(1,0,0,0,0,0,0,0,0); // held high cannot mature the token
    row(1,0,0,1,0,0,0,0,0); // same-epoch pop cannot create local capacity
    row(0,0,0,1,0,0,0,0,0);
    row(1,0,0,1,0,0,0,0,0); // first later rising edge: still waiting
    row(0,0,0,0,0,0,0,0,0);
    row(1,0,0,0,0,0,0,0,0); // second later edge commits maturity
    row(1,0,0,0,0,0,1,37,0);
    for (bool highZ : {false, true}) {
      root.take = gfsim::wire<gfsim::Bits<1>>::fromPacked(
          highZ ? gfsim::FourState<1>::highImpedance() : gfsim::FourState<1>::unknown());
      root.Work();
      require(!root.ready.isFullyKnown());
      require(root.capacity.isFullyKnown() && root.capacity.value() == gfsim::Bits<1>{0});
      require(root.available.value() == gfsim::Bits<1>{1} && root.head.value() == gfsim::Bits<13>{37});
      root.Xfer(); // held high: an unknown pop request causes no transfer
    }
    row(1,0,0,1,0,1,1,37,0); // public ready differs, capacity stays zero
    row(0,0,0,1,0,1,1,37,0);
    row(1,0,1,1,55,1,1,37,0,false); // Discard preserves old full head
    row(1,0,1,1,55,1,1,37,0); // retry full replacement, old head observed
    row(1,0,0,0,0,0,0,0,0);
    row(0,0,0,0,0,0,0,0,0);
    row(1,1,0,0,0,0,0,0,0); // reset takes effect only on the rising commit
    row(0,0,0,0,0,1,0,0,1);
    row(1,0,1,0,91,1,0,0,1,false); // discarded empty push keeps capacity
    row(1,0,1,0,91,1,0,0,1);
    row(0,0,0,0,0,0,0,0,0);
    row(1,0,0,0,0,0,0,0,0);
    row(0,0,0,0,0,0,0,0,0);
    row(1,0,0,0,0,0,0,0,0);
    row(0,0,0,1,0,1,1,91,0);
    row(1,0,0,1,0,1,1,91,0);
    row(0,0,0,0,0,1,0,0,1);
    std::cout << "LOCAL_CAPACITY_CPP_OK workers=" << workers << '\n';
  }
}
