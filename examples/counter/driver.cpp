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
    std::cerr << "counter oracle failed at " << at.line() << '\n';
    std::abort();
  }
}

auto bit(unsigned value) {
  return gfsim::wire<gfsim::Bits<1>>::known(gfsim::Bits<1>{value});
}

struct Row {
  unsigned clock, reset, enable, expected;
};
// Fixed post-wrap controls: hold, repeated high/low levels, reset priority,
// then resumed incrementing. None changes init or writes the register state.
constexpr Row tail[] = {
    {0, 0, 1, 0}, {1, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 1, 0}, {1, 0, 1, 0},
    {1, 0, 1, 1}, {1, 0, 1, 1}, {0, 0, 1, 1}, {0, 0, 1, 1}, {1, 0, 1, 1},
    {0, 0, 1, 2}, {1, 0, 0, 2}, {0, 0, 0, 2}, {1, 1, 0, 2}, {0, 1, 0, 0},
    {1, 0, 1, 0}, {0, 0, 1, 1},
};
constexpr unsigned enabledEdges = 256;
constexpr unsigned rampFrames = 2 * enabledEdges;
constexpr unsigned knownFrames = rampFrames + std::size(tail);

Row knownRow(unsigned epoch) {
  require(epoch < knownFrames);
  if (epoch < rampFrames)
    // The independent golden sequence is exactly 0,0,1,1,...,255,255.
    // Each subsequent low sample sees the preceding rising-edge commit;
    // the original post-edge counts 1..5 occur in low frames 2..10.
    return {epoch % 2, 0, 1, epoch / 2};
  return tail[epoch - rampFrames];
}

struct RunnerContext {
  pyc_dut &dut;
  unsigned sampled = 0;
  static void initialize(void *opaque) {
    auto &self = *static_cast<RunnerContext *>(opaque);
    require(self.sampled == 0);
    require(drive(opaque, 0));
  }
  static bool drive(void *opaque, std::uint64_t epoch) {
    auto &self = *static_cast<RunnerContext *>(opaque);
    if (epoch == knownFrames)
      return false;
    require(epoch < knownFrames);
    const auto row = knownRow(static_cast<unsigned>(epoch));
    pyc_dut::Inputs inputs;
    inputs.pyc_7079635f636c6b = bit(row.clock);
    inputs.pyc_7079635f727374 = bit(row.reset);
    inputs.enable = bit(row.enable);
    self.dut.drive(inputs);
    return true;
  }
  static void sample(void *opaque, std::uint64_t epoch) {
    auto &self = *static_cast<RunnerContext *>(opaque);
    require(epoch == self.sampled + 1 && self.sampled < knownFrames);
    const auto row = knownRow(self.sampled);
    const auto output = self.dut.sample();
    require(output.result.isFullyKnown());
    require(output.result.packed().value().value() == row.expected);
    std::cout << "WORK " << output.result.packed().value().value() << '\n';
    ++self.sampled;
  }
};

void nativeUnknownSelector(unsigned workers, unsigned startingCount, bool z) {
  require(startingCount == 0 || startingCount == 3);
  pyc_dut dut(workers);
  gfsim::SimExecutor executor(dut.system(), dut.observations(), {});
  constexpr std::string_view config = "{}";
  require(executor.ConfigureJson(
              reinterpret_cast<const std::uint8_t *>(config.data()),
              config.size()) == PYCIRCUIT_MODEL_STATUS_V1_OK);
  pyc_dut::Inputs inputs;
  inputs.pyc_7079635f636c6b = bit(0);
  inputs.pyc_7079635f727374 = bit(0);
  inputs.enable = bit(1);
  dut.drive(inputs);
  require(executor.Reset() == PYCIRCUIT_MODEL_STATUS_V1_OK);

  auto step = [&](unsigned clock, unsigned expectedValue,
                  unsigned expectedKnown) {
    inputs.pyc_7079635f636c6b = bit(clock);
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
  // Reach Q3 through exactly three real enabled rising edges, never a poke or
  // alternate init. A separate fresh DUT supplies the Q0 cases.
  for (unsigned count = 0; count < startingCount; ++count) {
    step(1, count, 255);
    step(0, count + 1, 255);
  }
  inputs.enable = gfsim::wire<gfsim::Bits<1>>::fromPacked(
      z ? gfsim::FourState<1>::highImpedance()
        : gfsim::FourState<1>::unknown());
  step(1, startingCount, 255);
  const unsigned partialKnown = startingCount == 0 ? 254 : 248;
  // Independent exact patterns: Q0 becomes 0000000x, Q3 becomes 00000xxx.
  step(0, 0, partialKnown);
  inputs.enable = bit(0);
  step(1, 0, partialKnown);
  step(0, 0, partialKnown);
  inputs.pyc_7079635f727374 = bit(1);
  step(1, 0, partialKnown);
  step(0, 0, 255);
  inputs.pyc_7079635f727374 = bit(0);
  inputs.enable = bit(1);
  step(1, 0, 255);
  step(0, 1, 255);
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
  // These are full generated native counter checks. They emit no WORK rows:
  // the comparable RTL trace above uses known controls only.
  for (unsigned workers : {1u, 2u})
    for (unsigned start : {0u, 3u})
      for (bool z : {false, true})
        nativeUnknownSelector(workers, start, z);
  return result;
}
