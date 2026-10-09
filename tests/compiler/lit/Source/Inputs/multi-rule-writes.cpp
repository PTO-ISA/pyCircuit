#include "pycircuit_system.hpp"
#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <source_location>
#include <stdexcept>

void require(bool ok, std::source_location at = std::source_location::current()) {
  if (!ok) { std::cerr << "multi-rule oracle line " << at.line() << '\n'; std::abort(); }
}
template<unsigned W> auto known(std::uint64_t value) {
  return gfsim::wire<gfsim::Bits<W>>::known(gfsim::Bits<W>{value});
}
template<unsigned W> auto unknown(bool z) {
  using B = gfsim::Bits<W>;
  return gfsim::wire<B>::fromPacked(gfsim::FourState<W>::fromMasks(
      B::ones(), B{0}, z ? B::ones() : B{0}));
}
#ifndef PACKED_WIDTH
#define PACKED_WIDTH 32
#endif
constexpr std::uint64_t fullMask = (std::uint64_t{1} << PACKED_WIDTH) - 1;
using Packed = gfsim::FourState<PACKED_WIDTH>;
void equal(Packed actual, std::uint64_t value, std::uint64_t mask = fullMask,
           std::uint64_t z = 0) {
  require(actual.invariantHolds());
  require(actual.knownMask().value() == mask && actual.zMask().value() == z);
  require((actual.value().value() & mask) == (value & mask));
}
void print(Packed value, const char *label) {
  std::cout << label << ' ';
  for (unsigned i = PACKED_WIDTH; i-- > 0;)
    std::cout << (value.zMask().bit(i) ? 'z' : !value.knownMask().bit(i) ? 'x' :
                 value.value().bit(i) ? '1' : '0');
  std::cout << '\n';
}
struct Harness {
  pyc_dut dut;
  pyc_dut::Inputs in;
  explicit Harness(unsigned workers) : dut(workers) {
    in.en_a = in.en_b = known<1>(0);
    in.data = in.other = known<8>(0);
    in.pyc_7079635f636c6b = in.pyc_7079635f727374 = known<1>(0);
    dut.drive(in); dut.system().Build(); dut.system().Reset();
    require(dut.system().state() == gfsim::SimSystemState::Ready);
  }
  Packed step(unsigned clock) {
    in.pyc_7079635f636c6b = known<1>(clock); dut.drive(in);
    require(dut.system().Step() == gfsim::SimStepResult::Running);
    return dut.sample().result.packed();
  }
};
#if !defined(TABLE_MODE) && !defined(GRANT_MODE) && !defined(FIELD_MODE)
void knownHistory(unsigned workers) {
  Harness h(workers);
#ifdef TABLES
  unsigned a = 0, b = 0, c = 0, side = 0;
#else
  unsigned a = 0, b = 0, c = 92, side = 9;
#endif
  bool last = false;
  constexpr std::array<unsigned, 14> clocks{0,1,1,0,1,0,1,0,1,1,0,1,0,1};
  for (unsigned n = 0; n < clocks.size(); ++n) {
    const unsigned data = (n * 37 + 17) & 255, other = (n * 61 + 33) & 255;
    const bool ena = n % 3 != 1, enb = n % 4 != 2, reset = n == 8 || n == 9;
    h.in.data = known<8>(data); h.in.other = known<8>(other);
    h.in.en_a = known<1>(ena); h.in.en_b = known<1>(enb);
    h.in.pyc_7079635f727374 = known<1>(reset);
    auto out = h.step(clocks[n]);
    equal(out, (a << 24) | (b << 16) | (c << 8) | side); print(out, "WORK");
    if (clocks[n] && !last) {
      if (reset) {
#ifdef TABLES
        a = b = c = side = 0;
#else
        a = b = 0; c = 92; side = 9;
#endif
      } else {
#ifdef TABLES
        a = data; b = other;
#elif defined(SOLO)
        if (ena) a = data;
        b = other; side ^= data;
#else
        const unsigned oldA = a, oldB = b;
        if (ena) a = oldB ^ data;
        if (enb) b = oldA ^ other;
        side ^= data;
#endif
      }
    }
    last = clocks[n];
  }
}

