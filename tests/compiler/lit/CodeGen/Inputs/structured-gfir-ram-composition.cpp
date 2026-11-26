#include "pycircuit_system.hpp"
#include "ram_composition_ports.hpp"
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
template <class T> void same(const T &value, const std::string &word) {
  const auto expected = bits<T::width>(word).packed(), actual = value.packed();
  require(actual.knownMask() == expected.knownMask() && actual.zMask() == expected.zMask());
  require((actual.value() & expected.knownMask()) == expected.value());
}
template <class T> void flag(const T &value, unsigned expected) {
  require(value.isFullyKnown() && value.value().value() == expected);
}
int main(int argc, char **argv) {
  require(argc == 2);
  std::ifstream rows(argv[1]); require(rows.good());
  pyc_root root("ram-composition");
  root.pyc_7079635f636c6b = bits<1>("0"); root.pyc_7079635f727374 = bits<1>("0");
  root.Reset(); root.Xfer();
  std::string clock, reset, active, addr, data, mask, late, response, value, count, head;
  unsigned valid, ready, present, accepted, discard, samples = 0;
  while (rows >> clock >> reset >> active >> addr >> data >> mask >> late >> response
              >> valid >> value >> count >> ready >> present >> head >> accepted >> discard) {
    root.pyc_7079635f636c6b = bits<1>(clock); root.pyc_7079635f727374 = bits<1>(reset);
    root.active = bits<1>(active); root.addr = bits<1>(addr); root.data = bits<8>(data);
    root.mask = bits<1>(mask); root.late = bits<1>(late); root.response = bits<1>(response);
    root.Work();
    flag(root.PYC_VALUE_VALID, valid);
    if (valid) same(root.value, value);
    same(root.count, count);
#ifdef MIXED
    flag(root.ready, ready); flag(root.present, present); same(root.head, head);
#endif
    gfsim::SimFailureInfo failure;
    require(root.pyc_validate_checks(failure) == bool(accepted));
    if (!accepted) require(failure.phase == gfsim::SimFailurePhase::Check);
    if (!accepted || discard) root.DiscardNext();
    root.Xfer(); ++samples;
  }
#ifdef MIXED
  require(rows.eof() && samples == 28);
#else
  require(rows.eof() && samples == 27);
#endif
}
