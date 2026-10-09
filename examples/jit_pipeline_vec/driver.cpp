// Independent three-stage scoreboard: expected values never come from a DUT.
#include "gfsim/SystemRunner.h"
#include "pycircuit_system.hpp"
#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <source_location>
#include <vector>

void require(bool ok,
             std::source_location at = std::source_location::current()) {
  if (!ok) {
    std::cerr << "jit_pipeline_vec oracle failed at " << at.line() << '\n';
    std::abort();
  }
}
template <unsigned W> auto known(std::uint32_t value) {
  return gfsim::wire<gfsim::Bits<W>>::known(gfsim::Bits<W>{value});
}
struct Input {
  std::uint32_t a = 0, b = 0, sel = 0;
};
struct Packet {
  std::uint32_t tag = 0, data = 0;
};
struct Row {
  unsigned clock = 0, reset = 0;
  Input input;
};
Packet compute(Input input) {
  return {input.a == input.b ? 1u : 0u,
          input.sel ? (input.a + input.b) % 65536u : input.a ^ input.b};
}
std::vector<Row> stimulus() {
  std::vector<Row> rows;
  auto row = [&](unsigned clock, unsigned reset, Input input) {
    rows.push_back({clock, reset, input});
  };
  const Input smoke{1, 1, 1}, wrap{65535, 1, 1}, xorPacket{85, 170, 0};
  // Retain original smoke, three real capture edges, then repeated levels with
  // different inputs, modular boundaries, selection changes and reset priority.
  row(0,0,smoke);row(1,0,smoke);row(1,0,wrap);row(0,0,wrap);
  row(1,0,wrap);row(0,0,xorPacket);row(1,0,xorPacket);row(0,0,{3,1,0});
  row(1,0,{3,1,0});row(1,0,{32768,32768,1});row(0,0,{17,31,1});
  row(0,1,wrap);row(1,1,wrap);row(1,0,smoke);row(0,0,smoke);
  row(1,0,smoke);row(0,0,wrap);row(1,0,wrap);
  std::uint32_t random = 0x83a7d529u;
  for (unsigned n = 0; n != 64; ++n) {
    random = random * 1664525u + 1013904223u;
    Input input{random >> 16, random & 65535u, n & 1u};
    if (n == 0) input = wrap;
    if (n == 1) input = {65535,65535,1};
    if (n == 2) input = {0,65535,0};
    if (n == 3) input = {32768,32768,1};
    if (n == 4) input = xorPacket;
    if (n == 5) input = {85,170,1};
    if (n == 6) input = {3,1,0};
    if (n == 7) input = smoke;
    row(0,0,input);row(1,n == 31,input);
  }
  for (unsigned edge = 0; edge != 3; ++edge) {
    row(0,0,smoke);row(1,0,smoke);
  }
  row(0,0,smoke);
  return rows;
}
struct Golden {
  std::array<Packet,3> stages{};
  bool lastClock = false;
  Packet output() const { return stages[2]; }
  void commit(Row row) {
    if (row.clock && !lastClock) {
      if (row.reset) stages = {};
      else {
        stages[2] = stages[1];stages[1] = stages[0];stages[0] = compute(row.input);
      }
    }
    lastClock = row.clock;
  }
};
pyc_dut::Inputs inputs(Row row) {
  pyc_dut::Inputs input;
  input.pyc_7079635f636c6b = known<1>(row.clock);
  input.pyc_7079635f727374 = known<1>(row.reset);
  input.a = known<16>(row.input.a);input.b = known<16>(row.input.b);
  input.sel = known<1>(row.input.sel);
  return input;
}
template <class Root> void drive(Root &root, const pyc_dut::Inputs &input) {
  root.pyc_7079635f636c6b = input.pyc_7079635f636c6b;
  root.pyc_7079635f727374 = input.pyc_7079635f727374;
  root.a = input.a;root.b = input.b;root.sel = input.sel;
}
template <class Output> void check(const Output &output, Packet expected, bool emit = false) {
  const auto &value = output.result.packed();
  require(output.result.isFullyKnown());
  require(gfsim::extract<1>(value,24).value() == gfsim::Bits<1>{expected.tag});
  require(gfsim::extract<16>(value,8).value() == gfsim::Bits<16>{expected.data});
  require(gfsim::extract<8>(value,0).value() == gfsim::Bits<8>{expected.data % 256u});
  if (emit) std::cout << "WORK " << gfsim::extract<1>(value,24).value().value() << ' '
                      << gfsim::extract<16>(value,8).value().value() << ' '
                      << gfsim::extract<8>(value,0).value().value() << '\n';
}
struct RunnerContext {
  pyc_dut &dut;
  std::vector<Row> rows = stimulus();
  Golden golden;
  unsigned sampled = 0;
  static void initialize(void *opaque) { require(driveInputs(opaque,0)); }
  static bool driveInputs(void *opaque, std::uint64_t epoch) {
    auto &self = *static_cast<RunnerContext *>(opaque);
    if (epoch == self.rows.size()) return false;
    require(epoch < self.rows.size());
    self.dut.drive(inputs(self.rows[epoch]));return true;
  }
  static void sample(void *opaque, std::uint64_t epoch) {
    auto &self = *static_cast<RunnerContext *>(opaque);
    require(epoch == self.sampled + 1 && self.sampled < self.rows.size());
    check(self.dut.sample(),self.golden.output(),true);
    self.golden.commit(self.rows[self.sampled]);++self.sampled;
  }
};
void directDiscard(unsigned workers) {
  gfsim::WorkExecutor pool(workers);
  pyc_root root("discard_probe",&pool);
  Golden golden;
  drive(root,inputs({}));root.Reset();root.Xfer();
  auto work = [&](Row row) {
    drive(root,inputs(row));root.Work();check(root,golden.output());
    root.Xfer();golden.commit(row);
  };
  const Input smoke{1,1,1}, first{3,1,1}, second{85,170,0}, retry{65535,1,1};
  work({0,0,smoke});work({1,0,smoke});work({0,0,first});work({1,0,first});
  work({0,0,second});work({1,0,second});work({0,0,retry});
  // Both all three payload proposals and clock history must be discarded.
  drive(root,inputs({1,0,retry}));root.Work();check(root,golden.output());
  root.DiscardNext();root.Xfer();work({1,0,retry});
  for (unsigned n = 0; n != 3; ++n) {
    const Input following{17u+n,31u+n,n & 1u};
    work({0,0,following});work({1,0,following});
  }
  work({0,0,first});
  // A discarded reset cannot erase a partially occupied pipeline or consume
  // its rising edge; a same-high retry must advance the original three stages.
  drive(root,inputs({1,1,{}}));root.Work();check(root,golden.output());
  root.DiscardNext();root.Xfer();work({1,0,retry});
  for (unsigned n = 0; n != 3; ++n) {
    work({0,0,smoke});work({1,0,smoke});
  }
  work({0,0,smoke});
}
struct Masks { unsigned value, known, z; };
struct Expected { unsigned tag, tagKnown, data, dataKnown, dataZ; };
template <unsigned W> auto masked(Masks input) {
  return gfsim::wire<gfsim::Bits<W>>::fromPacked(gfsim::FourState<W>::fromMasks(
      gfsim::Bits<W>{input.value},gfsim::Bits<W>{input.known},gfsim::Bits<W>{input.z}));
}
template <class Output> void checkMasks(const Output &output, Expected expected) {
  const auto &packed = output.result.packed();
  const auto tag = gfsim::extract<1>(packed,24);
  const auto data = gfsim::extract<16>(packed,8);
  const auto low = gfsim::extract<8>(packed,0);
  require(tag.knownMask() == gfsim::Bits<1>{expected.tagKnown} && tag.zMask() == gfsim::Bits<1>{0});
  require((tag.value() & tag.knownMask()) == gfsim::Bits<1>{expected.tag});
  require(data.knownMask() == gfsim::Bits<16>{expected.dataKnown} && data.zMask() == gfsim::Bits<16>{expected.dataZ});
  require((data.value() & data.knownMask()) == gfsim::Bits<16>{expected.data});
  require(low.knownMask() == gfsim::Bits<8>{expected.dataKnown & 255u});
  require(low.zMask() == gfsim::Bits<8>{expected.dataZ & 255u});
  // The slice must retain all planes, including latent value bits behind X.
  for (unsigned bit = 0; bit != 8; ++bit) require(low.value().bit(bit) == data.value().bit(bit));
}
void nativeFourState(unsigned workers) {
  struct Case { Masks a,b,sel;Expected expected; };
  constexpr Case cases[] = {
    {{1,65535,0},{1,65535,0},{0,0,0},{1,1,0,65533,0}},
    {{1,65535,0},{1,65535,0},{0,0,1},{1,1,0,65533,0}},
    {{3,65535,0},{1,65535,0},{0,0,0},{0,1,0,65529,0}},
    {{3,65535,0},{1,65535,0},{0,0,1},{0,1,0,65529,0}},
    {{85,65535,0},{170,65535,0},{0,0,0},{0,1,255,65535,0}},
    {{85,65535,0},{170,65535,0},{0,0,1},{0,1,255,65535,0}},
    {{1,65407,0},{0,65535,0},{0,1,0},{0,1,1,65407,0}},
    {{1,65407,128},{0,65535,0},{0,1,0},{0,1,1,65407,0}},
    {{1,65407,0},{0,65535,0},{1,1,0},{0,1,0,0,0}},
    {{1,65407,128},{0,65535,0},{1,1,0},{0,1,0,0,0}},
    {{0,0,0},{0,65535,0},{0,1,0},{0,0,0,0,0}},
    {{0,0,65535},{0,65535,0},{1,1,0},{0,0,0,0,0}},
  };
  for (const auto &row : cases) {
    gfsim::WorkExecutor pool(workers);pyc_root root("four_state_probe",&pool);
    auto input = inputs({});input.a=masked<16>(row.a);input.b=masked<16>(row.b);input.sel=masked<1>(row.sel);
    drive(root,input);root.Reset();root.Xfer();
    std::array<Expected,3> expected{{{0,1,0,65535,0},{0,1,0,65535,0},{0,1,0,65535,0}}};
    Expected incoming = row.expected;
    bool lastClock = false;
    auto work = [&](unsigned clock, unsigned reset) {
      input.pyc_7079635f636c6b=known<1>(clock);input.pyc_7079635f727374=known<1>(reset);
      drive(root,input);root.Work();checkMasks(root,expected[2]);root.Xfer();
      if (clock && !lastClock) {
        if (reset) expected={{{0,1,0,65535,0},{0,1,0,65535,0},{0,1,0,65535,0}}};
        else { expected[2]=expected[1];expected[1]=expected[0];expected[0]=incoming; }
      }
      lastClock=clock;
    };
    work(0,0);
    for (unsigned n = 0; n != 3; ++n) {work(1,0);work(0,0);}
    // No edge: changed inputs cannot bypass stage2 or alter the low-byte slice.
    input.a=known<16>(1);input.b=known<16>(1);input.sel=known<1>(1);
    incoming={1,1,2,65535,0};work(0,0);
    for (unsigned n = 0; n != 3; ++n) {work(1,0);work(0,0);}
    check(root,{1,2});
    work(0,1);work(1,1);work(0,0);
    check(root,{});
  }
}
int main(int argc, char **argv) {
  gfsim::SystemRunner runner(argc,argv);if (!runner.ready()) return 2;
  pyc_dut dut(runner.workers());RunnerContext context{dut};
  const gfsim::RunnerCallbacks callbacks{&context,&RunnerContext::initialize,
                                       &RunnerContext::driveInputs,&RunnerContext::sample};
  const int result=runner.Run(dut.system(),dut.observations(),{},callbacks);
  require(result==0 && context.sampled==context.rows.size());
  directDiscard(runner.workers());nativeFourState(runner.workers());
  return result;
}
