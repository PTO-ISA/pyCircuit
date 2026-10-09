#include "gfsim/SimExecutor.h"
#include "gfsim/SystemRunner.h"
#include "pycircuit_system.hpp"
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <source_location>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
void require(bool condition,
             std::source_location at = std::source_location::current()) {
  if (!condition) {
    std::cerr << "onehot_encode independent oracle at " << at.line()
              << '\n';
    std::abort();
  }
}
constexpr unsigned Width = 13;
struct Planes {
  std::string value, known, z;
  bool operator==(const Planes &) const = default;
};
Planes token(std::string_view symbols, bool latent = false) {
  Planes p;
  for (unsigned i = 0; i < symbols.size(); ++i) {
    char c = symbols[i];
    require(c == '0' || c == '1' || c == 'x' || c == 'z');
    p.value += c == '1' || ((c == 'x' || c == 'z') && latent) ? '1' : '0';
    p.known += c == '0' || c == '1' ? '1' : '0';
    p.z += c == 'z' ? '1' : '0';
  }
  return p;
}
Planes zero() { return token(std::string(Width, '0')); }
// Independent per-position ternary equation, not the DUT scan/mask lowering.
// The first eight positions are copied strictly; all five derived bits are fresh.
Planes transform(const Planes &p) {
  std::string index = "000";
  unsigned asserted = 0;
  bool uncertain = false;
  for (unsigned position = 8; position-- > 0;) {
    const unsigned i = 7 - position;
    char symbol = p.z[i] == '1' ? 'z'
                : p.known[i] == '0' ? 'x' : p.value[i];
    const std::string choice = {char('0' + ((position >> 2) & 1)),
                               char('0' + ((position >> 1) & 1)),
                               char('0' + (position & 1))};
    if (symbol == '1') {
      index = choice;
      ++asserted;
    } else if (symbol == 'x' || symbol == 'z') {
      uncertain = true;
      for (unsigned bit = 0; bit < 3; ++bit)
        if (choice[bit] != index[bit])
          index[bit] = 'x';
    }
  }
  auto derived = token(index + (asserted ? "1" : uncertain ? "x" : "0") +
                       (uncertain ? "x" : asserted > 1 ? "1" : "0"));
  Planes result = p;
  result.value.replace(8, 5, derived.value);
  result.known.replace(8, 5, derived.known);
  result.z.replace(8, 5, derived.z);
  return result;
}

std::string visible(const Planes &p) {
  std::string s;
  for (unsigned i = 0; i < p.value.size(); ++i)
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
  return gfsim::wire<gfsim::Bits<Width>>::fromPacked(
      gfsim::FourState<Width>::fromMasks(
          bits<Width>(p.value), bits<Width>(p.known), bits<Width>(p.z)));
}
template <unsigned W> auto known(unsigned n) {
  return gfsim::wire<gfsim::Bits<W>>::known(gfsim::Bits<W>{n});
}
std::string binary(std::uint64_t value, unsigned width) {
  std::string result;
  for (unsigned bit = width; bit; --bit)
    result += (value >> (bit - 1)) & 1 ? '1' : '0';
  return result;
}
const std::vector<std::string> knownVectors = [] {
  std::vector<std::string> values;
  for (unsigned flags = 0; flags < 256; ++flags)
    for (unsigned previous = 0; previous < 32; ++previous)
      values.push_back(binary((flags << 5) | previous, Width));
  require(values.size() == 8192);
  return values;
}();
std::vector<Planes> fourVectors() {
  std::vector<Planes> values;
  for (unsigned base : {0u, 255u, 0xa5u})
    for (unsigned bit = 0; bit < 8; ++bit)
      for (char c : {'x', 'z'})
        for (bool latent : {false, true}) {
          auto p = binary((base << 5) | 0x15u, Width);
          p[7 - bit] = c;
          values.push_back(token(p, latent));
        }
  require(values.size() == 96);
  for (std::string_view literal :
       {"x0000100", "z0000100", "1xxxxxxx", "01zzzzzz", "0000010x",
        "0000010z", "xxxxxxxx", "zzzzzzzz", "xzxzxzxz", "001x010z",
        "00000000", "11111111"})
    for (bool latent : {false, true})
      values.push_back(token(std::string(literal) + "10101", latent));
  require(values.size() == 120);
  // Every old derived bit, including the physical valid/conflict fields,
  // is overwritten for each carrier symbol and both latent alternatives.
  for (unsigned base : {0u, 255u, 0x84u})
    for (unsigned bit = 0; bit < 5; ++bit)
      for (char c : {'0', '1', 'x', 'z'})
        for (bool latent : {false, true}) {
          auto p = binary((base << 5) | 0x15u, Width);
          p[12 - bit] = c;
          values.push_back(token(p, latent));
        }
  require(values.size() == 240);
  for (unsigned pattern = 0; pattern < 4; ++pattern)
    for (bool latent : {false, true}) {
      std::string p;
      for (unsigned b = 0; b < Width; ++b)
        p += pattern == 0   ? 'x'
             : pattern == 1 ? 'z'
             : pattern == 2 ? "xz"[b % 2]
                            : "01xz"[b % 4];
      values.push_back(token(p, latent));
    }
  require(values.size() == 248);
  return values;
}

