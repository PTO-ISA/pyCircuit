// Goldens are host Python divmod values supplied independently of compiler IR.
#include "gfsim/SimExecutor.h"
#include "pycircuit_system.hpp"
#include <cstdlib>
#include <iostream>
#include <iterator>
#include <string>
#include <string_view>
void require(bool ok) { if (!ok) { std::cerr << "static divrem oracle failed\n"; std::abort(); } }
template <unsigned W> auto input(std::string_view text) {
  gfsim::Bits<W> value{0}, known{0}, z{0};
  for (unsigned bit = 0; bit < W; ++bit) {
    char symbol = text[W - bit - 1];
    auto one = std::uint64_t{1} << (bit % 64);
    auto word = bit / 64;
    // Include hidden value bits behind X/Z; arithmetic must make all bits X.
    if (symbol == '1' || ((symbol == 'x' || symbol == 'z') && bit % 3 == 0))
      value.setWord(word, value.word(word) | one);
    if (symbol == '0' || symbol == '1') known.setWord(word, known.word(word) | one);
    if (symbol == 'z') z.setWord(word, z.word(word) | one);
  }
  return gfsim::wire<gfsim::Bits<W>>::fromPacked(gfsim::FourState<W>::fromMasks(value, known, z));
}
template <unsigned W, class Output> void check(const Output &output, std::string_view expected) {
  std::string actual;
  const auto &p = output.packed();
  for (unsigned bit = W; bit; --bit)
    actual += p.zMask().bit(bit - 1) ? 'z' : !p.knownMask().bit(bit - 1) ? 'x' : p.value().bit(bit - 1) ? '1' : '0';
  require(actual == expected);
}
#include "static-divrem-vectors.hpp"
int main(int argc, char **argv) {
  require(argc == 2);
  pyc_dut dut(static_cast<unsigned>(std::stoul(argv[1])));
  gfsim::SimExecutor executor(dut.system(), dut.observations(), {});
  const std::string config =
      R"({"deadlock_window":null,"max_domain_cycles":{},"max_ticks":)" +
      std::to_string(std::size(expected) + 1) +
      R"(,"schema":"pycircuit-model-config","version":"1"})";
  require(executor.ConfigureJson(reinterpret_cast<const std::uint8_t *>(config.data()), config.size()) == PYCIRCUIT_MODEL_STATUS_V1_OK);
  require(executor.Reset() == PYCIRCUIT_MODEL_STATUS_V1_OK);
  for (unsigned row = 0; row < std::size(expected); ++row) {
    pyc_dut::Inputs ports;
    drive(ports, row);
    dut.drive(ports);
    PycircuitModelStepResultV1 status{sizeof(status)};
    require(executor.Step(&status) == PYCIRCUIT_MODEL_STATUS_V1_OK);
    require(status.state == PYCIRCUIT_MODEL_STEP_V1_RUNNING);
    check<result_width>(dut.sample().result, expected[row]);
    std::cout << (row >= known_sample_count && (row - known_sample_count) % 2 == 0 ? "MASK " : "WORK ") << row << '\n';
  }
}
