#include "pycircuit_system.hpp"
#include "selected_input_ports.hpp"
#include <cstdlib>
#include <fstream>
#include <string>

void require(bool ok) { if (!ok) std::abort(); }
template <unsigned W> auto bits(const std::string &text) {
  require(text.size() == W);
  unsigned value = 0, known = 0, z = 0;
  for (char bit : text) {
    value = value * 2 + unsigned(bit == '1');
    known = known * 2 + unsigned(bit == '0' || bit == '1');
    z = z * 2 + unsigned(bit == 'z');
  }
  using B = gfsim::Bits<W>;
  return gfsim::wire<B>::fromPacked(gfsim::FourState<W>::fromMasks(B{value}, B{known}, B{z}));
}
template <class T> void same(const T &actual, const std::string &word) {
  const auto expected = bits<T::width>(word).packed(), observed = actual.packed();
  require(observed.knownMask() == expected.knownMask() && observed.zMask() == expected.zMask());
  require((observed.value() & expected.knownMask()) == expected.value());
}
int main(int argc, char **argv) {
  require(argc == 2);
  std::ifstream rows(argv[1]); require(rows.good());
  pyc_root root("selected-clocked-input");
  root.pyc_7079635f636c6b = bits<1>("0"); root.pyc_7079635f727374 = bits<1>("0");
  root.Reset(); root.Xfer();
  std::string clock, reset, offer, data, selector, fallback, run, chosen, old, produced;
  unsigned valid, accepted, discard, count = 0;
  while (rows >> clock >> reset >> offer >> data >> selector >> fallback >> run
              >> valid >> chosen >> old >> produced >> accepted >> discard) {
    root.pyc_7079635f636c6b = bits<1>(clock); root.pyc_7079635f727374 = bits<1>(reset);
    root.offer = bits<1>(offer); root.data = bits<8>(data); root.select = bits<1>(selector);
    root.fallback = bits<8>(fallback); root.run = bits<1>(run);
    root.Work();
    require(root.PYC_RESULT_VALID.isFullyKnown() && root.PYC_RESULT_VALID.value().value() == valid);
    if (valid) same(root.result, chosen);
    same(root.old, old); same(root.producer_old, produced);
    gfsim::SimFailureInfo failure;
    require(root.pyc_validate_checks(failure) == bool(accepted));
    if (!accepted) require(failure.phase == gfsim::SimFailurePhase::Check);
    if (!accepted || discard) root.DiscardNext();
    root.Xfer(); ++count;
  }
  require(rows.eof() && count == 27);
}
