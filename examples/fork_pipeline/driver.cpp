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
    std::cerr << "fork independent oracle line " << at.line() << '\n';
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
std::vector<Planes> vectors(bool four) {
  std::vector<Planes> result;
  if (!four) {
    for (unsigned bit = 0; bit < 64; ++bit) {
      result.push_back(token(binary(std::uint64_t{1} << bit)));
      result.push_back(token(binary(~(std::uint64_t{1} << bit))));
    }
    for (auto v : {std::uint64_t{0}, ~std::uint64_t{0},
                   std::uint64_t{0x5555555555555555ULL},
                   std::uint64_t{0xaaaaaaaaaaaaaaaaULL}})
      result.push_back(token(binary(v)));
  } else {
    for (unsigned bit = 0; bit < 64; ++bit)
      for (char symbol : {'x', 'z'})
        for (bool latent : {false, true}) {
          auto s = binary(0xa5963c69f00f5a96ULL);
          s[63 - bit] = symbol;
          result.push_back(token(s, latent));
        }
    for (unsigned pattern = 0; pattern < 4; ++pattern)
      for (bool latent : {false, true}) {
        std::string s;
        for (unsigned bit = 0; bit < 64; ++bit)
          s += pattern == 0   ? 'x'
               : pattern == 1 ? 'z'
               : pattern == 2 ? "xz"[bit % 2]
                              : "01xz"[bit % 4];
        result.push_back(token(s, latent));
      }
  }
  require(result.size() == (four ? 264u : 132u));
  return result;
}
struct Row {
  unsigned clock, reset, valid;
  Planes data;
  std::array<unsigned, 2> take;
};
std::vector<Row> stimulus(bool four) {
  std::vector<Row> rows;
  const auto values = vectors(four);
  auto add = [&](unsigned c, unsigned r, unsigned v, const Planes &p,
                 unsigned l,
                 unsigned rr) { rows.push_back({c, r, v, p, {l, rr}}); };
  auto edge = [&](const Planes &p, unsigned v = 1, unsigned l = 1,
                  unsigned r = 1, unsigned reset = 0) {
    add(1, reset, v, p, l, r);
    add(0, reset, v, p, l, r);
  };
  add(0, 1, 0, zero(), 0, 0);
  edge(zero(), 0, 0, 0, 1);
  auto partial = [&](bool mirror) {
    for (unsigned i = 0; i < 3; ++i)
      edge(values[i], 1, 0, 0);
    // Fast branch receives held head once; slow branch remains full.
    for (unsigned i = 0; i < 5; ++i)
      edge(values[i + 3], 1, !mirror, mirror);
    add(1, 0, 1, values[8], !mirror, mirror);
    add(1, 1, 1, values[9], 1,
        1); // held-high reset must retain delivered flags
    add(0, 0, 1, values[10], !mirror, mirror);
    add(0, 0, 0, values[11], mirror, !mirror);
    edge(values[12]); // Complete slow delivery and replace the held source.
    edge(values[13], 0, 0,
         0); // The new head must offer to the fast branch again.
    edge(zero(), 1, 1, 1, 1); // Partial reset drops 1/3 consumer obligations.
  };
  partial(false);
  partial(true);
  for (const auto &p : values)
    edge(p);
  for (unsigned i = 0; i < 12; ++i)
    edge(zero(), 0);
  for (unsigned i = 0; i < 3; ++i)
    edge(values[i], 1, 0, 0); // all five physical slots
  for (unsigned i = 0; i < 8; ++i)
    edge(values[i + 3], 1, 0, 0);
  edge(zero(), 1, 1, 1,
       1); // 3/3 obligations, including the not-yet-delivered head
  edge(values.back());
  edge(zero(), 0);
  for (unsigned i = 0; i < 12; ++i)
    edge(zero(), 0);
  return rows;
}
struct Item {
  unsigned id;
  Planes data;
};
struct Expected {
  bool ready;
  std::array<bool, 2> valid;
  std::array<Planes, 2> data;
};
class Golden {
public:
  std::deque<Item> input;
  std::array<std::deque<Item>, 2> out;
  std::array<bool, 2> delivered{false, false};
  bool clock = false;
  unsigned nextId = 0;
  unsigned partialCopies = 0, suppressed = 0, replacements = 0;
  Expected output(const std::array<unsigned, 2> &take) const {
    std::array<bool, 2> room, now;
    Expected e;
    for (unsigned i = 0; i < 2; ++i) {
      e.valid[i] = !out[i].empty();
      e.data[i] = e.valid[i] ? out[i].front().data : zero();
      room[i] = out[i].size() < 2 || (e.valid[i] && take[i]);
      now[i] = delivered[i] || (!input.empty() && !delivered[i] && room[i]);
    }
    e.ready = input.empty() || (now[0] && now[1]);
    return e;
  }
  unsigned physical() const {
    return input.size() + out[0].size() + out[1].size();
  }
  void reset() {
    input.clear();
    out[0].clear();
    out[1].clear();
    delivered.fill(false);
  }
  void commit(const Row &r) {
    const auto e = output(r.take);
    if (r.clock && !clock) {
      if (r.reset)
        reset();
      else {
        std::array<bool, 2> copy, now;
        const bool occupied = !input.empty();
        const auto head = occupied ? input.front() : Item{0, zero()};
        for (unsigned i = 0; i < 2; ++i) {
          const bool room = out[i].size() < 2 || (e.valid[i] && r.take[i]);
          copy[i] = occupied && !delivered[i] && room;
          now[i] = delivered[i] || copy[i];
          suppressed += occupied && delivered[i] && room;
        }
        const bool complete = occupied && now[0] && now[1];
        partialCopies += occupied && (copy[0] != copy[1]) && !complete;
        for (unsigned i = 0; i < 2; ++i) {
          if (e.valid[i] && r.take[i])
            out[i].pop_front();
          if (copy[i])
            out[i].push_back(head);
        }
        if (complete) {
          input.pop_front();
          delivered.fill(false);
        } else if (occupied)
          delivered = now;
        if (e.ready && r.valid) {
          replacements += occupied && complete;
          input.push_back({nextId++, r.data});
        }
        require(input.size() <= 1 && out[0].size() <= 2 && out[1].size() <= 2);
        require(!input.empty() || (!delivered[0] && !delivered[1]));
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
  p.data = wire(r.data);
  p.left_take = known<1>(r.take[0]);
  p.right_take = known<1>(r.take[1]);
  return p;
}
template <class Packed> void payload(const Packed &p, const Planes &e) {
  require(p.value() == bits<64>(e.value));
  require(p.knownMask() == bits<64>(e.known));
  require(p.zMask() == bits<64>(e.z));
}
template <class Packed> void check(const Packed &p, const Expected &e) {
  for (auto bit : {130u, 129u, 64u})
    require(p.knownMask().bit(bit) && !p.zMask().bit(bit));
  require(p.value().bit(130) == e.ready && p.value().bit(129) == e.valid[0] &&
          p.value().bit(64) == e.valid[1]);
  payload(gfsim::extract<64>(p, 65), e.data[0]);
  payload(gfsim::extract<64>(p, 0), e.data[1]);
}
struct Context {
  pyc_dut &dut;
  std::vector<Row> rows;
  Golden golden;
  std::array<std::deque<Item>, 2> obligations;
  unsigned sampled = 0, accepted = 0, peak = 0, resetCount = 0;
  std::array<unsigned, 2> retired{0, 0}, dropped{0, 0};
  bool last = false;
  void observe(std::string_view label) {
    const auto &r = rows[sampled];
    const auto e = golden.output(r.take);
    const auto actual = dut.sample().result.packed();
    check(actual, e);
    std::cout << label << ' ' << sampled << ' ' << unsigned(e.ready) << ' '
              << unsigned(e.valid[0]) << ' ' << visible(e.data[0]) << ' '
              << unsigned(e.valid[1]) << ' ' << visible(e.data[1]) << '\n';
    if (r.clock && !last) {
      if (r.reset) {
        const auto dl = obligations[0].size(), dr = obligations[1].size();
        if (dl || dr) {
          std::cout << "DROP " << label << ' ' << dl << ' ' << dr << ' '
                    << golden.physical() << '\n';
          ++resetCount;
        }
        for (unsigned i = 0; i < 2; ++i) {
          dropped[i] += obligations[i].size();
          obligations[i].clear();
        }
      } else {
        const std::array<bool, 2> valid{actual.value().bit(129),
                                        actual.value().bit(64)};
        for (unsigned i = 0; i < 2; ++i)
          if (valid[i] && r.take[i]) {
            require(!obligations[i].empty());
            const auto old = obligations[i].front();
            require(old.id == golden.out[i].front().id);
            if (i == 0)
              payload(gfsim::extract<64>(actual, 65), old.data);
            else
              payload(gfsim::extract<64>(actual, 0), old.data);
            obligations[i].pop_front();
            ++retired[i];
          }
        if (actual.value().bit(130) && r.valid) {
          for (auto &q : obligations)
            q.push_back({accepted, r.data});
          ++accepted;
        }
      }
    }
    last = r.clock;
    golden.commit(r);
    peak = std::max(peak, golden.physical());
    for (unsigned i = 0; i < 2; ++i) {
      require(accepted == retired[i] + dropped[i] + obligations[i].size());
      require(obligations[i].size() <= 3);
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
    require(sampled == rows.size() && peak == 5 && resetCount == 3);
    require(golden.partialCopies >= 4 && golden.suppressed >= 4 &&
            golden.replacements > 20);
    require(obligations[0].empty() && obligations[1].empty());
    require(dropped[0] == 7 && dropped[1] == 7);
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
  pyc_root root("fork-discard", &pool);
  root.Build();
  Golden golden;
  auto r = Row{0, 0, 0, zero(), {0, 0}};
  auto drive = [&] {
    auto p = inputs(r);
    root.pyc_7079635f636c6b = p.pyc_7079635f636c6b;
    root.pyc_7079635f727374 = p.pyc_7079635f727374;
    root.valid = p.valid;
    root.data = p.data;
    root.left_take = p.left_take;
    root.right_take = p.right_take;
  };
  drive();
  root.Reset();
  root.Xfer();
  auto step = [&](bool discard = false) {
    drive();
    root.Work();
    check(root.result.packed(), golden.output(r.take));
    if (discard)
      root.DiscardNext();
    root.Xfer();
    if (!discard)
      golden.commit(r);
  };
  auto edge = [&](unsigned v, unsigned l, unsigned rr, unsigned value) {
    r = {0, 0, v, token(binary(value)), {l, rr}};
    step();
    r.clock = 1;
    step();
  };
  edge(1, 0, 0, 11);
  edge(1, 0, 0, 22);
  edge(1, 0, 0, 33);
  r = {0, 0, 1, token(binary(44)), {1, 0}};
  step();
  r.clock = 1;
  step(true);
  step(); // partial metadata proposal discarded then retried
  for (unsigned i = 0; i < 6; ++i)
    edge(1, 1, 0, 55 + i);
  require(golden.out[0].empty() && golden.input.size() == 1 &&
          golden.delivered[0] && !golden.delivered[1]);
  root.Reset();
  root.DiscardNext();
  root.Xfer();
  step(); // Discarded host Reset retains partial delivery.
  root.Reset();
  root.Xfer();
  golden.reset();
  golden.clock = false;
  r = {0, 0, 0, zero(), {0, 0}};
  step();
  std::cout << "PROBE partial delivery discard/retry, duplicate suppression "
               "and discarded host reset workers="
            << workers << '\n';
  pyc_dut dut(workers);
  gfsim::SimExecutor exec(dut.system(), dut.observations(), {});
  constexpr std::string_view config =
      R"({"deadlock_window":null,"max_domain_cycles":{},"max_ticks":64,"schema":"pycircuit-model-config","version":"1"})";
  require(
      exec.ConfigureJson(reinterpret_cast<const std::uint8_t *>(config.data()),
                         config.size()) == PYCIRCUIT_MODEL_STATUS_V1_OK);
  auto p = inputs({0, 0, 0, zero(), {0, 0}});
  dut.drive(p);
  require(exec.Reset() == PYCIRCUIT_MODEL_STATUS_V1_OK);
  auto s = PycircuitModelStepResultV1{sizeof(PycircuitModelStepResultV1)};
  for (unsigned value : {11u, 22u, 33u}) {
    p = inputs({0, 0, 1, token(binary(value)), {0, 0}});
    dut.drive(p);
    require(exec.Step(&s) == PYCIRCUIT_MODEL_STATUS_V1_OK);
    p.pyc_7079635f636c6b = known<1>(1);
    dut.drive(p);
    require(exec.Step(&s) == PYCIRCUIT_MODEL_STATUS_V1_OK);
  }
  p = inputs({0, 0, 0, zero(), {1, 0}});
  dut.drive(p);
  require(exec.Step(&s) == PYCIRCUIT_MODEL_STATUS_V1_OK);
  const auto epoch = exec.cycles();
  p.pyc_7079635f636c6b = known<1>(1);
  p.right_take = gfsim::wire<gfsim::Bits<1>>::unknown();
  dut.drive(p);
  require(exec.Step(&s) == PYCIRCUIT_MODEL_STATUS_V1_RUNTIME_FAILURE);
  require(exec.cycles() == epoch && dut.system().cycle() == epoch);
  bool unavailable = false;
  try {
    (void)dut.sample();
  } catch (const std::logic_error &) {
    unavailable = true;
  }
  require(unavailable &&
          exec.Step(&s) == PYCIRCUIT_MODEL_STATUS_V1_INVALID_STATE);
  p = inputs({0, 0, 0, zero(), {0, 0}});
  dut.drive(p);
  require(exec.Reset() == PYCIRCUIT_MODEL_STATUS_V1_OK);
  require(exec.Step(&s) == PYCIRCUIT_MODEL_STATUS_V1_OK);
  check(dut.sample().result.packed(), Golden{}.output({0, 0}));
  std::cout << "PROBE terminal unknown slow pop, unchanged epoch/sample "
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