#ifndef TABLES
void unknownData(unsigned workers, bool z) {
  Harness h(workers);
  h.in.data = unknown<8>(z); h.in.other = known<8>(0x5a);
  h.in.en_a = known<1>(1); h.in.en_b = known<1>(0);
  equal(h.step(0), 0x00005c09); equal(h.step(1), 0x00005c09);
  const auto out = h.step(0);
#ifdef SOLO
  // Direct field transport retains Z; the independent scalar XOR produces X.
  equal(out, 0x005a5c00, 0x00ffff00, z ? 0xff000000 : 0);
#else
  // Both modified fields pass through XOR and therefore map Z to X.
  equal(out, 0x00005c00, 0x00ffff00);
#endif
  print(out, "MASK");
}

#ifdef SOLO
void singleRuleUnknownCondition(unsigned workers, bool z) {
  Harness h(workers);
  h.in.en_a = unknown<1>(z); h.in.en_b = known<1>(0);
  h.in.data = known<8>(255); h.in.other = known<8>(0x5a);
  equal(h.step(0), 0x00005c09); equal(h.step(1), 0x00005c09);
  const auto out = h.step(0);
  // One rule unconditionally writes B after conditional A: owner enable is 1.
  equal(out, 0x005a5cf6, 0x00ffffff); print(out, "MASK");
}
#else
void driveRoot(pyc_root &root, const pyc_dut::Inputs &in) {
  root.en_a = in.en_a; root.en_b = in.en_b;
  root.data = in.data; root.other = in.other;
  root.pyc_7079635f636c6b = in.pyc_7079635f636c6b;
  root.pyc_7079635f727374 = in.pyc_7079635f727374;
}
struct TestSystem final : gfsim::SimSystem {
  bool Precheck() noexcept override { return true; }
};
void rejectedEnable(unsigned workers, bool z, bool knownSibling) {
  // Public DUT guard is checked independently from actual owner observability.
  Harness h(workers);
  h.in.en_a = knownSibling ? known<1>(1) : unknown<1>(z);
  h.in.en_b = knownSibling ? unknown<1>(z) : known<1>(0);
  h.in.data = known<8>(0x33); h.in.other = known<8>(0x44);
  equal(h.step(0), 0x00005c09);
  const auto epoch = h.dut.system().cycle();
  h.in.pyc_7079635f636c6b = known<1>(1); h.dut.drive(h.in);
  require(h.dut.system().Step() == gfsim::SimStepResult::Failed);
  require(h.dut.system().cycle() == epoch);
  bool guarded = false;
  try { (void)h.dut.sample(); } catch (const std::logic_error &) { guarded = true; }
  require(guarded);
  h.dut.system().Reset();
  h.in.en_a = h.in.en_b = known<1>(0);
  equal(h.step(0), 0x00005c09);

  // An ordinary public root attached to SimSystem exposes outputs without
  // rewriting generated headers. Observe old Q by evaluating at a held low
  // clock after system failure; never commit this diagnostic evaluation.
  gfsim::WorkExecutor pool(workers); pyc_root root("atomic", &pool);
  TestSystem system;
  h.in.data = known<8>(0x11); h.in.other = known<8>(0x22);
  h.in.en_a = h.in.en_b = known<1>(1);
  driveRoot(root, h.in); require(system.AddModule(root)); system.Build(); system.Reset();
  auto step = [&](unsigned clock) {
    h.in.pyc_7079635f636c6b = known<1>(clock); driveRoot(root, h.in);
    require(system.Step() == gfsim::SimStepResult::Running);
  };
  step(0); step(1); step(0); equal(root.result.packed(), 0x11225c18);
  h.in.en_a = knownSibling ? known<1>(1) : unknown<1>(z);
  h.in.en_b = knownSibling ? unknown<1>(z) : known<1>(0);
  h.in.data = known<8>(0x33); h.in.other = known<8>(0x44);
  h.in.pyc_7079635f636c6b = known<1>(1); driveRoot(root, h.in);
  const auto before = system.cycle();
  require(system.Step() == gfsim::SimStepResult::Failed);
  require(system.cycle() == before);
  h.in.pyc_7079635f636c6b = known<1>(0);
  h.in.en_a = h.in.en_b = known<1>(0); driveRoot(root, h.in); root.Work();
  equal(root.result.packed(), 0x11225c18); // Struct AND independent scalar unchanged.
  root.DiscardNext(); root.Xfer();
  // Retry the failed rising edge directly: discard preserved clock history too.
  h.in.pyc_7079635f636c6b = known<1>(1);
  h.in.en_a = h.in.en_b = known<1>(1); driveRoot(root, h.in);
  root.Work(); root.Xfer();
  h.in.pyc_7079635f636c6b = known<1>(0); driveRoot(root, h.in); root.Work();
  equal(root.result.packed(), 0x11555c2b); root.DiscardNext();
}
#endif
#endif

