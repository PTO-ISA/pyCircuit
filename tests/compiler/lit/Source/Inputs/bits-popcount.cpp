// Independent per-bit oracle; no DUT expression or Runtime popcount helper.
#include "gfsim/SimExecutor.h"
#include "pycircuit_system.hpp"
#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <source_location>
#include <string>
#include <string_view>

namespace {
unsigned frame = 0;
void require(bool ok,
             std::source_location at = std::source_location::current()) {
  if (!ok) {
    std::cerr << "popcount oracle failed at line " << at.line() << ", frame "
              << frame << '\n';
    std::abort();
  }
}
enum class Pattern { Scalar, Zero, Ones, OneHot, Mixed, Masked, DenseMasked };

template <unsigned W, class Port>
void drivePort(Port &port, Pattern pattern, unsigned row, bool highZ,
               bool latent) {
  gfsim::Bits<W> value{0}, known{0}, z{0};
  for (unsigned bit = 0; bit < W; ++bit) {
    bool set = false;
    const unsigned denseState = (bit + row) % 4;
    switch (pattern) {
    case Pattern::Scalar:
      set = bit < 9 && ((row >> bit) & 1);
      break;
    case Pattern::Zero:
      break;
    case Pattern::Ones:
      set = true;
      break;
    case Pattern::OneHot:
      set = bit == row;
      break;
    case Pattern::Mixed:
      set = ((bit * 7 + row * 3) % 11) < 5;
      break;
    case Pattern::Masked:
      set = (bit * 7 % 11) < 5;
      break;
    case Pattern::DenseMasked:
      set = denseState == 1;
      break;
    }
    const bool unknown = (pattern == Pattern::Masked && bit == row) ||
                         (pattern == Pattern::DenseMasked && denseState >= 2);
    if (unknown)
      set = latent;
    const auto mask = std::uint64_t{1} << (bit % 64);
    const auto word = bit / 64;
    if (set)
      value.setWord(word, value.word(word) | mask);
    if (!unknown)
      known.setWord(word, known.word(word) | mask);
    const bool isZ = pattern == Pattern::DenseMasked ? denseState == 3 : highZ;
    if (unknown && isZ)
      z.setWord(word, z.word(word) | mask);
  }
  port = Port::fromPacked(gfsim::FourState<W>::fromMasks(value, known, z));
}

pyc_dut::Inputs inputs(Pattern pattern, unsigned row = 0, bool highZ = false,
                       bool latent = false) {
  pyc_dut::Inputs ports;
  drivePort<1>(ports.w1, pattern, row, highZ, latent);
  drivePort<2>(ports.w2, pattern, row, highZ, latent);
  drivePort<3>(ports.w3, pattern, row, highZ, latent);
  drivePort<4>(ports.w4, pattern, row, highZ, latent);
  drivePort<5>(ports.w5, pattern, row, highZ, latent);
  drivePort<7>(ports.w7, pattern, row, highZ, latent);
  drivePort<8>(ports.w8, pattern, row, highZ, latent);
  drivePort<9>(ports.w9, pattern, row, highZ, latent);
  drivePort<15>(ports.w15, pattern, row, highZ, latent);
  drivePort<16>(ports.w16, pattern, row, highZ, latent);
  drivePort<17>(ports.w17, pattern, row, highZ, latent);
  drivePort<31>(ports.w31, pattern, row, highZ, latent);
  drivePort<32>(ports.w32, pattern, row, highZ, latent);
  drivePort<33>(ports.w33, pattern, row, highZ, latent);
  drivePort<63>(ports.w63, pattern, row, highZ, latent);
  drivePort<64>(ports.w64, pattern, row, highZ, latent);
  drivePort<65>(ports.w65, pattern, row, highZ, latent);
  drivePort<73>(ports.w73, pattern, row, highZ, latent);
  drivePort<127>(ports.w127, pattern, row, highZ, latent);
  drivePort<128>(ports.w128, pattern, row, highZ, latent);
  drivePort<129>(ports.w129, pattern, row, highZ, latent);
  drivePort<3>(ports.state, pattern, row, highZ, latent);
  return ports;
}

struct ExpectedBit {
  bool value = false;
  bool known = true;
  bool z = false;
  bool checkValue = true;
};
using Expected = std::array<ExpectedBit, 139>;

void knownNumber(Expected &expected, unsigned low, unsigned width,
                 unsigned value) {
  for (unsigned bit = 0; bit < width; ++bit)
    expected[low + bit] = {bool((value >> bit) & 1), true, false, true};
}
void unknownNumber(Expected &expected, unsigned low, unsigned width) {
  for (unsigned bit = 0; bit < width; ++bit)
    expected[low + bit] = {false, false, false, false};
}
// Count only selected source bits. Source width one is exact plane transport.
template <unsigned SourceWidth, class Port>
void count(Expected &expected, unsigned low, unsigned naturalWidth,
           const Port &port, unsigned start = 0,
           unsigned length = SourceWidth) {
  const auto &raw = port.packed();
  if constexpr (SourceWidth == 1) {
    expected[low] = {raw.value().bit(0), raw.knownMask().bit(0),
                     raw.zMask().bit(0), true};
  } else {
    bool known = true;
    unsigned ones = 0;
    for (unsigned bit = start; bit < start + length; ++bit) {
      known = known && raw.knownMask().bit(bit) && !raw.zMask().bit(bit);
      if (raw.value().bit(bit))
        ++ones;
    }
    if (known)
      knownNumber(expected, low, naturalWidth, ones);
    else
      unknownNumber(expected, low, naturalWidth);
  }
}

Expected golden(const pyc_dut::Inputs &ports) {
  Expected expected{};
  count<1>(expected, 138, 1, ports.w1);
  count<2>(expected, 136, 2, ports.w2);
  count<3>(expected, 134, 2, ports.w3);
  count<4>(expected, 131, 3, ports.w4);
  count<5>(expected, 128, 3, ports.w5);
  count<7>(expected, 125, 3, ports.w7);
  count<8>(expected, 121, 4, ports.w8);
  count<9>(expected, 117, 4, ports.w9);
  count<15>(expected, 113, 4, ports.w15);
  count<16>(expected, 108, 5, ports.w16);
  count<17>(expected, 103, 5, ports.w17);
  count<31>(expected, 98, 5, ports.w31);
  count<32>(expected, 92, 6, ports.w32);
  count<33>(expected, 86, 6, ports.w33);
  count<63>(expected, 80, 6, ports.w63);
  count<64>(expected, 73, 7, ports.w64);
  count<65>(expected, 66, 7, ports.w65);
  count<73>(expected, 59, 7, ports.w73);
  count<127>(expected, 52, 7, ports.w127);
  count<128>(expected, 44, 8, ports.w128);
  count<129>(expected, 36, 8, ports.w129);
  // Unwritten destination high bits are known zero, including masked inputs.
  count<1>(expected, 28, 1, ports.w1);
  count<65>(expected, 15, 7, ports.w65);
  count<73>(expected, 11, 4, ports.w73, 61, 9);
  count<3>(expected, 6, 2, ports.state);
  count<5>(expected, 3, 3, ports.w5);

  const auto &five = ports.w5.packed();
  bool fiveKnown = true;
  unsigned fiveValue = 0;
  for (unsigned bit = 0; bit < 5; ++bit) {
    fiveKnown =
        fiveKnown && five.knownMask().bit(bit) && !five.zMask().bit(bit);
    if (five.value().bit(bit))
      fiveValue += 1u << bit;
  }
  if (fiveKnown) {
    const unsigned incremented = (fiveValue + 1) % 32;
    unsigned ones = 0;
    for (unsigned bit = 0; bit < 5; ++bit)
      if ((incremented >> bit) & 1)
        ++ones;
    knownNumber(expected, 8, 3, ones);
  } else
    unknownNumber(expected, 8, 3);

  const auto &wide = ports.w73.packed();
  bool wideKnown = true;
  unsigned wideCount = 0;
  for (unsigned bit = 0; bit < 73; ++bit) {
    wideKnown =
        wideKnown && wide.knownMask().bit(bit) && !wide.zMask().bit(bit);
    if (wide.value().bit(bit))
      ++wideCount;
  }
  if (wideKnown) {
    unsigned nested = 0;
    for (unsigned bit = 0; bit < 7; ++bit)
      if ((wideCount >> bit) & 1)
        ++nested;
    knownNumber(expected, 0, 3, nested);
  } else
    unknownNumber(expected, 0, 3);
  return expected;
}

template <class Output>
std::string check(const Output &output, const Expected &expected) {
  const auto &raw = output.packed();
  std::string symbols;
  for (unsigned bit = 0; bit < expected.size(); ++bit) {
    const auto &gold = expected[bit];
    require(raw.knownMask().bit(bit) == gold.known);
    require(raw.zMask().bit(bit) == gold.z);
    if (gold.checkValue)
      require(raw.value().bit(bit) == gold.value);
  }
  for (unsigned bit = expected.size(); bit; --bit)
    symbols += raw.zMask().bit(bit - 1)        ? 'z'
               : !raw.knownMask().bit(bit - 1) ? 'x'
               : raw.value().bit(bit - 1)      ? '1'
                                               : '0';
  return symbols;
}
} // namespace

