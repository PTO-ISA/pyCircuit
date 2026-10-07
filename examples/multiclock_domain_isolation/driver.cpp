// Physical drive for the (b) submodule-composition + (c) Struct-grouping
// authoring form. Two CounterLane instances own one eight-bit DFF each; the
// two domain records d_a/d_b carry exactly two control pins per lane.
#include "gfsim/SimExecutor.h"
#include "gfsim/SystemRunner.h"
#include "pycircuit_system.hpp"
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <source_location>
#include <stdexcept>
#include <string_view>

void require(bool ok,
             std::source_location at = std::source_location::current()) {
  if (!ok) {
    std::cerr << "multiclock_domain_isolation oracle failed at " << at.line()
              << '\n';
    std::abort();
  }
}

using PinsWire = decltype(pyc_dut::Inputs::d_a);

// One domain record is an ordered {clk, rst} pair: field 0 occupies the most
// significant packed bit, so `field` and the packed bit index are complements.
PinsWire group(unsigned clk, unsigned rst, int unknown = -1,
               bool highZ = false) {
  using Packed = typename PinsWire::packed_type;
  std::uint64_t value = 0, known = 0, z = 0;
  for (unsigned field = 0; field != 2; ++field) {
    const unsigned bit = 1u - field;
    const bool missing = unknown == static_cast<int>(field);
    value |= std::uint64_t{field == 0 ? clk : rst} << bit;
    known |= std::uint64_t{missing ? 0u : 1u} << bit;
    z |= std::uint64_t{missing && highZ ? 1u : 0u} << bit;
  }
  return PinsWire::fromPacked(Packed::fromMasks(
      gfsim::Bits<2>{value}, gfsim::Bits<2>{known}, gfsim::Bits<2>{z}));
}

struct Row {
  unsigned ca, ra, cb, rb, a, b;
};
// Each row's (a, b) is the value Work samples during that epoch, which is the
// state committed by the PREVIOUS row's own rising edges: an edge commits only
// on its own lane's clock rise, and rst is sampled only at that own edge.
// Rows 0..2 establish known zero through each lane's own reset edge. Rows 3..6
// prove reset without an own edge is ignored and reset with one is taken.
// Rows 7..14 prove one lane counts while the other holds a different value.
// Rows 15..18 prove each lane's own reset leaves the other lane undisturbed.
// Rows 19..30 prove simultaneous edges keep two distinct per-lane states.
constexpr Row prefix[] = {
    {0, 1, 0, 1, 0, 0}, {1, 1, 1, 1, 0, 0}, {0, 0, 0, 0, 0, 0},
    {1, 0, 0, 0, 0, 0}, {1, 1, 0, 0, 1, 0}, {0, 1, 0, 0, 1, 0},
    {1, 1, 0, 0, 1, 0}, {0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 1, 0}, {1, 0, 0, 0, 1, 0}, {0, 0, 1, 0, 2, 0},
    {0, 0, 0, 0, 2, 1}, {0, 0, 1, 0, 2, 1}, {0, 0, 0, 0, 2, 2},
    {1, 1, 0, 0, 2, 2}, {0, 0, 0, 0, 0, 2}, {0, 0, 1, 1, 0, 2},
    {0, 0, 0, 0, 0, 0}, {1, 0, 1, 0, 0, 0}, {0, 0, 0, 0, 1, 1},
    {1, 0, 0, 0, 1, 1}, {0, 0, 0, 0, 2, 1}, {0, 0, 1, 0, 2, 1},
    {0, 0, 0, 0, 2, 2}, {1, 0, 1, 0, 2, 2}, {0, 0, 0, 0, 3, 3},
    {0, 0, 1, 0, 3, 3}, {0, 0, 0, 0, 3, 4}, {0, 0, 1, 0, 3, 4},
    {0, 0, 0, 0, 3, 5},
};
// Three sweeps of 512 frames give each lane 256 real rising edges per phase
// with rst held low, so each lane genuinely wraps 255 -> 0 without init, Reset
// or direct state assignment shortcutting the sweep. The three phases leave
// the two lanes at different offsets, so holding is never confused with a
// coincidental equal value.
constexpr unsigned sweepFrames = 3 * 512;
constexpr unsigned knownFrames = std::size(prefix) + sweepFrames;
Row knownRow(unsigned epoch) {
  require(epoch < knownFrames);
  if (epoch < std::size(prefix))
    return prefix[epoch];
  const unsigned frame = epoch - std::size(prefix);
  const unsigned phase = frame / 512, slot = frame % 512;
  const bool rising = slot % 2 == 0;
  // `step` counts this lane's own committed rising edges so far this phase:
  // 0 while its clock is still low on the first frame, then one per frame pair.
  const unsigned step = (slot + 1) / 2;
  return {phase == 1 ? 0u : unsigned(rising),
          0,
          phase == 0 ? 0u : unsigned(rising),
          0,
          phase == 1 ? 3u : (3u + step) & 255u,
          phase == 0 ? 5u : (5u + step) & 255u};
}
void drive(pyc_dut &dut, const Row &row) {
  pyc_dut::Inputs in;
  in.d_a = group(row.ca, row.ra);
  in.d_b = group(row.cb, row.rb);
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
void unassertedLevelSmoke(unsigned workers) {
  // This example is new, so it has no historical testbench to replay. This
  // smoke preserves only the driving shape of the frozen multiclock_regs
  // smoke: identical levels on both clocks, lane A reset for two epochs, lane
  // B reset deasserted, and no output expectation. It is not a replay of any
  // historical artifact and claims no historical provenance.
  pyc_dut dut(workers);
  gfsim::SimExecutor executor(dut.system(), dut.observations(), {});
  configure(executor);
  ::drive(dut, {0, 1, 0, 0, 0, 0});
  require(executor.Reset() == PYCIRCUIT_MODEL_STATUS_V1_OK);
  for (unsigned cycle = 0; cycle < 6; ++cycle) {
    const unsigned reset = cycle < 2 ? 1u : 0u;
    ::drive(dut, {1, reset, 1, 0, 0, 0});
    step(executor);
    ::drive(dut, {0, reset, 0, 0, 0, 0});
    step(executor);
  }
  require(executor.cycles() == 12);
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
      // Exactly one of the four physical control pins is X (z == false) or Z.
      in.d_a = pin < 2 ? group(1, 0, static_cast<int>(pin), z) : group(1, 0);
      in.d_b =
          pin < 2 ? group(1, 0) : group(1, 0, static_cast<int>(pin) - 2, z);
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
  // lane still commits, so each reset stays independent of the other clock.
  for (bool z : {false, true})
    for (bool a : {false, true}) {
      pyc_dut dut(workers);
      gfsim::SimExecutor executor(dut.system(), dut.observations(), {});
      configure(executor);
      ::drive(dut, {0, 0, 0, 0, 0, 0});
      require(executor.Reset() == PYCIRCUIT_MODEL_STATUS_V1_OK);
      pyc_dut::Inputs in;
      in.d_a = group(a ? 0 : 1, 0, a ? 1 : -1, z);
      in.d_b = group(a ? 1 : 0, 0, a ? -1 : 1, z);
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
  unassertedLevelSmoke(runner.workers());
  nativeUnknownContract(runner.workers());
  return result;
}
