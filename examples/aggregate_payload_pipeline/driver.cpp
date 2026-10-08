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
#include <utility>
#include <vector>
void require(bool condition,
             std::source_location at = std::source_location::current()) {
  if (!condition) {
    std::cerr << "aggregate_payload_pipeline independent oracle at "
              << at.line() << '\n';
    std::abort();
  }
}
constexpr unsigned Width = 28;
struct Planes {
  std::string value, known, z;
  bool operator==(const Planes &) const = default;
};
Planes token(std::string_view symbols, bool alternate = false) {
  Planes p;
  for (unsigned i = 0; i < symbols.size(); ++i) {
    char c = symbols[i];
    require(c == '0' || c == '1' || c == 'x' || c == 'z');
    p.value += c == '1' || ((c == 'x' || c == 'z') && (i % 3 == 0) != alternate)
                   ? '1'
                   : '0';
    p.known += c == '0' || c == '1' ? '1' : '0';
    p.z += c == 'z' ? '1' : '0';
  }
  return p;
}
Planes zero() { return token(std::string(Width, '0')); }
// Independent packed model: pair fields occupy high 3/5 bits; lane zero
// is most significant. Copies preserve every latent value/known/Z bit.
Planes transform(const Planes &p) {
  Planes result;
  for (auto [start, count] : {std::pair{0u, 3u}, std::pair{3u, 5u}}) {
    bool allKnown = true;
    unsigned value = 0;
    for (unsigned i = start; i < start + count; ++i) {
      allKnown &= p.known[i] == '1';
      value = (value << 1) | (p.value[i] == '1');
    }
    value = (value + 1) % (1u << count);
    for (unsigned i = 0; i < count; ++i) {
      result.value += allKnown && ((value >> (count - 1 - i)) & 1) ? '1' : '0';
      result.known += allKnown ? '1' : '0';
      result.z += '0';
    }
  }
  for (unsigned start : {12u, 16u, 20u, 8u, 16u}) {
    result.value += p.value.substr(start, 4);
    result.known += p.known.substr(start, 4);
    result.z += p.z.substr(start, 4);
  }
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
  for (unsigned first = 0; first < 8; ++first)
    for (unsigned second = 0; second < 32; ++second) {
      const unsigned lanes =
          (((first + 1) & 15) << 12) | (((second + 3) & 15) << 8) |
          (((first + second + 5) & 15) << 4) | ((second + 7) & 15);
      values.push_back(
          binary((((first << 5) | second) << 20) | (lanes << 4) | (second & 15),
                 Width));
    }
  values.emplace_back(Width, '0');
  values.emplace_back(Width, '1');
  for (unsigned bit = 0; bit < Width; ++bit)
    values.push_back(binary(std::uint64_t{1} << bit, Width));
  return values;
}();
std::vector<Planes> fourVectors() {
  std::vector<Planes> values;
  for (unsigned bit = 0; bit < Width; ++bit)
    for (char symbol : {'x', 'z'})
      for (bool alternate : {false, true}) {
        auto value = binary(0x2a5abcdu & ((1u << Width) - 1), Width);
        value[Width - 1 - bit] = symbol;
        values.push_back(token(value, alternate));
      }
  for (unsigned pattern = 0; pattern < 4; ++pattern)
    for (bool alternate : {false, true}) {
      std::string value;
      for (unsigned bit = 0; bit < Width; ++bit)
        value += pattern == 0   ? 'x'
                 : pattern == 1 ? 'z'
                 : pattern == 2 ? "xz"[bit % 2]
                                : "01xz"[bit % 4];
      values.push_back(token(value, alternate));
    }
  return values;
}
struct Row {
  unsigned clock, reset, valid;
  Planes data;
  unsigned take;
  unsigned original = 0, cycle = 0;
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
  // Exact original backend history, with post-edge state reevaluation.
  constexpr unsigned backendInput =
      (((5u << 5) | 29u) << 20) | (0x1234u << 4) | 15u;
  for (unsigned cycle = 0; cycle < 10; ++cycle) {
    edge(token(binary(cycle == 1 ? backendInput : 0, Width)), cycle == 1, 1,
         cycle == 0);
    rows.back().original = 1;
    rows.back().cycle = cycle;
  }
  edge(zero(), 0, 0, 1);
  // Original native vector, exactly five Work/Xfer ticks after initialization.
  constexpr unsigned nativeInput = (((5u << 5) | 17u) << 20) | (0x1234u << 4);
  for (unsigned cycle = 0; cycle < 5; ++cycle) {
    edge(token(binary(cycle == 0 ? nativeInput : 0, Width)), cycle == 0, 1);
    rows.back().original = 2;
    rows.back().cycle = cycle;
  }
  // E0 captures, E1 transforms, E2 retires a known zero result.
  edge(token(binary(((7u << 25) | (31u << 20)), Width)));
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
  return rows;
}
struct Expected {
  bool ready, valid;
  Planes data;
};
class Golden {
  std::optional<Planes> first, second;
  bool last = false;

public:
  Expected output(bool take) const {
    const bool room = !second || take;
    return {!first || room, second.has_value(), second.value_or(zero())};
  }
  void commit(const Row &r) {
    if (r.clock && !last) {
      if (r.reset) {
        first.reset();
        second.reset();
      } else {
        const auto oldFirst = first;
        const bool popSecond = second.has_value() && r.take;
        const bool room = !second || popSecond;
        const bool popFirst = first.has_value() && room;
        const bool inputRoom = !first || popFirst;
        if (oldFirst && room)
          second = transform(*oldFirst);
        else if (popSecond)
          second.reset();
        if (r.valid && inputRoom)
          first = r.data;
        else if (popFirst)
          first.reset();
      }
    }
    last = r.clock;
  }
};
pyc_dut::Inputs inputs(const Row &r) {
  pyc_dut::Inputs p;
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
  for (unsigned i = 0; i < 8; ++i)
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
  std::deque<Planes> history;
  unsigned sampled = 0, stalled = 0, accepted = 0, retired = 0, dropped = 0,
           peak = 0, replacements = 0, nativeOriginalRetired = 0;
  bool last = false;
  void observe(std::string_view label) {
    const auto &r = rows[sampled];
    const auto e = golden.output(r.take);
    const auto out = dut.sample().result.packed();
    require(out.knownMask().bit(Width + 1) && out.knownMask().bit(Width));
    require(!out.zMask().bit(Width + 1) && !out.zMask().bit(Width));
    require(out.value().bit(Width + 1) == e.ready &&
            out.value().bit(Width) == e.valid);
    const auto data = gfsim::extract<Width>(out, 0);
    checkPayload(data, e.data);
    std::cout << label << ' ' << sampled << ' ' << unsigned(e.ready) << ' '
              << unsigned(e.valid) << ' ' << visible(e.data) << '\n';
    if (r.original) {
      const bool present = r.cycle == (r.original == 1 ? 2u : 1u);
      const unsigned literal = r.original == 1 ? 232928275u : 220345363u;
      require(e.ready && e.valid == present);
      checkPayload(data, present ? token(binary(literal, Width)) : zero());
      if (r.original == 2 && r.cycle == 0)
        nativeOriginalRetired = retired;
      if (r.original == 2 && r.cycle == 4)
        require(retired == nativeOriginalRetired + 1);
      std::cout << (r.original == 1 ? "ORIGINAL_BACKEND " : "ORIGINAL_NATIVE ")
                << r.cycle << ' ' << unsigned(e.valid) << ' ' << visible(e.data)
                << '\n';
    }
    stalled += r.valid && !e.ready;
    if (r.clock && !last) {
      if (r.reset) {
        dropped += history.size();
        history.clear();
      } else {
        bool full = history.size() == 2;
        if (e.valid && r.take) {
          require(!history.empty());
          checkPayload(data, history.front());
          history.pop_front();
          ++retired;
        }
        if (e.ready && r.valid) {
          history.push_back(transform(r.data));
          ++accepted;
          if (full)
            ++replacements;
        }
      }
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
      R"({"deadlock_window":null,"max_domain_cycles":{},"max_ticks":2048,"schema":"pycircuit-model-config","version":"1"})";
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
    c.observe("FOUR");
  }
  c.finish();
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
  return status;
}
