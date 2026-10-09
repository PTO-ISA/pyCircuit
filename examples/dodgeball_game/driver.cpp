#include "gfsim/SystemRunner.h"
#include "pycircuit_system.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <source_location>
#include <vector>

void require(bool ok,
             std::source_location at = std::source_location::current()) {
  if (!ok) {
    std::cerr << "dodgeball_game oracle at " << at.line() << '\n';
    std::abort();
  }
}
template <unsigned W> auto known(std::uint64_t value) {
  return gfsim::wire<gfsim::Bits<W>>::known(gfsim::Bits<W>{value});
}
struct Row {
  unsigned clock, reset, button, start, left, right;
};
std::vector<Row> stimulus() {
  std::vector<Row> rows{
      {0, 1, 0, 0, 0, 0}, {1, 1, 0, 0, 0, 0}, {0, 1, 0, 0, 0, 0}};
  for (unsigned n = 0; n < 4000; ++n) {
    const unsigned start =
        n < 16 || (n >= 928 && n < 960) || (n >= 992 && n < 1024);
    const unsigned button = (n >= 896 && n < 928) || (n >= 960 && n < 1024);
    const unsigned left = (n >= 16 && n < 320) || (n >= 832 && n < 896);
    const unsigned right = n >= 320 && n < 896;
    rows.push_back({1, 0, button, start, left, right});
    if (n == 31)
      rows.push_back({1, 1, 1, 1, 1, 1});
    rows.push_back({0, 0, button, start, left, right});
    if (n == 32)
      rows.push_back({0, 1, 1, 1, 1, 1});
  }
  return rows;
}
// Number of active game ticks supplies an independent closed-form object
// oracle. Its j wraps every 32 ticks; each object advances twelve times per
// j period, starting at old-j values 1, 4 and 8 respectively.
unsigned objectY(unsigned activeTicks, unsigned first) {
  return (12 * (activeTicks / 32) +
          std::clamp(int(activeTicks % 32) - int(first), 0, 12)) &
         15;
}
struct Context {
  pyc_dut &dut;
  std::vector<Row> rows = stimulus();
  unsigned sampled = 0, age = 0, state = 0, activeTicks = 0, player = 8;
  unsigned tickCount = 0, leftStops = 0, rightStops = 0, bothHeld = 0,
           resets = 0, j20 = 0, hWrap = 0, blueSamples = 0, blankSamples = 0;
  bool last = false;
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
    in.RST_BTN = known<1>(r.button);
    in.START = known<1>(r.start);
    in.left = known<1>(r.left);
    in.right = known<1>(r.right);
    c.dut.drive(in);
    return true;
  }
  static void sample(void *p, std::uint64_t epoch) {
    auto &c = *static_cast<Context *>(p);
    require(epoch == c.sampled + 1);
    c.observe();
  }
  void observe() {
    const auto &r = rows[sampled];
    // The divider produces a carry on edge 4 and VGA consumes it on edge 5.
    // Horizontal count includes 800, giving 801 pixel strobes per line.
    const unsigned pixels = age ? (age - 1) / 4 : 0;
    const unsigned h = pixels % 801, v = pixels / 801;
    require(v <= 1 && state <= 1);
    const unsigned hs = h < 16 || h >= 112;
    const unsigned blue = v == 1 && h > 160 && h < 800 ? 8 : 0;
    std::uint64_t expected = hs;
    auto append = [&](unsigned value, unsigned width) {
      expected = (expected << width) | value;
    };
    append(1, 1);
    append(0, 4);
    append(0, 4);
    append(blue, 4);
    append(state, 3);
    append(activeTicks & 31, 5);
    append(player, 4);
    append(1, 4);
    append(objectY(activeTicks, 1), 4);
    append(4, 4);
    append(objectY(activeTicks, 4), 4);
    append(7, 4);
    append(objectY(activeTicks, 8), 4);
    const auto actual = dut.sample().result.packed();
    require(actual.isFullyKnown() && actual.value().value() == expected);
    std::cout << "WORK " << sampled << ' ' << actual.value().value() << '\n';
    blankSamples += !hs;
    blueSamples += blue != 0;
    hWrap += age == 3205;
    if (r.clock && !last) {
      if (r.reset) {
        age = 0;
        state = 0;
        activeTicks = 0;
        player = 8;
      } else {
        if ((age & 31) == 15) {
          ++tickCount;
          if (state == 1) {
            // This stimulus avoids collision coordinates; later movement is
            // fixed at x=15 while objects retain x=1,4,7.
            require(!((player == 1 && objectY(activeTicks, 1) == 10) ||
                      (player == 4 && objectY(activeTicks, 4) == 10) ||
                      (player == 7 && objectY(activeTicks, 8) == 10)));
            j20 += (activeTicks & 31) == 20;
            ++activeTicks;
            if (r.left && !r.right) {
              leftStops += player == 0;
              player -= player != 0;
            }
            if (r.right && !r.left) {
              rightStops += player == 15;
              player += player != 15;
            }
            bothHeld += r.left && r.right;
            if (r.button) {
              state = 0;
              ++resets;
            }
          } else if (r.start)
            state = 1;
        }
        ++age;
      }
    }
    last = r.clock;
    ++sampled;
  }
  void finish() const {
    require(sampled == rows.size() && sampled < 8192 && age == 4000 &&
            state == 1 && player == 15 && tickCount == 125 && leftStops > 0 &&
            rightStops > 0 && bothHeld == 2 && resets == 2 && j20 >= 3 &&
            hWrap > 0 && blueSamples > 0 && blankSamples > 0);
    std::cout << "HISTORY " << tickCount << ' ' << activeTicks << ' ' << player
              << ' ' << resets << '\n';
  }
};
int main(int argc, char **argv) {
  gfsim::SystemRunner runner(argc, argv);
  if (!runner.ready())
    return 2;
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
