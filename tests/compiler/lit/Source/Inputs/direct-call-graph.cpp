// Generated-DUT oracle independent of source graph binding and field scheduling.
#include "gfsim/SimExecutor.h"
#include "pycircuit_system.hpp"
#include <cstdlib>
#include <iostream>
#include <source_location>
#include <stdexcept>
#include <string>
#include <string_view>
#ifdef DIRECT_BYTE_STATIC
constexpr std::string_view config=R"({"deadlock_window":null,"max_domain_cycles":{},"max_ticks":2048,"schema":"pycircuit-model-config","version":"1"})";
#else
constexpr std::string_view config=R"({"deadlock_window":null,"max_domain_cycles":{},"max_ticks":96,"schema":"pycircuit-model-config","version":"1"})";
#endif
void require(bool ok,std::source_location at=std::source_location::current()) {
  if(!ok){std::cerr<<"direct-call oracle failed at "<<at.line()<<'\n';std::abort();}
}
template<unsigned W> auto known(std::uint64_t value) {
  return gfsim::wire<gfsim::Bits<W>>::known(gfsim::Bits<W>{value});
}
auto wide(unsigned row) {
  gfsim::Bits<73> value{0x0123456789abcdefULL ^ (std::uint64_t{row}*0x101010101010101ULL)};
  value.setWord(1,(row*51u+257u)%512u);
  return gfsim::wire<gfsim::Bits<73>>::known(value);
}
template<unsigned W> auto fromText(std::string_view bits) {
  require(bits.size()==W);gfsim::Bits<W> value{0},mask{0},z{0};
  for(unsigned n=0;n!=W;++n){unsigned bit=W-n-1,word=bit/64;auto one=std::uint64_t{1}<<(bit%64);
    if(bits[n]!='0')value.setWord(word,value.word(word)|one);
    if(bits[n]=='0'||bits[n]=='1')mask.setWord(word,mask.word(word)|one);
    if(bits[n]=='z')z.setWord(word,z.word(word)|one);
  }
  return gfsim::wire<gfsim::Bits<W>>::fromPacked(gfsim::FourState<W>::fromMasks(value,mask,z));
}
template<unsigned W,class Output>void equal(const Output &output,unsigned low,const gfsim::wire<gfsim::Bits<W>> &expected){
  const auto actual=gfsim::extract<W>(output.result.packed(),low);
  require(actual.value()==expected.packed().value());require(actual.knownMask()==expected.packed().knownMask());require(actual.zMask()==expected.packed().zMask());
}
template<unsigned W,class Output>void print(const Output &output,const char *prefix){
  const auto &p=output.result.packed();std::cout<<prefix<<' ';
  for(unsigned bit=W;bit!=0;--bit)std::cout<<(p.zMask().bit(bit-1)?'z':!p.knownMask().bit(bit-1)?'x':p.value().bit(bit-1)?'1':'0');
  std::cout<<'\n';
}
auto independentXor(const gfsim::wire<gfsim::Bits<73>> &a,const gfsim::wire<gfsim::Bits<73>> &b){
  gfsim::Bits<73> value{0},mask{0};
  for(unsigned bit=0;bit!=73;++bit){unsigned word=bit/64;auto one=std::uint64_t{1}<<(bit%64);
    if(a.packed().value().bit(bit)!=b.packed().value().bit(bit))value.setWord(word,value.word(word)|one);
    if(a.packed().knownMask().bit(bit)&&b.packed().knownMask().bit(bit))mask.setWord(word,mask.word(word)|one);
  }
  return gfsim::wire<gfsim::Bits<73>>::fromPacked(gfsim::FourState<73>::fromMasks(value,mask,gfsim::Bits<73>{0}));
}
#ifdef DIRECT_FEEDBACK
template<class Root>void driveRoot(Root &root,const pyc_dut::Inputs &input){
  root.delta=input.delta;root.enable=input.enable;
  root.pyc_7079635f636c6b=input.pyc_7079635f636c6b;root.pyc_7079635f727374=input.pyc_7079635f727374;
}
void discard(unsigned workers){
  gfsim::WorkExecutor pool(workers);pyc_root root("discard",&pool);
  pyc_dut::Inputs input;input.delta=wide(1);input.enable=known<1>(1);
  input.pyc_7079635f636c6b=known<1>(0);input.pyc_7079635f727374=known<1>(0);
  driveRoot(root,input);root.Reset();root.Xfer();auto state=known<73>(0);bool last=false;
  auto work=[&](unsigned clock,unsigned reset){
    input.pyc_7079635f636c6b=known<1>(clock);input.pyc_7079635f727374=known<1>(reset);
    driveRoot(root,input);root.Work();equal<73>(root,73,state);equal<73>(root,0,independentXor(state,input.delta));root.Xfer();
    if(clock&&!last)state=reset?known<73>(0):independentXor(state,input.delta);last=clock;
  };
  work(0,0);work(1,0);work(0,0);input.delta=wide(5);
  input.pyc_7079635f636c6b=known<1>(1);driveRoot(root,input);root.Work();equal<73>(root,73,state);
  root.DiscardNext();root.Xfer();work(1,0);work(0,0);
  input.pyc_7079635f636c6b=known<1>(1);input.pyc_7079635f727374=known<1>(1);
  driveRoot(root,input);root.Work();equal<73>(root,73,state);root.DiscardNext();root.Xfer();
  work(1,0);work(0,0);work(1,1);work(0,0);
  input.pyc_7079635f636c6b=known<1>(1);
  input.enable=gfsim::wire<gfsim::Bits<1>>::unknown();driveRoot(root,input);
  bool rejected=false;
  try{root.Work();}catch(const gfsim::FourStateViolation &){rejected=true;}
  require(rejected);root.DiscardNext();root.Xfer();
  input.enable=known<1>(1);work(1,0);work(0,0);
}
void failure(unsigned workers){
  pyc_dut dut(workers);gfsim::SimExecutor executor(dut.system(),dut.observations(),{});
  require(executor.ConfigureJson(reinterpret_cast<const std::uint8_t *>(config.data()),config.size())==PYCIRCUIT_MODEL_STATUS_V1_OK);
  require(executor.Reset()==PYCIRCUIT_MODEL_STATUS_V1_OK);
  pyc_dut::Inputs input;input.delta=wide(7);input.enable=gfsim::wire<gfsim::Bits<1>>::unknown();
  input.pyc_7079635f636c6b=known<1>(1);input.pyc_7079635f727374=known<1>(0);dut.drive(input);
  PycircuitModelStepResultV1 result{sizeof(result)};
  require(executor.Step(&result)==PYCIRCUIT_MODEL_STATUS_V1_RUNTIME_FAILURE);
  require(result.state==PYCIRCUIT_MODEL_STEP_V1_FAILED&&executor.cycles()==0&&dut.system().cycle()==0);
  bool rejected=false;try{(void)dut.sample();}catch(const std::logic_error &){rejected=true;}require(rejected);
  input.enable=known<1>(1);dut.drive(input);
  require(executor.Step(&result)==PYCIRCUIT_MODEL_STATUS_V1_INVALID_STATE);
  require(executor.Reset()==PYCIRCUIT_MODEL_STATUS_V1_OK);
  require(executor.Step(&result)==PYCIRCUIT_MODEL_STATUS_V1_OK);
  equal<73>(dut.sample(),73,known<73>(0));
  input.pyc_7079635f636c6b=known<1>(0);dut.drive(input);
  require(executor.Step(&result)==PYCIRCUIT_MODEL_STATUS_V1_OK);
  equal<73>(dut.sample(),73,input.delta);
}
#endif
#if defined(DIRECT_BYTE_STATIC) || defined(DIRECT_BYTE_STATE)
auto byteSum(const gfsim::wire<gfsim::Bits<8>> &total,
             const gfsim::wire<gfsim::Bits<8>> &input){
  if(!total.isFullyKnown()||!input.isFullyKnown())
    return gfsim::wire<gfsim::Bits<8>>::unknown();
  return known<8>((total.value().value()+input.value().value())&255u);
}
void bytePair(const auto &output,const auto &left,const auto &right){
  equal<8>(output,8,left);equal<8>(output,0,right);
}
#endif
#ifdef DIRECT_BYTE_STATE
void byteDriveRoot(pyc_root &root,const pyc_dut::Inputs &input){
  root.left=input.left;root.right=input.right;
  root.pyc_7079635f636c6b=input.pyc_7079635f636c6b;
  root.pyc_7079635f727374=input.pyc_7079635f727374;
}
void byteDiscard(unsigned workers){
  gfsim::WorkExecutor pool(workers);pyc_root root("byte-discard",&pool);
  pyc_dut::Inputs input;input.left=input.right=known<8>(0);
  input.pyc_7079635f636c6b=input.pyc_7079635f727374=known<1>(0);
  byteDriveRoot(root,input);root.Reset();root.Xfer();
  auto left=known<8>(0),right=known<8>(0);bool last=false;
  auto prepare=[&](unsigned clock,unsigned reset,unsigned l,unsigned r){
    input.left=known<8>(l);input.right=known<8>(r);
    input.pyc_7079635f636c6b=known<1>(clock);input.pyc_7079635f727374=known<1>(reset);
    byteDriveRoot(root,input);root.Work();
    bytePair(root,byteSum(left,input.left),byteSum(right,input.right));
  };
  auto commit=[&](unsigned clock,unsigned reset,unsigned l,unsigned r){
    prepare(clock,reset,l,r);root.Xfer();
    if(clock&&!last){left=reset?known<8>(0):byteSum(left,input.left);
      right=reset?known<8>(0):byteSum(right,input.right);}last=clock;
  };
  commit(0,0,0,0);commit(1,0,1,10);commit(0,0,0,0);
  prepare(1,0,200,30);root.DiscardNext();root.Xfer();
  // A different retry detects both leaked proposals and a leaked high clock.
  commit(1,0,3,0);commit(0,0,0,0);
  prepare(1,1,7,9);root.DiscardNext();root.Xfer();
  commit(1,0,0,11);commit(0,0,0,0);
  input.pyc_7079635f636c6b=gfsim::wire<gfsim::Bits<1>>::unknown();
  byteDriveRoot(root,input);bool rejected=false;
  try{root.Work();}catch(const gfsim::FourStateViolation &){rejected=true;}
  require(rejected);root.DiscardNext();root.Xfer();
  commit(1,0,1,2);commit(0,0,0,0);
  bytePair(root,known<8>(5),known<8>(23));
}
#endif
int main(int argc,char **argv){
  require(argc==2);unsigned workers=static_cast<unsigned>(std::stoul(argv[1]));pyc_dut dut(workers);
  gfsim::SimExecutor executor(dut.system(),dut.observations(),{});
  require(executor.ConfigureJson(reinterpret_cast<const std::uint8_t *>(config.data()),config.size())==PYCIRCUIT_MODEL_STATUS_V1_OK);
  require(executor.Reset()==PYCIRCUIT_MODEL_STATUS_V1_OK);
  auto step=[&](const pyc_dut::Inputs &input){dut.drive(input);PycircuitModelStepResultV1 status{sizeof(status)};
    require(executor.Step(&status)==PYCIRCUIT_MODEL_STATUS_V1_OK);require(status.state==PYCIRCUIT_MODEL_STEP_V1_RUNNING);return dut.sample();};
#ifdef DIRECT_BYTE_STATIC
  auto row=[&](const auto &l,const auto &r,const char *prefix){
    pyc_dut::Inputs input;input.left=l;input.right=r;auto output=step(input);
    bytePair(output,byteSum(known<8>(1),l),byteSum(known<8>(1),r));print<16>(output,prefix);
  };
  for(unsigned n=0;n!=256;++n){
    row(known<8>(n),known<8>((197*n+31)&255u),"WORK");
    row(known<8>(n),known<8>(n),"WORK");
    row(known<8>(n),known<8>(0),"WORK");
    row(known<8>(0),known<8>(n),"WORK");
  }
  for(unsigned n=0;n!=4;++n){
    auto unknown=fromText<8>(n%2?"zzzzzzzz":"xxxxxxxx");
    if(n<2)row(unknown,known<8>(n?31:17),"MASK");
    else row(known<8>(n==2?255:0),unknown,"MASK");
    row(known<8>(n+3),known<8>(n+7),"WORK");
  }
#elif defined(DIRECT_BYTE_STATE)
  auto left=known<8>(0),right=known<8>(0);bool last=false;
  auto row=[&](unsigned clock,unsigned reset,const auto &l,const auto &r,const char *prefix){
    pyc_dut::Inputs input;input.left=l;input.right=r;
    input.pyc_7079635f636c6b=known<1>(clock);input.pyc_7079635f727374=known<1>(reset);
    auto output=step(input);auto nextLeft=byteSum(left,l),nextRight=byteSum(right,r);
    bytePair(output,nextLeft,nextRight);if(prefix)print<16>(output,prefix);
    if(clock&&!last){left=reset?known<8>(0):nextLeft;right=reset?known<8>(0):nextRight;}last=clock;
  };
  struct Row{unsigned clock,reset,left,right;};
  constexpr Row trace[]={{0,0,0,0},{1,0,1,10},{0,0,0,0},{1,0,2,0},
    {0,0,0,0},{1,0,253,1},{0,0,0,0},{1,0,0,245},{1,0,7,99},{1,0,8,1},
    {0,0,0,0},{1,0,13,29},{0,0,0,0},{1,0,3,0},{0,0,0,0},{1,0,0,200},
    {0,0,0,0},{1,1,9,11},{1,0,4,5},{0,0,0,0},{1,0,255,1},{0,0,0,0}};
  for(const auto &r:trace)row(r.clock,r.reset,known<8>(r.left),known<8>(r.right),"WORK");
  for(unsigned n=0;n!=4;++n){
    auto unknown=fromText<8>(n%2?"zzzzzzzz":"xxxxxxxx");
    if(n<2)row(0,0,unknown,known<8>(n?31:17),"MASK");
    else row(0,0,known<8>(n==2?255:0),unknown,"MASK");
    row(0,0,known<8>(n+3),known<8>(n+7),"WORK");
  }
  row(1,0,fromText<8>("xxxxxxxx"),known<8>(7),nullptr);
  row(0,0,known<8>(0),known<8>(0),"MASK");
  row(1,1,known<8>(0),known<8>(0),nullptr);
  row(0,0,known<8>(2),known<8>(3),"WORK");
  row(1,0,known<8>(4),known<8>(5),"WORK");
  row(0,0,known<8>(0),known<8>(0),"WORK");
  byteDiscard(workers);
#elif defined(DIRECT_FEEDBACK)
  auto state=known<73>(0);bool last=false;
  auto row=[&](unsigned clock,unsigned reset,unsigned enable,const auto &delta,const char *prefix){
    pyc_dut::Inputs input;input.pyc_7079635f636c6b=known<1>(clock);input.pyc_7079635f727374=known<1>(reset);
    input.enable=known<1>(enable);input.delta=delta;auto output=step(input);
    equal<73>(output,73,state);equal<73>(output,0,independentXor(state,delta));if(prefix)print<146>(output,prefix);
    if(clock&&!last){if(reset)state=known<73>(0);else if(enable)state=independentXor(state,delta);}last=clock;
  };
  constexpr unsigned clocks[]={0,1,1,0,1,0,1,0,0,1,1,0,1,0,1,0};
  for(unsigned n=0;n!=16;++n)row(clocks[n],n==9,n!=4&&n!=6,wide(n),"WORK");
  for(unsigned n=0;n!=4;++n){auto unknown=wide(3+n);gfsim::Bits<73> mask{0},z{0};
    if(n%2)z=gfsim::Bits<73>::ones();
    unknown=gfsim::wire<gfsim::Bits<73>>::fromPacked(gfsim::FourState<73>::fromMasks(unknown.packed().value(),mask,z));
    row(0,0,1,unknown,nullptr);row(1,0,1,unknown,nullptr);row(0,0,0,known<73>(0),"MASK");
    row(1,1,0,known<73>(0),nullptr);row(0,0,0,known<73>(0),nullptr);
  }
  discard(workers);
  failure(workers);
#elif defined(DIRECT_LEXICAL)
  for(unsigned n=0;n!=12;++n){pyc_dut::Inputs input;input.first=known<13>(n*37);input.second=known<13>(n*101+17);auto output=step(input);
    const unsigned offsets[]={104,91,78,65,52,39,26,13,0};const bool later[]={false,true,true,false,false,true,false,false,true};
    for(unsigned field=0;field!=9;++field)equal<13>(output,offsets[field],later[field]?input.second:input.first);print<117>(output,"WORK");}
#else
  auto row=[&](const auto &value,const auto &x,const auto &y,const char *prefix){pyc_dut::Inputs input;input.wide=value;
#ifdef DIRECT_GRAPH
    input.x=x;input.y=y;auto output=step(input);equal<13>(output,185,x);equal<13>(output,172,y);
    equal<13>(output,159,y);equal<13>(output,146,x);equal<73>(output,73,value);equal<73>(output,0,value);print<198>(output,prefix);
#else
    (void)x;(void)y;auto output=step(input);
    for(unsigned offset:{365u,292u,219u,146u})equal<73>(output,offset,value);
    equal<73>(output,73,known<73>(17));equal<73>(output,0,known<73>(17));print<438>(output,prefix);
#endif
  };
  for(unsigned n=0;n!=12;++n)row(wide(n),known<13>(n*37),known<13>(n*101+17),"WORK");
  for(unsigned n=0;n!=4;++n){auto value=wide(n);gfsim::Bits<73> mask=gfsim::Bits<73>::ones(),z{0};
    if(n<2){mask.setWord(0,0x7fffffffffffffffULL);mask.setWord(1,510);if(n==1){z.setWord(0,0x8000000000000000ULL);z.setWord(1,1);}}
    else{mask=gfsim::Bits<73>{0};if(n==3)z=gfsim::Bits<73>::ones();}
    value=gfsim::wire<gfsim::Bits<73>>::fromPacked(gfsim::FourState<73>::fromMasks(value.packed().value(),mask,z));
    row(value,known<13>(73+n),known<13>(171+n),"MASK");}
#endif
}
