// Independent oracle for the u64 keypad calculator.
//
// Runner contract: invoked as `<driver> --config <config.json> --workers N`,
// emitting one `WORK ` record per sampled epoch and non-`WORK` check lines.
//
// Two oracles share this driver:
//   * the recovered historical idle oracle. The original `tb_calculator.py`
//     clocked `clk`, reset with 2 asserted + 1 deasserted cycles, drove
//     key=0/key_press=0, expected display==0, and called `finish(at=1)`. Its
//     `timeout(64)` was an upper bound that was never reached, so this driver
//     reproduces the real shape: reset, then one post-reset check that the
//     display is 0.
//   * a new independent functional oracle derived from the historical u64 state
//     machine, covering decimal entry, + - * /, EQ, AC and the
//     zero-divisor-replaced-by-one rule. This is stronger than the original and
//     is labelled as such.
//
// The host supplies stimulus and expected values only. Every observed value is
// read from the generated DUT; the model never drives DUT state.
//
// Key codes: 0..9 digit, 10 add, 11 sub, 12 mul, 13 div, 14 equals, 15 clear.
#include "gfsim/SystemRunner.h"
#include "pycircuit_system.hpp"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <source_location>
#include <string_view>
#include <vector>

void require(bool ok, const char *message = nullptr,
             std::source_location at = std::source_location::current()) {
  if (!ok) {
    std::cerr << "calculator oracle line " << at.line();
    if (message)
      std::cerr << ": " << message;
    std::cerr << '\n';
    std::abort();
  }
}
template <unsigned W> auto known(std::uint64_t value) {
  return gfsim::wire<gfsim::Bits<W>>::known(gfsim::Bits<W>{value});
}

// The historical model, transcribed from the original source. Outputs observe
// the committed state, so the DUT is compared before the model advances.
struct Model {
  std::uint64_t lhs = 0, rhs = 0, shown = 0;
  unsigned pending = 0;

  void press(unsigned key) {
    const bool digit = key <= 9;
    const bool add = key == 10, sub = key == 11;
    const bool mul = key == 12, div = key == 13;
    const bool eq = key == 14, ac = key == 15;
    if (digit) {
      if (entering_rhs) {
        rhs = rhs * 10 + key;
        shown = rhs;
      } else {
        lhs = lhs * 10 + key;
        shown = lhs;
      }
    }
    if (add || sub || mul || div) {
      entering_rhs = 1;
      rhs = 0;
      if (add) pending = 0;
      if (sub) pending = 1;
      if (mul) pending = 2;
      if (div) pending = 3;
    }
    const std::uint64_t divisor = rhs == 0 ? 1 : rhs;  // zero divisor becomes 1
    std::uint64_t computed = lhs;
    if (pending == 0) computed = lhs + rhs;
    if (pending == 1) computed = lhs - rhs;
    if (pending == 2) computed = lhs * rhs;
    if (pending == 3) computed = lhs / divisor;
    if (eq) {
      lhs = computed;
      shown = computed;
      rhs = 0;
      entering_rhs = 0;
    }
    if (ac) {
      lhs = rhs = shown = 0;
      pending = 0;
      entering_rhs = 0;
    }
  }

private:
  unsigned entering_rhs = 0;
};

// One stimulus entry per rising edge. A zero key with `press=false` is a hold.
struct Stimulus {
  unsigned key = 0;
  bool press = false;
};

pyc_dut::Inputs inputs(const Stimulus &s, unsigned clock, bool reset) {
  pyc_dut::Inputs in;
  in.key = known<5>(s.key);
  in.key_press = known<1>(s.press ? 1 : 0);
  in.pyc_7079635f636c6b = known<1>(clock);
  in.pyc_7079635f727374 = known<1>(reset ? 1u : 0u);
  return in;
}

