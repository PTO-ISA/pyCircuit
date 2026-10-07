#include "gfsim/SystemRunner.h"
#include "pycircuit_system.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <source_location>
#include <stdexcept>
#include <vector>

void require(bool ok,
             std::source_location at = std::source_location::current()) {
  if (!ok) {
    std::cerr << "bf16_fmac oracle at " << at.line() << '\n';
    std::abort();
  }
}
template <unsigned W> auto known(std::uint64_t value) {
  return gfsim::wire<gfsim::Bits<W>>::known(gfsim::Bits<W>{value});
}
struct Operands {
  std::uint16_t a, b;
  std::uint32_t acc;
};
std::uint32_t evaluate(const Operands &in) {
  const unsigned ae = (in.a >> 7) & 255, be = (in.b >> 7) & 255;
  const unsigned ce = (in.acc >> 23) & 255;
  if (!ae || !be)
    return ce ? in.acc : 0;
  std::uint32_t product = (128 + (in.a & 127)) * (128 + (in.b & 127));
  unsigned exponent = (ae + be + 1024 - 127) & 1023;
  if (product & 32768) {
    product >>= 1;
    exponent = (exponent + 1) & 1023;
  }
  exponent &= 255;
  std::uint32_t pm = product << 9;
  std::uint32_t cm = ce ? ((in.acc & 0x7fffff) | 0x800000) : 0;
  const unsigned distance = exponent > ce ? exponent - ce : ce - exponent;
  const unsigned shift = std::min(distance, 26u);
  if (exponent > ce)
    cm >>= shift;
  else
    pm >>= shift;
  const bool ps = ((in.a ^ in.b) & 0x8000) != 0, cs = (in.acc >> 31) != 0;
  bool sign = ps;
  std::uint32_t magnitude;
  if (ps == cs)
    magnitude = (pm + cm) & 0x3ffffff;
  else if (pm >= cm)
    magnitude = pm - cm;
  else {
    magnitude = cm - pm;
    sign = cs;
  }
  if (!magnitude)
    return 0;
  // Highest-set-bit normalization is independent of the DUT priority LZC.
  const int displacement = int(std::bit_width(magnitude)) - 24;
  const std::uint32_t normalized =
      displacement > 0 ? magnitude >> displacement : magnitude << -displacement;
  const unsigned packedExp =
      (std::max(exponent, ce) + 1024 + displacement) & 255;
  return (std::uint32_t(sign) << 31) | (packedExp << 23) |
         (normalized & 0x7fffff);
}
std::vector<Operands> corpus() {
  std::vector<Operands> values{{0, 0, 0},
                               {0, 0x3f80, 0x3f812345},
                               {0x8000, 0x3f80, 0xc0000000},
                               {0x007f, 0x3fff, 0xbf800000},
                               {0x3f80, 0x3f80, 0},
                               {0x3f80, 0x3f80, 0x3f800000},
                               {0xbf80, 0x3f80, 0},
                               {0x3f80, 0xbf80, 0x3f800000},
                               {0x3f80, 0x3f80, 0xc0000000},
                               {0xbf80, 0xbf80, 0xbf800000},
                               {0x3fc0, 0x3fc0, 0},
                               {0x3fff, 0x3fff, 0},
                               {0x3f81, 0x3f81, 0},
                               {0x3f80, 0x3f80, 0x4c000000},
                               {0x3f80, 0x3f80, 0x4c800000},
                               {0x3f80, 0x3f80, 0x4d000000},
                               {0x0080, 0x0080, 0},
                               {0x7f00, 0x7f00, 0},
                               {0x3f80, 0x3f80, 0x007fffff},
                               {0x3f80, 0x3f80, 0x80000000}};
  for (unsigned i = 0; i < 32; ++i)
    values.push_back({std::uint16_t(((i & 1) << 15) | ((90 + i * 7 % 71) << 7) |
                                    (i * 19 % 128)),
                      std::uint16_t((((i >> 1) & 1) << 15) |
                                    ((90 + i * 11 % 71) << 7) | (i * 37 % 128)),
                      ((i >> 2) & 1) << 31 | ((90 + i * 13 % 71) << 23) |
                          ((i * 2654435761u) & 0x7fffff)});
  return values;
}
void anchors() {
  require(evaluate({0x3f80, 0x3f80, 0}) == 0x3f800000);
  require(evaluate({0x3f80, 0x3f80, 0x3f800000}) == 0x40000000);
  require(evaluate({0xbf80, 0x3f80, 0}) == 0xbf800000);
  require(evaluate({0x3f80, 0xbf80, 0x3f800000}) == 0);
  require(evaluate({0x3f80, 0x3f80, 0xc0000000}) == 0xbf800000);
  require(evaluate({0x3fc0, 0x3fc0, 0}) == 0x40100000);
  require(evaluate({0x3fff, 0x3fff, 0}) == 0x407e0000);
  require(evaluate({0x3f81, 0x3f81, 0}) == 0x3f820200);
  require(evaluate({0x0080, 0x0080, 0}) == 0x41800000);
  require(evaluate({0x7f00, 0x7f00, 0}) == 0x3e800000);
}
struct Row {
  unsigned clock, reset, valid;
  Operands in;
};
std::vector<Row> stimulus() {
  std::vector<Row> rows{{0, 1, 0, {0, 0, 0}}};
  const auto values = corpus();
  auto edge = [&](Operands in, unsigned valid = 1, unsigned reset = 0,
                  bool held = false) {
    rows.push_back({1, reset, valid, in});
    if (held) {
      rows.push_back({1, 1, 1, values[6]});
      rows.push_back({1, 0, 0, values[7]});
    }
    rows.push_back({0, reset, valid, in});
    if (held) {
      rows.push_back({0, 1, 1, values[8]});
      rows.push_back({0, 0, 0, values[9]});
    }
  };
  edge(values[0], 0, 1);
  edge(values[4]);
  for (unsigned i = 0; i < 5; ++i)
    edge(values[6], 0);
  for (unsigned i = 0; i < 3; ++i)
    edge(values[i]);
  edge(values[5], 1,
       1); // Drop all three in-flight tokens; reset wins input valid.
  for (unsigned i = 0; i < values.size(); ++i) {
    edge(values[i], 1, 0, i == 5);
    if (i % 4 == 3)
      edge(values[(i + 7) % values.size()], 0);
  }
  for (unsigned i = 0; i < 6; ++i)
    edge(values[0], 0);
  return rows;
}
struct Context {
  pyc_dut &dut;
  std::vector<Row> rows = stimulus();
  std::array<std::optional<std::uint32_t>, 3> stages;
  std::uint32_t result = 0;
  bool outputValid = false, last = false;
  unsigned sampled = 0, accepted = 0, retired = 0, dropped = 0, peak = 0,
           holes = 0, holds = 0, resetFull = 0, repeated = 0;
  static void initialize(void *p) { require(drive(p, 0)); }
  static bool drive(void *p, std::uint64_t epoch) {
    auto &c = *static_cast<Context *>(p);
    if (epoch == c.rows.size())
      return false;
    require(epoch < c.rows.size());
    const auto &r = c.rows[epoch];
    pyc_dut::Inputs in;
    in.pyc_7079635f636c6b = known<1>(r.clock);
    in.pyc_7079635f727374 = known<1>(r.reset);
    in.a_in = known<16>(r.in.a);
    in.b_in = known<16>(r.in.b);
    in.acc_in = known<32>(r.in.acc);
    in.valid_in = known<1>(r.valid);
    c.dut.drive(in);
    return true;
  }
  static void sample(void *p, std::uint64_t epoch) {
    auto &c = *static_cast<Context *>(p);
    require(epoch == c.sampled + 1);
    c.observe();
  }
  void observe() {
    const auto actual = dut.sample().result.packed();
    require(actual.isFullyKnown());
    require(actual.value().value() ==
            (std::uint64_t(result) << 1 | outputValid));
    std::cout << "WORK " << sampled << ' ' << result << ' ' << outputValid
              << '\n';
    const auto &r = rows[sampled];
    repeated += r.clock == last;
    if (r.clock && !last) {
      unsigned occupancy = 0;
      for (const auto &stage : stages)
        occupancy += stage.has_value();
      if (r.reset) {
        resetFull += occupancy == 3;
        dropped += occupancy;
        stages = {};
        result = 0;
        outputValid = false;
      } else {
        outputValid = stages[2].has_value();
        if (outputValid) {
          result = *stages[2];
          ++retired;
        } else {
          ++holes;
          holds += result != 0;
        }
        stages[2] = stages[1];
        stages[1] = stages[0];
        stages[0] = r.valid ? std::optional{evaluate(r.in)} : std::nullopt;
        accepted += r.valid;
        occupancy = 0;
        for (const auto &stage : stages)
          occupancy += stage.has_value();
        peak = std::max(peak, occupancy);
      }
    }
    last = r.clock;
    ++sampled;
  }
  void finish() const {
    require(sampled == rows.size() && sampled < 220 && peak == 3 &&
            resetFull == 1 && dropped == 3 && repeated >= 4 && holes > 8 &&
            holds > 4);
    for (const auto &stage : stages)
      require(!stage);
    require(!outputValid && accepted == retired + dropped && accepted == 56);
    std::cout << "HISTORY " << accepted << ' ' << retired << ' ' << dropped
              << '\n';
  }
};
// Four-state probes share this executable and the runner's worker setting.
// Defined values and complete known/Z masks retain the frozen comparison oracle;
// latent computed-X payload bits are deliberately masked in FRAME output.
namespace four_state_checks {
template <unsigned W>
auto partial(std::uint64_t value, std::uint64_t mask, bool z, bool latent) {
  using Bits = gfsim::Bits<W>;
  value = (value & ~mask) | (latent ? mask : 0);
  return gfsim::wire<Bits>::fromPacked(gfsim::FourState<W>::fromMasks(
      Bits{value}, ~Bits{mask}, Bits{z ? mask : 0}));
}

using Packed = gfsim::FourState<33>;
struct Harness {
  pyc_dut dut;
  pyc_dut::Inputs inputs;
  unsigned id, frame = 0;