#endif // Existing scalar/Struct cases.

#ifdef TABLE_MODE
struct TableModel {
  std::array<unsigned, 5> a{}, b{};
  std::uint64_t packed() const {
#if TABLE_MODE == 4
    return (std::uint64_t{a[0]} << 24) | (std::uint64_t{b[1]} << 16);
#endif
    std::uint64_t result = 0;
    for (unsigned cell = 0; cell < (TABLE_MODE == 5 ? 5 : TABLE_MODE == 6 ? 1 : 3); ++cell) {
#if TABLE_MODE == 5 || TABLE_MODE == 6
      result = (result << 8) | a[cell];
#elif TABLE_MODE == 1 || TABLE_MODE == 7
      result = (result << 5) | a[cell];
#else
      result = (result << 14) | (std::uint64_t{a[cell]} << 9) | b[cell];
#endif
    }
    return result;
  }
  void update(unsigned d, unsigned o, bool ea, bool eb) {
    const auto oldA = a, oldB = b;
#if TABLE_MODE == 5 || TABLE_MODE == 6
    if (ea) a[d % (TABLE_MODE == 5 ? 5 : 1)] = d;
#elif TABLE_MODE == 4
    a[0] = d; b[1] = d;
#elif TABLE_MODE == 0
    if (ea) a[d % 3] = (oldB[d % 3] & 31) ^ (d & 31);
    if (eb) b[o % 3] = oldA[o % 3] ^ (o | 256);
#elif TABLE_MODE == 1
    if (ea) a[0] = oldA[2] ^ (d & 31);
    if (eb) a[2] = oldA[0] ^ (o & 31);
#elif TABLE_MODE == 7
    if (ea) a[0] = d & 3;
    if (eb) a[2] = o & 3;
#elif TABLE_MODE == 2
    if (ea) { a[0] = d & 31; b[0] = o | 256; }
    if (eb) a[2] = o & 31;
#else
    if (ea) a[0] = d & 31;
    b[0] = o | 256;
    if (eb) a[2] = o & 31;
#endif
  }
};
std::uint64_t aMask(unsigned cell) {
#if TABLE_MODE == 4
  return std::uint64_t{255} << (cell ? 16 : 24);
#elif TABLE_MODE == 1 || TABLE_MODE == 7
  return std::uint64_t{TABLE_MODE == 7 ? 3u : 31u} << (5 * (2 - cell));
#else
  return std::uint64_t{31} << (14 * (2 - cell) + 9);
#endif
}
void tableHistory(unsigned workers) {
  Harness h(workers); TableModel model; bool last = false;
  constexpr std::array<unsigned, 14> clocks{0,1,1,0,1,0,1,0,1,1,0,1,0,1};
  for (unsigned n = 0; n < clocks.size(); ++n) {
    unsigned d = (n * 37 + 17) & 255, o = (n * 61 + 33) & 255;
    bool ea = n % 3 != 1, eb = n % 4 != 2, reset = n == 8 || n == 9;
    h.in.data = known<8>(d); h.in.other = known<8>(o);
    h.in.en_a = known<1>(ea); h.in.en_b = known<1>(eb);
    h.in.pyc_7079635f727374 = known<1>(reset);
    auto out = h.step(clocks[n]); equal(out, model.packed()); print(out, "WORK");
    if (clocks[n] && !last) {
      if (reset) model = {};
      else model.update(d, o, ea, eb);
    }
    last = clocks[n];
  }
  // Both writers commit to different dynamic cells (0 and 2), then observe Q.
  h.in.data=known<8>(3); h.in.other=known<8>(5);
  h.in.en_a=h.in.en_b=known<1>(1); h.in.pyc_7079635f727374=known<1>(0);
  for (unsigned clock : {0u,1u,0u}) {
    auto out=h.step(clock); equal(out,model.packed()); print(out,"WORK");
    if(clock&&!last)model.update(3,5,true,true);
    last=clock;
  }
}
void tableUnknownData(unsigned workers, bool z, bool condition = false) {
  Harness h(workers); TableModel expected;
  h.in.en_a = condition ? unknown<1>(z) : known<1>(1);
  h.in.en_b = known<1>(0);
  h.in.data = condition ? known<8>(31) : unknown<8>(z);
  h.in.other = known<8>(0x5a);
  equal(h.step(0), 0); equal(h.step(1), 0);
  auto mask = aMask(0);
#if TABLE_MODE == 5 || TABLE_MODE == 6
  mask = fullMask;
#elif TABLE_MODE == 4
  mask |= aMask(1);
#elif TABLE_MODE == 0
  // An unknown payload also makes its dynamic index unknown: every A may change.
  mask |= aMask(1) | aMask(2);
#elif TABLE_MODE == 2 || TABLE_MODE == 3
  expected.b[0] = 0x15a;
#endif
  std::uint64_t zPlane = 0;
#if (TABLE_MODE >= 2 && TABLE_MODE <= 4) || TABLE_MODE == 7
  if (z && !condition) zPlane = mask; // Direct data transport, unlike XOR.
#endif
  auto out = h.step(0); equal(out, expected.packed(), fullMask ^ mask, zPlane);
  print(out, "MASK");
}
void tableDrive(pyc_root &root, const pyc_dut::Inputs &in) {
  root.en_a=in.en_a; root.en_b=in.en_b; root.data=in.data; root.other=in.other;
  root.pyc_7079635f636c6b=in.pyc_7079635f636c6b;
  root.pyc_7079635f727374=in.pyc_7079635f727374;
}
struct TableSystem final : gfsim::SimSystem { bool Precheck() noexcept override { return true; } };
void tableDiscard(unsigned workers, bool z, bool sibling) {
  Harness h(workers); gfsim::WorkExecutor pool(workers); pyc_root root("table", &pool);
  TableSystem system; TableModel model;
  tableDrive(root,h.in); require(system.AddModule(root)); system.Build(); system.Reset();
  auto prepare = [&](unsigned clock, unsigned d, unsigned o) {
    h.in.pyc_7079635f636c6b=known<1>(clock);
    h.in.data=known<8>(d); h.in.other=known<8>(o); tableDrive(root,h.in);
  };
  h.in.en_a=h.in.en_b=known<1>(1);
  prepare(0,3,6); require(system.Step()==gfsim::SimStepResult::Running);
  prepare(1,3,6); require(system.Step()==gfsim::SimStepResult::Running); model.update(3,6,true,true);
  prepare(0,0,0); require(system.Step()==gfsim::SimStepResult::Running);
  equal(root.result.packed(),model.packed());
  h.in.en_a=sibling?known<1>(1):unknown<1>(z);
  h.in.en_b=sibling?unknown<1>(z):known<1>(0);
  prepare(1,17,22); auto epoch=system.cycle();
  require(system.Step()==gfsim::SimStepResult::Failed); require(system.cycle()==epoch);
  h.in.en_a=h.in.en_b=known<1>(0); prepare(0,0,0); root.Work();
  equal(root.result.packed(),model.packed()); root.DiscardNext(); root.Xfer();
  // Discarding an explicit reset must not clear data or consume the rising edge.
  h.in.pyc_7079635f727374=known<1>(1); prepare(1,0,0); root.Work();
  root.DiscardNext(); root.Xfer(); h.in.pyc_7079635f727374=known<1>(0);
  h.in.en_a=h.in.en_b=known<1>(1); prepare(1,7,9); root.Work(); root.Xfer();
  model.update(7,9,true,true); prepare(0,0,0); root.Work();
  equal(root.result.packed(),model.packed()); root.DiscardNext();
}
#endif

