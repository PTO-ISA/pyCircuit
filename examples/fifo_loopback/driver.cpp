#include "gfsim/SimExecutor.h"
#include "gfsim/SystemRunner.h"
#include "pycircuit_system.hpp"
#include <algorithm>
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
    std::cerr << "fifo_loopback oracle at " << at.line() << '\n';
    std::abort();
  }
}
struct Planes {
  unsigned value, known = 255, z = 0;
  bool operator==(const Planes &) const = default;
};
std::string visible(Planes p) {
  std::string text;
  for (int bit = 7; bit >= 0; --bit)
    text += p.z >> bit & 1          ? 'z'
            : !(p.known >> bit & 1) ? 'x'
            : p.value >> bit & 1    ? '1'
                                    : '0';
  return text;
}
char band(char a, char b) {
  if (a == '0' || b == '0')
    return '0';
  return a == '1' && b == '1' ? '1' : 'x';
}
char bor(char a, char b) {
  if (a == '1' || b == '1')
    return '1';
  return a == '0' && b == '0' ? '0' : 'x';
}
auto control(char c) {
  using B = gfsim::Bits<1>;
  using W = gfsim::wire<B>;
  if (c == '0' || c == '1')
    return W::known(B{c == '1'});
  require(c == 'x' || c == 'z');
  return W::fromPacked(gfsim::FourState<1>::fromMasks(B{1}, B{0}, B{c == 'z'}));
}
auto wire(Planes p) {
  using B = gfsim::Bits<8>;
  return gfsim::wire<B>::fromPacked(
      gfsim::FourState<8>::fromMasks(B{p.value}, B{p.known}, B{p.z}));
}
struct Row {
  std::string tag;
  unsigned index;
  bool clock;
  char reset, valid, take;
  Planes data;
};
std::vector<Planes> values(bool four) {
  std::vector<Planes> result;
  if (!four) {
    for (unsigned i = 0; i < 256; ++i)
      result.push_back({i});
  } else {
    for (unsigned bit = 0; bit < 8; ++bit)
      for (bool z : {false, true})
        for (bool latent : {false, true}) {
          const unsigned mask = 1u << bit;
          result.push_back({(0xa5u & ~mask) | (latent ? mask : 0), 255u ^ mask,
                            z ? mask : 0});
        }
    for (unsigned pattern = 0; pattern < 4; ++pattern)
      for (bool latent : {false, true}) {
        Planes p{0, 0, 0};
        for (unsigned bit = 0; bit < 8; ++bit) {
          const char c = pattern == 0   ? 'x'
                         : pattern == 1 ? 'z'
                         : pattern == 2 ? "xz"[bit % 2]
                                        : "01xz"[bit % 4];
          p.value |= unsigned(c == '1' || ((c == 'x' || c == 'z') && latent))
                     << bit;
          p.known |= unsigned(c == '0' || c == '1') << bit;
          p.z |= unsigned(c == 'z') << bit;
        }
        result.push_back(p);
      }
  }
  require(result.size() == (four ? 40u : 256u));
  return result;
}
std::vector<Row> stimulus(bool four) {
  std::vector<Row> rows;
  unsigned index = 0;
  auto add = [&](std::string tag, bool c, char r = '0', char v = '0',
                 Planes p = {}, char t = '0') {
    rows.push_back({tag, index++, c, r, v, t, p});
  };
  if (!four) {
    unsigned smoke = 0, observation = 0;
    auto cycle = [&](char r, char v, Planes p, char t) {
      rows.push_back({"SMOKE", smoke++, false, r, v, t, p});
      rows.push_back({"SMOKE", smoke++, true, r, v, t, p});
      rows.push_back({"OBS_SMOKE", observation++, true, r, v, t, p});
    };
    cycle('1', '0', {}, '0');
    cycle('1', '0', {}, '0');
    cycle('0', '0', {}, '0');
    for (unsigned n = 0; n < 4; ++n)
      cycle('0', '1', {42}, '1');
    require(smoke == 14 && observation == 7);
    index = 0;
    add("DRAIN", false, '0', '0', {}, '1');
    add("DRAIN", true, '0', '0', {}, '1');
  }
  index = 0;
  const std::string tag = four ? "FOUR" : "EXT";
  auto row = [&](bool c, char r = '0', char v = '0', Planes p = {},
                 char t = '0') { add(tag, c, r, v, p, t); };
  auto edge = [&](Planes p = {}, char v = '0', char t = '0', char r = '0') {
    row(false, r, v, p, t);
    row(true, r, v, p, t);
  };
  row(false, '1');
  edge({}, '0', '0', '1');
  edge({0x17}, '1', '1'); // Empty both requests births, never bypasses.
  edge({0xe2}, '1');
  edge({0x33}, '1'); // Exact capacity and rejected third offer.
  row(true, '1', '1', {0xaa}, '0');
  row(true, '1', '1', {0x55}, '1');
  if (four) {
    row(true, 'x', '0', {0xff}, 'x');
    row(true, 'z', '0', {0x00}, 'z');
  }
  row(false, '1', '1', {0x44});
  row(false, '1', '1', {0x77});
  edge({0x91}, '1', '1'); // Full replacement consumes the old 0x17 head.
  for (unsigned i = 0; i < 40; ++i)
    edge({(i * 37) & 255}, '1', '1');
  edge({}, four ? 'z' : '1', four ? 'x' : '1', '1');
  for (unsigned i = 0; i < 4; ++i)
    edge({}, '0', '1');
  for (Planes p : values(four))
    edge(p, '1', '1');
  for (unsigned i = 0; i < 3; ++i)
    edge({}, '0', '1');
  edge({0x11}, '1');
  edge({0x22}, '1');
  if (four) {
    edge({}, 'x');
    edge({}, 'z');
  }
  edge({}, four ? 'x' : '1', four ? 'z' : '1', '1');
  for (unsigned i = 0; i < 3; ++i)
    edge({}, '0', '1');
  if (four) {
    edge({}, '0', 'x');
    edge({}, '0', 'z');
  }
  require(index == (four ? 213u : 635u));
  require(rows.size() == (four ? 213u : 658u));
  return rows;
}
struct Expected {
  char ready;
  bool valid;
  Planes data;
};
struct Golden {
  std::deque<Planes> q;
  bool clock = false;
  Expected read(char take) const {
    const char valid = q.empty() ? '0' : '1';
    return {bor(q.size() < 2 ? '1' : '0', band(valid, take)), !q.empty(),
            q.empty() ? Planes{} : q.front()};
  }
  void commit(const Row &r) {
    if (r.clock && !clock) {
      require(r.reset == '0' || r.reset == '1');
      if (r.reset == '1')
        q.clear();
      else {
        const auto e = read(r.take);
        const char push = band(r.valid, e.ready),
                   pop = band(r.take, e.valid ? '1' : '0');
        require((push == '0' || push == '1') && (pop == '0' || pop == '1'));
        if (pop == '1')
          q.pop_front();
        if (push == '1')
          q.push_back(r.data);
      }
      require(q.size() <= 2);
    }
    clock = r.clock;
  }
};
pyc_dut::Inputs ports(const Row &r) {
  pyc_dut::Inputs p;
  static_assert(decltype(p.in_valid)::width == 1);
  static_assert(decltype(p.out_ready)::width == 1);
  static_assert(decltype(p.in_data)::width == 8);
  p.pyc_7079635f636c6b = control(r.clock ? '1' : '0');
  p.pyc_7079635f727374 = control(r.reset);
  p.in_valid = control(r.valid);
  p.out_ready = control(r.take);
  p.in_data = decltype(p.in_data)::fromPacked(wire(r.data).packed());
  return p;
}
template <class P> void checkData(const P &p, Planes e) {
  require(p.value() == gfsim::Bits<8>{e.value});
  require(p.knownMask() == gfsim::Bits<8>{e.known});
  require(p.zMask() == gfsim::Bits<8>{e.z});
}
template <class P> char symbol(const P &p, unsigned b) {
  return p.zMask().bit(b)        ? 'z'
         : !p.knownMask().bit(b) ? 'x'
         : p.value().bit(b)      ? '1'
                                 : '0';
}
struct LedgerToken {
  std::uint64_t identity;
  Planes data;
};
struct Context {
  pyc_dut &dut;
  std::vector<Row> rows;
  Golden golden;
  std::deque<LedgerToken> history;
  std::string phase;
  unsigned sampled = 0, initial = 0, accepted = 0, retired = 0, dropped = 0,
           peak = 0, full = 0, resetFull = 0;
  std::uint64_t nextIdentity = 0;
  bool clock = false;
  void finishPhase() {
    require(initial + accepted == retired + dropped + history.size());
    if (phase == "SMOKE")
      require(initial == 0 && accepted == 4 && retired == 3 && dropped == 0 &&
              history.size() == 1 && history.front().data == Planes{42});
    if (phase == "DRAIN")
      require(initial == 1 && accepted == 0 && retired == 1 && dropped == 0 &&
              history.empty());
    if (phase == "EXT" || phase == "FOUR")
      require(initial == 0 && accepted == (phase == "EXT" ? 301u : 85u) &&
              retired == accepted - 4 && dropped == 4 && history.empty() &&
              peak == 2 && full == 41 && resetFull == 2);
    std::cout << "HISTORY " << phase << ' ' << initial << ' ' << accepted << ' '
              << retired << ' ' << dropped << ' ' << history.size() << ' '
              << peak << '\n';
  }
  void observe() {
    const auto &r = rows[sampled];
    const std::string nextPhase = r.tag == "OBS_SMOKE" ? "SMOKE" : r.tag;
    if (phase != nextPhase) {
      if (!phase.empty())
        finishPhase();
      phase = nextPhase;
      initial = history.size();
      accepted = retired = dropped = full = resetFull = 0;
      peak = initial;
    }
    const auto e = golden.read(r.take);
    const auto out = dut.sample().result.packed();
    const char actualReady = symbol(out, 9), actualValid = symbol(out, 8);
    require(actualReady == e.ready && actualValid == (e.valid ? '1' : '0'));
    const auto data = gfsim::extract<8>(out, 0);
    checkData(data, e.data);
    std::cout << (r.tag == "FOUR" ? "FOUR " : "WORK ") << sampled << ' '
              << r.tag << ' ' << r.index << ' ' << actualReady << ' '
              << actualValid << ' ' << visible(e.data) << '\n';
    if (r.clock && !clock) {
      if (r.reset == '1') {
        resetFull += history.size() == 2;
        dropped += history.size();
        history.clear();
      } else {
        const bool wasFull = history.size() == 2;
        if (actualValid == '1' && r.take == '1') {
          require(!history.empty());
          checkData(data, history.front().data);
          history.pop_front();
          ++retired;
        }
        if (actualReady == '1' && r.valid == '1') {
          history.push_back({nextIdentity++, r.data});
          ++accepted;
          full += wasFull;
        }
      }
      peak = std::max(peak, unsigned(history.size()));
      require(history.size() <= 2);
    }
    clock = r.clock;
    golden.commit(r);
    require(history.size() == golden.q.size());
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
    c.observe();
  }
  void finish() {
    require(sampled == rows.size());
    finishPhase();
  }
};
constexpr std::string_view config =
    R"({"deadlock_window":null,"max_domain_cycles":{},"max_ticks":4000,"schema":"pycircuit-model-config","version":"1"})";
