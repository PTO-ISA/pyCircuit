#include "pycircuit_system.hpp"
#include "nested_method_ports.hpp"
#include <cstdlib>
#include <fstream>
#include <string>

void require(bool ok) { if (!ok) std::abort(); }
auto level(const std::string &text) {
  using B = gfsim::Bits<1>; using F = gfsim::FourState<1>;
  return gfsim::wire<B>::fromPacked(text == "x" ? F::unknown() :
      text == "z" ? F::highImpedance() : F::known(B{std::stoul(text)}));
}
template <class T> void expect(const T &value, unsigned expected) {
  require(value.isFullyKnown() && value.packed().value().value() == expected);
}
int main(int argc, char **argv) {
  require(argc == 2);
  std::ifstream rows(argv[1]); require(rows.good());
  pyc_root root("nested-child-method");
  root.pyc_7079635f636c6b = level("0"); root.pyc_7079635f727374 = level("0");
  root.Reset(); root.Xfer();
  std::string clock, reset, allow, index;
  unsigned total, packedRows, selected, candidate, available, accepted, count = 0;
  while (rows >> clock >> reset >> allow >> index >> std::hex >> total >> packedRows
              >> selected >> candidate >> available >> accepted) {
    root.pyc_7079635f636c6b = level(clock); root.pyc_7079635f727374 = level(reset);
    root.allow = level(allow); root.index = level(index);
    root.Work();
    expect(root.q, (total << 8) | 0x5a); expect(root.rows, packedRows);
    expect(root.observed, total); expect(root.observed_row, selected);
    expect(root.untouched, 0x5a); expect(root.untouched_rows, 0);
    expect(root.PYC_CANDIDATE_VALID, available);
    if (available) expect(root.candidate, candidate);
    gfsim::SimFailureInfo failure;
    require(root.pyc_validate_checks(failure) == bool(accepted));
    if (!accepted) {
      require(failure.phase == gfsim::SimFailurePhase::Check);
      root.DiscardNext();
    }
    root.Xfer(); ++count;
  }
  require(rows.eof() && count == 22);
}
