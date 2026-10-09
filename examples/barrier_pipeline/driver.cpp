#include "gfsim/SimExecutor.h"
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
#include <string_view>
#include <vector>
void require(bool ok,
             std::source_location at = std::source_location::current()) {
  if (!ok) {
    std::cerr << "barrier independent oracle at " << at.line() << '\n';
    std::abort();
  }
}
struct Planes {
  std::uint64_t value = 0, known = 0, z = 0;
  bool operator==(const Planes &) const = default;
};
std::uint64_t mask(unsigned w) { return (std::uint64_t{1} << w) - 1; }
Planes known(unsigned w, std::uint64_t v = 0) {
  return {v & mask(w), mask(w), 0};
}
std::string visible(Planes p, unsigned w) {
  std::string s;
  for (int b = int(w) - 1; b >= 0; --b)
    s += p.z >> b & 1          ? 'z'
         : !(p.known >> b & 1) ? 'x'
         : p.value >> b & 1    ? '1'
                               : '0';
  return s;
}
template <unsigned W> auto wire(Planes p) {
  using B = gfsim::Bits<W>;
  return gfsim::wire<B>::fromPacked(
      gfsim::FourState<W>::fromMasks(B{p.value}, B{p.known}, B{p.z}));
}
auto bit(bool v) {
  return gfsim::wire<gfsim::Bits<1>>::known(gfsim::Bits<1>{v});
}
auto unknown() { return gfsim::wire<gfsim::Bits<1>>::unknown(); }
struct Pair {
  Planes left, right;
};
Pair value(unsigned n) {
  return {known(16, 0xa55au ^ (n * 257u)),
          known(32, 0x89abcdefu ^ (n * 0x01010101u))};
}
std::vector<Planes> raw(unsigned w) {
  std::vector<Planes> a;
  for (unsigned b = 0; b < w; ++b)
    for (bool z : {false, true})
      for (bool latent : {false, true}) {
        auto m = std::uint64_t{1} << b;
        a.push_back({(0xa55aa55au & mask(w) & ~m) | (latent ? m : 0),
                     mask(w) ^ m, z ? m : 0});
      }
  for (unsigned pattern = 0; pattern < 4; ++pattern)
    for (bool latent : {false, true}) {
      Planes p;
      for (unsigned b = 0; b < w; ++b) {
        char c = pattern == 0   ? 'x'
                 : pattern == 1 ? 'z'
                 : pattern == 2 ? "xz"[b % 2]
                                : "01xz"[b % 4];
        p.value |= std::uint64_t(c == '1' || ((c == 'x' || c == 'z') && latent))
                   << b;
        p.known |= std::uint64_t(c == '0' || c == '1') << b;
        p.z |= std::uint64_t(c == 'z') << b;
      }
      a.push_back(p);
    }
  return a;
}
std::vector<Pair> vectors(bool four) {
  std::vector<Pair> a;
  if (!four) {
    for (unsigned n = 0; n < 256; ++n)
      a.push_back(value(n));
  } else {
    auto l = raw(16), r = raw(32);
    for (unsigned n = 0; n < l.size(); ++n)
      a.push_back({l[n], value(n).right});
    for (unsigned n = 0; n < r.size(); ++n)
      a.push_back({value(n).left, r[n]});
    for (unsigned n = 0; n < 8; ++n)
      a.push_back({l[l.size() - 8 + n], r[r.size() - 8 + (7 - n)]});
  }
  require(a.size() == (four ? 216u : 256u));
  return a;
}
struct Row {
  bool clock, reset, lv, rv, lt, rt;
  Pair data;
};
std::vector<Row> stimulus(bool four) {
  std::vector<Row> a;
  auto row = [&](bool c, bool r, bool lv, bool rv, bool lt, bool rt, Pair p) {
    a.push_back({c, r, lv, rv, lt, rt, p});
  };
  auto edge = [&](Pair p, bool lv = true, bool rv = true, bool lt = true,
                  bool rt = true, bool r = false) {
    row(false, r, lv, rv, lt, rt, p);
    row(true, r, lv, rv, lt, rt, p);
  };
  auto zero = Pair{known(16), known(32)};
  row(false, true, false, false, false, false, zero);
  edge(zero, false, false, false, false, true);
  // Early left arrivals cannot move without a positional right peer.
  edge(value(0), true, false, false, false);
  edge(value(1), true, false, false, false);
  edge(value(2), true, false, false, false);
  row(true, true, true, false, true, true, value(3));
  row(true, false, true, false, false, false, value(4));
  row(false, true, false, false, false, false, zero);
  row(false, true, false, false, false, false, zero);
  edge(value(0), false, true, false, false);
  edge(value(1), false, true, false, false);
  for (unsigned n = 0; n < 8; ++n)
    edge(value(5 + n));
  for (unsigned n = 0; n < 6; ++n)
    edge(zero, false, false);
  edge(zero, false, false, false, false, true);
  // Four pairs fill all eight contributors. The fifth offer must be rejected.
  for (unsigned n = 0; n < 4; ++n)
    edge(value(20 + n), true, true, false, false);
  edge(value(24), true, true, false, false);
  for (unsigned n = 0; n < 3; ++n)
    edge(value(25 + n), true, true, true, false);
  row(true, true, true, true, false, true, value(30));
  row(false, true, true, true, false, false, value(31));
  edge(value(32), true, true, false, true);
  for (unsigned n = 0; n < 24; ++n)
    edge(value(33 + n));
  // Restore eight occupied contributors, then reset all four queue owners.
  edge(value(58), true, true, false, true);
  edge(zero, true, true, true, true, true);
  // Mirror the blocked-output direction, with independently skewed inputs.
  for (unsigned n = 0; n < 4; ++n)
    edge(value(60 + n), true, true, false, false);
  for (unsigned n = 0; n < 3; ++n)
    edge(value(64 + n), true, true, false, true);
  edge(value(68), true, true, true, false);
  for (unsigned n = 0; n < 8; ++n)
    edge(value(69 + n));
  for (Pair p : vectors(four))
    edge(p);
  for (unsigned n = 0; n < 8; ++n)
    edge(zero, false, false);
  for (unsigned n = 0; n < 4; ++n)
    edge(value(80 + n), true, true, false, false);
  edge(zero, true, true, true, true, true);
  for (unsigned n = 0; n < 5; ++n)
    edge(zero, false, false);
  return a;
}
struct Token {
  std::uint64_t id;
  Planes data;
};
struct Expected {
  std::array<bool, 2> ready, valid;
  std::array<Planes, 2> data;
  bool move;
  std::array<bool, 2> pop;
};
struct Golden {
  std::array<std::deque<Token>, 2> input, output;
  std::array<std::uint64_t, 2> next{};
  bool clock = false;
  unsigned pairs = 0;
  std::vector<std::array<std::uint64_t, 2>> pairIds;
  Expected read(const Row &r) const {
    bool l = !input[0].empty(), rr = !input[1].empty();
    bool pl = !output[0].empty() && r.lt, pr = !output[1].empty() && r.rt;
    bool move =
        l && rr && (output[0].size() < 2 || pl) && (output[1].size() < 2 || pr);
    return {{input[0].size() < 2 || move, input[1].size() < 2 || move},
            {!output[0].empty(), !output[1].empty()},
            {output[0].empty() ? known(16) : output[0].front().data,
             output[1].empty() ? known(32) : output[1].front().data},
            move,
            {pl, pr}};
  }
  void commit(const Row &r) {
    auto e = read(r);
    if (r.clock && !clock) {
      if (r.reset) {
        for (auto &q : input)
          q.clear();
        for (auto &q : output)
          q.clear();
      } else {
        if (e.move) {
          pairIds.push_back({input[0].front().id, input[1].front().id});
          ++pairs;
        }
        for (unsigned i = 0; i < 2; ++i) {
          if (e.pop[i])
            output[i].pop_front();
          if (e.move) {
            output[i].push_back(input[i].front());
            input[i].pop_front();
          }
          if ((i ? r.rv : r.lv) && e.ready[i])
            input[i].push_back({next[i]++, i ? r.data.right : r.data.left});
          require(input[i].size() <= 2 && output[i].size() <= 2);
        }
      }
    }
    clock = r.clock;
  }
};
pyc_dut::Inputs ports(const Row &r) {
  pyc_dut::Inputs p;
  static_assert(decltype(p.left_data)::width == 16 &&
                decltype(p.right_data)::width == 32);
  p.pyc_7079635f636c6b = bit(r.clock);
  p.pyc_7079635f727374 = bit(r.reset);
  p.left_valid = bit(r.lv);
  p.right_valid = bit(r.rv);
  p.left_take = bit(r.lt);
  p.right_take = bit(r.rt);
  p.left_data =
      decltype(p.left_data)::fromPacked(wire<16>(r.data.left).packed());
  p.right_data =
      decltype(p.right_data)::fromPacked(wire<32>(r.data.right).packed());
  return p;
}
template <unsigned W, class P> void same(const P &p, Planes e) {
  require(p.value() == gfsim::Bits<W>{e.value} &&
          p.knownMask() == gfsim::Bits<W>{e.known} &&
          p.zMask() == gfsim::Bits<W>{e.z});
}
struct Context {
  pyc_dut &dut;
  std::vector<Row> rows;
  Golden golden;
  std::array<std::deque<Token>, 2> history;
  std::array<unsigned, 2> accepted{}, retired{}, dropped{}, next{}, peak{};
  unsigned sampled = 0, totalPeak = 0, blockedLeft = 0, blockedRight = 0,
           independentLeft = 0, independentRight = 0, replacement = 0,
           fullReset = 0;
  bool clock = false;
  void observe(std::string_view label) {
    const auto &r = rows[sampled];
    auto e = golden.read(r);
    auto p = dut.sample().result.packed();
    static_assert(decltype(dut.sample().result)::width == 52);
    const unsigned rb[2] = {51, 33}, vb[2] = {50, 32};
    std::array<bool, 2> ar, av;
    for (unsigned i = 0; i < 2; ++i) {
      require(p.knownMask().bit(rb[i]) && p.knownMask().bit(vb[i]) &&
              !p.zMask().bit(rb[i]) && !p.zMask().bit(vb[i]));
      ar[i] = p.value().bit(rb[i]);
      av[i] = p.value().bit(vb[i]);
      require(ar[i] == e.ready[i] && av[i] == e.valid[i]);
    }
    auto ld = gfsim::extract<16>(p, 34);
    auto rd = gfsim::extract<32>(p, 0);
    same<16>(ld, e.data[0]);
    same<32>(rd, e.data[1]);
    std::cout << label << ' ' << sampled << ' ' << ar[0] << ' ' << av[0] << ' '
              << visible(e.data[0], 16) << ' ' << ar[1] << ' ' << av[1] << ' '
              << visible(e.data[1], 32) << '\n';
    if (r.clock && !clock) {
      if (r.reset) {
        fullReset += history[0].size() + history[1].size() == 8;
        for (unsigned i = 0; i < 2; ++i) {
          dropped[i] += history[i].size();
          history[i].clear();
        }
      } else {
        blockedLeft += golden.output[0].size() == 2 && !r.lt;
        blockedRight += golden.output[1].size() == 2 && !r.rt;
        independentLeft += av[0] && r.lt && !(av[1] && r.rt);
        independentRight += av[1] && r.rt && !(av[0] && r.lt);
        replacement += e.move && golden.input[0].size() == 2 &&
                       golden.input[1].size() == 2 && r.lv && r.rv;
        for (unsigned i = 0; i < 2; ++i) {
          if (av[i] && (i ? r.rt : r.lt)) {
            require(!history[i].empty());
            if (i)
              same<32>(rd, history[i].front().data);
            else
              same<16>(ld, history[i].front().data);
            history[i].pop_front();
            ++retired[i];
          }
          if (ar[i] && (i ? r.rv : r.lv)) {
            history[i].push_back({next[i]++, i ? r.data.right : r.data.left});
            ++accepted[i];
          }
          peak[i] = std::max(peak[i], unsigned(history[i].size()));
          require(history[i].size() <= 4);
        }
      }
      totalPeak =
          std::max(totalPeak, unsigned(history[0].size() + history[1].size()));
    }
    clock = r.clock;
    golden.commit(r);
    for (unsigned i = 0; i < 2; ++i)
      require(history[i].size() ==
              golden.input[i].size() + golden.output[i].size());
    ++sampled;
  }
  static void initialize(void *p) { require(drive(p, 0)); }
  static bool drive(void *p, std::uint64_t epoch) {
    auto &c = *static_cast<Context *>(p);
    if (epoch == c.rows.size())
      return false;
    c.dut.drive(ports(c.rows[epoch]));
    return true;
  }
  static void sample(void *p, std::uint64_t epoch) {
    auto &c = *static_cast<Context *>(p);
    require(epoch == c.sampled + 1);
    c.observe("WORK");
  }
  void finish() {
    require(sampled == rows.size() && totalPeak == 8 && peak[0] == 4 &&
            peak[1] == 4 && blockedLeft >= 3 && blockedRight >= 3 &&
            independentLeft >= 2 && independentRight >= 2 &&
            replacement >= 10 && fullReset >= 2);
    for (unsigned i = 0; i < 2; ++i) {
      require(history[i].empty() && accepted[i] == retired[i] + dropped[i]);
      std::cout << "HISTORY " << i << ' ' << accepted[i] << ' ' << retired[i]
                << ' ' << dropped[i] << ' ' << history[i].size() << ' '
                << peak[i] << '\n';
    }
    std::cout << "PAIRS " << golden.pairs << " PEAK " << totalPeak
              << " INDEPENDENT " << independentLeft << ' ' << independentRight
              << " BLOCKED " << blockedLeft << ' ' << blockedRight
              << " REPLACEMENT " << replacement << " FULL_RESET " << fullReset
              << '\n';
  }
};
constexpr std::string_view config =
    R"({"deadlock_window":null,"max_domain_cycles":{},"max_ticks":6000,"schema":"pycircuit-model-config","version":"1"})";