int main(int argc, char **argv) {
  require(argc == 2);
  const unsigned workers = static_cast<unsigned>(std::stoul(argv[1]));
  require(workers == 1 || workers == 2);
  pyc_dut dut(workers);
  gfsim::SimExecutor executor(dut.system(), dut.observations(), {});
  constexpr std::string_view config =
      R"({"deadlock_window":null,"max_domain_cycles":{},"max_ticks":2048,"schema":"pycircuit-model-config","version":"1"})";
  require(executor.ConfigureJson(
              reinterpret_cast<const std::uint8_t *>(config.data()),
              config.size()) == PYCIRCUIT_MODEL_STATUS_V1_OK);
  dut.drive(inputs(Pattern::Zero));
  require(executor.Reset() == PYCIRCUIT_MODEL_STATUS_V1_OK);
  unsigned knownFrames = 0, maskedFrames = 0;
  auto run = [&](Pattern pattern, unsigned row = 0, bool highZ = false,
                 bool latent = false) {
    const auto ports = inputs(pattern, row, highZ, latent);
    const auto expected = golden(ports);
    dut.drive(ports);
    PycircuitModelStepResultV1 status{sizeof(status)};
    require(executor.Step(&status) == PYCIRCUIT_MODEL_STATUS_V1_OK);
    require(status.state == PYCIRCUIT_MODEL_STEP_V1_RUNNING);
    const auto symbols = check(dut.sample().result, expected);
    const bool masked =
        pattern == Pattern::Masked || pattern == Pattern::DenseMasked;
    std::cout << (masked ? "MASK " : "WORK ") << symbols << '\n';
    ++(masked ? maskedFrames : knownFrames);
    ++frame;
  };
  for (unsigned row = 0; row < 512; ++row)
    run(Pattern::Scalar, row);
  run(Pattern::Zero);
  run(Pattern::Ones);
  for (unsigned bit = 0; bit < 129; ++bit)
    run(Pattern::OneHot, bit);
  for (unsigned row = 0; row < 16; ++row)
    run(Pattern::Mixed, row);
  require(knownFrames == 659);
  for (unsigned bit = 0; bit < 129; ++bit)
    for (bool highZ : {false, true})
      for (bool latent : {false, true})
        run(Pattern::Masked, bit, highZ, latent);
  for (unsigned phase = 0; phase < 4; ++phase)
    for (bool latent : {false, true})
      run(Pattern::DenseMasked, phase, false, latent);
  run(Pattern::Ones);
  require(knownFrames == 660 && maskedFrames == 524 && frame == 1184);
}