struct RunnerContext {
  pyc_dut &dut;
  std::vector<Stimulus> trace;
  Model model;
  unsigned sampled = 0;
  unsigned checked = 0;
  unsigned frames() const { return 2 * static_cast<unsigned>(trace.size()); }
  static void initialize(void *opaque) { require(drive(opaque, 0)); }
  static bool drive(void *opaque, std::uint64_t epoch) {
    auto &self = *static_cast<RunnerContext *>(opaque);
    if (epoch >= self.frames())
      return false;
    const bool high = epoch % 2 == 1;
    const Stimulus s = high ? self.trace[static_cast<size_t>(epoch / 2)] : Stimulus{};
    self.dut.drive(inputs(s, high ? 1u : 0u, epoch < 4));
    return true;
  }
  static void sample(void *opaque, std::uint64_t epoch) {
    auto &self = *static_cast<RunnerContext *>(opaque);
    require(epoch == self.sampled + 1);
    // drive() receives the pre-Step index; sample() receives the completed
    // count. Work observed the driven frame before its Xfer commits.
    const std::uint64_t frame = epoch - 1;
    const bool high = frame % 2 == 1;
    // CalcResult packs to 66 bits; the first declared field takes the most
    // significant bits, so op_pending is the low plane and display sits above.
    const auto packed = self.dut.sample().result.packed();
    const auto pendingBits = gfsim::extract<2>(packed, 0);
    const auto shownBits = gfsim::extract<64>(packed, 2);
    require(shownBits.knownMask().value() == ~0ull && shownBits.zMask().value() == 0,
            "display must be fully known");
    require(pendingBits.knownMask().value() == 3u && pendingBits.zMask().value() == 0,
            "op_pending must be fully known");
    const std::uint64_t shown = shownBits.value().value();
    const unsigned pending = static_cast<unsigned>(pendingBits.value().value());
    if (shown != self.model.shown || pending != self.model.pending) {
      const unsigned drivenKey =
          high ? self.trace[static_cast<size_t>(frame / 2)].key : 0u;
      std::cerr << "mismatch at runner epoch " << epoch << " (sample "
                << self.sampled << ", " << (high ? "clock-high" : "clock-low")
                << ", key=" << drivenKey << "): got display=" << shown
                << " op_pending=" << pending << ", want display=" << self.model.shown
                << " op_pending=" << self.model.pending << '\n';
    }
    require(shown == self.model.shown, "display disagrees with the historical model");
    require(pending == self.model.pending, "op_pending disagrees with the model");
    std::cout << "WORK " << self.sampled << ' ' << shown << ' ' << pending << '\n';
    ++self.sampled;
    ++self.checked;
    if (high) {
      const Stimulus &applied = self.trace[static_cast<size_t>(frame / 2)];
      // Spacer entries hold the keypad idle; only a real press changes state.
      if (applied.press)
        self.model.press(applied.key);
    }
  }
};

void appendKey(std::vector<Stimulus> &trace, unsigned key) {
  trace.push_back({key, true});
  trace.push_back({0, false});
}

int main(int argc, char **argv) {
  gfsim::SystemRunner runner(argc, argv);
  if (!runner.ready())
    return 2;
  pyc_dut dut(runner.workers());

  // Historical idle shape: reset, then hold key=0/key_press=0. The original
  // testbench checked display==0 once after reset; the longer hold below is a
  // stronger extension and is reported as such.
  RunnerContext context{dut, {}};
  context.trace.push_back({0, false});  // reset asserted window
  context.trace.push_back({0, false});
  context.trace.push_back({0, false});
  for (unsigned i = 0; i < 64; ++i)
    context.trace.push_back({0, false});  // extended idle hold (not historical)
  require(context.model.shown == 0, "idle display must remain 0");

  // 12 + 34 = 46, then 7 * 6 = 42, 9 - 4 = 5, AC, 8 / 0 = 8, 100 / 4 = 25.
  for (unsigned key : {1u, 2u, 10u, 3u, 4u, 14u})
    appendKey(context.trace, key);
  for (unsigned key : {15u, 7u, 12u, 6u, 14u})
    appendKey(context.trace, key);
  for (unsigned key : {15u, 9u, 11u, 4u, 14u})
    appendKey(context.trace, key);
  appendKey(context.trace, 15u);
  for (unsigned key : {8u, 13u, 14u})
    appendKey(context.trace, key);
  for (unsigned key : {15u, 1u, 0u, 0u, 13u, 4u, 14u})
    appendKey(context.trace, key);

  // Exercise the full unsigned range and modulo-2^64 behavior through keys.
  appendKey(context.trace, 15u);
  for (char digit : std::string_view("18446744073709551615"))
    appendKey(context.trace, static_cast<unsigned>(digit - '0'));
  for (unsigned key : {13u, 3u, 14u, 12u, 3u, 14u, 10u, 1u, 14u,
                       11u, 1u, 14u, 12u, 2u, 14u, 15u})
    appendKey(context.trace, key);
  for (char digit : std::string_view("184467440737095516150"))
    appendKey(context.trace, static_cast<unsigned>(digit - '0'));

  const gfsim::RunnerCallbacks callbacks{
      &context, &RunnerContext::initialize, &RunnerContext::drive,
      &RunnerContext::sample};
  const int result =
      runner.Run(dut.system(), dut.observations(), {}, callbacks);
  require(result == 0, "runner reported a failure");
  require(context.checked == context.frames(), "not every epoch was sampled");
  require(context.model.shown == UINT64_C(18446744073709551606),
          "decimal entry must wrap modulo 2^64");
  std::cout << "PASS calculator workers=" << runner.workers()
            << " epochs=" << context.checked << '\n';
  return 0;
}
