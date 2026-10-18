#include "gfsim/SimExecutor.h"
#include "gfsim/SystemRunner.h"
#include "pycircuit_system.hpp"
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <optional>
#include <utility>
#include <source_location>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
void require(bool condition,
             std::source_location at = std::source_location::current()) {
  if (!condition) {
    std::cerr << "frontend_composition_pipeline independent oracle at " << at.line()
              << '\n';
    std::abort();
  }
}
constexpr unsigned Width = 34;
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
// Independent scalar-symbol transport and ternary equations. No DUT helper,
// scan/mask lowering, record evaluator or Runtime queue supplies expectations.
Planes transform(const Planes &p) {
  require(p.value.size() == Width && p.known.size() == Width && p.z.size() == Width);
  std::string index = "00";
  unsigned asserted = 0;
  bool uncertain = false;
  for (unsigned position = 4; position-- > 0;) {
    const unsigned i = 20 - position; // packed flags[16:13]
    char symbol = p.z[i] == '1' ? 'z'
                : p.known[i] == '0' ? 'x' : p.value[i];
    const std::string choice = {char('0' + ((position >> 1) & 1)),
                               char('0' + (position & 1))};
    if (symbol == '1') {
      index = choice;
      ++asserted;
    } else if (symbol == 'x' || symbol == 'z') {
      uncertain = true;
      for (unsigned bit = 0; bit < 2; ++bit)
        if (choice[bit] != index[bit]) index[bit] = 'x';
    }
  }
  const auto opcode = token("1001");
  const auto status = token(index + (asserted ? "1" : uncertain ? "x" : "0") +
                            (uncertain ? "x" : asserted > 1 ? "1" : "0"));
  // High21 unchanged, next5 copied from patch[25:21], constantOpcode9, status4.
  return {p.value.substr(0,21) + p.value.substr(8,5) + opcode.value + status.value,
          p.known.substr(0,21) + p.known.substr(8,5) + opcode.known + status.known,
          p.z.substr(0,21) + p.z.substr(8,5) + opcode.z + status.z};
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
std::uint64_t packed(unsigned headerOpcode, unsigned headerTag,
                     unsigned patchTag, unsigned patchValid, unsigned opcode,
                     unsigned flags, unsigned old) {
  require(headerOpcode < 16 && headerTag < 16 && patchTag < 16 &&
          patchValid < 2 && opcode < 16 && flags < 16 && old < 8192);
  return (std::uint64_t{headerOpcode} << 30) |
         (std::uint64_t{headerTag} << 26) | (std::uint64_t{patchTag} << 22) |
         (std::uint64_t{patchValid} << 21) | (std::uint64_t{opcode} << 17) |
         (std::uint64_t{flags} << 13) | old;
}
const std::vector<std::string> knownVectors = [] {
  std::vector<std::string> values;
  // All flags16 x rawOpcode16 x patchTag16 x patchValid2. The odd multiplier
  // also visits every old13 combination once without exhausting all Item34.
  for (unsigned i = 0; i < 8192; ++i) {
    const unsigned flags = i / 512, opcode = (i / 32) % 16,
                   tag = (i / 2) % 16, valid = i % 2;
    values.push_back(binary(packed((flags * 3 + opcode) % 16,
                                  (tag + valid * 7) % 16, tag, valid,
                                  opcode, flags, (i * 73 + 0x152b) % 8192), Width));
  }
  // Every old field range across every flags control, including invalid old
  // nominalOpcode codes. Fixed widths and constants are independent stimuli.
  for (unsigned flags = 0; flags < 16; ++flags)
    for (auto [low, width] : {std::pair{9u,4u}, {8u,1u}, {4u,4u},
                             {2u,2u}, {1u,1u}, {0u,1u}})
      for (unsigned old = 0; old < (1u << width); ++old) {
        auto p = packed((flags + 3) % 16, (flags * 5) % 16, flags,
                        flags % 2, (flags + 7) % 16, flags, 0x152b);
        const auto mask = ((std::uint64_t{1} << width) - 1) << low;
        values.push_back(binary((p & ~mask) | (std::uint64_t{old} << low), Width));
      }
  // Every combination of the two preserved header fields independently.
  for (unsigned header = 0; header < 256; ++header)
    values.push_back(binary(packed(header / 16, header % 16, (header * 7) % 16,
                                  header % 2, (header * 3) % 16, header % 16,
                                  (header * 29) % 8192), Width));
  require(values.size() == 9120);
  return values;
}();
std::vector<Planes> fourVectors() {
  std::vector<Planes> values;
  // Every preserved high21 bit includes header, patch, raw nominal opcode and
  // flags. The patch is checked a second time at its derived copy positions.
  for (unsigned flags : {0u, 15u, 10u})
    for (unsigned bit = 13; bit < Width; ++bit)
      for (char c : {'x', 'z'})
        for (bool latent : {false, true}) {
          auto p = binary(packed(10,5,7,1,9,flags,0x152b), Width);
          p[Width - 1 - bit] = c;
          values.push_back(token(p, latent));
        }
  require(values.size() == 252);
  // Every old derived bit is overwritten, with symbols0/1/X/Z and two latent
  // alternatives; sparse and multi-hot/zero controls select different outputs.
  for (unsigned flags : {0u, 15u, 10u})
    for (unsigned bit = 0; bit < 13; ++bit)
      for (char c : {'0', '1', 'x', 'z'})
        for (bool latent : {false, true}) {
          auto p = binary(packed(10,5,7,1,9,flags,0x152b), Width);
          p[Width - 1 - bit] = c;
          values.push_back(token(p, latent));
        }
  require(values.size() == 564);
  for (std::string_view literal :
       {"x001", "z001", "1xxx", "01zz", "001x", "001z", "xxxx", "zzzz",
        "xzxz", "x10z", "0000", "1111"})
    for (bool latent : {false, true}) {
      auto p = binary(packed(10,5,7,1,9,0,0x152b), Width);
      p.replace(17, 4, literal);
      values.push_back(token(p, latent));
    }
  require(values.size() == 588);
  for (unsigned pattern = 0; pattern < 4; ++pattern)
    for (bool latent : {false, true}) {
      std::string p;
      for (unsigned b = 0; b < Width; ++b)
        p += pattern == 0 ? 'x' : pattern == 1 ? 'z' :
             pattern == 2 ? "xz"[b % 2] : "01xz"[b % 4];
      values.push_back(token(p, latent));
    }
  require(values.size() == 596);
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
    edge(token(binary(std::uint64_t{15} << 13, Width)));
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
  require(rows.size() == (four ? 1253 : 18281));
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
  static_assert(decltype(p.data)::width == 34);
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
  for (unsigned i = 30; i < Width; ++i)
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
    static_assert(decltype(dut.sample().result)::width == 36);
    const auto out = dut.sample().result.packed();
    require(out.knownMask().bit(Width + 1) && out.knownMask().bit(Width));
    require(!out.zMask().bit(Width + 1) && !out.zMask().bit(Width));
    require(out.value().bit(Width + 1) == e.ready &&
            out.value().bit(Width) == e.valid);
    const bool actualReady = out.value().bit(35);
    const bool actualValid = out.value().bit(34);
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

constexpr std::string_view probeConfig =
    R"({"deadlock_window":null,"max_domain_cycles":{},"max_ticks":20000,"schema":"pycircuit-model-config","version":"1"})";
void configure(gfsim::SimExecutor &exec) {
  require(exec.ConfigureJson(
              reinterpret_cast<const std::uint8_t *>(probeConfig.data()),
              probeConfig.size()) == PYCIRCUIT_MODEL_STATUS_V1_OK);
}
void driveRoot(pyc_root &root, const Row &r) {
  auto p = inputs(r);
  root.pyc_7079635f636c6b = p.pyc_7079635f636c6b;
  root.pyc_7079635f727374 = p.pyc_7079635f727374;
  root.valid = p.valid;
  root.data = p.data;
  root.take = p.take;
}
void checkRoot(pyc_root &root, bool ready,
               const std::optional<Planes> &payload) {
  const auto out = root.result.packed();
  require(out.knownMask().bit(35) && out.knownMask().bit(34));
  require(!out.zMask().bit(35) && !out.zMask().bit(34));
  require(out.value().bit(35) == ready &&
          out.value().bit(34) == payload.has_value());
  checkPayload(gfsim::extract<Width>(out, 0), payload ? *payload : zero());
}
void ownerProbes(unsigned workers) {
  for (unsigned mode = 0; mode < 3; ++mode) {
    // mode 0: discard a full replacement; mode 1: failed accept during
    // old-input move; mode 2: failed output pop while both owners contain
    // tokens.
    gfsim::WorkExecutor pool(workers);
    pyc_root root("owner", &pool);
    root.Build();
    const auto a = token(binary(packed(10,5,7,1,9,9,0x152b), Width));
    const auto b = token(binary(packed(3,12,2,0,6,4,0x1234), Width));
    const auto c = token(binary(packed(15,1,13,1,5,0,0x1fff), Width));
    Row r{0, 0, 0, zero(), 0};
    auto work = [&](const Row &row) {
      driveRoot(root, row);
      root.Work();
    };
    auto commit = [&](const Row &row) {
      work(row);
      root.Xfer();
    };
    driveRoot(root, r);
    root.Reset();
    root.Xfer();
    commit({1, 0, 1, a, 0});
    commit({0, 0, 0, zero(), 0});
    if (mode != 1) {
      commit({1, 0, 1, b, 0});
      commit({0, 0, 0, zero(), 0});
    }
    r = {1, 0, 1, c, mode == 1 ? 0u : 1u};
    driveRoot(root, r);
    if (mode == 1)
      root.valid = gfsim::wire<gfsim::Bits<1>>::unknown();
    if (mode == 2)
      root.take = gfsim::wire<gfsim::Bits<1>>::unknown();
    bool failed = false;
    try {
      root.Work();
    } catch (const gfsim::FourStateViolation &) {
      failed = true;
    }
    require(failed == (mode != 0));
    root.DiscardNext();
    root.Xfer();
    // Same rising level retries: failed/discarded proposals changed neither
    // owner nor clock.
    work({1, 0, mode == 1 ? 0u : 1u, c, mode == 1 ? 0u : 1u});
    checkRoot(root, true,
              mode == 1 ? std::nullopt : std::optional<Planes>(transform(a)));
    root.Xfer();
    work({1, 0, 0, zero(), 0});
    checkRoot(root, mode == 1, transform(mode == 1 ? a : b));
    root.Xfer();
    unsigned retired = 0;
    std::vector<Planes> expected =
        mode == 1 ? std::vector<Planes>{transform(a)}
                  : std::vector<Planes>{transform(b), transform(c)};
    for (unsigned i = 0; i < 4; ++i) {
      commit({0, 0, 0, zero(), 1});
      work({1, 0, 0, zero(), 1});
      const auto out = root.result.packed();
      if (out.value().bit(34)) {
        require(retired < expected.size());
        checkPayload(gfsim::extract<Width>(out, 0), expected[retired++]);
      }
      root.Xfer();
    }
    require(retired == expected.size());
    work({0, 0, 0, zero(), 0});
    checkRoot(root, true, std::nullopt);
    root.Xfer();
    std::cout << "OWNER " << mode
              << " both-owner zero-commit/discard/reprepare passed\n";
  }
}
void terminalProbes(unsigned workers) {
  for (unsigned mode = 0; mode < 2; ++mode) {
    pyc_dut dut(workers);
    gfsim::SimExecutor exec(dut.system(), dut.observations(), {});
    configure(exec);
    auto step = [&](const Row &r) {
      dut.drive(inputs(r));
      PycircuitModelStepResultV1 s{sizeof(s)};
      require(exec.Step(&s) == PYCIRCUIT_MODEL_STATUS_V1_OK);
    };
    const auto a = token(binary(packed(10,5,7,1,9,9,0x152b), Width));
    dut.drive(inputs({0, 0, 0, zero(), 0}));
    require(exec.Reset() == PYCIRCUIT_MODEL_STATUS_V1_OK);
    step({0, 0, 0, zero(), 0});
    step({1, 0, 1, a, 0});
    step({0, 0, 0, zero(), 0});
    if (mode) {
      step({1, 0, 1, a, 0});
      step({0, 0, 0, zero(), 0});
    }
    const auto epoch = exec.cycles();
    auto p = inputs({1, 0, 1, a, 1});
    if (mode)
      p.take = gfsim::wire<gfsim::Bits<1>>::unknown();
    else
      p.valid = gfsim::wire<gfsim::Bits<1>>::unknown();
    dut.drive(p);
    PycircuitModelStepResultV1 status{sizeof(status)};
    require(exec.Step(&status) == PYCIRCUIT_MODEL_STATUS_V1_RUNTIME_FAILURE &&
            status.state == PYCIRCUIT_MODEL_STEP_V1_FAILED &&
            exec.cycles() == epoch && dut.system().cycle() == epoch);
    require(exec.Step(&status) == PYCIRCUIT_MODEL_STATUS_V1_INVALID_STATE);
    bool unavailable = false;
    try {
      (void)dut.sample();
    } catch (const std::logic_error &) {
      unavailable = true;
    }
    require(unavailable);
    dut.drive(inputs({0, 0, 0, zero(), 0}));
    require(exec.Reset() == PYCIRCUIT_MODEL_STATUS_V1_OK);
    step({0, 0, 0, zero(), 0});
    const auto out = dut.sample().result.packed();
    require(out.value().bit(35) && !out.value().bit(34));
    checkPayload(gfsim::extract<Width>(out, 0), zero());
    // Successful token execution after Reset, rather than merely accepting
    // Reset.
    step({1, 0, 1, a, 1});
    step({0, 0, 0, zero(), 1});
    step({1, 0, 0, zero(), 1});
    step({0, 0, 0, zero(), 1});
    require(dut.sample().result.packed().value().bit(34));
    checkPayload(gfsim::extract<Width>(dut.sample().result.packed(), 0),
                 transform(a));
    step({1, 0, 0, zero(), 1});
    step({0, 0, 0, zero(), 1});
    require(!dut.sample().result.packed().value().bit(34));
    std::cout << "NEGATIVE " << mode
              << " terminal unknown handshake; unavailable sample; Reset "
                 "execution recovery\n";
  }
}

int main(int argc, char **argv) {
  // Custom mode is removed before the shared SystemRunner parses its own
  // options.
  std::string mode;
  std::vector<char *> runnerArgs{argv[0]};
  for (int i = 1; i < argc; ++i) {
    if (std::string_view(argv[i]) == "--probe-mode") {
      require(i + 1 < argc && mode.empty());
      mode = argv[++i];
    } else
      runnerArgs.push_back(argv[i]);
  }
  runnerArgs.push_back(nullptr);
  gfsim::SystemRunner runner(static_cast<int>(runnerArgs.size()) - 1,
                             runnerArgs.data());
  if (!runner.ready())
    return 2;
  if (!mode.empty()) {
    require(mode == "terminal");
    terminalProbes(runner.workers());
    return 0;
  }
  pyc_dut dut(runner.workers());
  Context c{dut, stimulus(false)};
  const gfsim::RunnerCallbacks callbacks{&c, &Context::initialize,
                                         &Context::drive, &Context::sample};
  const int status =
      runner.Run(dut.system(), dut.observations(), {}, callbacks);
  require(status == 0);
  c.finish();
  fourState(runner.workers());
  ownerProbes(runner.workers());
  return status;
}
