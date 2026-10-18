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
    std::cerr << "latency_pipeline independent oracle at " << at.line() << '\n';
    std::abort();
  }
}
constexpr unsigned Width = 64;
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
// Independent 64-bit ripple arithmetic, separate for each complete field.
Planes transform(const Planes &p) {
  Planes result = p;
  for (unsigned field = 0; field < Width / 64; ++field) {
    const unsigned start = field * 64;
    bool fullyKnown = true;
    for (unsigned i = start; i < start + 64; ++i)
      fullyKnown &= p.known[i] == '1' && p.z[i] == '0';
    bool carry = true;
    for (unsigned offset = 0; offset < 64; ++offset) {
      const unsigned i = start + 63 - offset;
      const bool bit = p.value[i] == '1';
      result.value[i] = fullyKnown && (bit != carry) ? '1' : '0';
      result.known[i] = fullyKnown ? '1' : '0';
      result.z[i] = '0';
      carry = carry && (field == 1 ? !bit : bit);
    }
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
std::uint64_t carryValue(unsigned length) {
  return length == 64 ? ~std::uint64_t{0} : (std::uint64_t{1} << length) - 1;
}
std::uint64_t borrowValue(unsigned length) {
  return length == 64 ? 0 : std::uint64_t{1} << length;
}
std::vector<std::uint64_t> randomValues() {
  std::vector<std::uint64_t> values;
  std::uint64_t state = 0xd2106421;
  for (unsigned i = 0; i < 64; ++i) {
    state = state * 6364136223846793005ULL + 1442695040888963407ULL;
    values.push_back(state);
  }
  return values;
}
const std::vector<std::string> knownVectors = [] {
  std::vector<std::string> values;
  constexpr std::uint64_t max = ~std::uint64_t{0};
  constexpr std::uint64_t high = std::uint64_t{1} << 63;
  const std::uint64_t boundaries[] = {0,        max,  max - 1,
                                      high - 1, high, high + 1};
  const auto randoms = randomValues();
  auto append = [&](std::uint64_t value, std::uint64_t remaining = 0) {
    auto packed = binary(value, 64);
    if constexpr (Width == 128)
      packed += binary(remaining, 64);
    values.push_back(packed);
  };
  if constexpr (Width == 64) {
    for (unsigned length = 0; length <= 64; ++length)
      append(carryValue(length));
    for (unsigned length = 0; length <= 64; ++length)
      append(borrowValue(length));
    for (auto value : boundaries)
      append(value);
    append(0x5555555555555555ULL);
    append(0xaaaaaaaaaaaaaaaaULL);
    for (auto value : randoms)
      append(value);
  } else {
    for (unsigned carry = 0; carry <= 64; ++carry)
      for (unsigned borrow = 0; borrow <= 64; ++borrow)
        append(carryValue(carry), borrowValue(borrow));
    for (auto value : boundaries)
      for (auto remaining : boundaries)
        append(value, remaining);
    for (unsigned bit = 0; bit < 64; ++bit)
      append(std::uint64_t{1} << bit, 0);
    for (unsigned bit = 0; bit < 64; ++bit)
      append(0, std::uint64_t{1} << bit);
    for (unsigned i = 0; i < 64; ++i)
      append(randoms[i], randoms[63 - i]);
    append(0x5555555555555555ULL, 0xaaaaaaaaaaaaaaaaULL);
    append(0xaaaaaaaaaaaaaaaaULL, 0x5555555555555555ULL);
  }
  require(values.size() == (Width == 64 ? 202 : 4455));
  return values;
}();
std::vector<Planes> fourVectors() {
  std::vector<Planes> values;
  for (unsigned field = 0; field < Width / 64; ++field)
    for (unsigned bit = 0; bit < 64; ++bit)
      for (char symbol : {'x', 'z'})
        for (bool latent : {false, true}) {
          auto value = binary(0xa5a5a5a55a5a5a5aULL, 64);
          if constexpr (Width == 128)
            value += binary(0x0123456789abcdefULL, 64);
          value[field * 64 + 63 - bit] = symbol;
          values.push_back(token(value, latent));
        }
  auto dense = [](unsigned pattern) {
    std::string value;
    for (unsigned bit = 0; bit < 64; ++bit)
      value += pattern == 0   ? 'x'
               : pattern == 1 ? 'z'
               : pattern == 2 ? "xz"[bit % 2]
                              : "01xz"[bit % 4];
    return value;
  };
  for (unsigned field = 0; field < Width / 64; ++field)
    for (unsigned pattern = 0; pattern < 4; ++pattern)
      for (bool latent : {false, true}) {
        auto value = std::string(64, '1');
        if constexpr (Width == 128)
          value += std::string(64, '0');
        value.replace(field * 64, 64, dense(pattern));
        values.push_back(token(value, latent));
      }
  if constexpr (Width == 128) {
    const unsigned patterns[][2] = {{0, 1}, {1, 0}, {2, 3}, {3, 2}};
    for (const auto &pair : patterns)
      for (bool latent : {false, true})
        values.push_back(token(dense(pair[0]) + dense(pair[1]), latent));
  }
  require(values.size() == (Width == 64 ? 264 : 536));
  return values;
}

struct Row {
  unsigned clock, reset, valid;
  Planes data;
  unsigned take;
};
std::vector<Row> stimulus(bool four) {
  std::vector<Row> rows;
  std::vector<Planes> values;
  if (four)
    values = fourVectors();
  else
    for (const auto &text : knownVectors)
      values.push_back(token(text));
  auto add = [&](unsigned c, unsigned r, unsigned v, const Planes &p,
                 unsigned t) { rows.push_back({c, r, v, p, t}); };
  auto edge = [&](const Planes &p, unsigned v = 1, unsigned t = 1,
                  unsigned r = 0) {
    add(1, r, v, p, t);
    add(0, r, v, p, t);
  };
  const auto wrapInput = token(std::string(64, '1'));
  add(0, 1, 0, zero(), 0);
  edge(zero(), 0, 0, 1);
  // E0 capture, E1 result birth, E3 maturity, E4 earliest retirement.
  edge(wrapInput);
  for (unsigned i = 0; i < 4; ++i)
    edge(zero(), 0);
  for (unsigned i = 0; i < 7; ++i)
    edge(values[i], 1, 0);
  add(1, 0, 1, values[7], 0);
  add(1, 1, 1, values[8], 1); // held-high reset cannot transfer or age
  add(0, 0, 1, values[9], 1);
  add(0, 0, 1, values[10], 0);
  edge(zero(), 1, 1, 1); // Drop exactly six live tokens.
  for (unsigned i = 0; i < 7; ++i)
    edge(values[i], 1, 0);
  for (unsigned i = 0; i < 16; ++i)
    edge(values[i % values.size()], 1, 0);
  for (unsigned i = 0; i < 6; ++i)
    edge(values[i + 6]);
  for (const auto &p : values)
    edge(p);
  for (unsigned i = 0; i < 10; ++i)
    edge(zero(), 0);
  for (unsigned i = 0; i < 7; ++i)
    edge(values[i], 1, 0);
  edge(zero(), 1, 1,
       1); // Second six-token reset; stale timing must remain hidden.
  edge(wrapInput);
  edge(zero());
  for (unsigned i = 0; i < 10; ++i)
    edge(zero(), 0);
  return rows;
}

struct Expected {
  bool ready, valid;
  Planes data;
};
class Golden {
  struct Birth {
    Planes data;
    std::int64_t maturity;
  };
  std::deque<Birth> first, second;
  bool last = false;
  std::int64_t edge = -2; // Setup synchronous reset precedes traffic E0.
public:
  Expected output(bool take) const {
    const bool eligible = !second.empty() && second.front().maturity <= edge;
    const bool pop = eligible && take;
    const bool room = second.size() < 4 || pop;
    const bool move = !first.empty() && room;
    return {first.size() < 2 || move, eligible,
            eligible ? second.front().data : zero()};
  }
  void commit(const Row &r) {
    if (r.clock && !last) {
      const auto old = output(r.take);
      const bool pop = old.valid && r.take;
      const bool room = second.size() < 4 || pop;
      const bool move = !first.empty() && room;
      const auto head = first.empty() ? zero() : first.front().data;
      ++edge;
      require(
          edge <
          2000); // All absolute arithmetic remains within the finite domain.
      if (r.reset) {
        first.clear();
        second.clear();
      } else {
        if (pop)
          second.pop_front();
        if (move) {
          first.pop_front();
          second.push_back({transform(head), edge + 2});
        }
        if (r.valid && old.ready)
          first.push_back({r.data, edge});
        require(first.size() <= 2 && second.size() <= 4);
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
  // The entire scalar payload is computed; known bits still require exact
  // values.
  for (unsigned i = 0; i < Width; ++i)
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
           peak = 0, replacements = 0, resetFull = 0;
  bool last = false;
  void observe(std::string_view label) {
    const auto &r = rows[sampled];
    const auto e = golden.output(r.take);
    const auto out = dut.sample().result.packed();
    require(out.knownMask().bit(Width + 1) && out.knownMask().bit(Width));
    require(!out.zMask().bit(Width + 1) && !out.zMask().bit(Width));
    require(out.value().bit(Width + 1) == e.ready &&
            out.value().bit(Width) == e.valid);
    const bool actualReady = out.value().bit(Width + 1);
    const bool actualValid = out.value().bit(Width);
    const auto data = gfsim::extract<Width>(out, 0);
    checkPayload(data, e.data);
    std::cout << label << ' ' << sampled << ' ' << unsigned(e.ready) << ' '
              << unsigned(e.valid) << ' ' << visible(e.data) << '\n';
    stalled += r.valid && !e.ready;
    if (r.clock && !last) {
      if (r.reset) {
        resetFull += history.size() == 6;
        dropped += history.size();
        history.clear();
      } else {
        bool full = history.size() == 6;
        if (actualValid && r.take) {
          require(!history.empty());
          checkPayload(data, history.front());
          history.pop_front();
          ++retired;
        }
        if (actualReady && r.valid) {
          history.push_back(transform(r.data));
          ++accepted;
          if (full)
            ++replacements;
        }
      }
      peak = std::max(peak, unsigned(history.size()));
      require(history.size() <= 6);
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
    require(sampled == rows.size() && stalled >= 3 && peak == 6 &&
            replacements >= 6);
    require(history.empty() && dropped == 12 && resetFull == 2 &&
            accepted == retired + dropped);
    std::cout << "HISTORY " << accepted << ' ' << retired << ' ' << dropped
              << ' ' << history.size() << ' ' << peak << '\n';
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
    PycircuitModelStepResultV1 status{sizeof(status)};
    require(exec.Step(&status) == PYCIRCUIT_MODEL_STATUS_V1_OK);
    require(status.state == PYCIRCUIT_MODEL_STEP_V1_RUNNING);
    c.observe("FOUR");
  }
  c.finish();
}

void discardAndFailure(unsigned workers) {
  gfsim::WorkExecutor pool(workers);
  pyc_root root("latency-discard", &pool);
  root.Build();
  auto p = inputs({0, 0, 0, zero(), 0});
  auto driveRoot = [&] {
    root.pyc_7079635f636c6b = p.pyc_7079635f636c6b;
    root.pyc_7079635f727374 = p.pyc_7079635f727374;
    root.valid = p.valid;
    root.take = p.take;
    root.data = p.data;
  };
  auto check = [&](bool valid) {
    const auto packed = root.result.packed();
    require(packed.knownMask().bit(64) && packed.value().bit(64) == valid);
    checkPayload(gfsim::extract<64>(packed, 0),
                 valid ? token(binary(42, 64)) : zero());
  };
  driveRoot();
  root.Reset();
  root.Xfer();
  auto step = [&](unsigned clock) {
    p.pyc_7079635f636c6b = known<1>(clock);
    driveRoot();
    root.Work();
    root.Xfer();
  };
  p.valid = known<1>(1);
  p.data = wire(token(binary(41, 64)));
  step(1); // E0
  p.valid = known<1>(0);
  step(0);
  step(1); // E1 birth
  step(0);
  step(1); // E2 waiting
  step(0);
  p.pyc_7079635f636c6b = known<1>(1);
  driveRoot();
  root.Work();
  check(false);
  root.DiscardNext();
  root.Xfer(); // Discard E3 maturity.
  root.Work();
  check(false);
  root.Xfer(); // Same edge retry commits maturity.
  root.Work();
  check(true);
  root.Xfer(); // Held high observes but cannot transfer.
  p.take = known<1>(1);
  driveRoot();
  root.Work();
  check(true);
  root.Xfer();
  step(0);
  step(1);
  check(true);
  root.Work();
  check(false);
  root.Xfer();
  std::cout << "PROBE direct maturity discard/retry and held-clock retirement "
               "workers="
            << workers << '\n';

  pyc_dut dut(workers);
  gfsim::SimExecutor exec(dut.system(), dut.observations(), {});
  constexpr std::string_view config =
      R"({"deadlock_window":null,"max_domain_cycles":{},"max_ticks":64,"schema":"pycircuit-model-config","version":"1"})";
  require(
      exec.ConfigureJson(reinterpret_cast<const std::uint8_t *>(config.data()),
                         config.size()) == PYCIRCUIT_MODEL_STATUS_V1_OK);
  p = inputs({0, 0, 0, zero(), 0});
  dut.drive(p);
  require(exec.Reset() == PYCIRCUIT_MODEL_STATUS_V1_OK);
  PycircuitModelStepResultV1 result{sizeof(result)};
  auto execStep = [&](unsigned clock) {
    p.pyc_7079635f636c6b = known<1>(clock);
    dut.drive(p);
    require(exec.Step(&result) == PYCIRCUIT_MODEL_STATUS_V1_OK);
  };
  p.valid = known<1>(1);
  p.data = wire(token(binary(31, 64)));
  execStep(1);
  p.valid = known<1>(0);
  for (unsigned i = 0; i < 3; ++i) {
    execStep(0);
    execStep(1);
  }
  execStep(0);
  const auto epoch = exec.cycles();
  p.take = gfsim::wire<gfsim::Bits<1>>::unknown();
  p.pyc_7079635f636c6b = known<1>(1);
  dut.drive(p);
  require(exec.Step(&result) == PYCIRCUIT_MODEL_STATUS_V1_RUNTIME_FAILURE);
  require(exec.cycles() == epoch && dut.system().cycle() == epoch);
  bool unavailable = false;
  try {
    (void)dut.sample();
  } catch (const std::logic_error &) {
    unavailable = true;
  }
  require(unavailable &&
          exec.Step(&result) == PYCIRCUIT_MODEL_STATUS_V1_INVALID_STATE);
  p = inputs({0, 0, 0, zero(), 0});
  dut.drive(p);
  require(exec.Reset() == PYCIRCUIT_MODEL_STATUS_V1_OK);
  execStep(0);
  const auto resetOut = dut.sample().result.packed();
  require(resetOut.knownMask().bit(64) && !resetOut.value().bit(64));
  checkPayload(gfsim::extract<64>(resetOut, 0), zero());
  std::cout << "PROBE terminal effective-unknown failure, unchanged epoch and "
               "mandatory reset workers="
            << workers << '\n';
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
  discardAndFailure(runner.workers());
  return status;
}
