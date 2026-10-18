// Independent source-bit oracle for fixed/fixed dynamic shifts.
#include "gfsim/SimExecutor.h"
#include "pycircuit_system.hpp"
#include <cstdlib>
#include <iostream>
#include <source_location>
#include <string>
#include <string_view>

void require(bool ok,
             std::source_location at = std::source_location::current()) {
  if (!ok) {
    std::cerr << "dynamic fixed shift oracle failed at " << at.line() << '\n';
    std::abort();
  }
}

char symbol(unsigned bit, unsigned width, unsigned row, bool mask) {
  if (mask) {
    if (row == 1)
      return 'x';
    if (row == 2)
      return 'z';
    if (bit == 0 || bit == width - 1 || bit == 64 || bit == 129)
      return 'x';
    if (bit == 1 || bit == 63 || bit == 65 || bit == 128)
      return 'z';
  }
  return (bit * 17 + row * 13) % 5 < 2 ? '1' : '0';
}

template <unsigned W> auto pattern(unsigned row, bool mask = false) {
  gfsim::Bits<W> value{0}, known{0}, z{0};
  for (unsigned bit = 0; bit != W; ++bit) {
    const auto one = std::uint64_t{1} << (bit % 64);
    const auto word = bit / 64;
    const char state = symbol(bit, W, row, mask);
    if (state == '1' || ((state == 'x' || state == 'z') && bit % 3 == 0))
      value.setWord(word, value.word(word) | one);
    if (state == '0' || state == '1')
      known.setWord(word, known.word(word) | one);
    if (state == 'z')
      z.setWord(word, z.word(word) | one);
  }
  return gfsim::wire<gfsim::Bits<W>>::fromPacked(
      gfsim::FourState<W>::fromMasks(value, known, z));
}

template <unsigned W> auto knownCount(unsigned row, unsigned width) {
  gfsim::Bits<W> value{0};
  if (row == 5) {
    if constexpr (W > 64)
      value.setWord((W - 1) / 64, std::uint64_t{1} << ((W - 1) % 64));
    else
      value.setWord(0, (std::uint64_t{1} << W) - 1);
  } else {
    const unsigned amount[] = {0, 1, width - 1, width, width + 1, 0};
    value.setWord(0, amount[row]);
  }
  return gfsim::wire<gfsim::Bits<W>>::known(value);
}

template <unsigned W> auto unknownCount(unsigned position, bool zBit) {
  gfsim::Bits<W> value{0}, known;
  for (unsigned word = 0; word != (W + 63) / 64; ++word)
    known.setWord(word, ~std::uint64_t{0});
  if constexpr (W % 64)
    known.setWord((W - 1) / 64,
                  known.word((W - 1) / 64) &
                      ((std::uint64_t{1} << (W % 64)) - 1));
  known.setWord(position / 64,
                known.word(position / 64) &
                    ~(std::uint64_t{1} << (position % 64)));
  gfsim::Bits<W> z{0};
  if (zBit)
    z.setWord(position / 64, std::uint64_t{1} << (position % 64));
  return gfsim::wire<gfsim::Bits<W>>::fromPacked(
      gfsim::FourState<W>::fromMasks(value, known, z));
}

template <unsigned Out, unsigned In, class Output>
void shifted(const Output &output, unsigned low,
             const gfsim::wire<gfsim::Bits<In>> &input, unsigned count,
             bool right) {
  const auto actual = gfsim::extract<Out>(output.packed(), low);
  for (unsigned bit = 0; bit != Out; ++bit) {
    const int source = right ? static_cast<int>(bit + count)
                             : static_cast<int>(bit) - static_cast<int>(count);
    const bool retained = source >= 0 && source < static_cast<int>(In);
    require(actual.value().bit(bit) ==
            (retained && input.packed().value().bit(source)));
    require(actual.knownMask().bit(bit) ==
            (!retained || input.packed().knownMask().bit(source)));
    require(actual.zMask().bit(bit) ==
            (retained && input.packed().zMask().bit(source)));
  }
}

template <unsigned W, class Output>
void pair(const Output &output, unsigned leftLow, unsigned rightLow,
          const gfsim::wire<gfsim::Bits<W>> &input, unsigned count) {
  shifted<W>(output, leftLow, input, count, false);
  shifted<W>(output, rightLow, input, count, true);
}

template <unsigned W>
unsigned boundedCount(unsigned row) {
  const unsigned amount[] = {0, 1, W - 1, W, W + 1, W};
  return amount[row];
}

template <class Output> void print(const Output &output, const char *prefix) {
  std::cout << prefix << ' ';
  const auto &packed = output.packed();
  for (unsigned bit = 429; bit != 0; --bit)
    std::cout << (packed.zMask().bit(bit - 1)       ? 'z'
                  : !packed.knownMask().bit(bit - 1) ? 'x'
                  : packed.value().bit(bit - 1)      ? '1'
                                                      : '0');
  std::cout << '\n';
}


template <unsigned W, class Output>
void entirelyUnknown(const Output &output, unsigned low) {
  const auto actual = gfsim::extract<W>(output.packed(), low);
  for (unsigned bit = 0; bit != W; ++bit) {
    require(!actual.knownMask().bit(bit));
    require(!actual.zMask().bit(bit));
  }
}

