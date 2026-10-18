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
    std::cerr << "broadcast independent oracle line " << at.line() << '\n';
    std::abort();
  }
}
struct Planes {
  std::string value, known, z;
  bool operator==(const Planes &) const = default;
};
Planes token(std::string_view symbols, bool latent = false) {
  Planes p;
  for (char c : symbols) {
    require(c == '0' || c == '1' || c == 'x' || c == 'z');
    p.value += c == '1' || ((c == 'x' || c == 'z') && latent) ? '1' : '0';
    p.known += c == '0' || c == '1' ? '1' : '0';
    p.z += c == 'z' ? '1' : '0';
  }
  return p;
}
Planes zero() { return token(std::string(64, '0')); }
std::string binary(std::uint64_t value) {
  std::string s;
  for (unsigned b = 64; b; --b)
    s += (value >> (b - 1)) & 1 ? '1' : '0';
  return s;
}
std::string visible(const Planes &p) {
  std::string s;
  for (unsigned i = 0; i < 64; ++i)
    s += p.z[i] == '1' ? 'z' : p.known[i] == '0' ? 'x' : p.value[i];
  return s;
}
template <unsigned W> gfsim::Bits<W> bits(std::string_view s) {
  require(s.size() == W);
  gfsim::Bits<W> b{0};
  for (unsigned i = 0; i < W; ++i)
    if (s[W - 1 - i] == '1')
      b.setWord(i / 64, b.word(i / 64) | (std::uint64_t{1} << (i % 64)));
  return b;
}
auto wire(const Planes &p) {
  return gfsim::wire<gfsim::Bits<64>>::fromPacked(
      gfsim::FourState<64>::fromMasks(bits<64>(p.value), bits<64>(p.known),
                                      bits<64>(p.z)));
}
template <unsigned W> auto known(unsigned n) {
  return gfsim::wire<gfsim::Bits<W>>::known(gfsim::Bits<W>{n});
}
Planes increment(const Planes &p, unsigned delta) {
  Planes result = zero();
  const bool known =
      p.known == std::string(64, '1') && p.z == std::string(64, '0');
  unsigned carry = delta;
  for (unsigned b = 0; b < 64; ++b) {
    const unsigned i = 63 - b, sum = (p.value[i] == '1') + (carry & 1);
    result.value[i] = known && (sum & 1) ? '1' : '0';
    result.known[i] = known ? '1' : '0';
    carry = (carry >> 1) + (sum >> 1);
  }
  return result;
}
std::vector<Planes> vectors(bool four) {
  std::vector<Planes> result;
  if (!four) {
    for (unsigned b = 0; b <= 64; ++b) {
      auto c = b == 64 ? ~std::uint64_t{0} : (std::uint64_t{1} << b) - 1;
      result.push_back(token(binary(c)));
    }
    for (unsigned b = 0; b <= 64; ++b)
      result.push_back(token(binary(b == 64 ? 0 : std::uint64_t{1} << b)));
    for (auto x : {std::uint64_t{0}, ~std::uint64_t{0}, ~std::uint64_t{1},
                   std::uint64_t{0x7fffffffffffffffULL},
                   std::uint64_t{0x8000000000000000ULL},
                   std::uint64_t{0x8000000000000001ULL},
                   std::uint64_t{0x5555555555555555ULL},
                   std::uint64_t{0xaaaaaaaaaaaaaaaaULL}})
      result.push_back(token(binary(x)));
    std::uint64_t random = 0xd2106421;
    for (unsigned i = 0; i < 64; ++i) {
      random = random * 6364136223846793005ULL + 1442695040888963407ULL;
      result.push_back(token(binary(random)));
    }
    for (unsigned b = 0; b <= 64; ++b)
      result.push_back(token(binary(b == 64  ? ~std::uint64_t{1}
                                    : b == 0 ? 0
                                             : (std::uint64_t{1} << b) - 2)));
  } else {
    for (unsigned b = 0; b < 64; ++b)
      for (char symbol : {'x', 'z'})
        for (bool latent : {false, true}) {
          auto s = binary(0xa5963c69f00f5a96ULL);
          s[63 - b] = symbol;
          result.push_back(token(s, latent));
        }
    for (unsigned pattern = 0; pattern < 4; ++pattern)
      for (bool latent : {false, true}) {
        std::string s;
        for (unsigned b = 0; b < 64; ++b)
          s += pattern == 0   ? 'x'
               : pattern == 1 ? 'z'
               : pattern == 2 ? "xz"[b % 2]
                              : "01xz"[b % 4];
        result.push_back(token(s, latent));
      }
  }
  require(result.size() == (four ? 264u : 267u));
  return result;
}
struct Row {
  unsigned clock, reset, valid;
  Planes data;
  unsigned take;
  bool mustAccept = false;
};
std::vector<Row> stimulus(bool four) {
  std::vector<Row> rows;
  const auto values = vectors(four);
  auto add = [&](unsigned c, unsigned r, unsigned v, const Planes &p,
                 unsigned t, bool mustAccept = false) {
    rows.push_back({c, r, v, p, t, mustAccept});
  };
  auto edge = [&](const Planes &p, unsigned v = 1, unsigned t = 1,
                  unsigned r = 0, bool mustAccept = false) {
    add(1, r, v, p, t, mustAccept);
    add(0, r, v, p, t, mustAccept);
  };
  auto marker = [&](unsigned serial) {
    return token(binary(0x1000000000000000ULL + serial * 16));
  };
  auto fill = [&] {
    edge(marker(0), 1, 0);
    for (unsigned i = 0; i < 4; ++i)
      edge(zero(), 0, 0);
    for (unsigned i = 0; i < 8; ++i)
      edge(marker(i + 1), 1, 0);
  };
  add(0, 1, 0, zero(), 0);
  edge(zero(), 0, 0, 1);
  edge(token(std::string(64, '1')));
  for (unsigned i = 0; i < 7; ++i)
    edge(zero(), 0);
  fill();
  add(1, 0, 1, marker(20), 0);
  add(1, 1, 1, marker(21), 1);
  add(0, 0, 1, marker(22), 0);
  add(0, 0, 0, marker(23), 1);
  edge(zero(), 1, 1,
       1); // All eight slots; source identities each owe two copies.
  fill();
  for (unsigned i = 0; i < 16; ++i)
    edge(marker(i + 30)); // Reachable retired-RTL duplication counterexample.
  for (unsigned i = 0; i < 32; ++i)
    edge(zero(), 0);
  // Space offers so each carry/X/Z case is accepted exactly once, even though
  // every source identity owes two sink results.
  for (const auto &p : values) {
    edge(p, 1, 1, 0, true);
    edge(zero(), 0);
    edge(zero(), 0);
  }
  for (unsigned i = 0; i < 32; ++i)
    edge(zero(), 0);
  fill();
  edge(zero(), 1, 1, 1); // Second complete eight-slot reset.
  edge(token(std::string(64, '1')));
  for (unsigned i = 0; i < 8; ++i)
    edge(zero(), 0);
  return rows;
}
struct Item {
  unsigned uid, branch;
  Planes data;
  std::int64_t birth;
};
struct Decision {
  bool sink, mergeRoom, leftGrant, rightGrant, leftMove, rightMove, leftRoom,
      rightRoom, fan, ready;
};
class Golden {
public:
  std::array<std::deque<Item>, 6> q;
  bool clock = false;
  std::int64_t edge = -2;
  unsigned nextId = 0, asymmetric = 0, atomicBirths = 0, replacements = 0;
  std::array<std::int64_t, 8> first{-99, -99, -99, -99, -99, -99, -99, -99};
  Decision decision(unsigned take) const {
    Decision d;
    d.sink = !q[5].empty() && take;
    d.mergeRoom = q[5].size() < 2 || d.sink;
    d.leftGrant = !q[3].empty() && d.mergeRoom;
    d.rightGrant = q[3].empty() && !q[4].empty() && d.mergeRoom;
    d.leftMove = !q[1].empty() && (q[3].empty() || d.leftGrant);
    d.rightMove = !q[2].empty() && (q[4].empty() || d.rightGrant);
    d.leftRoom = q[1].empty() || d.leftMove;
    d.rightRoom = q[2].empty() || d.rightMove;
    d.fan = !q[0].empty() && d.leftRoom && d.rightRoom;
    d.ready = q[0].size() < 2 || d.fan;
    return d;
  }
  unsigned physical() const {
    unsigned n = 0;
    for (const auto &lane : q)
      n += lane.size();
    return n;
  }
  void reset() {
    for (auto &lane : q)
      lane.clear();
  }
  void commit(const Row &r) {
    if (r.clock && !clock) {
      const auto d = decision(r.take);
      std::array<Item, 6> head;
      for (unsigned i = 0; i < 6; ++i)
        head[i] = q[i].empty() ? Item{0, 0, zero(), -99} : q[i].front();
      ++edge;
      require(edge < 2000);
      if (r.reset)
        reset();
      else {
        const std::array<bool, 6> pop{d.fan,       d.leftMove,   d.rightMove,
                                      d.leftGrant, d.rightGrant, d.sink};
        for (unsigned i = 0; i < 6; ++i)
          if (pop[i]) {
            require(head[i].birth < edge);
            q[i].pop_front();
          }
        asymmetric += !q[0].empty() && (d.leftRoom != d.rightRoom);
        if (d.fan) {
          ++atomicBirths;
          auto l = head[0], rr = head[0];
          l.branch = 0;
          rr.branch = 1;
          l.birth = rr.birth = edge;
          q[1].push_back(l);
          q[2].push_back(rr);
          if (l.uid == 0)
            first[1] = edge;
        }
        if (d.leftMove) {
          auto item = head[1];
          item.data = increment(item.data, 1);
          item.birth = edge;
          q[3].push_back(item);
          if (item.uid == 0)
            first[2] = edge;
        }
        if (d.rightMove) {
          auto item = head[2];
          item.data = increment(item.data, 2);
          item.birth = edge;
          q[4].push_back(item);
          if (item.uid == 0)
            first[3] = edge;
        }
        if (d.leftGrant || d.rightGrant) {
          auto item = head[d.leftGrant ? 3 : 4];
          item.birth = edge;
          q[5].push_back(item);
          if (item.uid == 0)
            first[4 + item.branch] = edge;
        }
        if (d.sink && head[5].uid == 0)
          first[6 + head[5].branch] = edge;
        if (d.ready && r.valid) {
          replacements += d.fan && q[0].size() == 1;
          q[0].push_back({nextId++, 0, r.data, edge});
          if (nextId == 1)
            first[0] = edge;
        }
        const unsigned depth[] = {2, 1, 1, 1, 1, 2};
        for (unsigned i = 0; i < 6; ++i)
          require(q[i].size() <= depth[i]);
      }
    }
    clock = r.clock;
  }
};
pyc_dut::Inputs inputs(const Row &r) {
  pyc_dut::Inputs p;
  p.pyc_7079635f636c6b = known<1>(r.clock);
  p.pyc_7079635f727374 = known<1>(r.reset);
  p.valid = known<1>(r.valid);
  p.take = known<1>(r.take);
  p.data = wire(r.data);
  return p;
}
template <class Packed> void payload(const Packed &p, const Planes &e) {
  // Incremented X has exact known/Z masks; its hidden computed value is
  // unasserted.
  const auto mask = bits<64>(e.known);
  require((p.value() & mask) == (bits<64>(e.value) & mask));
  require(p.knownMask() == mask);
  require(p.zMask() == bits<64>(e.z));
}
template <class Packed>
void check(const Packed &p, const Golden &g, unsigned take) {
  for (auto b : {65u, 64u})
    require(p.knownMask().bit(b) && !p.zMask().bit(b));
  require(p.value().bit(65) == g.decision(take).ready &&
          p.value().bit(64) == !g.q[5].empty());
  payload(gfsim::extract<64>(p, 0),
          g.q[5].empty() ? zero() : g.q[5].front().data);
}
struct Obligation {
  Planes input;
  std::array<bool, 2> owed{true, true};
};
struct Context {
  pyc_dut &dut;
  std::vector<Row> rows;
  Golden golden;
  std::vector<Obligation> ledger;
  unsigned sampled = 0, accepted = 0, peak = 0, fullResets = 0;
  std::array<unsigned, 2> retired{0, 0}, dropped{0, 0};
  bool last = false;
  void observe(std::string_view label) {
    const auto &r = rows[sampled];
    const auto actual = dut.sample().result.packed();
    check(actual, golden, r.take);
    const auto e = golden.decision(r.take);
    const bool valid = !golden.q[5].empty();
    const auto data = valid ? golden.q[5].front().data : zero();
    std::cout << label << ' ' << sampled << ' ' << unsigned(e.ready) << ' '
              << unsigned(valid) << ' ' << visible(data) << '\n';
    if (r.clock && !last) {
      if (r.reset) {
        unsigned dl = 0, dr = 0;
        for (auto &item : ledger) {
          dl += item.owed[0];
          dr += item.owed[1];
          item.owed.fill(false);
        }
        dropped[0] += dl;
        dropped[1] += dr;
        if (dl || dr) {
          require(golden.physical() == 8 && dl == 5 && dr == 5);
          ++fullResets;
          std::cout << "DROP " << label << ' ' << dl << ' ' << dr << ' '
                    << golden.physical() << '\n';
        }
      } else {
        if (r.mustAccept)
          require(r.valid && actual.value().bit(65));
        if (actual.value().bit(64) && r.take) {
          const auto old = golden.q[5].front();
          require(old.uid < ledger.size() && ledger[old.uid].owed[old.branch]);
          payload(gfsim::extract<64>(actual, 0),
                  increment(ledger[old.uid].input, old.branch + 1));
          ledger[old.uid].owed[old.branch] = false;
          ++retired[old.branch];
        }
        if (actual.value().bit(65) && r.valid) {
          ledger.push_back({r.data});
          ++accepted;
        }
      }
    }
    last = r.clock;
    golden.commit(r);
    peak = std::max(peak, golden.physical());
    for (unsigned branch = 0; branch < 2; ++branch) {
      unsigned owed = 0;
      for (const auto &item : ledger)
        owed += item.owed[branch];
      require(accepted == retired[branch] + dropped[branch] + owed);
    }
    ++sampled;
  }
  static void initialize(void *p) { require(drive(p, 0)); }
  static bool drive(void *p, std::uint64_t epoch) {
    auto &c = *static_cast<Context *>(p);
    if (epoch == c.rows.size())
      return false;
    require(epoch < c.rows.size());
    c.dut.drive(inputs(c.rows[epoch]));
    return true;
  }
  static void sample(void *p, std::uint64_t epoch) {
    auto &c = *static_cast<Context *>(p);
    require(epoch == c.sampled + 1);
    c.observe("WORK");
  }
  void finish() {
    require(sampled == rows.size() && peak == 8 && fullResets == 2 &&
            golden.physical() == 0);
    require(golden.asymmetric > 0 && golden.replacements > 5);
    require(golden.first ==
            std::array<std::int64_t, 8>{0, 1, 2, 2, 3, 4, 4, 5});
    for (unsigned i = 0; i < 2; ++i)
      require(dropped[i] == 10 && accepted == retired[i] + dropped[i]);
    std::cout << "HISTORY " << accepted << ' ' << retired[0] << ' '
              << retired[1] << ' ' << dropped[0] << ' ' << dropped[1] << ' '
              << peak << '\n';
  }
};
void fourState(unsigned workers) {
  pyc_dut dut(workers);
  gfsim::SimExecutor exec(dut.system(), dut.observations(), {});
  constexpr std::string_view config =
      R"({"deadlock_window":null,"max_domain_cycles":{},"max_ticks":2000,"schema":"pycircuit-model-config","version":"1"})";
  require(
      exec.ConfigureJson(reinterpret_cast<const std::uint8_t *>(config.data()),
                         config.size()) == PYCIRCUIT_MODEL_STATUS_V1_OK);
  Context c{dut, stimulus(true)};
  dut.drive(inputs(c.rows[0]));
  require(exec.Reset() == PYCIRCUIT_MODEL_STATUS_V1_OK);
  for (const auto &r : c.rows) {
    dut.drive(inputs(r));
    PycircuitModelStepResultV1 s{sizeof(s)};
    require(exec.Step(&s) == PYCIRCUIT_MODEL_STATUS_V1_OK);
    c.observe("FOUR");
  }
  c.finish();
}
void probes(unsigned workers) {
  gfsim::WorkExecutor pool(workers);
  pyc_root root("broadcast-discard", &pool);
  root.Build();
  Golden golden;
  Row r{0, 0, 0, zero(), 0};
  auto drive = [&] {
    const auto p = inputs(r);
    root.pyc_7079635f636c6b = p.pyc_7079635f636c6b;
    root.pyc_7079635f727374 = p.pyc_7079635f727374;
    root.valid = p.valid;
    root.data = p.data;
    root.take = p.take;
  };
  drive();
  root.Reset();
  root.Xfer();
  auto step = [&](bool discard = false) {
    drive();
    root.Work();
    check(root.result.packed(), golden, r.take);
    if (discard)
      root.DiscardNext();
    root.Xfer();
    if (!discard)
      golden.commit(r);
  };
  auto edge = [&](unsigned v, unsigned take, std::uint64_t value) {
    r = {0, 0, v, token(binary(value)), take};
    step();
    r.clock = 1;
    step();
  };
  edge(1, 0, 0x1000000000000000ULL);
  for (unsigned i = 0; i < 4; ++i)
    edge(0, 0, 0);
  for (unsigned i = 0; i < 8; ++i)
    edge(1, 0, 0x2000000000000000ULL + i * 16);
  require(golden.physical() == 8);
  r = {0, 0, 1, token(binary(0x3000000000000000ULL)), 1};
  step();
  r.clock = 1;
  step(true);
  step();
  for (unsigned i = 0; i < 16; ++i)
    edge(1, 1, 0x4000000000000000ULL + i * 16);
  require(golden.asymmetric > 0);
  root.Reset();
  root.DiscardNext();
  root.Xfer();
  step();
  root.Reset();
  root.Xfer();
  golden.reset();
  golden.clock = false;
  r = {0, 0, 0, zero(), 0};
  step();
  std::cout << "PROBE atomic/asymmetric discard retry and discarded host reset "
               "workers="
            << workers << '\n';
  pyc_dut dut(workers);
  gfsim::SimExecutor exec(dut.system(), dut.observations(), {});
  constexpr std::string_view config =
      R"({"deadlock_window":null,"max_domain_cycles":{},"max_ticks":128,"schema":"pycircuit-model-config","version":"1"})";
  require(
      exec.ConfigureJson(reinterpret_cast<const std::uint8_t *>(config.data()),
                         config.size()) == PYCIRCUIT_MODEL_STATUS_V1_OK);
  auto p = inputs({0, 0, 0, zero(), 1});
  dut.drive(p);
  require(exec.Reset() == PYCIRCUIT_MODEL_STATUS_V1_OK);
  PycircuitModelStepResultV1 status{sizeof(status)};
  auto execStep = [&](unsigned clock) {
    p.pyc_7079635f636c6b = known<1>(clock);
    dut.drive(p);
    require(exec.Step(&status) == PYCIRCUIT_MODEL_STATUS_V1_OK);
  };
  p.valid = known<1>(1);
  p.data = wire(token(std::string(64, '1')));
  execStep(1);
  p.valid = known<1>(0);
  for (unsigned i = 0; i < 4; ++i) {
    execStep(0);
    execStep(1);
  }
  execStep(0);
  const auto epoch = exec.cycles();
  p.take = gfsim::wire<gfsim::Bits<1>>::unknown();
  p.pyc_7079635f636c6b = known<1>(1);
  dut.drive(p);
  require(exec.Step(&status) == PYCIRCUIT_MODEL_STATUS_V1_RUNTIME_FAILURE);
  require(exec.cycles() == epoch && dut.system().cycle() == epoch);
  bool unavailable = false;
  try {
    (void)dut.sample();
  } catch (const std::logic_error &) {
    unavailable = true;
  }
  require(unavailable &&
          exec.Step(&status) == PYCIRCUIT_MODEL_STATUS_V1_INVALID_STATE);
  p = inputs({0, 0, 0, zero(), 0});
  dut.drive(p);
  require(exec.Reset() == PYCIRCUIT_MODEL_STATUS_V1_OK);
  execStep(0);
  check(dut.sample().result.packed(), Golden{}, 0);
  std::cout << "PROBE terminal unknown sink pop, unchanged epoch/sample "
               "rejection and mandatory reset workers="
            << workers << '\n';
}
int main(int argc, char **argv) {
  gfsim::SystemRunner runner(argc, argv);
  if (!runner.ready())
    return 2;
  pyc_dut dut(runner.workers());
  Context c{dut, stimulus(false)};
  gfsim::RunnerCallbacks callbacks{&c, &Context::initialize, &Context::drive,
                                   &Context::sample};
  const auto status =
      runner.Run(dut.system(), dut.observations(), {}, callbacks);
  require(status == 0);
  c.finish();
  fourState(runner.workers());
  probes(runner.workers());
  return status;
}
