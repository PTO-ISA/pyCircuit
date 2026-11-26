#include "pycircuit_system.hpp"
#include "loop_break_ports.hpp"
#include <cstdlib>
#include <fstream>
#include <string>

void require(bool ok) { if (!ok) std::abort(); }
template <unsigned W> auto bits(const std::string &text) {
  unsigned value = 0, known = 0, z = 0;
  require(text.size() == W);
  for (char bit : text) {
    value = value * 2 + unsigned(bit == '1');
    known = known * 2 + unsigned(bit == '0' || bit == '1');
    z = z * 2 + unsigned(bit == 'z');
  }
  using B = gfsim::Bits<W>;
  return gfsim::wire<B>::fromPacked(gfsim::FourState<W>::fromMasks(B{value}, B{known}, B{z}));
}
int main(int argc, char **argv) {
  require(argc == 2);
  std::ifstream rows(argv[1]); require(rows.good());
  pyc_root root("loop-break");
  root.pyc_7079635f636c6b = bits<1>("0"); root.pyc_7079635f727374 = bits<1>("0");
  root.Reset(); root.Xfer();
  std::string clock, reset, stops;
  unsigned q, valid, expected, accepted, count = 0;
  while (rows >> clock >> reset >> stops >> std::hex >> q >> valid >> expected >> accepted) {
    root.pyc_7079635f636c6b = bits<1>(clock); root.pyc_7079635f727374 = bits<1>(reset);
    root.stops = bits<3>(stops);
    root.Work();
    require(root.q.isFullyKnown() && root.q.value().value() == q);
    require(root.PYC_RESULT_VALID.isFullyKnown() && root.PYC_RESULT_VALID.value().value() == valid);
    if (valid) require(root.result.isFullyKnown() && root.result.packed().value().value() == expected);
    gfsim::SimFailureInfo failure;
    require(root.pyc_validate_checks(failure) == bool(accepted));
    if (!accepted) {
      require(failure.phase == gfsim::SimFailurePhase::Check);
      root.DiscardNext();
    }
    root.Xfer(); ++count;
  }
  require(rows.eof() && count == 20);
}
