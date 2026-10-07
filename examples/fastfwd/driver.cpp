// Full-port oracle for the historical stateless Fastfwd stub.
#include "gfsim/SimExecutor.h"
#include "gfsim/SystemRunner.h"
#include "pycircuit_system.hpp"
#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <source_location>
#include <string_view>
#include <type_traits>

void require(bool ok,
             std::source_location at = std::source_location::current()) {
  if (!ok) {
    std::cerr << "fastfwd oracle failed at " << at.line() << '\n';
    std::abort();
  }
}
template <unsigned W> auto known(std::uint64_t value) {
  return gfsim::wire<gfsim::Bits<W>>::known(gfsim::Bits<W>{value});
}
auto data(std::uint64_t low, std::uint64_t high) {
  gfsim::Bits<128> value{low};value.setWord(1,high);
  return gfsim::wire<gfsim::Bits<128>>::known(value);
}
struct Channel {
  gfsim::wire<gfsim::Bits<1>> valid;
  gfsim::wire<gfsim::Bits<128>> data;
};
struct Lane : Channel {
  gfsim::wire<gfsim::Bits<5>> control;
};
struct Frame {
  std::array<Lane,4> lanes;
  std::array<Channel,4> engines;
};
Channel knownChannel(unsigned epoch, unsigned path) {
  std::uint64_t low = 0x0123456789abcdefULL ^ (std::uint64_t{epoch} * 0x100000001ULL) ^
                      (std::uint64_t{path} * 0x0101010101010101ULL);
  std::uint64_t high = 0xfedcba9876543210ULL ^ (std::uint64_t{epoch} * 0xa500000000000001ULL) ^
                       (std::uint64_t{path} * 0x0f0f0f0f0f0f0f0fULL);
  if (epoch == 0) {low=0;high=0;}
  if (epoch == 1) {low=~std::uint64_t{0};high=0;}
  if (epoch == 2) {low=0;high=~std::uint64_t{0};}
  if (epoch == 3) {low=0;high=std::uint64_t{1} << ((path*9)%64);}
  if (epoch == 4) {low=std::uint64_t{1} << ((path*9)%64);high=0;}
  if (epoch == 5) {low=0xaaaaaaaaaaaaaaaaULL ^ path;high=0x5555555555555555ULL ^ path;}
  const unsigned valid=epoch<8 ? (epoch+path)%2
                              : epoch<16 ? unsigned(path==epoch-8) : unsigned(path!=epoch-16);
  return {known<1>(valid),data(low,high)};
}
Frame knownFrame(unsigned epoch) {
  Frame frame;
  for (unsigned n=0;n!=4;++n) {
    const auto channel=knownChannel(epoch,n);
    frame.lanes[n]={channel,known<5>((epoch*7+n*11)%32)};
    frame.engines[n]=knownChannel(epoch,n+4);
  }
  return frame;
}
pyc_dut::Inputs inputs(const Frame &frame) {
  pyc_dut::Inputs input;
  static_assert(std::remove_cvref_t<decltype(input.lane0.packed().value())>::kWidth==134);
  static_assert(std::remove_cvref_t<decltype(input.engine0.packed().value())>::kWidth==129);
  auto lane = [&](unsigned n) {
    const auto &value=frame.lanes[n];
    return gfsim::concat(gfsim::concat(value.valid.packed(),value.data.packed()),value.control.packed());
  };
  auto engine = [&](unsigned n) {
    return gfsim::concat(frame.engines[n].valid.packed(),frame.engines[n].data.packed());
  };
  input.lane0=decltype(input.lane0)::fromPacked(lane(0));
  input.lane1=decltype(input.lane1)::fromPacked(lane(1));
  input.lane2=decltype(input.lane2)::fromPacked(lane(2));
  input.lane3=decltype(input.lane3)::fromPacked(lane(3));
  input.engine0=decltype(input.engine0)::fromPacked(engine(0));
  input.engine1=decltype(input.engine1)::fromPacked(engine(1));
  input.engine2=decltype(input.engine2)::fromPacked(engine(2));
  input.engine3=decltype(input.engine3)::fromPacked(engine(3));
  return input;
}
template <unsigned W, class Packed>
void transported(const Packed &output,unsigned low,const gfsim::wire<gfsim::Bits<W>> &input) {
  const auto actual=gfsim::extract<W>(output,low);
  require(actual.value()==input.packed().value());
  require(actual.knownMask()==input.packed().knownMask());
  require(actual.zMask()==input.packed().zMask());
}
template <unsigned W, class Packed> void zero(const Packed &output,unsigned low) {
  const auto actual=gfsim::extract<W>(output,low);
  require(actual.value()==gfsim::Bits<W>{0});
  require(actual.knownMask()==gfsim::Bits<W>::ones());
  require(actual.zMask()==gfsim::Bits<W>{0});
}
template <class Output> void check(const Output &output,const Frame &frame,bool emit=false) {
  const auto &packed=output.result.packed();
  static_assert(std::remove_cvref_t<decltype(packed.value())>::kWidth==1557);
  // Independent declaration-order layout: bkpr1, four Channel129, four Engine260.
  zero<1>(packed,1556);
  for(unsigned n=0;n!=4;++n) {
    const unsigned laneLow=1427-129*n,engineLow=780-260*n;
    transported<1>(packed,laneLow+128,frame.lanes[n].valid);
    transported<128>(packed,laneLow,frame.lanes[n].data);
    transported<1>(packed,engineLow+259,frame.engines[n].valid);
    transported<128>(packed,engineLow+131,frame.engines[n].data);
    zero<2>(packed,engineLow+129);zero<1>(packed,engineLow+128);zero<128>(packed,engineLow);
  }
  if(emit) {
    require(output.result.isFullyKnown());
    std::cout << "WORK ";
    for(unsigned bit=1557;bit!=0;--bit)std::cout << (packed.value().bit(bit-1)?'1':'0');
    std::cout << '\n';
  }
}
struct RunnerContext {
  pyc_dut &dut;
  unsigned sampled=0;
  static void initialize(void *opaque) {require(drive(opaque,0));}
  static bool drive(void *opaque,std::uint64_t epoch) {
    auto &self=*static_cast<RunnerContext *>(opaque);
    if(epoch==24)return false;
    require(epoch<24);self.dut.drive(inputs(knownFrame(static_cast<unsigned>(epoch))));return true;
  }
  static void sample(void *opaque,std::uint64_t epoch) {
    auto &self=*static_cast<RunnerContext *>(opaque);
    require(epoch==self.sampled+1 && self.sampled<24);
    check(self.dut.sample(),knownFrame(self.sampled),true);++self.sampled;
  }
};
template <unsigned W> auto masks(const gfsim::Bits<W> &value,const gfsim::Bits<W> &knownMask,
                                 const gfsim::Bits<W> &zMask) {
  return gfsim::wire<gfsim::Bits<W>>::fromPacked(gfsim::FourState<W>::fromMasks(value,knownMask,zMask));
}
void nativeFourState(unsigned workers) {
  pyc_dut dut(workers);gfsim::SimExecutor executor(dut.system(),dut.observations(),{});
  constexpr std::string_view config="{}";
  require(executor.ConfigureJson(reinterpret_cast<const std::uint8_t *>(config.data()),config.size())==PYCIRCUIT_MODEL_STATUS_V1_OK);
  require(executor.Reset()==PYCIRCUIT_MODEL_STATUS_V1_OK);
  auto step = [&](const Frame &frame,bool emit) {
    dut.drive(inputs(frame));PycircuitModelStepResultV1 status{sizeof(status)};
    require(executor.Step(&status)==PYCIRCUIT_MODEL_STATUS_V1_OK);
    require(status.state==PYCIRCUIT_MODEL_STEP_V1_RUNNING);
    const auto output=dut.sample();check(output,frame);
    if(emit) {
      const auto &packed=output.result.packed();std::cout << "MASK ";
      for(unsigned bit=1557;bit!=0;--bit)
        std::cout << (packed.zMask().bit(bit-1)?'z':!packed.knownMask().bit(bit-1)?'x'
                                                       :packed.value().bit(bit-1)?'1':'0');
      std::cout << '\n';
    }
  };
  for(unsigned mode=0;mode!=6;++mode) {
    Frame frame=knownFrame(10+mode);
    for(unsigned n=0;n!=4;++n) {
      // Unused controls may be X/Z without gating any output or constant.
      frame.lanes[n].control=masks<5>(gfsim::Bits<5>{31},gfsim::Bits<5>{0},gfsim::Bits<5>{mode%2?31u:0u});
      if(mode>=2) {
        frame.lanes[n].valid=masks<1>(gfsim::Bits<1>{1},gfsim::Bits<1>{0},gfsim::Bits<1>{(mode+n)%2});
        frame.engines[n].valid=masks<1>(gfsim::Bits<1>{1},gfsim::Bits<1>{0},gfsim::Bits<1>{(mode+n+1)%2});
        gfsim::Bits<128> value=frame.lanes[n].data.packed().value();
        gfsim::Bits<128> mask=gfsim::Bits<128>::ones(),z{0};
        if(mode==2) {mask.setWord(0,0xfffffffffffffffeULL);z.setWord(1,std::uint64_t{1}<<63);mask.setWord(1,0x7fffffffffffffffULL);}
        if(mode==3) {mask=gfsim::Bits<128>{0};z=gfsim::Bits<128>::ones();}
        if(mode==4) {mask=gfsim::Bits<128>{0};}
        if(mode==5) {mask.setWord(0,0x5555555555555555ULL);mask.setWord(1,0xaaaaaaaaaaaaaaaaULL);z.setWord(0,0xaaaaaaaaaaaaaaaaULL);}
        frame.lanes[n].data=masks<128>(value,mask,z);
        value=frame.engines[n].data.packed().value();
        frame.engines[n].data=masks<128>(value,mask,z);
      }
    }
    step(frame,true);
    // The next known inputs recover in the same epoch, without Reset or a clock.
    step(knownFrame(23-mode),false);
  }
}
int main(int argc,char **argv) {
  gfsim::SystemRunner runner(argc,argv);if(!runner.ready())return 2;
  pyc_dut dut(runner.workers());RunnerContext context{dut};
  const gfsim::RunnerCallbacks callbacks{&context,&RunnerContext::initialize,&RunnerContext::drive,&RunnerContext::sample};
  const int result=runner.Run(dut.system(),dut.observations(),{},callbacks);
  require(result==0 && context.sampled==24);nativeFourState(runner.workers());return result;
}
