#include "pycircuit_system.hpp"
#include <cstdlib>

template <unsigned Width>
gfsim::wire<gfsim::Bits<Width>> known(unsigned value) {
  return gfsim::wire<gfsim::Bits<Width>>::known(gfsim::Bits<Width>{value});
}

template <unsigned Width>
void expect(const gfsim::wire<gfsim::Bits<Width>> &actual, unsigned value,
            unsigned knownMask, unsigned zMask = 0) {
  auto packed = actual.packed();
  if (packed.knownMask() != gfsim::Bits<Width>{knownMask} ||
      packed.zMask() != gfsim::Bits<Width>{zMask} ||
      (packed.value() & packed.knownMask()) !=
          gfsim::Bits<Width>{value & knownMask})
    std::abort();
}

int main() {
  pyc_dut dut;
  dut.system().Build();
  dut.system().Reset();

  const auto sample = [&](gfsim::wire<gfsim::Bits<8>> data, bool permit) {
    pyc_dut::Inputs inputs;
    inputs.stimulus = data;
    inputs.permit = known<1>(permit);
    dut.drive(inputs);
    if (dut.system().Step() != gfsim::SimStepResult::Running)
      std::abort();
    return dut.sample();
  };

  const auto blocked = sample(known<8>(0x93), false);
  expect(blocked.forwarded, 0x39, 0xff);
  expect(blocked.mirror, 0x6c, 0xff);
  expect(blocked.did_send, 0, 1);
  expect(blocked.accepted, 0, 0xff);

  const auto open = sample(known<8>(0x2e), true);
  expect(open.forwarded, 0x84, 0xff);
  expect(open.mirror, 0xd1, 0xff);
  expect(open.did_send, 1, 1);
  expect(open.accepted, 0x55, 0xff);

  using Byte = gfsim::Bits<8>;
  const auto mixed = gfsim::wire<Byte>::fromPacked(
      gfsim::FourState<8>::fromMasks(Byte{0x20}, Byte{0xf0}, Byte{0x08}));
  const auto passed = sample(mixed, true);
  // XOR consumes both unknown and high-impedance input bits as unknowns.
  expect(passed.forwarded, 0x80, 0xf0);
  expect(passed.mirror, 0xd0, 0xf0);
  expect(passed.did_send, 1, 1);
  expect(passed.accepted, 0x50, 0xf0);

  const auto closed = sample(mixed, false);
  expect(closed.forwarded, 0x80, 0xf0);
  expect(closed.mirror, 0xd0, 0xf0);
  expect(closed.did_send, 0, 1);
  expect(closed.accepted, 0, 0xff);

  const auto unknownReady = sample(gfsim::wire<Byte>::fromPacked(
      gfsim::FourState<8>::fromMasks(Byte{0x2e}, Byte{0x7f}, Byte{0x80})), true);
  expect(unknownReady.forwarded, 0x04, 0x7f);
  expect(unknownReady.mirror, 0x51, 0x7f);
  expect(unknownReady.did_send, 0, 0);
  expect(unknownReady.accepted, 0, 0x2a);
}
