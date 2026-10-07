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
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
void require(bool ok,
             std::source_location at = std::source_location::current()) {
  if (!ok) {
    std::cerr << "table_rule independent oracle at " << at.line() << '\n';
    std::abort();
  }
}
struct Entry {
  unsigned value = 0, known = 255, z = 0, strict = 255;
  bool operator==(const Entry &) const = default;
};
Entry entry(unsigned index = 0, unsigned value = 0) {
  return {(index << 7) | value, 255, 0, 255};
}
std::string visible(Entry p) {
  std::string s;
  for (unsigned b = 8; b; --b)
    s += (p.z >> (b - 1)) & 1          ? 'z'
         : !((p.known >> (b - 1)) & 1) ? 'x'
         : (p.value >> (b - 1)) & 1    ? '1'
                                       : '0';
  return s;
}
// Per-bit conditional truth table. Unknown destination decodes X for each
// row; this is current table-map semantics, not a legacy bounds assertion.
Entry merge(Entry old, Entry replacement) {
  Entry out{0, 0, 0, 0};
  for (unsigned b = 0; b < 8; ++b) {
    auto m = 1u << b;
    if ((old.known & m) && (replacement.known & m) &&
        !((old.value ^ replacement.value) & m)) {
      out.known |= m;
      out.value |= replacement.value & m;
      out.strict |= m;
    } else if ((old.z & m) && (replacement.z & m))
      out.z |= m;
  }
  return out;
}
bool knownIndex(Entry p) { return (p.known & 128) && !(p.z & 128); }
Entry read(const std::array<Entry, 2> &rows, Entry p) {
  return knownIndex(p) ? rows[p.value >> 7] : Entry{0, 0, 0, 0};
}
void install(std::array<Entry, 2> &rows, Entry p) {
  if (knownIndex(p))
    rows[p.value >> 7] = p;
  else
    for (auto &row : rows)
      row = merge(row, p);
}
template <unsigned W> auto wire(unsigned value, unsigned known, unsigned z) {
  return gfsim::wire<gfsim::Bits<W>>::fromPacked(gfsim::FourState<W>::fromMasks(
      gfsim::Bits<W>{value}, gfsim::Bits<W>{known}, gfsim::Bits<W>{z}));
}
auto bit(bool b) { return wire<1>(b, 1, 0); }
template <class P> void same(const P &p, Entry e) {
  require((p.value() & gfsim::Bits<8>{e.strict}) ==
          gfsim::Bits<8>{e.value & e.strict});
  require(p.knownMask() == gfsim::Bits<8>{e.known} &&
          p.zMask() == gfsim::Bits<8>{e.z});
}
struct Case {
  Entry data;
  bool commonZ = false;
};
std::vector<Case> vectors(bool four) {
  std::vector<Case> a;
  if (!four) {
    for (unsigned value = 0; value < 256; ++value)
      a.push_back({Entry{value}});
  } else {
    for (unsigned index = 0; index < 2; ++index)
      for (unsigned b = 0; b < 7; ++b)
        for (bool z : {false, true})
          for (bool latent : {false, true}) {
            auto p = entry(index, 85);
            auto m = 1u << b;
            p.value = (p.value & ~m) | (latent ? m : 0);
            p.known ^= m;
            p.z = z ? m : 0;
            a.push_back({p});
          }
    for (unsigned index = 0; index < 2; ++index)
      for (unsigned pattern = 0; pattern < 4; ++pattern)
        for (bool latent : {false, true}) {
          Entry p = entry(index);
          for (unsigned b = 0; b < 7; ++b) {
            char c = pattern == 0   ? 'x'
                     : pattern == 1 ? 'z'
                     : pattern == 2 ? "xz"[b % 2]
                                    : "01xz"[b % 4];
            auto m = 1u << b;
            p.value |= (c == '1' || ((c == 'x' || c == 'z') && latent)) ? m : 0;
            if (c == 'x' || c == 'z')
              p.known &= ~m;
            if (c == 'z')
              p.z |= m;
          }
          a.push_back({p});
        }
    require(a.size() == 72);
    for (bool z : {false, true})
      for (bool latent : {false, true})
        for (unsigned value : {0u, 127u, 42u, 85u})
          a.push_back({Entry{value | (latent ? 128u : 0), 127, z ? 128u : 0}});
    for (bool z : {false, true})
      for (bool latent : {false, true})
        for (unsigned pattern = 0; pattern < 4; ++pattern) {
          Entry p{latent ? 128u : 0, 0, z ? 128u : 0};
          for (unsigned b = 0; b < 7; ++b) {
            char c = pattern == 0   ? 'x'
                     : pattern == 1 ? 'z'
                     : pattern == 2 ? "xz"[b % 2]
                                    : "01xz"[b % 4];
            auto m = 1u << b;
            p.value |= (c == '1' || ((c == 'x' || c == 'z') && latent)) ? m : 0;
            if (c == '0' || c == '1')
              p.known |= m;
            if (c == 'z')
              p.z |= m;
          }
          a.push_back({p});
        }
    for (bool z : {false, true})
      for (bool latent : {false, true})
        a.push_back({Entry{latent ? 255u : 0, 0, 127 | (z ? 128u : 0)}, true});
    require(a.size() == 108);
  }
  return a;
}
struct Row {
  bool clock = false, reset = false, valid = false, take = false;
  Entry data;
};
struct Token {
  unsigned id = 0;
  Entry data;
  std::uint64_t birth = 0;
};
struct Expected {
  bool ready, valid, pop, move;
  Entry data;
};
struct Golden {
  std::array<std::deque<Token>, 2> q;
  std::array<Entry, 2> rows{};
  bool clock = false;
  unsigned next = 0;
  std::uint64_t edge = 0;
  unsigned size() const { return q[0].size() + q[1].size(); }
  Expected read(const Row &r) const {
    bool pop = !q[1].empty() && r.take, room = q[1].empty() || pop,
         move = !q[0].empty() && room;
    return {q[0].size() < 2 || move, !q[1].empty(), pop, move,
            q[1].empty() ? entry() : q[1].front().data};
  }
  void commit(const Row &r) {
    auto e = read(r);
    if (r.clock && !clock) {
      if (r.reset) {
        for (auto &a : q)
          a.clear();
        rows = {};
        edge = 0;
      } else {
        if (e.pop)
          q[1].pop_front();
        if (e.move) {
          auto t = q[0].front();
          q[0].pop_front();
          auto replacement = t.data;
          t.data = ::read(rows, replacement);
          install(rows, replacement);
          q[1].push_back(t);
        }
        if (r.valid && e.ready)
          q[0].push_back({next++, r.data, edge});
        ++edge;
      }
      require(q[0].size() <= 2 && q[1].size() <= 1 && size() <= 3);
    }
    clock = r.clock;
  }
};
std::vector<Row> stimulus(bool four) {
  std::vector<Row> a;
  Golden g;
  auto row = [&](Row r) {
    g.commit(r);
    a.push_back(r);
  };
  auto edge = [&](Entry p = entry(), bool v = false, bool t = true,
                  bool r = false) {
    Row x{false, r, v, t, p};
    row(x);
    x.clock = true;
    row(x);
  };
  auto drain = [&] {
    for (unsigned i = 0; i < 8; ++i)
      edge();
    require(g.size() == 0);
  };
  row({false, true});
  edge(entry(), false, false, true);
  edge(entry(0, 10), true);
  edge(entry(0, 20), true);
  edge(entry(1, 30), true);
  edge(entry(0, 40), true);
  drain();
  auto fill = [&] {
    edge(entry(0, 50), true, false);
    edge(entry(1, 60), true, false);
    edge(entry(0, 70), true, false);
    require(g.size() == 3);
  };
  fill();
  edge(entry(1, 80), true, false);
  row({true, true, true, true, entry(1, 90)});
  row({true, false, true, false, entry(0, 100)});
  edge(entry(1, 110), true, false);
  for (unsigned i = 0; i < 12; ++i)
    edge(entry(i % 2, (i * 11) % 128), true, true);
  drain();
  fill();
  edge(entry(), false, true, true);
  drain();
  for (const auto &c : vectors(four)) {
    if (!knownIndex(c.data)) {
      Entry a0 = entry(0, 42), a1 = entry(1, 85);
      if (c.commonZ) {
        a0 = {c.data.value & 127, 128, 127};
        a1 = {128 | (c.data.value & 127), 128, 127};
      }
      edge(a0, true);
      edge(a1, true);
      edge();
      edge();
      require(g.size() == 0);
    }
    unsigned index = knownIndex(c.data) ? c.data.value >> 7 : 0;
    edge(c.data, true);
    edge(entry(index, 3), true);
    edge(entry(index ^ 1, 5), true);
    edge();
    edge();
    require(g.size() == 0);
  }
  if (four)
    for (bool z : {false, true}) {
      edge(entry(0, 10), true);
      edge(entry(1, 20), true);
      drain();
      edge(entry(0, 30), true, false);
      edge(Entry{55, 127, z ? 128u : 0}, true, false);
      edge(entry(1, 99), true, false);
      require(g.size() == 3);
      edge(entry(0, 11), true, false);
      edge(entry(1, 12), true, false);
      row({true, false, true, false, entry(0, 13)});
      for (unsigned i = 0; i < 4; ++i)
        edge();
      edge(entry(0, 7), true);
      edge(entry(1, 8), true);
      drain();
    }
  fill();
  edge(entry(), true, true, true);
  drain();
  return a;
}
pyc_dut::Inputs ports(const Row &r) {
  pyc_dut::Inputs p;
  static_assert(decltype(p.data)::width == 8);
  p.pyc_7079635f636c6b = bit(r.clock);
  p.pyc_7079635f727374 = bit(r.reset);
  p.valid = bit(r.valid);
  p.take = bit(r.take);
  p.data = decltype(p.data)::fromPacked(
      wire<8>(r.data.value, r.data.known, r.data.z).packed());
  return p;
}
template <class P> void check(const P &p, const Expected &e) {
  require(p.knownMask().bit(9) && p.knownMask().bit(8) && !p.zMask().bit(9) &&
          !p.zMask().bit(8));
  require(p.value().bit(9) == e.ready && p.value().bit(8) == e.valid);
  same(gfsim::extract<8>(p, 0), e.data);
}
struct Context {
  pyc_dut &dut;
  std::vector<Row> rows;
  Golden g;
  std::array<Entry, 2> images{};
  std::deque<Token> admitted, results;
  unsigned sampled = 0, accepted = 0, retired = 0, dropped = 0, peak = 0,
           blocked = 0, replacements = 0, fullReset = 0;
  void observe(std::string_view label) {
    const auto &r = rows[sampled];
    auto e = g.read(r);
    auto out = dut.sample().result.packed();
    static_assert(decltype(dut.sample().result)::width == 10);
    check(out, e);
    std::cout << label << ' ' << sampled << ' ' << out.value().bit(9) << ' '
              << out.value().bit(8) << ' ' << visible(e.data) << '\n';
    if (r.clock && !g.clock) {
      if (r.reset) {
        fullReset += g.size() == 3;
        dropped += admitted.size() + results.size();
        admitted.clear();
        results.clear();
        images = {};
      } else {
        blocked += !g.q[1].empty() && !r.take;
        replacements += e.move && g.q[0].size() == 2 && !g.q[1].empty() &&
                        r.take && r.valid;
        if (out.value().bit(8) && r.take) {
          require(!results.empty());
          same(gfsim::extract<8>(out, 0), results.front().data);
          require(g.edge >= results.front().birth + 2);
          results.pop_front();
          ++retired;
        }
        // Install timing is inferred by old slots. Row contents are
        // independently checked by later known-index public transactions, never
        // private pokes.
        if (e.move) {
          require(!admitted.empty());
          auto t = admitted.front();
          admitted.pop_front();
          auto replacement = t.data;
          t.data = ::read(images, replacement);
          install(images, replacement);
          results.push_back(t);
        }
        if (out.value().bit(9) && r.valid)
          admitted.push_back({accepted++, r.data, g.edge});
      }
    }
    g.commit(r);
    peak = std::max(peak, g.size());
    require(admitted.size() == g.q[0].size() &&
            results.size() == g.q[1].size() && images == g.rows);
    ++sampled;
  }
  static void initialize(void *p) { require(drive(p, 0)); }
  static bool drive(void *p, std::uint64_t epoch) {
    auto &c = *static_cast<Context *>(p);
    if (epoch == c.rows.size())
      return false;
    require(epoch < c.rows.size());
    c.dut.drive(ports(c.rows[epoch]));
    return true;
  }
  static void sample(void *p, std::uint64_t epoch) {
    auto &c = *static_cast<Context *>(p);
    require(epoch == c.sampled + 1);
    c.observe("WORK");
  }
  void finish() {
    require(sampled == rows.size() && g.size() == 0 &&
            accepted == retired + dropped && peak == 3 && fullReset >= 2 &&
            blocked >= 2 && replacements >= 8);
    std::cout << "HISTORY " << accepted << ' ' << retired << ' ' << dropped
              << ' ' << peak << ' ' << blocked << ' ' << replacements << ' '
              << fullReset << '\n';
  }
};
constexpr std::string_view config =
    R"({"deadlock_window":null,"max_domain_cycles":{},"max_ticks":6000,"schema":"pycircuit-model-config","version":"1"})";