struct Row {
  unsigned clock, reset, valid;
  Planes data;
  unsigned take;
};
std::vector<Row> stimulus(bool four) {
  std::vector<Row> rows;
  auto add = [&](unsigned c, unsigned r, unsigned v, const Planes &p,
                 unsigned t) { rows.push_back({c, r, v, p, t}); };
  auto edge = [&](const Planes &p, unsigned v = 1, unsigned t = 1,
                  unsigned r = 0) {
    add(1, r, v, p, t);
    add(0, r, v, p, t);
  };
  add(0, 1, 0, zero(), 0);
  edge(zero(), 0, 0, 1);
  // E0 captures, E1 transforms, E2 retires a known zero token.
  edge(token(binary(31, Width)));
  edge(zero(), 0);
  edge(zero(), 0);
  for (unsigned i = 0; i < 5; ++i)
    edge(token(knownVectors[i]), 1, 0);
  add(1, 0, 1, token(knownVectors[5]), 0);
  add(1, 0, 1, token(knownVectors[6]), 1);
  add(0, 0, 1, token(knownVectors[7]), 1);
  add(0, 0, 1, token(knownVectors[8]), 1);
  edge(token(knownVectors[9]));
  edge(token(knownVectors[10]));
  edge(zero(), 1, 0, 1);
  if (four) {
    // Occupancy depends on tokens even when every payload bit is X or Z.
    edge(token(std::string(Width, 'x')), 1, 0);
    edge(token(std::string(Width, 'z'), true), 1, 0);
    edge(zero(), 1, 0);
    add(1, 0, 1, token(std::string(Width, 'x'), true), 0);
    add(1, 1, 1, zero(), 1); // held-high reset must not commit
    add(0, 0, 1, token(std::string(Width, '1')), 1);
    add(0, 0, 0, zero(), 0);
    edge(token(knownVectors[0]));
    edge(zero(), 0);
    edge(zero(), 0);
    for (const auto &p : fourVectors())
      edge(p);
    // Recover to known flags after uncertain tokens, including zero/allones.
    edge(token(binary(31, Width)));
    edge(token(binary(255u << 5, Width)));
  } else {
    for (const auto &s : knownVectors)
      edge(token(s));
  }
  edge(zero(), 0, 1);
  edge(zero(), 0, 1);
  edge(zero(), 0, 1);
  edge(token(knownVectors[0]), 1, 0);
  edge(token(knownVectors[1]), 1, 0);
  edge(zero(), 1, 1, 1);
  require(rows.size() == (four ? 557 : 16425));
  return rows;
}
struct Expected {
  bool ready, valid;
  Planes data;
};
struct Identity {
  std::uint64_t id;
  Planes data;
  std::uint64_t birth;
};
class Golden {
  std::deque<Identity> first, second;
  bool last = false;
  std::uint64_t edge = 0, next = 0;

public:
  Expected output(bool take) const {
    bool room = second.empty() || take;
    return {first.empty() || room, !second.empty(),
            second.empty() ? zero() : second.front().data};
  }
  void commit(const Row &r) {
    if (r.clock && !last) {
      if (r.reset) {
        first.clear();
        second.clear();
        edge = 0;
      } else {
        bool pop = !second.empty() && r.take,
             move = !first.empty() && (second.empty() || pop),
             push = r.valid && (first.empty() || move);
        if (pop) {
          require(edge >= second.front().birth + 2);
          second.pop_front();
        }
        if (move) {
          auto t = first.front();
          first.pop_front();
          t.data = transform(t.data);
          second.push_back(t);
        }
        if (push)
          first.push_back({next++, r.data, edge});
        require(first.size() <= 1 && second.size() <= 1);
        ++edge;
      }
    }
    last = r.clock;
  }
};

