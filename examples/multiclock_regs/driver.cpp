#include "gfsim/SimExecutor.h"
#include "gfsim/SystemRunner.h"
#include "pycircuit_system.hpp"
#include <cstdlib>
#include <iostream>
#include <source_location>
#include <stdexcept>
#include <string_view>

void require(bool ok,
             std::source_location at = std::source_location::current()) {
  if (!ok) {
    std::cerr << "multiclock_regs oracle failed at " << at.line() << '\n';
    std::abort();
  }
}
auto known(unsigned value) {
  return gfsim::wire<gfsim::Bits<1>>::known(gfsim::Bits<1>{value});
}
struct Row {
  unsigned ca, ra, cb, rb, a, b;
};
// This parity oracle explicitly resets BOTH lanes as its own preamble.
// It is separate from the historical unasserted A-only-reset smoke below.
constexpr Row prefix[] = {
    {0, 1, 0, 1, 0, 0}, {1, 1, 1, 1, 0, 0}, {0, 0, 0, 0, 0, 0},
    {1, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 1, 0}, {0, 0, 1, 0, 1, 0},
    {0, 0, 1, 0, 1, 1}, {1, 0, 0, 0, 1, 1}, {0, 0, 0, 0, 2, 1},
    {1, 0, 1, 0, 2, 1}, {1, 1, 1, 1, 3, 2}, {0, 1, 0, 1, 3, 2},
    {1, 1, 0, 1, 3, 2}, {0, 0, 0, 1, 0, 2}, {1, 0, 1, 1, 0, 2},
    {0, 1, 0, 0, 1, 0}, {0, 1, 1, 0, 1, 0}, {0, 1, 0, 0, 1, 1},
    {1, 1, 1, 0, 1, 1}, {0, 0, 0, 0, 0, 2}, {0, 0, 1, 1, 0, 2},
    {0, 0, 0, 0, 0, 0},
};
constexpr unsigned sweepFrames = 3 * 512;
constexpr unsigned knownFrames = std::size(prefix) + sweepFrames;
Row knownRow(unsigned epoch) {
  require(epoch < knownFrames);
  if (epoch < std::size(prefix))
    return prefix[epoch];
  epoch -= std::size(prefix);
  const unsigned phase = epoch / 512, slot = epoch % 512, before = slot / 2;
  const bool rising = slot % 2 == 0;
  const unsigned after = (before + 1) & 255;
  // Every lane's wrap is reached by 256 actual rising edges; neither
  // init, Reset nor direct state assignment shortcuts the sweep.
  const unsigned count = rising ? before : after;
  return {phase == 1 ? 0u : unsigned(rising),
          0,
          phase == 0 ? 0u : unsigned(rising),
          0,
          phase == 1 ? 0u : count,
          phase == 0 ? 0u : count};
}
void drive(pyc_dut &dut, const Row &row) {
  pyc_dut::Inputs in;
  in.clk_a = known(row.ca);
  in.rst_a = known(row.ra);
  in.clk_b = known(row.cb);
  in.rst_b = known(row.rb);
  dut.drive(in);
}
void check(const pyc_dut::Outputs &out, unsigned a, unsigned b) {
  require(out.a_count.isFullyKnown() && out.a_count.value().value() == a);
  require(out.b_count.isFullyKnown() && out.b_count.value().value() == b);
}
struct RunnerContext {
  pyc_dut &dut;
  unsigned sampled = 0;
  static void initialize(void *opaque) { require(drive(opaque, 0)); }
  static bool drive(void *opaque, std::uint64_t epoch) {
    auto &self = *static_cast<RunnerContext *>(opaque);
    if (epoch == knownFrames)
      return false;
    ::drive(self.dut, knownRow(epoch));
    return true;
  }
  static void sample(void *opaque, std::uint64_t epoch) {
    auto &self = *static_cast<RunnerContext *>(opaque);
    require(epoch == self.sampled + 1 && self.sampled < knownFrames);
    const auto row = knownRow(self.sampled);
    const auto out = self.dut.sample();
    check(out, row.a, row.b);
    std::cout << "WORK " << out.a_count.value().value() << ' '
              << out.b_count.value().value() << '\n';
    ++self.sampled;
  }
};
void configure(gfsim::SimExecutor &executor) {
  constexpr std::string_view config = "{}";
  require(executor.ConfigureJson(
              reinterpret_cast<const std::uint8_t *>(config.data()),
              config.size()) == PYCIRCUIT_MODEL_STATUS_V1_OK);
}
void step(gfsim::SimExecutor &executor) {
  PycircuitModelStepResultV1 result{sizeof(result)};
  require(executor.Step(&result) == PYCIRCUIT_MODEL_STATUS_V1_OK);
  require(result.state == PYCIRCUIT_MODEL_STEP_V1_RUNNING);
}
void historicalSmoke(unsigned workers) {
  pyc_dut dut(workers);
  gfsim::SimExecutor executor(dut.system(), dut.observations(), {});
  configure(executor);
  // The old C++ TB's Wire=Bits inputs defaulted to0, including rst_b.
  // This replay carries that simulator default; it does not reset B in RTL
  // or assert anything about B's physical startup value.
  ::drive(dut, {0, 1, 0, 0, 0, 0});
  require(executor.Reset() == PYCIRCUIT_MODEL_STATUS_V1_OK);
  for (unsigned cycle = 0; cycle < 6; ++cycle) {
    // Original A reset2 asserted /1 deasserted, followed by cycles0..2.
    // Both registered clocks have half_period1,phase0,start_high=false.
    const unsigned reset = cycle < 2 ? 1u : 0u;
    ::drive(dut, {1, reset, 1, 0, 0, 0});
    step(executor);
    ::drive(dut, {0, reset, 0, 0, 0, 0});
    step(executor);
  }
  // The historical test had no output assertions.
  require(executor.cycles() == 12);
}
auto unknown(bool highZ) {
  return gfsim::wire<gfsim::Bits<1>>::fromPacked(gfsim::FourState<1>::fromMasks(
      gfsim::Bits<1>{0}, gfsim::Bits<1>{0}, gfsim::Bits<1>{highZ ? 1u : 0u}));
}
void nativeUnknownContract(unsigned workers) {
  for (bool z : {false, true})
    for (unsigned pin = 0; pin < 4; ++pin) {
      pyc_dut dut(workers);
      gfsim::SimExecutor executor(dut.system(), dut.observations(), {});
      configure(executor);
      ::drive(dut, {0, 0, 0, 0, 0, 0});
      require(executor.Reset() == PYCIRCUIT_MODEL_STATUS_V1_OK);
      pyc_dut::Inputs in;
      in.clk_a = known(1);
      in.clk_b = known(1);
      in.rst_a = known(0);
      in.rst_b = known(0);
      if (pin == 0)
        in.clk_a = unknown(z);
      if (pin == 1)
        in.clk_b = unknown(z);
      if (pin == 2)
        in.rst_a = unknown(z);
      if (pin == 3)
        in.rst_b = unknown(z);
      dut.drive(in);
      PycircuitModelStepResultV1 result{sizeof(result)};
      require(executor.Step(&result) ==
              PYCIRCUIT_MODEL_STATUS_V1_RUNTIME_FAILURE);
      require(result.state == PYCIRCUIT_MODEL_STEP_V1_FAILED &&
              executor.cycles() == 0);
      require(dut.system().cycle() == 0);
      // A failed epoch publishes no sample through the generated DUT ABI.
      bool rejectedSample = false;
      try {
        (void)dut.sample();
      } catch (const std::logic_error &) {
        rejectedSample = true;
      }
      require(rejectedSample);
      // Recover through the public Reset contract after terminal failure.
      ::drive(dut, {0, 0, 0, 0, 0, 0});
      require(executor.Reset() == PYCIRCUIT_MODEL_STATUS_V1_OK);
      ::drive(dut, {1, 0, 1, 0, 0, 0});
      step(executor);
      ::drive(dut, {0, 0, 0, 0, 0, 0});
      step(executor);
      check(dut.sample(), 1, 1);
    }
  // Unknown resets are irrelevant without their OWN rising edge. The other
  // lane can still commit; each reset is independent from the other clock.
  for (bool z : {false, true})
    for (bool a : {false, true}) {
      pyc_dut dut(workers);
      gfsim::SimExecutor executor(dut.system(), dut.observations(), {});
      configure(executor);
      ::drive(dut, {0, 0, 0, 0, 0, 0});
      require(executor.Reset() == PYCIRCUIT_MODEL_STATUS_V1_OK);
      pyc_dut::Inputs in;
      in.clk_a = known(a ? 0 : 1);
      in.clk_b = known(a ? 1 : 0);
      in.rst_a = a ? unknown(z) : known(0);
      in.rst_b = a ? known(0) : unknown(z);
      dut.drive(in);
      step(executor);
      ::drive(dut, {0, 0, 0, 0, 0, 0});
      step(executor);
      check(dut.sample(), a ? 0 : 1, a ? 1 : 0);
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
  historicalSmoke(runner.workers());
  nativeUnknownContract(runner.workers());
  return result;
}
