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
int main(int argc, char **argv) {
  require(argc == 2);
  std::ifstream rows(argv[1]); require(rows.good());
  pyc_root root("guard_consumer");
  root.pyc_7079635f636c6b = bits<1>("0");
  root.pyc_7079635f727374 = bits<1>("0");
  root.Reset(); root.Xfer();
  std::string clock, reset, offer, payload, enable, late, count, returned;
  unsigned accepted, samples = 0;
  while (rows >> clock >> reset >> offer >> payload >> enable >> late >> count >> returned >> accepted) {
    root.pyc_7079635f636c6b = bits<1>(clock);
    root.pyc_7079635f727374 = bits<1>(reset);
    root.offer = bits<1>(offer); root.payload = bits<1>(payload); root.enable = bits<1>(enable);
    root.late = bits<1>(late);
    root.Work();
    require(root.count.isFullyKnown());
    require(root.count.value().value() == std::stoul(count, nullptr, 16));
    require(root.return_count.isFullyKnown());
    require(root.return_count.value().value() == std::stoul(returned, nullptr, 16));
    gfsim::SimFailureInfo failure;
    require(root.pyc_validate_checks(failure) == bool(accepted));
    if (!accepted) {
      require(failure.phase == gfsim::SimFailurePhase::Check);
      root.DiscardNext();
    }
    root.Xfer(); ++samples;
  }
  require(rows.eof() && samples == 23);
}
