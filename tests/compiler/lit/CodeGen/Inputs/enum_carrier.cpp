// Independent Python symbol goldens; no Runtime compare/select computes expectations.
#include "gfsim/SimExecutor.h"
#include "gfsim/SystemRunner.h"
#include "pycircuit_system.hpp"
#include <cstdlib>
#include <iostream>
#include <source_location>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
void require(bool ok, std::source_location at=std::source_location::current()) {
  if(!ok){std::cerr<<"enum execution oracle at "<<at.line()<<'\n';std::abort();}
}
template<unsigned W> auto input(std::string_view text) {
  require(text.size()==W);gfsim::Bits<W> value{0},known{0},z{0};
  for(unsigned bit=0;bit<W;++bit){
    const char symbol=text[W-bit-1];const auto word=bit/64;const auto one=std::uint64_t{1}<<(bit%64);
    if(symbol=='1'||((symbol=='x'||symbol=='z')&&bit%3==0))value.setWord(word,value.word(word)|one);
    if(symbol=='0'||symbol=='1')known.setWord(word,known.word(word)|one);
    if(symbol=='z')z.setWord(word,z.word(word)|one);
  }
  return gfsim::wire<gfsim::Bits<W>>::fromPacked(gfsim::FourState<W>::fromMasks(value,known,z));
}
template<unsigned W,class Wire> void expected(const Wire &wire,std::string_view golden) {
  require(golden.size()==W);const auto packed=wire.packed();std::string observed;
  for(unsigned bit=W;bit;--bit)observed+=packed.zMask().bit(bit-1)?'z':!packed.knownMask().bit(bit-1)?'x':packed.value().bit(bit-1)?'1':'0';
  if(observed!=golden){std::cerr<<observed<<" expected "<<golden<<'\n';require(false);}
}
template<class A,class B> void samePlanes(const A &a,const B &b) {
  require(a.packed().value()==b.packed().value());require(a.packed().knownMask()==b.packed().knownMask());require(a.packed().zMask()==b.packed().zMask());
}
#include "enum_vectors.hpp"
struct Context {
  pyc_dut &dut;unsigned sampled=0;
  static void initialize(void *opaque){require(driveNext(opaque,0));}
  static bool driveNext(void *opaque,std::uint64_t epoch){
    auto &self=*static_cast<Context*>(opaque);if(epoch==row_count)return false;require(epoch<row_count);
    pyc_dut::Inputs ports;drive(ports,epoch);self.dut.drive(ports);return true;
  }
  static void sample(void *opaque,std::uint64_t epoch){
    auto &self=*static_cast<Context*>(opaque);require(epoch==self.sampled+1&&self.sampled<row_count);
    const auto output=self.dut.sample();check(output,self.sampled);
    std::cout<<"WORK "<<self.sampled<<' '<<golden_trace[self.sampled]<<'\n';++self.sampled;
  }
};
#ifdef ENUM_STORAGE
void lifecycle(unsigned workers){
  gfsim::WorkExecutor pool(workers);pyc_root root("probe",&pool);
  pyc_dut::Inputs ports;driveProbe(ports,0);driveRoot(root,ports);
  // A default payload wrapper code{} must never establish known wire state.
  root.Work();cold(root);root.DiscardNext();root.Xfer();
  root.Reset();cold(root);root.Xfer();
  for(unsigned row=0;row<probe_count;++row){
    driveProbe(ports,row);driveRoot(root,ports);bool failed=false;
    try{root.Work();}catch(const gfsim::FourStateViolation&){failed=true;}
    require(failed==(probe_action[row]==2));if(!failed)checkProbe(root,row);
    if(probe_action[row])root.DiscardNext();root.Xfer();
  }
  for(unsigned row=0;row<failure_count;++row){
    pyc_dut dut(workers);gfsim::SimExecutor executor(dut.system(),dut.observations(),{});
    constexpr std::string_view config=R"({"deadlock_window":null,"max_domain_cycles":{},"max_ticks":16,"schema":"pycircuit-model-config","version":"1"})";
    require(executor.ConfigureJson(reinterpret_cast<const std::uint8_t*>(config.data()),config.size())==PYCIRCUIT_MODEL_STATUS_V1_OK);
    driveProbe(ports,0);dut.drive(ports);require(executor.Reset()==PYCIRCUIT_MODEL_STATUS_V1_OK);
    driveFailure(ports,row);dut.drive(ports);const auto epoch=executor.cycles();PycircuitModelStepResultV1 status{sizeof(status)};
    require(executor.Step(&status)==PYCIRCUIT_MODEL_STATUS_V1_RUNTIME_FAILURE);
    require(status.state==PYCIRCUIT_MODEL_STEP_V1_FAILED&&status.epoch_time==epoch);
    require(executor.cycles()==epoch&&dut.system().cycle()==epoch);
    bool unavailable=false;try{(void)dut.sample();}catch(const std::logic_error&){unavailable=true;}
    require(unavailable&&executor.Step(&status)==PYCIRCUIT_MODEL_STATUS_V1_INVALID_STATE);
    driveProbe(ports,0);dut.drive(ports);require(executor.Reset()==PYCIRCUIT_MODEL_STATUS_V1_OK);
    require(executor.Step(&status)==PYCIRCUIT_MODEL_STATUS_V1_OK);checkProbe(dut.sample(),0);
  }
}
#endif
int main(int argc,char **argv){
  gfsim::SystemRunner runner(argc,argv);if(!runner.ready())return 2;
  pyc_dut dut(runner.workers());Context context{dut};
  const gfsim::RunnerCallbacks callbacks{&context,&Context::initialize,&Context::driveNext,&Context::sample};
  const int status=runner.Run(dut.system(),dut.observations(),{},callbacks);require(status==0&&context.sampled==row_count);
#ifdef ENUM_STORAGE
  lifecycle(runner.workers());
#endif
  return status;
}
