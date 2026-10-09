#include "gfsim/SystemRunner.h"
#include "pycircuit_system.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <source_location>
#include <vector>

void require(bool ok,
             std::source_location at = std::source_location::current()) {
  if (!ok) {
    std::cerr << "issue_queue_2picker oracle at " << at.line() << '\n';
    std::abort();
  }
}
template <unsigned W> auto known(std::uint64_t value) {
  return gfsim::wire<gfsim::Bits<W>>::known(gfsim::Bits<W>{value});
}
struct Row {
  unsigned clock, reset, valid, data, ready0, ready1;
};
std::vector<Row> stimulus() {
  std::vector<Row> rows;
  auto add = [&](unsigned c, unsigned r, unsigned v, unsigned d, unsigned a,
                 unsigned b) { rows.push_back({c, r, v, d, a, b}); };
  auto edge = [&](unsigned d, unsigned v = 1, unsigned a = 0,
                  unsigned b = 0, unsigned r = 0) {
    add(1, r, v, d, a, b);
    add(0, r, v, d, a, b);
  };
  add(0, 1, 0, 0, 0, 0);
  edge(0, 0, 0, 0, 1);
  for (auto d : {0u, 255u, 254u, 128u})
    edge(d, 1, 0, 1);
  edge(11); edge(12); edge(13, 1, 0, 1);
  edge(14, 1, 1, 0); // Full pop-and-replace, using the old head.
  edge(15, 1, 1, 1);
  add(1, 0, 1, 16, 1, 1);
  add(1, 1, 1, 17, 1, 1); // Held high cannot reset or transfer.
  add(0, 0, 1, 18, 0, 1);
  add(0, 0, 0, 19, 1, 1);
  edge(20); edge(21);
  edge(0, 1, 1, 1, 1); // Reset overrides simultaneous requests at full.
  for (unsigned i = 0; i < 4; ++i) edge(250 + i);
  for (unsigned i = 0; i < 6; ++i) edge(i, 1, 1, 0);
  for (unsigned i = 0; i < 48; ++i)
    edge((i * 73 + 29) & 255, i % 5 != 1, i % 4 != 0, i % 3 != 1);
  for (unsigned i = 0; i < 4; ++i) edge(0, 0, 1, 1);
  for (unsigned i = 0; i < 4; ++i) edge(96 + i);
  edge(255, 1, 1, 1, 1);
  edge(255); edge(0);
  for (unsigned i = 0; i < 4; ++i) edge(0, 0, 1, 1);
  return rows;
}
struct Context {
  pyc_dut &dut;
  std::vector<Row> rows = stimulus();
  std::deque<unsigned> queue;
  bool last = false;
  unsigned sampled = 0, accepted = 0, retired = 0, dropped = 0, peak = 0,
           stalled = 0, replacements = 0, dual = 0, gated = 0, resetFull = 0;
  static void initialize(void *p) { require(drive(p, 0)); }
  static bool drive(void *p, std::uint64_t epoch) {
    auto &c = *static_cast<Context *>(p);
    if (epoch == c.rows.size()) return false;
    require(epoch < c.rows.size());
    const auto &r = c.rows[epoch];
    pyc_dut::Inputs in;
    in.pyc_7079635f636c6b = known<1>(r.clock);
    in.pyc_7079635f727374 = known<1>(r.reset);
    in.in_valid = known<1>(r.valid);
    in.in_data = known<8>(r.data);
    in.out0_ready = known<1>(r.ready0);
    in.out1_ready = known<1>(r.ready1);
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
    const bool valid0 = !queue.empty(), valid1 = queue.size() >= 2;
    const bool pop0 = valid0 && r.ready0;
    const bool pop1 = valid1 && r.ready1 && pop0;
    const bool ready = queue.size() < 4 || pop0;
    const auto actual = dut.sample().result.packed();
    require(actual.isFullyKnown());
    const auto value = actual.value().value();
    require(bool((value >> 18) & 1) == ready);
    require(bool((value >> 17) & 1) == valid0);
    require(bool((value >> 8) & 1) == valid1);
    const unsigned data0 = (value >> 9) & 255, data1 = value & 255;
    if (valid0) require(data0 == queue[0]);
    if (valid1) require(data1 == queue[1]);
    // Invalid data is unspecified by the historical picker protocol. Trace
    // normalization does not relax any valid-payload or handshake assertion.
    std::cout << "WORK " << sampled << ' ' << ready << ' ' << valid0 << ' '
              << (valid0 ? data0 : 0) << ' ' << valid1 << ' '
              << (valid1 ? data1 : 0) << '\n';
    if (r.clock && !last) {
      if (r.reset) {
        resetFull += queue.size() == 4;
        dropped += queue.size();
        queue.clear();
      } else {
        stalled += r.valid && !ready;
        replacements += queue.size() == 4 && pop0 && r.valid;
        dual += pop1;
        gated += valid1 && r.ready1 && !r.ready0;
        if (pop0) { queue.pop_front(); ++retired; }
        if (pop1) { queue.pop_front(); ++retired; }
        if (r.valid && ready) { queue.push_back(r.data); ++accepted; }
      }
      peak = std::max(peak, unsigned(queue.size()));
      require(queue.size() <= 4);
    }
    last = r.clock;
    ++sampled;
  }
  void finish() const {
    require(sampled == rows.size() && rows.size() <= 250 && queue.empty());
    require(peak == 4 && stalled >= 3 && replacements >= 6 && dual > 0 &&
            gated > 0 && resetFull == 2);
    require(accepted == retired + dropped && dropped == 8);
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
