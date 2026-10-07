#include "gfsim/SimExecutor.h"
#include "gfsim/SystemRunner.h"
#include "pycircuit_system.hpp"

// Observe the declared packed result without discarding X/Z planes. The
// historical stimulus, expected values and checks below remain unchanged.
struct ObservedOutputs {
  gfsim::wire<gfsim::Bits<8>> result;
};
ObservedOutputs observe(const pyc_dut::Outputs &output) {
  return {
      gfsim::wire<gfsim::Bits<8>>::fromPacked(
          gfsim::extract<8>(output.result.packed(), 0)),
  };
}
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>

void require(bool ok) {
  if (!ok) {
    std::cerr << "jit_control_flow fixed oracle failed\n";
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
  unsigned a, b, op, result;
};
// Historical1,2,add=>7 first. Independent goldens apply one +4 modulo256
// to the selected mathematical/bitwise value, not the source's four stages.
constexpr Row rows[] = {
    {1, 2, 0, 7},       {0, 0, 0, 4},     {0, 0, 1, 4},      {0, 0, 2, 4},
    {0, 0, 3, 4},       {1, 2, 0, 7},     {1, 2, 1, 3},      {1, 2, 2, 7},
    {1, 2, 3, 4},       {255, 1, 0, 4},   {255, 1, 1, 2},    {255, 1, 2, 2},
    {255, 1, 3, 5},     {0, 255, 0, 3},   {0, 255, 1, 5},    {0, 255, 2, 3},
    {0, 255, 3, 4},     {254, 255, 0, 1}, {254, 255, 1, 3},  {254, 255, 2, 5},
    {254, 255, 3, 2},   {128, 127, 0, 3}, {128, 127, 1, 5},  {128, 127, 2, 3},
    {128, 127, 3, 4},   {128, 128, 0, 4}, {128, 128, 1, 4},  {128, 128, 2, 4},
    {128, 128, 3, 132}, {255, 255, 0, 2}, {255, 255, 1, 4},  {255, 255, 2, 4},
    {255, 255, 3, 3},   {85, 170, 0, 3},  {85, 170, 1, 175}, {85, 170, 2, 3},
    {85, 170, 3, 4},    {170, 85, 0, 3},  {170, 85, 1, 89},  {170, 85, 2, 3},
    {170, 85, 3, 4},    {252, 4, 0, 4},   {252, 4, 1, 252},  {252, 4, 2, 252},
    {252, 4, 3, 8},     {253, 3, 0, 4},   {253, 3, 1, 254},  {253, 3, 2, 2},
    {253, 3, 3, 5},     {250, 9, 0, 7},   {250, 9, 1, 245},  {250, 9, 2, 247},
    {250, 9, 3, 12},    {7, 200, 0, 211}, {7, 200, 1, 67},   {7, 200, 2, 211},
    {7, 200, 3, 4},     {17, 31, 0, 52},  {17, 31, 1, 246},  {17, 31, 2, 18},
    {17, 31, 3, 21},    {254, 0, 0, 2},   {254, 0, 1, 2},    {254, 0, 2, 2},
    {254, 0, 3, 4},
};
struct RunnerContext {
  pyc_dut &dut;
  unsigned sampled = 0;
  static void initialize(void *opaque) { require(drive(opaque, 0)); }
  static bool drive(void *opaque, std::uint64_t epoch) {
    auto &self = *static_cast<RunnerContext *>(opaque);
    if (epoch == std::size(rows))
      return false;
    require(epoch < std::size(rows));
    const auto row = rows[epoch];
    pyc_dut::Inputs in;
    in.a = gfsim::wire<gfsim::Bits<8>>::known(gfsim::Bits<8>{row.a});
    in.b = gfsim::wire<gfsim::Bits<8>>::known(gfsim::Bits<8>{row.b});
    in.op = gfsim::wire<gfsim::Bits<2>>::known(gfsim::Bits<2>{row.op});
    self.dut.drive(in);
    return true;
  }
  static void sample(void *opaque, std::uint64_t epoch) {
    auto &self = *static_cast<RunnerContext *>(opaque);
    require(epoch == self.sampled + 1 && self.sampled < std::size(rows));
    const auto out = observe(self.dut.sample());
    require(out.result.isFullyKnown() &&
            out.result.value().value() == rows[self.sampled].result);
    std::cout << "WORK " << out.result.value().value() << '\n';
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
    std::string_view a, b, op, result;
  };
  // Exact nested selection order: op0 sum, else op1 difference, else op2 XOR,
  // else AND. Partial mux/bitwise X is spread by the first +1 arithmetic;
  // annihilated or equal-known branches stay known throughout all increments.
  constexpr Masks cases[] = {
      {"00000000", "00000000", "xx", "00000100"},
      {"00000000", "00000000", "zz", "00000100"},
      {"00000000", "00000000", "0z", "00000100"},
      {"00000000", "00000000", "z0", "00000100"},
      {"00000000", "00000000", "1z", "00000100"},
      {"xxxxxxxx", "00000000", "11", "00000100"},
      {"zzzzzzzz", "00000000", "11", "00000100"},
      {"z1111111", "01111111", "11", "10000011"},
      {"0000000x", "11111110", "11", "00000100"},
      {"xxxxxxxx", "00000000", "00", "xxxxxxxx"},
      {"zzzzzzzz", "11111111", "01", "xxxxxxxx"},
      {"0000000x", "00000000", "10", "xxxxxxxx"},
      {"0000000z", "11111111", "11", "xxxxxxxx"},
      {"10000000", "10000000", "0x", "xxxxxxxx"},
      {"00000000", "00001000", "x0", "xxxxxxxx"},
      {"00000001", "00000010", "xx", "xxxxxxxx"},
      {"00001111", "11110000", "1x", "xxxxxxxx"},
      {"xxxxxxxx", "00000000", "z1", "xxxxxxxx"},
  };
  auto step = [&](std::string_view a, std::string_view b, std::string_view op) {
    pyc_dut::Inputs in;
    in.a = wire<8>(a);
    in.b = wire<8>(b);
    in.op = wire<2>(op);
    dut.drive(in);
    PycircuitModelStepResultV1 result{sizeof(result)};
    require(executor.Step(&result) == PYCIRCUIT_MODEL_STATUS_V1_OK &&
            result.state == PYCIRCUIT_MODEL_STEP_V1_RUNNING);
    return observe(dut.sample());
  };
  for (const auto &row : cases) {
    const auto out = step(row.a, row.b, row.op);
    const auto actual = check(out.result, row.result);
    std::cout << "MASK " << row.a << ' ' << row.b << ' ' << row.op << ' '
              << actual << '\n';
    const auto recovered = step("00000001", "00000010", "00");
    check(recovered.result, "00000111");
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
  require(result == 0 && context.sampled == std::size(rows));
  fourStateAndRecovery(runner.workers());
  return result;
}