void configure(gfsim::SimExecutor &exec) {
  require(
      exec.ConfigureJson(reinterpret_cast<const std::uint8_t *>(config.data()),
                         config.size()) == PYCIRCUIT_MODEL_STATUS_V1_OK);
}
void step(gfsim::SimExecutor &exec) {
  PycircuitModelStepResultV1 result{sizeof(result)};
  require(exec.Step(&result) == PYCIRCUIT_MODEL_STATUS_V1_OK &&
          result.state == PYCIRCUIT_MODEL_STEP_V1_RUNNING);
}
void extensions(unsigned workers) {
  // This current-contract cold probe is not a historical native parity claim.
  gfsim::WorkExecutor pool(workers);
  pyc_root cold("cold", &pool);
  cold.Build();
  cold.pyc_7079635f636c6b = cold.pyc_7079635f727374 = cold.in_valid =
      cold.out_ready = control('0');
  cold.in_data = wire({0});
  cold.Work();
  const auto p = cold.result.packed();
  require(p.knownMask() == gfsim::Bits<10>{} && p.zMask() == gfsim::Bits<10>{});
  cold.Xfer();
  std::cout
      << "CONTRACT cold X before Reset; historical native initialized empty\n";
  pyc_dut dut(workers);
  gfsim::SimExecutor exec(dut.system(), dut.observations(), {});
  configure(exec);
  Context c{dut, stimulus(true)};
  dut.drive(ports(c.rows[0]));
  require(exec.Reset() == PYCIRCUIT_MODEL_STATUS_V1_OK);
  for (const auto &r : c.rows) {
    dut.drive(ports(r));
    step(exec);
    c.observe();
  }
  c.finish();
  for (unsigned bad = 1; bad <= 3; ++bad) {
    pyc_dut terminal(workers);
    gfsim::SimExecutor failed(terminal.system(), terminal.observations(), {});
    configure(failed);
    Row r{"NEGATIVE", 0, false, '0', '0', '0', {0}};
    terminal.drive(ports(r));
    require(failed.Reset() == PYCIRCUIT_MODEL_STATUS_V1_OK);
    step(failed);
    if (bad == 3) {
      r.clock = true;
      r.valid = '1';
      r.data = {0x37};
      terminal.drive(ports(r));
      step(failed);
      r.clock = false;
      r.valid = '0';
      terminal.drive(ports(r));
      step(failed);
    }
    const auto epoch = failed.cycles();
    r.clock = true;
    if (bad == 1)
      r.reset = 'x';
    else if (bad == 2)
      r.valid = 'z';
    else
      r.take = 'x';
    terminal.drive(ports(r));
    PycircuitModelStepResultV1 result{sizeof(result)};
    require(failed.Step(&result) == PYCIRCUIT_MODEL_STATUS_V1_RUNTIME_FAILURE &&
            result.state == PYCIRCUIT_MODEL_STEP_V1_FAILED &&
            failed.cycles() == epoch && terminal.system().cycle() == epoch);
    require(failed.Step(&result) == PYCIRCUIT_MODEL_STATUS_V1_INVALID_STATE);
    bool unavailable = false;
    try {
      (void)terminal.sample();
    } catch (const std::logic_error &) {
      unavailable = true;
    }
    require(unavailable);
    r = {"NEGATIVE", 0, false, '0', '0', '0', {0}};
    terminal.drive(ports(r));
    require(failed.Reset() == PYCIRCUIT_MODEL_STATUS_V1_OK);
    step(failed);
    const auto empty = terminal.sample().result.packed();
    require(empty.value() == gfsim::Bits<10>{512} &&
            empty.knownMask() == gfsim::Bits<10>{1023} &&
            empty.zMask() == gfsim::Bits<10>{});
    std::cout << "NEGATIVE " << bad
              << " terminal failure, unavailable sample, Reset recovery\n";
  }
}
int main(int argc, char **argv) {
  gfsim::SystemRunner runner(argc, argv);
  if (!runner.ready())
    return 2;
  pyc_dut dut(runner.workers());
  static_assert(decltype(dut.sample().result)::width == 10);
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
