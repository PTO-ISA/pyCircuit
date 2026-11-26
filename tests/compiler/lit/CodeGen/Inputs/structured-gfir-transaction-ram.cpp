#include "pycircuit_system.hpp"
#include "ram_ports.hpp"
#include <cstdlib>
#include <fstream>
#include <string>

void require(bool condition) { if (!condition) std::abort(); }
template <unsigned W> auto bits(unsigned value) {
  return gfsim::wire<gfsim::Bits<W>>::known(gfsim::Bits<W>{value});
}
auto level(const std::string &text) {
  using F = gfsim::FourState<1>;
  return gfsim::wire<gfsim::Bits<1>>::fromPacked(
      text == "x" ? F::unknown() : text == "z" ? F::highImpedance()
      : F::known(gfsim::Bits<1>{std::stoul(text, nullptr, 16)}));
}
template <class T> void expect(const T &value, unsigned expected) {
  require(value.isFullyKnown() && value.value().value() == expected);
}
int main(int argc, char **argv) {
  require(argc == 2);
  std::ifstream rows(argv[1]); require(rows.good());
  pyc_root root("ram");
  root.pyc_7079635f636c6b = bits<1>(0);
  root.pyc_7079635f727374 = bits<1>(0);
  root.Reset(); root.Xfer();
  unsigned clock, reset, req0, req1, write, addr0, addr1, waddr, data, mask;
  std::string consume;
  unsigned valid0, expected0, valid1, expected1, bitValid, responseBit, seen,
      accepted, count = 0;
  while (rows >> std::hex >> clock >> reset >> req0 >> req1 >> write >> addr0 >> addr1
              >> waddr >> data >> mask >> consume >> valid0 >> expected0 >> valid1
              >> expected1 >> bitValid >> responseBit >> seen >> accepted) {
    root.pyc_7079635f636c6b = bits<1>(clock);
    root.pyc_7079635f727374 = bits<1>(reset);
    root.req0 = bits<1>(req0); root.req1 = bits<1>(req1); root.write = bits<1>(write);
    root.addr0 = bits<2>(addr0); root.addr1 = bits<2>(addr1); root.waddr = bits<2>(waddr);
    root.data = bits<16>(data); root.mask = bits<2>(mask);
    root.consume = level(consume);
    root.Work();
    expect(root.PYC_A_VALID, valid0); expect(root.PYC_B_VALID, valid1);
    expect(root.PYC_BIT_VALID, bitValid); expect(root.count, seen);
    require(bitValid == valid0);
    if (valid0) expect(root.a, expected0);
    if (valid1) expect(root.b, expected1);
    if (bitValid) expect(root.response_bit, responseBit);
    gfsim::SimFailureInfo failure;
    require(root.pyc_validate_checks(failure) == bool(accepted));
    if (!accepted) {
      require(failure.phase == gfsim::SimFailurePhase::Check);
      root.DiscardNext();
    }
    root.Xfer(); ++count;
  }
  require(rows.eof() && count == 46);
}
