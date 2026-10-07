#include "gfsim/SystemRunner.h"
#include "pycircuit_system.hpp"
#include <cstdlib>
#include <iostream>
#include <source_location>
#include <string_view>

void require(bool ok, std::source_location at = std::source_location::current()) {
  if (!ok) {
    std::cerr << "record transport oracle failed at " << at.line() << '\n';
    std::abort();
  }
}
template <unsigned W> auto input(std::string_view text) {
  require(text.size() == W);
  gfsim::Bits<W> value{0}, known{0}, z{0};
  for (unsigned bit = 0; bit < W; ++bit) {
    const char symbol = text[W - bit - 1];
    const auto one = std::uint64_t{1} << (bit % 64);
    const auto word = bit / 64;
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
#include "record-projection-vectors.hpp"
struct Context {
  pyc_dut &dut;
  unsigned sampled = 0;
  static void initialize(void *opaque) { require(driveInputs(opaque, 0)); }
  static bool driveInputs(void *opaque, std::uint64_t epoch) {
    auto &self = *static_cast<Context *>(opaque);
    if (epoch == row_count)
      return false;
    require(epoch < row_count);
    pyc_dut::Inputs ports;
    drive(ports, epoch);
    self.dut.drive(ports);
    return true;
  }
  static void sample(void *opaque, std::uint64_t epoch) {
    auto &self = *static_cast<Context *>(opaque);
    require(epoch == self.sampled + 1 && self.sampled < row_count);
    const auto output = self.dut.sample();
    const auto &packed = output.result.packed();
    const unsigned row = self.sampled;
    require(packed.value() == input<result_width>(expected_value[row]).packed().value());
    require(packed.knownMask() == input<result_width>(expected_known[row]).packed().value());
    require(packed.zMask() == input<result_width>(expected_z[row]).packed().value());
    std::cout << "WORK " << row << ' ' << expected[row] << '\n';
    ++self.sampled;
  }
};
int main(int argc, char **argv) {
  gfsim::SystemRunner runner(argc, argv);
  if (!runner.ready())
    return 2;
  pyc_dut dut(runner.workers());
  Context context{dut};
  const gfsim::RunnerCallbacks callbacks{
      &context, &Context::initialize, &Context::driveInputs, &Context::sample};
  const int status = runner.Run(dut.system(), dut.observations(), {}, callbacks);
  require(status == 0 && context.sampled == row_count);
  return status;
}
