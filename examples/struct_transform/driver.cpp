// Independent baseline oracle: all actual values come from the generated DUT.
#include "gfsim/SystemRunner.h"
#include "pycircuit_system.hpp"
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <source_location>
#include <vector>

void require(bool ok,
             std::source_location at = std::source_location::current()) {
  if (!ok) {
    std::cerr << "struct_transform oracle failed at " << at.line() << '\n';
    std::abort();
  }
}
template <unsigned W> auto known(std::uint32_t value) {
  return gfsim::wire<gfsim::Bits<W>>::known(gfsim::Bits<W>{value});
}
struct Packet {
  std::uint32_t op = 0, dst = 0, word = 0, valid = 0;
};
struct Row {
  unsigned clock = 0, reset = 0;
  Packet input;
};
// Original smoke first, then changing data on repeated levels, reset priority,
// modular boundaries and deterministic traffic. No expected value uses a DUT.
std::vector<Row> stimulus() {
  std::vector<Row> rows;
  auto row = [&](unsigned clock, unsigned reset, Packet input) {
    rows.push_back({clock, reset, input});
  };
  const Packet smoke{1, 2, 3, 1}, wrap{15, 63, 0xffffffffu, 0};
  row(0, 0, smoke);
  row(1, 0, smoke);
  row(0, 0, smoke);
  row(1, 0, wrap);
  row(1, 0, {3, 4, 19, 1});
  row(1, 0, {4, 5, 20, 0});
  row(0, 0, {5, 6, 21, 1});
  row(0, 0, {6, 7, 22, 0});
  row(1, 0, smoke);
  row(0, 0, smoke);
  row(0, 1, wrap);
  row(1, 1, wrap);
  row(1, 0, wrap);
  row(0, 0, wrap);
  row(1, 0, wrap);
  row(0, 0, wrap);
  row(1, 0, smoke);
  row(0, 0, smoke);
  std::uint32_t random = 0x83a7d529u;
  for (unsigned n = 0; n < 64; ++n) {
    random = random * 1664525u + 1013904223u;
    Packet value{(random >> 28) & 15u, (random >> 21) & 63u, random, n & 1u};
    if (n == 0)
      value.word = 0xfffffff0u;
    if (n == 1)
      value.word = 0xffffffffu;
    if (n == 2)
      value.word = 0;
    row(0, 0, value);
    row(1, n == 31, value);
  }
  row(0, 0, smoke);
  row(1, 0, smoke);
  row(0, 0, smoke);
  return rows;
}
struct Golden {
  Packet first{}, second{};
  bool lastClock = false;
  Packet output() const {
    Packet out = first;
    out.word = first.word + first.op + 1u;
    return out;
  }
  void commit(const Row &row) {
    if (row.clock && !lastClock) {
      if (row.reset) {
        first = {};
        second = {};
      } else {
        first = row.input;
      }
    }
    lastClock = row.clock;
  }
};
pyc_dut::Inputs inputs(const Row &row) {
  pyc_dut::Inputs in;
  in.pyc_7079635f636c6b = known<1>(row.clock);
  in.pyc_7079635f727374 = known<1>(row.reset);
  in.op = known<4>(row.input.op);
  in.dst = known<6>(row.input.dst);
  in.word = known<32>(row.input.word);
  in.valid = known<1>(row.input.valid);
  return in;
}
template <class Output>
void check(const Output &output, const Packet &expected, bool emit = false) {
  const auto &packed = output.result.packed();
  require(output.result.isFullyKnown());
  require(gfsim::extract<4>(packed, 39).value() == gfsim::Bits<4>{expected.op});
  require(gfsim::extract<6>(packed, 33).value() ==
          gfsim::Bits<6>{expected.dst});
  require(gfsim::extract<32>(packed, 1).value() ==
          gfsim::Bits<32>{expected.word});
  require(gfsim::extract<1>(packed, 0).value() ==
          gfsim::Bits<1>{expected.valid});
  if (emit)
    std::cout << "WORK " << gfsim::extract<4>(packed, 39).value().value() << ' '
              << gfsim::extract<6>(packed, 33).value().value() << ' '
              << gfsim::extract<32>(packed, 1).value().value() << ' '
              << gfsim::extract<1>(packed, 0).value().value() << '\n';
}
struct RunnerContext {
  pyc_dut &dut;
  std::vector<Row> rows = stimulus();
  Golden golden;
  unsigned sampled = 0;
  static void initialize(void *opaque) { require(drive(opaque, 0)); }
  static bool drive(void *opaque, std::uint64_t epoch) {
    auto &self = *static_cast<RunnerContext *>(opaque);
    if (epoch == self.rows.size())
      return false;
    require(epoch < self.rows.size());
    self.dut.drive(inputs(self.rows[epoch]));
    return true;
  }
  static void sample(void *opaque, std::uint64_t epoch) {
    auto &self = *static_cast<RunnerContext *>(opaque);
    require(epoch == self.sampled + 1 && self.sampled < self.rows.size());
    check(self.dut.sample(), self.golden.output(), true);
    self.golden.commit(self.rows[self.sampled]);
    ++self.sampled;
  }
};
void directDiscard(unsigned workers) {
  gfsim::WorkExecutor pool(workers);
  pyc_root root("discard_probe", &pool);
  Golden golden;
  auto drive = [&](const pyc_dut::Inputs &in) {
    root.pyc_7079635f636c6b = in.pyc_7079635f636c6b;
    root.pyc_7079635f727374 = in.pyc_7079635f727374;
    root.op = in.op;
    root.dst = in.dst;
    root.word = in.word;
    root.valid = in.valid;
  };
  drive(inputs({}));
  root.Reset();
  root.Xfer();
  auto work = [&](Row row) {
    drive(inputs(row));
    root.Work();
    check(root, golden.output());
    root.Xfer();
    golden.commit(row);
  };
  const Packet initial{1, 2, 3, 1}, retry{15, 63, 0xffffffffu, 0};
  work({0, 0, initial});
  work({1, 0, initial});
  work({0, 0, initial});
  // Discard must cancel both payload and edge history, including an accidental
  // Xfer. The same high level must still be accepted as the retry rising edge.
  drive(inputs({1, 0, retry}));
  root.Work();
  check(root, golden.output());
  root.DiscardNext();
  root.Xfer();
  work({1, 0, retry});
  work({0, 0, initial});
  work({1, 0, initial});
  work({0, 0, initial});
  // Reset proposals are also cancellable: discarded reset cannot erase state.
  drive(inputs({1, 1, {}}));
  root.Work();
  check(root, golden.output());
  root.DiscardNext();
  root.Xfer();
  work({1, 0, retry});
  work({0, 0, retry});
}
void nativeFourState(unsigned workers, bool z) {
  gfsim::WorkExecutor pool(workers);
  pyc_root root("four_state_probe", &pool);
  auto in = inputs({0, 0, {1, 2, 3, 1}});
  auto drive = [&] {
    root.pyc_7079635f636c6b = in.pyc_7079635f636c6b;
    root.pyc_7079635f727374 = in.pyc_7079635f727374;
    root.op = in.op;
    root.dst = in.dst;
    root.word = in.word;
    root.valid = in.valid;
  };
  drive();
  root.Reset();
  root.Xfer();
  in.valid = gfsim::wire<gfsim::Bits<1>>::fromPacked(
      z ? gfsim::FourState<1>::highImpedance()
        : gfsim::FourState<1>::unknown());
  auto step = [&](unsigned clock) {
    in.pyc_7079635f636c6b = known<1>(clock);
    drive();
    root.Work();
    root.Xfer();
  };
  step(0);
  step(1);
  in.pyc_7079635f636c6b = known<1>(0);
  drive();
  root.Work();
  const auto valid = gfsim::extract<1>(root.result.packed(), 0);
  require(valid.knownMask() == gfsim::Bits<1>{0});
  require(valid.zMask() == gfsim::Bits<1>{z ? 1u : 0u});
  root.Xfer();
  // Synchronous reset recovers both data and four-state control planes.
  in.pyc_7079635f727374 = known<1>(1);
  step(1);
  in.pyc_7079635f636c6b = known<1>(0);
  drive();
  root.Work();
  check(root, Golden{}.output());
  root.Xfer();
}

