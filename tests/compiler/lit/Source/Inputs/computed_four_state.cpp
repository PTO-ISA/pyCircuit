#include "gfsim/SimExecutor.h"
#include "pycircuit_system.hpp"
#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>

void require(bool ok) { if (!ok) std::abort(); }
struct Masks { std::uint64_t value,known,z; };
struct Row { Masks inputs[5]; std::string_view golden[7]; };
// Fixed four-state golden vectors, independently specified bit by bit.
const Row rows[] = {
  // X selector, equal known branches
  {{{1,8191,0},{1,8191,0},{68719476741,137438953471,0},{68719476741,137438953471,0},{0,0,0}},
   {"0000000000001","0000000000000000000000000000000000000","1111111111111","1000000000000000000000000000000000101","0","1","0000000000000"}},
  // Z selector, branches differ only at bit 36
  {{{2,8191,0},{1,8191,0},{0,137438953471,0},{68719476736,137438953471,0},{0,0,1}},
   {"0000000000000","1000000000000000000000000000000000000","1111111111110","x000000000000000000000000000000000000","0","1","0000000000000"}},
  // Narrow X/Z: AND zero dominates Z; OR one dominates Z
  {{{4,8188,2},{5,8191,0},{0,137438953471,0},{0,137438953471,0},{1,1,0}},
   {"000000000010x","0000000000000000000000000000000000000","111111111111x","0000000000000000000000000000000000000","x","1","0000000000000"}},
  // High Z at bit 36, X at bit 35: selected input preserves both
  {{{0,8191,0},{8,8188,1},{291,34359738367,68719476736},{0,137438953471,0},{0,1,0}},
   {"0000000000000","xx00000000000000000000000000100100011","11111111101xx","zx00000000000000000000000000100100011","x","1","0000000000000"}},
  // Z selector merges bit 1 disagreement while retaining bit 36
  {{{5461,8191,0},{2730,8191,0},{68719476741,137438953471,0},{68719476743,137438953471,0},{0,0,1}},
   {"0000000000000","0000000000000000000000000000000000010","1010101010101","10000000000000000000000000000000001x1","0","1","0000000000000"}},
  // Bitwise operations convert Z to X; controlling zero/one remain known
  {{{0,0,8191},{0,8191,0},{137438953471,137438953471,0},{291,68719476735,68719476736},{1,1,0}},
   {"0000000000000","x111111111111111111111111111011011100","1111111111111","x000000000000000000000000000100100011","x","1","0000000000000"}},
};

template <unsigned W> auto drive(Masks m) {
  return gfsim::wire<gfsim::Bits<W>>::fromPacked(gfsim::FourState<W>::fromMasks(
      gfsim::Bits<W>{m.value},gfsim::Bits<W>{m.known},gfsim::Bits<W>{m.z}));
}

template <unsigned W>
std::string check(const gfsim::wire<gfsim::Bits<W>> &wire,std::string_view golden) {
  require(golden.size()==W);
  std::uint64_t expectedValue=0,expectedKnown=0,expectedZ=0;
  for (unsigned i=0;i<W;++i) {
    const std::uint64_t mask=std::uint64_t{1}<<(W-i-1);
    const char bit=golden[i];
    require(bit=='0' || bit=='1' || bit=='x' || bit=='z');
    if(bit=='0' || bit=='1')expectedKnown|=mask;
    if(bit=='1')expectedValue|=mask;
    if(bit=='z')expectedZ|=mask;
  }
  const auto &state=wire.packed();
  require(state.knownMask()==gfsim::Bits<W>{expectedKnown});
  require(state.zMask()==gfsim::Bits<W>{expectedZ});
  // Payload bits under X/Z are unspecified; every known payload bit is checked.
  require((state.value() & state.knownMask())==gfsim::Bits<W>{expectedValue});
  std::string actual;
  for(unsigned i=0;i<W;++i) {
    const auto mask=std::uint64_t{1}<<(W-i-1);
    actual+=state.zMask().value() & mask ? 'z' :
            !(state.knownMask().value() & mask) ? 'x' :
            state.value().value() & mask ? '1' : '0';
  }
  return actual;
}

int main(int argc,char **argv) {
  require(argc==2);
  pyc_dut dut(std::strtoul(argv[1],nullptr,10));
  gfsim::SimExecutor executor(dut.system(),dut.observations(),{});
  constexpr std::string_view config="{}";
  require(executor.ConfigureJson(reinterpret_cast<const std::uint8_t *>(config.data()),
                                 config.size())==PYCIRCUIT_MODEL_STATUS_V1_OK);
  require(executor.Reset()==PYCIRCUIT_MODEL_STATUS_V1_OK);
  for(const auto &r:rows) {
    pyc_dut::Inputs in;
    in.a13=drive<13>(r.inputs[0]);in.b13=drive<13>(r.inputs[1]);
    in.a37=drive<37>(r.inputs[2]);in.b37=drive<37>(r.inputs[3]);
    in.choose=drive<1>(r.inputs[4]);dut.drive(in);
    PycircuitModelStepResultV1 step{sizeof(step)};
    require(executor.Step(&step)==PYCIRCUIT_MODEL_STATUS_V1_OK);
    const auto out=dut.sample();
    const std::array actual{check(out.masked13,r.golden[0]),check(out.xor37,r.golden[1]),
                            check(out.mixed13,r.golden[2]),check(out.pick37,r.golden[3]),
                            check(out.less,r.golden[4]),check(out.truth,r.golden[5]),
                            check(out.zero13,r.golden[6])};
    std::cout << "WORK";
    for(const auto &bits:actual)std::cout << ' ' << bits;
    std::cout << '\n';
  }
}
