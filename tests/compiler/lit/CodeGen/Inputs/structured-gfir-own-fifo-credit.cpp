#include "pycircuit_system.hpp"
#include <array>
#include <cstdlib>
#include <deque>
#include <iostream>

void require(bool ok) { if (!ok) std::abort(); }
template <unsigned W> auto known(unsigned value) {
  return gfsim::wire<gfsim::Bits<W>>::known(gfsim::Bits<W>{value});
}
auto control(char value) {
  using B = gfsim::Bits<1>;
  return gfsim::wire<B>::fromPacked(gfsim::FourState<1>::fromMasks(
      B{value == '1'}, B{value == '0' || value == '1'}, B{value == 'z'}));
}
struct Row { unsigned clock, reset, data; char take, late; bool discard = false; };
int main() {
  const std::array<Row, 30> rows{{
      {0,0,0x11,'0','1'}, {1,0,0x11,'0','1'}, // Unselected pop fills an empty FIFO.
      {0,0,0x22,'0','1'}, {1,0,0x22,'0','1'}, // Full, without pop credit.
      {0,0,0x22,'1','1'}, {1,0,0x22,'1','1'}, // Full same-edge replacement.
      {1,0,0x99,'1','1'},                     // Held high keeps the old slot.
      {0,0,0x33,'0','x'}, {1,0,0x33,'0','x'}, // Unavailable push bypasses late X.
      {0,0,0x33,'1','0'}, {1,0,0x33,'1','0'}, // Late false cancels both directions.
      {0,0,0x44,'1','x'}, {1,0,0x44,'1','x'},
      {1,0,0x44,'1','z'}, {1,0,0x44,'1','1'}, // Failed high retries the same edge.
      {0,0,0x55,'1','1'}, {1,0,0x55,'1','1',true},
      {1,0,0x55,'1','1'},                     // Explicit discard also keeps the edge.
      {0,0,0x66,'x','1'}, {1,0,0x66,'x','1'}, {1,0,0x66,'1','1'},
      {0,0,0x77,'z','1'}, {1,0,0x77,'z','1'}, {1,0,0x77,'1','1'},
      {0,1,0x88,'0','x'}, {1,1,0x88,'1','1'}, // Reset clears a full slot.
      {1,0,0x88,'1','z'},                     // Held, empty: selected out is unavailable.
      {0,0,0x99,'0','1'}, {1,0,0x99,'0','1'}, {0,0,0xaa,'1','1'},
  }};
  std::deque<unsigned> queue;
  unsigned history = 0, faults = 0, replacements = 0;
  pyc_root root("own-fifo-credit");
  root.pyc_7079635f636c6b = known<1>(0); root.pyc_7079635f727374 = known<1>(0);
  root.Reset(); root.Xfer();
  for (const Row row : rows) {
    // Independent one-slot transaction model; no expected value is read from DUT.
    const bool present = !queue.empty(), rising = row.clock && !history;
    const bool selected = row.take == '0' || row.take == '1';
    const bool canRun = selected && ((row.take == '0' && !present) ||
                                     (row.take == '1' && present));
    const bool grant = canRun && row.late == '1';
    const bool pop = grant && row.take == '1';
    const bool ready = !present || pop;
    const bool unknownLate = row.late == 'x' || row.late == 'z';
    const bool fault = rising && !row.reset && (!selected || (canRun && unknownLate));
    const unsigned head = present ? queue.front() : 0;
    root.take = control(row.take); root.data = known<16>(row.data); root.late = control(row.late);
    root.pyc_7079635f636c6b = known<1>(row.clock);
    root.pyc_7079635f727374 = known<1>(row.reset);
    root.Work();
    require(root.ready.isFullyKnown() && root.ready.value().value() == unsigned(ready));
    require(root.present.isFullyKnown() && root.present.value().value() == unsigned(present));
    require(root.head.isFullyKnown() && root.head.value().value() == head);
    gfsim::SimFailureInfo failure;
    require(root.pyc_validate_checks(failure) == !fault);
    if (fault) require(failure.phase == gfsim::SimFailurePhase::Check);
    if (fault || row.discard) root.DiscardNext();
    root.Xfer();
    std::cout << row.clock << ' ' << row.reset << ' ' << row.data << ' '
              << row.take << ' ' << row.late << ' ' << ready << ' ' << present
              << ' ' << head << ' ' << !fault << ' ' << row.discard << '\n';
    if (fault || row.discard) { faults += fault; continue; }
    if (rising) {
      if (row.reset) queue.clear();
      else if (grant) {
        if (pop) { queue.pop_front(); ++replacements; }
        queue.push_back(row.data);
      }
    }
    require(queue.size() <= 1);
    history = row.clock;
  }
  require(faults == 4 && replacements == 5 && queue.size() == 1 && queue.front() == 0x99);
}
