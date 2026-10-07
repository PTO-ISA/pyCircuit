#include "gfsim/SimExecutor.h"
#include "gfsim/SystemRunner.h"
#include "pycircuit_system.hpp"

// Observe the declared packed result without discarding X/Z planes. The
// historical stimulus, expected values and checks below remain unchanged.
struct ObservedOutputs {
  gfsim::wire<gfsim::Bits<8>> y;
};
ObservedOutputs observe(const pyc_dut::Outputs &output) {
  return {
      gfsim::wire<gfsim::Bits<8>>::fromPacked(
          gfsim::extract<8>(output.result.packed(), 0)),
  };
}

#include <cstdlib>
#include <iostream>
#include <source_location>
#include <string_view>

void require(bool ok,
             std::source_location at = std::source_location::current()) {
  if (!ok) {
    std::cerr << "hier_modules oracle failed at " << at.line() << '\n';
    std::abort();
  }
}
auto known(unsigned value) {
  return gfsim::wire<gfsim::Bits<8>>::known(gfsim::Bits<8>{value});
}
struct Row {
  unsigned x, y;
};
// Independent fixed combinational goldens: original1->4, wrap boundaries,
// then changing inputs every epoch. A register delay fails these rows.
constexpr Row rows[] = {{1, 4},     {0, 3},     {252, 255}, {253, 0},
                        {254, 1},   {255, 2},   {2, 5},     {127, 130},
                        {128, 131}, {250, 253}, {5, 8},     {17, 20}};
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
    inputs.x = known(rows[epoch].x);
    self.dut.drive(inputs);
    return true;
  }
  static void sample(void *opaque, std::uint64_t epoch) {
    auto &self = *static_cast<RunnerContext *>(opaque);
    require(epoch == self.sampled + 1 && self.sampled < std::size(rows));
    const auto output = observe(self.dut.sample());
    require(output.y.isFullyKnown() &&
            output.y.value().value() == rows[self.sampled].y);
    std::cout << "WORK " << output.y.value().value() << '\n';
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
  inputs.x = known(0);
  dut.drive(inputs);
  require(executor.Reset() == PYCIRCUIT_MODEL_STATUS_V1_OK);
  struct Masks {
    unsigned value, knownMask, zMask, recoveryX, recoveryY;
  };
  constexpr Masks cases[] = {{0, 254, 0, 1, 4},
                             {0, 127, 128, 255, 2},
                             {0, 0, 0, 1, 4},
                             {0, 0, 255, 255, 2}};
  auto step = [&] {
    dut.drive(inputs);
    PycircuitModelStepResultV1 result{sizeof(result)};
    require(executor.Step(&result) == PYCIRCUIT_MODEL_STATUS_V1_OK);
    require(result.state == PYCIRCUIT_MODEL_STEP_V1_RUNNING);
    return observe(dut.sample());
  };
  for (const auto &row : cases) {
    inputs.x =
        gfsim::wire<gfsim::Bits<8>>::fromPacked(gfsim::FourState<8>::fromMasks(
            gfsim::Bits<8>{row.value}, gfsim::Bits<8>{row.knownMask},
            gfsim::Bits<8>{row.zMask}));
    const auto output = step();
    require(output.y.packed().knownMask() == gfsim::Bits<8>{0});
    require(output.y.packed().zMask() == gfsim::Bits<8>{0});
    // No Reset between unknown data and the next known input: current-epoch
    // recovery is part of the original pure combinational hardware behavior.
    inputs.x = known(row.recoveryX);
    const auto recovered = step();
    require(recovered.y.isFullyKnown() &&
            recovered.y.value().value() == row.recoveryY);
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
