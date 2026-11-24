#include "pycircuit_system.hpp"
#include "child_method_ports.hpp"
#include <cstdlib>
#include <fstream>
#include <string>

void require(bool condition) { if (!condition) std::abort(); }
template <unsigned W> auto bits(const std::string &text) {
  using B = gfsim::Bits<W>; using F = gfsim::FourState<W>;
  return gfsim::wire<B>::fromPacked(text == "x" ? F::unknown() :
      text == "z" ? F::highImpedance() : F::known(B{std::stoul(text, nullptr, 16)}));
}
template <class T> void expect(const T &value, unsigned expected) {
  require(value.isFullyKnown() && value.value().value() == expected);
}
int main(int argc, char **argv) {
  require(argc == 2);
  std::ifstream rows(argv[1]); require(rows.good());
  pyc_root root("child_methods");
  root.pyc_7079635f636c6b = bits<1>("0");
  root.pyc_7079635f727374 = bits<1>("0");
  root.Reset(); root.Xfer();
  std::string clock, reset, allow;
  unsigned old, available, first, second, accepted, samples = 0;
  while (rows >> clock >> reset >> allow >> std::hex >> old >> available >> first >> second >> accepted) {
    root.pyc_7079635f636c6b = bits<1>(clock);
    root.pyc_7079635f727374 = bits<1>(reset);
    root.allow = bits<1>(allow);
    root.Work();
    expect(root.q, old); expect(root.observed, old);
    expect(root.PYC_FIRST_VALID, available); expect(root.PYC_SECOND_VALID, available);
    if (available) { expect(root.first, first); expect(root.second, second); }
    gfsim::SimFailureInfo failure;
    require(root.pyc_validate_checks(failure) == bool(accepted));
    if (!accepted) {
      require(failure.phase == gfsim::SimFailurePhase::Check);
      root.DiscardNext();
    }
    root.Xfer(); ++samples;
  }
  require(rows.eof() && samples == 22);
}
