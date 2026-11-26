#include "pycircuit_system.hpp"
#include "byte_ports.hpp"
#include <cstdlib>
#include <fstream>
#include <string>

void require(bool condition) { if (!condition) std::abort(); }
template <unsigned W> auto bits(const std::string &text) {
  using B = gfsim::Bits<W>; using F = gfsim::FourState<W>;
  return gfsim::wire<B>::fromPacked(text == "x" ? F::unknown() :
      text == "z" ? F::highImpedance() : F::known(B{std::stoul(text, nullptr, 16)}));
}
template <class T> void expect(const T &value, const std::string &text) {
  require(value.isFullyKnown() && value.value().value() == std::stoul(text, nullptr, 16));
}
int main(int argc, char **argv) {
  require(argc == 2);
  std::ifstream rows(argv[1]); require(rows.good());
  pyc_root root("bytes");
  root.pyc_7079635f636c6b = bits<1>("0");
  root.pyc_7079635f727374 = bits<1>("0");
  root.Reset(); root.Xfer();
  std::string action, clock, reset, before, after, address, write, waddr, data,
      mask, valid, expected, counter;
  unsigned count = 0;
  while (rows >> action >> clock >> reset >> before >> after >> address >> write
              >> waddr >> data >> mask >> valid >> expected >> counter) {
    root.pyc_7079635f636c6b = bits<1>(clock);
    root.pyc_7079635f727374 = bits<1>(reset);
    root.earlier = bits<1>(before); root.later = bits<1>(after);
    root.address = bits<2>(address); root.write = bits<1>(write);
    root.waddr = bits<2>(waddr); root.data = bits<16>(data); root.mask = bits<2>(mask);
    bool failed = false;
    try { root.Work(); } catch (const gfsim::FourStateViolation &) { failed = true; }
    require(failed == (action == "F"));
    if (failed) {
      root.DiscardNext();
    } else {
      expect(root.PYC_VALUE_VALID, valid); expect(root.count, counter);
      if (valid == "1") expect(root.value, expected);
      gfsim::SimFailureInfo failure; require(root.pyc_validate_checks(failure));
    }
    root.Xfer(); ++count;
  }
  require(rows.eof() && count == 24);
}
