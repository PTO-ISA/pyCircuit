#include "gfsim/SimExecutor.h"
#include "gfsim/SystemRunner.h"
#include "pycircuit_system.hpp"
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>

void require(bool ok) {
  if (!ok) {
    std::cerr << "cache_params fixed oracle failed\n";
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
  std::uint64_t addr;
  unsigned tag;
};
// Fixed configuration: 40 address bits, six offset and six index bits,
// 64-byte lines / eight-byte words. First row preserves the original TB.
constexpr Row rows[] = {
    {0ULL, 0u},
    {2048ULL, 0u},
    {4095ULL, 0u},
    {4096ULL, 1u},
    {4097ULL, 1u},
    {8191ULL, 1u},
    {8192ULL, 2u},
    {549755813888ULL, 134217728u},
    {1099511627775ULL, 268435455u},
    {78187493530ULL, 19088743u},
    {737894400291ULL, 180150000u},
    {178954240ULL, 43690u},
    {89477120ULL, 21845u},
    {0ULL, 0u},
};
auto knownAddress(std::uint64_t value) {
  return gfsim::wire<gfsim::Bits<40>>::known(gfsim::Bits<40>{value});
}
struct ObservedOutputs {
  gfsim::wire<gfsim::Bits<28>> tag;
  gfsim::wire<gfsim::Bits<9>> line_words, tag_bits;
};
ObservedOutputs observe(const pyc_dut::Outputs &out) {
  const auto &packed = out.result.packed();
  return {
      gfsim::wire<gfsim::Bits<28>>::fromPacked(gfsim::extract<28>(packed, 18)),
      gfsim::wire<gfsim::Bits<9>>::fromPacked(gfsim::extract<9>(packed, 9)),
      gfsim::wire<gfsim::Bits<9>>::fromPacked(gfsim::extract<9>(packed, 0))};
}
void constants(const ObservedOutputs &out) {
  require(out.line_words.isFullyKnown() && out.line_words.value().value() == 8);
  require(out.tag_bits.isFullyKnown() && out.tag_bits.value().value() == 28);
}
struct RunnerContext {
  pyc_dut &dut;
  unsigned sampled = 0;
  static void initialize(void *opaque) {
    require(static_cast<RunnerContext *>(opaque)->sampled == 0);
    require(drive(opaque, 0));
  }
  static bool drive(void *opaque, std::uint64_t epoch) {
    auto &self = *static_cast<RunnerContext *>(opaque);
    if (epoch == std::size(rows))
      return false;
    require(epoch < std::size(rows));
    pyc_dut::Inputs in;
    in.addr = knownAddress(rows[epoch].addr);
    self.dut.drive(in);
    return true;
  }
  static void sample(void *opaque, std::uint64_t epoch) {
    auto &self = *static_cast<RunnerContext *>(opaque);
    require(epoch == self.sampled + 1 && self.sampled < std::size(rows));
    const auto out = observe(self.dut.sample());
    constants(out);
    require(out.tag.isFullyKnown() &&
            out.tag.value().value() == rows[self.sampled].tag);
    std::cout << "WORK " << out.tag.value().value() << " 8 28\n";
    ++self.sampled;
  }
};
void unknownAndRecovery(unsigned workers) {
  pyc_dut dut(workers);
  gfsim::SimExecutor executor(dut.system(), dut.observations(), {});
  constexpr std::string_view config = "{}";
  require(executor.ConfigureJson(
              reinterpret_cast<const std::uint8_t *>(config.data()),
              config.size()) == PYCIRCUIT_MODEL_STATUS_V1_OK);
  require(executor.Reset() == PYCIRCUIT_MODEL_STATUS_V1_OK);
  struct Masks {
    std::string_view addr, tag;
  };
  constexpr Masks cases[] = {
      {"0000000000000000000000000101xxxxxxxxxxxx",
       "0000000000000000000000000101"},
      {"0000000000000000000000001001zzzzzzzzzzzz",
       "0000000000000000000000001001"},
      {"x000000000000000000000000000111111111111",
       "x000000000000000000000000000"},
      {"z000000000000000000000000000000000000000",
       "z000000000000000000000000000"},
      {"01xz000000000000000000000000xzxzxzxzxzxz",
       "01xz000000000000000000000000"},
      {"zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz",
       "zzzzzzzzzzzzzzzzzzzzzzzzzzzz"},
      {"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
       "xxxxxxxxxxxxxxxxxxxxxxxxxxxx"},
  };
  auto step = [&](auto addr) {
    pyc_dut::Inputs in;
    in.addr = addr;
    dut.drive(in);
    PycircuitModelStepResultV1 result{sizeof(result)};
    require(executor.Step(&result) == PYCIRCUIT_MODEL_STATUS_V1_OK);
    require(result.state == PYCIRCUIT_MODEL_STEP_V1_RUNNING);
    auto out = observe(dut.sample());
    constants(out);
    return out;
  };
  for (const auto &row : cases) {
    const auto out = step(wire<40>(row.addr));
    std::cout << "MASK " << check(out.tag, row.tag) << " 8 28\n";
    // Consecutive Work immediately recovers with known data, without Reset.
    const auto recovered = step(knownAddress(4096));
    require(recovered.tag.isFullyKnown() && recovered.tag.value().value() == 1);
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
  unknownAndRecovery(runner.workers());
  return result;
}