void nativeUnknownArithmetic(unsigned workers, bool z, bool unknownOp) {
  gfsim::WorkExecutor pool(workers);
  pyc_root root("arithmetic_probe", &pool);
  auto in = inputs({0, 0, {1, 2, 3, 1}});
  auto drive = [&] {
    root.pyc_7079635f636c6b = in.pyc_7079635f636c6b;
    root.pyc_7079635f727374 = in.pyc_7079635f727374;
    root.op = in.op;
    root.dst = in.dst;
    root.word = in.word;
    root.valid = in.valid;
  };
  drive();
  root.Reset();
  root.Xfer();
  if (unknownOp)
    in.op = gfsim::wire<gfsim::Bits<4>>::fromPacked(
        z ? gfsim::FourState<4>::highImpedance()
          : gfsim::FourState<4>::unknown());
  else
    in.word = gfsim::wire<gfsim::Bits<32>>::fromPacked(
        z ? gfsim::FourState<32>::highImpedance()
          : gfsim::FourState<32>::unknown());
  auto step = [&](unsigned clock) {
    in.pyc_7079635f636c6b = known<1>(clock);
    drive();
    root.Work();
    root.Xfer();
  };
  step(0);
  step(1);
  in.pyc_7079635f636c6b = known<1>(0);
  drive();
  root.Work();
  const auto word = gfsim::extract<32>(root.result.packed(), 1);
  // Arithmetic consumes X or Z as unknown; unlike copied packet fields, the
  // result has no retained Z plane and no known arithmetic bits.
  require(word.knownMask() == gfsim::Bits<32>{0});
  require(word.zMask() == gfsim::Bits<32>{0});
  if (unknownOp) {
    const auto op = gfsim::extract<4>(root.result.packed(), 39);
    require(op.knownMask() == gfsim::Bits<4>{0});
    require(op.zMask() == gfsim::Bits<4>{z ? 15u : 0u});
  }
  root.Xfer();
  in.pyc_7079635f727374 = known<1>(1);
  step(1);
  in.pyc_7079635f636c6b = known<1>(0);
  drive();
  root.Work();
  check(root, Golden{}.output());
  root.Xfer();
}

int main(int argc, char **argv) {
  gfsim::SystemRunner runner(argc, argv);
  if (!runner.ready())
    return 2;
  pyc_dut dut(runner.workers());
  RunnerContext context{dut};
  const gfsim::RunnerCallbacks callbacks{&context, &RunnerContext::initialize,
                                         &RunnerContext::drive,
                                         &RunnerContext::sample};
  const int result =
      runner.Run(dut.system(), dut.observations(), {}, callbacks);
  require(result == 0 && context.sampled == context.rows.size());
  directDiscard(runner.workers());
  nativeFourState(runner.workers(), false);
  nativeFourState(runner.workers(), true);
  for (bool z : {false, true}) {
    nativeUnknownArithmetic(runner.workers(), z, false);
    nativeUnknownArithmetic(runner.workers(), z, true);
  }
  return result;
}
