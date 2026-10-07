// Independent per-bit placement checks; no DUT concatenation helper is an
// oracle.
#include "gfsim/SimExecutor.h"
#include "pycircuit_system.hpp"
#include <cstdlib>
#include <iostream>
#include <source_location>
#include <string>
void require(bool ok,
             std::source_location at = std::source_location::current()) {
  if (!ok) {
    std::cerr << "concat oracle failed at " << at.line() << '\n';
    std::abort();
  }
}
template <unsigned W> auto input(unsigned row, bool masked) {
  gfsim::Bits<W> value{0}, known{0}, z{0};
  for (unsigned bit = 0; bit != W; ++bit) {
    const unsigned state =
        masked ? (bit + row / 2) % 4 : (bit + row * 3) % 5 < 2;
    const auto mask = std::uint64_t{1} << (bit % 64);
    const auto word = bit / 64;
    if (state == 1 || (state >= 2 && (bit + row % 2) % 2))
      value.setWord(word, value.word(word) | mask);
    if (state < 2)
      known.setWord(word, known.word(word) | mask);
    if (state == 3)
      z.setWord(word, z.word(word) | mask);
  }
  return gfsim::wire<gfsim::Bits<W>>::fromPacked(
      gfsim::FourState<W>::fromMasks(value, known, z));
}
template <typename Output, typename Input>
void segment(const Output &output, unsigned low, const Input &input,
             unsigned inputLow, unsigned width) {
  const auto &actual = output.packed();
  const auto &expected = input.packed();
  for (unsigned bit = 0; bit != width; ++bit) {
    require(actual.value().bit(low + bit) ==
            expected.value().bit(inputLow + bit));
    require(actual.knownMask().bit(low + bit) ==
            expected.knownMask().bit(inputLow + bit));
    require(actual.zMask().bit(low + bit) ==
            expected.zMask().bit(inputLow + bit));
  }
}
template <unsigned W, typename Packet>
std::string symbols(const Packet &packet) {
  const auto &packed = packet.packed();
  std::string result;
  for (unsigned index = 0; index != W; ++index) {
    const unsigned bit = W - index - 1;
    result += packed.zMask().bit(bit)        ? 'z'
              : !packed.knownMask().bit(bit) ? 'x'
              : packed.value().bit(bit)      ? '1'
                                             : '0';
  }
  return result;
}
int main(int argc, char **argv) {
  require(argc == 2);
  pyc_dut dut(static_cast<unsigned>(std::stoul(argv[1])));
  gfsim::SimExecutor executor(dut.system(), dut.observations(), {});
  const std::string config =
      R"({"deadlock_window":null,"max_domain_cycles":{},"max_ticks":32,"schema":"pycircuit-model-config","version":"1"})";
  require(executor.ConfigureJson(
              reinterpret_cast<const std::uint8_t *>(config.data()),
              config.size()) == PYCIRCUIT_MODEL_STATUS_V1_OK);
  require(executor.Reset() == PYCIRCUIT_MODEL_STATUS_V1_OK);
  for (bool masked : {false, true})
    for (unsigned row = 0; row != 8; ++row) {
      pyc_dut::Inputs inputs;
      inputs.tiny = input<1>(row, masked);
      inputs.n5 = input<5>(row, masked);
      inputs.middle = input<7>(row, masked);
      inputs.wide = input<73>(row, masked);
      inputs.state =
          decltype(inputs.state)::fromPacked(input<3>(row, masked).packed());
      dut.drive(inputs);
      PycircuitModelStepResultV1 status{sizeof(status)};
      require(executor.Step(&status) == PYCIRCUIT_MODEL_STATUS_V1_OK);
      require(status.state == PYCIRCUIT_MODEL_STEP_V1_RUNNING);
      const auto output = dut.sample().result;
      segment(output, 175, inputs.tiny, 0, 1); // singleton
      segment(output, 170, inputs.n5, 0, 5);
      segment(output, 169, inputs.tiny, 0, 1);
      segment(output, 168, inputs.tiny, 0, 1);
      segment(output, 163, inputs.n5, 0, 5);
      segment(output, 152, inputs.wide, 0, 11);
      segment(output, 145, inputs.middle, 0, 7);
      segment(output, 136, inputs.wide, 61, 9); // five unequal operands
      segment(output, 63, inputs.wide, 0, 73);
      segment(output, 58, inputs.n5, 0, 5);
      segment(output, 56, inputs.n5, 0, 2);
      segment(output, 55, inputs.tiny, 0, 1);
      segment(output, 48, inputs.wide, 62, 7); // nested concat then slice
      segment(output, 36, inputs.n5, 0, 5);
      segment(output, 35, inputs.tiny, 0, 1);
      for (unsigned bit = 41; bit != 48; ++bit) {
        require(!output.packed().value().bit(bit));
        require(output.packed().knownMask().bit(bit));
        require(!output.packed().zMask().bit(bit));
      }
      segment(output, 20, inputs.wide, 57,
              15); // explicit slice before narrowing
      segment(output, 17, inputs.n5, 1, 3);
      segment(output, 16, inputs.tiny, 0, 1);
      segment(output, 10, inputs.tiny, 0, 1);
      const auto &packed = output.packed();
      if (inputs.n5.isFullyKnown()) {
        const unsigned expected = (inputs.n5.value().value() + 1) % 32;
        for (unsigned bit = 0; bit != 5; ++bit) {
          require(packed.knownMask().bit(11 + bit));
          require(!packed.zMask().bit(11 + bit));
          require(packed.value().bit(11 + bit) == bool((expected >> bit) & 1));
        }
      } else
        for (unsigned bit = 11; bit != 16; ++bit) {
          require(!packed.knownMask().bit(bit));
          require(!packed.zMask().bit(bit));
        }
      segment(output, 7, inputs.state, 0, 3);
      segment(output, 6, inputs.tiny, 0, 1);
      segment(output, 1, inputs.n5, 0, 5);
      segment(output, 0, inputs.tiny, 0, 1);
      std::cout << (masked ? "MASK " : "WORK ") << symbols<176>(output) << '\n';
    }
}
