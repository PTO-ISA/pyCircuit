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
    std::cerr << "enum_payload_pipeline independent oracle at " << at.line()
              << '\n';
    std::abort();
  }
}
constexpr unsigned Width = 26;
struct Planes {
  std::string value, known, z;
  bool operator==(const Planes &) const = default;
};
Planes token(std::string_view symbols) {
  Planes p;
  for (unsigned i = 0; i < symbols.size(); ++i) {
    char c = symbols[i];
    require(c == '0' || c == '1' || c == 'x' || c == 'z');
    p.value += c == '1' || ((c == 'x' || c == 'z') && i % 3 == 0) ? '1' : '0';
    p.known += c == '0' || c == '1' ? '1' : '0';
    p.z += c == 'z' ? '1' : '0';
  }
  return p;
}
Planes zero() { return token(std::string(Width, '0')); }
Planes slice(const Planes &p, unsigned start, unsigned count) {
  return {p.value.substr(start, count), p.known.substr(start, count),
          p.z.substr(start, count)};
}
Planes append(Planes a, const Planes &b) {
  a.value += b.value;
  a.known += b.known;
  a.z += b.z;
  return a;
}
Planes equality(const Planes &p, std::string_view code) {
  bool uncertain = false;
  for (unsigned i = 0; i < code.size(); ++i) {
    if (p.known[i] == '1' && p.value[i] != code[i])
      return token("0");
    uncertain |= p.known[i] == '0';
  }
  return uncertain ? Planes{"0", "0", "0"} : token("1");
}
Planes transform(const Planes &p) {
  return append(append(append(slice(p, 0, 6), token("01")), slice(p, 8, 17)),
                equality(slice(p, 6, 2), "10"));
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
  for (unsigned mode = 0; mode < 4; ++mode)
    for (unsigned opcode = 0; opcode < 64; ++opcode)
      values.push_back(binary(opcode, 6) + binary(mode, 2) +
                       binary((opcode * 1237 + mode * 7919) & 131071, 17) +
                       binary((opcode + mode) & 1, 1));
  values.emplace_back(Width, '0');
  values.emplace_back(Width, '1');
  for (unsigned bit = 0; bit < Width; ++bit) {
    std::string value(Width, '0');
    value[Width - 1 - bit] = '1';
    values.push_back(std::move(value));
  }
  return values;
}();
const char *fourVectors[] = {
    "xzxzxz10xzxzxzxzxzxzxzxzxz", "1111111100000000000000000x",
    "000000x0zzzzzzzzzzzzzzzzz1", "0101010zxxxxxxxxxxxxxxxxx0",
    "1010101z01xz01xz01xz01xzz1", "zzzzzzzzzzzzzzzzzzzzzzzzzz",
    "xxxxxxxxxxxxxxxxxxxxxxxxxx", "11111101100000000000000000"};
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
    for (const char *s : fourVectors)
      edge(token(s));
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
  // Only the computed comparison's unknown value bit is unspecified. Copied
  // payload bits retain their raw latent values, including behind X/Z.
  if (expected.known.back() == '0')
    valueMask.back() = '0';
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
           peak = 0, replacements = 0;
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
      R"({"deadlock_window":null,"max_domain_cycles":{},"max_ticks":128,"schema":"pycircuit-model-config","version":"1"})";
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
