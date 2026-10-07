// Independent fixed-position and four-state oracles for generated source slices.
#include "gfsim/SimExecutor.h"
#include "pycircuit_system.hpp"
#include <array>
#include <cstdlib>
#include <iostream>
#include <source_location>
#include <string>
#include <string_view>
void require(bool ok, std::source_location at = std::source_location::current()) {
  if (!ok) { std::cerr << "fixed slices oracle failed at " << at.line() << '\n'; std::abort(); }
}
template<unsigned W> auto known(std::uint64_t value) {
  return gfsim::wire<gfsim::Bits<W>>::known(gfsim::Bits<W>{value});
}
template<unsigned W> auto fromText(std::string_view bits) {
  require(bits.size() == W);
  gfsim::Bits<W> value{0}, mask{0}, z{0};
  for (unsigned index = 0; index != W; ++index) {
    const unsigned bit = W - index - 1, word = bit / 64;
    const auto bitMask = std::uint64_t{1} << (bit % 64);
    if (bits[index] != '0') value.setWord(word, value.word(word) | bitMask);
    if (bits[index] == '0' || bits[index] == '1') mask.setWord(word, mask.word(word) | bitMask);
    if (bits[index] == 'z') z.setWord(word, z.word(word) | bitMask);
  }
  return gfsim::wire<gfsim::Bits<W>>::fromPacked(gfsim::FourState<W>::fromMasks(value, mask, z));
}
template<unsigned W, typename Packet> auto field(const Packet &packet, unsigned low) {
  return gfsim::wire<gfsim::Bits<W>>::fromPacked(gfsim::extract<W>(packet.packed(), low));
}
template<unsigned Out, unsigned In, typename Packet>
void slice(const Packet &packet, unsigned fieldLow,
           const gfsim::wire<gfsim::Bits<In>> &input, unsigned inputLow) {
  const auto actual = field<Out>(packet, fieldLow).packed();
  const auto &expected = input.packed();
  for (unsigned bit = 0; bit != Out; ++bit) {
    require(actual.value().bit(bit) == expected.value().bit(inputLow + bit));
    require(actual.knownMask().bit(bit) == expected.knownMask().bit(inputLow + bit));
    require(actual.zMask().bit(bit) == expected.zMask().bit(inputLow + bit));
  }
}
template<unsigned W, typename Packet> std::string text(const Packet &packet) {
  const auto &value = packet.packed();
  std::string result;
  for (unsigned index = 0; index != W; ++index) {
    const unsigned bit = W - index - 1;
    result += value.zMask().bit(bit) ? 'z' : !value.knownMask().bit(bit) ? 'x'
                                         : value.value().bit(bit) ? '1' : '0';
  }
  return result;
}
int main(int argc, char **argv) {
  require(argc == 2);
  pyc_dut dut(static_cast<unsigned>(std::stoul(argv[1])));
  gfsim::SimExecutor executor(dut.system(), dut.observations(), {});
  constexpr std::string_view config = "{}";
  require(executor.ConfigureJson(reinterpret_cast<const std::uint8_t *>(config.data()),
                                config.size()) == PYCIRCUIT_MODEL_STATUS_V1_OK);
  require(executor.Reset() == PYCIRCUIT_MODEL_STATUS_V1_OK);
#ifdef TABLE_SLICES
  auto cells = std::array{known<13>(0), known<13>(0)};
  bool lastClock = false;
  auto row = [&](unsigned clock, unsigned reset, unsigned index,
                 gfsim::wire<gfsim::Bits<13>> value, unsigned write, const char *prefix) {
    pyc_dut::Inputs inputs;
    inputs.pyc_7079635f636c6b = known<1>(clock);
    inputs.pyc_7079635f727374 = known<1>(reset);
    inputs.index = known<1>(index); inputs.value = value; inputs.write = known<1>(write);
    dut.drive(inputs);
    PycircuitModelStepResultV1 status{sizeof(status)};
    require(executor.Step(&status) == PYCIRCUIT_MODEL_STATUS_V1_OK);
    require(status.state == PYCIRCUIT_MODEL_STEP_V1_RUNNING);
    const auto output = dut.sample().result;
    slice<13>(output, 7, cells[index], 0);
    slice<7>(output, 0, cells[index], 2);
    if (prefix) std::cout << prefix << ' ' << text<20>(output) << '\n';
    if (clock && !lastClock) {
      if (reset) cells = {known<13>(0), known<13>(0)};
      else if (write) cells[index] = value;
    }
    lastClock = clock;
  };
  struct Row { unsigned clock, reset, index, value, write; };
  constexpr Row rows[] = {{0,0,0,1,1},{1,0,0,8191,1},{1,0,0,2,1},{0,0,0,3,0},
                         {1,0,1,4096,1},{0,0,1,0,0},{1,0,0,0,0},{0,0,1,2047,1},
                         {1,0,1,31,1},{0,0,1,0,0},{0,1,0,7,1},{1,1,0,7,1},
                         {1,0,1,9,1},{0,0,1,9,1},{1,0,1,9,1},{0,0,1,0,0}};
  for (const auto &input : rows)
    row(input.clock,input.reset,input.index,known<13>(input.value),input.write,"WORK");
  for (std::string_view value : {"1x0z0101zx0z1", "xxxxxxxxxxxxx", "zzzzzzzzzzzzz", "0z101x0101z01"}) {
    row(0,0,1,fromText<13>(value),1,nullptr);
    row(1,0,1,fromText<13>(value),1,nullptr);
    row(0,0,1,known<13>(0),0,"MASK");
    row(1,1,1,known<13>(0),0,nullptr);
    row(0,0,1,known<13>(0),0,nullptr);
  }
#else
  auto row = [&](const pyc_dut::Inputs &inputs, const char *prefix) {
    dut.drive(inputs);
    PycircuitModelStepResultV1 status{sizeof(status)};
    require(executor.Step(&status) == PYCIRCUIT_MODEL_STATUS_V1_OK);
    require(status.state == PYCIRCUIT_MODEL_STEP_V1_RUNNING);
    const auto output = dut.sample().result;
    slice<1>(output,248,inputs.tiny,0); slice<5>(output,243,inputs.n13,0);
    slice<7>(output,236,inputs.n13,3); slice<5>(output,231,inputs.n13,8);
    slice<13>(output,218,inputs.n13,0); slice<4>(output,214,inputs.n13,5);
    slice<28>(output,186,inputs.address,12); slice<9>(output,177,inputs.wide,0);
    slice<17>(output,160,inputs.wide,55); slice<9>(output,151,inputs.wide,64);
    slice<73>(output,78,inputs.wide,0); slice<6>(output,72,inputs.n13,1);
    const auto arithmetic = field<13>(output,59);
    if (inputs.n13.isFullyKnown()) {
      require(arithmetic.isFullyKnown());
      require(arithmetic.value().value() == ((inputs.n13.value().value() + 1u) % 8192));
    } else {
      require(arithmetic.packed().knownMask() == gfsim::Bits<13>{0});
      require(arithmetic.packed().zMask() == gfsim::Bits<13>{0});
    }
    slice<6>(output,53,arithmetic,0);
    const auto selected = field<40>(output,13).packed();
    for (unsigned bit = 0; bit != 40; ++bit) {
      auto state = [&](const auto &packed) {
        return packed.zMask().bit(bit) ? 3 : !packed.knownMask().bit(bit) ? 2
                                            : packed.value().bit(bit) ? 1 : 0;
      };
      const int a = state(inputs.address.packed()), b = state(inputs.wide.packed());
      const int expected = inputs.choose.isFullyKnown()
                             ? (inputs.choose.value().value() ? a : b)
                             : a == b ? a : 2;
      require(state(selected) == expected);
    }
    slice<13>(output,0,field<40>(output,13),4);
    std::cout << prefix << ' ' << text<249>(output) << '\n';
  };
  constexpr unsigned n13[] = {0,1,8191,4096,2730,5461,254,6001,8190,17};
  constexpr std::uint64_t addresses[] = {0,1,1099511627775ULL,549755813888ULL,
                                       366503875925ULL,733007751850ULL,4095,4096,1099511627774ULL,17};
  for (unsigned index = 0; index != 10; ++index) {
    pyc_dut::Inputs inputs;
    inputs.tiny = known<1>(index % 2); inputs.n13 = known<13>(n13[index]);
    inputs.address = known<40>(addresses[index]); inputs.choose = known<1>(index % 2);
    gfsim::Bits<73> wide{addresses[9-index] * 65537ULL};
    wide.setWord(1,(index * 57u) % 512u);
    inputs.wide = gfsim::wire<gfsim::Bits<73>>::known(wide);
    row(inputs,"WORK");
  }
  struct Unknown { std::string_view tiny, n13, address, wide, choose; };
  const std::array<Unknown,4> cases{{
    {"z","1x0z0101zx0z1","1x0z1x0z1x0z1x0z1x0z1x0z1x0z1x0z1x0z1x0z","1x0z1x0z1x0z1x0z1x0z1x0z1x0z1x0z1x0z1x0z1x0z1x0z1x0z1x0z1x0z1x0z1x0z1x0z1","1"},
    {"x","zzzzzzzzzzzzz","zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz","zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz","0"},
    {"1","0000000000000","0000000000000000000000000000000000000000","1111111111111111111111111111111111111111111111111111111111111111111111111","x"},
    {"0","0000000000000","0000000000000000000000000000000000000000","zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz0000000000000000000000000000000000000000","z"}
  }};
  for (const auto &input : cases) {
    pyc_dut::Inputs inputs;
    inputs.tiny=fromText<1>(input.tiny); inputs.n13=fromText<13>(input.n13);
    inputs.address=fromText<40>(input.address); inputs.wide=fromText<73>(input.wide);
    inputs.choose=fromText<1>(input.choose); row(inputs,"MASK");
  }
#endif
}
