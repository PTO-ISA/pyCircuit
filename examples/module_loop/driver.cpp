#include "gfsim/SimExecutor.h"
#include "gfsim/SystemRunner.h"
#include "pycircuit_system.hpp"

#include <cstdlib>
#include <iostream>
#include <source_location>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

void require(bool condition,
             std::source_location at = std::source_location::current()) {
  if (!condition) {
    std::cerr << "module_loop oracle failed at " << at.line() << '\n';
    std::abort();
  }
}

auto bit(unsigned value) {
  return gfsim::wire<gfsim::Bits<1>>::known(gfsim::Bits<1>{value});
}

using Row = std::pair<unsigned, unsigned>;

std::vector<Row> run(unsigned workers) {
  pyc_dut dut(workers);
  gfsim::SimExecutor executor(dut.system(), dut.observations(), {});
  require(executor.created());
  constexpr std::string_view config = "{}";
  require(executor.ConfigureJson(
              reinterpret_cast<const std::uint8_t *>(config.data()),
              config.size()) == PYCIRCUIT_MODEL_STATUS_V1_OK);

  auto noSample = [&] {
    bool rejected = false;
    try {
      (void)dut.sample();
    } catch (const std::logic_error &) {
      rejected = true;
    }
    require(rejected);
  };
  noSample();
  pyc_dut::Inputs inputs;
  inputs.clk = bit(0);
  inputs.rst = bit(0);
  inputs.en_left = bit(1);
  inputs.en_right = bit(1);
  inputs.data_left = bit(1);
  inputs.data_right = bit(0);
  inputs.init_left = bit(0);
  inputs.init_right = bit(1);
  dut.drive(inputs);
  require(executor.Reset() == PYCIRCUIT_MODEL_STATUS_V1_OK);
  require(executor.cycles() == 0);
  noSample();

  std::vector<Row> trace;
  auto step = [&](unsigned clock, Row expected) {
    inputs.clk = bit(clock);
    dut.drive(inputs);
    PycircuitModelStepResultV1 result{sizeof(result)};
    require(executor.Step(&result) == PYCIRCUIT_MODEL_STATUS_V1_OK);
    require(result.state == PYCIRCUIT_MODEL_STEP_V1_RUNNING);
    const auto &output = dut.sample();
    require(output.left.isFullyKnown() && output.right.isFullyKnown());
    Row actual{static_cast<unsigned>(output.left.value().value()),
               static_cast<unsigned>(output.right.value().value())};
    require(actual == expected);
    trace.push_back(actual);
  };

  // Each row is the old-Q Work snapshot. Only the following Work sees a
  // successful transfer; no Eval or second evaluation is used as a refresh.
  step(0, {0, 1});
  step(1, {0, 1}); // proposes (1,1)
  step(0, {1, 1});
  inputs.en_left = bit(0);
  inputs.data_right = bit(1);
  step(1, {1, 1}); // holds left, proposes right=0
  step(0, {1, 0});

  // One module may have completed its proposal when the other fails. Step
  // must join workers, discard the whole tree and leave the epoch unchanged.
  inputs.en_left = bit(1);
  inputs.en_right = gfsim::wire<gfsim::Bits<1>>::unknown();
  inputs.clk = bit(1);
  dut.drive(inputs);
  const auto beforeFailure = executor.cycles();
  PycircuitModelStepResultV1 failed{sizeof(failed)};
  require(executor.Step(&failed) == PYCIRCUIT_MODEL_STATUS_V1_RUNTIME_FAILURE);
  require(failed.state == PYCIRCUIT_MODEL_STEP_V1_FAILED);
  require(failed.epoch_time == beforeFailure);
  require(executor.cycles() == beforeFailure);
  noSample();
  PycircuitModelStepResultV1 sticky{sizeof(sticky)};
  require(executor.Step(&sticky) == PYCIRCUIT_MODEL_STATUS_V1_INVALID_STATE);

  inputs.en_right = bit(1);
  inputs.data_right = bit(0);
  inputs.en_left = bit(1);
  inputs.clk = bit(0);
  dut.drive(inputs);
  require(executor.Reset() == PYCIRCUIT_MODEL_STATUS_V1_OK);
  noSample();
  step(0, {0, 1});
  step(1, {0, 1});
  step(0, {1, 1});
  // Clocked reset samples independent init pins, even when enables are low.
  inputs.rst = bit(1);
  inputs.en_left = bit(0);
  inputs.en_right = bit(0);
  step(1, {1, 1});
  step(0, {0, 1});
  return trace;
}

