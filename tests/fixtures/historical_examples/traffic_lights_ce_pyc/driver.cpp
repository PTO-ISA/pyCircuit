// Independent eleven-tick table, not the DUT's ordered next-state expressions.
#include "gfsim/SimExecutor.h"
#include "gfsim/SystemRunner.h"
#include "pycircuit_system.hpp"
#include <array>
#include <cstdlib>
#include <iostream>
#include <source_location>
#include <stdexcept>
#include <string_view>
#include <vector>

void require(bool ok,
             std::source_location at = std::source_location::current()) {
  if (!ok) {
    std::cerr << "traffic_lights oracle failed at " << at.line() << '\n';
    std::abort();
  }
}
template <unsigned W> auto known(unsigned value) {
  return gfsim::wire<gfsim::Bits<W>>::known(gfsim::Bits<W>{value});
}
auto uncertain(bool z) {
  return gfsim::wire<gfsim::Bits<1>>::fromPacked(
      z ? gfsim::FourState<1>::highImpedance()
        : gfsim::FourState<1>::unknown());
}
struct Tick {
  unsigned phase, ew, ns, blink;
};
constexpr Tick ticks[] = {{0, 3, 4, 0}, {0, 2, 3, 0}, {0, 1, 2, 0},
                          {0, 0, 1, 0}, {1, 1, 0, 0}, {1, 0, 0, 1},
                          {2, 3, 2, 0}, {2, 2, 1, 0}, {2, 1, 0, 0},
                          {3, 0, 1, 0}, {3, 0, 0, 1}};
