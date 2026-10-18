// The original coerced comparison chain: eight known values, any unknown ->
// 0xxxxxxx.
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
    std::cerr << "countdown encoder oracle failed at " << at.line() << '\n';
    std::abort();
  }
}
template <class T>
concept HasClock = requires(T value) { value.pyc_7079635f636c6b; };
template <class T>
concept HasReset = requires(T value) { value.pyc_7079635f727374; };
static_assert(!HasClock<pyc_dut::Inputs> && !HasReset<pyc_dut::Inputs>);
auto pattern(unsigned code) {
  gfsim::Bits<3> value{0}, known{0}, z{0};
  for (unsigned bit = 0; bit < 3; ++bit) {
    const unsigned digit = code % 4;
    code /= 4;
    const unsigned one = 1u << bit;
    if (digit == 1)
      value = value | gfsim::Bits<3>{one};
    if (digit < 2)
      known = known | gfsim::Bits<3>{one};
    if (digit == 3)
      z = z | gfsim::Bits<3>{one};
  }
  return gfsim::wire<gfsim::Bits<3>>::fromPacked(
      gfsim::FourState<3>::fromMasks(value, known, z));
}
template <unsigned W, class Value> std::string text(const Value &value) {
  const auto &p = value.packed();
  std::string result;
  for (unsigned bit = W; bit != 0; --bit)
    result += p.zMask().bit(bit - 1)        ? 'z'
              : !p.knownMask().bit(bit - 1) ? 'x'
              : p.value().bit(bit - 1)      ? '1'
                                            : '0';
  return result;
}
int main(int argc, char **argv) {
  require(argc == 2);
  pyc_dut dut(static_cast<unsigned>(std::stoul(argv[1])));
  gfsim::SimExecutor executor(dut.system(), dut.observations(), {});
  constexpr std::string_view config =
      R"({"deadlock_window":null,"max_domain_cycles":{},"max_ticks":128,"schema":"pycircuit-model-config","version":"1"})";
  require(executor.ConfigureJson(
              reinterpret_cast<const std::uint8_t *>(config.data()),
              config.size()) == PYCIRCUIT_MODEL_STATUS_V1_OK);
  require(executor.Reset() == PYCIRCUIT_MODEL_STATUS_V1_OK);
  constexpr unsigned knownValues[] = {0x48, 0x49, 0x58, 0x59,
                                      0x5a, 0x5b, 0x5c, 0x5d};
  auto sample = [&](gfsim::wire<gfsim::Bits<3>> count) {
    pyc_dut::Inputs input;
    input.count = count;
    dut.drive(input);
    PycircuitModelStepResultV1 status{sizeof(status)};
    require(executor.Step(&status) == PYCIRCUIT_MODEL_STATUS_V1_OK);
    require(status.state == PYCIRCUIT_MODEL_STEP_V1_RUNNING);
    const auto output = dut.sample().result;
    const auto &p = output.packed();
    require(p.zMask() == gfsim::Bits<8>{0});
    if (count.isFullyKnown()) {
      require(output.isFullyKnown());
      require(p.value().value() == knownValues[count.value().value()]);
    } else {
      require(p.knownMask() == gfsim::Bits<8>{128});
      require((p.value() & p.knownMask()) == gfsim::Bits<8>{0});
    }
    return output;
  };
  unsigned knownCases = 0;
  for (unsigned code = 0; code < 64; ++code) {
    const auto input = pattern(code);
    const auto output = sample(input);
    std::cout << (input.isFullyKnown() ? "WORK " : "MASK ") << text<3>(input)
              << ' ' << text<8>(output) << '\n';
    if (input.isFullyKnown())
      ++knownCases;
    else {
      const auto recovery =
          gfsim::wire<gfsim::Bits<3>>::known(gfsim::Bits<3>{7});
      require(sample(recovery).isFullyKnown());
    }
  }
  require(knownCases == 8);
}
