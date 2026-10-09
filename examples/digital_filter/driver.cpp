// Independent signed FIR recurrence; no DUT sign-extension expression is
// reused.
#include "gfsim/SimExecutor.h"
#include "gfsim/SystemRunner.h"
#include "pycircuit_system.hpp"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <source_location>
#include <stdexcept>
#include <string_view>
#include <vector>

void require(bool ok,
             std::source_location at = std::source_location::current()) {
  if (!ok) {
    std::cerr << "digital_filter oracle failed at " << at.line() << '\n';
    std::abort();
  }
}
template <unsigned W> auto known(std::uint64_t value) {
  return gfsim::wire<gfsim::Bits<W>>::known(gfsim::Bits<W>{value});
}
template <unsigned W> auto uncertain(bool z) {
  return gfsim::wire<gfsim::Bits<W>>::fromPacked(
      z ? gfsim::FourState<W>::highImpedance()
        : gfsim::FourState<W>::unknown());
}
constexpr std::uint64_t outputMask = (std::uint64_t{1} << 34) - 1;
struct Row {
  unsigned clock = 0, reset = 0, raw = 0, valid = 0;
};
std::vector<Row> stimulus() {
  std::vector<Row> rows;
  auto row = [&](unsigned clock, unsigned reset, int sample, unsigned valid) {
    rows.push_back(
        {clock, reset, static_cast<unsigned>(sample) & 65535u, valid});
  };
  // Preserve every historical emulator scenario, plus negative impulse and
  // both true signed16 boundaries. Each scenario starts with a real reset edge.
  constexpr int scenarios[][8] = {{1, 0, 0, 0, 0, 0, 0, 0},
                                  {-1, 0, 0, 0, 0, 0, 0, 0},
                                  {1, 1, 1, 1, 1, 1, 1, 1},
                                  {0, 1, 2, 3, 4, 5, 6, 7},
                                  {100, -100, 100, -100, 100, -100, 100, -100},
                                  {10000, 10000, 10000, 10000, 0, 0, 0, 0},
                                  {32767, 32767, 32767, 32767, 0, 0, 0, 0},
                                  {-32768, -32768, -32768, -32768, 0, 0, 0, 0}};
  for (const auto &sequence : scenarios) {
    row(0, 1, 999, 1);
    row(1, 1, 999, 1);
    row(0, 0, 0, 0);
    for (int sample : sequence) {
      row(0, 0, sample, 1);
      row(1, 0, sample, 1);
    }
    row(0, 0, 12345, 0);
  }
  row(0, 1, 999, 1);
  row(1, 1, 999, 1);
  row(0, 0, 0, 0);
  for (int sample : {3, -7, 11}) {
    row(0, 0, sample, 1);
    row(1, 0, sample, 1);
  }
  row(0, 0, 222, 0);
  // Thirty-two invalid edges change input but preserve all three histories.
  for (unsigned n = 0; n != 32; ++n) {
    row(0, 0, n * 7919u, 0);
    row(1, 0, 65535u - n * 3571u, 0);
  }
  row(1, 0, 32767, 1);
  row(1, 1, -32768, 1);
  row(0, 1, 1234, 1);
  row(0, 0, -1234, 1);
  row(1, 0, -17, 1);
  row(0, 0, 101, 0);
  row(1, 1, 4567, 1);
  row(1, 0, -4567, 1);
  row(0, 0, -123, 1);
  row(1, 0, -123, 1);
  row(1, 0, 123, 1);
  row(0, 0, 456, 1);
  row(0, 0, 789, 0);
  row(1, 0, 999, 0);
  row(0, 0, 111, 0);
  std::uint32_t random = 0x9273a61du;
  for (unsigned n = 0; n != 64; ++n) {
    random = random * 1664525u + 1013904223u;
    row(0, 0, random >> 16, n % 7 != 0);
    row(1, 0, random >> 16, n % 7 != 0);
  }
  row(0, 0, 0, 0);
  return rows;
}
struct Golden {
  std::array<std::int64_t, 3> history{};
  std::uint64_t output = 0;
  unsigned valid = 0;
  bool lastClock = false;
  void commit(Row row) {
    if (row.clock && !lastClock) {
      if (row.reset) {
        history = {};
        output = 0;
        valid = 0;
      } else {
        if (row.valid) {
          const std::int64_t sample =
              row.raw < 32768 ? row.raw
                              : static_cast<std::int64_t>(row.raw) - 65536;
          const auto sum =
              sample + 2 * history[0] + 3 * history[1] + 4 * history[2];
          output = static_cast<std::uint64_t>(sum) & outputMask;
          history = {sample, history[0], history[1]};
        }
        valid = row.valid;
      }
    }
    lastClock = row.clock;
  }
};
pyc_dut::Inputs inputs(Row row) {
  pyc_dut::Inputs in;
  in.pyc_7079635f636c6b = known<1>(row.clock);
  in.pyc_7079635f727374 = known<1>(row.reset);
  in.x_in = known<16>(row.raw);
  in.x_valid = known<1>(row.valid);
  return in;
}
void drive(pyc_root &root, const pyc_dut::Inputs &in) {
  root.pyc_7079635f636c6b = in.pyc_7079635f636c6b;
  root.pyc_7079635f727374 = in.pyc_7079635f727374;
  root.x_in = in.x_in;
  root.x_valid = in.x_valid;
}
template <class Output>
void check(const Output &output, const Golden &golden, bool emit = false) {
  const auto &packed = output.result.packed();
  require(output.result.isFullyKnown());
  const auto data = gfsim::extract<34>(packed, 1).value().value();
  const auto valid = gfsim::extract<1>(packed, 0).value().value();
  require(data == golden.output && valid == golden.valid);
  if (emit)
    std::cout << "WORK " << data << ' ' << valid << '\n';
}
struct RunnerContext {
  pyc_dut &dut;
  const std::vector<Row> rows = stimulus();
  Golden golden;
  unsigned sampled = 0;
  static void initialize(void *opaque) { require(driveInputs(opaque, 0)); }
  static bool driveInputs(void *opaque, std::uint64_t epoch) {
    auto &self = *static_cast<RunnerContext *>(opaque);
    if (epoch == self.rows.size())
      return false;
    require(epoch < self.rows.size());
    self.dut.drive(inputs(self.rows[epoch]));
    return true;
  }
  static void sample(void *opaque, std::uint64_t epoch) {
    auto &self = *static_cast<RunnerContext *>(opaque);
    require(epoch == self.sampled + 1 && self.sampled < self.rows.size());
    check(self.dut.sample(), self.golden, true);
    self.golden.commit(self.rows[self.sampled]);
    ++self.sampled;
  }
};
void directLifecycle(unsigned workers) {
  // Independently retry failed X/Z clock/reset controls at the same high level.
  for (unsigned failure = 0; failure != 5; ++failure) {
    gfsim::WorkExecutor pool(workers);
    pyc_root root("lifecycle_probe", &pool);
    Golden golden;
    drive(root, inputs({}));
    root.Reset();
    root.Xfer();
    auto work = [&](Row row) {
      drive(root, inputs(row));
      root.Work();
      check(root, golden);
      root.Xfer();
      golden.commit(row);
    };
    for (unsigned raw : {3u, 65529u, 11u}) {
      work({0, 0, raw, 1});
      work({1, 0, raw, 1});
    }
    work({0, 0, 0, 0});
    const Row retry{1, 0, 65523, 1};
    auto bad = inputs(retry);
    if (failure == 0) {
      drive(root, bad);
      root.Work();
      check(root, golden);
      root.DiscardNext();
      root.Xfer();
      // Reset is a proposal too: cancelling it must retain all five old Qs.
      bad.pyc_7079635f727374 = known<1>(1);
      drive(root, bad);
      root.Work();
      check(root, golden);
      root.DiscardNext();
      root.Xfer();
    } else {
      if (failure <= 2)
        bad.pyc_7079635f636c6b = uncertain<1>(failure == 2);
      else
        bad.pyc_7079635f727374 = uncertain<1>(failure == 4);
      drive(root, bad);
      bool rejected = false;
      try {
        root.Work();
      } catch (const gfsim::FourStateViolation &) {
        rejected = true;
      }
      require(rejected);
      root.DiscardNext();
      root.Xfer();
    }
    work(retry);
    // Distinct subsequent samples expose every retained or wrongly shifted tap.
    for (unsigned raw : {17u, 65517u, 23u, 0u}) {
      work({0, 0, raw, 1});
      work({1, 0, raw, 1});
    }
    work({0, 0, 0, 0});
    // Host Reset, including after populated clock history, clears every owner.
    drive(root, inputs({}));
    root.Reset();
    root.Xfer();
    golden = {};
    work({0, 0, 0, 0});
    work({1, 0, 1, 1});
    work({0, 0, 0, 0});
  }
}
void executorFailure(unsigned workers, bool z) {
  pyc_dut dut(workers);
  gfsim::SimExecutor executor(dut.system(), dut.observations(), {});
  constexpr std::string_view config =
      "{\"deadlock_window\":null,\"max_domain_cycles\":{},\"max_ticks\":64,"
      "\"schema\":\"pycircuit-model-config\",\"version\":\"1\"}";
  require(executor.ConfigureJson(
              reinterpret_cast<const std::uint8_t *>(config.data()),
              config.size()) == PYCIRCUIT_MODEL_STATUS_V1_OK);
  dut.drive(inputs({}));
  require(executor.Reset() == PYCIRCUIT_MODEL_STATUS_V1_OK);
  Golden golden;
  auto step = [&](Row row) {
    dut.drive(inputs(row));
    PycircuitModelStepResultV1 result{sizeof(result)};
    require(executor.Step(&result) == PYCIRCUIT_MODEL_STATUS_V1_OK);
    check(dut.sample(), golden);
    golden.commit(row);
  };
  for (unsigned raw : {3u, 65529u, 11u}) {
    step({0, 0, raw, 1});
    step({1, 0, raw, 1});
  }
  step({0, 0, 0, 0});
  auto bad = inputs({1, 0, 65523, 1});
  bad.pyc_7079635f636c6b = uncertain<1>(z);
  dut.drive(bad);
  const auto epoch = executor.cycles();
  PycircuitModelStepResultV1 failed{sizeof(failed)};
  require(executor.Step(&failed) == PYCIRCUIT_MODEL_STATUS_V1_RUNTIME_FAILURE);
  require(failed.state == PYCIRCUIT_MODEL_STEP_V1_FAILED &&
          failed.epoch_time == epoch);
  require(executor.cycles() == epoch && dut.system().cycle() == epoch);
  bool rejected = false;
  try {
    (void)dut.sample();
  } catch (const std::logic_error &) {
    rejected = true;
  }
  require(rejected);
  require(executor.Step(&failed) == PYCIRCUIT_MODEL_STATUS_V1_INVALID_STATE);
  dut.drive(inputs({}));
  require(executor.Reset() == PYCIRCUIT_MODEL_STATUS_V1_OK);
  golden = {};
  step({0, 0, 0, 0});
  step({1, 0, 65535, 1});
  step({0, 0, 0, 0});
}
struct Masks {
  std::uint64_t value, known, z;
};
template <unsigned W>
void checkPlane(const gfsim::FourState<W> &actual, Masks expected) {
  require(actual.knownMask() == gfsim::Bits<W>{expected.known});
  require(actual.zMask() == gfsim::Bits<W>{expected.z});
  require((actual.value() & actual.knownMask()) ==
          gfsim::Bits<W>{expected.value});
}
void nativeFourState(unsigned workers) {
  for (bool z : {false, true})
    for (unsigned test = 0; test != 4; ++test) {
      gfsim::WorkExecutor pool(workers);
      pyc_root root("four_state_probe", &pool);
      auto in = inputs({});
      drive(root, in);
      root.Reset();
      root.Xfer();
      Masks expectedData{0, outputMask, 0}, expectedValid{0, 1, 0};
      auto work = [&](unsigned clock, unsigned reset = 0) {
        in.pyc_7079635f636c6b = known<1>(clock);
        in.pyc_7079635f727374 = known<1>(reset);
        drive(root, in);
        root.Work();
        checkPlane(gfsim::extract<34>(root.result.packed(), 1), expectedData);
        checkPlane(gfsim::extract<1>(root.result.packed(), 0), expectedValid);
        root.Xfer();
      };
      work(0);
      if (test < 2) {
        in.x_valid = uncertain<1>(z);
        in.x_in = known<16>(test);
        work(1);
        expectedData = {0, outputMask ^ test, 0};
        expectedValid = {0, 0, z ? 1u : 0u};
        work(0);
        work(0); // equal branches stay known; only differing bit0 becomes X.
      } else {
        in.x_in = test == 3
                      ? uncertain<16>(z)
                      : gfsim::wire<gfsim::Bits<16>>::fromPacked(
                            gfsim::FourState<16>::fromMasks(
                                gfsim::Bits<16>{1}, gfsim::Bits<16>{65407},
                                gfsim::Bits<16>{z ? 128u : 0u}));
        // Known inactive input must not contaminate data or histories.
        in.x_valid = known<1>(0);
        work(1);
        work(0);
        in.x_valid = known<1>(1);
        work(1);
        expectedData = {0, 0, 0};
        expectedValid = {1, 1, 0};
        work(0);
      }
      // Invalid unknown data holds output/history but unconditionally clears
      // valid.
      in.x_in = uncertain<16>(!z);
      in.x_valid = known<1>(0);
      work(1);
      expectedValid = {0, 1, 0};
      work(0);
      work(1);
      work(0);
      in.x_in = known<16>(0);
      in.x_valid = known<1>(1);
      for (unsigned edge = 1; edge <= 4; ++edge) {
        work(1);
        expectedData = {0, (test == 0 || edge == 4) ? outputMask : 0, 0};
        expectedValid = {1, 1, 0};
        work(0);
      }
      // Recontaminate populated storage, then clocked reset dominates uncertain
      // valid.
      in.x_in = uncertain<16>(z);
      work(1);
      expectedData = {0, 0, 0};
      work(0);
      in.x_valid = uncertain<1>(z);
      work(1, 1);
      expectedData = {0, outputMask, 0};
      expectedValid = {0, 1, 0};
      work(0);
      in.x_in = known<16>(65535);
      in.x_valid = known<1>(1);
      work(1);
      expectedData = {outputMask, outputMask, 0};
      expectedValid = {1, 1, 0};
      work(0);
    }
}
int main(int argc, char **argv) {
  gfsim::SystemRunner runner(argc, argv);
  if (!runner.ready())
    return 2;
  pyc_dut dut(runner.workers());
  RunnerContext context{dut};
  const gfsim::RunnerCallbacks callbacks{&context, &RunnerContext::initialize,
                                         &RunnerContext::driveInputs,
                                         &RunnerContext::sample};
  const int result =
      runner.Run(dut.system(), dut.observations(), {}, callbacks);
  require(result == 0 && context.sampled == context.rows.size());
  directLifecycle(runner.workers());
  executorFailure(runner.workers(), false);
  executorFailure(runner.workers(), true);
  nativeFourState(runner.workers());
  return result;
}
