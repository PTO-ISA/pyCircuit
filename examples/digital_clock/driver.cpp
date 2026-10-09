// Host calendar/edge oracle; every DUT epoch runs through the shared SystemRunner.
#include "gfsim/SimExecutor.h"
#include "gfsim/SystemRunner.h"
#include "pycircuit_system.hpp"
#include <array>
#include <cstdlib>
#include <iostream>
#include <source_location>
#include <stdexcept>
#include <string_view>

void require(bool ok, std::source_location at = std::source_location::current()) {
  if (!ok) {
    std::cerr << "DigitalClock independent oracle failed at " << at.line() << '\n';
    std::abort();
  }
}
template <unsigned W> auto known(unsigned value) {
  return gfsim::wire<gfsim::Bits<W>>::known(gfsim::Bits<W>{value});
}
auto uncertain(bool z) {
  return gfsim::wire<gfsim::Bits<1>>::fromPacked(
      z ? gfsim::FourState<1>::highImpedance() : gfsim::FourState<1>::unknown());
}
constexpr std::uint64_t period = 50000000;
#ifdef PYC_DIGITAL_CLOCK_PILOT_EDGES
constexpr std::uint64_t edgeLimit = PYC_DIGITAL_CLOCK_PILOT_EDGES;
constexpr bool pilot = true;
static_assert(edgeLimit >= 4 && edgeLimit < period);
#else
constexpr std::uint64_t edgeLimit = 2 * period;
constexpr bool pilot = false;
#endif
constexpr unsigned resultMask = (1u << 27) - 1;
struct Row { unsigned clock = 0, reset = 0, set = 0, plus = 0, minus = 0; };
struct Golden {
  unsigned time = 0, mode = 0, blink = 0;
  bool lastClock = false;
  std::uint64_t edges = 0, ticks = 0, midnight = 0, heldTick = 0;
  static unsigned bcd(unsigned value) { return (value / 10) * 16 + value % 10; }
  unsigned output() const {
    return (bcd(time / 3600) << 19) | (bcd((time / 60) % 60) << 11) |
           (bcd(time % 60) << 3) | (mode << 1) | blink;
  }
  void commit(Row row) {
    if (row.clock && !lastClock) {
      if (row.reset) {
        time = mode = blink = 0;
        edges = ticks = midnight = heldTick = 0;
      } else {
        ++edges;
        if (edges % period == 0) {
          ++ticks;
          blink ^= 1;
          if (mode == 0) {
            if (time == 86399) ++midnight;
            time = (time + 1) % 86400;
          } else ++heldTick;
        }
        if (mode && (row.minus || row.plus)) {
          const unsigned unit = mode == 1 ? 3600 : mode == 2 ? 60 : 1;
          const unsigned modulus = mode == 1 ? 24 : 60;
          const unsigned old = (time / unit) % modulus;
          const unsigned next = (old + (row.minus ? modulus - 1 : 1)) % modulus;
          time = time - old * unit + next * unit;
        }
        if (row.set) mode = (mode + 1) % 4;
      }
    }
    lastClock = row.clock;
  }
};
struct ShortStream {
  std::array<Row, 512> rows{};
  unsigned size = 0;
  void add(Row row) { require(size < rows.size()); rows[size++] = row; }
  void edges(unsigned count, unsigned set = 0, unsigned plus = 0, unsigned minus = 0) {
    for (unsigned n = 0; n < count; ++n) {
      add({0, 0, set, plus, minus}); add({1, 0, set, plus, minus});
    }
  }
  void reset() { add({0, 1, 1, 1, 1}); add({1, 1, 1, 1, 1}); add({0}); }
  ShortStream() {
    reset();
    edges(1, 1); // RUN -> HOUR.
    edges(1, 0, 0, 1); // 0 -> 23.
    edges(1, 0, 1); // 23 -> 0.
    edges(24, 0, 1); // Held plus, all hour values and wrap.
    edges(1, 0, 1, 1); // Minus wins.
    edges(1, 1, 1); // Old HOUR edits while mode becomes MINUTE.
    edges(1, 0, 0, 1); edges(1, 0, 1);
    edges(60, 0, 1); edges(1, 0, 1, 1);
    edges(1, 1, 0, 1); // Old MINUTE edit before SECOND.
    edges(1, 0, 0, 1); edges(1, 0, 1);
    edges(60, 0, 1); edges(1, 0, 1, 1);
    edges(1, 1, 1, 1); // Old SECOND minus and mode -> RUN.
    edges(2, 0, 1, 1); // Buttons irrelevant in RUN.
    // Held high/low and reset at low do not invent edges; rising reset wins buttons.
    add({1, 0, 1, 1, 1}); add({1, 1, 1, 1, 1}); add({0, 1, 1, 1, 1});
    add({0, 0, 1, 1, 1}); add({0, 1, 1, 1, 1}); add({1, 1, 1, 1, 1});
    add({1, 0, 1, 1, 1}); add({0});
    reset(); // Full-stream edge count starts here, not after its four setup edges.
  }
};
pyc_dut::Inputs inputs(Row row) {
  pyc_dut::Inputs in;
  in.pyc_7079635f636c6b = known<1>(row.clock);
  in.pyc_7079635f727374 = known<1>(row.reset);
  in.btn_set = known<1>(row.set); in.btn_plus = known<1>(row.plus);
  in.btn_minus = known<1>(row.minus);
  return in;
}
void drive(pyc_root &root, const pyc_dut::Inputs &in) {
  root.pyc_7079635f636c6b = in.pyc_7079635f636c6b;
  root.pyc_7079635f727374 = in.pyc_7079635f727374;
  root.btn_set = in.btn_set; root.btn_plus = in.btn_plus; root.btn_minus = in.btn_minus;
}
template <class Output> void check(const Output &output, unsigned expected,
                                  std::uint64_t epoch = 0, bool emit = false) {
  require(output.result.isFullyKnown());
  require(output.result.packed().value().value() == expected);
  if (emit)
    std::cout << "WORK " << epoch << ' ' << ((expected >> 19) & 255) << ' '
              << ((expected >> 11) & 255) << ' ' << ((expected >> 3) & 255) << ' '
              << ((expected >> 1) & 3) << ' ' << (expected & 1) << '\n';
}
template <class Output> void masks(const Output &output, unsigned value, unsigned mask) {
  const auto &p = output.result.packed();
  require(p.knownMask() == gfsim::Bits<27>{mask});
  require(p.zMask() == gfsim::Bits<27>{0});
  require((p.value() & p.knownMask()) == gfsim::Bits<27>{value & mask});
}
struct RunnerContext {
  pyc_dut &dut;
  const ShortStream shortRows;
  Golden golden;
  std::uint64_t sampled = 0;
  std::uint64_t size() const { return shortRows.size + 2 * edgeLimit + 1; }
  Row row(std::uint64_t epoch) const {
    if (epoch < shortRows.size) return shortRows.rows[epoch];
    const auto local = epoch - shortRows.size;
    const auto edge = local / 2 + 1;
    const bool set = edge <= 4 || (edge > period && edge <= period + 3) || edge == 2 * period;
    return {unsigned(local % 2), 0, unsigned(set), 0, unsigned(edge >= 2 && edge <= 4)};
  }
  static void initialize(void *opaque) { require(driveInputs(opaque, 0)); }
  static bool driveInputs(void *opaque, std::uint64_t epoch) {
    auto &self = *static_cast<RunnerContext *>(opaque);
    if (epoch == self.size()) return false;
    require(epoch < self.size()); self.dut.drive(inputs(self.row(epoch))); return true;
  }
  static void sample(void *opaque, std::uint64_t epoch) {
    auto &self = *static_cast<RunnerContext *>(opaque);
    require(epoch == self.sampled + 1 && self.sampled < self.size());
    const auto row = self.row(self.sampled);
    bool emit = self.sampled < self.shortRows.size;
    if (!emit) {
      const auto local = self.sampled - self.shortRows.size;
      const auto edge = local / 2 + 1;
      emit = local < 10 || local == 2 * edgeLimit ||
             edge % 1000000 == 0 || edge == period + 1 || edge == 2 * period + 1;
      if (row.clock && edge % 1000000 == 0) std::cerr << "progress edges " << edge << '\n';
    }
    check(self.dut.sample(), self.golden.output(), epoch, emit);
    self.golden.commit(row); ++self.sampled;
  }
  void finished() const {
    require(sampled == size() && golden.edges == edgeLimit);
    require(golden.ticks == (pilot ? 0 : 2));
    require(golden.midnight == (pilot ? 0 : 1));
    require(golden.heldTick == (pilot ? 0 : 1));
    require(golden.time == (pilot ? 86399 : 0));
    require(golden.mode == 0 && golden.blink == 0);
    std::cout << "WORK summary " << sampled << ' ' << golden.edges << ' ' << golden.ticks
              << ' ' << golden.midnight << ' ' << golden.heldTick << ' ' << pilot << '\n';
  }
};
struct Probe {
  gfsim::WorkExecutor pool;
  pyc_root root;
  Golden golden;
  explicit Probe(unsigned workers) : pool(workers), root("probe", &pool) {
    drive(root, inputs({})); root.Reset(); root.Xfer();
  }
  void work(Row row) {
    drive(root, inputs(row)); root.Work(); check(root, golden.output());
    root.Xfer(); golden.commit(row);
  }
  void edge(unsigned set = 0, unsigned plus = 0, unsigned minus = 0) {
    work({0, 0, set, plus, minus}); work({1, 0, set, plus, minus}); work({0});
  }
};
void lifecycle(unsigned workers) {
  Probe probe(workers); probe.edge(1);
  const Row retry{1, 0, 0, 1};
  for (bool reset : {false, true}) {
    drive(probe.root, inputs({1, unsigned(reset), 0, 1}));
    probe.root.Work(); check(probe.root, probe.golden.output());
    probe.root.DiscardNext(); probe.root.Xfer();
  }
  probe.work(retry); probe.work({0});
  require(probe.golden.time == 3600 && probe.golden.mode == 1);
  for (bool reset : {false, true}) for (bool z : {false, true}) {
    Probe atomic(workers); atomic.edge(1); atomic.edge(0, 1);
    auto badRoot = inputs({1, 0, 0, 1, 1});
    if (reset) badRoot.pyc_7079635f727374 = uncertain(z);
    else badRoot.pyc_7079635f636c6b = uncertain(z);
    drive(atomic.root, badRoot); bool rejected = false;
    try { atomic.root.Work(); } catch (const gfsim::FourStateViolation &) { rejected = true; }
    require(rejected); atomic.root.DiscardNext(); atomic.root.Xfer();
    // Retry the same rising edge with known inputs: old Q and edge history survived.
    atomic.work({1, 0, 0, 1, 1}); atomic.work({0});
    require(atomic.golden.time == 0 && atomic.golden.mode == 1);
    pyc_dut dut(workers); gfsim::SimExecutor executor(dut.system(), dut.observations(), {});
    constexpr std::string_view config = R"({"deadlock_window":null,"max_domain_cycles":{},"max_ticks":32,"schema":"pycircuit-model-config","version":"1"})";
    require(executor.ConfigureJson(reinterpret_cast<const std::uint8_t *>(config.data()), config.size()) == PYCIRCUIT_MODEL_STATUS_V1_OK);
    dut.drive(inputs({})); require(executor.Reset() == PYCIRCUIT_MODEL_STATUS_V1_OK);
    Golden golden;
    auto step = [&](Row row) {
      dut.drive(inputs(row)); PycircuitModelStepResultV1 result{sizeof(result)};
      require(executor.Step(&result) == PYCIRCUIT_MODEL_STATUS_V1_OK);
      check(dut.sample(), golden.output()); golden.commit(row);
    };
    step({0, 0, 1}); step({1, 0, 1}); step({0});
    auto bad = inputs({1, 0, 0, 1, 1});
    if (reset) bad.pyc_7079635f727374 = uncertain(z);
    else bad.pyc_7079635f636c6b = uncertain(z);
    dut.drive(bad); const auto epoch = executor.cycles();
    PycircuitModelStepResultV1 failed{sizeof(failed)};
    require(executor.Step(&failed) == PYCIRCUIT_MODEL_STATUS_V1_RUNTIME_FAILURE);
    require(failed.state == PYCIRCUIT_MODEL_STEP_V1_FAILED && failed.epoch_time == epoch);
    require(executor.cycles() == epoch && dut.system().cycle() == epoch);
    bool unavailable = false;
    try { (void)dut.sample(); } catch (const std::logic_error &) { unavailable = true; }
    require(unavailable && executor.Step(&failed) == PYCIRCUIT_MODEL_STATUS_V1_INVALID_STATE);
    dut.drive(inputs({})); require(executor.Reset() == PYCIRCUIT_MODEL_STATUS_V1_OK); golden = {};
    step({0}); step({1, 0, 1}); step({0}); step({1, 0, 0, 1}); step({0});
    require(golden.time == 3600);
  }
}
void fourState(unsigned workers) {
  for (bool z : {false, true}) for (unsigned scenario = 0; scenario < 5; ++scenario) {
    Probe probe(workers);
    if (scenario == 1 || scenario == 2) probe.edge(1);
    if (scenario == 4) for (unsigned n = 0; n < 3; ++n) probe.edge(1);
    auto ports = inputs({1});
    unsigned value = probe.golden.output(), mask = resultMask;
    if (scenario == 0) { ports.btn_plus = uncertain(z); ports.btn_minus = uncertain(z); }
    if (scenario == 1 || scenario == 2) {
      ports.btn_plus = uncertain(z);
      if (scenario == 1) mask &= ~(255u << 19);
      else { ports.btn_minus = known<1>(1); value = (0x23u << 19) | 2; }
    }
    if (scenario == 3) { ports.btn_set = uncertain(z); mask &= ~2u; }
    if (scenario == 4) { ports.btn_minus = uncertain(z); mask &= ~(255u << 3); }
    drive(probe.root, ports); probe.root.Work(); check(probe.root, probe.golden.output()); probe.root.Xfer();
    drive(probe.root, inputs({0})); probe.root.Work(); masks(probe.root, value, mask); probe.root.Xfer();
    // Rising reset has priority even when the data selections contain X/Z.
    drive(probe.root, inputs({1, 1, 1, 1, 1})); probe.root.Work(); masks(probe.root, value, mask); probe.root.Xfer();
    drive(probe.root, inputs({0})); probe.root.Work(); check(probe.root, 0); probe.root.Xfer();
    std::cerr << "FOUR DigitalClock scenario " << scenario << " selector " << (z ? 'z' : 'x') << '\n';
  }
}
int main(int argc, char **argv) {
  gfsim::SystemRunner runner(argc, argv); if (!runner.ready()) return 2;
  lifecycle(runner.workers()); fourState(runner.workers());
  pyc_dut dut(runner.workers()); RunnerContext context{dut};
  const gfsim::RunnerCallbacks callbacks{&context, &RunnerContext::initialize,
                                        &RunnerContext::driveInputs, &RunnerContext::sample};
  const int result = runner.Run(dut.system(), dut.observations(), {}, callbacks);
  require(result == 0); context.finished(); return result;
}
