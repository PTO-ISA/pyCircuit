#include "pycircuit_system.hpp"
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
  pyc_root root("two_domains");
  root.clk_e = bits<1>("0"); root.rst_e = bits<1>("0");
  root.clk_w = bits<1>("0"); root.rst_w = bits<1>("0");
  root.allow_e = bits<1>("1"); root.allow_w = bits<1>("1");
  root.Reset(); root.Xfer();
  std::string ce, re, cw, rw, ae, aw, east, west;
  unsigned accepted, samples = 0;
  while (rows >> ce >> re >> cw >> rw >> ae >> aw >> east >> west >> accepted) {
    root.clk_e = bits<1>(ce); root.rst_e = bits<1>(re);
    root.clk_w = bits<1>(cw); root.rst_w = bits<1>(rw);
    root.allow_e = bits<1>(ae); root.allow_w = bits<1>(aw);
    root.Work();
    expect(root.east_value, east); expect(root.west_value, west);
    gfsim::SimFailureInfo failure;
    require(root.pyc_validate_checks(failure) == bool(accepted));
    if (!accepted) {
      require(failure.phase == gfsim::SimFailurePhase::Check);
      root.DiscardNext();
    }
    root.Xfer(); ++samples;
  }
  require(rows.eof() && samples == 16);
}