#ifdef GRANT_MODE
struct GrantModel {
#if GRANT_MODE == 2
  unsigned a = 0, b = 0, c = 0, side = 9;
#else
  unsigned a = 7, b = 17, c = 29, side = 9;
#endif
  std::uint64_t packed() const {
    return (std::uint64_t{a} << 24) | (std::uint64_t{b} << 16) | (c << 8) | side;
  }
  void update(unsigned d, unsigned o, bool p, bool q) {
    const bool ga = p;
#ifdef GRANT_COMPLEMENT
    const bool gb = !p;
#else
    const bool gb = !p && q;
#endif
#if GRANT_MODE == 0
    a ^= ga ? d : gb ? o : d ^ o;
#elif GRANT_MODE == 1
    if (ga) { a = b ^ d; b = d; }
    else if (gb) a = b ^ o;
    else { a = b ^ (d ^ o); c = d ^ o; }
#elif GRANT_MODE == 2
    unsigned index = ga ? d % 2 : gb ? o % 2 : 0;
    unsigned value = ga ? d : gb ? o : d ^ o;
    if (index == 0) a = value; else b = value;
#elif GRANT_MODE == 5
    if (p) a ^= o;
#else
    a ^= p ? d : o;
#endif
    side ^= d;
  }
};
void grantHistory(unsigned workers) {
  Harness h(workers); GrantModel m; bool last = false;
  // Include hold-high epochs, all request combinations and reset, then observe
  // after each edge; a disabled last registration must not erase its winner.
  constexpr std::array<unsigned,20> clocks{0,1,0,1,0,1,0,1,1,0,1,0,1,0,1,0,1,0,1,0};
  for (unsigned n=0; n<clocks.size(); ++n) {
    unsigned d=(n*37+17)&255, o=(n*61+33)&255;
    bool p=n%4<2, q=n%3!=0, reset=n==12;
    h.in.data=known<8>(d); h.in.other=known<8>(o);
    h.in.en_a=known<1>(p); h.in.en_b=known<1>(q);
    h.in.pyc_7079635f727374=known<1>(reset);
    auto out=h.step(clocks[n]); equal(out,m.packed()); print(out,"WORK");
    if(clocks[n]&&!last) { if(reset)m={}; else m.update(d,o,p,q); }
    last=clocks[n];
  }
}
void grantUnknownData(unsigned workers, bool z) {
  Harness h(workers); GrantModel m;
  h.in.en_a=known<1>(1); h.in.en_b=known<1>(0);
  h.in.data=unknown<8>(z); h.in.other=known<8>(0x5a);
  equal(h.step(0),m.packed()); equal(h.step(1),m.packed());
#if GRANT_MODE == 1
  equal(h.step(0),m.packed(),0x0000ff00,z?0x00ff0000:0);
#elif GRANT_MODE == 2
  equal(h.step(0),m.packed(),0x0000ff00);
#elif GRANT_MODE == 5
  m.a ^= 0x5a; equal(h.step(0),m.packed(),0xffffff00);
#else
  equal(h.step(0),m.packed(),0x00ffff00);
#endif
  print(h.dut.sample().result.packed(),"MASK");
}
void grantDrive(pyc_root &root,const pyc_dut::Inputs &in) {
  root.en_a=in.en_a; root.en_b=in.en_b; root.data=in.data; root.other=in.other;
  root.pyc_7079635f636c6b=in.pyc_7079635f636c6b;
  root.pyc_7079635f727374=in.pyc_7079635f727374;
}
struct GrantSystem final : gfsim::SimSystem { bool Precheck() noexcept override { return true; } };
void grantDiscard(unsigned workers,bool z) {
  Harness h(workers); gfsim::WorkExecutor pool(workers); pyc_root root("grant",&pool);
  GrantSystem system; GrantModel m;
  grantDrive(root,h.in); require(system.AddModule(root)); system.Build(); system.Reset();
  auto prepare=[&](unsigned clk,unsigned d,unsigned o) {
    h.in.pyc_7079635f636c6b=known<1>(clk);h.in.data=known<8>(d);h.in.other=known<8>(o);grantDrive(root,h.in);
  };
  h.in.en_a=known<1>(1);h.in.en_b=known<1>(0);
  prepare(0,17,34);require(system.Step()==gfsim::SimStepResult::Running);
  prepare(1,17,34);require(system.Step()==gfsim::SimStepResult::Running);m.update(17,34,true,false);
  prepare(0,0,0);require(system.Step()==gfsim::SimStepResult::Running);equal(root.result.packed(),m.packed());
  // The priority grant q & ~p is known zero when p=1,q=X/Z. This
  // succeeds. PoisonGrant deliberately retains a separate unknown owner.
  h.in.en_a=known<1>(1);h.in.en_b=unknown<1>(z);prepare(1,51,68);
#if GRANT_MODE == 4
  auto epoch=system.cycle();require(system.Step()==gfsim::SimStepResult::Failed);require(system.cycle()==epoch);
#else
  require(system.Step()==gfsim::SimStepResult::Running);m.update(51,68,true,false);
  prepare(0,0,0);require(system.Step()==gfsim::SimStepResult::Running);
#endif
  h.in.en_a=unknown<1>(z);h.in.en_b=known<1>(0);prepare(1,85,102);
  auto before=system.cycle();require(system.Step()==gfsim::SimStepResult::Failed);require(system.cycle()==before);
  // The public sample guard must reject after an unknown owner too.
  h.in.pyc_7079635f636c6b=known<1>(0);h.dut.drive(h.in);
  require(h.dut.system().Step()==gfsim::SimStepResult::Running);
  h.in.pyc_7079635f636c6b=known<1>(1);h.dut.drive(h.in);
  require(h.dut.system().Step()==gfsim::SimStepResult::Failed);
  bool guarded=false;try{(void)h.dut.sample();}catch(const std::logic_error&){guarded=true;}require(guarded);
  h.dut.system().Reset();h.in.en_a=h.in.en_b=known<1>(0);equal(h.step(0),GrantModel{}.packed());
  // Inspect actual Q and discard diagnostic work. Retry the same rising edge
  // after discarding a reset: neither owner Q nor clock history may change.
  prepare(0,0,0);root.Work();equal(root.result.packed(),m.packed());root.DiscardNext();root.Xfer();
  h.in.pyc_7079635f727374=known<1>(1);prepare(1,0,0);root.Work();root.DiscardNext();root.Xfer();
  h.in.pyc_7079635f727374=known<1>(0);h.in.en_a=known<1>(0);h.in.en_b=known<1>(1);
  prepare(1,7,9);root.Work();root.Xfer();m.update(7,9,false,true);
  prepare(0,0,0);root.Work();equal(root.result.packed(),m.packed());root.DiscardNext();
}
#endif


