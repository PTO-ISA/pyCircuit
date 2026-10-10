#include "gfsim/SystemRunner.h"
#include "pycircuit_system.hpp"
#include <cstdlib>
#include <iostream>
#include <source_location>
#include <string_view>

void require(bool condition,
             std::source_location at = std::source_location::current()) {
  if (!condition) {
    std::cerr << "bounded-for oracle failed at " << at.line() << '\n';
    std::abort();
  }
}

template <unsigned Width> auto bits(std::string_view symbols) {
  require(symbols.size() == Width);
  gfsim::Bits<Width> value{0}, known{0}, z{0};
  for (unsigned bit = 0; bit < Width; ++bit) {
    const char symbol = symbols[Width - bit - 1];
    const auto mask = std::uint64_t{1} << (bit % 64);
    const auto word = bit / 64;
    if (symbol == '1' || ((symbol == 'x' || symbol == 'z') && bit % 3 == 0))
      value.setWord(word, value.word(word) | mask);
    if (symbol == '0' || symbol == '1')
      known.setWord(word, known.word(word) | mask);
    if (symbol == 'z')
      z.setWord(word, z.word(word) | mask);
  }
  return gfsim::wire<gfsim::Bits<Width>>::fromPacked(
      gfsim::FourState<Width>::fromMasks(value, known, z));
}

template <unsigned Width> auto known(std::uint64_t value) {
  return gfsim::wire<gfsim::Bits<Width>>::known(gfsim::Bits<Width>{value});
}

#include "bounded-for-vectors.hpp"

const char *field(unsigned bit) {
  if (bit < 65)
    return "carrier";
  if (bit < 73)
    return "bookkeeping";
  if (bit < 81)
    return "signed_shifted";
  if (bit == 81)
    return "wide_flag";
  return "known_scalar_payload";
}

void mismatch(unsigned row, unsigned bit, const char *plane, bool actual,
              bool expected) {
  std::cerr << "bounded-for row " << row << " bit " << bit << " field "
            << field(bit) << ' ' << plane << " actual=" << actual
            << " expected=" << expected << '\n';
  std::abort();
}

struct Context {
  pyc_dut &dut;
  unsigned sampled = 0;

  static void initialize(void *opaque) { require(drive(opaque, 0)); }
  static bool drive(void *opaque, std::uint64_t epoch) {
    auto &self = *static_cast<Context *>(opaque);
    if (epoch == row_count)
      return false;
    require(epoch < row_count);
    pyc_dut::Inputs inputs;
    drive_inputs(inputs, epoch);
    self.dut.drive(inputs);
    return true;
  }
  static void sample(void *opaque, std::uint64_t epoch) {
    auto &self = *static_cast<Context *>(opaque);
    require(epoch == self.sampled + 1 && self.sampled < row_count);
    const auto packed = self.dut.sample().result.packed();
    const auto expected = bits<result_width>(expected_symbols[self.sampled]);
    for (unsigned bit = 0; bit < result_width; ++bit) {
      const bool actualKnown = packed.knownMask().bit(bit);
      const bool expectedKnown = expected.packed().knownMask().bit(bit);
      if (actualKnown != expectedKnown)
        mismatch(self.sampled, bit, "known", actualKnown, expectedKnown);
      const bool actualZ = packed.zMask().bit(bit);
      const bool expectedZ = expected.packed().zMask().bit(bit);
      if (actualZ != expectedZ)
        mismatch(self.sampled, bit, "z", actualZ, expectedZ);
      const bool actualValue = packed.value().bit(bit);
      const bool expectedValue = expected.packed().value().bit(bit);
      // A computed unknown comparison has canonical latent value zero. Carrier
      // bits are copied and remain exact on all three planes.
      if (bit == 81 && !expectedKnown) {
        if (actualValue)
          mismatch(self.sampled, bit, "canonical-value", actualValue, false);
      } else if (actualValue != expectedValue) {
        mismatch(self.sampled, bit, "value", actualValue, expectedValue);
      }
    }
    std::cout << "WORK " << self.sampled << ' ' << expected_symbols[self.sampled]
              << '\n';
    ++self.sampled;
  }
};

int main(int argc, char **argv) {
  gfsim::SystemRunner runner(argc, argv);
  if (!runner.ready())
    return 2;
  pyc_dut dut(runner.workers());
  Context context{dut};
  const gfsim::RunnerCallbacks callbacks{&context, &Context::initialize,
                                         &Context::drive, &Context::sample};
  const int status =
      runner.Run(dut.system(), dut.observations(), {}, callbacks);
  require(status == 0 && context.sampled == row_count);
  return status;
}
