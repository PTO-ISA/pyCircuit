// Physical IR pyc_clk/pyc_rst use the existing injective generated identifier
// spelling: pyc_7079635f636c6b / pyc_7079635f727374.
#include "gfsim/SimExecutor.h"
#include "gfsim/SystemRunner.h"
#include "pycircuit_system.hpp"

#include <array>
#include <cstdlib>
#include <iostream>
#include <source_location>
#include <stdexcept>
#include <string_view>
#include <vector>

void require(bool ok,
             std::source_location at = std::source_location::current()) {
  if (!ok) {
    std::cerr << "ROB independent oracle failed at " << at.line() << '\n';
    std::abort();
  }
}
template <unsigned Width> auto known(unsigned value) {
  return gfsim::wire<gfsim::Bits<Width>>::known(gfsim::Bits<Width>{value});
}
struct Event {
  unsigned allocate = 0, tag = 0, complete = 0, index = 0;
  unsigned completeTag = 0, value = 0, retire = 0, flush = 0;
};
struct Result {
  unsigned allocated = 0, completed = 0, retired = 0;
  unsigned index = 0, tag = 0, value = 0, count = 0;
};
// This software scoreboard predicts only expected results. Every actual result
// below is read from the generated DUT; the scoreboard never drives DUT state.
struct Golden {
  struct Entry { bool valid = false, done = false; unsigned tag = 0, value = 0; };
  std::array<Entry, 4> entries{};
  unsigned head = 0, tail = 0, count = 0;
  Result transact(const Event &event) {
    Result result;
    if (event.flush) {
      *this = {};
      return result;
    }
    Entry &completed = entries[event.index];
    if (event.complete && completed.valid && completed.tag == event.completeTag) {
      completed.done = true;
      completed.value = event.value;
      result.completed = 1;
    }
    Entry &retired = entries[head];
    if (event.retire && count && retired.done) {
      result.retired = 1;
      result.tag = retired.tag;
      result.value = retired.value;
      retired.valid = false;
      head = (head + 1) % entries.size();
      --count;
    }
    if (event.allocate && count < entries.size()) {
      result.allocated = 1;
      result.index = tail;
      entries[tail] = {true, false, event.tag, 0};
      tail = (tail + 1) % entries.size();
      ++count;
    }
    result.count = count;
    return result;
  }
};
std::vector<Event> events() {
  std::vector<Event> trace{
      {0,0,1,0,0,7,1,0}, {1,11}, {1,12}, {1,13}, {1,14}, {1,15},
      {0,0,1,0,99,999}, {0,0,1,2,13,0x1333}, {0,0,0,0,0,0,1},
      {0,0,1,0,11,0x1111,1}, {1,15}, {1,16,1,1,12,0x1222,1},
      {0,0,1,1,12,0xffff}, {0,0,1,0,15,0x1555,1},
      {0,0,0,0,0,0,1}, {1,17,1,3,14,0x1444,1},
      {1,18,1,0,15,0x1555,1}, {0,0,1,1,16,0x1666,1},
      {0,0,1,2,17,0x1777,1}, {0,0,1,3,18,0x1888,1},
      {0,0,1,3,18,9,1}, {1,21}, {1,22,1,0,21,21,1,1},
      {1,31}, {0,0,1,0,21,21,1}, {0,0,1,0,31,0x3131,1},
      {0,0,0,0,0,0,0,1}};
  std::uint32_t random = 0x6d2b79f5;
  for (unsigned n = 0; n < 256; ++n) {
    random ^= random << 13; random ^= random >> 17; random ^= random << 5;
    trace.push_back({random & 1u, (random >> 8) & 255u,
                     (random >> 1) & 1u, (random >> 2) & 3u,
                     (random >> 16) & 255u, (random >> 8) & 65535u,
                     (random >> 4) & 1u, unsigned((random & 127u) == 0)});
  }
  trace.push_back({0,0,0,0,0,0,0,1});
  return trace;
}
template <class Outputs>
void check(const Outputs &output, const Result &expected, bool print) {
  require(output.result.isFullyKnown());
  const auto result = output.result.value();
  auto value = [](const auto &bits) { return static_cast<unsigned>(bits.value()); };
  const std::array actual{value(result.allocate_accepted), value(result.complete_accepted),
                          value(result.retire_accepted), value(result.allocated_index),
                          value(result.retired_tag), value(result.retired_value), value(result.count)};
  require(actual == (std::array{expected.allocated, expected.completed, expected.retired,
                                expected.index, expected.tag, expected.value, expected.count}));
  if (print) {
    std::cout << "WORK";
    for (unsigned field : actual) std::cout << ' ' << field;
    std::cout << '\n';
  }
}
pyc_dut::Inputs inputs(const Event &event, unsigned clock) {
  pyc_dut::Inputs in;
  in.pyc_7079635f636c6b = known<1>(clock); in.pyc_7079635f727374 = known<1>(0);
  in.allocate = known<1>(event.allocate); in.allocate_tag = known<8>(event.tag);
  in.complete = known<1>(event.complete); in.complete_index = known<2>(event.index);
  in.complete_tag = known<8>(event.completeTag); in.complete_value = known<16>(event.value);
  in.retire = known<1>(event.retire); in.flush = known<1>(event.flush);
  return in;
}
struct RunnerContext {
  pyc_dut &dut;
  const std::vector<Event> trace = events();
  Golden golden;
  unsigned sampled = 0;
  unsigned frames() const { return 2 * trace.size() + 1; }
  static void initialize(void *opaque) { require(drive(opaque, 0)); }
  static bool drive(void *opaque, std::uint64_t epoch) {
    auto &self = *static_cast<RunnerContext *>(opaque);
    if (epoch == self.frames()) return false;
    require(epoch < self.frames());
    const bool high = epoch % 2;
    self.dut.drive(inputs(high ? self.trace[epoch / 2] : Event{}, high));
    return true;
  }
  static void sample(void *opaque, std::uint64_t epoch) {
    auto &self = *static_cast<RunnerContext *>(opaque);
    require(epoch == self.sampled + 1);
    const bool high = self.sampled % 2;
    const Result expected = self.golden.transact(high ? self.trace[self.sampled / 2] : Event{});
    check(self.dut.sample(), expected, true);
    ++self.sampled;
  }
};
void configure(gfsim::SimExecutor &executor) {
  constexpr std::string_view config = "{}";
  require(executor.ConfigureJson(reinterpret_cast<const std::uint8_t *>(config.data()),
                                 config.size()) == PYCIRCUIT_MODEL_STATUS_V1_OK);
}
void nativeFailure(unsigned workers) {
  pyc_dut dut(workers);
  gfsim::SimExecutor executor(dut.system(), dut.observations(), {});
  configure(executor);
  dut.drive(inputs({}, 0));
  require(executor.Reset() == PYCIRCUIT_MODEL_STATUS_V1_OK);
  auto in = inputs({1,77}, 1);
  in.allocate = gfsim::wire<gfsim::Bits<1>>::unknown();
  dut.drive(in);
  PycircuitModelStepResultV1 result{sizeof(result)};
  require(executor.Step(&result) == PYCIRCUIT_MODEL_STATUS_V1_RUNTIME_FAILURE);
  require(result.state == PYCIRCUIT_MODEL_STEP_V1_FAILED && executor.cycles() == 0);
  require(dut.system().cycle() == 0);
  bool rejected = false;
  try { (void)dut.sample(); } catch (const std::logic_error &) { rejected = true; }
  require(rejected);
  dut.drive(inputs({}, 0));
  require(executor.Reset() == PYCIRCUIT_MODEL_STATUS_V1_OK);
  dut.drive(inputs({1,77}, 1));
  require(executor.Step(&result) == PYCIRCUIT_MODEL_STATUS_V1_OK);
  check(dut.sample(), {1,0,0,0,0,0,1}, false);
  dut.drive(inputs({}, 0));
  require(executor.Step(&result) == PYCIRCUIT_MODEL_STATUS_V1_OK);
  check(dut.sample(), {0,0,0,0,0,0,1}, false);
}
void directDiscard(unsigned workers) {
  gfsim::WorkExecutor pool(workers);
  pyc_root root("discard_probe", &pool);
  Golden golden;
  auto drive = [&](const pyc_dut::Inputs &in) {
    root.pyc_7079635f636c6b=in.pyc_7079635f636c6b; root.pyc_7079635f727374=in.pyc_7079635f727374;
    root.allocate=in.allocate; root.allocate_tag=in.allocate_tag;
    root.complete=in.complete; root.complete_index=in.complete_index;
    root.complete_tag=in.complete_tag; root.complete_value=in.complete_value;
    root.retire=in.retire; root.flush=in.flush;
  };
  drive(inputs({}, 0)); root.Reset(); root.Xfer();
  auto work = [&](const Event &event, unsigned clock) {
    drive(inputs(event, clock));
    root.Work();
    check(root, golden.transact(event), false);
    root.Xfer();
  };
  auto event = [&](const Event &operation) { work({}, 0); work(operation, 1); };
  event({1,41}); event({1,42}); event({1,43});
  event({0,0,1,0,41,0x4141});
  work({}, 0);
  const Event transaction{1,44,1,1,42,0x4242,1};
  // Every storage leaf has prepared its next data and clock history. Discard
  // plus an accidental Xfer must publish neither, including head/tail/count.
  drive(inputs(transaction, 1)); root.Work();
  Golden proposed = golden;
  check(root, proposed.transact(transaction), false);
  root.DiscardNext(); root.Xfer();
  // An unknown allocation guard reaches the inferred standard-leaf enable
  // and rejects a real generated Work at the same rising edge.
  // The successful retry below distinguishes retained state and clock history
  // from any partial commit; later drains distinguish every occupied payload.
  auto bad = inputs(transaction, 1);
  bad.allocate = gfsim::wire<gfsim::Bits<1>>::unknown();
  drive(bad);
  bool rejected = false;
  try { root.Work(); } catch (const gfsim::FourStateViolation &) { rejected = true; }
  require(rejected);
  root.DiscardNext(); root.Xfer();
  work(transaction, 1);
  work({}, 0);
  event({0,0,0,0,0,0,1});
  event({0,0,1,2,43,0x4343,1});
  event({0,0,1,3,44,0x4444,1});
  work({}, 0);
  require(golden.count == 0);
  // A subsequent allocation must use the original tail after exactly four
  // accepted allocations, closing the pointer-wrap and discarded-write check.
  event({1,45}); event({0,0,1,0,45,0x4545,1}); work({}, 0);
}
int main(int argc, char **argv) {
  gfsim::SystemRunner runner(argc, argv);
  if (!runner.ready()) return 2;
  pyc_dut dut(runner.workers());
  RunnerContext context{dut};
  const gfsim::RunnerCallbacks callbacks{&context, &RunnerContext::initialize,
                                         &RunnerContext::drive, &RunnerContext::sample};
  const int result = runner.Run(dut.system(), dut.observations(), {}, callbacks);
  require(result == 0 && context.sampled == context.frames());
  nativeFailure(runner.workers());
  directDiscard(runner.workers());
  return result;
}
