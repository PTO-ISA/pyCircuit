#include "pycircuit_system.hpp"
#include "availability_ports.hpp"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>

void require(bool condition) {
  if (!condition)
    std::abort();
}

template <unsigned Width> auto bits(const std::string &text) {
  using Value = gfsim::FourState<Width>;
  using Bits = gfsim::Bits<Width>;
  return gfsim::wire<Bits>::fromPacked(
      text == "x" ? Value::unknown()
      : text == "z" ? Value::highImpedance()
                    : Value::known(Bits{std::stoul(text, nullptr, 16)}));
}

template <class Value> void expect(const Value &value, unsigned expected) {
  require(value.isFullyKnown() && value.value().value() == expected);
}

template <unsigned Width>
void same(const gfsim::FourState<Width> &actual,
          const gfsim::FourState<Width> &expected) {
  require(actual.knownMask() == expected.knownMask());
  require(actual.zMask() == expected.zMask());
  require((actual.value() & expected.knownMask()) ==
          (expected.value() & expected.knownMask()));
}

int main(int argc, char **argv) {
  require(argc == 2);
  std::ifstream rows(argv[1]);
  require(rows.good());
  pyc_root root("availability");
  root.pyc_7079635f636c6b = bits<1>("0");
  root.pyc_7079635f727374 = bits<1>("0");
  root.Reset();
  root.Xfer();

  bool previousClock = false;
  unsigned ticks = 0, accepted = 0, samples = 0;
  std::string clock, reset, allow, data;
  while (rows >> clock >> reset >> allow >> data) {
    const bool high = clock == "1";
    const bool rising = high && !previousClock;
    const bool resetting = reset == "1";
    const bool unknown = allow == "x" || allow == "z";
    const bool checks = !rising || resetting || !unknown;
    const bool available = allow == "1";
    root.pyc_7079635f636c6b = bits<1>(clock);
    root.pyc_7079635f727374 = bits<1>(reset);
    root.allow = bits<1>(allow);
    root.data = bits<8>(data);
    root.Work();
    expect(root.PYC_PACKET_VALID, available);
    expect(root.PYC_PROJECTED_VALID, available);
    expect(root.independent, ticks);
    expect(root.accepted, accepted);
    same(gfsim::extract<8>(root.packet.packed(), 0), gfsim::FourState<8>::known(gfsim::Bits<8>{ticks}));
    if (available) {
      same(gfsim::extract<8>(root.packet.packed(), 8), bits<8>(data).packed());
      same(root.projected.packed(), bits<8>(data).packed());
    }
    gfsim::SimFailureInfo failure;
    require(root.pyc_validate_checks(failure) == checks);
    if (!checks) {
      require(failure.sourceJson.find("\"line\":41") != std::string::npos);
      root.DiscardNext();
    }
    root.Xfer();
    if (checks) {
      if (rising) {
        if (resetting) {
          ticks = accepted = 0;
        } else {
          ticks = (ticks + 1) & 255u;
          if (available)
            accepted = (accepted + 1) & 255u;
        }
      }
      previousClock = high;
    }
    std::cout << "ROW " << samples++ << ' ' << available << ' ' << checks << ' '
              << ticks << ' ' << accepted << '\n';
  }
  require(rows.eof() && samples == 20);
}
