// Endpoint-scan oracle derived from the reviewed three-case X/Z contract.
// No DUT expression, prefix-fill algorithm, or Runtime count helper is used.
#include "gfsim/SimExecutor.h"
#include "pycircuit_system.hpp"
#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <source_location>
#include <string>
#include <string_view>
#include <vector>

namespace {
unsigned frame = 0;
void require(bool ok,
             std::source_location at = std::source_location::current()) {
  if (!ok) {
    std::cerr << "count-zero oracle failed at line " << at.line()
              << ", frame " << frame << '\n';
    std::abort();
  }
}
enum class Pattern {
  Scalar, Zero, Ones, OneHot, Prefix, Suffix, Mixed,
  ZeroMasked, OnesMasked, LeadingEndpoint, TrailingEndpoint, Dense, Double,
  LiteralControl
};
constexpr std::array<std::string_view, 7> literalInputs = {
    "1xxx", "01xz", "x111", "x011", "00x1", "xx11", "0000"};
constexpr std::array<std::string_view, 7> literalCounts = {
    "000", "001", "00x", "0xx", "xxx", "xxx", "100"};

char inputSymbol(unsigned width, unsigned bit, Pattern pattern, unsigned row,
                 bool highZ) {
  const char unknown = highZ ? 'z' : 'x';
  switch (pattern) {
  case Pattern::Scalar: return bit < 9 && ((row >> bit) & 1) ? '1' : '0';
  case Pattern::Zero: return '0';
  case Pattern::Ones: return '1';
  case Pattern::OneHot: return bit == row ? '1' : '0';
  case Pattern::Prefix: return row < width && bit < width - row ? '1' : '0';
  case Pattern::Suffix: return bit >= row ? '1' : '0';
  case Pattern::Mixed: return ((bit * 7 + row * 3) % 11) < 5 ? '1' : '0';
  case Pattern::ZeroMasked: return bit == row ? unknown : '0';
  case Pattern::OnesMasked: return bit == row ? unknown : '1';
  case Pattern::LeadingEndpoint:
    if (bit == width - 1) return unknown;
    return row < width && bit == width - 1 - row ? '1' : '0';
  case Pattern::TrailingEndpoint:
    if (bit == 0) return unknown;
    return row < width && bit == row ? '1' : '0';
  case Pattern::Dense: return "01xz"[(bit + row) % 4];
  case Pattern::Double: return bit == 0 || bit == width - 1 ? unknown : '0';
  case Pattern::LiteralControl:
    if (width != 4) return '0';
    require(row < 14);
    // The first seven are MSB-first leading controls; the next seven reverse
    // the same input bits to exercise exactly the corresponding trailing count.
    return literalInputs[row % 7][row < 7 ? 3 - bit : bit];
  }
  std::abort();
}

template <unsigned W, class Port>
void drivePort(Port &port, Pattern pattern, unsigned row, bool highZ,
               bool latent) {
  gfsim::Bits<W> value{0}, known{0}, z{0};
  for (unsigned bit = 0; bit < W; ++bit) {
    const char symbol = inputSymbol(W, bit, pattern, row, highZ);
    const bool isKnown = symbol == '0' || symbol == '1';
    const auto mask = std::uint64_t{1} << (bit % 64);
    const auto word = bit / 64;
    if (isKnown ? symbol == '1' : latent)
      value.setWord(word, value.word(word) | mask);
    if (isKnown)
      known.setWord(word, known.word(word) | mask);
    if (symbol == 'z')
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

// Sources and expected results are LSB-first symbol vectors. Input X and Z
// remain distinct; every computed unknown is X, with latent value unchecked.
using Symbols = std::vector<char>;
using Expected = std::array<char, 278>;
unsigned bitWidth(unsigned value) {
  unsigned width = 0;
  do { ++width; value >>= 1; } while (value);
  return width;
}
Symbols number(unsigned value, unsigned width) {
  Symbols result(width, '0');
  for (unsigned bit = 0; bit < width; ++bit)
    result[bit] = ((value >> bit) & 1) ? '1' : '0';
  return result;
}
template <unsigned W, class Port>
Symbols source(const Port &port, unsigned start = 0, unsigned length = W) {
  Symbols result(length);
  const auto &raw = port.packed();
  for (unsigned bit = 0; bit < length; ++bit)
    result[bit] = raw.zMask().bit(start + bit) ? 'z'
                  : !raw.knownMask().bit(start + bit) ? 'x'
                  : raw.value().bit(start + bit) ? '1' : '0';
  return result;
}
// Scan from the requested endpoint until the first known one. An unknown at
// position zero alone poisons bit_width(distance) low result bits; any other
// preceding unknown poisons the complete natural result. No Z survives.
Symbols countZeros(const Symbols &value, bool leading) {
  const unsigned width = value.size();
  const unsigned natural = bitWidth(width);
  unsigned distance = width, unknowns = 0;
  bool endpointUnknown = false;
  for (unsigned position = 0; position < width; ++position) {
    const char symbol = value[leading ? width - 1 - position : position];
    if (symbol == '1') { distance = position; break; }
    if (symbol == 'x' || symbol == 'z') {
      ++unknowns;
      if (position == 0) endpointUnknown = true;
    }
  }
  if (!unknowns) return number(distance, natural);
  Symbols result(natural, '0');
  const unsigned poison = endpointUnknown && unknowns == 1
                              ? bitWidth(distance) : natural;
  for (unsigned bit = 0; bit < poison; ++bit) result[bit] = 'x';
  return result;
}
void put(Expected &expected, unsigned low, unsigned width,
         const Symbols &value) {
  require(value.size() <= width);
  for (unsigned bit = 0; bit < width; ++bit)
    expected[low + bit] = bit < value.size() ? value[bit] : '0';
}
Symbols incrementFive(const Symbols &value) {
  unsigned knownValue = 0;
  for (unsigned bit = 0; bit < 5; ++bit) {
    if (value[bit] != '0' && value[bit] != '1') return Symbols(5, 'x');
    if (value[bit] == '1') knownValue += 1u << bit;
  }
  return number((knownValue + 1) % 32, 5);
}
Expected golden(const pyc_dut::Inputs &ports) {
  Expected expected{};
  expected.fill('0');
  put(expected, 277, 1, countZeros(source<1>(ports.w1), true)); // leading1
  put(expected, 276, 1, countZeros(source<1>(ports.w1), false)); // trailing1
  put(expected, 274, 2, countZeros(source<2>(ports.w2), true)); // leading2
  put(expected, 272, 2, countZeros(source<2>(ports.w2), false)); // trailing2
  put(expected, 270, 2, countZeros(source<3>(ports.w3), true)); // leading3
  put(expected, 268, 2, countZeros(source<3>(ports.w3), false)); // trailing3
  put(expected, 265, 3, countZeros(source<4>(ports.w4), true)); // leading4
  put(expected, 262, 3, countZeros(source<4>(ports.w4), false)); // trailing4
  put(expected, 259, 3, countZeros(source<5>(ports.w5), true)); // leading5
  put(expected, 256, 3, countZeros(source<5>(ports.w5), false)); // trailing5
  put(expected, 253, 3, countZeros(source<7>(ports.w7), true)); // leading7
  put(expected, 250, 3, countZeros(source<7>(ports.w7), false)); // trailing7
  put(expected, 246, 4, countZeros(source<8>(ports.w8), true)); // leading8
  put(expected, 242, 4, countZeros(source<8>(ports.w8), false)); // trailing8
  put(expected, 238, 4, countZeros(source<9>(ports.w9), true)); // leading9
  put(expected, 234, 4, countZeros(source<9>(ports.w9), false)); // trailing9
  put(expected, 230, 4, countZeros(source<15>(ports.w15), true)); // leading15
  put(expected, 226, 4, countZeros(source<15>(ports.w15), false)); // trailing15
  put(expected, 221, 5, countZeros(source<16>(ports.w16), true)); // leading16
  put(expected, 216, 5, countZeros(source<16>(ports.w16), false)); // trailing16
  put(expected, 211, 5, countZeros(source<17>(ports.w17), true)); // leading17
  put(expected, 206, 5, countZeros(source<17>(ports.w17), false)); // trailing17
  put(expected, 201, 5, countZeros(source<31>(ports.w31), true)); // leading31
  put(expected, 196, 5, countZeros(source<31>(ports.w31), false)); // trailing31
  put(expected, 190, 6, countZeros(source<32>(ports.w32), true)); // leading32
  put(expected, 184, 6, countZeros(source<32>(ports.w32), false)); // trailing32
  put(expected, 178, 6, countZeros(source<33>(ports.w33), true)); // leading33
  put(expected, 172, 6, countZeros(source<33>(ports.w33), false)); // trailing33
  put(expected, 166, 6, countZeros(source<63>(ports.w63), true)); // leading63
  put(expected, 160, 6, countZeros(source<63>(ports.w63), false)); // trailing63
  put(expected, 153, 7, countZeros(source<64>(ports.w64), true)); // leading64
  put(expected, 146, 7, countZeros(source<64>(ports.w64), false)); // trailing64
  put(expected, 139, 7, countZeros(source<65>(ports.w65), true)); // leading65
  put(expected, 132, 7, countZeros(source<65>(ports.w65), false)); // trailing65
  put(expected, 125, 7, countZeros(source<73>(ports.w73), true)); // leading73
  put(expected, 118, 7, countZeros(source<73>(ports.w73), false)); // trailing73
  put(expected, 111, 7, countZeros(source<127>(ports.w127), true)); // leading127
  put(expected, 104, 7, countZeros(source<127>(ports.w127), false)); // trailing127
  put(expected, 96, 8, countZeros(source<128>(ports.w128), true)); // leading128
  put(expected, 88, 8, countZeros(source<128>(ports.w128), false)); // trailing128
  put(expected, 80, 8, countZeros(source<129>(ports.w129), true)); // leading129
  put(expected, 72, 8, countZeros(source<129>(ports.w129), false)); // trailing129
  put(expected, 64, 8, countZeros(source<1>(ports.w1), true)); // leadingWiden1
  put(expected, 56, 8, countZeros(source<1>(ports.w1), false)); // trailingWiden1
  put(expected, 43, 13, countZeros(source<65>(ports.w65), true)); // leadingWiden65
  put(expected, 30, 13, countZeros(source<65>(ports.w65), false)); // trailingWiden65
  put(expected, 26, 4, countZeros(source<73>(ports.w73, 61, 9), true)); // leadingSlice73
  put(expected, 22, 4, countZeros(source<73>(ports.w73, 61, 9), false)); // trailingSlice73
  put(expected, 19, 3, countZeros(incrementFive(source<5>(ports.w5)), true)); // leadingArithmetic5
  put(expected, 16, 3, countZeros(incrementFive(source<5>(ports.w5)), false)); // trailingArithmetic5
  put(expected, 14, 2, countZeros(source<3>(ports.state), true)); // leadingEnum3
  put(expected, 12, 2, countZeros(source<3>(ports.state), false)); // trailingEnum3
  put(expected, 9, 3, countZeros(source<5>(ports.w5), true)); // leadingChild5
  put(expected, 6, 3, countZeros(source<5>(ports.w5), false)); // trailingChild5
  put(expected, 3, 3, countZeros(countZeros(source<73>(ports.w73), false), true)); // leadingNested73
  put(expected, 0, 3, countZeros(countZeros(source<73>(ports.w73), true), false)); // trailingNested73
  return expected;
}

template <class Output>
std::string check(const Output &output, const Expected &expected) {
  const auto &raw = output.packed();
  for (unsigned bit = 0; bit < expected.size(); ++bit) {
    const char symbol = expected[bit];
    const bool known = symbol == '0' || symbol == '1';
    const bool ok = raw.knownMask().bit(bit) == known &&
                    !raw.zMask().bit(bit) &&
                    (!known || raw.value().bit(bit) == (symbol == '1'));
    if (!ok)
      std::cerr << "mismatch output bit " << bit << " expected " << symbol
                << ", value/known/z " << raw.value().bit(bit) << '/'
                << raw.knownMask().bit(bit) << '/' << raw.zMask().bit(bit)
                << '\n';
    require(ok);
  }
  std::string symbols;
  for (unsigned bit = expected.size(); bit; --bit)
    symbols += raw.zMask().bit(bit - 1) ? 'z'
               : !raw.knownMask().bit(bit - 1) ? 'x'
               : raw.value().bit(bit - 1) ? '1' : '0';
  return symbols;
}
template <class Output>
void checkLiteralControl(const Output &output, unsigned row) {
  const auto &raw = output.packed();
  const unsigned low = row < 7 ? 265 : 262;
  const auto literal = literalCounts[row % 7];
  for (unsigned bit = 0; bit < 3; ++bit) {
    const char symbol = literal[2 - bit];
    const bool known = symbol == '0' || symbol == '1';
    require(raw.knownMask().bit(low + bit) == known &&
            !raw.zMask().bit(low + bit) &&
            (!known || raw.value().bit(low + bit) == (symbol == '1')));
  }
}
} // namespace

int main(int argc, char **argv) {
  require(argc == 2);
  const unsigned workers = static_cast<unsigned>(std::stoul(argv[1]));
  require(workers == 1 || workers == 2);
  pyc_dut dut(workers);
  gfsim::SimExecutor executor(dut.system(), dut.observations(), {});
  constexpr std::string_view config =
      R"({"deadlock_window":null,"max_domain_cycles":{},"max_ticks":4096,"schema":"pycircuit-model-config","version":"1"})";
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
    if (pattern == Pattern::LiteralControl)
      checkLiteralControl(dut.sample().result, row);
    const bool masked = pattern >= Pattern::ZeroMasked;
    std::cout << (masked ? "MASK " : "WORK ") << symbols << '\n';
    ++(masked ? maskedFrames : knownFrames);
    ++frame;
  };
  // Every scalar at widths <=9, then all zeros/ones, every one-hot position,
  // both long zero-prefix directions, and varied known mixtures.
  for (unsigned row = 0; row < 512; ++row) run(Pattern::Scalar, row);
  run(Pattern::Zero); run(Pattern::Ones);
  for (unsigned row = 0; row < 129; ++row) run(Pattern::OneHot, row);
  for (unsigned row = 0; row < 129; ++row) {
    run(Pattern::Prefix, row); run(Pattern::Suffix, row);
  }
  for (unsigned row = 0; row < 16; ++row) run(Pattern::Mixed, row);
  require(knownFrames == 917);
  // Full-X prefixes, endpoint partial X, ignored suffix X/Z, true Z, and
  // both native latent payloads. Each latent alternative is replayed in RTL.
  for (auto pattern : {Pattern::ZeroMasked, Pattern::OnesMasked})
    for (unsigned row = 0; row < 129; ++row)
      for (bool highZ : {false, true})
        for (bool latent : {false, true}) run(pattern, row, highZ, latent);
  for (auto pattern : {Pattern::LeadingEndpoint, Pattern::TrailingEndpoint})
    for (unsigned row = 1; row <= 129; ++row)
      for (bool highZ : {false, true})
        for (bool latent : {false, true}) run(pattern, row, highZ, latent);
  for (unsigned phase = 0; phase < 4; ++phase)
    for (bool latent : {false, true}) run(Pattern::Dense, phase, false, latent);
  for (bool highZ : {false, true})
    for (bool latent : {false, true}) run(Pattern::Double, 0, highZ, latent);
  require(maskedFrames == 2076);
  // Exact reviewed four-bit truth-table rows, then reversed trailing counterparts.
  // All other input ports are known zero. Each native latent alternative is
  // replayed as the same visible X/Z row in the genuine RTL testbench.
  for (unsigned row = 0; row < 14; ++row)
    for (bool latent : {false, true})
      run(Pattern::LiteralControl, row, false, latent);
  require(maskedFrames == 2104);
  run(Pattern::Ones); // Known recovery after masked frames.
  require(knownFrames == 918 && frame == 3022);
}