constexpr unsigned display[] = {0x48, 0x49, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d};
constexpr unsigned resultMask = (1u << 22) - 1;
constexpr unsigned emergencyOutput = (0x88u << 14) | (0x88u << 6) | 0x24u;
unsigned expected(unsigned position, bool emergency) {
  if (emergency)
    return emergencyOutput;
  require(position < 44);
  const auto &tick = ticks[position / 4];
  const unsigned lights =
      ((tick.phase >= 2) << 5) | ((tick.phase == 1 && tick.blink) << 4) |
      ((tick.phase == 0) << 3) | ((tick.phase < 2) << 2) |
      ((tick.phase == 3 && tick.blink) << 1) | (tick.phase == 2);
  return (display[tick.ew] << 14) | (display[tick.ns] << 6) | lights;
}
struct Row {
  unsigned clock = 0, reset = 0, go = 1, emergency = 0;
};
struct Golden {
  unsigned position = 0;
  bool lastClock = false;
  void commit(Row row) {
    if (row.clock && !lastClock) {
      if (row.reset)
        position = 0;
      else if (row.go && !row.emergency)
        position = (position + 1) % 44;
    }
    lastClock = row.clock;
  }
};
std::vector<Row> stimulus() {
  std::vector<Row> rows;
  auto row = [&](unsigned clock, unsigned reset, unsigned go,
                 unsigned emergency) {
    rows.push_back({clock, reset, go, emergency});
  };
  auto edges = [&](unsigned count, unsigned go = 1, unsigned emergency = 0) {
    for (unsigned n = 0; n < count; ++n) {
      row(0, 0, go, emergency);
      row(1, 0, go, emergency);
    }
  };
  auto reset = [&] {
    row(0, 1, 1, 0);
    row(1, 1, 1, 0);
    row(0, 0, 1, 0);
  };
  edges(88);
  row(0, 0, 1, 0); // Two complete 44-enabled-edge cycles, all eight outputs.
  reset();
  edges(2);
  edges(5, 0);
  row(1, 0, 1, 0);
  edges(2);
  reset();
  edges(18);
  edges(5, 0);
  edges(4);
  edges(5, 0);
  // Immediate override/release at held high and low levels; no output register.
  row(1, 0, 1, 1);
  row(1, 0, 1, 0);
  row(0, 0, 1, 1);
  edges(8, 1, 1);
  row(0, 0, 1, 0);
  edges(16);
  edges(5, 0);
  edges(4);
  edges(5, 0);
  edges(2);
  edges(7);
  row(0, 1, 0, 1);
  row(1, 1, 0, 1);
  row(1, 0, 1, 0);
  row(0, 0, 1, 0);
  edges(5);
  row(1, 0, 0, 0);
  row(1, 0, 1, 0);
  row(0, 0, 0, 0);
  row(0, 1, 1, 0);
  row(1, 1, 0, 0);
  row(0, 0, 1, 0);
  // Reset while yellow is visibly lit: low reset holds phase/counts; the
  // rising reset wins over go=0 and restores the original reset displays.
  edges(22);
  row(0, 1, 0, 0);
  row(1, 1, 0, 0);
  row(0, 0, 1, 0);
  return rows;
}
pyc_dut::Inputs inputs(Row row) {
  pyc_dut::Inputs in;
  in.pyc_7079635f636c6b = known<1>(row.clock);
  in.pyc_7079635f727374 = known<1>(row.reset);
  in.go = known<1>(row.go);
  in.emergency = known<1>(row.emergency);
  return in;
}
void drive(pyc_root &root, const pyc_dut::Inputs &in) {
  root.pyc_7079635f636c6b = in.pyc_7079635f636c6b;
  root.pyc_7079635f727374 = in.pyc_7079635f727374;
  root.go = in.go;
  root.emergency = in.emergency;
}
template <class Output>
void check(const Output &output, unsigned golden, bool emit = false) {
  require(output.result.isFullyKnown());
  require(output.result.packed().value().value() == golden);
  if (emit)
    std::cout << "WORK " << ((golden >> 14) & 255) << ' '
              << ((golden >> 6) & 255) << ' ' << ((golden >> 5) & 1) << ' '
              << ((golden >> 4) & 1) << ' ' << ((golden >> 3) & 1) << ' '
              << ((golden >> 2) & 1) << ' ' << ((golden >> 1) & 1) << ' '
              << (golden & 1) << '\n';
}
template <class Output>
void masks(const Output &output, unsigned value, unsigned knownMask) {
  const auto &p = output.result.packed();
  require(p.knownMask() == gfsim::Bits<22>{knownMask});
  require(p.zMask() == gfsim::Bits<22>{0});
  require((p.value() & p.knownMask()) == gfsim::Bits<22>{value & knownMask});
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
    const auto row = self.rows[self.sampled];
    check(self.dut.sample(), expected(self.golden.position, row.emergency),
          true);
    self.golden.commit(row);
    ++self.sampled;
  }
};
struct Probe {
  gfsim::WorkExecutor pool;
  pyc_root root;
  Golden golden;
  explicit Probe(unsigned workers) : pool(workers), root("probe", &pool) {
    drive(root, inputs({}));
    root.Reset();
    root.Xfer();
  }
  void work(Row row) {
    drive(root, inputs(row));
    root.Work();
    check(root, expected(golden.position, row.emergency));
    root.Xfer();
    golden.commit(row);
  }
  void edges(unsigned count) {
    for (unsigned n = 0; n < count; ++n) {
      work({0});
      work({1});
    }
    work({0});
  }
};
void nativeDiscard(unsigned workers) {
  for (unsigned position : {19u, 23u, 35u}) {
    Probe probe(workers);
    probe.edges(position);
    const Row retry{1};
    drive(probe.root, inputs(retry));
    probe.root.Work();
    check(probe.root, expected(position, false));
    probe.root.DiscardNext();
    probe.root.Xfer();
    drive(probe.root, inputs({1, 1}));
    probe.root.Work();
    check(probe.root, expected(position, false));
    probe.root.DiscardNext();
    probe.root.Xfer();
    probe.work(retry);
    probe.edges(8);
  }
  for (bool reset : {false, true})
    for (bool z : {false, true}) {
      Probe probe(workers);
      probe.edges(23);
      auto bad = inputs({1});
      if (reset)
        bad.pyc_7079635f727374 = uncertain(z);
      else
        bad.pyc_7079635f636c6b = uncertain(z);
      drive(probe.root, bad);
      bool rejected = false;
      try {
        probe.root.Work();
      } catch (const gfsim::FourStateViolation &) {
        rejected = true;
      }
      require(rejected);
      probe.root.DiscardNext();
      probe.root.Xfer();
      probe.work({1});
      probe.edges(8);
    }
}
void executorFailure(unsigned workers, bool reset, bool z) {
  pyc_dut dut(workers);
  gfsim::SimExecutor executor(dut.system(), dut.observations(), {});
  constexpr std::string_view config =
      R"({"deadlock_window":null,"max_domain_cycles":{},"max_ticks":128,"schema":"pycircuit-model-config","version":"1"})";
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
    check(dut.sample(), expected(golden.position, row.emergency));
    golden.commit(row);
  };
  for (unsigned n = 0; n < 23; ++n) {
    step({0});
    step({1});
  }
  step({0});
  auto bad = inputs({1});
  if (reset)
    bad.pyc_7079635f727374 = uncertain(z);
  else
    bad.pyc_7079635f636c6b = uncertain(z);
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
  for (unsigned n = 0; n < 8; ++n) {
    step({0});
    step({1});
  }
  step({0});
}
void nativeFourState(unsigned workers) {
  for (bool z : {false, true})
    for (unsigned scenario = 0; scenario < 3; ++scenario) {
      Probe probe(workers);
      probe.edges(scenario == 2 ? 3 : 22);
      auto in = inputs({0});
      auto work = [&](unsigned clock, unsigned value, unsigned mask,
                      unsigned reset = 0) {
        in.pyc_7079635f636c6b = known<1>(clock);
        in.pyc_7079635f727374 = known<1>(reset);
        drive(probe.root, in);
        probe.root.Work();
        masks(probe.root, value, mask);
        probe.root.Xfer();
      };
      if (scenario == 0) {
        in.go = uncertain(z);
        in.emergency = known<1>(1);
        work(0, emergencyOutput, resultMask);
        for (unsigned n = 0; n < 4; ++n) {
          work(1, emergencyOutput, resultMask);
          work(0, emergencyOutput, resultMask);
        }
        in.go = known<1>(1);
        in.emergency = known<1>(0);
        work(0, expected(22, false), resultMask);
        // Every masked edge must hold the original prescaler and yellow blink.
        for (unsigned position = 22; position < 24; ++position) {
          work(1, expected(position, false), resultMask);
          work(0, expected(position + 1, false), resultMask);
        }
      } else if (scenario == 1) {
        in.go = known<1>(0);
        in.emergency = uncertain(z);
        const auto base = expected(22, false);
        const auto mask = (~(base ^ emergencyOutput)) & resultMask;
        work(0, base, mask);
        for (unsigned n = 0; n < 4; ++n) {
          work(1, base, mask);
          work(0, base, mask);
        }
        in.go = known<1>(1);
        in.emergency = known<1>(0);
        work(0, base, resultMask);
        for (unsigned position = 22; position < 24; ++position) {
          work(1, expected(position, false), resultMask);
          work(0, expected(position + 1, false), resultMask);
        }
      } else {
        in.go = uncertain(z);
        work(1, expected(3, false), resultMask);
        constexpr unsigned contaminatedKnown =
            (0x80u << 14) | (0x80u << 6) | 63u;
        work(0, 0x0c, contaminatedKnown);
        in.go = known<1>(0);
        for (unsigned n = 0; n < 3; ++n) {
          work(1, 0x0c, contaminatedKnown);
          work(0, 0x0c, contaminatedKnown);
        }
        in.emergency = known<1>(1);
        work(0, emergencyOutput, resultMask);
        in.emergency = known<1>(0);
        in.go = uncertain(z);
        work(1, 0x0c, contaminatedKnown, 1);
        in.go = known<1>(1);
        work(0, expected(0, false), resultMask);
        for (unsigned position = 0; position < 4; ++position) {
          work(1, expected(position, false), resultMask);
          work(0, expected(position + 1, false), resultMask);
        }
      }
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
  nativeDiscard(runner.workers());
  for (bool reset : {false, true})
    for (bool z : {false, true})
      executorFailure(runner.workers(), reset, z);
  nativeFourState(runner.workers());
  return result;
}
