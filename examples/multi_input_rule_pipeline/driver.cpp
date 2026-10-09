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
constexpr bool IsJoin = true;
unsigned currentRow = 0;
void require(bool ok,
             std::source_location at = std::source_location::current()) {
  if (!ok) {
    std::cerr << "multi_input_rule_pipeline independent oracle at " << at.line()
              << ", row " << currentRow << '\n';
    std::abort();
  }
}
struct Planes {
  std::string value, known, z;
};
Planes token(std::string_view symbols, bool latent = false) {
  Planes p;
  for (char c : symbols) {
    require(c == '0' || c == '1' || c == 'x' || c == 'z');
    p.value += c == '1' || ((c == 'x' || c == 'z') && latent) ? '1' : '0';
    p.known += c == '0' || c == '1' ? '1' : '0';
    p.z += c == 'z' ? '1' : '0';
  }
  require(p.value.size() == 64);
  return p;
}
Planes zero() { return token(std::string(64, '0')); }
bool fullyKnown(const Planes &p) {
  return p.known == std::string(64, '1') && p.z == std::string(64, '0');
}
// Independent full-adder, exactly64bits; computed X has no latent-value
// promise.
Planes add(const Planes &a, const Planes &b) {
  if (!fullyKnown(a) || !fullyKnown(b))
    return token(std::string(64, 'x'));
  std::string result(64, '0');
  bool carry = false;
  for (unsigned bit = 0; bit < 64; ++bit) {
    unsigned i = 63 - bit;
    bool x = a.value[i] == '1', y = b.value[i] == '1';
    result[i] = (x != y) != carry ? '1' : '0';
    carry = (x && y) || (carry && (x != y));
  }
  return token(result);
}
Planes transform(const Planes &a, bool right) {
  if (!right)
    return add(a, token(std::string(63, '0') + '1'));
  // Preserve arithmetic multiplication: even unknown discarded bit63 makes
  // allX.
  if (!fullyKnown(a))
    return token(std::string(64, 'x'));
  return token(a.value.substr(1) + '0');
}
std::string visible(const Planes &p) {
  std::string s;
  for (unsigned i = 0; i < 64; ++i)
    s += p.z[i] == '1' ? 'z' : p.known[i] == '0' ? 'x' : p.value[i];
  return s;
}
std::string binary(std::uint64_t value) {
  std::string s;
  for (unsigned i = 64; i; --i)
    s += (value >> (i - 1)) & 1 ? '1' : '0';
  return s;
}
std::uint64_t carryValue(unsigned n) {
  return n == 64 ? ~std::uint64_t{0} : (std::uint64_t{1} << n) - 1;
}
std::uint64_t onehot(unsigned n) { return n == 64 ? 0 : std::uint64_t{1} << n; }
std::vector<std::uint64_t> randoms() {
  std::vector<std::uint64_t> v;
  std::uint64_t s = 0x41479;
  for (unsigned i = 0; i < 64; ++i) {
    s = s * 6364136223846793005ULL + 1442695040888963407ULL;
    v.push_back(s);
  }
  return v;
}
std::vector<std::uint64_t> knownValues() {
  std::vector<std::uint64_t> v;
  for (unsigned i = 0; i <= 64; ++i)
    v.push_back(carryValue(i));
  for (unsigned i = 0; i < 64; ++i)
    v.push_back(onehot(i));
  const std::uint64_t high = std::uint64_t{1} << 63, max = ~std::uint64_t{0};
  for (auto x : std::array<std::uint64_t, 8>{0, max, max - 1, high - 1, high,
                                             high + 1, 0x5555555555555555ULL,
                                             0xaaaaaaaaaaaaaaaaULL})
    v.push_back(x);
  const auto r = randoms();
  v.insert(v.end(), r.begin(), r.end());
  require(v.size() == 201);
  return v;
}
struct Vector {
  Planes left, right;
};
std::vector<Vector> vectors(bool four) {
  std::vector<Vector> v;
  if (!four) {
    if constexpr (IsJoin) {
      for (unsigned a = 0; a <= 64; ++a)
        for (unsigned b = 0; b <= 64; ++b)
          v.push_back({token(binary(carryValue(a))), token(binary(onehot(b)))});
      const auto k = knownValues();
      for (unsigned a = 129; a < 135; ++a)
        for (unsigned b = 129; b < 135; ++b)
          v.push_back({token(binary(k[a])), token(binary(k[b]))});
      const auto r = randoms();
      for (unsigned i = 0; i < 64; ++i)
        v.push_back({token(binary(r[i])), token(binary(r[63 - i]))});
    } else {
      const auto k = knownValues();
      for (unsigned i = 0; i < k.size(); ++i)
        v.push_back(
            {token(binary(k[i])), token(binary(k[(i * 47) % k.size()]))});
    }
    require(v.size() == (IsJoin ? 4325 : 201));
    return v;
  }
  for (unsigned owner = 0; owner < 2; ++owner)
    for (unsigned bit = 0; bit < 64; ++bit)
      for (char symbol : {'x', 'z'})
        for (bool latent : {false, true}) {
          std::array<std::string, 2> p{binary(0xa5a5a5a55a5a5a5aULL),
                                       binary(0x0123456789abcdefULL)};
          p[owner][63 - bit] = symbol;
          v.push_back({token(p[0], latent), token(p[1], latent)});
        }
  auto dense = [](unsigned n) {
    std::string s;
    for (unsigned i = 0; i < 64; ++i)
      s += n == 0 ? 'x' : n == 1 ? 'z' : n == 2 ? "xz"[i % 2] : "01xz"[i % 4];
    return s;
  };
  for (unsigned owner = 0; owner < 2; ++owner)
    for (unsigned pattern = 0; pattern < 4; ++pattern)
      for (bool latent : {false, true}) {
        std::array<std::string, 2> p{binary(~std::uint64_t{0}),
                                     binary(std::uint64_t{1} << 63)};
        p[owner] = dense(pattern);
        v.push_back({token(p[0], latent), token(p[1], latent)});
      }
  for (const auto pair :
       std::array<std::array<unsigned, 2>, 4>{{{0, 1}, {1, 0}, {2, 3}, {3, 2}}})
    for (bool latent : {false, true})
      v.push_back(
          {token(dense(pair[0]), latent), token(dense(pair[1]), latent)});
  require(v.size() == 536);
  return v;
}
struct Row {
  unsigned clock, reset, leftValid;
  Planes left;
  unsigned rightValid;
  Planes right;
  unsigned leftTake, rightTake;
};
std::vector<Row> stimulus(bool four) {
  const auto v = vectors(four);
  std::vector<Row> rows;
  auto put = [&](unsigned c, unsigned r, unsigned lv, const Planes &ld,
                 unsigned rv, const Planes &rd, unsigned lt, unsigned rt) {
    rows.push_back({c, r, lv, ld, rv, rd, lt, rt});
  };
  auto edge = [&](const Vector &p, unsigned lv = 1, unsigned rv = 1,
                  unsigned lt = 1, unsigned rt = 1, unsigned r = 0) {
    put(1, r, lv, p.left, rv, p.right, lt, rt);
    put(0, r, lv, p.left, rv, p.right, lt, rt);
  };
  const Vector z{zero(), zero()},
      startup{token(binary(~std::uint64_t{0})),
              token(binary(IsJoin ? 1 : std::uint64_t{1} << 63))};
  put(0, 1, 0, z.left, 0, z.right, 0, 0);
  edge(z, 0, 0, 0, 0, 1);
  edge(startup);
  edge(z, 0, 0);
  edge(z, 0, 0);
  if constexpr (IsJoin) {
    for (unsigned i = 0; i < 3; ++i)
      edge(v[i], 1, 0);
    for (unsigned i = 0; i < 2; ++i)
      edge(v[i + 3], 0, 1);
    for (unsigned i = 0; i < 2; ++i)
      edge(z, 0, 0);
    for (unsigned i = 0; i < 3; ++i)
      edge(v[i + 5], 0, 1);
    for (unsigned i = 0; i < 2; ++i)
      edge(v[i + 8], 1, 0);
    for (unsigned i = 0; i < 2; ++i)
      edge(z, 0, 0);
  } else {
    for (unsigned i = 0; i < 8; ++i)
      edge(v[i], 1, 0);
    for (unsigned i = 0; i < 8; ++i)
      edge(v[i + 8], 0, 1);
    for (unsigned i = 0; i < 6; ++i)
      edge(v[i], 1, 1, 0, 1);
    for (unsigned i = 0; i < 6; ++i)
      edge(v[i + 6], 1, 1, 1, 0);
    for (unsigned i = 0; i < 3; ++i)
      edge(z, 0, 0);
  }
  const unsigned fill = IsJoin ? 3 : 2;
  for (unsigned i = 0; i < fill + 1; ++i)
    edge(v[i], 1, 1, 0, 0);
  put(1, 0, 1, v[4].left, 1, v[4].right, 0, 0);
  put(1, 1, 1, v[5].left, 1, v[5].right, 1, 1);
  put(0, 0, 1, v[6].left, 1, v[6].right, 1, 0);
  put(0, 0, 1, v[7].left, 1, v[7].right, 0, 1);
  edge(z, 1, 1, 1, 1, 1);
  for (unsigned i = 0; i < fill; ++i)
    edge(v[i], 1, 1, 0, 0);
  for (unsigned i = 0; i < 6; ++i)
    edge(v[i + 3]);
  for (const auto &p : v)
    edge(p);
  for (unsigned i = 0; i < 5; ++i)
    edge(z, 0, 0);
  for (unsigned i = 0; i < fill; ++i)
    edge(v[i], 1, 1, 0, 0);
  edge(z, 1, 1, 1, 1, 1);
  edge(startup);
  edge(z);
  for (unsigned i = 0; i < 5; ++i)
    edge(z, 0, 0);
  require(rows.size() == 2 * v.size() + (IsJoin ? 101 : 129));
  return rows;
}
struct Expected {
  bool leftReady, rightReady, leftValid, rightValid;
  Planes left, right;
};
class Golden {
  std::deque<Planes> li, ri, lo, ro;
  bool last = false;

public:
  Expected read(unsigned lt, unsigned rt) const {
    const bool lp = !lo.empty() && lt, lr = lo.empty() || lp;
    if constexpr (IsJoin) {
      const bool fire = !li.empty() && !ri.empty() && lr;
      return {li.size() < 2 || fire,
              ri.size() < 2 || fire,
              !lo.empty(),
              false,
              lo.empty() ? zero() : lo.front(),
              zero()};
    }
    const bool rp = !ro.empty() && rt, rr = ro.empty() || rp;
    return {li.empty() || (!li.empty() && lr),
            ri.empty() || (!ri.empty() && rr),
            !lo.empty(),
            !ro.empty(),
            lo.empty() ? zero() : lo.front(),
            ro.empty() ? zero() : ro.front()};
  }
  void commit(const Row &r) {
    const auto e = read(r.leftTake, r.rightTake);
    if (r.clock && !last) {
      if (r.reset) {
        li.clear();
        ri.clear();
        lo.clear();
        ro.clear();
      } else if constexpr (IsJoin) {
        const bool pop = !lo.empty() && r.leftTake, room = lo.empty() || pop,
                   fire = !li.empty() && !ri.empty() && room;
        const auto a = li.empty() ? zero() : li.front(),
                   b = ri.empty() ? zero() : ri.front();
        if (pop)
          lo.pop_front();
        if (fire) {
          lo.push_back(add(a, b));
          li.pop_front();
          ri.pop_front();
        }
        if (e.leftReady && r.leftValid)
          li.push_back(r.left);
        if (e.rightReady && r.rightValid)
          ri.push_back(r.right);
      } else {
        const bool lp = !lo.empty() && r.leftTake, lr = lo.empty() || lp,
                   la = !li.empty() && lr;
        const bool rp = !ro.empty() && r.rightTake, rr = ro.empty() || rp,
                   ra = !ri.empty() && rr;
        const auto a = li.empty() ? zero() : li.front(),
                   b = ri.empty() ? zero() : ri.front();
        if (lp)
          lo.pop_front();
        if (rp)
          ro.pop_front();
        if (la) {
          lo.push_back(transform(a, false));
          li.pop_front();
        }
        if (ra) {
          ro.push_back(transform(b, true));
          ri.pop_front();
        }
        if (e.leftReady && r.leftValid)
          li.push_back(r.left);
        if (e.rightReady && r.rightValid)
          ri.push_back(r.right);
      }
      require(li.size() <= (IsJoin ? 2 : 1) && ri.size() <= (IsJoin ? 2 : 1) &&
              lo.size() <= 1 && ro.size() <= (IsJoin ? 0 : 1));
    }
    last = r.clock;
  }
  unsigned pendingLeft() const { return li.size() + lo.size(); }
  unsigned pendingRight() const {
    return ri.size() + (IsJoin ? lo.size() : ro.size());
  }
};
gfsim::Bits<64> bits(std::string_view text) {
  require(text.size() == 64);
  gfsim::Bits<64> b{0};
  for (unsigned i = 0; i < 64; ++i)
    if (text[63 - i] == '1')
      b.setWord(0, b.word(0) | (std::uint64_t{1} << i));
  return b;
}
auto wire(const Planes &p) {
  return gfsim::wire<gfsim::Bits<64>>::fromPacked(
      gfsim::FourState<64>::fromMasks(bits(p.value), bits(p.known), bits(p.z)));
}
auto control(unsigned value) {
  return gfsim::wire<gfsim::Bits<1>>::known(gfsim::Bits<1>{value});
}
pyc_dut::Inputs inputs(const Row &r) {
  pyc_dut::Inputs p;
  p.pyc_7079635f636c6b = control(r.clock);
  p.pyc_7079635f727374 = control(r.reset);
  p.left_valid = control(r.leftValid);
  p.left_data = wire(r.left);
  p.right_valid = control(r.rightValid);
  p.right_data = wire(r.right);
  p.take = control(r.leftTake);
  return p;
}
template <class Packed> void checkPayload(const Packed &a, const Planes &e) {
  require(a.knownMask() == bits(e.known) && a.zMask() == bits(e.z));
  for (unsigned i = 0; i < 64; ++i)
    if (e.known[63 - i] == '1')
      require(a.value().bit(i) == (e.value[63 - i] == '1'));
}
struct Identity {
  std::uint64_t ordinal;
  Planes data;
};
struct Context {
  pyc_dut &dut;
  std::vector<Row> rows;
  Golden golden;
  std::array<std::deque<Identity>, 2> history;
  std::array<unsigned, 2> accepted{}, retired{}, dropped{}, peak{},
      replacements{}, stalled{};
  std::array<std::uint64_t, 2> lastId{};
  std::array<bool, 2> hasRetired{};
  void retireId(unsigned side) {
    const auto id = history[side].front().ordinal;
    require(!hasRetired[side] || id > lastId[side]);
    lastId[side] = id;
    hasRetired[side] = true;
  }
  unsigned sampled = 0, physicalPeak = 0, contributorPeak = 0, fullResets = 0;
  bool last = false;
  void observe(std::string_view label) {
    currentRow = sampled;
    const auto &r = rows[sampled];
    const auto e = golden.read(r.leftTake, r.rightTake);
    const auto raw = dut.sample().result.packed();
    auto ctrl = [&](unsigned bit) {
      require(raw.knownMask().bit(bit) && !raw.zMask().bit(bit));
      return raw.value().bit(bit);
    };
    const bool actualLeftReady = ctrl(66), actualRightReady = ctrl(65),
               actualLeftValid = ctrl(64), actualRightValid = false;
    const auto leftData = gfsim::extract<64>(raw, 0), rightData = leftData;
    require(actualLeftReady == e.leftReady &&
            actualRightReady == e.rightReady &&
            actualLeftValid == e.leftValid && actualRightValid == e.rightValid);
    checkPayload(leftData, e.left);
    if constexpr (!IsJoin)
      checkPayload(rightData, e.right);
    std::cout << label << ' ' << sampled << ' ' << actualLeftReady << ' '
              << actualRightReady << ' ' << actualLeftValid << ' '
              << visible(e.left) << '\n';
    stalled[0] += r.leftValid && !actualLeftReady;
    stalled[1] += r.rightValid && !actualRightReady;
    const unsigned physical = history[0].size() + history[1].size() -
                              (IsJoin && actualLeftValid ? 1 : 0);
    require(physical <= (IsJoin ? 5 : 4));
    physicalPeak = std::max(physicalPeak, physical);
    if (r.clock && !last) {
      if (r.reset) {
        fullResets += physical == (IsJoin ? 5 : 4);
        for (unsigned side = 0; side < 2; ++side) {
          dropped[side] += history[side].size();
          history[side].clear();
        }
      } else {
        const std::array<bool, 2> full{history[0].size() == (IsJoin ? 3 : 2),
                                       history[1].size() == (IsJoin ? 3 : 2)};
        if (actualLeftValid && r.leftTake) {
          require(!history[0].empty());
          if constexpr (IsJoin) {
            require(!history[1].empty());
            checkPayload(leftData,
                         add(history[0].front().data, history[1].front().data));
            retireId(1);
            history[1].pop_front();
            ++retired[1];
          } else
            checkPayload(leftData, transform(history[0].front().data, false));
          retireId(0);
          history[0].pop_front();
          ++retired[0];
        }
        if constexpr (!IsJoin)
          if (actualRightValid && r.rightTake) {
            require(!history[1].empty());
            checkPayload(rightData, transform(history[1].front().data, true));
            retireId(1);
            history[1].pop_front();
            ++retired[1];
          }
        if (actualLeftReady && r.leftValid) {
          history[0].push_back({accepted[0], r.left});
          ++accepted[0];
          replacements[0] += full[0];
        }
        if (actualRightReady && r.rightValid) {
          history[1].push_back({accepted[1], r.right});
          ++accepted[1];
          replacements[1] += full[1];
        }
      }
    }
    last = r.clock;
    golden.commit(r);
    require(history[0].size() == golden.pendingLeft() &&
            history[1].size() == golden.pendingRight());
    for (unsigned side = 0; side < 2; ++side) {
      require(accepted[side] ==
              retired[side] + dropped[side] + history[side].size());
      require(history[side].size() <= (IsJoin ? 3 : 2));
      peak[side] = std::max(peak[side], unsigned(history[side].size()));
    }
    contributorPeak = std::max(contributorPeak,
                               unsigned(history[0].size() + history[1].size()));
    ++sampled;
  }
  void finish() {
    require(sampled == rows.size() && history[0].empty() &&
            history[1].empty() && physicalPeak == (IsJoin ? 5 : 4) &&
            contributorPeak == (IsJoin ? 6 : 4) && fullResets == 2);
    for (unsigned side = 0; side < 2; ++side)
      require(stalled[side] >= 3 && peak[side] == (IsJoin ? 3 : 2) &&
              replacements[side] >= 6 && dropped[side] == (IsJoin ? 6 : 4));
    require(retired[0] == retired[1]);
    std::cout << "HISTORY " << accepted[0] << ' ' << accepted[1] << ' '
              << retired[0] << ' ' << dropped[0] << ' ' << dropped[1] << ' '
              << history[0].size() << ' ' << history[1].size() << ' '
              << physicalPeak << ' ' << contributorPeak << '\n';
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
};
void fourState(unsigned workers) {
  pyc_dut dut(workers);
  gfsim::SimExecutor exec(dut.system(), dut.observations(), {});
  constexpr std::string_view config =
      R"({"deadlock_window":null,"max_domain_cycles":{},"max_ticks":12000,"schema":"pycircuit-model-config","version":"1"})";
  require(
      exec.ConfigureJson(reinterpret_cast<const std::uint8_t *>(config.data()),
                         config.size()) == PYCIRCUIT_MODEL_STATUS_V1_OK);
  Context c{dut, stimulus(true)};
  dut.drive(inputs(c.rows[0]));
  require(exec.Reset() == PYCIRCUIT_MODEL_STATUS_V1_OK);
  for (const auto &r : c.rows) {
    dut.drive(inputs(r));
    PycircuitModelStepResultV1 status{sizeof(status)};
    require(exec.Step(&status) == PYCIRCUIT_MODEL_STATUS_V1_OK &&
            status.state == PYCIRCUIT_MODEL_STEP_V1_RUNNING);
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
