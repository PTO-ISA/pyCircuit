#include "gfsim/SystemRunner.h"
#include "pycircuit_system.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <source_location>
#include <string>
#include <vector>

void require(bool ok,
             std::source_location at = std::source_location::current()) {
  if (!ok) {
    std::cerr << "recursive_pipeline oracle at " << at.line() << '\n';
    std::abort();
  }
}
template <unsigned W> auto known(std::uint64_t value) {
  return gfsim::wire<gfsim::Bits<W>>::known(gfsim::Bits<W>{value});
}
std::string binary(std::uint64_t value) {
  std::string text;
  for (unsigned bit = 64; bit; --bit)
    text += (value >> (bit - 1)) & 1 ? '1' : '0';
  return text;
}
struct Row {
  unsigned clock, reset, valid;
  std::uint64_t data;
  unsigned take;
};
std::vector<Row> stimulus() {
  std::vector<Row> rows;
  auto add = [&](unsigned c, unsigned r, unsigned v, std::uint64_t d,
                 unsigned t) { rows.push_back({c, r, v, d, t}); };
  auto edge = [&](std::uint64_t d, unsigned v = 1, unsigned t = 1,
                  unsigned r = 0) {
    add(1, r, v, d, t);
    add(0, r, v, d, t);
  };
  constexpr auto max = ~std::uint64_t{0};
  add(0, 1, 0, 0, 0);
  edge(0, 0, 0, 1);
  edge(max); // E0, then no flow-through; earliest consumption is E4.
  for (unsigned i = 0; i < 4; ++i) edge(0, 0);
  for (unsigned i = 0; i < 12; ++i) edge(max - i, 1, 0);
  add(1, 0, 1, 17, 0);
  add(1, 1, 1, 18, 1); // Held-high reset/take cannot change Q.
  add(0, 0, 1, 19, 1);
  add(0, 0, 0, 20, 0);
  edge(0, 1, 1, 1); // Reset drops all eight live tokens.
  for (unsigned i = 0; i < 12; ++i) edge(i, 1, 0);
  for (unsigned i = 0; i < 6; ++i) edge(max - i);
  std::uint64_t random = 0xd2106421;
  for (unsigned i = 0; i < 48; ++i) {
    random = random * 6364136223846793005ULL + 1442695040888963407ULL;
    const auto d = i % 8 == 0 ? max : i % 8 == 1 ? max - 1 : random;
    edge(d, i % 5 != 1, i % 4 != 0);
  }
  for (unsigned i = 0; i < 12; ++i) edge(0, 0);
  for (unsigned i = 0; i < 12; ++i) edge(96 + i, 1, 0);
  edge(0, 1, 1, 1);
  edge(max - 2); // A real zero result after three modulo-2^64 additions.
  for (unsigned i = 0; i < 4; ++i) edge(0, 0);
  return rows;
}
struct Token {
  std::uint64_t value;
  unsigned birth;
};
struct Context {
  pyc_dut &dut;
  std::vector<Row> rows = stimulus();
  std::array<std::deque<std::uint64_t>, 4> queues;
  std::deque<Token> history;
  bool last = false;
  unsigned sampled = 0, edges = 0, accepted = 0, retired = 0, dropped = 0,
           peak = 0, stalled = 0, replacements = 0, resetFull = 0;
  bool firstRetirement = false, zeroRetirement = false;
  static void initialize(void *p) { require(drive(p, 0)); }
  static bool drive(void *p, std::uint64_t epoch) {
    auto &c = *static_cast<Context *>(p);
    if (epoch == c.rows.size()) return false;
    require(epoch < c.rows.size());
    const auto &r = c.rows[epoch];
    pyc_dut::Inputs in;
    in.pyc_7079635f636c6b = known<1>(r.clock);
    in.pyc_7079635f727374 = known<1>(r.reset);
    in.valid = known<1>(r.valid);
    in.data = known<64>(r.data);
    in.take = known<1>(r.take);
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
    // Derive all transfers from old occupancies/heads before mutating a deque.
    std::array<bool, 4> pop;
    pop[3] = !queues[3].empty() && r.take;
    for (int i = 2; i >= 0; --i)
      pop[i] = !queues[i].empty() && (queues[i + 1].size() < 2 || pop[i + 1]);
    const bool ready = queues[0].size() < 2 || pop[0];
    const bool valid = !queues[3].empty();
    const auto expected = valid ? queues[3].front() : 0;
    const auto actual = dut.sample().result.packed();
    require(actual.isFullyKnown());
    require(actual.value().bit(65) == ready && actual.value().bit(64) == valid);
    const auto data = gfsim::extract<64>(actual, 0).value().value();
    require(data == expected);
    std::cout << "WORK " << sampled << ' ' << actual.value().bit(65) << ' '
              << actual.value().bit(64) << ' ' << binary(data) << '\n';
    if (r.clock && !last) {
      if (r.reset) {
        resetFull += history.size() == 8;
        dropped += history.size();
        history.clear();
        for (auto &q : queues) q.clear();
      } else {
        stalled += r.valid && !ready;
        replacements += history.size() == 8 && pop[3] && r.valid && ready;
        if (pop[3]) {
          require(!history.empty() && data == history.front().value);
          require(edges - history.front().birth >= 4);
          if (!firstRetirement) {
            require(edges - history.front().birth == 4 && data == 2);
            firstRetirement = true;
          }
          zeroRetirement |= data == 0;
          history.pop_front();
          ++retired;
        }
        if (r.valid && ready) {
          history.push_back({r.data + std::uint64_t{3}, edges});
          ++accepted;
        }
        std::array<std::uint64_t, 4> oldHeads{};
        for (unsigned i = 0; i < 4; ++i)
          if (!queues[i].empty()) oldHeads[i] = queues[i].front();
        for (unsigned i = 0; i < 4; ++i)
          if (pop[i]) queues[i].pop_front();
        for (unsigned i = 1; i < 4; ++i)
          if (pop[i - 1]) queues[i].push_back(oldHeads[i - 1] + std::uint64_t{1});
        if (r.valid && ready) queues[0].push_back(r.data);
      }
      unsigned occupancy = 0;
      for (const auto &q : queues) { require(q.size() <= 2); occupancy += q.size(); }
      require(occupancy == history.size() && occupancy <= 8);
      peak = std::max(peak, occupancy);
      ++edges;
    }
    last = r.clock;
    ++sampled;
  }
  void finish() const {
    require(sampled == rows.size() && rows.size() <= 250 && history.empty());
    require(peak == 8 && stalled >= 3 && replacements >= 6 && resetFull == 2);
    require(firstRetirement && zeroRetirement && dropped == 16 &&
            accepted == retired + dropped);
    std::cout << "HISTORY " << accepted << ' ' << retired << ' ' << dropped
              << " 0 " << peak << '\n';
  }
};
int main(int argc, char **argv) {
  gfsim::SystemRunner runner(argc, argv);
  if (!runner.ready()) return 2;
  pyc_dut dut(runner.workers());
  Context context{dut};
  const gfsim::RunnerCallbacks callbacks{&context, &Context::initialize,
                                         &Context::drive, &Context::sample};
  const int status = runner.Run(dut.system(), dut.observations(), {}, callbacks);
  require(status == 0);
  context.finish();
  return status;
}