#ifdef FIELD_MODE
void fieldRows(unsigned workers, const char *path) {
  std::ifstream rows(path); require(bool(rows));
  gfsim::WorkExecutor pool(workers); pyc_root root("field-footprint", &pool);
  root.en_a=root.en_b=known<1>(0);root.data=root.other=known<8>(0);
  root.pyc_7079635f636c6b=root.pyc_7079635f727374=known<1>(0);
  root.Build(); root.Reset(); root.Xfer();
  auto plane=[](unsigned value,unsigned knownMask,unsigned zMask,unsigned width) {
    require(width==1 || width==8);
    return gfsim::FourState<8>::fromMasks(gfsim::Bits<8>{value},gfsim::Bits<8>{knownMask},gfsim::Bits<8>{zMask});
  };
  unsigned clock,reset,av,ak,az,bv,bk,bz,dv,dk,dz,ov,ok,oz,action,four;
  std::uint64_t expectedValue,expectedKnown,expectedZ,valueMask;
  unsigned sampled=0,discarded=0,failed=0,discardedReset=0;
  while(rows>>std::hex>>clock>>reset>>av>>ak>>az>>bv>>bk>>bz>>dv>>dk>>dz>>ov>>ok>>oz>>action>>four
            >>expectedValue>>expectedKnown>>expectedZ>>valueMask) {
    auto flag=[](unsigned v,unsigned k,unsigned z){return gfsim::wire<gfsim::Bits<1>>::fromPacked(
      gfsim::FourState<1>::fromMasks(gfsim::Bits<1>{v},gfsim::Bits<1>{k},gfsim::Bits<1>{z}));};
    root.en_a=flag(av,ak,az);root.en_b=flag(bv,bk,bz);
    root.data=decltype(root.data)::fromPacked(plane(dv,dk,dz,8));
    root.other=decltype(root.other)::fromPacked(plane(ov,ok,oz,8));
    root.pyc_7079635f636c6b=known<1>(clock);root.pyc_7079635f727374=known<1>(reset);
    if(action==3){root.Reset();root.Xfer();continue;}
    if(action==4){root.Reset();root.DiscardNext();root.Xfer();++discardedReset;continue;}
    if(action==2){
      bool rejected=false;
      try{root.Work();}catch(const gfsim::FourStateViolation&){rejected=true;}
      require(rejected);root.DiscardNext();root.Xfer();++failed;continue;
    }
    root.Work();auto actual=root.result.packed();
    require(actual.invariantHolds());
    if(actual.knownMask().value()!=expectedKnown || actual.zMask().value()!=expectedZ ||
       (actual.value().value()&valueMask)!=(expectedValue&valueMask)) {
      std::cerr<<"field mode "<<FIELD_MODE<<" sample "<<sampled<<" expected "<<std::hex
               <<expectedValue<<'/'<<expectedKnown<<'/'<<expectedZ<<" actual "
               <<actual.value().value()<<'/'<<actual.knownMask().value()<<'/'<<actual.zMask().value()<<'\n';
      require(false);
    }
    print(actual,four?"MASK":"WORK");++sampled;
    if(action==1){root.DiscardNext();++discarded;}
    root.Xfer();
  }
  require(rows.eof()&&sampled>30&&discarded==2&&discardedReset==1);
#if FIELD_MODE >= 9
  require(failed==4);
#elif FIELD_MODE != 6 && FIELD_MODE != 8
  require(failed==2);
#endif
}
#endif

