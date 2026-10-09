#include "gfsim/SimExecutor.h"
#include "pycircuit_system.hpp"
#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string_view>

void require(bool ok) { if (!ok) std::abort(); }
template <unsigned W> auto known(std::uint64_t v) {
  return gfsim::wire<gfsim::Bits<W>>::known(gfsim::Bits<W>{v});
}
template <typename T> std::uint64_t value(const T &v) {
  require(v.isFullyKnown());
  return v.value().value();
}

// Independent, fixed golden values exercise narrow/wide boundary bits and
// changing dependency paths. They are not calculated from DUT expressions.
constexpr std::uint64_t rows[][12] = {
  {0,0,0,0,0,0,0,8191,0,0,1,0},
  {0,8191,137438953471,0,1,0,137438953471,0,0,1,1,0},
  {8191,0,68719476736,1,0,0,68719476737,8191,68719476736,0,1,0},
  {5461,2730,68719476737,137438953471,1,0,68719476734,5461,137438953471,0,1,0},
  {4096,4095,68719476735,68719476736,0,0,137438953471,4096,68719476735,0,1,0},
  {8191,8191,137438953471,137438953471,1,8191,0,8191,137438953471,0,1,0},
  {1,2,3,5,0,0,6,8189,3,1,1,0},
  {2,1,5,3,1,0,6,8190,3,0,1,0},
};

int main(int argc, char **argv) {
  require(argc == 2);
  pyc_dut dut(std::strtoul(argv[1], nullptr, 10));
  gfsim::SimExecutor executor(dut.system(), dut.observations(), {});
  constexpr std::string_view config = "{}";
  require(executor.ConfigureJson(reinterpret_cast<const std::uint8_t *>(config.data()),
                                 config.size()) == PYCIRCUIT_MODEL_STATUS_V1_OK);
  require(executor.Reset() == PYCIRCUIT_MODEL_STATUS_V1_OK);
  for (const auto &r : rows) {
    pyc_dut::Inputs in;
    in.a13=known<13>(r[0]); in.b13=known<13>(r[1]);
    in.a37=known<37>(r[2]); in.b37=known<37>(r[3]); in.choose=known<1>(r[4]);
    dut.drive(in);
    PycircuitModelStepResultV1 step{sizeof(step)};
    require(executor.Step(&step) == PYCIRCUIT_MODEL_STATUS_V1_OK);
    const auto out=dut.sample();
    const std::array actual{value(out.masked13),value(out.xor37),value(out.mixed13),
                            value(out.pick37),value(out.less),value(out.truth),value(out.zero13)};
    std::cout << "WORK";
    for (unsigned i=0;i<actual.size();++i) {
      require(actual[i]==r[i+5]);
      std::cout << ' ' << actual[i];
    }
    std::cout << '\n';
  }
}