  explicit Harness(unsigned id, unsigned workers) : dut(workers), id(id) {
    inputs.a_in = known<16>(0x3f80);
    inputs.b_in = known<16>(0x3f80);
    inputs.acc_in = known<32>(0);
    inputs.valid_in = known<1>(0);
    inputs.pyc_7079635f727374 = known<1>(0);
    inputs.pyc_7079635f636c6b = known<1>(0);
    dut.drive(inputs);
    dut.system().Build();
    require(dut.system().state() == gfsim::SimSystemState::Built);
    dut.system().Reset();
    require(dut.system().state() == gfsim::SimSystemState::Ready);
  }

  Packed step(bool clock) {
    inputs.pyc_7079635f636c6b = known<1>(clock);
    dut.drive(inputs);
    require(dut.system().Step() == gfsim::SimStepResult::Running);
    const auto value = dut.sample().result.packed();
    require(value.invariantHolds());
    std::cout << "FRAME " << id << ' ' << frame++ << ' '
              << (value.value() & value.knownMask()).value() << ' '
              << value.knownMask().value()
              << ' ' << value.zMask().value() << '\n';
    return value;
  }

  static void expect(Packed value, std::uint32_t data, bool valid) {
    require(value.isFullyKnown());
    require(value.value().value() == ((std::uint64_t(data) << 1) | valid));
  }

