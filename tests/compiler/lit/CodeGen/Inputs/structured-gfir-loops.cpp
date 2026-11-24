#include "pycircuit_system.hpp"
#include <cstdlib>
#include <fstream>

void require(bool condition) { if (!condition) std::abort(); }
template <unsigned W> auto bits(unsigned value) {
  return gfsim::wire<gfsim::Bits<W>>::known(gfsim::Bits<W>{value});
}
int main(int argc, char **argv) {
  require(argc == 2);
  std::ifstream rows(argv[1]); require(rows.good());
  pyc_root root("loops");
  root.pyc_7079635f636c6b = bits<1>(0);
  root.pyc_7079635f727374 = bits<1>(0);
  root.Reset(); root.Xfer();
  unsigned clock, reset, x, y, count = 0;
  std::uint64_t expected;
  while (rows >> std::hex >> clock >> reset >> x >> y >> expected) {
    root.pyc_7079635f636c6b = bits<1>(clock);
    root.pyc_7079635f727374 = bits<1>(reset);
    root.x = bits<8>(x); root.y = bits<8>(y);
    root.Work();
    require(root.result.isFullyKnown());
    require(root.result.packed().value().value() == expected);
    gfsim::SimFailureInfo failure; require(root.pyc_validate_checks(failure));
    root.Xfer(); ++count;
  }
  require(rows.eof() && count == 14);
}
