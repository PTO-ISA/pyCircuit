#include "pycircuit_system.hpp"
#include "child_availability_ports.hpp"
#include <cstdlib>
#include <fstream>
#include <string>

void require(bool condition) {
  if (!condition)
    std::abort();
}
template <unsigned W> auto bits(const std::string &text) {
  using B = gfsim::Bits<W>;
  using F = gfsim::FourState<W>;
  return gfsim::wire<B>::fromPacked(
      text == "x" ? F::unknown()
      : text == "z" ? F::highImpedance()
                    : F::known(B{std::stoul(text, nullptr, 16)}));
}
template <class T> void expect(const T &value, unsigned expected) {
  require(value.isFullyKnown() && value.value().value() == expected);
}
template <unsigned W>
void same(const gfsim::FourState<W> &actual,
          const gfsim::FourState<W> &expected) {
  require(actual.knownMask() == expected.knownMask());
  require(actual.zMask() == expected.zMask());
  require((actual.value() & expected.knownMask()) ==
          (expected.value() & expected.knownMask()));
}
int main(int argc, char **argv) {
  require(argc == 2);
  std::ifstream rows(argv[1]);
  require(rows.good());
  pyc_root root("child_availability");
  root.pyc_7079635f636c6b = bits<1>("0");
  root.pyc_7079635f727374 = bits<1>("0");
  root.Reset();
  root.Xfer();
  std::string clock, reset, leftAllow, leftPayload, rightAllow, rightPayload,
      leftEnable, rightEnable, conjoined;
  unsigned leftValid, rightValid, leftQ, rightQ, leftSeen, rightSeen, accepted,
      samples = 0;
  while (rows >> clock >> reset >> leftAllow >> leftPayload >> rightAllow >>
         rightPayload >> leftEnable >> rightEnable >> std::hex >> leftValid >>
         rightValid >> conjoined >> leftQ >> rightQ >> leftSeen >> rightSeen >>
         accepted) {
    root.pyc_7079635f636c6b = bits<1>(clock);
    root.pyc_7079635f727374 = bits<1>(reset);
    root.left_allow = bits<1>(leftAllow);
    root.left_payload = bits<1>(leftPayload);
    root.right_allow = bits<1>(rightAllow);
    root.right_payload = bits<1>(rightPayload);
    root.left_enable = bits<1>(leftEnable);
    root.right_enable = bits<1>(rightEnable);
    root.Work();
    expect(root.PYC_PACKET_VALID, leftValid);
    expect(root.PYC_RIGHT_VALID, rightValid);
    expect(root.left_committed, leftQ);
    expect(root.right_committed, rightQ);
    expect(root.left_seen, leftSeen);
    expect(root.right_seen, rightSeen);
    same(gfsim::extract<8>(root.packet.packed(), 0),
         gfsim::FourState<8>::known(gfsim::Bits<8>{leftQ}));
    if (leftValid)
      same(gfsim::extract<1>(root.packet.packed(), 8), bits<1>(conjoined).packed());
    if (rightValid)
      same(root.right_gate.packed(), bits<1>(rightPayload).packed());
    gfsim::SimFailureInfo failure;
    require(root.pyc_validate_checks(failure) == bool(accepted));
    if (!accepted) {
      require(failure.phase == gfsim::SimFailurePhase::Check);
      root.DiscardNext();
    }
    root.Xfer();
    ++samples;
  }
  require(rows.eof() && samples == 30);
}