  Packed edge() {
    step(true);
    return step(false);
  }
};

struct Anchor {
  std::uint16_t a, b;
  std::uint32_t acc, result;
};
constexpr std::array<Anchor, 4> anchors{{
    {0x3f80, 0x3f80, 0, 0x3f800000},       // 1 * 1
    {0xbf80, 0x3f80, 0, 0xbf800000},       // -1 * 1
    {0x3f80, 0xbf80, 0x3f800000, 0},      // cancellation
    {0x3fc0, 0x3fc0, 0, 0x40100000},      // 1.5 * 1.5
}};

void operandCase(unsigned id, unsigned workers, const Anchor &anchor,
                 unsigned pattern = 0, bool z = false, bool latent = false) {
  Harness h(id, workers);
  h.inputs.a_in = known<16>(anchor.a);
  h.inputs.b_in = known<16>(anchor.b);
  h.inputs.acc_in = known<32>(anchor.acc);
  // Masks select multiplier fraction, exponent alignment, sign selection,
  // and cancellation/normalization inputs. These are whole-DUT probes;
  // they do not claim independently observed internal helper coverage.
  switch (pattern) {
  case 1: h.inputs.a_in = partial<16>(anchor.a, 0x0005, z, latent); break;
  case 2: h.inputs.b_in = partial<16>(anchor.b, 0x0060, z, latent); break;
  case 3: h.inputs.a_in = partial<16>(anchor.a, 0x0180, z, latent); break;
  case 4: h.inputs.acc_in = partial<32>(anchor.acc, 0x01800000, z, latent); break;
  case 5: h.inputs.a_in = partial<16>(anchor.a, 0x8000, z, latent); break;
  case 6: h.inputs.acc_in = partial<32>(anchor.acc, 0x00400101, z, latent); break;
  default: require(pattern == 0); break;
  }
  Harness::expect(h.step(false), 0, false);
  h.inputs.valid_in = known<1>(1);
  Harness::expect(h.edge(), 0, false);
  h.inputs.valid_in = known<1>(0);
  Harness::expect(h.edge(), 0, false);
  Harness::expect(h.step(true), 0, false);
  // Repeated high levels must not add a pipeline edge.
  Harness::expect(h.step(true), 0, false);
  Harness::expect(h.step(false), 0, false);
  Harness::expect(h.step(true), 0, false); // Work sees output's old Q.
  const auto result = h.step(false);
  require((result.knownMask().value() & 1) == 1);
  require((result.value().value() & 1) == 1);
  if (pattern == 0)
    Harness::expect(result, anchor.result, true);
  const auto held = h.edge();
  require(held.value().value() == (result.value().value() & ~std::uint64_t{1}));
  require(held.knownMask() == result.knownMask());
  require(held.zMask() == result.zMask());
  // A real reset edge clears stage/output state even after partial-X/Z data.
  h.inputs.pyc_7079635f727374 = known<1>(1);
  Harness::expect(h.edge(), 0, false);
}

void invalidEnableCase(unsigned id, unsigned workers, bool z, bool latent) {
  Harness h(id, workers);
  Harness::expect(h.step(false), 0, false);
  h.inputs.valid_in = known<1>(1);
  h.edge();
  h.inputs.valid_in = known<1>(0);
  h.edge();
  h.edge();
  Harness::expect(h.edge(), 0x3f800000, true);
  Harness::expect(h.edge(), 0x3f800000, false);

  h.inputs.valid_in = partial<1>(0, 1, z, latent);
  Harness::expect(h.edge(), 0x3f800000, false);
  h.inputs.valid_in = known<1>(0);
  Harness::expect(h.edge(), 0x3f800000, false);
  Harness::expect(h.edge(), 0x3f800000, false);
  const auto committed = h.dut.system().cycle();
  h.inputs.pyc_7079635f636c6b = known<1>(1);
  h.dut.drive(h.inputs);
  require(h.dut.system().Step() == gfsim::SimStepResult::Failed);
  require(h.dut.system().state() == gfsim::SimSystemState::Failed);
  require(h.dut.system().cycle() == committed);
  bool guarded = false;
  try {
    (void)h.dut.sample();
  } catch (const std::logic_error &) {
    guarded = true;
  }
  require(guarded);
  require(h.dut.system().Step() == gfsim::SimStepResult::Failed);
  require(h.dut.system().cycle() == committed);
  std::cout << "FAILURE " << id << ' ' << committed << ' '
            << static_cast<unsigned>(h.dut.system().failureInfo().phase) << '\n';

  // Public Reset is the supported recovery. Private owner Q/clock history are
  // not observable here, so this is not an all-owner zero-commit proof.
  h.dut.system().Reset();
  require(h.dut.system().cycle() == 0);
  Harness::expect(h.step(false), 0, false);
  h.inputs.valid_in = known<1>(1);
  Harness::expect(h.edge(), 0, false);
  h.inputs.valid_in = known<1>(0);
  Harness::expect(h.edge(), 0, false);
  Harness::expect(h.edge(), 0, false);
  Harness::expect(h.edge(), 0x3f800000, true);
}
void run(unsigned workers) {
  unsigned id = 0;
  for (const auto &anchor : anchors)
    operandCase(id++, workers, anchor);
  for (unsigned pattern = 1; pattern <= 6; ++pattern)
    for (bool z : {false, true})
      for (bool latent : {false, true})
        operandCase(id++, workers,
                    pattern >= 4 ? anchors[2] : anchors[3], pattern, z, latent);
  for (bool z : {false, true})
    for (bool latent : {false, true})
      invalidEnableCase(id++, workers, z, latent);
  require(id == 32);
  std::cout << "CASES " << id << '\n';
}
} // namespace four_state_checks

int main(int argc, char **argv) {
  anchors();
  gfsim::SystemRunner runner(argc, argv);
  if (!runner.ready())
    return 2;
  four_state_checks::run(runner.workers());
  pyc_dut dut(runner.workers());
  Context context{dut};
  const gfsim::RunnerCallbacks callbacks{&context, &Context::initialize,
                                         &Context::drive, &Context::sample};
  const int status =
      runner.Run(dut.system(), dut.observations(), {}, callbacks);
  require(status == 0);
  context.finish();
  return status;
}
