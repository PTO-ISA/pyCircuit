// Independent source-bit positions and three-plane transport, not host shifts.
#include "gfsim/SimExecutor.h"
#include "pycircuit_system.hpp"
#include <cstdlib>
#include <iostream>
#include <source_location>
#include <string>
#include <string_view>

void require(bool ok,
             std::source_location at = std::source_location::current()) {
  if (!ok) {
    std::cerr << "fixed unsigned shift oracle failed at " << at.line() << '\n';
    std::abort();
  }
}
char state(unsigned bit, unsigned width, unsigned row, bool mask) {
  if (mask) {
    if (row == 1)
      return 'x';
    if (row == 2)
      return 'z';
    if (row == 0) {
      if (bit == 0 || bit == width - 1 || bit == 63 || bit == 128)
        return 'x';
      if (bit == 1 || bit + 2 == width || bit == 64 || bit == 127)
        return 'z';
    } else {
      if (bit % 11 == 0)
        return 'z';
      if (bit % 11 == 1)
        return 'x';
    }
    return bit % 2 ? '1' : '0';
  }
  switch (row) {
  case 0:
    return '0';
  case 1:
    return '1';
  case 2:
    return bit == width - 1 ? '1' : '0';
  case 3:
    return bit == 0 ? '1' : '0';
  case 4:
    return bit == (width > 63 ? 63 : width / 2) ? '1' : '0';
  case 5:
    return bit == (width > 64 ? 64 : 0) ? '1' : '0';
  case 6:
    return bit % 2 ? '1' : '0';
  case 7:
    return bit % 2 ? '0' : '1';
  default:
    return (bit * 29 + row * 17) % 7 < 3 ? '1' : '0';
  }
}
template <unsigned W> auto pattern(unsigned row, bool mask = false) {
  gfsim::Bits<W> value{0}, known{0}, z{0};
  for (unsigned bit = 0; bit != W; ++bit) {
    const auto one = std::uint64_t{1} << (bit % 64);
    const auto word = bit / 64;
    const char symbol = state(bit, W, row, mask);
    // Noncanonical latent bits behind X/Z deliberately differ. Native checks
    // preserve them exactly; RTL compares only observable four-state symbols.
    if (symbol == '1' || ((symbol == 'x' || symbol == 'z') && bit % 3 == 0))
      value.setWord(word, value.word(word) | one);
    if (symbol == '0' || symbol == '1')
      known.setWord(word, known.word(word) | one);
    if (symbol == 'z')
      z.setWord(word, z.word(word) | one);
  }
  return gfsim::wire<gfsim::Bits<W>>::fromPacked(
      gfsim::FourState<W>::fromMasks(value, known, z));
}
template <unsigned Out, unsigned In, unsigned Expression = In, class Output>
void shifted(const Output &output, unsigned low,
             const gfsim::wire<gfsim::Bits<In>> &input, unsigned count,
             bool right) {
  const auto actual = gfsim::extract<Out>(output.packed(), low);
  for (unsigned bit = 0; bit != Out; ++bit) {
    const int source = right ? static_cast<int>(bit + count)
                             : static_cast<int>(bit) - static_cast<int>(count);
    const bool retained = bit < Expression && source >= 0 &&
                          source < static_cast<int>(Expression) &&
                          source < static_cast<int>(In);
    require(actual.value().bit(bit) ==
            (retained && input.packed().value().bit(source)));
    require(actual.knownMask().bit(bit) ==
            (!retained || input.packed().knownMask().bit(source)));
    require(actual.zMask().bit(bit) ==
            (retained && input.packed().zMask().bit(source)));
  }
}
template <unsigned W, class Output>
void matrix(const Output &output, unsigned low,
            const gfsim::wire<gfsim::Bits<W>> &input) {
  // The final slot denotes an arbitrary-precision overshift, not count modulo
  // W.
  const unsigned counts[] = {0, 1, W - 1, W, W + 1, W};
  for (unsigned direction = 0; direction != 2; ++direction)
    for (unsigned count = 0; count != 6; ++count)
      shifted<W>(output, low + (11 - direction * 6 - count) * W, input,
                 counts[count], direction == 1);
}
template <unsigned W, class Output>
void print(const Output &output, const char *prefix) {
  std::cout << prefix << ' ';
  const auto &p = output.packed();
  for (unsigned bit = W; bit != 0; --bit)
    std::cout << (p.zMask().bit(bit - 1)        ? 'z'
                  : !p.knownMask().bit(bit - 1) ? 'x'
                  : p.value().bit(bit - 1)      ? '1'
                                                : '0');
  std::cout << '\n';
}
int main(int argc, char **argv) {
  require(argc == 2);
  pyc_dut dut(static_cast<unsigned>(std::stoul(argv[1])));
  gfsim::SimExecutor executor(dut.system(), dut.observations(), {});
  constexpr std::string_view config =
      R"({"deadlock_window":null,"max_domain_cycles":{},"max_ticks":64,"schema":"pycircuit-model-config","version":"1"})";
  require(executor.ConfigureJson(
              reinterpret_cast<const std::uint8_t *>(config.data()),
              config.size()) == PYCIRCUIT_MODEL_STATUS_V1_OK);
  require(executor.Reset() == PYCIRCUIT_MODEL_STATUS_V1_OK);
  auto sample = [&](unsigned row, bool mask, bool emit = true) {
    pyc_dut::Inputs input;
    input.a = pattern<8>(row, mask);
#ifndef EXACT_INTEGER
    input.n1 = pattern<1>(row, mask);
    input.n5 = pattern<5>(row, mask);
    input.n13 = pattern<13>(row, mask);
    input.n65 = pattern<65>(row, mask);
    input.n130 = pattern<130>(row, mask);
#endif
    dut.drive(input);
    PycircuitModelStepResultV1 status{sizeof(status)};
    require(executor.Step(&status) == PYCIRCUIT_MODEL_STATUS_V1_OK);
    require(status.state == PYCIRCUIT_MODEL_STEP_V1_RUNNING);
#ifdef EXACT_INTEGER
    const auto output = dut.sample();
    require(output.identity.isFullyKnown());
    require(output.identity.value().value() == input.a.value().value());
    require(output.add_before.isFullyKnown() &&
            output.static_signed.isFullyKnown());
    // Host mathematical arithmetic: unsigned255 + 1 is256 before division.
    require(output.add_before.value().value() ==
            (input.a.value().value() + 1) / 256);
    require(output.static_signed.value().value() == 252);
    if (emit)
      std::cout << "WORK " << output.add_before.value().value() << " 252 "
                << output.identity.value().value() << '\n';
#else
    const auto output = dut.sample().result;
    matrix<1>(output, 2804, input.n1);
    matrix<5>(output, 2744, input.n5);
    matrix<13>(output, 2588, input.n13);
    matrix<65>(output, 1808, input.n65);
    matrix<130>(output, 248, input.n130);
    shifted<13>(output, 235, input.n13, 3, false);
    shifted<65>(output, 170, input.n65, 1, true);
    // Fixed u8 addition wraps before this width8 overshift. Even X input
    // arithmetic is discarded by the overshift's known-zero result.
    shifted<8>(output, 162, input.a, 8, true);
    shifted<16, 8>(output, 146, input.a, 1, false);
    shifted<16, 8, 16>(output, 130, input.a, 1, false);
    shifted<130>(output, 0, input.n130, 1, false);
    if (emit)
      print<2816>(output, mask ? "MASK" : "WORK");
    if (!mask)
      require(output.isFullyKnown());
#endif
  };
  for (unsigned row = 0; row != 16; ++row)
    sample(row, false);
#ifndef EXACT_INTEGER
  for (unsigned row = 0; row != 4; ++row) {
    sample(row, true);
    sample(1, false, false);
  }
#endif
}
