#include "gfsim/SimExecutor.h"
#include "pycircuit_system.hpp"
#include <array>
#include <cstdlib>
#include <iostream>
#include <string_view>
void require(bool ok) {
  if (!ok)
    std::abort();
}
template <unsigned W> auto known(unsigned v) {
  return gfsim::wire<gfsim::Bits<W>>::known(gfsim::Bits<W>{v});
}
struct Row {
  unsigned a, c, d, integerInput, leftFlag, rightFlag, def, fifteen, alternate,
      computed, integerBit, booleanBit, integerEqual, booleanEqual,
      computedMask;
};
// Literal goldens for parameter actuals, computed types and same source kinds.
constexpr Row rows[] = {
    {0, 0, 255, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0},
    {255, 240, 15, 1, 0, 1, 255, 15, 170, 0, 0, 1, 1, 0, 0},
    {170, 170, 85, 0, 1, 0, 170, 10, 170, 0, 1, 1, 0, 0, 10},
    {85, 255, 173, 1, 1, 1, 85, 5, 0, 173, 0, 0, 1, 1, 15},
    {1, 128, 128, 0, 0, 0, 1, 1, 0, 128, 1, 0, 0, 1, 0},
    {254, 127, 129, 1, 0, 1, 254, 14, 170, 1, 0, 1, 1, 0, 15}};
int main(int argc, char **argv) {
  require(argc == 2);
  pyc_dut dut(std::strtoul(argv[1], nullptr, 10));
  gfsim::SimExecutor executor(dut.system(), dut.observations(), {});
  constexpr std::string_view config = "{}";
  require(executor.ConfigureJson(
              reinterpret_cast<const std::uint8_t *>(config.data()),
              config.size()) == PYCIRCUIT_MODEL_STATUS_V1_OK);
  require(executor.Reset() == PYCIRCUIT_MODEL_STATUS_V1_OK);
  for (const auto &r : rows) {
    pyc_dut::Inputs in;
    in.a = known<8>(r.a);
    in.c = known<8>(r.c);
    in.d = known<8>(r.d);
    in.integer_input = known<1>(r.integerInput);
    in.left_flag = known<1>(r.leftFlag);
    in.right_flag = known<1>(r.rightFlag);
    dut.drive(in);
    PycircuitModelStepResultV1 step{sizeof(step)};
    require(executor.Step(&step) == PYCIRCUIT_MODEL_STATUS_V1_OK);
    const auto out = dut.sample();
    const std::array words{out.default_left,   out.default_right,
                           out.fifteen_left,   out.fifteen_right,
                           out.alternate_left, out.alternate_right,
                           out.computed,       out.computed_left,
                           out.computed_right, out.signed_constant};
    const std::array expectedWords{
        r.def,       r.def,      r.fifteen,      r.fifteen,      r.alternate,
        r.alternate, r.computed, r.computedMask, r.computedMask, 255u};
    const std::array bits{out.integer_bit, out.boolean_bit, out.integer_equal,
                          out.boolean_equal};
    const std::array expectedBits{r.integerBit, r.booleanBit, r.integerEqual,
                                  r.booleanEqual};
    std::cout << "WORK";
    for (unsigned i = 0; i < words.size(); ++i) {
      require(words[i].isFullyKnown() &&
              words[i].value().value() == expectedWords[i]);
      std::cout << ' ' << words[i].value().value();
    }
    for (unsigned i = 0; i < bits.size(); ++i) {
      require(bits[i].isFullyKnown() &&
              bits[i].value().value() == expectedBits[i]);
      std::cout << ' ' << bits[i].value().value();
    }
    std::cout << '\n';
  }
}