struct RunnerContext {
  pyc_dut &dut;
  pyc_dut::Inputs inputs;
  std::vector<Row> trace;
  bool initialized = false;
  static constexpr Row golden[] = {{0, 1}, {0, 1}, {1, 1}, {1, 1},
                                   {1, 0}, {1, 0}, {0, 1}, {0, 1},
                                   {1, 1}, {1, 1}, {0, 1}};
  static void initialize(void *opaque) {
    auto &self = *static_cast<RunnerContext *>(opaque);
    require(!self.initialized);
    self.initialized = true;
    self.inputs.clk = bit(0);
    self.inputs.rst = bit(0);
    self.inputs.en_left = bit(1);
    self.inputs.en_right = bit(1);
    self.inputs.data_left = bit(1);
    self.inputs.data_right = bit(0);
    self.inputs.init_left = bit(0);
    self.inputs.init_right = bit(1);
    self.dut.drive(self.inputs);
  }
  static bool drive(void *opaque, std::uint64_t epoch) {
    auto &self = *static_cast<RunnerContext *>(opaque);
    require(self.initialized);
    if (epoch == std::size(golden))
      return false;
    require(epoch < std::size(golden));
    static constexpr unsigned clock[] = {0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0};
    static constexpr unsigned reset[] = {0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 1};
    self.inputs.clk = bit(clock[epoch]);
    self.inputs.rst = bit(reset[epoch]);
    self.inputs.en_left =
        bit(epoch >= 3 && epoch <= 5 ? 0 : (epoch >= 9 ? 0 : 1));
    self.inputs.en_right = bit(epoch == 5 || epoch >= 9 ? 0 : 1);
    self.inputs.data_right = bit(epoch >= 3 && epoch <= 5 ? 1 : 0);
    self.dut.drive(self.inputs);
    return true;
  }
  static void sample(void *opaque, std::uint64_t committedEpoch) {
    auto &self = *static_cast<RunnerContext *>(opaque);
    require(committedEpoch == self.trace.size() + 1);
    require(self.trace.size() < std::size(golden));
    const auto &output = self.dut.sample();
    require(output.left.isFullyKnown() && output.right.isFullyKnown());
    Row row{static_cast<unsigned>(output.left.value().value()),
            static_cast<unsigned>(output.right.value().value())};
    require(row == golden[self.trace.size()]);
    self.trace.push_back(row);
    std::cout << "WORK " << row.first << ' ' << row.second << '\n';
  }
};

int main(int argc, char **argv) {
  gfsim::SystemRunner runner(argc, argv);
  if (!runner.ready())
    return 2;
  const std::vector<Row> faultGolden{{0, 1}, {0, 1}, {1, 1}, {1, 1}, {1, 0},
                                     {0, 1}, {0, 1}, {1, 1}, {1, 1}, {0, 1}};
  const auto serial = run(1);
  const auto parallel = run(2);
  require(serial == faultGolden && parallel == faultGolden);
  pyc_dut dut(runner.workers());
  RunnerContext context{dut};
  const gfsim::RunnerCallbacks callbacks{&context, &RunnerContext::initialize,
                                         &RunnerContext::drive,
                                         &RunnerContext::sample};
  const int result =
      runner.Run(dut.system(), dut.observations(), {}, callbacks);
  require(result == 0);
  require(context.trace.size() == std::size(RunnerContext::golden));
  return result;
}
