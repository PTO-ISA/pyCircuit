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
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
void require(bool condition,
             std::source_location at = std::source_location::current()) {
  if (!condition) {
    std::cerr << "enum_helpers independent oracle at " << at.line()
              << '\n';
    std::abort();
  }
}
constexpr unsigned Width = 20, InputWidth = 10;
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
Planes inputZero() { return token(std::string(InputWidth, '0')); }
// Independent scalar symbol truth tables; no enum/encoder Runtime helper,
// compiler lowering or Python DUT execution provides the expected result.
char equality(std::string_view bits, unsigned code) {
  bool uncertain=false;
  for(unsigned i=0;i<bits.size();++i){
    const char want='0'+((code>>(bits.size()-1-i))&1);
    if(bits[i]=='0'||bits[i]=='1'){if(bits[i]!=want)return '0';}
    else uncertain=true;
  }
  return uncertain?'x':'1';
}
char either(char a,char b){return a=='1'||b=='1'?'1':a=='0'&&b=='0'?'0':'x';}
std::string choose(char condition,std::string yes,std::string no){
  require(yes.size()==no.size());if(condition=='1')return yes;if(condition=='0')return no;
  for(unsigned i=0;i<yes.size();++i)if(yes[i]!=no[i])yes[i]='x';return yes;
}
std::string symbols(const Planes &p){
  std::string s;for(unsigned i=0;i<p.value.size();++i)s+=p.z[i]=='1'?'z':p.known[i]=='0'?'x':p.value[i];return s;
}
Planes transform(const Planes &p){
  const auto s=symbols(p);require(s.size()==InputWidth);
  const auto raw=s.substr(0,4),mask=s.substr(4,2),selector=s.substr(6,4);
  char rn=equality(raw,1),rr=equality(raw,3),rw=equality(raw,9),re=equality(raw,15);
  char valid=either(either(rn,rr),either(rw,re));
  const auto decoded=choose(valid,choose(either(rn,rr),choose(rn,"0001","0011"),choose(rw,"1001","1111")),"0001");
  char b0=mask[1]=='0'?'0':mask[1]=='1'?'1':'x',b1=mask[0]=='0'?'0':mask[0]=='1'?'1':'x';
  char present=either(b0,b1),conflict=(b0=='x'||b1=='x')?'x':b0=='1'&&b1=='1'?'1':'0';
  const auto onehot=choose(conflict,"1111",choose(present,choose(b0,"0011","1001"),"0001"));
  char sn=equality(selector,1),sr=equality(selector,3),sw=equality(selector,9),se=equality(selector,15);
  char selected=either(sr,sw),member=either(either(sn,sr),either(sw,se));
  const auto classification=choose(member,choose(either(sn,sr),choose(sn,"00001010","00001011"),choose(sw,"00001100","00001101")),"11111111");
  return token(decoded+valid+onehot+present+conflict+selected+classification);
}
void oracleWitnesses(){
  for(char c:{'x','z'}){
    auto raw=std::string(1,c)+"001";
    const auto a=symbols(transform(token(raw+"01"+"0001")));
    require(a.substr(0,5)=="xxx1x");
    const auto b=symbols(transform(token("0001"+std::string("01")+"00"+c+"1")));
    require(b.substr(12,8)=="xxxx1xxx");
    const auto low=symbols(transform(token(std::string("0001")+"0"+c+"0001")));
    require(low.substr(5,6)=="xxx1xx");
    const auto high=symbols(transform(token(std::string("0001")+c+"1"+"0001")));
    require(high.substr(5,6)=="xx111x");
  }
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
  return gfsim::wire<gfsim::Bits<InputWidth>>::fromPacked(
      gfsim::FourState<InputWidth>::fromMasks(
          bits<InputWidth>(p.value), bits<InputWidth>(p.known), bits<InputWidth>(p.z)));
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
const std::vector<std::string> knownVectors=[] {
  std::vector<std::string> values;
  for(unsigned i=0;i<1024;++i)values.push_back(binary(i,InputWidth));
  return values;
}();
std::string symbolic(unsigned value,unsigned width){
  std::string s;for(unsigned b=width;b;--b)s+="01xz"[(value>>(2*(b-1)))&3];return s;
}
std::vector<Planes> fourVectors(){
  std::vector<Planes> values;constexpr unsigned codes[4]={1,3,9,15};
  for(unsigned i=0;i<256;++i)for(bool latent:{false,true})
    values.push_back(token(symbolic(i,4)+binary(i%4,2)+binary(codes[i%4],4),latent));
  for(unsigned i=0;i<256;++i)for(bool latent:{false,true})
    values.push_back(token(binary(codes[i%4],4)+binary((i/4)%4,2)+symbolic(i,4),latent));
  for(unsigned i=0;i<16;++i)for(bool latent:{false,true})
    values.push_back(token("0001"+symbolic(i,2)+"1111",latent));
  require(values.size()==1056);
  for(std::string_view raw:{"xxxx","zzzz","xzxz","01xz"})
    for(std::string_view selector:{"xxxx","zzzz","zxzx","xz01"})
      for(unsigned mask=0;mask<16;++mask)for(bool latent:{false,true})
        values.push_back(token(std::string(raw)+symbolic(mask,2)+std::string(selector),latent));
  require(values.size()==1568);
  for(char c:{'x','z'}){
    std::vector<std::string> witness={std::string(1,c)+"001010001",
      std::string("00010100")+c+"1",std::string("00010")+c+"0001",std::string("0001")+c+"10001"};
    for(const auto &s:witness)for(bool latent:{false,true})values.push_back(token(s,latent));
  }
  require(values.size()==1584);return values;
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
  add(0, 1, 0, inputZero(), 0);
  edge(inputZero(), 0, 0, 1);
  // E0 captures, E1 transforms, E2 retires a known zero token.
  edge(token(binary(31, InputWidth)));
  edge(inputZero(), 0);
  edge(inputZero(), 0);
  for (unsigned i = 0; i < 5; ++i)
    edge(token(knownVectors[i]), 1, 0);
  add(1, 0, 1, token(knownVectors[5]), 0);
  add(1, 0, 1, token(knownVectors[6]), 1);
  add(0, 0, 1, token(knownVectors[7]), 1);
  add(0, 0, 1, token(knownVectors[8]), 1);
  edge(token(knownVectors[9]));
  edge(token(knownVectors[10]));
  edge(inputZero(), 1, 0, 1);
  if (four) {
    // Occupancy depends on tokens even when every payload bit is X or Z.
    edge(token(std::string(InputWidth, 'x')), 1, 0);
    edge(token(std::string(InputWidth, 'z'), true), 1, 0);
    edge(inputZero(), 1, 0);
    add(1, 0, 1, token(std::string(InputWidth, 'x'), true), 0);
    add(1, 1, 1, inputZero(), 1); // held-high reset must not commit
    add(0, 0, 1, token(std::string(InputWidth, '1')), 1);
    add(0, 0, 0, inputZero(), 0);
    edge(token(knownVectors[0]));
    edge(inputZero(), 0);
    edge(inputZero(), 0);
    for (const auto &p : fourVectors())
      edge(p);
    // Recover to known flags after uncertain tokens, including zero/allones.
    edge(token(binary(31, InputWidth)));
    edge(token(binary(1023, InputWidth)));
  } else {
    for (const auto &s : knownVectors)
      edge(token(s));
  }
  edge(inputZero(), 0, 1);
  edge(inputZero(), 0, 1);
  edge(inputZero(), 0, 1);
  edge(token(knownVectors[0]), 1, 0);
  edge(token(knownVectors[1]), 1, 0);
  edge(inputZero(), 1, 1, 1);
  require(rows.size() == (four ? 3229 : 2089));
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
  static_assert(decltype(p.data)::width == 10);
  p.pyc_7079635f636c6b = known<1>(r.clock);
  p.pyc_7079635f727374 = known<1>(r.reset);
  p.valid = known<1>(r.valid);
  p.take = known<1>(r.take);
  p.data = decltype(p.data)::fromPacked(wire(r.data).packed());
  return p;
}
template <class Packed>
void checkPayload(const Packed &actual, const Planes &expected) {
  // Every result bit is computed: compare exact known/Z and only known
  // value bits. Computed-X latent value is unspecified; no Z may be emitted.
  const auto mask=bits<Width>(expected.known);
  require((actual.value()&mask)==(bits<Width>(expected.value)&mask));
  require(actual.knownMask()==mask&&actual.zMask()==bits<Width>(expected.z));
  require(actual.zMask()==gfsim::Bits<Width>{0});
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
    static_assert(decltype(dut.sample().result)::width == 22);
    const auto out = dut.sample().result.packed();
    require(out.knownMask().bit(Width + 1) && out.knownMask().bit(Width));
    require(!out.zMask().bit(Width + 1) && !out.zMask().bit(Width));
    require(out.value().bit(Width + 1) == e.ready &&
            out.value().bit(Width) == e.valid);
    const bool actualReady = out.value().bit(21);
    const bool actualValid = out.value().bit(20);
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
      R"({"deadlock_window":null,"max_domain_cycles":{},"max_ticks":4000,"schema":"pycircuit-model-config","version":"1"})";
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
    R"({"deadlock_window":null,"max_domain_cycles":{},"max_ticks":4000,"schema":"pycircuit-model-config","version":"1"})";
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
  require(out.knownMask().bit(21) && out.knownMask().bit(20));
  require(!out.zMask().bit(21) && !out.zMask().bit(20));
  require(out.value().bit(21) == ready &&
          out.value().bit(20) == payload.has_value());
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
    const auto a = token(binary(0x0d3,InputWidth));
    const auto b = token(binary(0x269,InputWidth));
    const auto c = token(binary(0x3ff,InputWidth));
    Row r{0, 0, 0, inputZero(), 0};
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
    commit({0, 0, 0, inputZero(), 0});
    if (mode != 1) {
      commit({1, 0, 1, b, 0});
      commit({0, 0, 0, inputZero(), 0});
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
    work({1, 0, 0, inputZero(), 0});
    checkRoot(root, mode == 1, transform(mode == 1 ? a : b));
    root.Xfer();
    unsigned retired = 0;
    std::vector<Planes> expected =
        mode == 1 ? std::vector<Planes>{transform(a)}
                  : std::vector<Planes>{transform(b), transform(c)};
    for (unsigned i = 0; i < 4; ++i) {
      commit({0, 0, 0, inputZero(), 1});
      work({1, 0, 0, inputZero(), 1});
      const auto out = root.result.packed();
      if (out.value().bit(20)) {
        require(retired < expected.size());
        checkPayload(gfsim::extract<Width>(out, 0), expected[retired++]);
      }
      root.Xfer();
    }
    require(retired == expected.size());
    work({0, 0, 0, inputZero(), 0});
    checkRoot(root, true, std::nullopt);
    root.Xfer();
    commit({0,0,1,a,0});commit({1,0,1,a,0});
    root.Reset();root.DiscardNext();root.Xfer();
    unsigned retained=0;
    for(unsigned i=0;i<4;++i){
      commit({0,0,0,inputZero(),1});work({1,0,0,inputZero(),1});
      const auto out=root.result.packed();
      if(out.value().bit(Width)){checkPayload(gfsim::extract<Width>(out,0),transform(a));++retained;}
      root.Xfer();
    }
    require(retained==1);work({0,0,0,inputZero(),0});checkRoot(root,true,std::nullopt);root.Xfer();
    root.Reset();root.Xfer();work({0,0,0,inputZero(),0});checkRoot(root,true,std::nullopt);root.Xfer();
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
    const auto a = token(binary(0x0d3,InputWidth));
    dut.drive(inputs({0, 0, 0, inputZero(), 0}));
    require(exec.Reset() == PYCIRCUIT_MODEL_STATUS_V1_OK);
    step({0, 0, 0, inputZero(), 0});
    step({1, 0, 1, a, 0});
    step({0, 0, 0, inputZero(), 0});
    if (mode) {
      step({1, 0, 1, a, 0});
      step({0, 0, 0, inputZero(), 0});
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
    dut.drive(inputs({0, 0, 0, inputZero(), 0}));
    require(exec.Reset() == PYCIRCUIT_MODEL_STATUS_V1_OK);
    step({0, 0, 0, inputZero(), 0});
    const auto out = dut.sample().result.packed();
    require(out.value().bit(21) && !out.value().bit(20));
    checkPayload(gfsim::extract<Width>(out, 0), zero());
    // Successful token execution after Reset, rather than merely accepting
    // Reset.
    step({1, 0, 1, a, 1});
    step({0, 0, 0, inputZero(), 1});
    step({1, 0, 0, inputZero(), 1});
    step({0, 0, 0, inputZero(), 1});
    require(dut.sample().result.packed().value().bit(20));
    checkPayload(gfsim::extract<Width>(dut.sample().result.packed(), 0),
                 transform(a));
    step({1, 0, 0, inputZero(), 1});
    step({0, 0, 0, inputZero(), 1});
    require(!dut.sample().result.packed().value().bit(20));
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
  oracleWitnesses();
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
