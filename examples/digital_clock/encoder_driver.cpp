// Exhaustive six-symbol encoder truth table, with ordinary unsigned divmod goldens.
#include "gfsim/SimExecutor.h"
#include "pycircuit_system.hpp"
#include <cstdlib>
#include <iostream>
#include <source_location>
#include <string>
#include <string_view>
void require(bool ok, std::source_location at = std::source_location::current()) {
  if (!ok) { std::cerr << "DigitalClock encoder oracle failed at " << at.line() << '\n'; std::abort(); }
}
template <class T> concept HasClock = requires(T value) { value.pyc_7079635f636c6b; };
template <class T> concept HasReset = requires(T value) { value.pyc_7079635f727374; };
static_assert(!HasClock<pyc_dut::Inputs> && !HasReset<pyc_dut::Inputs>);
auto pattern(unsigned code) {
  gfsim::Bits<6> value{0}, known{0}, z{0};
  for (unsigned bit = 0; bit < 6; ++bit) {
    const unsigned digit = code % 4; code /= 4; const unsigned one = 1u << bit;
    if (digit == 1 || (digit >= 2 && bit % 2 == 0)) value = value | gfsim::Bits<6>{one};
    if (digit < 2) known = known | gfsim::Bits<6>{one};
    if (digit == 3) z = z | gfsim::Bits<6>{one};
  }
  return gfsim::wire<gfsim::Bits<6>>::fromPacked(gfsim::FourState<6>::fromMasks(value, known, z));
}
template <unsigned W, class Value> std::string text(const Value &value) {
  const auto &p = value.packed(); std::string result;
  for (unsigned bit = W; bit; --bit)
    result += p.zMask().bit(bit - 1) ? 'z' : !p.knownMask().bit(bit - 1) ? 'x' : p.value().bit(bit - 1) ? '1' : '0';
  return result;
}
int main(int argc, char **argv) {
  require(argc == 2); pyc_dut dut(static_cast<unsigned>(std::stoul(argv[1])));
  gfsim::SimExecutor executor(dut.system(), dut.observations(), {});
  constexpr std::string_view config = R"({"deadlock_window":null,"max_domain_cycles":{},"max_ticks":8192,"schema":"pycircuit-model-config","version":"1"})";
  require(executor.ConfigureJson(reinterpret_cast<const std::uint8_t *>(config.data()), config.size()) == PYCIRCUIT_MODEL_STATUS_V1_OK);
  require(executor.Reset() == PYCIRCUIT_MODEL_STATUS_V1_OK);
  auto sample = [&](auto value) {
    pyc_dut::Inputs input; input.value = value; dut.drive(input);
    PycircuitModelStepResultV1 status{sizeof(status)};
    require(executor.Step(&status) == PYCIRCUIT_MODEL_STATUS_V1_OK && status.state == PYCIRCUIT_MODEL_STEP_V1_RUNNING);
    const auto output = dut.sample().result; const auto &p = output.packed();
    require(p.zMask() == gfsim::Bits<8>{0});
    if (value.isFullyKnown()) {
      const auto n = value.value().value(); require(output.isFullyKnown());
      require(p.value().value() == (n / 10) * 16 + n % 10);
    } else require(p.knownMask() == gfsim::Bits<8>{0});
    return output;
  };
  unsigned knownCases = 0;
  for (unsigned code = 0; code < 4096; ++code) {
    const auto value = pattern(code); const auto output = sample(value);
    std::cout << (value.isFullyKnown() ? "WORK " : "MASK ") << text<6>(value) << ' ' << text<8>(output) << '\n';
    if (value.isFullyKnown()) ++knownCases;
    else require(sample(gfsim::wire<gfsim::Bits<6>>::known(gfsim::Bits<6>{63})).isFullyKnown());
  }
  require(knownCases == 64 && executor.cycles() == 8128);
}
