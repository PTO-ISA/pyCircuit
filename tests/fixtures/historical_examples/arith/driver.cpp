#include "gfsim/SystemRunner.h"
#include "pycircuit_system.hpp"

// Observe the declared packed result without discarding X/Z planes. The
// historical stimulus, expected values and checks below remain unchanged.
struct ObservedOutputs {
  gfsim::wire<gfsim::Bits<19>> sum;
  gfsim::wire<gfsim::Bits<16>> lane_mask;
  gfsim::wire<gfsim::Bits<8>> acc_width;
};
ObservedOutputs observe(const pyc_dut::Outputs &output) {
  return {
      gfsim::wire<gfsim::Bits<19>>::fromPacked(
          gfsim::extract<19>(output.result.packed(), 24)),
      gfsim::wire<gfsim::Bits<16>>::fromPacked(
          gfsim::extract<16>(output.result.packed(), 8)),
      gfsim::wire<gfsim::Bits<8>>::fromPacked(
          gfsim::extract<8>(output.result.packed(), 0)),
  };
}
#include <cstdint>
#include <cstdlib>
#include <iostream>

void require(bool ok) {
  if (!ok)
    std::abort();
}
auto known(unsigned value) {
  return gfsim::wire<gfsim::Bits<19>>::known(gfsim::Bits<19>{value});
}
struct Row {
  unsigned a, b, sum;
};
// Fixed golden values retain the original 1+2 case and cover 19-bit overflow.
constexpr Row rows[] = {{1, 2, 3},
                        {0, 0, 0},
                        {524287, 0, 524287},
                        {524287, 1, 0},
                        {524287, 524287, 524286},
                        {262144, 262144, 0},
                        {262143, 1, 262144},
                        {524286, 3, 1},
                        {273067, 174762, 447829},
                        {12345, 54321, 66666}};
struct Context {
  pyc_dut &dut;
  unsigned sampled = 0;
  static void initialize(void *opaque) {
    require(static_cast<Context *>(opaque)->sampled == 0);
    require(drive(opaque, 0));
  }
  static bool drive(void *opaque, std::uint64_t epoch) {
    auto &self = *static_cast<Context *>(opaque);
    if (epoch == std::size(rows))
      return false;
    require(epoch < std::size(rows));
    pyc_dut::Inputs inputs;
    inputs.a = known(rows[epoch].a);
    inputs.b = known(rows[epoch].b);
    self.dut.drive(inputs);
    return true;
  }
  static void sample(void *opaque, std::uint64_t epoch) {
    auto &self = *static_cast<Context *>(opaque);
    require(epoch == self.sampled + 1 && self.sampled < std::size(rows));
    const auto output = observe(self.dut.sample());
    require(output.sum.isFullyKnown() && output.lane_mask.isFullyKnown() &&
            output.acc_width.isFullyKnown());
    require(output.sum.value().value() == rows[self.sampled].sum);
    require(output.lane_mask.value().value() == 65535);
    require(output.acc_width.value().value() == 19);
    ++self.sampled;
    std::cout << "WORK " << output.sum.value().value() << " 65535 19\n";
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
  const int result =
      runner.Run(dut.system(), dut.observations(), {}, callbacks);
  require(result == 0 && context.sampled == std::size(rows));
  return result;
}
