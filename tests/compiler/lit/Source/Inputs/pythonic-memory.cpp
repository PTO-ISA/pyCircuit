#include "pycircuit_system.hpp"
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <source_location>
#include <stdexcept>
#include <string>
#include <vector>

constexpr unsigned W = MEM_WIDTH, A = MEM_ADDR_WIDTH, S = (W + 7) / 8;
void require(bool ok, std::source_location at = std::source_location::current()) {
  if (!ok) { std::cerr << "memory oracle line " << at.line() << '\n'; std::abort(); }
}
template<unsigned N> auto known(std::uint64_t value) {
  return gfsim::wire<gfsim::Bits<N>>::known(gfsim::Bits<N>{value});
}
#ifdef MEM_PROTOCOL
constexpr unsigned RequestBits=REQUEST_BITS, ResultBits=RESULT_BITS;
struct ProtocolRow {
  unsigned clock,reset,v0,t0,v1,t1;
  std::uint64_t p0,p1,want,mask;
};
auto protocolInputs(const ProtocolRow &r) {
  pyc_dut::Inputs in;
  in.pyc_7079635f636c6b=known<1>(r.clock);in.pyc_7079635f727374=known<1>(r.reset);
#if MEM_PROTOCOL >= 5
  in.valid_writer=known<1>(r.v0);in.take_writer=known<1>(r.t0);
  in.writer=decltype(in.writer)::fromPacked(gfsim::FourState<RequestBits>::known(gfsim::Bits<RequestBits>{r.p0}));
  in.valid_reader=known<1>(r.v1);in.take_reader=known<1>(r.t1);
  in.reader=decltype(in.reader)::fromPacked(gfsim::FourState<RequestBits>::known(gfsim::Bits<RequestBits>{r.p1}));
#else
  in.valid=known<1>(r.v0);in.take=known<1>(r.t0);
  in.request=decltype(in.request)::fromPacked(gfsim::FourState<RequestBits>::known(gfsim::Bits<RequestBits>{r.p0}));
#endif
  return in;
}
void protocolDrive(pyc_root &root,const pyc_dut::Inputs &in) {
  root.pyc_7079635f636c6b=in.pyc_7079635f636c6b;root.pyc_7079635f727374=in.pyc_7079635f727374;
#if MEM_PROTOCOL >= 5
  root.valid_writer=in.valid_writer;root.writer=in.writer;root.take_writer=in.take_writer;
  root.valid_reader=in.valid_reader;root.reader=in.reader;root.take_reader=in.take_reader;
#else
  root.valid=in.valid;root.request=in.request;root.take=in.take;
#endif
}
void protocolCheck(const gfsim::FourState<ResultBits> &actual,const ProtocolRow &r,unsigned n) {
  require(actual.invariantHolds());
  if((actual.knownMask().value()&r.mask)!=r.mask||(actual.value().value()&r.mask)!=(r.want&r.mask))
    std::cerr<<"protocol mode="<<MEM_PROTOCOL<<" frame="<<n<<" wanted="<<r.want
             <<" mask="<<r.mask<<" value="<<actual.value().value()
             <<" known="<<actual.knownMask().value()<<'\n';
  require((actual.knownMask().value()&r.mask)==r.mask);
  require((actual.value().value()&r.mask)==(r.want&r.mask));
}
auto protocolRows(const char *path) {
  std::ifstream stream(path);require(stream.good());std::vector<ProtocolRow> rows;ProtocolRow r;
  while(stream>>r.clock>>r.reset>>r.v0>>r.p0>>r.t0>>r.v1>>r.p1>>r.t1>>r.want>>r.mask)
    rows.push_back(r);
  require(stream.eof()&&rows.size()==MEM_FRAMES);return rows;
}
void protocolTrace(unsigned workers,const std::vector<ProtocolRow> &rows) {
  pyc_dut dut(workers);dut.drive(protocolInputs({}));dut.system().Build();dut.system().Reset();
  unsigned n=0;
  for(const auto &r:rows) {
    dut.drive(protocolInputs(r));require(dut.system().Step()==gfsim::SimStepResult::Running);
    const auto actual=dut.sample().result.packed();protocolCheck(actual,r,n);
    std::cout<<"WORK "<<n++<<' ';
    for(unsigned bit=ResultBits;bit-- >0;)
      std::cout<<(((r.mask>>bit)&1)?(actual.value().bit(bit)?'1':'0'):'-');
    std::cout<<'\n';
  }
}
struct ProtocolSystem final : gfsim::SimSystem { bool Precheck() noexcept override { return true; } };
void protocolResetRead(ProtocolSystem &system,pyc_root &root,unsigned address,unsigned expected) {
  // A failed edge must not leak a RAM write; reset cancels protocol state but
  // retains previously committed RAM contents. Probe through the real interface.
  protocolDrive(root,protocolInputs({}));system.Reset();unsigned delivered=0;
  for(unsigned cycle=0;cycle<20;++cycle) {
    ProtocolRow r{};r.t0=r.t1=1;
    const std::uint64_t packet=(std::uint64_t{address}<<(RequestBits==29?25:24))|247;
#if MEM_PROTOCOL >= 5
    r.v1=cycle==0;r.p1=packet;
#else
    r.v0=cycle==0;r.p0=packet;
#endif
    for(unsigned clock:{0u,1u}) {
      r.clock=clock;protocolDrive(root,protocolInputs(r));
      require(system.Step()==gfsim::SimStepResult::Running);
      const auto result=root.result.packed();
#if MEM_PROTOCOL >= 5
      constexpr unsigned validBit=60;
      require(result.knownMask().bit(61)&&!result.value().bit(61));
#else
      constexpr unsigned validBit=RequestBits+2;
#endif
      require(result.knownMask().bit(validBit));
      if(result.value().bit(validBit)) {
        const std::uint64_t mask=(std::uint64_t{1}<<RequestBits)-1;
        require((result.knownMask().value()&mask)==mask);
        require((result.value().value()&mask)==(packet|(std::uint64_t{expected}<<8)));
        if(clock)++delivered;
      }
    }
  }
  require(delivered==1);
}
void protocolFailure(unsigned workers,const std::vector<ProtocolRow> &rows,
                     unsigned faultRow,unsigned flavor,bool resetProbe,unsigned address,unsigned word) {
  require(faultRow<rows.size()&&rows[faultRow].clock==1&&!rows[faultRow].reset);
  require(faultRow&&rows[faultRow-1].clock==0);
  gfsim::WorkExecutor pool(workers);pyc_root root("protocol-failure",&pool);
  ProtocolSystem system;protocolDrive(root,protocolInputs({}));
  require(system.AddModule(root));system.Build();system.Reset();bool manual=false;
  for(unsigned n=0;n<rows.size();++n) {
    const auto &r=rows[n];auto in=protocolInputs(r);
    if(n==faultRow) {
      const auto epoch=system.cycle();
      if(flavor<2) {
        in.pyc_7079635f636c6b=gfsim::wire<gfsim::Bits<1>>::fromPacked(
            gfsim::FourState<1>::fromMasks(gfsim::Bits<1>{1},gfsim::Bits<1>{0},
                                          gfsim::Bits<1>{flavor?1u:0u}));
        protocolDrive(root,in);require(system.Step()==gfsim::SimStepResult::Failed);
      } else {
        protocolDrive(root,in);root.Work();root.DiscardNext();root.Xfer();
      }
      require(system.cycle()==epoch);
      if(resetProbe) { protocolResetRead(system,root,address,word); return; }
      // Inspect old state without committing the diagnostic Work.
      in=protocolInputs(r);in.pyc_7079635f636c6b=known<1>(0);
      protocolDrive(root,in);root.Work();protocolCheck(root.result.packed(),r,n);
      root.DiscardNext();root.Xfer();
      // Controlled root retry retains the failed edge, then validates the entire
      // remaining independent trace, including response old data and RAM readback.
      in=protocolInputs(r);protocolDrive(root,in);root.Work();
      protocolCheck(root.result.packed(),r,n);root.Xfer();manual=true;
    } else {
      protocolDrive(root,in);
      if(manual) {
        root.Work();protocolCheck(root.result.packed(),r,n);root.Xfer();
      } else {
        require(system.Step()==gfsim::SimStepResult::Running);
        protocolCheck(root.result.packed(),r,n);
      }
    }
  }
}
int main(int argc,char **argv) {
  require(argc==4);unsigned workers=std::strtoul(argv[1],nullptr,10);
  require(workers==1||workers==2);auto rows=protocolRows(argv[2]);protocolTrace(workers,rows);
  std::ifstream faults(argv[3]);require(faults.good());std::string phase;unsigned row,address,word,n=0;
  while(faults>>phase>>row>>address>>word) {
    for(unsigned flavor=0;flavor<3;++flavor)for(bool resetProbe:{false,true})
      protocolFailure(workers,rows,row,flavor,resetProbe,address,word);
    std::cout<<"CHECK "<<phase<<" X Z discard retry reset-RAM\n";++n;
  }
  require(faults.eof()&&n==3);std::cout<<"PASS\n";
}
#else
void data(pyc_dut::Inputs &in, std::uint64_t value) {
  in.wdata = decltype(in.wdata)::fromPacked(
      gfsim::FourState<W>::known(gfsim::Bits<W>{value}));
}
void drive(pyc_root &root, const pyc_dut::Inputs &in) {
  root.ren0=in.ren0; root.raddr0=in.raddr0;
  root.ren1=in.ren1; root.raddr1=in.raddr1;
  root.wvalid=in.wvalid; root.waddr=in.waddr; root.wdata=in.wdata; root.wstrb=in.wstrb;
  root.pyc_7079635f636c6b=in.pyc_7079635f636c6b;
  root.pyc_7079635f727374=in.pyc_7079635f727374;
}
auto initial() {
  pyc_dut::Inputs in;
  in.ren0=in.ren1=known<1>(1); in.raddr0=in.raddr1=in.waddr=known<A>(0);
  in.wvalid=known<1>(0); in.wstrb=known<S>((1u << S) - 1); data(in,0);
  in.pyc_7079635f636c6b=in.pyc_7079635f727374=known<1>(0);
  return in;
}
template<class Packed> void expected(const Packed &packed, unsigned port,
                                    std::uint64_t value, bool valid) {
  const auto actual = gfsim::extract<W>(packed, port ? 0 : W);
  require(actual.invariantHolds());
  if (valid) {
    require(actual.isFullyKnown()); require(actual.value().value()==value);
  } else {
    require(actual.knownMask()==gfsim::Bits<W>{0});
    require(actual.zMask()==gfsim::Bits<W>{0});
  }
}
void trace(unsigned workers, const char *path) {
  pyc_dut dut(workers); auto in=initial(); dut.drive(in);
  dut.system().Build(); dut.system().Reset();
  std::ifstream stream(path); require(stream.good());
  unsigned clock,reset,ren0,ra0,ren1,ra1,write,wa,strobe,v0,v1,n=0;
  std::uint64_t word,q0,q1;
  while (stream>>clock>>reset>>ren0>>ra0>>ren1>>ra1>>write>>wa>>word>>strobe>>q0>>q1>>v0>>v1) {
    in.ren0=known<1>(ren0); in.raddr0=known<A>(ra0);
    in.ren1=known<1>(ren1); in.raddr1=known<A>(ra1);
    in.wvalid=known<1>(write); in.waddr=known<A>(wa); data(in,word); in.wstrb=known<S>(strobe);
    in.pyc_7079635f636c6b=known<1>(clock); in.pyc_7079635f727374=known<1>(reset);
    dut.drive(in); require(dut.system().Step()==gfsim::SimStepResult::Running);
    const auto out=dut.sample().result.packed(); expected(out,0,q0,v0); expected(out,1,q1,v1);
    // Compare valid windows; never replace invalid DUT Q with a fabricated value.
    std::cout << "WORK " << n++ << ' ';
    if(v0) std::cout<<q0; else std::cout<<'-';
    std::cout<<' '; if(v1) std::cout<<q1; else std::cout<<'-'; std::cout<<'\n';
  }
  require(stream.eof() && n==MEM_FRAMES);
}
struct System final : gfsim::SimSystem { bool Precheck() noexcept override { return true; } };
void failureAndDiscard(unsigned workers, bool z, bool control) {
  gfsim::WorkExecutor pool(workers); pyc_root root("discard",&pool);
  System system; auto in=initial(); drive(root,in);
  require(system.AddModule(root)); system.Build(); system.Reset();
  auto step=[&](unsigned clock) {
    in.pyc_7079635f636c6b=known<1>(clock); drive(root,in);
    require(system.Step()==gfsim::SimStepResult::Running);
  };
  step(0); in.wvalid=known<1>(1); data(in,0x1a55); step(1); step(0);
  in.wvalid=known<1>(0); step(1); step(0);
  expected(root.result.packed(),0,0x1a55,true);
  in.wvalid=known<1>(1);
  in.wdata=decltype(in.wdata)::fromPacked(gfsim::FourState<W>::fromMasks(
      gfsim::Bits<W>::ones(),gfsim::Bits<W>{0},z?gfsim::Bits<W>::ones():gfsim::Bits<W>{0}));
  if(control){
    data(in,0x1abc);
    in.wvalid=gfsim::wire<gfsim::Bits<1>>::fromPacked(gfsim::FourState<1>::fromMasks(
        gfsim::Bits<1>{1},gfsim::Bits<1>{0},gfsim::Bits<1>{z?1u:0u}));
  }
  in.pyc_7079635f636c6b=known<1>(1); drive(root,in);
  const auto epoch=system.cycle();
  require(system.Step()==gfsim::SimStepResult::Failed); require(system.cycle()==epoch);
  // Evaluate old Q/RAM after the system's discard, without committing this probe.
  in.wvalid=known<1>(0); data(in,0); in.pyc_7079635f636c6b=known<1>(0);
  drive(root,in); root.Work(); expected(root.result.packed(),0,0x1a55,true);
  expected(root.result.packed(),1,MEM_KIND==2?0x1a55:0,true);
  root.DiscardNext(); root.Xfer();
  // Retry the same rising edge with a real write: this also detects a leaked
  // clock-history update from the failed epoch or discarded diagnostic Work.
  in.wvalid=known<1>(1); data(in,0x1555); in.pyc_7079635f636c6b=known<1>(1);
  drive(root,in); root.Work(); root.Xfer();
  in.wvalid=known<1>(0); in.pyc_7079635f636c6b=known<1>(0);
  drive(root,in); root.Work(); root.Xfer();
  in.pyc_7079635f636c6b=known<1>(1); drive(root,in); root.Work(); root.Xfer();
  in.pyc_7079635f636c6b=known<1>(0); drive(root,in); root.Work();
  expected(root.result.packed(),0,0x1555,true); root.Xfer();
  // Explicitly discarded known writes must not alter RAM either.
  in.wvalid=known<1>(1); data(in,0); in.pyc_7079635f636c6b=known<1>(1);
  drive(root,in); root.Work(); root.DiscardNext(); root.Xfer();
  in.wvalid=known<1>(0); drive(root,in); root.Work(); root.Xfer();
  in.pyc_7079635f636c6b=known<1>(0); drive(root,in); root.Work();
  expected(root.result.packed(),0,0x1555,true); root.DiscardNext();
}
int main(int argc,char **argv) {
  require(argc==3); unsigned workers=std::strtoul(argv[1],nullptr,10);
  require(workers==1||workers==2); trace(workers,argv[2]);
  for(bool z:{false,true})for(bool control:{false,true})failureAndDiscard(workers,z,control);
  std::cout<<"PASS\n";
}

#endif // MEM_PROTOCOL
