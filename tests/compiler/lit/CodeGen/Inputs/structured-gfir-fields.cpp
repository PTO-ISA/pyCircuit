#include "pycircuit_system.hpp"
#include <cstdlib>
#include <fstream>
#include <string>

void require(bool condition) { if (!condition) std::abort(); }
template <unsigned W> auto bits(const std::string &text) {
  using B = gfsim::Bits<W>;
  B value{0}, known{0}, z{0};
  require(text.size() == W);
  for (unsigned i = 0; i < W; ++i) {
    B bit{std::uint64_t{1} << (W - 1 - i)};
    if (text[i] == '0' || text[i] == '1') known = known | bit;
    if (text[i] == '1') value = value | bit;
    if (text[i] == 'z') z = z | bit;
  }
  return gfsim::wire<B>::fromPacked(gfsim::FourState<W>::fromMasks(value, known, z));
}
template <class T> void expect(const T &actual, const std::string &expected) {
  auto reference = bits<40>(expected).packed();
  auto observed = actual.packed();
  require(observed.knownMask() == reference.knownMask());
  require(observed.zMask() == reference.zMask());
  require((observed.value() & reference.knownMask()) == reference.value());
}
int main(int argc, char **argv) {
  require(argc == 2);
  std::ifstream rows(argv[1]); require(rows.good());
  pyc_root root("fields");
  root.pyc_7079635f636c6b = bits<1>("0");
  root.pyc_7079635f727374 = bits<1>("0");
  root.Reset(); root.Xfer();
  std::string clock, reset, raw, data, tag, old, snapshot, candidate;
  unsigned count = 0;
  while (rows >> clock >> reset >> raw >> data >> tag >> old >> snapshot >> candidate) {
    root.pyc_7079635f636c6b = bits<1>(clock);
    root.pyc_7079635f727374 = bits<1>(reset);
    root.raw = bits<2>(raw); root.data = bits<8>(data); root.tag = bits<8>(tag);
    root.Work();
    expect(root.old, old); expect(root.snapshot, snapshot); expect(root.candidate, candidate);
    gfsim::SimFailureInfo failure; require(root.pyc_validate_checks(failure));
    root.Xfer(); ++count;
  }
  require(rows.eof() && count == 16);
}
