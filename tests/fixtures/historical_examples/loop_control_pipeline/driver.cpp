#include "gfsim/SimExecutor.h"
#include "gfsim/SystemRunner.h"
#include "pycircuit_system.hpp"
#include <algorithm>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <optional>
#include <source_location>
#include <string>
#include <string_view>
#include <vector>

void require(bool ok,
             std::source_location at = std::source_location::current()) {
  if (!ok) {
    std::cerr << "loop_control_pipeline oracle at " << at.line() << '\n';
    std::abort();
  }
}
struct Planes {
  unsigned value = 0, known = 63, z = 0;
  bool operator==(const Planes &) const = default;
};
char symbol(Planes p, unsigned b) {
  return p.z >> b & 1          ? 'z'
         : !(p.known >> b & 1) ? 'x'
         : p.value >> b & 1    ? '1'
                               : '0';
}
std::string visible(Planes p) {
  std::string s;
  for (int b = 5; b >= 0; --b)
    s += symbol(p, b);
  return s;
}
char band(char a, char b) {
  return a == '0' || b == '0' ? '0' : a == '1' && b == '1' ? '1' : 'x';
}
char bor(char a, char b) {
  return a == '1' || b == '1' ? '1' : a == '0' && b == '0' ? '0' : 'x';
}
char bnot(char a) { return a == '0' ? '1' : a == '1' ? '0' : 'x'; }
char continuation(Planes p) {
  const char positive = (p.known & 60) == 60 ? (p.value >> 2 ? '1' : '0') : 'x';
  return band(positive, bnot(symbol(p, 1)));
}
Planes decrement(Planes p) {
  require((p.known & 60) == 60 && p.value >> 2 > 0);
  p.value -= 4;
  return p;
}
Planes completed(Planes p) {
  require(continuation(p) != 'x');
  return continuation(p) == '1' ? Planes{p.value & 3, p.known, p.z} : p;
}
Planes symbolic(unsigned index, bool latent) {
  Planes p{0, 0, 0};
  for (unsigned b = 0; b < 6; ++b, index /= 4) {
    const unsigned c = index % 4;
    p.value |= unsigned(c == 1 || (c >= 2 && latent)) << b;
    p.known |= unsigned(c < 2) << b;
    p.z |= unsigned(c == 3) << b;
  }
  return p;
}
auto control(char c) {
  using B = gfsim::Bits<1>;
  return gfsim::wire<B>::fromPacked(gfsim::FourState<1>::fromMasks(
      B{c != '0'}, B{c == '0' || c == '1'}, B{c == 'z'}));
}
auto wire(Planes p) {
  using B = gfsim::Bits<6>;
  return gfsim::wire<B>::fromPacked(
      gfsim::FourState<6>::fromMasks(B{p.value}, B{p.known}, B{p.z}));
}
struct Row {
  std::string tag;
  bool clock;
  char reset = '0', valid = '0', take = '0';
  Planes data;
};
std::vector<Row> stimulus(bool four) {
  std::vector<Row> rows;
  const std::string tag = four ? "FOUR" : "KNOWN";
  auto row = [&](std::string t, bool c, Planes p = {}, char v = '0',
                 char take = '0', char reset = '0') {
    rows.push_back({t, c, reset, v, take, p});
  };
  auto edge = [&](std::string t, Planes p = {}, char v = '0', char take = '0',
                  char reset = '0') {
    row(t, false, p, v, take, reset);
    row(t, true, p, v, take, reset);
  };
  edge(tag, {}, '0', '0', '1');
  const unsigned limit = four ? 4096 : 64;
  for (unsigned i = 0; i < limit; ++i)
    for (unsigned latent = 0; latent < (four ? 2u : 1u); ++latent) {
      const Planes p = four ? symbolic(i, latent) : Planes{i};
      if (continuation(p) == 'x')
        continue;
      const unsigned n = continuation(p) == '1' ? p.value >> 2 : 0;
      edge(tag, p, '1', '1');
      for (unsigned j = 0; j < n + 3; ++j)
        edge(tag, {}, '0', '1');
    }
  if (four)
    return rows;
  edge("EXT", {38}, '1'); // Break token becomes a blocked result.
  edge("EXT", {61}, '1'); // Fifteen iterations despite the full result.
  edge("EXT", {19}, '1');
  edge("EXT", {30},
       '1'); // Source D2 + feedback D1 + result D1 are all occupied.
  for (unsigned i = 0; i < 20; ++i)
    edge("EXT", {unsigned(i % 16) * 4}, '1');
  row("EXT", true, {63}, '1', '1', '1'); // Held-high reset cannot commit.
  row("EXT", true, {17}, '1', '0');
  row("EXT", false, {23}, '1', '0', '1');
  row("EXT", false, {27}, '1', '0', '1');
  for (unsigned i = 0; i < 80; ++i)
    edge("EXT", {unsigned(i % 5) * 4 + unsigned(i % 3 == 0) * 2 + i % 2}, '1',
         '1');
  for (unsigned i = 0; i < 24; ++i)
    edge("EXT", {}, '0', '1');
  edge("EXT", {38}, '1');
  edge("EXT", {61}, '1');
  edge("EXT", {19}, '1');
  edge("EXT", {30}, '1');
  edge("EXT", {}, '1', '1', '1'); // Busy synchronous reset drops all four.
  for (unsigned i = 0; i < 4; ++i)
    edge("EXT", {}, '0', '1');
  return rows;
}
struct Expected {
  char ready;
  bool valid;
  Planes data;
};
struct Golden {
  std::deque<Planes> source;
  std::optional<Planes> feedback, output;
  bool clock = false;
  Planes chosen() const {
    return feedback ? *feedback : source.empty() ? Planes{} : source.front();
  }
  char resultReady(char take) const { return output ? take : '1'; }
  char advance(char take) const {
    return bor(continuation(chosen()), resultReady(take));
  }
  char sourceTake(char take) const {
    return band(feedback ? '0' : '1', advance(take));
  }
  Expected read(char take) const {
    return {bor(source.size() < 2 ? '1' : '0',
                band(source.empty() ? '0' : '1', sourceTake(take))),
            output.has_value(), output.value_or(Planes{})};
  }
  unsigned occupancy() const {
    return source.size() + feedback.has_value() + output.has_value();
  }
  void commit(const Row &r) {
    if (r.clock && !clock) {
      require(r.reset == '0' || r.reset == '1');
      if (r.reset == '1') {
        source.clear();
        feedback.reset();
        output.reset();
      } else {
        const auto d = chosen();
        const char c = continuation(d), h = advance(r.take);
        const bool available = feedback.has_value() || !source.empty();
        const char spop = band(source.empty() ? '0' : '1', sourceTake(r.take));
        const char spush = band(r.valid, read(r.take).ready);
        const char fpop = band(feedback ? '1' : '0', h);
        const char fpush =
            band(available ? c : '0', bor(feedback ? '0' : '1', fpop));
        const char opop = band(output ? '1' : '0', r.take);
        const char opush = band(available ? bnot(c) : '0', resultReady(r.take));
        for (char transfer : {spop, spush, fpop, fpush, opop, opush})
          require(transfer == '0' || transfer == '1');
        if (spop == '1')
          source.pop_front();
        if (spush == '1')
          source.push_back(r.data);
        if (fpop == '1')
          feedback.reset();
        if (fpush == '1')
          feedback = decrement(d);
        if (opop == '1')
          output.reset();
        if (opush == '1')
          output = d;
        require(source.size() <= 2 && occupancy() <= 4);
      }
    }
    clock = r.clock;
  }
};
pyc_dut::Inputs ports(const Row &r) {
  pyc_dut::Inputs p;
  static_assert(decltype(p.data)::width == 6);
  p.pyc_7079635f636c6b = control(r.clock ? '1' : '0');
  p.pyc_7079635f727374 = control(r.reset);
  p.valid = control(r.valid);
  p.take = control(r.take);
  p.data = decltype(p.data)::fromPacked(wire(r.data).packed());
  return p;
}
template <class P> char symbol(const P &p, unsigned b) {
  return p.zMask().bit(b)        ? 'z'
         : !p.knownMask().bit(b) ? 'x'
         : p.value().bit(b)      ? '1'
                                 : '0';
}
template <class P> void same(const P &p, Planes e) {
  require(p.value() == gfsim::Bits<6>{e.value});
  require(p.knownMask() == gfsim::Bits<6>{e.known});
  require(p.zMask() == gfsim::Bits<6>{e.z});
}
template <class P> void check(const P &p, Expected e) {
  require(symbol(p, 7) == e.ready && symbol(p, 6) == (e.valid ? '1' : '0'));
  same(gfsim::extract<6>(p, 0), e.data);
}
struct LedgerToken {
  unsigned identity, birth, latency;
  Planes result;
};
struct Context {
  pyc_dut &dut;
  std::vector<Row> rows;
  Golden golden;
  std::deque<LedgerToken> history;
  unsigned sampled = 0, edges = 0, nextIdentity = 0, accepted = 0, retired = 0,
           dropped = 0, peak = 0, blocked = 0, fullProgress = 0,
           feedbackExit = 0, fullExit = 0, replacements = 0, resetFull = 0;
  void observe() {
    const auto &r = rows[sampled];
    const auto e = golden.read(r.take);
    const auto out = dut.sample().result.packed();
    check(out, e);
    std::cout << (r.tag == "FOUR" ? "FOUR " : "WORK ") << sampled << ' '
              << r.tag << ' ' << e.ready << ' ' << e.valid << ' '
              << visible(e.data) << '\n';
    if (r.clock && !golden.clock) {
      if (r.reset == '1') {
        resetFull += history.size() == 4;
        dropped += history.size();
        history.clear();
      } else {
        const char c = continuation(golden.chosen());
        const bool resident = golden.feedback.has_value(),
                   full = golden.source.size() == 2;
        blocked += resident && c == '0' && golden.resultReady(r.take) == '0';
        fullProgress +=
            resident && c == '1' && golden.output.has_value() && r.take == '0';
        if (resident && c == '0' && golden.resultReady(r.take) == '1') {
          ++feedbackExit;
          fullExit += full;
          require(golden.sourceTake(r.take) == '0');
          if (full)
            require(e.ready == '0');
        }
        replacements += full && e.ready == '1' && r.valid == '1';
        if (e.valid && r.take == '1') {
          require(!history.empty());
          same(gfsim::extract<6>(out, 0), history.front().result);
          if (r.tag == "KNOWN" || r.tag == "FOUR")
            require(edges - history.front().birth == history.front().latency);
          history.pop_front();
          ++retired;
        }
        if (e.ready == '1' && r.valid == '1') {
          const unsigned n =
              continuation(r.data) == '1' ? r.data.value >> 2 : 0;
          history.push_back({nextIdentity++, edges, n + 2, completed(r.data)});
          ++accepted;
        }
      }
      ++edges;
    }
    golden.commit(r);
    peak = std::max(peak, unsigned(history.size()));
    require(history.size() == golden.occupancy());
    require(accepted == retired + dropped + history.size());
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
  void finish(bool four) {
    require(sampled == rows.size() && history.empty() &&
            accepted == retired + dropped);
    if (four)
      require(accepted == 2192 && retired == accepted && dropped == 0);
    else
      require(accepted >= 100 && peak == 4 && dropped == 4 && resetFull == 1 &&
              blocked >= 5 && fullProgress >= 10 && fullExit >= 1 &&
              replacements >= 5);
    std::cout << "HISTORY " << (four ? "FOUR" : "KNOWN_EXT") << ' ' << accepted
              << ' ' << retired << ' ' << dropped << ' ' << history.size()
              << ' ' << peak << '\n';
    std::cout << "COVERAGE " << (four ? "FOUR" : "KNOWN_EXT") << ' ' << blocked
              << ' ' << fullProgress << ' ' << feedbackExit << ' ' << fullExit
              << ' ' << replacements << ' ' << resetFull << '\n';
  }
};
constexpr std::string_view config =
    R"({"deadlock_window":null,"max_domain_cycles":{},"max_ticks":100000,"schema":"pycircuit-model-config","version":"1"})";
void configure(gfsim::SimExecutor &exec) {
  require(
      exec.ConfigureJson(reinterpret_cast<const std::uint8_t *>(config.data()),
                         config.size()) == PYCIRCUIT_MODEL_STATUS_V1_OK);
}
void step(gfsim::SimExecutor &exec) {
  PycircuitModelStepResultV1 s{sizeof(s)};
  require(exec.Step(&s) == PYCIRCUIT_MODEL_STATUS_V1_OK &&
          s.state == PYCIRCUIT_MODEL_STEP_V1_RUNNING);
}
void rawDrive(pyc_root &dut, const Row &r) {
  auto p = ports(r);
  dut.pyc_7079635f636c6b = p.pyc_7079635f636c6b;
  dut.pyc_7079635f727374 = p.pyc_7079635f727374;
  dut.valid = p.valid;
  dut.take = p.take;
  dut.data = p.data;
}
void owner(unsigned workers, unsigned mode) {
  gfsim::WorkExecutor pool(workers);
  pyc_root dut("owner", &pool);
  dut.Build();
  Golden golden;
  Row r{"OWNER", false};
  rawDrive(dut, r);
  dut.Reset();
  dut.Xfer();
  std::deque<Planes> ledger;
  unsigned accepted = 0, retired = 0;
  auto commit = [&](Row current) {
    rawDrive(dut, current);
    dut.Work();
    const auto expected = golden.read(current.take);
    check(dut.result.packed(), expected);
    if (current.clock && !golden.clock) {
      if (expected.valid && current.take == '1') {
        require(!ledger.empty());
        same(gfsim::extract<6>(dut.result.packed(), 0), ledger.front());
        ledger.pop_front();
        ++retired;
      }
      if (expected.ready == '1' && current.valid == '1') {
        ledger.push_back(completed(current.data));
        ++accepted;
      }
    }
    dut.Xfer();
    golden.commit(current);
  };
  auto edge = [&](Planes p, char valid, char take = '0') {
    commit({"OWNER", false, '0', valid, take, p});
    commit({"OWNER", true, '0', valid, take, p});
  };
  edge({38}, '1');
  edge({13}, '1');
  edge({19}, '1');
  edge({30}, '1');
  require(golden.occupancy() == 4 && golden.feedback->value == 5);
  r = {"OWNER", false};
  commit(r);
  r.clock = true;
  r.take = mode == 1 ? 'x' : '1';
  rawDrive(dut, r);
  bool failed = false;
  if (mode == 2)
    dut.Reset();
  else
    try {
      dut.Work();
    } catch (const gfsim::FourStateViolation &) {
      failed = true;
    }
  require(failed == (mode == 1));
  dut.DiscardNext();
  dut.Xfer();
  r.take = '1';
  commit(r); // Same edge reprepare; the pending output retires once.
  commit(r); // Held high cannot retire another token or decrement again.
  for (unsigned i = 0; i < 8; ++i)
    edge({}, '0', '1');
  require(accepted == 4 && retired == 4 && ledger.empty() &&
          golden.occupancy() == 0);
  std::cout << "OWNER " << mode
            << " old-Q/all-three-owner discard and exact-once drain\n";
}
void recovery(pyc_dut &dut, gfsim::SimExecutor &exec, unsigned label) {
  Row r{"RECOVERY", false, '0', '1', '1', {5}}; // remaining1, stop0, skip1.
  auto observe = [&](Expected expected) {
    dut.drive(ports(r));
    step(exec);
    check(dut.sample().result.packed(), expected);
  };
  observe({'1', false, {}});
  r.clock = true;
  observe({'1', false, {}}); // E0: one actual source acceptance.
  unsigned retired = 0;
  r.valid = '0';
  for (unsigned edge = 1; edge <= 4; ++edge) {
    const bool available = edge == 3;
    const Expected expected{'1', available, available ? Planes{1} : Planes{}};
    r.clock = false;
    observe(expected);
    r.clock = true;
    observe(expected); // E1 decrement, E2 exit push, E3 actual retirement.
    retired += symbol(dut.sample().result.packed(), 6) == '1';
  }
  require(retired == 1);
  observe({'1', false, {}}); // Held high and drained: no repeated retirement.
  std::cout << "RECOVERY " << label
            << " known one-iteration token accepted and retired exactly once\n";
}
void terminalCases(unsigned workers) {
  pyc_dut dut(workers);
  gfsim::SimExecutor exec(dut.system(), dut.observations(), {});
  configure(exec);
  unsigned failures = 0;
  for (unsigned i = 0; i < 4096; ++i)
    for (unsigned latent = 0; latent < 2; ++latent) {
      const Planes p = symbolic(i, latent);
      if (continuation(p) != 'x')
        continue;
      Row r{"NEGATIVE", false};
      dut.drive(ports(r));
      require(exec.Reset() == PYCIRCUIT_MODEL_STATUS_V1_OK);
      step(exec);
      r.clock = true;
      r.valid = '1';
      r.data = p;
      dut.drive(ports(r));
      step(exec);
      r.clock = false;
      r.valid = '0';
      dut.drive(ports(r));
      step(exec);
      const auto epoch = exec.cycles();
      r.clock = true;
      dut.drive(ports(r));
      PycircuitModelStepResultV1 s{sizeof(s)};
      require(exec.Step(&s) == PYCIRCUIT_MODEL_STATUS_V1_RUNTIME_FAILURE &&
              s.state == PYCIRCUIT_MODEL_STEP_V1_FAILED &&
              exec.cycles() == epoch && dut.system().cycle() == epoch);
      require(exec.Step(&s) == PYCIRCUIT_MODEL_STATUS_V1_INVALID_STATE);
      bool unavailable = false;
      try {
        (void)dut.sample();
      } catch (const std::logic_error &) {
        unavailable = true;
      }
      require(unavailable);
      r = {"NEGATIVE", false};
      dut.drive(ports(r));
      require(exec.Reset() == PYCIRCUIT_MODEL_STATUS_V1_OK);
      step(exec);
      check(dut.sample().result.packed(), {'1', false, {}});
      ++failures;
    }
  require(failures == 6000);
  recovery(dut, exec, 0);
  std::cout << "NEGATIVE_EXHAUSTIVE 4096 2 2192 " << failures
            << " terminal Reset recovery\n";
  for (unsigned bad = 5; bad <= 7; ++bad) {
    Row r{"CONTROL", false};
    dut.drive(ports(r));
    require(exec.Reset() == PYCIRCUIT_MODEL_STATUS_V1_OK);
    step(exec);
    if (bad == 7) {
      for (unsigned p : {38u, 13u, 19u, 30u}) {
        r = {"CONTROL", false, '0', '1', '0', {p}};
        dut.drive(ports(r));
        step(exec);
        r.clock = true;
        dut.drive(ports(r));
        step(exec);
      }
      r = {"CONTROL", false};
      dut.drive(ports(r));
      step(exec);
    }
    r.clock = true;
    if (bad == 5)
      r.reset = 'x';
    if (bad == 6)
      r.valid = 'z';
    if (bad == 7)
      r.take = 'x';
    const auto epoch = exec.cycles();
    dut.drive(ports(r));
    PycircuitModelStepResultV1 s{sizeof(s)};
    require(exec.Step(&s) == PYCIRCUIT_MODEL_STATUS_V1_RUNTIME_FAILURE &&
            s.state == PYCIRCUIT_MODEL_STEP_V1_FAILED &&
            exec.cycles() == epoch && dut.system().cycle() == epoch);
    require(exec.Step(&s) == PYCIRCUIT_MODEL_STATUS_V1_INVALID_STATE);
    bool unavailable = false;
    try {
      (void)dut.sample();
    } catch (const std::logic_error &) {
      unavailable = true;
    }
    require(unavailable);
    r = {"CONTROL", false};
    dut.drive(ports(r));
    require(exec.Reset() == PYCIRCUIT_MODEL_STATUS_V1_OK);
    step(exec);
    check(dut.sample().result.packed(), {'1', false, {}});
    recovery(dut, exec, bad);
    std::cout << "NEGATIVE_CONTROL " << bad << " terminal Reset recovery\n";
  }
}
void extensions(unsigned workers) {
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
  c.finish(true);
  for (unsigned mode = 0; mode < 3; ++mode)
    owner(workers, mode);
  terminalCases(workers);
}
int main(int argc, char **argv) {
  gfsim::SystemRunner runner(argc, argv);
  if (!runner.ready())
    return 2;
  pyc_dut dut(runner.workers());
  static_assert(decltype(dut.sample().result)::width == 8);
  Context c{dut, stimulus(false)};
  const gfsim::RunnerCallbacks callbacks{&c, &Context::initialize,
                                         &Context::drive, &Context::sample};
  const int status =
      runner.Run(dut.system(), dut.observations(), {}, callbacks);
  require(status == 0);
  c.finish(false);
  extensions(runner.workers());
  return status;
}
