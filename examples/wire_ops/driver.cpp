#include "gfsim/SystemRunner.h"
#include "pycircuit_system.hpp"
#include <cstdint>
#include <cstdlib>
#include <iostream>

void require(bool ok) { if (!ok) std::abort(); }
template <unsigned W> auto known(unsigned v) {
  return gfsim::wire<gfsim::Bits<W>>::known(gfsim::Bits<W>{v});
}

struct Row { unsigned clk,rst,a,b,sel,y; };
// Fixed hardware oracle. y is the old-Q Work snapshot; each active edge
// becomes visible in the next row. Rows 0..2 retain the original TB case.
constexpr Row rows[] = {
  {0,0,3,1,1,0}, {1,0,3,1,1,0}, {0,0,3,1,1,1},
  {1,0,170,15,0,1}, {0,0,170,15,0,165},
  {0,0,255,0,1,165}, {1,0,255,0,1,165}, {0,0,255,0,1,0},
  {1,0,255,255,1,0}, {0,0,255,255,1,255},
  {1,1,255,255,1,255}, {0,1,255,255,1,0},
  {1,0,128,1,0,0}, {0,0,128,1,0,129},
};

struct Context {
  pyc_dut &dut;
  unsigned sampled=0;
  static void initialize(void *opaque) {
    require(static_cast<Context *>(opaque)->sampled==0);
    require(drive(opaque,0));
  }
  static bool drive(void *opaque,std::uint64_t epoch) {
    auto &self=*static_cast<Context *>(opaque);
    if(epoch==std::size(rows))return false;
    require(epoch<std::size(rows));
    const auto &r=rows[epoch];
    pyc_dut::Inputs in;
    in.pyc_7079635f636c6b=known<1>(r.clk);in.pyc_7079635f727374=known<1>(r.rst);
    in.a=known<8>(r.a);in.b=known<8>(r.b);in.sel=known<1>(r.sel);
    self.dut.drive(in);
    return true;
  }
  static void sample(void *opaque,std::uint64_t committed) {
    auto &self=*static_cast<Context *>(opaque);
    require(committed==self.sampled+1 && self.sampled<std::size(rows));
    const auto output=self.dut.sample();
    require(output.result.isFullyKnown());
    const auto y=output.result.packed().value().value();
    require(y==rows[self.sampled].y);
    ++self.sampled;
    std::cout << "WORK " << y << '\n';
  }
};

int main(int argc,char **argv) {
  gfsim::SystemRunner runner(argc,argv);
  if(!runner.ready())return 2;
  pyc_dut dut(runner.workers());
  Context context{dut};
  const gfsim::RunnerCallbacks callbacks{&context,&Context::initialize,
                                         &Context::drive,&Context::sample};
  const int result=runner.Run(dut.system(),dut.observations(),{},callbacks);
  require(result==0 && context.sampled==std::size(rows));
  return result;
}