template <class Inputs> void driveKnownDomain(pyc_dut &dut, Inputs input) {
  if constexpr (requires(Inputs value) { value.pyc_7079635f636c6b; }) {
    input.pyc_7079635f636c6b =
        gfsim::wire<gfsim::Bits<1>>::known(gfsim::Bits<1>{0});
    input.pyc_7079635f727374 =
        gfsim::wire<gfsim::Bits<1>>::known(gfsim::Bits<1>{0});
  }
  dut.drive(input);
}

int main(int argc, char **argv) {
  require(argc == 2);
  pyc_dut dut(static_cast<unsigned>(std::stoul(argv[1])));
  gfsim::SimExecutor executor(dut.system(), dut.observations(), {});
  constexpr std::string_view config =
      R"({"deadlock_window":null,"max_domain_cycles":{},"max_ticks":64,"schema":"pycircuit-model-config","version":"1"})";
  require(executor.ConfigureJson(
              reinterpret_cast<const std::uint8_t *>(config.data()),
              config.size()) == PYCIRCUIT_MODEL_STATUS_V1_OK);
  require(executor.Reset() == PYCIRCUIT_MODEL_STATUS_V1_OK);
  auto sample = [&](unsigned row, bool mask, bool emit = true) {
    pyc_dut::Inputs input;
    input.v1 = pattern<1>(row, mask);
    input.v5 = pattern<5>(row, mask);
    input.v65 = pattern<65>(row, mask);
    input.v130 = pattern<130>(row, mask);
    input.count_wide = knownCount<5>(row, 1);
    input.count_narrow = knownCount<3>(row, 5);
    input.count65 = knownCount<65>(row, 65);
    input.count130 = knownCount<130>(row, 130);
    input.choose = gfsim::wire<gfsim::Bits<1>>::known(
        gfsim::Bits<1>{static_cast<std::uint64_t>(row & 1)});
    driveKnownDomain(dut, input);
    PycircuitModelStepResultV1 status{sizeof(status)};
    require(executor.Step(&status) == PYCIRCUIT_MODEL_STATUS_V1_OK);
    require(status.state == PYCIRCUIT_MODEL_STEP_V1_RUNNING);
    const auto output = dut.sample().result;
    pair<1>(output, 413, 412, input.v1, boundedCount<1>(row));
    pair<5>(output, 407, 402, input.v5, boundedCount<5>(row));
    pair<65>(output, 337, 272, input.v65, boundedCount<65>(row));
    pair<130>(output, 142, 12, input.v130, boundedCount<130>(row));
    if (!mask) {
      const auto v5 = input.v5.value().value();
      const auto literal = row & 1 ? 1u : 3u;
      require(gfsim::extract<5>(output.packed(), 424).value().value() ==
              ((v5 << 3) & 31));
      unsigned quotient = v5;
      for (unsigned iteration = 0; iteration != 3; ++iteration)
        quotient /= 3;
      require(gfsim::extract<5>(output.packed(), 419).value().value() ==
              quotient);
      unsigned remainder = v5;
      for (unsigned iteration = 0; iteration != 3; ++iteration)
        remainder %= 5;
      require(gfsim::extract<5>(output.packed(), 414).value().value() ==
              remainder);
      require(gfsim::extract<5>(output.packed(), 7).value().value() ==
              (v5 & literal));
      require(gfsim::extract<5>(output.packed(), 2).value().value() ==
              (literal & v5));
      require(output.packed().value().bit(1) == (2 < v5));
      require(output.packed().value().bit(0) == (v5 > 2));
      require(output.isFullyKnown());
    } else {
      shifted<5>(output, 424, input.v5, 3, false);
      entirelyUnknown<5>(output, 419);
      entirelyUnknown<5>(output, 414);
    }
    if (emit)
      print(output, mask ? "DMASK" : "DYNAMIC");
  };
  auto unknown = [&](bool high, bool zBit) {
    pyc_dut::Inputs input;
    input.v1 = pattern<1>(3);
    input.v5 = pattern<5>(3);
    input.v65 = pattern<65>(3);
    input.v130 = pattern<130>(3);
    input.count_wide = unknownCount<5>(high ? 4 : 0, zBit);
    input.count_narrow = unknownCount<3>(high ? 2 : 0, zBit);
    input.count65 = unknownCount<65>(high ? 64 : 0, zBit);
    input.count130 = unknownCount<130>(high ? 129 : 0, zBit);
    input.choose =
        gfsim::wire<gfsim::Bits<1>>::known(gfsim::Bits<1>{0});
    driveKnownDomain(dut, input);
    PycircuitModelStepResultV1 status{sizeof(status)};
    require(executor.Step(&status) == PYCIRCUIT_MODEL_STATUS_V1_OK);
    const auto output = dut.sample().result;
    entirelyUnknown<1>(output, 413);
    entirelyUnknown<1>(output, 412);
    entirelyUnknown<5>(output, 407);
    entirelyUnknown<5>(output, 402);
    entirelyUnknown<65>(output, 337);
    entirelyUnknown<65>(output, 272);
    entirelyUnknown<130>(output, 142);
    entirelyUnknown<130>(output, 12);
    print(output, high ? (zBit ? "COUNT HZ" : "COUNT HX")
                       : (zBit ? "COUNT LZ" : "COUNT LX"));
    sample(3, false, false);
  };
  for (unsigned row = 0; row != 6; ++row)
    sample(row, false);
  for (unsigned row = 0; row != 4; ++row)
    sample(row, true);
  for (bool high : {false, true})
    for (bool zBit : {false, true})
      unknown(high, zBit);
}
