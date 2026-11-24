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
  pyc_root root("assertions");
  root.pyc_7079635f636c6b = bits<1>("0");
  root.pyc_7079635f727374 = bits<1>("0");
  root.Reset(); root.Xfer();
  std::string clock, reset, predicate, allow, count, outcome;
  unsigned samples = 0;
  while (rows >> clock >> reset >> predicate >> allow >> count >> outcome) {
    root.pyc_7079635f636c6b = bits<1>(clock);
    root.pyc_7079635f727374 = bits<1>(reset);
    root.predicate = bits<1>(predicate); root.allow = bits<1>(allow);
    root.Work();
    require(root.count.isFullyKnown());
    require(root.count.value().value() == std::stoul(count, nullptr, 16));
    gfsim::SimFailureInfo failure;
    require(root.pyc_validate_checks(failure) == (outcome == "C"));
    if (outcome != "C") {
      require(failure.phase == gfsim::SimFailurePhase::Check);
      require(failure.code == "source_check_failed");
      if (outcome == "A")
        require(failure.message == "cycle assertion");
      else
        require(failure.message != "cycle assertion");
      require(!failure.instance.empty() && !failure.sourceJson.empty() && !failure.checkIdJson.empty());
      root.DiscardNext();
    }
    root.Xfer(); ++samples;
  }
  require(rows.eof() && samples == 22);
}
