#include "gfsim/SimExecutor.h"
#include "pycircuit_system.hpp"
#include <cstdlib>

template <unsigned W> auto known(unsigned value) {
  return gfsim::wire<gfsim::Bits<W>>::known(gfsim::Bits<W>{value});
}
void require(bool value) { if (!value) std::abort(); }
int main() {
  gfsim::WorkExecutor pool(2);
  pyc_root root("clocked_feedback", &pool);
  unsigned left = 0, right = 0;
  bool lastClock = false;
  auto drive = [&](bool clock, bool reset) {
    root.pyc_7079635f636c6b = known<1>(clock);
    root.pyc_7079635f727374 = known<1>(reset);
    root.late_valid = known<1>(0);
  };
  auto check = [&] {
    require(root.left.isFullyKnown() && root.right.isFullyKnown());
    require(root.left.packed().value() == gfsim::Bits<8>{left});
    require(root.right.packed().value() == gfsim::Bits<8>{right});
  };
  auto step = [&](bool clock, bool reset = false) {
    drive(clock, reset);
    root.Work();
    check();
    root.Xfer();
    if (clock && !lastClock) {
      if (reset) left = right = 0;
      else {
        const auto previousLeft = left;
        left = (right + 1) & 255;
        right = (previousLeft + 3) & 255;
      }
    }
    lastClock = clock;
  };
  drive(false, false);
  root.Reset();
  root.Xfer();
  step(false);
  step(true);
  step(true); // held high does not advance either child
  step(false);
  step(false);
  step(true);
  step(false);
  // Cancelling an otherwise valid sample must also cancel both clock histories.
  drive(true, false);
  root.Work(); check(); root.DiscardNext(); root.Xfer();
  step(true);
  step(false);
  for (bool z : {false, true}) {
    // Both FIFOs are empty. The second instance's invalid raw control fails
    // after independent siblings may already have prepared state and history.
    drive(true, false);
    root.late_valid = gfsim::wire<gfsim::Bits<1>>::fromPacked(
        z ? gfsim::FourState<1>::highImpedance() : gfsim::FourState<1>::unknown());
    bool failed = false;
    try { root.Work(); } catch (const gfsim::FourStateViolation &) { failed = true; }
    require(failed);
    root.DiscardNext(); root.Xfer();
    step(true); // same high level retries the cancelled rising edge
    step(false);
  }
  step(true, true);
  step(false);
  step(true);
  step(false);
  // Enough updates to exercise byte wrapping and distinct instance identity.
  for (unsigned i = 0; i != 130; ++i) { step(true); step(false); }
}