void configure(gfsim::SimExecutor &e) {
  require(e.ConfigureJson(reinterpret_cast<const std::uint8_t *>(config.data()),
                          config.size()) == PYCIRCUIT_MODEL_STATUS_V1_OK);
}
void step(gfsim::SimExecutor &e) {
  PycircuitModelStepResultV1 r{sizeof(r)};
  require(e.Step(&r) == PYCIRCUIT_MODEL_STATUS_V1_OK);
}
void drive(pyc_root &r, const Row &row) {
  auto p = ports(row);
  r.pyc_7079635f636c6b = p.pyc_7079635f636c6b;
  r.pyc_7079635f727374 = p.pyc_7079635f727374;
  r.left_valid = p.left_valid;
  r.right_valid = p.right_valid;
  r.left_data = p.left_data;
  r.right_data = p.right_data;
  r.left_take = p.left_take;
  r.right_take = p.right_take;
}
void owner(unsigned workers, bool fail) {
  gfsim::WorkExecutor pool(workers);
  pyc_root r("owner", &pool);
  r.Build();
  Row row{false, false, false, false, false, false, {known(16), known(32)}};
  drive(r, row);
  r.Reset();
  r.Xfer();
  row.clock = true;
  row.lv = row.rv = true;
  row.data = value(90);
  drive(r, row);
  r.Work();
  r.Xfer();
  row.clock = false;
  row.lv = row.rv = false;
  drive(r, row);
  r.Work();
  r.Xfer();
  row.clock = true;
  row.lv = row.rv = true;
  row.data = value(91);
  drive(r, row);
  if (fail)
    r.right_valid = unknown();
  bool failed = false;
  try {
    r.Work();
  } catch (const gfsim::FourStateViolation &) {
    failed = true;
  }
  require(failed == fail);
  r.DiscardNext();
  r.Xfer();
  row.lv = row.rv = false;
  drive(r, row);
  r.Work();
  r.Xfer();
  r.Work();
  auto out = r.result.packed();
  require(out.value().bit(50) && out.value().bit(32));
  same<16>(gfsim::extract<16>(out, 34), value(90).left);
  same<32>(gfsim::extract<32>(out, 0), value(90).right);
  r.Xfer();
  std::cout << "OWNER "
            << (fail ? "late sibling discard/reprepare"
                     : "explicit discard/reprepare")
            << " passed\n";
}
void extensions(unsigned workers) {
  pyc_dut dut(workers);
  gfsim::SimExecutor exec(dut.system(), dut.observations(), {});
  configure(exec);
  Context c{dut, stimulus(true)};
  dut.drive(ports(c.rows[0]));
  require(exec.Reset() == PYCIRCUIT_MODEL_STATUS_V1_OK);
  for (auto &r : c.rows) {
    dut.drive(ports(r));
    step(exec);
    c.observe("FOUR");
  }
  c.finish();
  owner(workers, false);
  owner(workers, true);
  pyc_dut terminal(workers);
  gfsim::SimExecutor bad(terminal.system(), terminal.observations(), {});
  configure(bad);
  Row r{false, false, false, false, false, false, {known(16), known(32)}};
  terminal.drive(ports(r));
  require(bad.Reset() == PYCIRCUIT_MODEL_STATUS_V1_OK);
  step(bad);
  r.clock = true;
  r.lv = r.rv = true;
  r.data = value(100);
  terminal.drive(ports(r));
  step(bad);
  r.clock = false;
  r.lv = r.rv = false;
  terminal.drive(ports(r));
  step(bad);
  const auto epoch = bad.cycles();
  r.clock = true;
  r.lv = r.rv = true;
  r.data = value(101);
  auto p = ports(r);
  p.right_valid = unknown();
  terminal.drive(p);
  PycircuitModelStepResultV1 status{sizeof(status)};
  require(bad.Step(&status) == PYCIRCUIT_MODEL_STATUS_V1_RUNTIME_FAILURE &&
          status.state == PYCIRCUIT_MODEL_STEP_V1_FAILED &&
          bad.cycles() == epoch && terminal.system().cycle() == epoch);
  require(bad.Step(&status) == PYCIRCUIT_MODEL_STATUS_V1_INVALID_STATE);
  bool unavailable = false;
  try {
    (void)terminal.sample();
  } catch (const std::logic_error &) {
    unavailable = true;
  }
  require(unavailable);
  r = {false, false, false, false, false, false, {known(16), known(32)}};
  terminal.drive(ports(r));
  require(bad.Reset() == PYCIRCUIT_MODEL_STATUS_V1_OK);
  step(bad);
  auto e = terminal.sample().result.packed();
  require(e.knownMask() == gfsim::Bits<52>{(std::uint64_t{1} << 52) - 1} &&
          e.zMask() == gfsim::Bits<52>{} &&
          e.value() == gfsim::Bits<52>{(std::uint64_t{1} << 51) |
                                       (std::uint64_t{1} << 33)});
  std::cout << "NEGATIVE late sibling terminal failure, epoch/sample "
               "unchanged, Reset recovery\n";
}
int main(int argc, char **argv) {
  gfsim::SystemRunner runner(argc, argv);
  if (!runner.ready())
    return 2;
  pyc_dut dut(runner.workers());
  Context c{dut, stimulus(false)};
  const gfsim::RunnerCallbacks callbacks{&c, &Context::initialize,
                                         &Context::drive, &Context::sample};
  const int status =
      runner.Run(dut.system(), dut.observations(), {}, callbacks);
  require(status == 0);
  c.finish();
  extensions(runner.workers());
  return status;
}