void configure(gfsim::SimExecutor &e) {
  require(e.ConfigureJson(reinterpret_cast<const std::uint8_t *>(config.data()),
                          config.size()) == PYCIRCUIT_MODEL_STATUS_V1_OK);
}
void driveRoot(pyc_root &root, const Row &r) {
  auto p = ports(r);
  root.pyc_7079635f636c6b = p.pyc_7079635f636c6b;
  root.pyc_7079635f727374 = p.pyc_7079635f727374;
  root.valid = p.valid;
  root.data = p.data;
  root.take = p.take;
}
void owner(unsigned workers, bool fail) {
  gfsim::WorkExecutor pool(workers);
  pyc_root root("owner", &pool);
  root.Build();
  Golden g;
  Row r;
  driveRoot(root, r);
  root.Reset();
  root.Xfer();
  std::vector<Entry> observed;
  auto step = [&](Row x) {
    driveRoot(root, x);
    root.Work();
    auto e = g.read(x);
    check(root.result.packed(), e);
    if (x.clock && !g.clock && x.take && e.valid)
      observed.push_back(e.data);
    root.Xfer();
    g.commit(x);
  };
  auto edge = [&](Row x) {
    x.clock = false;
    step(x);
    x.clock = true;
    step(x);
  };
  edge({false, false, true, true, entry(0, 10)});
  edge({false, false, true, true, entry(1, 20)});
  edge({false, false, false, true});
  edge({false, false, false, true});
  observed.clear();
  edge({false, false, true, false, entry(0, 30)});
  edge({false, false, true, false, entry(1, 40)});
  r = {false, false, true, true, entry(0, 50)};
  step(r);
  r.clock = true;
  driveRoot(root, r);
  if (fail)
    root.valid = wire<1>(0, 0, 0);
  bool failed = false;
  try {
    root.Work();
  } catch (const gfsim::FourStateViolation &) {
    failed = true;
  }
  require(failed == fail);
  root.DiscardNext();
  root.Xfer();
  r.valid = false;
  step(r);
  for (unsigned i = 0; i < 4; ++i)
    edge({false, false, false, true});
  require(observed == std::vector<Entry>{entry(0, 10), entry(1, 20)});
  observed.clear();
  // Both table rows are queried after the rejected activation. A partial table
  // install would corrupt these saved old-Q snapshots on retry or row query.
  edge({false, false, true, true, entry(0, 60)});
  edge({false, false, true, true, entry(1, 70)});
  for (unsigned i = 0; i < 3; ++i)
    edge({false, false, false, true});
  require(observed == std::vector<Entry>{entry(0, 30), entry(1, 40)});
  observed.clear();
  edge({false, false, true, false, entry(0, 80)});
  root.Reset();
  root.DiscardNext();
  root.Xfer();
  for (unsigned i = 0; i < 4; ++i)
    edge({false, false, false, true});
  require(observed == std::vector<Entry>{entry(0, 60)});
  observed.clear();
  edge({false, false, true, true, entry(0, 90)});
  edge({false, false, true, true, entry(1, 100)});
  for (unsigned i = 0; i < 3; ++i)
    edge({false, false, false, true});
  require(observed == std::vector<Entry>{entry(0, 80), entry(1, 70)});
  observed.clear();
  root.Reset();
  root.Xfer();
  g = Golden{};
  step({});
  edge({false, false, true, true, entry(0, 1)});
  edge({false, false, true, true, entry(1, 2)});
  for (unsigned i = 0; i < 3; ++i)
    edge({false, false, false, true});
  require(observed == std::vector<Entry>{entry(), entry()});
  std::cout << "OWNER " << fail
            << " atomic queues/table; both-row discard/reset queries passed\n";
}
void terminal(unsigned workers) {
  for (bool pop : {false, true}) {
    pyc_dut dut(workers);
    gfsim::SimExecutor e(dut.system(), dut.observations(), {});
    configure(e);
    auto step = [&](Row x) {
      dut.drive(ports(x));
      PycircuitModelStepResultV1 s{sizeof(s)};
      require(e.Step(&s) == PYCIRCUIT_MODEL_STATUS_V1_OK);
    };
    dut.drive(ports({}));
    require(e.Reset() == PYCIRCUIT_MODEL_STATUS_V1_OK);
    step({});
    if (pop) {
      step({true, false, true, false, entry(1, 7)});
      step({false, false, false, false});
      step({true, false, false, false});
      step({false, false, false, false});
    }
    auto p = ports({true, false, false, pop});
    if (pop)
      p.take = wire<1>(0, 0, 0);
    else
      p.valid = wire<1>(0, 0, 0);
    dut.drive(p);
    auto epoch = e.cycles();
    PycircuitModelStepResultV1 s{sizeof(s)};
    require(e.Step(&s) == PYCIRCUIT_MODEL_STATUS_V1_RUNTIME_FAILURE &&
            s.state == PYCIRCUIT_MODEL_STEP_V1_FAILED && e.cycles() == epoch &&
            dut.system().cycle() == epoch);
    require(e.Step(&s) == PYCIRCUIT_MODEL_STATUS_V1_INVALID_STATE);
    bool unavailable = false;
    try {
      (void)dut.sample();
    } catch (const std::logic_error &) {
      unavailable = true;
    }
    require(unavailable);
    dut.drive(ports({}));
    require(e.Reset() == PYCIRCUIT_MODEL_STATUS_V1_OK);
    step({});
    step({true, false, true, true, entry(1, 9)});
    step({false, false, false, true});
    step({true, false, false, true});
    step({false, false, false, true});
    require(dut.sample().result.packed().value().bit(8));
    same(gfsim::extract<8>(dut.sample().result.packed(), 0), entry());
    step({true, false, false, true});
    step({false, false, false, true});
    require(!dut.sample().result.packed().value().bit(8));
    std::cout << "NEGATIVE protocol " << pop
              << " terminal; empty table Reset execution recovery\n";
  }
}
int main(int argc, char **argv) {
  std::string mode;
  std::vector<char *> args{argv[0]};
  for (int i = 1; i < argc; ++i)
    if (std::string_view(argv[i]) == "--probe-mode") {
      require(i + 1 < argc && mode.empty());
      mode = argv[++i];
    } else
      args.push_back(argv[i]);
  args.push_back(nullptr);
  gfsim::SystemRunner runner(static_cast<int>(args.size()) - 1, args.data());
  if (!runner.ready())
    return 2;
  if (!mode.empty()) {
    require(mode == "terminal");
    terminal(runner.workers());
    return 0;
  }
  pyc_dut dut(runner.workers());
  Context c{dut, stimulus(false)};
  const gfsim::RunnerCallbacks callbacks{&c, &Context::initialize,
                                         &Context::drive, &Context::sample};
  int status = runner.Run(dut.system(), dut.observations(), {}, callbacks);
  require(status == 0);
  c.finish();
  pyc_dut four(runner.workers());
  gfsim::SimExecutor e(four.system(), four.observations(), {});
  configure(e);
  Context f{four, stimulus(true)};
  four.drive(ports(f.rows[0]));
  require(e.Reset() == PYCIRCUIT_MODEL_STATUS_V1_OK);
  for (const auto &r : f.rows) {
    four.drive(ports(r));
    PycircuitModelStepResultV1 s{sizeof(s)};
    require(e.Step(&s) == PYCIRCUIT_MODEL_STATUS_V1_OK);
    f.observe("FOUR");
  }
  f.finish();
  owner(runner.workers(), false);
  owner(runner.workers(), true);
  return status;
}
