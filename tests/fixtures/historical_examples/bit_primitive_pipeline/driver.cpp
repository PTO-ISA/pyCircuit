#include "gfsim/SimExecutor.h"
#include "gfsim/SystemRunner.h"
#include "pycircuit_system.hpp"
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <optional>
#include <source_location>
#include <string>
#include <string_view>
#include <vector>
void require(bool condition,
             std::source_location at = std::source_location::current()) {
  if (!condition) {
    std::cerr << "bit_primitive_pipeline independent oracle at " << at.line()
              << '\n';
    std::abort();
  }
}
constexpr unsigned Width = 28;
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
// Scalar-symbol oracle: ordered ternary choices and endpoint positions.
// None of these equations use the DUT's mask/scan/popcount composition.
char symbol(const Planes &p, unsigned i) {
  return p.z[i] == '1' ? 'z' : p.known[i] == '0' ? 'x' : p.value[i];
}
std::string priority(const std::string &s, bool high) {
  std::string answer = "000";
  for (unsigned step = 0; step < 8; ++step) {
    const unsigned position = high ? step : 7 - step;
    const char predicate = s[7 - position];
    std::string choice;
    for (unsigned bit = 3; bit; --bit)
      choice += (position >> (bit - 1)) & 1 ? '1' : '0';
    if (predicate == '1')
      answer = choice;
    else if (predicate == 'x' || predicate == 'z')
      for (unsigned i = 0; i < 3; ++i)
        if (answer[i] != choice[i])
          answer[i] = 'x';
  }
  return answer;
}
std::string endpoint(const std::string &s, bool leading) {
  unsigned distance = 8, uncertain = 0;
  bool endpointUnknown = false;
  for (unsigned position = 0; position < 8; ++position) {
    char c = s[leading ? position : 7 - position];
    if (c == '1') {
      distance = position;
      break;
    }
    if (c == 'x' || c == 'z') {
      ++uncertain;
      endpointUnknown |= position == 0;
    }
  }
  unsigned poisoned = uncertain ? 4 : 0;
  if (endpointUnknown && uncertain == 1) {
    poisoned = 0;
    for (unsigned n = distance; n; n >>= 1)
      ++poisoned;
  }
  std::string answer;
  for (unsigned bit = 4; bit; --bit)
    answer +=
        bit - 1 < poisoned ? 'x' : ((distance >> (bit - 1)) & 1 ? '1' : '0');
  return answer;
}
Planes transform(const Planes &p) {
  std::string source;
  unsigned population = 0;
  bool uncertain = false;
  for (unsigned i = 0; i < 8; ++i) {
    const char c = symbol(p, i);
    source += c;
    population += c == '1';
    uncertain |= c == 'x' || c == 'z';
  }
  const char valid = population ? '1' : uncertain ? 'x' : '0';
  const char conflict = uncertain ? 'x' : population > 1 ? '1' : '0';
  std::string count;
  for (unsigned bit = 4; bit; --bit)
    count += uncertain ? 'x' : ((population >> (bit - 1)) & 1 ? '1' : '0');
  const auto derived = token(priority(source, false) + priority(source, true) +
                             valid + conflict + count + endpoint(source, true) +
                             endpoint(source, false));
  Planes result = p;
  result.value.replace(8, 20, derived.value);
  result.known.replace(8, 20, derived.known);
  result.z.replace(8, 20, derived.z);
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
// All source bytes; focused Cartesian old-index and old-count sweeps.
const std::vector<std::string> knownVectors = [] {
  std::vector<std::string> values;
  for (unsigned value = 0; value < 256; ++value)
    values.push_back(
        binary((value << 20) | ((value * 7919u + 0xabcdeu) & 0xfffffu), Width));
  for (unsigned value : {0u, 1u, 128u, 129u, 255u})
    for (unsigned low = 0; low < 8; ++low)
      for (unsigned high = 0; high < 8; ++high)
        values.push_back(binary((value << 20) | (low << 17) | (high << 14) |
                                    ((low & 1) << 13) | ((high & 1) << 12) |
                                    0xa5bu,
                                Width));
  for (unsigned value : {0u, 1u, 128u, 129u, 255u})
    for (unsigned pop = 0; pop < 16; ++pop)
      for (unsigned leading = 0; leading < 16; ++leading)
        for (unsigned trailing = 0; trailing < 16; ++trailing)
          values.push_back(binary((value << 20) | (5u << 17) | (3u << 14) |
                                      (((pop + leading) & 1) << 13) |
                                      (((pop + trailing) & 1) << 12) |
                                      (pop << 8) | (leading << 4) | trailing,
                                  Width));
  require(values.size() == 21056);
  return values;
}();
std::vector<Planes> fourVectors() {
  std::vector<Planes> values;
  for (unsigned base : {0u, 255u, 0xabu})
    for (unsigned bit = 0; bit < 8; ++bit)
      for (char c : {'x', 'z'})
        for (bool latent : {false, true}) {
          auto p = binary((base << 20) | 0xabcdeu, Width);
          p[7 - bit] = c;
          values.push_back(token(p, latent));
        }
  require(values.size() == 96);
  for (bool leading : {true, false})
    for (unsigned d = 1; d <= 8; ++d)
      for (char c : {'x', 'z'})
        for (bool latent : {false, true}) {
          auto p = binary(0xabcdeu, Width);
          p[leading ? 0 : 7] = c;
          if (d < 8)
            p[leading ? d : 7 - d] = '1';
          values.push_back(token(p, latent));
        }
  require(values.size() == 160);
  for (std::string_view literal :
       {"x0000100", "z0000100", "000001x0", "000001z0", "1xxxxxxx", "01zzzzzz",
        "x1111111", "x0111111", "00x11111", "xx111111", "00000000", "10101010"})
    for (bool reverse : {false, true})
      for (bool latent : {false, true}) {
        std::string p(literal);
        if (reverse)
          std::reverse(p.begin(), p.end());
        p += binary(0xabcdeu, 20);
        values.push_back(token(p, latent));
      }
  require(values.size() == 208);
  // Every old derived position is exercised as 0/1/X/Z, including old bool
  // storage.
  for (unsigned bit = 0; bit < 20; ++bit)
    for (char c : {'0', '1', 'x', 'z'})
      for (bool latent : {false, true}) {
        auto p = binary(((bit % 2 ? 255u : 0u) << 20) | 0x55555u, Width);
        p[27 - bit] = c;
        values.push_back(token(p, latent));
      }
  require(values.size() == 368);
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
  require(values.size() == 376);
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
  // E0 captures, E1 transforms, E2 retires a known zero result.
  edge(token(binary(0xfffffu, Width)));
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
    // Recover to known counts after uncertain tokens, including zero/allones.
    edge(token(binary(0xfffffu, Width)));
    edge(token(binary(255u << 20, Width)));
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
  require(rows.size() == (four ? 813 : 42153));
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
  static_assert(decltype(p.data)::width == 28);
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
  // Computed unknown arithmetic bits have no contractual latent value.
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
    static_assert(decltype(dut.sample().result)::width == 30);
    const auto out = dut.sample().result.packed();
    require(out.knownMask().bit(Width + 1) && out.knownMask().bit(Width));
    require(!out.zMask().bit(Width + 1) && !out.zMask().bit(Width));
    require(out.value().bit(Width + 1) == e.ready &&
            out.value().bit(Width) == e.valid);
    const bool actualReady = out.value().bit(29);
    const bool actualValid = out.value().bit(28);
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
      R"({"deadlock_window":null,"max_domain_cycles":{},"max_ticks":45000,"schema":"pycircuit-model-config","version":"1"})";
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
    R"({"deadlock_window":null,"max_domain_cycles":{},"max_ticks":45000,"schema":"pycircuit-model-config","version":"1"})";
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
  require(out.knownMask().bit(29) && out.knownMask().bit(28));
  require(!out.zMask().bit(29) && !out.zMask().bit(28));
  require(out.value().bit(29) == ready &&
          out.value().bit(28) == payload.has_value());
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
    const auto a = token(binary((0x81u << 20) | 0xabcdeu, Width));
    const auto b = token(binary((0x04u << 20) | 0x12345u, Width));
    const auto c = token(binary(0xfffffu, Width));
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
      if (out.value().bit(28)) {
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
    const auto a = token(binary(0x81u << 20, Width));
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
    require(out.value().bit(29) && !out.value().bit(28));
    checkPayload(gfsim::extract<Width>(out, 0), zero());
    // Successful token execution after Reset, rather than merely accepting
    // Reset.
    step({1, 0, 1, a, 1});
    step({0, 0, 0, zero(), 1});
    step({1, 0, 0, zero(), 1});
    step({0, 0, 0, zero(), 1});
    require(dut.sample().result.packed().value().bit(28));
    checkPayload(gfsim::extract<Width>(dut.sample().result.packed(), 0),
                 transform(a));
    step({1, 0, 0, zero(), 1});
    step({0, 0, 0, zero(), 1});
    require(!dut.sample().result.packed().value().bit(28));
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
