#include "gfsim/SimExecutor.h"
#include "gfsim/SystemRunner.h"
#include "pycircuit_system.hpp"
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <map>
#include <set>
#include <source_location>
#include <string>
#include <string_view>
#include <vector>
void require(bool ok,
             std::source_location at = std::source_location::current()) {
  if (!ok) {
    std::cerr << "conditional independent oracle at " << at.line() << '\n';
    std::abort();
  }
}
constexpr std::uint64_t All = (std::uint64_t{1} << 33) - 1;
struct Item {
  std::uint64_t value = 0, known = All, z = 0;
  bool operator==(const Item &) const = default;
};
Item known(std::uint32_t v = 0, bool route = false) {
  return {(std::uint64_t{v} << 1) | route, All, 0};
}
Item transform(Item p, bool falseBranch) {
  bool complete = (p.known >> 1) == 0xffffffffu && (p.z >> 1) == 0;
  return {(complete ? (std::uint64_t{std::uint32_t(std::uint32_t(p.value >> 1) +
                                                   (falseBranch ? 20u : 10u))}
                       << 1)
                    : 0) |
              (p.value & 1),
          (complete ? std::uint64_t{0xffffffffu} << 1 : 0) | (p.known & 1),
          p.z & 1};
}
std::string visible(Item p) {
  std::string s;
  for (int b = 32; b >= 0; --b)
    s += p.z >> b & 1          ? 'z'
         : !(p.known >> b & 1) ? 'x'
         : p.value >> b & 1    ? '1'
                               : '0';
  return s;
}
auto bit(bool v) {
  return gfsim::wire<gfsim::Bits<1>>::known(gfsim::Bits<1>{v});
}
auto wire(Item p) {
  using B = gfsim::Bits<33>;
  return gfsim::wire<B>::fromPacked(
      gfsim::FourState<33>::fromMasks(B{p.value}, B{p.known}, B{p.z}));
}
struct Row {
  bool clock, reset, valid, take;
  Item data;
  std::string tag = "TRAFFIC";
};
struct Token {
  std::uint64_t id;
  Item data;
  std::uint64_t sourceBirth, stageBirth;
};
struct Expected {
  bool ready, valid;
  Item data;
  std::uint64_t id;
  bool sourcePop;
  unsigned selected;
  std::array<bool, 2> fanPop, resPop, fanReady;
};
struct Golden {
  std::array<std::deque<Token>, 6> q;
  bool clock = false;
  std::uint64_t edge = 0, next = 0;
  unsigned priority = 0, overtakes = 0, masked = 0;
  std::set<std::uint64_t> retiredIds;
  std::vector<std::uint64_t> retirement;
  Expected read(const Row &r) const {
    bool mergedReady = q[5].size() < 1 || (!q[5].empty() && r.take);
    bool fv = !q[3].empty(), tv = !q[4].empty();
    std::array<bool, 2> rp{fv && mergedReady, tv && !fv && mergedReady};
    std::array<bool, 2> resReady{q[3].size() < 1 || rp[0],
                                 q[4].size() < 1 || rp[1]};
    std::array<bool, 2> fp{!q[1].empty() && resReady[0],
                           !q[2].empty() && resReady[1]};
    std::array<bool, 2> fr{q[1].size() < 1 || fp[0], q[2].size() < 1 || fp[1]};
    bool move = false;
    unsigned selected = 0;
    if (!q[0].empty()) {
      auto p = q[0].front().data;
      if ((p.known & 1) && (p.z & 1) == 0) {
        selected = (p.value & 1) ? 0 : 1;
        move = fr[selected];
      } else
        require(!fr[0] && !fr[1]);
    }
    return {q[0].size() < 2 || move,
            !q[5].empty(),
            q[5].empty() ? Item{} : q[5].front().data,
            q[5].empty() ? 0 : q[5].front().id,
            move,
            selected,
            fp,
            rp,
            fr};
  }
  unsigned size() const {
    unsigned n = 0;
    for (auto &a : q)
      n += a.size();
    return n;
  }
  void commit(const Row &r) {
    auto e = read(r);
    if (r.clock && !clock) {
      if (r.reset) {
        for (auto &a : q)
          a.clear();
        edge = 0;
      } else {
        bool fv = !q[3].empty(), tv = !q[4].empty();
        if (fv && tv && e.resPop[0])
          ++priority;
        if (!q[0].empty() && !(q[0].front().data.known & 1))
          ++masked;
        if (e.valid && r.take) {
          auto id = q[5].front().id;
          require(retiredIds.insert(id).second);
          for (auto &a : q)
            for (auto &t : a)
              if (t.id < id)
                ++overtakes;
          retirement.push_back(id);
          q[5].pop_front();
        }
        Token chosen;
        if (e.resPop[0])
          chosen = q[3].front();
        else if (e.resPop[1])
          chosen = q[4].front();
        for (unsigned i = 0; i < 2; ++i) {
          if (e.resPop[i])
            q[3 + i].pop_front();
          if (e.fanPop[i]) {
            auto t = q[1 + i].front();
            q[1 + i].pop_front();
            t.data = transform(t.data, i == 0);
            t.stageBirth = edge;
            q[3 + i].push_back(t);
          }
        }
        if (e.resPop[0] || e.resPop[1]) {
          chosen.stageBirth = edge;
          q[5].push_back(chosen);
        }
        if (e.sourcePop) {
          auto t = q[0].front();
          q[0].pop_front();
          t.stageBirth = edge;
          q[1 + e.selected].push_back(t);
        }
        if (r.valid && e.ready)
          q[0].push_back({next++, r.data, edge, edge});
        ++edge;
      }
      require(q[0].size() <= 2);
      for (unsigned i = 1; i < 6; ++i)
        require(q[i].size() <= 1);
      require(size() <= 7);
    }
    clock = r.clock;
  }
};
Item marker(unsigned n, bool route) {
  return known(0x10203040u + n * 0x01010101u, route);
}
std::vector<Item> vectors(bool four) {
  std::vector<Item> a;
  if (!four) {
    for (unsigned length = 0; length <= 32; ++length) {
      auto v = length == 32 ? 0xffffffffu : (std::uint32_t{1} << length) - 1;
      for (bool r : {false, true})
        a.push_back(known(v, r));
    }
    for (std::uint32_t v : {0u, 0xffffffffu, 0xfffffff0u, 0x7fffffffu,
                            0x80000000u, 0x80000001u, 0x55555555u, 0xaaaaaaaau})
      for (bool r : {false, true})
        a.push_back(known(v, r));
  } else {
    for (unsigned b = 0; b < 32; ++b)
      for (bool z : {false, true})
        for (bool latent : {false, true})
          for (bool route : {false, true}) {
            auto m = std::uint64_t{1} << (b + 1);
            auto p = known(0xa55aa55a, route);
            p.value = (p.value & ~m) | (latent ? m : 0);
            p.known ^= m;
            p.z = z ? m : 0;
            a.push_back(p);
          }
    for (unsigned pattern = 0; pattern < 4; ++pattern)
      for (bool latent : {false, true})
        for (bool route : {false, true}) {
          Item p{std::uint64_t(route), 1, 0};
          for (unsigned b = 0; b < 32; ++b) {
            char c = pattern == 0   ? 'x'
                     : pattern == 1 ? 'z'
                     : pattern == 2 ? "xz"[b % 2]
                                    : "01xz"[b % 4];
            p.value |=
                std::uint64_t(c == '1' || ((c == 'x' || c == 'z') && latent))
                << (b + 1);
            p.known |= std::uint64_t(c == '0' || c == '1') << (b + 1);
            p.z |= std::uint64_t(c == 'z') << (b + 1);
          }
          a.push_back(p);
        }
  }
  require(a.size() == (four ? 272u : 82u));
  return a;
}
std::vector<Row> stimulus(bool four) {
  std::vector<Row> a;
  Golden g;
  auto row = [&](bool c, bool reset, bool v, bool t, Item p,
                 std::string tag = "TRAFFIC") {
    Row r{c, reset, v, t, p, tag};
    g.commit(r);
    a.push_back(r);
  };
  auto edge = [&](Item p = Item{}, bool v = false, bool t = true,
                  bool r = false, std::string tag = "TRAFFIC") {
    row(false, r, v, t, p, tag);
    row(true, r, v, t, p, tag);
  };
  row(false, true, false, false, Item{});
  edge({}, false, false, true);
  edge(known(7, false), true, true, false, "E0");
  for (unsigned i = 1; i <= 4; ++i)
    edge({}, false, true, false, "E" + std::to_string(i));
  edge({}, false, true);
  edge({}, false, false, true);
  // Five downstream occupants: else/true/else/true/else. Source becomes empty.
  for (unsigned i = 0; i < 5; ++i)
    edge(marker(i, i % 2 == 0), true, false);
  for (unsigned i = 0; i < 8; ++i)
    edge({}, false, false);
  require(g.q[0].empty() && g.size() == 5);
  // Mixed routes make both results available; priority chooses newer else
  // first.
  row(true, true, true, false, marker(10, true), "HELD");
  row(false, true, true, false, marker(11, false), "FALL");
  for (unsigned i = 0; i < 12; ++i)
    edge({}, false, true, false, "PRIORITY");
  require(g.size() == 0 && g.priority >= 2 && g.overtakes >= 2);
  // Source backlog plus both routes/result/merge fills all seven slots.
  for (unsigned i = 0; i < 7; ++i)
    edge(marker(20 + i, i % 2 == 0), true, false);
  for (unsigned i = 0; i < 6; ++i)
    edge({}, false, false);
  require(g.size() == 7);
  edge(marker(30, false), true, false, false, "REJECT");
  for (unsigned i = 0; i < 40; ++i)
    edge(marker(40 + i, i % 4 == 0 || i % 4 == 3), true, true);
  for (Item p : vectors(four))
    edge(p, true, true);
  for (unsigned i = 0; i < 12; ++i)
    edge({}, false, true);
  require(g.size() == 0);
  // Full reset drops seven complete identities, independently of branch
  // location.
  for (unsigned i = 0; i < 7; ++i)
    edge(marker(90 + i, i % 2 == 0), true, false);
  for (unsigned i = 0; i < 6; ++i)
    edge({}, false, false);
  require(g.size() == 7);
  edge({}, true, true, true);
  for (unsigned i = 0; i < 5; ++i)
    edge({}, false, true);
  if (four) {
    for (unsigned i = 0; i < 5; ++i)
      edge(marker(120 + i, i % 2 == 0), true, false);
    for (unsigned i = 0; i < 8; ++i)
      edge({}, false, false);
    require(g.q[0].empty() && g.size() == 5);
    Item x = known(0x31415926);
    x.known &= ~std::uint64_t{1};
    edge(x, true, false, false, "MASKED_ROUTE");
    for (unsigned i = 0; i < 3; ++i)
      edge({}, false, false, false, "MASKED_ROUTE");
    edge(marker(125, true), true, false, false, "MASKED_ROUTE");
    require(g.size() == 7 && g.masked >= 3);
    edge({}, false, false, true, "MASKED_RESET");
    for (unsigned i = 0; i < 5; ++i)
      edge({}, false, true);
  }
  return a;
}
pyc_dut::Inputs ports(const Row &r) {
  pyc_dut::Inputs p;
  static_assert(decltype(p.data)::width == 33);
  p.pyc_7079635f636c6b = bit(r.clock);
  p.pyc_7079635f727374 = bit(r.reset);
  p.valid = bit(r.valid);
  p.take = bit(r.take);
  p.data = decltype(p.data)::fromPacked(wire(r.data).packed());
  return p;
}
template <class P> void same(const P &p, Item e) {
  auto valueMask = (e.known >> 1) == 0xffffffffu ? All : std::uint64_t{1};
  require((p.value() & gfsim::Bits<33>{valueMask}) ==
          (gfsim::Bits<33>{e.value} & gfsim::Bits<33>{valueMask}));
  require(p.knownMask() == gfsim::Bits<33>{e.known} &&
          p.zMask() == gfsim::Bits<33>{e.z});
}
struct Context {
  pyc_dut &dut;
  std::vector<Row> rows;
  Golden golden;
  std::map<std::uint64_t, Item> live;
  std::uint64_t next = 0;
  unsigned sampled = 0, accepted = 0, retired = 0, dropped = 0, peak = 0,
           resetFull = 0, maskedRows = 0;
  void observe(std::string_view label) {
    auto &r = rows[sampled];
    auto e = golden.read(r);
    auto p = dut.sample().result.packed();
    static_assert(decltype(dut.sample().result)::width == 35);
    require(p.knownMask().bit(34) && p.knownMask().bit(33) &&
            !p.zMask().bit(34) && !p.zMask().bit(33));
    bool ar = p.value().bit(34), av = p.value().bit(33);
    require(ar == e.ready && av == e.valid);
    auto d = gfsim::extract<33>(p, 0);
    same(d, e.data);
    std::cout << label << ' ' << sampled << ' ' << r.tag << ' ' << ar << ' '
              << av << ' ' << visible(e.data) << '\n';
    maskedRows += r.tag == "MASKED_ROUTE";
    if (r.clock && !golden.clock) {
      if (r.reset) {
        resetFull += live.size() == 7;
        dropped += live.size();
        live.clear();
      } else {
        if (av && r.take) {
          require(live.contains(e.id));
          auto raw = live.at(e.id);
          require(raw.known & 1);
          same(d, transform(raw, raw.value & 1));
          live.erase(e.id);
          ++retired;
        }
        if (ar && r.valid) {
          require(live.emplace(next++, r.data).second);
          ++accepted;
        }
        peak = std::max(peak, unsigned(live.size()));
        require(live.size() <= 7);
      }
    }
    golden.commit(r);
    require(live.size() == golden.size());
    for (auto &q : golden.q)
      for (auto &t : q)
        require(live.contains(t.id));
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
    require(sampled == rows.size() && live.empty() &&
            accepted == retired + dropped && peak == 7 && resetFull >= 1 &&
            golden.priority >= 2 && golden.overtakes >= 2);
    std::cout << "HISTORY " << accepted << ' ' << retired << ' ' << dropped
              << ' ' << live.size() << ' ' << peak << ' ' << golden.priority
              << ' ' << golden.overtakes << ' ' << golden.masked << '\n';
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
void drive(pyc_root &r, const Row &x) {
  auto p = ports(x);
  r.pyc_7079635f636c6b = p.pyc_7079635f636c6b;
  r.pyc_7079635f727374 = p.pyc_7079635f727374;
  r.valid = p.valid;
  r.take = p.take;
  r.data = p.data;
}
void owner(unsigned workers) {
  gfsim::WorkExecutor pool(workers);
  pyc_root r("discard", &pool);
  r.Build();
  Row x{false, false, false, false, known(7, false)};
  drive(r, x);
  r.Reset();
  r.Xfer();
  x.clock = true;
  x.valid = true;
  drive(r, x);
  r.Work();
  r.Xfer();
  x.clock = false;
  x.valid = false;
  drive(r, x);
  r.Work();
  r.Xfer();
  x.clock = true;
  drive(r, x);
  r.Work();
  r.DiscardNext();
  r.Xfer();
  r.Work();
  r.Xfer();
  for (unsigned edge = 0; edge < 3; ++edge) {
    x.clock = false;
    drive(r, x);
    r.Work();
    r.Xfer();
    x.clock = true;
    drive(r, x);
    r.Work();
    r.Xfer();
  }
  r.Work();
  auto out = r.result.packed();
  require(out.value().bit(33));
  same(gfsim::extract<33>(out, 0), known(17, false));
  r.Xfer();
  unsigned retired = 0;
  x.take = true;
  for (unsigned edge = 0; edge < 6; ++edge) {
    x.clock = false;
    drive(r, x);
    r.Work();
    r.Xfer();
    x.clock = true;
    drive(r, x);
    r.Work();
    auto observed = r.result.packed();
    if (observed.value().bit(33)) {
      same(gfsim::extract<33>(observed, 0), known(17, false));
      ++retired;
    }
    r.Xfer();
  }
  require(retired == 1);
  std::cout << "OWNER discard/reprepare and held observation passed\n";
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
  owner(workers);
  for (unsigned mode = 0; mode < 2; ++mode) {
    pyc_dut bad(workers);
    gfsim::SimExecutor terminal(bad.system(), bad.observations(), {});
    configure(terminal);
    Row r{false, false, false, false, Item{}};
    bad.drive(ports(r));
    require(terminal.Reset() == PYCIRCUIT_MODEL_STATUS_V1_OK);
    step(terminal);
    if (mode) {
      for (unsigned n = 0; n < 5; ++n) {
        r = {false, false, true, false, marker(200 + n, n % 2 == 0)};
        bad.drive(ports(r));
        step(terminal);
        r.clock = true;
        bad.drive(ports(r));
        step(terminal);
      }
      for (unsigned n = 0; n < 8; ++n) {
        r = {false, false, false, false, {}};
        bad.drive(ports(r));
        step(terminal);
        r.clock = true;
        bad.drive(ports(r));
        step(terminal);
      }
    }
    Item x = known(0x31415926);
    x.known &= ~std::uint64_t{1};
    r = {false, false, true, false, x};
    bad.drive(ports(r));
    step(terminal);
    r.clock = true;
    bad.drive(ports(r));
    step(terminal);
    r = {false, false, false, false, {}};
    bad.drive(ports(r));
    step(terminal);
    if (mode) {
      for (unsigned n = 0; n < 2; ++n) {
        r.clock = true;
        bad.drive(ports(r));
        step(terminal);
        r.clock = false;
        bad.drive(ports(r));
        step(terminal);
      }
    }
    const auto epoch = terminal.cycles();
    r.clock = true;
    r.take = mode;
    bad.drive(ports(r));
    PycircuitModelStepResultV1 status{sizeof(status)};
    require(terminal.Step(&status) ==
                PYCIRCUIT_MODEL_STATUS_V1_RUNTIME_FAILURE &&
            status.state == PYCIRCUIT_MODEL_STEP_V1_FAILED &&
            terminal.cycles() == epoch);
    require(terminal.Step(&status) == PYCIRCUIT_MODEL_STATUS_V1_INVALID_STATE);
    bool unavailable = false;
    try {
      (void)bad.sample();
    } catch (const std::logic_error &) {
      unavailable = true;
    }
    require(unavailable);
    r = {false, false, false, false, {}};
    bad.drive(ports(r));
    require(terminal.Reset() == PYCIRCUIT_MODEL_STATUS_V1_OK);
    step(terminal);
    auto empty = bad.sample().result.packed();
    require(empty.knownMask() ==
                gfsim::Bits<35>{(std::uint64_t{1} << 35) - 1} &&
            empty.zMask() == gfsim::Bits<35>{} &&
            empty.value() == gfsim::Bits<35>{std::uint64_t{1} << 34});
    std::cout << "NEGATIVE " << mode
              << " effective unknown route terminal; Reset recovery\n";
  }
}
int main(int argc, char **argv) {
  gfsim::SystemRunner runner(argc, argv);
  if (!runner.ready())
    return 2;
  pyc_dut dut(runner.workers());
  Context c{dut, stimulus(false)};
  const gfsim::RunnerCallbacks callbacks{&c, &Context::initialize,
                                         &Context::drive, &Context::sample};
  int status = runner.Run(dut.system(), dut.observations(), {}, callbacks);
  require(status == 0);
  c.finish();
  extensions(runner.workers());
  return status;
}
