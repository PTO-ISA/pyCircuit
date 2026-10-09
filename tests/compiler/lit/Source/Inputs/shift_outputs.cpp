#include "gfsim/SimExecutor.h"
#include "pycircuit_system.hpp"
#include "shift_vectors.h"
#include <array>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>

void require(bool ok) {
  if (!ok)
    std::abort();
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
int main(int argc, char **argv) {
  require(argc == 2);
  pyc_dut dut(std::strtoul(argv[1], nullptr, 10));
  gfsim::SimExecutor executor(dut.system(), dut.observations(), {});
  constexpr std::string_view config = "{}";
  require(executor.ConfigureJson(
              reinterpret_cast<const std::uint8_t *>(config.data()),
              config.size()) == PYCIRCUIT_MODEL_STATUS_V1_OK);
  require(executor.Reset() == PYCIRCUIT_MODEL_STATUS_V1_OK);
  for (const auto &r : shiftRows) {
    pyc_dut::Inputs in;
    in.a = wire<8>(r.inputs[0]);
    in.mid = wire<13>(r.inputs[1]);
    in.addr = wire<40>(r.inputs[2]);
    in.wide = wire<65>(r.inputs[3]);
    in.one = wire<1>(r.inputs[4]);
    dut.drive(in);
    PycircuitModelStepResultV1 step{sizeof(step)};
    require(executor.Step(&step) == PYCIRCUIT_MODEL_STATUS_V1_OK);
    const auto out = dut.sample();
    const std::array actual{check(out.identity, r.golden[0]),
                            check(out.original_xor, r.golden[1]),
                            check(out.original_or, r.golden[2]),
                            check(out.original_equal, r.golden[3]),
                            check(out.mid_zero, r.golden[4]),
                            check(out.mid_one, r.golden[5]),
                            check(out.mid_last, r.golden[6]),
                            check(out.mid_computed, r.golden[7]),
                            check(out.mid_at, r.golden[8]),
                            check(out.mid_above, r.golden[9]),
                            check(out.mid_huge, r.golden[10]),
                            check(out.tag, r.golden[11]),
                            check(out.wide_zero, r.golden[12]),
                            check(out.wide_one, r.golden[13]),
                            check(out.wide_last, r.golden[14]),
                            check(out.wide_at, r.golden[15]),
                            check(out.wide_above, r.golden[16]),
                            check(out.wide_huge, r.golden[17]),
                            check(out.one_zero, r.golden[18]),
                            check(out.one_at, r.golden[19]),
                            check(out.add_before, r.golden[20]),
                            check(out.masked, r.golden[21]),
                            check(out.singleton, r.golden[22]),
                            check(out.static_signed, r.golden[23]),
                            check(out.bound, r.golden[24])};
    std::cout << "WORK";
    for (const auto &v : actual)
      std::cout << ' ' << v;
    std::cout << '\n';
  }
}
