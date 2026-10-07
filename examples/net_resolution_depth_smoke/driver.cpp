#include "gfsim/SimExecutor.h"
#include "gfsim/SystemRunner.h"
#include "pycircuit_system.hpp"

#include <cstdlib>
#include <iostream>
#include <source_location>
#include <string_view>

void require(bool ok,
             std::source_location at = std::source_location::current()) {
  if (!ok) {
    std::cerr << "net_resolution_depth_smoke oracle failed at " << at.line()
              << '\n';
    std::abort();
  }
}
template <unsigned W> auto known(unsigned value) {
  return gfsim::wire<gfsim::Bits<W>>::known(gfsim::Bits<W>{value});
}
struct Row {
  unsigned clock, reset, data, expected;
};
// Two asserted reset cycles, then the original x1 pre0/post5 and x2 pre5/post6.
// Following low Work observes each preceding rising-edge commit.
constexpr Row prefix[] = {{0, 1, 0, 0},   {1, 1, 0, 0}, {0, 1, 255, 0},
                          {1, 1, 255, 0}, {0, 0, 1, 0}, {1, 0, 1, 0},
                          {0, 0, 2, 5},   {1, 0, 2, 5}, {0, 0, 2, 6}};
constexpr Row tail[] = {{0, 0, 100, 3},  {0, 0, 200, 3},  {1, 0, 17, 3},
                        {1, 0, 250, 21}, {1, 0, 0, 21},   {0, 0, 252, 21},
                        {0, 0, 255, 21}, {1, 1, 255, 21}, {0, 1, 255, 0},
                        {1, 0, 7, 0},    {0, 0, 1, 11}};
constexpr unsigned sweepFrames = 512;
constexpr unsigned knownFrames =
    std::size(prefix) + sweepFrames + std::size(tail);

Row knownRow(unsigned epoch) {
  require(epoch < knownFrames);
  if (epoch < std::size(prefix))
    return prefix[epoch];
  epoch -= std::size(prefix);
  if (epoch < sweepFrames) {
    const unsigned data = epoch / 2;
    // Independent fixed sweep of every input byte, with its old register
    // result at the edge and exactly one-edge new value on following Work.
    const unsigned before = data == 0 ? 6 : ((data + 3) & 255);
    return {epoch % 2 == 0 ? 1u : 0u, 0, data,
            epoch % 2 == 0 ? before : ((data + 4) & 255)};
  }
  return tail[epoch - sweepFrames];
}

struct RunnerContext {
  pyc_dut &dut;
  unsigned sampled = 0;
  static void initialize(void *opaque) {
    require(static_cast<RunnerContext *>(opaque)->sampled == 0);
    require(drive(opaque, 0));
  }
  static bool drive(void *opaque, std::uint64_t epoch) {
    auto &self = *static_cast<RunnerContext *>(opaque);
    if (epoch == knownFrames)
      return false;
    require(epoch < knownFrames);
    const auto row = knownRow(static_cast<unsigned>(epoch));
    pyc_dut::Inputs inputs;
    inputs.pyc_7079635f636c6b = known<1>(row.clock);
    inputs.pyc_7079635f727374 = known<1>(row.reset);
    inputs.in_x = known<8>(row.data);
    self.dut.drive(inputs);
    return true;
  }
  static void sample(void *opaque, std::uint64_t epoch) {
    auto &self = *static_cast<RunnerContext *>(opaque);
    require(epoch == self.sampled + 1 && self.sampled < knownFrames);
    const auto row = knownRow(self.sampled);
    const auto output = self.dut.sample();
    require(output.result.isFullyKnown() &&
            output.result.packed().value().value() == row.expected);
    std::cout << "WORK " << output.result.packed().value().value() << '\n';
    ++self.sampled;
  }
};

void nativeUnknownTiming(unsigned workers) {
  pyc_dut dut(workers);
  gfsim::SimExecutor executor(dut.system(), dut.observations(), {});
  constexpr std::string_view config = "{}";
  require(executor.ConfigureJson(
              reinterpret_cast<const std::uint8_t *>(config.data()),
              config.size()) == PYCIRCUIT_MODEL_STATUS_V1_OK);
  pyc_dut::Inputs inputs;
  inputs.pyc_7079635f636c6b = known<1>(0);
  inputs.pyc_7079635f727374 = known<1>(0);
  inputs.in_x = known<8>(1);
  dut.drive(inputs);
  require(executor.Reset() == PYCIRCUIT_MODEL_STATUS_V1_OK);
  auto step = [&](unsigned clock, unsigned expectedValue,
                  unsigned expectedKnown) {
    inputs.pyc_7079635f636c6b = known<1>(clock);
    dut.drive(inputs);
    PycircuitModelStepResultV1 result{sizeof(result)};
    require(executor.Step(&result) == PYCIRCUIT_MODEL_STATUS_V1_OK);
    require(result.state == PYCIRCUIT_MODEL_STEP_V1_RUNNING);
    const auto output = dut.sample();
    const auto &actual = output.result.packed();
    require(actual.knownMask() == gfsim::Bits<8>{expectedKnown});
    require(actual.zMask() == gfsim::Bits<8>{0});
    require((actual.value() & actual.knownMask()) ==
            gfsim::Bits<8>{expectedValue});
  };
  step(0, 0, 255);
  step(1, 0, 255);
  step(0, 5, 255);
  struct Masks {
    unsigned value, knownMask, zMask;
  };
  constexpr Masks cases[] = {
      {0, 254, 0}, {0, 127, 128}, {0, 0, 0}, {0, 0, 255}};
  for (const auto &row : cases) {
    const auto unknown =
        gfsim::wire<gfsim::Bits<8>>::fromPacked(gfsim::FourState<8>::fromMasks(
            gfsim::Bits<8>{row.value}, gfsim::Bits<8>{row.knownMask},
            gfsim::Bits<8>{row.zMask}));
    inputs.in_x = unknown;
    step(0, 5, 255); // Repeated low: X/Z data cannot alter current Q.
    step(1, 5, 255); // Old-Q Work, then capture arithmetic's all-X result.
    step(1, 0, 0);   // Repeated high retains the unknown register state.
    inputs.in_x = known<8>(2);
    step(1, 0, 0);
    step(0, 0, 0); // Known data and falling clock cannot recover Q.
    step(1, 0, 0);
    step(0, 6, 255); // Next real edge recovers, without Reset.
    inputs.in_x = unknown;
    inputs.pyc_7079635f727374 = known<1>(1);
    step(1, 6, 255);
    step(0, 0, 255); // Reset overrides X/Z data at the edge.
    inputs.pyc_7079635f727374 = known<1>(0);
    inputs.in_x = known<8>(1);
    step(1, 0, 255);
    step(0, 5, 255);
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
  require(result == 0 && context.sampled == knownFrames);
  // Actual full native DUT data-timing checks. Known RTL rows stay comparable;
  // Verilator is not counted as evidence for four-state register behavior.
  for (unsigned workers : {1u, 2u})
    nativeUnknownTiming(workers);
  return result;
}
