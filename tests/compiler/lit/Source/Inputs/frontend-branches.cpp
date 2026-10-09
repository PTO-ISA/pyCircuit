// Expected vectors come from independent host arithmetic and symbol truth tables.
#include "gfsim/SimExecutor.h"
#include "gfsim/SystemRunner.h"
#include "pycircuit_system.hpp"
#include <cstdlib>
#include <iostream>
#include <source_location>
#include <stdexcept>
#include <string>
#include <string_view>
void require(bool ok, std::source_location at = std::source_location::current()) {
  if (!ok) { std::cerr << "frontend branch oracle failed at " << at.line() << '\n'; std::abort(); }
}
template <unsigned W> auto input(std::string_view text) {
  require(text.size() == W);
  gfsim::Bits<W> value{0}, known{0}, z{0};
  for (unsigned bit = 0; bit < W; ++bit) {
    const char symbol = text[W - bit - 1];
    const auto one = std::uint64_t{1} << (bit % 64); const auto word = bit / 64;
    if (symbol == '1' || ((symbol == 'x' || symbol == 'z') && bit % 3 == 0)) value.setWord(word, value.word(word) | one);
    if (symbol == '0' || symbol == '1') known.setWord(word, known.word(word) | one);
    if (symbol == 'z') z.setWord(word, z.word(word) | one);
  }
  return gfsim::wire<gfsim::Bits<W>>::fromPacked(gfsim::FourState<W>::fromMasks(value, known, z));
}
#include "frontend-branches-width.hpp"
template <class Output> void check(const Output &output, std::string_view expected) {
  const auto &p = output.result.packed(); std::string actual;
  for (unsigned bit = result_width; bit; --bit)
    actual += p.zMask().bit(bit - 1) ? 'z' : !p.knownMask().bit(bit - 1) ? 'x' : p.value().bit(bit - 1) ? '1' : '0';
  if (actual != expected) { std::cerr << "got " << actual << " expected " << expected << '\n'; require(false); }
}
#include "frontend-branches-vectors.hpp"
struct Context {
  pyc_dut &dut;
  unsigned sampled = 0;
  static void initialize(void *opaque) { require(driveInputs(opaque, 0)); }
  static bool driveInputs(void *opaque, std::uint64_t epoch) {
    auto &self = *static_cast<Context *>(opaque);
    if (epoch == row_count) return false;
    require(epoch < row_count); pyc_dut::Inputs ports; drive(ports, epoch); self.dut.drive(ports); return true;
  }
  static void sample(void *opaque, std::uint64_t epoch) {
    auto &self = *static_cast<Context *>(opaque);
    require(epoch == self.sampled + 1 && self.sampled < row_count);
    check(self.dut.sample(), expected[self.sampled]);
    std::cout << "WORK " << self.sampled << ' ' << expected[self.sampled] << '\n'; ++self.sampled;
  }
};
#ifdef STATE_BRANCHES
void lifecycle(unsigned workers) {
  gfsim::WorkExecutor pool(workers); pyc_root root("probe", &pool);
  pyc_dut::Inputs initial; driveProbe(initial, 0); driveRoot(root, initial); root.Reset(); root.Xfer();
  for (unsigned row = 0; row < probe_count; ++row) {
    pyc_dut::Inputs ports; driveProbe(ports, row); driveRoot(root, ports);
    bool failed = false;
    try { root.Work(); } catch (const gfsim::FourStateViolation &) { failed = true; }
    require(failed == (probe_action[row] == 2));
    if (!failed) check(root, probe_expected[row]);
    if (probe_action[row]) root.DiscardNext();
    root.Xfer();
  }
  for (unsigned row = 0; row < failure_count; ++row) {
    pyc_dut dut(workers); gfsim::SimExecutor executor(dut.system(), dut.observations(), {});
    constexpr std::string_view config = R"({"deadlock_window":null,"max_domain_cycles":{},"max_ticks":32,"schema":"pycircuit-model-config","version":"1"})";
    require(executor.ConfigureJson(reinterpret_cast<const std::uint8_t *>(config.data()), config.size()) == PYCIRCUIT_MODEL_STATUS_V1_OK);
    driveProbe(initial, 0); dut.drive(initial); require(executor.Reset() == PYCIRCUIT_MODEL_STATUS_V1_OK);
    pyc_dut::Inputs ports; driveFailure(ports, row); dut.drive(ports);
    const auto epoch = executor.cycles(); PycircuitModelStepResultV1 failed{sizeof(failed)};
    require(executor.Step(&failed) == PYCIRCUIT_MODEL_STATUS_V1_RUNTIME_FAILURE);
    require(failed.state == PYCIRCUIT_MODEL_STEP_V1_FAILED && failed.epoch_time == epoch);
    require(executor.cycles() == epoch && dut.system().cycle() == epoch);
    bool unavailable = false;
    try { (void)dut.sample(); } catch (const std::logic_error &) { unavailable = true; }
    require(unavailable && executor.Step(&failed) == PYCIRCUIT_MODEL_STATUS_V1_INVALID_STATE);
    driveProbe(initial, 0); dut.drive(initial); require(executor.Reset() == PYCIRCUIT_MODEL_STATUS_V1_OK);
    PycircuitModelStepResultV1 recovered{sizeof(recovered)};
    require(executor.Step(&recovered) == PYCIRCUIT_MODEL_STATUS_V1_OK);
    check(dut.sample(), probe_expected[0]);
  }
}
#endif
int main(int argc, char **argv) {
  gfsim::SystemRunner runner(argc, argv); if (!runner.ready()) return 2;
  pyc_dut dut(runner.workers()); Context context{dut};
  const gfsim::RunnerCallbacks callbacks{&context, &Context::initialize, &Context::driveInputs, &Context::sample};
  const int status = runner.Run(dut.system(), dut.observations(), {}, callbacks);
  require(status == 0 && context.sampled == row_count);
#ifdef STATE_BRANCHES
  lifecycle(runner.workers());
#endif
  return status;
}
