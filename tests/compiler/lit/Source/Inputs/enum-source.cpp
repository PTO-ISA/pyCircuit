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
  if (!ok) { std::cerr << "enum source oracle failed at " << at.line() << '\n'; std::abort(); }
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
template<class Wire,unsigned W> void planes(const Wire &output,unsigned offset,const gfsim::wire<gfsim::Bits<W>> &expected,unsigned sourceOffset=0,unsigned count=W) {
  const auto &actual=output.packed();const auto &gold=expected.packed();
  require(sourceOffset+count<=W);
  for(unsigned bit=0;bit<count;++bit){
    require(actual.value().bit(offset+bit)==gold.value().bit(sourceOffset+bit));
    require(actual.knownMask().bit(offset+bit)==gold.knownMask().bit(sourceOffset+bit));
    require(actual.zMask().bit(offset+bit)==gold.zMask().bit(sourceOffset+bit));
  }
}
template<unsigned W>
auto latentInput(std::string_view text, unsigned pattern) {
  require(text.size() == W);
  gfsim::Bits<W> value{0}, known{0}, z{0};
  for (unsigned bit = 0; bit < W; ++bit) {
    const char c = text[W - 1 - bit];
    const auto mask = std::uint64_t{1} << (bit % 64);
    const auto lane = bit / 64;
    // All 4 latent masks for two-bit words, repeated across wide word boundaries.
    const bool latent = (pattern >> (bit % 2)) & 1;
    if (c == '1' || ((c == 'x' || c == 'z') && latent))
      value.setWord(lane, value.word(lane) | mask);
    if (c == '0' || c == '1') known.setWord(lane, known.word(lane) | mask);
    if (c == 'z') z.setWord(lane, z.word(lane) | mask);
  }
  return gfsim::wire<gfsim::Bits<W>>::fromPacked(gfsim::FourState<W>::fromMasks(value, known, z));
}


#include "enum-source-width.hpp"
template <class Output> void check(const Output &output, std::string_view expected) {
  const auto &p = output.result.packed(); std::string actual;
  for (unsigned bit = result_width; bit; --bit)
    actual += p.zMask().bit(bit - 1) ? 'z' : !p.knownMask().bit(bit - 1) ? 'x' : p.value().bit(bit - 1) ? '1' : '0';
  if (actual != expected) { std::cerr << "got " << actual << " expected " << expected << '\n'; require(false); }
}
#include "enum-source-vectors.hpp"
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
    const auto output=self.dut.sample();
    check(output, expected[self.sampled]);checkPlanes(output,self.sampled);
    std::cout << "WORK " << self.sampled << ' ' << expected[self.sampled] << '\n'; ++self.sampled;
  }
};
#ifdef ENUM_SOURCE_STATE
void lifecycle(unsigned workers) {
  gfsim::WorkExecutor pool(workers); pyc_root root("probe", &pool);
  pyc_dut::Inputs initial; driveProbe(initial, 0); driveRoot(root, initial);
  root.Work();require(!root.result.isFullyKnown());root.DiscardNext();root.Xfer();
  root.Reset();require(!root.result.isFullyKnown());root.Xfer();
  for (unsigned row = 0; row < probe_count; ++row) {
    pyc_dut::Inputs ports; driveProbe(ports, row); driveRoot(root, ports);
    bool failed = false;
    try { root.Work(); } catch (const gfsim::FourStateViolation &) { failed = true; }
    if(failed != (probe_action[row] == 2))std::cerr<<"probe failure classification row "<<row<<" observed "<<failed<<" expected "<<(probe_action[row]==2)<<'\n';
    require(failed == (probe_action[row] == 2));
    if (!failed) {check(root, probe_expected[row]);checkProbePlanes(root,row);}
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
#ifdef ENUM_SOURCE_STATE
  lifecycle(runner.workers());
#endif
#ifdef ENUM_SOURCE_LATENT
  replayLatentTuples(runner.workers());
#endif
  return status;
}
