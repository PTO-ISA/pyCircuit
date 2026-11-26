#include "pycircuit_system.hpp"
#include "credit_ports.hpp"
#include <array>
#include <cstdlib>
#include <deque>
#include <iostream>

void require(bool ok) { if (!ok) std::abort(); }
template <unsigned W> auto known(unsigned value) {
  return gfsim::wire<gfsim::Bits<W>>::known(gfsim::Bits<W>{value});
}
auto control(char value) {
  using B = gfsim::Bits<16>;
  return gfsim::wire<B>::fromPacked(gfsim::FourState<16>::fromMasks(
      B{value == '1' ? 1u : 0u}, B{value == '0' || value == '1' ? 0xffffu : 0xfffeu},
      B{value == 'z' ? 1u : 0u}));
}
struct Row { unsigned clock, reset, left; char right; };
int main() {
  const std::array<Row, 28> rows{{
      {0,0,10,'x'}, {1,0,10,'x'}, // Empty out suppresses the later unknown guard.
      {0,0,11,'0'}, {1,0,11,'0'}, // Fill both stages.
      {0,0,12,'0'}, {1,0,12,'0'}, // Full: late false must block every replacement.
      {0,0,13,'1'}, {1,0,13,'1'}, // Pop10, move11, accept13 simultaneously.
      {1,0,99,'0'},              // Held high must not commit another request.
      {0,0,14,'x'}, {1,0,14,'x'}, // Reached post-out X fails and rolls back.
      {1,0,14,'0'},              // Same-high false retry consumes nothing.
      {0,0,15,'z'}, {1,0,15,'z'}, // Reached Z also preserves old slots/edge.
      {1,0,15,'1'},              // Same-high successful retry still returns11.
      {0,0,16,'1'}, {1,0,16,'1'},
      {0,0,17,'1'}, {1,0,17,'1'},
      {0,1,18,'0'}, {1,1,18,'0'}, // Reset clears a full path.
      {1,0,19,'1'},              // Held high after reset remains empty.
      {0,0,20,'1'}, {1,0,20,'1'},
      {0,0,21,'1'}, {1,0,21,'1'},
      {0,0,22,'1'}, {1,0,22,'1'},
  }};
  std::deque<unsigned> ingress, egress;
  unsigned history = 0, faults = 0, blocked = 0, replacements = 0;
  gfsim::WorkExecutor executor{1};
  pyc_root root("local-fifo-credit", &executor);
  root.Build(); root.Reset(); root.Xfer();
  for (const Row r : rows) {
    const bool rising = r.clock && !history;
    const bool pop = !egress.empty() && r.right == '1';
    const bool move = !ingress.empty() && (egress.empty() || pop);
    const bool send = ingress.empty() || move;
    const bool fault = rising && !r.reset && !egress.empty() &&
                       (r.right == 'x' || r.right == 'z');
    const unsigned result = egress.empty() ? 0 : egress.front();
    root.left = known<16>(r.left); root.right = control(r.right);
    root.pyc_7079635f636c6b = known<1>(r.clock);
    root.pyc_7079635f727374 = known<1>(r.reset);
    root.Work();
    require(root.PYC_RESULT_VALID.isFullyKnown());
    require(root.PYC_RESULT_VALID.value().value() == unsigned(pop));
    if (pop) require(root.result.isFullyKnown() && root.result.value().value() == result);
    gfsim::SimFailureInfo failure;
    require(root.pyc_validate_checks(failure) == !fault);
    if (fault) {
      require(failure.phase == gfsim::SimFailurePhase::Check);
      root.DiscardNext();
    }
    root.Xfer();
    // Expected rows are derived solely from the independent two-slot model.
    std::cout << r.clock << ' ' << r.reset << ' ' << r.left << ' ' << r.right << ' '
              << pop << ' ' << result << ' ' << !fault << '\n';
    if (fault) { ++faults; continue; }
    if (rising) {
      if (r.reset) { ingress.clear(); egress.clear(); }
      else {
        blocked += !egress.empty() && r.right == '0';
        replacements += !ingress.empty() && !egress.empty() && pop && move && send;
        if (pop) egress.pop_front();
        if (move) { egress.push_back(ingress.front()); ingress.pop_front(); }
        if (send) ingress.push_back(r.left);
      }
    }
    require(ingress.size() <= 1 && egress.size() <= 1);
    history = r.clock;
  }
  require(faults == 2 && blocked >= 2 && replacements >= 3);
}
