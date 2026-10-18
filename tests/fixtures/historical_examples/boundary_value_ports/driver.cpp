#include "gfsim/SimExecutor.h"
#include "gfsim/SystemRunner.h"
#include "pycircuit_system.hpp"

// Observe the declared packed result without discarding X/Z planes. The
// historical stimulus, expected values and checks below remain unchanged.
struct ObservedOutputs {
  gfsim::wire<gfsim::Bits<32>> acc;
};
ObservedOutputs observe(const pyc_dut::Outputs &output) {
  return {
      gfsim::wire<gfsim::Bits<32>>::fromPacked(
          gfsim::extract<32>(output.result.packed(), 0)),
  };
}

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <source_location>
#include <string_view>

void require(bool ok,
             std::source_location at = std::source_location::current()) {
  if (!ok) {
    std::cerr << "boundary_value_ports oracle failed at " << at.line() << '\n';
    std::abort();
  }
}
auto known(std::uint32_t value) {
  return gfsim::wire<gfsim::Bits<32>>::known(gfsim::Bits<32>{value});
}
struct Row {
  std::uint32_t seed, acc;
};
// Independent fixed values: original10->48, lane wraps at seed+12/seed+6,
// sum wrap, bit31, alternating high bits, and changing consecutive epochs.
constexpr Row rows[] = {
    {10, 48},
    {0, 18},
    {1, 21},
    {4294967283u, 4294967275u},
    {4294967284u, 4294967278u},
    {4294967289u, 4294967293u},
    {4294967290u, 0},
    {4294967291u, 3},
    {4294967294u, 12},
    {4294967295u, 15},
    {2147483648u, 2147483666u},
    {2147483647u, 2147483663u},
    {1073741824u, 3221225490u},
    {1431655765u, 17},
    {2863311530u, 16},
    {17, 69},
};
struct RunnerContext {
  pyc_dut &dut;
  unsigned sampled = 0;
  static void initialize(void *opaque) {
    require(static_cast<RunnerContext *>(opaque)->sampled == 0);
    require(drive(opaque, 0));
  }
  static bool drive(void *opaque, std::uint64_t epoch) {
    auto &self = *static_cast<RunnerContext *>(opaque);
    if (epoch == std::size(rows))
      return false;
    require(epoch < std::size(rows));
    pyc_dut::Inputs inputs;
    inputs.seed = known(rows[epoch].seed);
    self.dut.drive(inputs);
    return true;
  }
  static void sample(void *opaque, std::uint64_t epoch) {
    auto &self = *static_cast<RunnerContext *>(opaque);
    require(epoch == self.sampled + 1 && self.sampled < std::size(rows));
    const auto output = observe(self.dut.sample());
    require(output.acc.isFullyKnown() &&
            output.acc.value().value() == rows[self.sampled].acc);
    std::cout << "WORK " << output.acc.value().value() << '\n';
    ++self.sampled;
  }
};

void nativeUnknownAndRecovery(unsigned workers) {
  pyc_dut dut(workers);
  gfsim::SimExecutor executor(dut.system(), dut.observations(), {});
  constexpr std::string_view config = "{}";
  require(executor.ConfigureJson(
              reinterpret_cast<const std::uint8_t *>(config.data()),
              config.size()) == PYCIRCUIT_MODEL_STATUS_V1_OK);
  pyc_dut::Inputs inputs;
  inputs.seed = known(0);
  dut.drive(inputs);
  require(executor.Reset() == PYCIRCUIT_MODEL_STATUS_V1_OK);
  struct Masks {
    std::uint32_t value, knownMask, zMask, recoverySeed, recoveryAcc;
  };
  constexpr Masks cases[] = {
      {0, 4294967294u, 0, 10, 48},
      {0, 2147483647u, 2147483648u, 4294967295u, 15},
      {0, 0, 0, 10, 48},
      {0, 0, 4294967295u, 4294967295u, 15},
  };
  auto step = [&] {
    dut.drive(inputs);
    PycircuitModelStepResultV1 result{sizeof(result)};
    require(executor.Step(&result) == PYCIRCUIT_MODEL_STATUS_V1_OK);
    require(result.state == PYCIRCUIT_MODEL_STEP_V1_RUNNING);
    return observe(dut.sample());
  };
  for (const auto &row : cases) {
    inputs.seed = gfsim::wire<gfsim::Bits<32>>::fromPacked(
        gfsim::FourState<32>::fromMasks(gfsim::Bits<32>{row.value},
                                        gfsim::Bits<32>{row.knownMask},
                                        gfsim::Bits<32>{row.zMask}));
    const auto output = step();
    require(output.acc.packed().knownMask() == gfsim::Bits<32>{0});
    require(output.acc.packed().zMask() == gfsim::Bits<32>{0});
    // Recover on the next epoch without Reset; all lanes and the sum are pure.
    inputs.seed = known(row.recoverySeed);
    const auto recovered = step();
    require(recovered.acc.isFullyKnown() &&
            recovered.acc.value().value() == row.recoveryAcc);
  }
}

int main(int argc, char **argv) {
  gfsim::SystemRunner runner(argc, argv);
  if (!runner.ready())
    return 2;
  pyc_dut dut(runner.workers());
  RunnerContext context{dut};
  const gfsim::RunnerCallbacks callbacks{&context, &RunnerContext::initialize,
                                         &RunnerContext::drive,
                                         &RunnerContext::sample};
  const int result =
      runner.Run(dut.system(), dut.observations(), {}, callbacks);
  require(result == 0 && context.sampled == std::size(rows));
  for (unsigned workers : {1u, 2u})
    nativeUnknownAndRecovery(workers);
  return result;
}
