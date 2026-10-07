#include "gfsim/SimExecutor.h"
#include "pycircuit_system.hpp"
#include <cstdlib>
#include <iostream>
#include <string_view>

void require(bool ok) {
  if (!ok)
    std::abort();
}
template <unsigned W> auto known(unsigned value) {
  return gfsim::wire<gfsim::Bits<W>>::known(gfsim::Bits<W>{value});
}
struct Row {
  unsigned clk, rst, en, a, b, q, held;
};
constexpr Row rows[] = {{0, 0, 1, 0, 0, 0, 0},     {1, 0, 1, 255, 255, 0, 0},
                        {0, 0, 1, 1, 2, 254, 1},   {1, 0, 0, 128, 127, 254, 1},
                        {0, 0, 1, 0, 255, 255, 1}, {1, 1, 1, 170, 85, 255, 1},
                        {0, 0, 1, 7, 9, 0, 0},     {1, 0, 1, 5, 3, 0, 0},
                        {0, 0, 1, 3, 5, 8, 15}};
int main(int argc, char **argv) {
  require(argc == 2);
  pyc_dut dut(std::strtoul(argv[1], nullptr, 10));
  gfsim::SimExecutor executor(dut.system(), dut.observations(), {});
  constexpr std::string_view config = "{}";
  require(executor.ConfigureJson(
              reinterpret_cast<const std::uint8_t *>(config.data()),
              config.size()) == PYCIRCUIT_MODEL_STATUS_V1_OK);
  pyc_dut::Inputs in;
  in.clk = known<1>(0);
  in.rst = known<1>(0);
  in.en = known<1>(1);
  in.a = known<8>(0);
  in.b = known<8>(0);
  dut.drive(in);
  require(executor.Reset() == PYCIRCUIT_MODEL_STATUS_V1_OK);
  for (const auto &r : rows) {
    in.clk = known<1>(r.clk);
    in.rst = known<1>(r.rst);
    in.en = known<1>(r.en);
    in.a = known<8>(r.a);
    in.b = known<8>(r.b);
    dut.drive(in);
    PycircuitModelStepResultV1 step{sizeof(step)};
    require(executor.Step(&step) == PYCIRCUIT_MODEL_STATUS_V1_OK);
    const auto out = dut.sample();
    require(out.q.isFullyKnown() && out.held.isFullyKnown());
    require(out.q.value().value() == r.q && out.held.value().value() == r.held);
    std::cout << "WORK " << out.q.value().value() << ' '
              << out.held.value().value() << '\n';
  }
}
