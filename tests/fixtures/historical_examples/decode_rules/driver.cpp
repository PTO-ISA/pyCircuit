#include "gfsim/SimExecutor.h"
#include "gfsim/SystemRunner.h"
#include "pycircuit_system.hpp"

// Observe the declared packed result without discarding X/Z planes. The
// historical stimulus, expected values and checks below remain unchanged.
struct ObservedOutputs {
  gfsim::wire<gfsim::Bits<4>> op;
  gfsim::wire<gfsim::Bits<3>> len;
};
ObservedOutputs observe(const pyc_dut::Outputs &output) {
  return {
      gfsim::wire<gfsim::Bits<4>>::fromPacked(
          gfsim::extract<4>(output.result.packed(), 3)),
      gfsim::wire<gfsim::Bits<3>>::fromPacked(
          gfsim::extract<3>(output.result.packed(), 0)),
  };
}
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>

void require(bool ok) {
  if (!ok) {
    std::cerr << "decode_rules fixed oracle failed\n";
    std::abort();
  }
}
template <unsigned W> auto wire(std::string_view bits) {
  require(bits.size() == W);
  gfsim::Bits<W> value{0}, known{0}, z{0};
  for (unsigned i = 0; i < W; ++i) {
    const unsigned bit = W - i - 1, word = bit / 64;
    const auto mask = std::uint64_t{1} << (bit % 64);
    const char c = bits[i];
    require(c == '0' || c == '1' || c == 'x' || c == 'z');
    if (c == '0' || c == '1')
      known.setWord(word, known.word(word) | mask);
    if (c == '1')
      value.setWord(word, value.word(word) | mask);
    if (c == 'z')
      z.setWord(word, z.word(word) | mask);
  }
  return gfsim::wire<gfsim::Bits<W>>::fromPacked(
      gfsim::FourState<W>::fromMasks(value, known, z));
}
template <unsigned W>
std::string check(const gfsim::wire<gfsim::Bits<W>> &actual,
                  std::string_view golden) {
  const auto expected = wire<W>(golden);
  const auto &a = actual.packed();
  const auto &e = expected.packed();
  require(a.knownMask() == e.knownMask() && a.zMask() == e.zMask());
  require((a.value() & a.knownMask()) == e.value());
  std::string result;
  for (unsigned i = 0; i < W; ++i) {
    const unsigned bit = W - i - 1;
    result += a.zMask().bit(bit)        ? 'z'
              : !a.knownMask().bit(bit) ? 'x'
              : a.value().bit(bit)      ? '1'
                                        : '0';
  }
  return result;
}
struct Row {
  unsigned insn, op, length;
};
constexpr unsigned knownFrames = 257;
Row knownRow(unsigned epoch) {
  require(epoch < knownFrames);
  if (epoch == 0)
    return {0x10, 1, 4}; // Exact historical smoke.
  const unsigned insn = epoch - 1;
  // Independent complete-byte oracle: three disjoint16-value ranges,
  // fixed op1/2/3 and len4. Every other byte has both outputs0.
  if (insn >= 16 && insn <= 31)
    return {insn, 1, 4};
  if (insn >= 32 && insn <= 47)
    return {insn, 2, 4};
  if (insn >= 48 && insn <= 63)
    return {insn, 3, 4};
  return {insn, 0, 0};
}
struct RunnerContext {
  pyc_dut &dut;
  unsigned sampled = 0;
  static void initialize(void *opaque) { require(drive(opaque, 0)); }
  static bool drive(void *opaque, std::uint64_t epoch) {
    auto &self = *static_cast<RunnerContext *>(opaque);
    if (epoch == knownFrames)
      return false;
    const auto row = knownRow(epoch);
    pyc_dut::Inputs in;
    in.insn = gfsim::wire<gfsim::Bits<8>>::known(gfsim::Bits<8>{row.insn});
    self.dut.drive(in);
    return true;
  }
  static void sample(void *opaque, std::uint64_t epoch) {
    auto &self = *static_cast<RunnerContext *>(opaque);
    require(epoch == self.sampled + 1 && self.sampled < knownFrames);
    const auto row = knownRow(self.sampled);
    const auto out = observe(self.dut.sample());
    require(out.op.isFullyKnown() && out.op.value().value() == row.op);
    require(out.len.isFullyKnown() && out.len.value().value() == row.length);
    std::cout << "WORK " << out.op.value().value() << ' '
              << out.len.value().value() << '\n';
    ++self.sampled;
  }
};
void fourStateAndRecovery(unsigned workers) {
  pyc_dut dut(workers);
  gfsim::SimExecutor executor(dut.system(), dut.observations(), {});
  constexpr std::string_view config = "{}";
  require(executor.ConfigureJson(
              reinterpret_cast<const std::uint8_t *>(config.data()),
              config.size()) == PYCIRCUIT_MODEL_STATUS_V1_OK);
  require(executor.Reset() == PYCIRCUIT_MODEL_STATUS_V1_OK);
  struct Masks {
    std::string_view insn, op, length;
  };
  // Lower-nibble X/Z is masked away. High-nibble uncertainty propagates
  // through each ordered last-match select; output Z never survives equality.
  constexpr Masks cases[] = {
      {"0001xxxx", "0001", "100"}, {"0001zzzz", "0001", "100"},
      {"0010xzxz", "0010", "100"}, {"0011zxzx", "0011", "100"},
      {"1111xxxx", "0000", "000"}, {"000x1010", "000x", "x00"},
      {"00x01111", "00x0", "x00"}, {"001x0000", "00xx", "x00"},
      {"00x10000", "00xx", "x00"}, {"00xx1010", "00xx", "x00"},
      {"0x010000", "000x", "x00"}, {"x0010000", "000x", "x00"},
      {"x100zzzz", "0000", "000"}, {"xxxxxxxx", "00xx", "x00"},
      {"zzzzzzzz", "00xx", "x00"}, {"001zzzzz", "00xx", "x00"},
  };
  auto step = [&](std::string_view bits) {
    pyc_dut::Inputs in;
    in.insn = wire<8>(bits);
    dut.drive(in);
    PycircuitModelStepResultV1 result{sizeof(result)};
    require(executor.Step(&result) == PYCIRCUIT_MODEL_STATUS_V1_OK &&
            result.state == PYCIRCUIT_MODEL_STEP_V1_RUNNING);
    return observe(dut.sample());
  };
  for (const auto &row : cases) {
    const auto out = step(row.insn);
    const auto op = check(out.op, row.op), length = check(out.len, row.length);
    std::cout << "MASK " << row.insn << ' ' << op << ' ' << length << '\n';
    // Consecutive pure Work recovers known op3/len4, without Reset.
    const auto recovered = step("00110000");
    check(recovered.op, "0011");
    check(recovered.len, "100");
  }
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
  require(result == 0 && context.sampled == knownFrames);
  fourStateAndRecovery(runner.workers());
  return result;
}