pyc_dut::Inputs inputs(const Row &r) {
  pyc_dut::Inputs p;
  static_assert(decltype(p.data)::width == 13);
  p.pyc_7079635f636c6b = known<1>(r.clock);
  p.pyc_7079635f727374 = known<1>(r.reset);
  p.valid = known<1>(r.valid);
  p.take = known<1>(r.take);
  p.data = decltype(p.data)::fromPacked(wire(r.data).packed());
  return p;
}
template <class Packed>
void checkPayload(const Packed &actual, const Planes &expected) {
  auto valueMask = std::string(Width, '1');
  // Computed unknown helper bits have no contractual latent value.
  // All copied fields retain their complete value, known and Z planes.
  for (unsigned i = 8; i < Width; ++i)
    if (expected.known[i] == '0')
      valueMask[i] = '0';
  const auto mask = bits<Width>(valueMask);
  require((actual.value() & mask) == (bits<Width>(expected.value) & mask));
  require(actual.knownMask() == bits<Width>(expected.known));
  require(actual.zMask() == bits<Width>(expected.z));
}
struct Context {
  pyc_dut &dut;
  std::vector<Row> rows;
  Golden golden;
  std::deque<Identity> history;
  std::uint64_t nextIdentity = 0, edge = 0;
  unsigned sampled = 0, stalled = 0, accepted = 0, retired = 0, dropped = 0,
           peak = 0, replacements = 0;
  bool last = false;
  void observe(std::string_view label) {
    const auto &r = rows[sampled];
    const auto e = golden.output(r.take);
    static_assert(decltype(dut.sample().result)::width == 15);
    const auto out = dut.sample().result.packed();
    require(out.knownMask().bit(Width + 1) && out.knownMask().bit(Width));
    require(!out.zMask().bit(Width + 1) && !out.zMask().bit(Width));
    require(out.value().bit(Width + 1) == e.ready &&
            out.value().bit(Width) == e.valid);
    const bool actualReady = out.value().bit(14);
    const bool actualValid = out.value().bit(13);
    const auto data = gfsim::extract<Width>(out, 0);
    checkPayload(data, e.data);
    std::cout << label << ' ' << sampled << ' ' << unsigned(e.ready) << ' '
              << unsigned(e.valid) << ' ' << visible(e.data) << '\n';
    stalled += r.valid && !e.ready;
    if (r.clock && !last) {
      if (r.reset) {
        dropped += history.size();
        history.clear();
        edge = 0;
      } else {
        bool full = history.size() == 2;
        if (actualValid && r.take) {
          require(!history.empty());
          checkPayload(data, history.front().data);
          require(edge >= history.front().birth + 2);
          history.pop_front();
          ++retired;
        }
        if (actualReady && r.valid) {
          history.push_back({nextIdentity++, transform(r.data), edge});
          ++accepted;
          if (full)
            ++replacements;
        }
      }
      if (!r.reset)
        ++edge;
      peak = std::max(peak, unsigned(history.size()));
      require(history.size() <= 2);
    }
    last = r.clock;
    golden.commit(r);
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
    require(sampled == rows.size() && stalled >= 3 && peak == 2 &&
            replacements >= 2);
    require(history.empty() && dropped >= 2 && accepted == retired + dropped);
    std::cout << "HISTORY " << accepted << ' ' << retired << ' ' << dropped
              << ' ' << history.size() << ' ' << peak << '\n';
  }
};
void fourState(unsigned workers) {
  pyc_dut dut(workers);
  gfsim::SimExecutor exec(dut.system(), dut.observations(), {});
  constexpr std::string_view config =
      R"({"deadlock_window":null,"max_domain_cycles":{},"max_ticks":20000,"schema":"pycircuit-model-config","version":"1"})";
  require(
      exec.ConfigureJson(reinterpret_cast<const std::uint8_t *>(config.data()),
                         config.size()) == PYCIRCUIT_MODEL_STATUS_V1_OK);
  Context c{dut, stimulus(true)};
  dut.drive(inputs(c.rows[0]));
  require(exec.Reset() == PYCIRCUIT_MODEL_STATUS_V1_OK);
  for (const auto &r : c.rows) {
    dut.drive(inputs(r));
    PycircuitModelStepResultV1 status{sizeof(status)};
    require(exec.Step(&status) == PYCIRCUIT_MODEL_STATUS_V1_OK);
    require(status.state == PYCIRCUIT_MODEL_STEP_V1_RUNNING);
    c.observe("FOUR");
  }
  c.finish();
}
void directAndTerminal(unsigned workers) {
  Row r{0, 0, 0, token(binary(31, Width)), 0};
  auto drive = [&](pyc_root &root, const Row &x) {
    auto p = inputs(x);
    root.pyc_7079635f636c6b = p.pyc_7079635f636c6b;
    root.pyc_7079635f727374 = p.pyc_7079635f727374;
    root.valid = p.valid;
    root.data = p.data;
    root.take = p.take;
  };
  gfsim::WorkExecutor pool(workers);
  pyc_root root("discard", &pool);
  root.Build();
  drive(root, r);
  root.Reset();
  root.Xfer();
  r.clock = 1;
  r.valid = 1;
  drive(root, r);
  root.Work();
  root.Xfer();
  r.clock = 0;
  r.valid = 0;
  drive(root, r);
  root.Work();
  root.Xfer();
  r.clock = 1;
  drive(root, r);
  root.Work();
  root.DiscardNext();
  root.Xfer();
  root.Work();
  root.Xfer();
  root.Work();
  auto observed = root.result.packed();
  require(observed.value().bit(13));
  checkPayload(gfsim::extract<Width>(observed, 0), transform(r.data));
  root.Xfer();
  unsigned retired = 0;
  r.take = 1;
  for (unsigned i = 0; i < 4; ++i) {
    r.clock = 0;
    drive(root, r);
    root.Work();
    root.Xfer();
    r.clock = 1;
    drive(root, r);
    root.Work();
    if (root.result.packed().value().bit(13))
      ++retired;
    root.Xfer();
  }
  require(retired == 1);
  r = {0, 0, 0, token(binary((0x84u << 5) | 0x1fu, Width)), 0};
  drive(root, r);
  root.Reset();
  root.Xfer();
  r.clock = 1;
  r.valid = 1;
  drive(root, r);
  root.Work();
  root.Xfer();
  r.clock = 0;
  r.valid = 0;
  drive(root, r);
  root.Work();
  root.Xfer();
  r.clock = 1;
  drive(root, r);
  root.valid = gfsim::wire<gfsim::Bits<1>>::unknown();
  bool failed = false;
  try {
    root.Work();
  } catch (const std::exception &) {
    failed = true;
  }
  require(failed);
  root.DiscardNext();
  root.Xfer(); // Even an erroneous later Xfer must publish neither proposal.
  r.clock = 0;
  drive(root, r);
  root.Work();
  require(!root.result.packed().value().bit(Width));
  checkPayload(gfsim::extract<Width>(root.result.packed(), 0), zero());
  root.Xfer();
  r.clock = 1;
  drive(root, r);
  root.Work();
  root.Xfer();
  r.clock = 0;
  drive(root, r);
  root.Work();
  require(root.result.packed().value().bit(Width));
  checkPayload(gfsim::extract<Width>(root.result.packed(), 0), transform(r.data));
  root.Xfer();
  retired = 0;
  r.take = 1;
  for (unsigned i = 0; i < 4; ++i) {
    r.clock = 1;
    drive(root, r);
    root.Work();
    retired += root.result.packed().value().bit(Width);
    root.Xfer();
    r.clock = 0;
    drive(root, r);
    root.Work();
    root.Xfer();
  }
  require(retired == 1);
  std::cout << "OWNER discard/reprepare; failed whole-owner proposals commit zero; exact one result\n";
  pyc_dut dut(workers);
  gfsim::SimExecutor exec(dut.system(), dut.observations(), {});
  constexpr std::string_view cfg =
      R"({"deadlock_window":null,"max_domain_cycles":{},"max_ticks":20000,"schema":"pycircuit-model-config","version":"1"})";
  require(exec.ConfigureJson(reinterpret_cast<const std::uint8_t *>(cfg.data()),
                             cfg.size()) == PYCIRCUIT_MODEL_STATUS_V1_OK);
  r = {0, 0, 0, zero(), 0};
  dut.drive(inputs(r));
  require(exec.Reset() == PYCIRCUIT_MODEL_STATUS_V1_OK);
  PycircuitModelStepResultV1 status{sizeof(status)};
  require(exec.Step(&status) == PYCIRCUIT_MODEL_STATUS_V1_OK);
  r.clock = 1;
  r.valid = 1;
  r.data = token(binary((0x81u << 5) | 31u, Width));
  dut.drive(inputs(r));
  require(exec.Step(&status) == PYCIRCUIT_MODEL_STATUS_V1_OK);
  r.clock = 0;
  r.valid = 0;
  dut.drive(inputs(r));
  require(exec.Step(&status) == PYCIRCUIT_MODEL_STATUS_V1_OK);
  auto epoch = exec.cycles();
  r.clock = 1;
  r.valid = 1;
  auto p = inputs(r);
  p.valid = gfsim::wire<gfsim::Bits<1>>::unknown();
  dut.drive(p);
  require(exec.Step(&status) == PYCIRCUIT_MODEL_STATUS_V1_RUNTIME_FAILURE &&
          status.state == PYCIRCUIT_MODEL_STEP_V1_FAILED &&
          status.epoch_time == epoch && exec.cycles() == epoch);
  require(exec.Step(&status) == PYCIRCUIT_MODEL_STATUS_V1_INVALID_STATE);
  bool unavailable = false;
  try {
    (void)dut.sample();
  } catch (const std::logic_error &) {
    unavailable = true;
  }
  require(unavailable);
  r = {0, 0, 0, zero(), 0};
  dut.drive(inputs(r));
  require(exec.Reset() == PYCIRCUIT_MODEL_STATUS_V1_OK);
  require(exec.Step(&status) == PYCIRCUIT_MODEL_STATUS_V1_OK);
  const auto recovered = dut.sample().result.packed();
  require(recovered.knownMask() == gfsim::Bits<15>{0x7fff} &&
          recovered.zMask() == gfsim::Bits<15>{0} &&
          recovered.value() == gfsim::Bits<15>{0x4000});
  checkPayload(gfsim::extract<Width>(recovered, 0), zero());
  std::cout << "NEGATIVE terminal unknown handshake; unavailable sample; Reset "
               "recovery\n";
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
  fourState(runner.workers());
  directAndTerminal(runner.workers());
  return status;
}
