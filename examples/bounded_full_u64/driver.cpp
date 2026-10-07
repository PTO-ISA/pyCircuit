#include "gfsim/SystemRunner.h"
#include "pycircuit_system.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <optional>
#include <source_location>
#include <vector>

void require(bool ok,
             std::source_location at = std::source_location::current()) {
  if (!ok) {
    std::cerr << "bounded_full_u64 oracle at " << at.line() << '\n';
    std::abort();
  }
}
template <unsigned W> auto known(std::uint64_t value) {
  return gfsim::wire<gfsim::Bits<W>>::known(gfsim::Bits<W>{value});
}
struct Row {
  unsigned clock, reset, valid, takes;
  std::uint64_t raw;
};
std::vector<Row> stimulus() {
  std::vector<Row> rows;
  auto add = [&](unsigned c, unsigned r, unsigned v, std::uint64_t d,
                 unsigned t) { rows.push_back({c, r, v, t, d}); };
  auto edge = [&](std::uint64_t d, unsigned v = 1, unsigned t = 15,
                  unsigned r = 0) {
    add(1, r, v, d, t);
    add(0, r, v, d, t);
  };
  add(0, 1, 0, 0, 0);
  edge(0, 0, 0, 1);
  for (auto d : {std::uint64_t{0}, UINT64_MAX, std::uint64_t{1} << 63,
                 (std::uint64_t{1} << 63) - 1})
    edge(d, 1, 0);
  // One blocked branch prevents every output push, even while others drain.
  for (unsigned branch = 0; branch < 4; ++branch) {
    for (unsigned i = 0; i < 4; ++i)
      edge(32 + branch * 4 + i, 1, 15 ^ (1 << branch));
    edge(64 + branch);
    edge(68 + branch);
  }
  add(1, 0, 1, 77, 0);
  add(1, 1, 1, 78, 15);
  add(0, 0, 1, 79, 0);
  add(0, 0, 0, 80, 15);
  edge(81, 1, 0);
  edge(82, 1, 0);
  edge(0, 1, 15, 1);
  for (auto d :
       {std::uint64_t{0}, UINT64_MAX, std::uint64_t{1}, std::uint64_t{1} << 63,
        (std::uint64_t{1} << 63) - 1, UINT64_MAX - 1})
    edge(d);
  for (unsigned i = 0; i < 96; ++i)
    edge((std::uint64_t{i} * UINT64_C(0x9e3779b97f4a7c15)) ^
             UINT64_C(0xa5a55a5af00f0ff0),
         i % 7 != 1,
         ((i % 3 != 0) << 0) | ((i % 5 != 1) << 1) | ((i % 4 != 2) << 2) |
             ((i % 6 != 3) << 3));
  for (unsigned i = 0; i < 4; ++i)
    edge(0, 0, 15);
  edge(UINT64_MAX, 1, 0);
  edge(0, 1, 0);
  edge(0, 1, 15, 1);
  edge(UINT64_MAX);
  edge(0);
  for (unsigned i = 0; i < 4; ++i)
    edge(0, 0, 15);
  return rows;
}
struct Context {
  pyc_dut &dut;
  std::vector<Row> rows = stimulus();
  std::optional<std::uint64_t> source;
  std::array<std::optional<std::uint64_t>, 4> sinks;
  std::array<std::deque<std::uint64_t>, 4> histories;
  std::array<unsigned, 4> retired{}, dropped{}, isolated{};
  bool last = false;
  unsigned sampled = 0, accepted = 0, peak = 0, stalled = 0, replacements = 0,
           transfers = 0;
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
    in.valid = known<1>(r.valid);
    in.raw = known<64>(r.raw);
    in.take_wrapped = known<1>(r.takes & 1);
    in.take_saturated = known<1>((r.takes >> 1) & 1);
    in.take_checked_value = known<1>((r.takes >> 2) & 1);
    in.take_checked_flag = known<1>((r.takes >> 3) & 1);
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
    bool room = true;
    for (unsigned k = 0; k < 4; ++k)
      room &= !sinks[k] || ((r.takes >> k) & 1);
    const bool ready = !source || room;
    const auto out = dut.sample().result.packed();
    require(out.isFullyKnown() && out.value().bit(197) == ready);
    const std::array<unsigned, 4> validBits{196, 131, 66, 1};
    const std::array<std::uint64_t, 4> values{
        gfsim::extract<64>(out, 132).value().value(),
        gfsim::extract<64>(out, 67).value().value(),
        gfsim::extract<64>(out, 2).value().value(),
        std::uint64_t(out.value().bit(0))};
    std::cout << "WORK " << sampled << ' ' << ready;
    for (unsigned k = 0; k < 4; ++k) {
      require(out.value().bit(validBits[k]) == sinks[k].has_value());
      require(values[k] == (sinks[k] ? (k == 3 ? 1 : *sinks[k]) : 0));
      std::cout << ' ' << sinks[k].has_value() << ' ' << values[k];
    }
    std::cout << '\n';
    if (r.clock && !last) {
      if (r.reset) {
        source.reset();
        for (unsigned k = 0; k < 4; ++k) {
          dropped[k] += histories[k].size();
          histories[k].clear();
          sinks[k].reset();
        }
      } else {
        stalled += r.valid && !ready;
        replacements += source && room && r.valid;
        const auto oldSource = source;
        for (unsigned k = 0; k < 4; ++k) {
          const bool pop = sinks[k] && ((r.takes >> k) & 1);
          if (pop) {
            require(!histories[k].empty() && *sinks[k] == histories[k].front());
            histories[k].pop_front();
            ++retired[k];
            isolated[k] += !room;
          }
          if (oldSource && room)
            sinks[k] = oldSource;
          else if (pop)
            sinks[k].reset();
          if (r.valid && ready)
            histories[k].push_back(r.raw);
          peak = std::max(peak, unsigned(histories[k].size()));
          require(histories[k].size() <= 2);
        }
        transfers += oldSource.has_value() && room;
        if (r.valid && ready) {
          source = r.raw;
          ++accepted;
        } else if (oldSource && room)
          source.reset();
      }
    }
    last = r.clock;
    ++sampled;
  }
  void finish() const {
    require(sampled == rows.size() && sampled <= 400 && peak == 2 &&
            stalled >= 4 && replacements >= 6 && transfers >= 20 && !source);
    for (unsigned k = 0; k < 4; ++k) {
      require(!sinks[k] && histories[k].empty() && dropped[k] >= 2 &&
              isolated[k] > 0 && accepted == retired[k] + dropped[k]);
      std::cout << "HISTORY " << k << ' ' << accepted << ' ' << retired[k]
                << ' ' << dropped[k] << " 0 " << peak << '\n';
    }
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
