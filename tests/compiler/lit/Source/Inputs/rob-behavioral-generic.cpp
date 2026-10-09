// Physical IR pyc_clk/pyc_rst use the existing injective generated identifier
// spelling: pyc_7079635f636c6b / pyc_7079635f727374.
#include "pycircuit_system.hpp"
#include "gfsim/SimExecutor.h"
#include <array>
#include <cstdlib>
#include <iostream>
#include <string_view>

void require(bool passed) { if (!passed) std::abort(); }
template <unsigned W> auto known(unsigned value) {
  return gfsim::wire<gfsim::Bits<W>>::known(gfsim::Bits<W>{value});
}
struct Row { unsigned clock, enable, index, value; };
constexpr std::array<Row, 6> tail{{{1,0,0,0},{1,1,0,67},{0,0,0,0},
                                  {0,1,0,88},{1,1,0,99},{0,0,0,0}}};
Row row(unsigned n) {
  if (n >= 256) return tail[n - 256];
  const unsigned item = n / 2;
  return {n % 2, n % 2, item % 8, item};
}
int main(int argc, char **argv) {
  require(argc == 2);
  pyc_dut dut(std::strtoul(argv[1], nullptr, 10));
  gfsim::SimExecutor executor(dut.system(), dut.observations(), {});
  constexpr std::string_view config = "{}";
  require(executor.ConfigureJson(reinterpret_cast<const std::uint8_t *>(config.data()),
                                 config.size()) == PYCIRCUIT_MODEL_STATUS_V1_OK);
  pyc_dut::Inputs in;
  in.pyc_7079635f636c6b=known<1>(0);in.pyc_7079635f727374=known<1>(0);
  in.enable=known<1>(0);in.index=known<3>(0);in.value=known<7>(0);
  dut.drive(in); require(executor.Reset() == PYCIRCUIT_MODEL_STATUS_V1_OK);
  std::array<unsigned, 8> values{}, active{};
  unsigned cursor=3, audit=341, previousClock=0;
  auto value = [](const auto &wire) {
    require(wire.isFullyKnown());return static_cast<unsigned>(wire.value().value());
  };
  for (unsigned n=0;n<262;++n) {
    const Row stimulus=row(n);
    const unsigned nextCursor=(cursor+stimulus.enable)%8;
    const unsigned nextAudit=(audit+5*stimulus.enable)%512;
    const unsigned snapshot=(cursor+2)%8;
    in.pyc_7079635f636c6b=known<1>(stimulus.clock);in.enable=known<1>(stimulus.enable);
    in.index=known<3>(stimulus.index);in.value=known<7>(stimulus.value);
    dut.drive(in);
    PycircuitModelStepResultV1 result{sizeof(result)};
    require(executor.Step(&result) == PYCIRCUIT_MODEL_STATUS_V1_OK);
    require(result.state == PYCIRCUIT_MODEL_STEP_V1_RUNNING);
    const auto out=dut.sample();
    require(value(out.prior_payload)==values[stimulus.index] && value(out.active)==active[stimulus.index]);
    require(value(out.cursor)==nextCursor && value(out.audit)==nextAudit);
    require(value(out.snapshot)==snapshot);
    std::cout << "WORK " << value(out.prior_payload) << ' ' << value(out.active) << ' '
              << value(out.cursor) << ' ' << value(out.audit) << ' '
              << value(out.snapshot) << '\n';
    if (stimulus.clock && !previousClock && stimulus.enable) {
      values[stimulus.index]=stimulus.value;active[stimulus.index]=1;
      cursor=nextCursor;audit=nextAudit;
    }
    previousClock=stimulus.clock;
  }
  require(values[0]==99 && cursor==4 && audit==474);
}
