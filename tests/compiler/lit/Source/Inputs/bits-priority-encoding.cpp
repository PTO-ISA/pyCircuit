// Independent scalar conditional-fold oracle; no DUT scan/mask algorithm.
#include "gfsim/SimExecutor.h"
#include "pycircuit_system.hpp"
#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <source_location>
#include <string>
#include <string_view>
#include <vector>
namespace {
unsigned frame = 0;
void require(bool ok, std::source_location at = std::source_location::current()) {
  if (!ok) { std::cerr << "priority oracle line " << at.line() << ", frame " << frame << '\n'; std::abort(); }
}
enum class Pattern { Scalar, Zero, Ones, OneHot, Mixed, FourSmall,
                     ZeroMasked, OnesMasked, UpperOne, LowerOne, Dense, Literal };
constexpr std::array<std::string_view,7> tableInputs = {"0000","0100","1010","x100","0x01","000x","1xxx"};
constexpr std::array<std::string_view,7> tableLow = {"00","10","01","10","00","00","xx"};
constexpr std::array<std::string_view,7> tableHigh = {"00","10","11","1x","x0","00","11"};
constexpr std::string_view tableValid = "01111x1", tableConflict = "001xxxx";
char inputSymbol(unsigned width, unsigned bit, Pattern pattern, unsigned row, bool highZ) {
  char unknown = highZ ? 'z' : 'x';
  switch (pattern) {
  case Pattern::Scalar: return bit < 8 && ((row >> bit) & 1) ? '1' : '0';
  case Pattern::Zero: return '0';
  case Pattern::Ones: return '1';
  case Pattern::OneHot: return bit == row ? '1' : '0';
  case Pattern::Mixed: return ((bit * 7 + row * 3) % 11) < 5 ? '1' : '0';
  case Pattern::FourSmall: return width <= 4 ? "01xz"[(row >> (2 * bit)) & 3] : '0';
  case Pattern::ZeroMasked: return bit == row ? unknown : '0';
  case Pattern::OnesMasked: return bit == row ? unknown : '1';
  case Pattern::UpperOne: return bit == row ? unknown : bit == width - 1 ? '1' : '0';
  case Pattern::LowerOne: return bit == row ? unknown : bit == 0 ? '1' : '0';
  case Pattern::Dense: return "01xz"[(bit + row) % 4];
  case Pattern::Literal:
    if (width != 4) return '0';
    require(row < 7);
    return tableInputs[row][3-bit] == 'x' ? unknown : tableInputs[row][3-bit];
  }
  std::abort();
}
template<unsigned W, class Port>
void drivePort(Port &port, Pattern pattern, unsigned row, bool highZ, bool latent) {
  gfsim::Bits<W> value{0}, known{0}, z{0};
  for (unsigned bit=0; bit<W; ++bit) {
    char symbol = inputSymbol(W,bit,pattern,row,highZ);
    bool isKnown = symbol=='0' || symbol=='1';
    auto mask = std::uint64_t{1} << (bit%64); auto word = bit/64;
    if (isKnown ? symbol=='1' : latent) value.setWord(word,value.word(word)|mask);
    if (isKnown) known.setWord(word,known.word(word)|mask);
    if (symbol=='z') z.setWord(word,z.word(word)|mask);
  }
  port = Port::fromPacked(gfsim::FourState<W>::fromMasks(value,known,z));
}
pyc_dut::Inputs inputs(Pattern pattern,unsigned row,bool highZ,bool latent) {
  pyc_dut::Inputs ports;
  drivePort<1>(ports.w1,pattern,row,highZ,latent);
  drivePort<2>(ports.w2,pattern,row,highZ,latent);
  drivePort<3>(ports.w3,pattern,row,highZ,latent);
  drivePort<4>(ports.w4,pattern,row,highZ,latent);
  drivePort<5>(ports.w5,pattern,row,highZ,latent);
  drivePort<8>(ports.w8,pattern,row,highZ,latent);
  drivePort<9>(ports.w9,pattern,row,highZ,latent);
  drivePort<13>(ports.w13,pattern,row,highZ,latent);
  drivePort<31>(ports.w31,pattern,row,highZ,latent);
  drivePort<32>(ports.w32,pattern,row,highZ,latent);
  drivePort<33>(ports.w33,pattern,row,highZ,latent);
  drivePort<63>(ports.w63,pattern,row,highZ,latent);
  drivePort<64>(ports.w64,pattern,row,highZ,latent);
  drivePort<65>(ports.w65,pattern,row,highZ,latent);
  drivePort<73>(ports.w73,pattern,row,highZ,latent);
  drivePort<127>(ports.w127,pattern,row,highZ,latent);
  drivePort<128>(ports.w128,pattern,row,highZ,latent);
  drivePort<129>(ports.w129,pattern,row,highZ,latent);
  return ports;
}
using Symbols = std::vector<char>;
using Expected = std::array<char,510>;
unsigned indexWidth(unsigned value) {
  unsigned result=0; do { ++result; value >>= 1; } while(value); return result;
}
Symbols number(unsigned value,unsigned width) {
  Symbols result(width,'0');
  for(unsigned bit=0;bit<width;++bit) result[bit]=(value>>bit)&1?'1':'0';
  return result;
}
template<unsigned W,class Port>
Symbols source(const Port &port,unsigned start=0,unsigned length=W) {
  Symbols result(length); const auto &raw=port.packed();
  for(unsigned bit=0;bit<length;++bit)
    result[bit]=raw.zMask().bit(start+bit)?'z':!raw.knownMask().bit(start+bit)?'x':raw.value().bit(start+bit)?'1':'0';
  return result;
}
struct Selection { Symbols index; char valid; char conflict; };
Selection encode(const Symbols &input,bool high) {
  unsigned width=input.size(), natural=indexWidth(width-1), population=0;
  Symbols index(natural,'0'); bool uncertain=false, anyOne=false;
  // Visit lowest-priority position first. Unknown ternary merges equal bits
  // with the selected integer constant, preserving each common known bit.
  for(unsigned visit=0;visit<width;++visit) {
    unsigned position=high?visit:width-1-visit; char predicate=input[position];
    for(unsigned bit=0;bit<natural;++bit) {
      char choice=(position>>bit)&1?'1':'0';
      if(predicate=='1') index[bit]=choice;
      else if(predicate!='0' && index[bit]!=choice) index[bit]='x';
    }
    if(predicate=='1') {anyOne=true;++population;}
    if(predicate!='0' && predicate!='1') uncertain=true;
  }
  return {index,anyOne?'1':uncertain?'x':'0',uncertain?'x':population>1?'1':'0'};
}
void put(Expected &result,unsigned low,unsigned width,const Symbols &symbols) {
  require(symbols.size()<=width);
  for(unsigned bit=0;bit<width;++bit) result[low+bit]=bit<symbols.size()?symbols[bit]:'0';
}
void putFlag(Expected &result,unsigned low,char flag) { result[low]=flag; }
Symbols increment(const Symbols &input) {
  unsigned value=0;
  for(unsigned bit=0;bit<input.size();++bit) {
    if(input[bit]!='0' && input[bit]!='1') return Symbols(input.size(),'x');
    if(input[bit]=='1') value|=1u<<bit;
  }
  return number((value+1)%32,5);
}
Expected golden(const pyc_dut::Inputs &ports) {
  Expected result{}; result.fill('0'); Selection selected;
  selected=encode(source<1>(ports.w1),false);
  put(result,509,1,selected.index); putFlag(result,508,selected.valid);
  selected=encode(source<1>(ports.w1),true);
  put(result,507,1,selected.index); putFlag(result,506,selected.valid);
  selected=encode(source<1>(ports.w1),false);
  put(result,505,1,selected.index); putFlag(result,504,selected.valid);
  putFlag(result,503,selected.conflict);
  selected=encode(source<1>(ports.w1),true);
  put(result,502,1,selected.index); putFlag(result,501,selected.valid);
  putFlag(result,500,selected.conflict);
  selected=encode(source<2>(ports.w2),false);
  put(result,499,1,selected.index); putFlag(result,498,selected.valid);
  selected=encode(source<2>(ports.w2),true);
  put(result,497,1,selected.index); putFlag(result,496,selected.valid);
  selected=encode(source<2>(ports.w2),false);
  put(result,495,1,selected.index); putFlag(result,494,selected.valid);
  putFlag(result,493,selected.conflict);
  selected=encode(source<2>(ports.w2),true);
  put(result,492,1,selected.index); putFlag(result,491,selected.valid);
  putFlag(result,490,selected.conflict);
  selected=encode(source<3>(ports.w3),false);
  put(result,488,2,selected.index); putFlag(result,487,selected.valid);
  selected=encode(source<3>(ports.w3),true);
  put(result,485,2,selected.index); putFlag(result,484,selected.valid);
  selected=encode(source<3>(ports.w3),false);
  put(result,482,2,selected.index); putFlag(result,481,selected.valid);
  putFlag(result,480,selected.conflict);
  selected=encode(source<3>(ports.w3),true);
  put(result,478,2,selected.index); putFlag(result,477,selected.valid);
  putFlag(result,476,selected.conflict);
  selected=encode(source<4>(ports.w4),false);
  put(result,474,2,selected.index); putFlag(result,473,selected.valid);
  selected=encode(source<4>(ports.w4),true);
  put(result,471,2,selected.index); putFlag(result,470,selected.valid);
  selected=encode(source<4>(ports.w4),false);
  put(result,468,2,selected.index); putFlag(result,467,selected.valid);
  putFlag(result,466,selected.conflict);
  selected=encode(source<4>(ports.w4),true);
  put(result,464,2,selected.index); putFlag(result,463,selected.valid);
  putFlag(result,462,selected.conflict);
  selected=encode(source<5>(ports.w5),false);
  put(result,459,3,selected.index); putFlag(result,458,selected.valid);
  selected=encode(source<5>(ports.w5),true);
  put(result,455,3,selected.index); putFlag(result,454,selected.valid);
  selected=encode(source<5>(ports.w5),false);
  put(result,451,3,selected.index); putFlag(result,450,selected.valid);
  putFlag(result,449,selected.conflict);
  selected=encode(source<5>(ports.w5),true);
  put(result,446,3,selected.index); putFlag(result,445,selected.valid);
  putFlag(result,444,selected.conflict);
  selected=encode(source<8>(ports.w8),false);
  put(result,441,3,selected.index); putFlag(result,440,selected.valid);
  selected=encode(source<8>(ports.w8),true);
  put(result,437,3,selected.index); putFlag(result,436,selected.valid);
  selected=encode(source<8>(ports.w8),false);
  put(result,433,3,selected.index); putFlag(result,432,selected.valid);
  putFlag(result,431,selected.conflict);
  selected=encode(source<8>(ports.w8),true);
  put(result,428,3,selected.index); putFlag(result,427,selected.valid);
  putFlag(result,426,selected.conflict);
  selected=encode(source<9>(ports.w9),false);
  put(result,422,4,selected.index); putFlag(result,421,selected.valid);
  selected=encode(source<9>(ports.w9),true);
  put(result,417,4,selected.index); putFlag(result,416,selected.valid);
  selected=encode(source<9>(ports.w9),false);
  put(result,412,4,selected.index); putFlag(result,411,selected.valid);
  putFlag(result,410,selected.conflict);
  selected=encode(source<9>(ports.w9),true);
  put(result,406,4,selected.index); putFlag(result,405,selected.valid);
  putFlag(result,404,selected.conflict);
  selected=encode(source<13>(ports.w13),false);
  put(result,400,4,selected.index); putFlag(result,399,selected.valid);
  selected=encode(source<13>(ports.w13),true);
  put(result,395,4,selected.index); putFlag(result,394,selected.valid);
  selected=encode(source<13>(ports.w13),false);
  put(result,390,4,selected.index); putFlag(result,389,selected.valid);
  putFlag(result,388,selected.conflict);
  selected=encode(source<13>(ports.w13),true);
  put(result,384,4,selected.index); putFlag(result,383,selected.valid);
  putFlag(result,382,selected.conflict);
  selected=encode(source<31>(ports.w31),false);
  put(result,377,5,selected.index); putFlag(result,376,selected.valid);
  selected=encode(source<31>(ports.w31),true);
  put(result,371,5,selected.index); putFlag(result,370,selected.valid);
  selected=encode(source<31>(ports.w31),false);
  put(result,365,5,selected.index); putFlag(result,364,selected.valid);
  putFlag(result,363,selected.conflict);
  selected=encode(source<31>(ports.w31),true);
  put(result,358,5,selected.index); putFlag(result,357,selected.valid);
  putFlag(result,356,selected.conflict);
  selected=encode(source<32>(ports.w32),false);
  put(result,351,5,selected.index); putFlag(result,350,selected.valid);
  selected=encode(source<32>(ports.w32),true);
  put(result,345,5,selected.index); putFlag(result,344,selected.valid);
  selected=encode(source<32>(ports.w32),false);
  put(result,339,5,selected.index); putFlag(result,338,selected.valid);
  putFlag(result,337,selected.conflict);
  selected=encode(source<32>(ports.w32),true);
  put(result,332,5,selected.index); putFlag(result,331,selected.valid);
  putFlag(result,330,selected.conflict);
  selected=encode(source<33>(ports.w33),false);
  put(result,324,6,selected.index); putFlag(result,323,selected.valid);
  selected=encode(source<33>(ports.w33),true);
  put(result,317,6,selected.index); putFlag(result,316,selected.valid);
  selected=encode(source<33>(ports.w33),false);
  put(result,310,6,selected.index); putFlag(result,309,selected.valid);
  putFlag(result,308,selected.conflict);
  selected=encode(source<33>(ports.w33),true);
  put(result,302,6,selected.index); putFlag(result,301,selected.valid);
  putFlag(result,300,selected.conflict);
  selected=encode(source<63>(ports.w63),false);
  put(result,294,6,selected.index); putFlag(result,293,selected.valid);
  selected=encode(source<63>(ports.w63),true);
  put(result,287,6,selected.index); putFlag(result,286,selected.valid);
  selected=encode(source<63>(ports.w63),false);
  put(result,280,6,selected.index); putFlag(result,279,selected.valid);
  putFlag(result,278,selected.conflict);
  selected=encode(source<63>(ports.w63),true);
  put(result,272,6,selected.index); putFlag(result,271,selected.valid);
  putFlag(result,270,selected.conflict);
  selected=encode(source<64>(ports.w64),false);
  put(result,264,6,selected.index); putFlag(result,263,selected.valid);
  selected=encode(source<64>(ports.w64),true);
  put(result,257,6,selected.index); putFlag(result,256,selected.valid);
  selected=encode(source<64>(ports.w64),false);
  put(result,250,6,selected.index); putFlag(result,249,selected.valid);
  putFlag(result,248,selected.conflict);
  selected=encode(source<64>(ports.w64),true);
  put(result,242,6,selected.index); putFlag(result,241,selected.valid);
  putFlag(result,240,selected.conflict);
  selected=encode(source<65>(ports.w65),false);
  put(result,233,7,selected.index); putFlag(result,232,selected.valid);
  selected=encode(source<65>(ports.w65),true);
  put(result,225,7,selected.index); putFlag(result,224,selected.valid);
  selected=encode(source<65>(ports.w65),false);
  put(result,217,7,selected.index); putFlag(result,216,selected.valid);
  putFlag(result,215,selected.conflict);
  selected=encode(source<65>(ports.w65),true);
  put(result,208,7,selected.index); putFlag(result,207,selected.valid);
  putFlag(result,206,selected.conflict);
  selected=encode(source<73>(ports.w73),false);
  put(result,199,7,selected.index); putFlag(result,198,selected.valid);
  selected=encode(source<73>(ports.w73),true);
  put(result,191,7,selected.index); putFlag(result,190,selected.valid);
  selected=encode(source<73>(ports.w73),false);
  put(result,183,7,selected.index); putFlag(result,182,selected.valid);
  putFlag(result,181,selected.conflict);
  selected=encode(source<73>(ports.w73),true);
  put(result,174,7,selected.index); putFlag(result,173,selected.valid);
  putFlag(result,172,selected.conflict);
  selected=encode(source<127>(ports.w127),false);
  put(result,165,7,selected.index); putFlag(result,164,selected.valid);
  selected=encode(source<127>(ports.w127),true);
  put(result,157,7,selected.index); putFlag(result,156,selected.valid);
  selected=encode(source<127>(ports.w127),false);
  put(result,149,7,selected.index); putFlag(result,148,selected.valid);
  putFlag(result,147,selected.conflict);
  selected=encode(source<127>(ports.w127),true);
  put(result,140,7,selected.index); putFlag(result,139,selected.valid);
  putFlag(result,138,selected.conflict);
  selected=encode(source<128>(ports.w128),false);
  put(result,131,7,selected.index); putFlag(result,130,selected.valid);
  selected=encode(source<128>(ports.w128),true);
  put(result,123,7,selected.index); putFlag(result,122,selected.valid);
  selected=encode(source<128>(ports.w128),false);
  put(result,115,7,selected.index); putFlag(result,114,selected.valid);
  putFlag(result,113,selected.conflict);
  selected=encode(source<128>(ports.w128),true);
  put(result,106,7,selected.index); putFlag(result,105,selected.valid);
  putFlag(result,104,selected.conflict);
  selected=encode(source<129>(ports.w129),false);
  put(result,96,8,selected.index); putFlag(result,95,selected.valid);
  selected=encode(source<129>(ports.w129),true);
  put(result,87,8,selected.index); putFlag(result,86,selected.valid);
  selected=encode(source<129>(ports.w129),false);
  put(result,78,8,selected.index); putFlag(result,77,selected.valid);
  putFlag(result,76,selected.conflict);
  selected=encode(source<129>(ports.w129),true);
  put(result,68,8,selected.index); putFlag(result,67,selected.valid);
  putFlag(result,66,selected.conflict);
  selected=encode(source<1>(ports.w1),true);
  put(result,58,8,selected.index); putFlag(result,57,selected.valid); putFlag(result,56,selected.conflict);
  selected=encode(source<73>(ports.w73,61,9),true);
  put(result,52,4,selected.index); putFlag(result,51,selected.valid); putFlag(result,50,selected.conflict);
  selected=encode(increment(source<5>(ports.w5)),true);
  put(result,47,3,selected.index); putFlag(result,46,selected.valid); putFlag(result,45,selected.conflict);
  selected=encode(source<3>(ports.w3),true);
  put(result,43,2,selected.index); putFlag(result,42,selected.valid); putFlag(result,41,selected.conflict);
  selected=encode(source<5>(ports.w5),true);
  put(result,38,3,selected.index); putFlag(result,37,selected.valid); putFlag(result,36,selected.conflict);
  selected=encode(source<8>(ports.w8,0,6),false);
  put(result,33,3,selected.index); putFlag(result,32,selected.valid);
  selected=encode(source<8>(ports.w8,0,6),true);
  put(result,29,3,selected.index); putFlag(result,28,selected.valid);
  selected=encode(source<8>(ports.w8,0,6),false);
  put(result,25,3,selected.index); putFlag(result,24,selected.valid);
  putFlag(result,23,selected.conflict);
  selected=encode(source<8>(ports.w8,0,6),true);
  put(result,20,3,selected.index); putFlag(result,19,selected.valid);
  putFlag(result,18,selected.conflict);
  selected=encode(source<8>(ports.w8,0,7),false);
  put(result,15,3,selected.index); putFlag(result,14,selected.valid);
  selected=encode(source<8>(ports.w8,0,7),true);
  put(result,11,3,selected.index); putFlag(result,10,selected.valid);
  selected=encode(source<8>(ports.w8,0,7),false);
  put(result,7,3,selected.index); putFlag(result,6,selected.valid);
  putFlag(result,5,selected.conflict);
  selected=encode(source<8>(ports.w8,0,7),true);
  put(result,2,3,selected.index); putFlag(result,1,selected.valid);
  putFlag(result,0,selected.conflict);
  return result;
}
template<class Output>
std::string check(const Output &output,const Expected &expected) {
  const auto &raw=output.packed();
  for(unsigned bit=0;bit<expected.size();++bit) {
    char symbol=expected[bit];bool known=symbol=='0'||symbol=='1';
    bool ok=raw.knownMask().bit(bit)==known && !raw.zMask().bit(bit) && (!known || raw.value().bit(bit)==(symbol=='1'));
    if(!ok) std::cerr<<"bit "<<bit<<" expected "<<symbol<<" value/known/z "<<raw.value().bit(bit)<<'/'<<raw.knownMask().bit(bit)<<'/'<<raw.zMask().bit(bit)<<'\n';
    require(ok);
  }
  std::string symbols;
  for(unsigned bit=expected.size();bit;--bit) symbols+=!raw.knownMask().bit(bit-1)?'x':raw.value().bit(bit-1)?'1':'0';
  return symbols;
}
template<class Output>
void tableCheck(const Output &output,unsigned row) {
  Expected explicitExpected{}; explicitExpected.fill('0');
  const auto &raw=output.packed();
  auto checkText=[&](unsigned low,std::string_view text) {
    for(unsigned bit=0;bit<text.size();++bit) {
      char symbol=text[text.size()-1-bit]; bool known=symbol=='0'||symbol=='1';
      require(raw.knownMask().bit(low+bit)==known && !raw.zMask().bit(low+bit) && (!known||raw.value().bit(low+bit)==(symbol=='1')));
    }
  };
  checkText(474,tableLow[row]);
  checkText(473,tableValid.substr(row,1));
  checkText(471,tableHigh[row]);
  checkText(470,tableValid.substr(row,1));
  checkText(468,tableLow[row]);
  checkText(467,tableValid.substr(row,1));
  checkText(466,tableConflict.substr(row,1));
  checkText(464,tableHigh[row]);
  checkText(463,tableValid.substr(row,1));
  checkText(462,tableConflict.substr(row,1));
}
} // namespace
int main(int argc,char **argv) {
  require(argc==2); unsigned workers=std::stoul(argv[1]); require(workers==1||workers==2);
  pyc_dut dut(workers); gfsim::SimExecutor executor(dut.system(),dut.observations(),{});
  constexpr std::string_view config=R"({"deadlock_window":null,"max_domain_cycles":{},"max_ticks":4096,"schema":"pycircuit-model-config","version":"1"})";
  require(executor.ConfigureJson(reinterpret_cast<const std::uint8_t*>(config.data()),config.size())==PYCIRCUIT_MODEL_STATUS_V1_OK);
  dut.drive(inputs(Pattern::Zero,0,false,false)); require(executor.Reset()==PYCIRCUIT_MODEL_STATUS_V1_OK);
  unsigned known=0,masked=0;
  auto run=[&](Pattern pattern,unsigned row=0,bool highZ=false,bool latent=false) {
    auto ports=inputs(pattern,row,highZ,latent); auto expected=golden(ports); dut.drive(ports);
    PycircuitModelStepResultV1 status{sizeof(status)};
    require(executor.Step(&status)==PYCIRCUIT_MODEL_STATUS_V1_OK); require(status.state==PYCIRCUIT_MODEL_STEP_V1_RUNNING);
    auto output=dut.sample().result; auto trace=check(output,expected);
    if(pattern==Pattern::Literal) tableCheck(output,row);
    bool isMasked=pattern>=Pattern::FourSmall;
    std::cout<<(isMasked?"MASK ":"WORK ")<<trace<<'\n'; ++(isMasked?masked:known); ++frame;
  };
  for(unsigned row=0;row<256;++row) run(Pattern::Scalar,row);
  run(Pattern::Zero);run(Pattern::Ones);
  for(unsigned row=0;row<129;++row) run(Pattern::OneHot,row);
  for(unsigned row=0;row<32;++row) run(Pattern::Mixed,row);
  require(known==419);
  // Complete 0/1/X/Z alphabet at W1-4, both latent native payload alternatives.
  for(unsigned row=0;row<256;++row) for(bool latent:{false,true}) run(Pattern::FourSmall,row,false,latent);
  // Every uncertain position: all-zero/one peers and known endpoint priorities.
  for(auto pattern:{Pattern::ZeroMasked,Pattern::OnesMasked,Pattern::UpperOne,Pattern::LowerOne})
    for(unsigned row=0;row<129;++row) for(bool z:{false,true}) for(bool latent:{false,true}) run(pattern,row,z,latent);
  for(unsigned phase=0;phase<4;++phase) for(bool latent:{false,true}) run(Pattern::Dense,phase,false,latent);
  // The actual seven reviewed inputs, followed by X->Z replacements. Both
  // encoders and both orders are checked directly against explicit literals.
  for(unsigned row=0;row<7;++row) for(bool z:{false,true}) for(bool latent:{false,true}) run(Pattern::Literal,row,z,latent);
  require(masked==2612); run(Pattern::Ones);
  require(known==420 && frame==3032);
}