int main(int argc, char **argv) {
  require(argc ==
#ifdef FIELD_MODE
    3
#else
    2
#endif
  ); unsigned workers = std::strtoul(argv[1], nullptr, 10);
  require(workers == 1 || workers == 2);
#ifdef FIELD_MODE
  fieldRows(workers,argv[2]);
#elif defined(GRANT_MODE)
  grantHistory(workers);
  for(bool z:{false,true}){grantUnknownData(workers,z);grantDiscard(workers,z);}
#elif defined(TABLE_MODE)
  tableHistory(workers);
  for (bool z : {false, true}) {
    tableUnknownData(workers,z);
#if TABLE_MODE == 3
    tableUnknownData(workers,z,true);
    tableDiscard(workers,z,true);
#elif TABLE_MODE == 5 || TABLE_MODE == 6
    tableDiscard(workers,z,false);
#elif TABLE_MODE != 4
    tableDiscard(workers,z,false); tableDiscard(workers,z,true);
#endif
  }
#else
  knownHistory(workers);
#ifndef TABLES
  for (bool z : {false, true}) {
    unknownData(workers, z);
#ifdef SOLO
    singleRuleUnknownCondition(workers, z);
#else
    for (bool knownSibling : {false, true}) rejectedEnable(workers, z, knownSibling);
#endif
  }
#endif
#endif
  std::cout << "PASS\n";
}
