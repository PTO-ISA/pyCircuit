#include "gfsim/SimExecutor.h"
#include "gfsim/SystemRunner.h"
#include "pycircuit_system.hpp"

// Decode the typed result while retaining value, known and Z planes.
struct ObservedOutputs {
  gfsim::wire<gfsim::Bits<8>> y, q;
};
ObservedOutputs observe(const pyc_dut::Outputs &output) {
  return {
      gfsim::wire<gfsim::Bits<8>>::fromPacked(
          gfsim::extract<8>(output.result.packed(), 8)),
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
    std::cerr << "obs_points oracle failed at " << at.line() << '\n';
    std::abort();
  }
}
template <unsigned W> auto known(unsigned value) {
  return gfsim::wire<gfsim::Bits<W>>::known(gfsim::Bits<W>{value});
}
struct Row {
  unsigned clock, reset, x, y, q;
};
// Two complete reset cycles. Original x10 pre11/0 then same-input next Work
// post11; x20 pre21/11 then next Work post21, with no extra rising edge.
constexpr Row prefix[] = {
    {0, 1, 0, 1, 0},    {1, 1, 0, 1, 0},    {0, 1, 0, 1, 0},
    {1, 1, 0, 1, 0},    {0, 0, 10, 11, 0},  {1, 0, 10, 11, 0},
    {0, 0, 10, 11, 11}, {1, 0, 20, 21, 11}, {0, 0, 20, 21, 21},
};
constexpr Row tail[] = {
    {0, 0, 100, 101, 0},  {0, 0, 200, 201, 0}, {1, 0, 17, 18, 0},
    {1, 0, 250, 251, 18}, {1, 0, 0, 1, 18},    {0, 0, 252, 253, 18},
    {0, 0, 255, 0, 18},   {1, 1, 255, 0, 18},  {0, 1, 255, 0, 0},
    {1, 0, 7, 8, 0},      {0, 0, 1, 2, 8},
};
constexpr unsigned sweepFrames = 512;
constexpr unsigned knownFrames =
    std::size(prefix) + sweepFrames + std::size(tail);
Row knownRow(unsigned epoch) {
  require(epoch < knownFrames);
  if (epoch < std::size(prefix))
    return prefix[epoch];
  epoch -= std::size(prefix);
  if (epoch < sweepFrames) {
    const unsigned value = epoch / 2, immediate = (value + 1) & 255;
    // Golden current-Q comes from the previous byte's completed edge; y is
    // already the new byte plus1, including 255->0, throughout this Work.
    const unsigned before = value == 0 ? 21 : value;
    return {epoch % 2 == 0 ? 1u : 0u, 0, value, immediate,
            epoch % 2 == 0 ? before : immediate};
  }
  return tail[epoch - sweepFrames];
}
struct RunnerContext {
  pyc_dut &dut;
  unsigned sampled = 0;
  static void initialize(void *opaque) { require(drive(opaque, 0)); }
  static bool drive(void *opaque, std::uint64_t epoch) {
    auto &self = *static_cast<RunnerContext *>(opaque);
    if (epoch == knownFrames)
      return false;
    const auto row = knownRow(epoch);
    pyc_dut::Inputs in;
    in.pyc_7079635f636c6b = known<1>(row.clock);
    in.pyc_7079635f727374 = known<1>(row.reset);
    in.x = known<8>(row.x);
    self.dut.drive(in);
    return true;
  }
  static void sample(void *opaque, std::uint64_t epoch) {
    auto &self = *static_cast<RunnerContext *>(opaque);
    require(epoch == self.sampled + 1 && self.sampled < knownFrames);
    const auto row = knownRow(self.sampled);
    const auto out = observe(self.dut.sample());
    require(out.y.isFullyKnown() && out.y.value().value() == row.y);
    require(out.q.isFullyKnown() && out.q.value().value() == row.q);
    std::cout << "WORK " << out.y.value().value() << ' '
              << out.q.value().value() << '\n';
    ++self.sampled;
  }
};
void nativeFourState(unsigned workers) {
  pyc_dut dut(workers);
  gfsim::SimExecutor executor(dut.system(), dut.observations(), {});
  constexpr std::string_view config = "{}";
  require(executor.ConfigureJson(
              reinterpret_cast<const std::uint8_t *>(config.data()),
              config.size()) == PYCIRCUIT_MODEL_STATUS_V1_OK);
  pyc_dut::Inputs in;
  in.pyc_7079635f636c6b = known<1>(0);
  in.pyc_7079635f727374 = known<1>(0);
  in.x = known<8>(1);
  dut.drive(in);
  require(executor.Reset() == PYCIRCUIT_MODEL_STATUS_V1_OK);
  auto step = [&](unsigned clock, unsigned y, unsigned yKnown, unsigned q,
                  unsigned qKnown) {
    in.pyc_7079635f636c6b = known<1>(clock);
    dut.drive(in);
    PycircuitModelStepResultV1 result{sizeof(result)};
    require(executor.Step(&result) == PYCIRCUIT_MODEL_STATUS_V1_OK &&
            result.state == PYCIRCUIT_MODEL_STEP_V1_RUNNING);
    auto check = [&](const auto &actual, unsigned value, unsigned mask) {
      const auto &bits = actual.packed();
      require(bits.knownMask() == gfsim::Bits<8>{mask});
      require(bits.zMask() == gfsim::Bits<8>{0});
      require((bits.value() & bits.knownMask()) == gfsim::Bits<8>{value});
    };
    const auto out = observe(dut.sample());
    check(out.y, y, yKnown);
    check(out.q, q, qKnown);
  };
  step(0, 2, 255, 0, 255);
  step(1, 2, 255, 0, 255);
  step(0, 2, 255, 2, 255);
  struct Masks {
    unsigned value, known, z;
  };
  constexpr Masks cases[] = {
      {0, 254, 0}, {0, 127, 128}, {0, 0, 0}, {0, 0, 255}};
  for (const auto &row : cases) {
    const auto unknown =
        gfsim::wire<gfsim::Bits<8>>::fromPacked(gfsim::FourState<8>::fromMasks(
            gfsim::Bits<8>{row.value}, gfsim::Bits<8>{row.known},
            gfsim::Bits<8>{row.z}));
    in.x = unknown;
    step(0, 0, 0, 2, 255);
    step(1, 0, 0, 2, 255);
    step(1, 0, 0, 0, 0);
    in.x = known<8>(2);
    step(1, 3, 255, 0, 0);
    step(0, 3, 255, 0, 0);
    step(1, 3, 255, 0, 0);
    step(0, 3, 255, 3, 255);
    // Reset priority recovers q on the edge, while combinational y remains X.
    in.x = unknown;
    in.pyc_7079635f727374 = known<1>(1);
    step(1, 0, 0, 3, 255);
    step(0, 0, 0, 0, 255);
    in.x = known<8>(1);
    in.pyc_7079635f727374 = known<1>(0);
    step(1, 2, 255, 0, 255);
    step(0, 2, 255, 2, 255);
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
  nativeFourState(runner.workers());
  return result;
}
