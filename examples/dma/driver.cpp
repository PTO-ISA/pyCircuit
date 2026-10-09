#include "gfsim/SystemRunner.h"
#include "pycircuit_system.hpp"
#include <array>
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <deque>
#include <fstream>
#include <iostream>
#include <optional>
#include <source_location>
#include <string>
#include <vector>

void require(bool condition, std::source_location at = std::source_location::current()) {
  if (!condition) { std::cerr << "DMA independent oracle line " << at.line() << '\n'; std::abort(); }
}
struct Packet {
  unsigned dram=0, sram=0, data=0, tag=0;
  std::uint32_t packed() const { return (dram<<28)|(sram<<24)|(data<<8)|tag; }
  bool operator==(const Packet&) const = default;
};
struct Row {
  bool clock=0, reset=0;
  std::array<bool,3> valid{};
  std::array<Packet,3> request{};
  std::array<bool,3> take{true,true,true};
};
struct Expected {
  std::array<bool,3> ready{}, valid{};
  std::array<unsigned,2> accepted{}, enqueued{};
  std::array<Packet,3> response{};
};
// Seven independent latency-one queues. Jobs use absolute edge deadlines and
// old RAM words recorded when accepted, independently of controller registers.
struct Oracle {
  enum Q { SeedRequest, CopyRequest, CheckRequest, SeedResponse, DramResponse, CopyResponse, CheckResponse };
  static constexpr std::array<unsigned,7> capacity{2,4,2,2,2,2,2};
  struct Job { Packet response; unsigned endpoint; std::uint64_t due; };
  std::array<std::deque<Packet>,7> queues;
  std::array<std::array<unsigned,16>,2> ram{};
  std::array<std::optional<Job>,2> jobs;
  std::array<unsigned,7> peak{};
  std::array<unsigned,2> priority{}, blocked{}, concurrent{};
  unsigned sent=0, retired=0, canceled=0, held=0, resets=0;
  std::uint64_t edge=0;
  bool last=false;
  bool room(unsigned q) const { return queues[q].size()<capacity[q]; }
  Expected output() const {
    Expected e;
    for (unsigned i=0;i<3;++i) e.ready[i]=room(i);
    constexpr unsigned responseQ[]{SeedResponse,CopyResponse,CheckResponse};
    for (unsigned i=0;i<3;++i) {
      e.valid[i]=!queues[responseQ[i]].empty();
      if(e.valid[i]) e.response[i]=queues[responseQ[i]].front();
    }
    for(unsigned m=0;m<2;++m) {
      if(jobs[m]) {
        auto j=*jobs[m];
        unsigned q=m==0?(j.endpoint==1?DramResponse:SeedResponse):(j.endpoint==1?CheckResponse:CopyResponse);
        if(edge>=j.due && room(q)) e.enqueued[m]=j.endpoint;
      } else {
        unsigned high=m==0?CopyRequest:CheckRequest, low=m==0?SeedRequest:DramResponse;
        if(!queues[high].empty()) e.accepted[m]=1;
        else if(!queues[low].empty()) e.accepted[m]=2;
      }
    }
    return e;
  }
  unsigned pending() const {
    unsigned n=unsigned(bool(jobs[0]))+unsigned(bool(jobs[1]));
    for(auto &q:queues)n+=q.size();
    return n;
  }
  void commit(const Row &r) {
    if(r.clock && !last) {
      auto e=output();
      if(r.reset) {
        canceled+=pending();for(auto &q:queues)q.clear();jobs={};++resets;
      } else {
        auto old=queues;
        // Pop public responses and RAM request heads from the old queues.
        constexpr unsigned publicQ[]{SeedResponse,CopyResponse,CheckResponse};
        for(unsigned i=0;i<3;++i)if(e.valid[i]&&r.take[i]) { queues[publicQ[i]].pop_front();++retired; }
        for(unsigned m=0;m<2;++m) {
          unsigned high=m==0?CopyRequest:CheckRequest, low=m==0?SeedRequest:DramResponse;
          if(e.accepted[m]) {
            if(!old[high].empty()&&!old[low].empty())++priority[m];
            auto q=e.accepted[m]==1?high:low;
            Packet p=old[q].front();queues[q].pop_front();
            unsigned a=m==0?p.dram:p.sram;
            unsigned oldData=ram[m][a];
            if(e.accepted[m]==2)ram[m][a]=p.data;
            p.data=oldData;jobs[m]=Job{p,e.accepted[m],edge+(m==0?3:2)};
          } else if(jobs[m]) {
            if(e.enqueued[m]) {
              auto j=*jobs[m];
              unsigned q=m==0?(j.endpoint==1?DramResponse:SeedResponse):(j.endpoint==1?CheckResponse:CopyResponse);
              queues[q].push_back(j.response);jobs[m].reset();
            } else if(edge>=jobs[m]->due)++blocked[m];
          }
        }
        for(unsigned i=0;i<3;++i)if(r.valid[i]&&e.ready[i]) {queues[i].push_back(r.request[i]);++sent;}
        if(jobs[0]&&jobs[1])++concurrent[0];
        for(unsigned q=0;q<7;++q) {peak[q]=std::max(peak[q],unsigned(queues[q].size()));require(queues[q].size()<=capacity[q]);}
        require(sent==retired+canceled+pending());
      }
      ++edge;
    } else ++held;
    last=r.clock;
  }
};
std::string binary(std::uint64_t value,unsigned width) {
  std::string s;for(unsigned b=width;b;--b)s+=((value>>(b-1))&1)?'1':'0';return s;
}
std::string packed(const Expected &e,bool mask=false) {
  std::string s;
  for(bool b:e.ready)s+=mask?'1':b?'1':'0';
  for(bool b:e.valid)s+=mask?'1':b?'1':'0';
  for(unsigned m=0;m<2;++m) {s+=mask?"11":binary(e.accepted[m],2);s+=mask?"11":binary(e.enqueued[m],2);}
  for(unsigned i=0;i<3;++i)s+=mask?std::string(32,e.valid[i]?'1':'0'):binary(e.valid[i]?e.response[i].packed():0,32);
  require(s.size()==110);return s;
}
struct Plan {
  Oracle oracle;
  std::vector<Row> rows;
  void add(Row r) {rows.push_back(r);oracle.commit(r);}
  void tick(Row r={}) {r.clock=1;add(r);r.clock=0;add(r);}
  void reset() {Row r;r.reset=1;tick(r);}
  void drain() {for(unsigned n=0;oracle.pending();++n) {require(n<1000);tick();}}
  void transfer(unsigned endpoint,Packet p) {
    Row r;r.valid[endpoint]=1;r.request[endpoint]=p;
    unsigned n=0;while(!oracle.output().ready[endpoint]) {require(n++<1000);tick();}
    tick(r);
  }
  void finish(unsigned endpoint,Packet expected) {
    for(unsigned n=0;;++n) {
      require(n<1000);auto out=oracle.output();
      if(out.valid[endpoint]) {require(out.response[endpoint]==expected);tick();break;}
      tick();
    }
  }
};
Plan stimulus() {
  Plan p;p.add({});p.reset();
  const auto originalStart=p.oracle.edge;
  p.transfer(0,{5,3,0x1234,1});p.finish(0,{5,3,0,1});
  p.transfer(1,{5,3,0,2});p.finish(1,{5,3,0,2});
  p.transfer(2,{0,3,0,3});p.finish(2,{0,3,0x1234,3});
  require(p.oracle.edge-originalStart<64);
  // Distinct addresses, retained DRAM, repeated destination, nonzero old SRAM.
  p.transfer(0,{9,1,0xbeef,4});p.finish(0,{9,1,0,4});
  p.transfer(1,{9,3,0x7777,5});p.finish(1,{9,3,0x1234,5});
  p.transfer(2,{12,3,0x5555,6});p.finish(2,{12,3,0xbeef,6});
  // Saturate both RAM selected response paths and all seven queues. Finite
  // high-priority streams end, so delayed seed/copy writers must eventually run.
  std::array<std::deque<Packet>,3> offers;
  unsigned tag=10;
  for(unsigned i=0;i<14;++i)offers[0].push_back({i%16,(i+5)%16,0x4000+i,tag++});
  for(unsigned i=0;i<22;++i)offers[1].push_back({(i*3)%16,(i*5+3)%16,0xa000+i,tag++});
  for(unsigned i=0;i<24;++i)offers[2].push_back({(i+7)%16,(i*5+3)%16,0x9000+i,tag++});
  for(unsigned n=0; n<650; ++n) {
    Row r;r.take={n>=450,n>=370,n>=110};
    auto out=p.oracle.output();
    for(unsigned i=0;i<3;++i)if(!offers[i].empty()) {r.valid[i]=1;r.request[i]=offers[i].front();if(out.ready[i])offers[i].pop_front();}
    p.tick(r);
    if(n==10||n==90) {
      r.valid={0,0,0}; // Change offers without creating duplicate accepted tokens.
      r.clock=0;r.reset=1;p.add(r); // Held-low reset is not an edge.
      r.clock=1;r.reset=0;p.add(r);
      r.reset=1;p.add(r); // Held-high reset cancels nothing.
      r.clock=0;r.reset=0;p.add(r);
    }
    bool empty=true;for(auto&q:offers)empty&=q.empty();
    if(n>=450&&empty&&!p.oracle.pending())break;
    require(n<649);
  }
  p.drain();
  // Reset while a copied write has committed but its response remains pending.
  const unsigned old14=p.oracle.ram[0][14];
  p.transfer(0,{14,11,0x7654,100});p.finish(0,{14,11,old14,100});
  p.transfer(1,{14,11,0,101});
  for(unsigned n=0; !(p.oracle.jobs[1]&&p.oracle.jobs[1]->endpoint==2);++n){require(n<64);p.tick();}
  require(p.oracle.ram[1][11]==0x7654);
  p.reset();require(p.oracle.pending()==0);
  p.transfer(2,{6,11,0,102});p.finish(2,{6,11,0x7654,102});
  // Cancel occupied input/response queues and outstanding transactions.
  for(unsigned i=0;i<8;++i){Row r;r.valid={1,1,1};r.request={Packet{2,7,0x4567,110+i},Packet{9,4,0,130+i},Packet{0,3,0,150+i}};r.take={0,0,0};p.tick(r);}
  require(p.oracle.pending()>0);p.reset();p.drain();
  p.transfer(1,{14,11,0,180});p.finish(1,{14,11,0x7654,180});
  p.transfer(2,{0,11,0,181});p.finish(2,{0,11,0x7654,181});
  auto &o=p.oracle;
  for(unsigned q=0;q<7;++q)require(o.peak[q]==Oracle::capacity[q]);
  require(o.priority[0]>0&&o.priority[1]>0&&o.blocked[0]>4&&o.blocked[1]>4&&o.concurrent[0]>0);
  require(o.canceled>0&&o.resets>=3&&o.sent==o.retired+o.canceled);
  return p;
}
template<unsigned W>auto known(unsigned v){return gfsim::wire<gfsim::Bits<W>>::known(gfsim::Bits<W>{v});}
pyc_dut::Inputs inputs(const Row&r) {
  pyc_dut::Inputs p;p.pyc_7079635f636c6b=known<1>(r.clock);p.pyc_7079635f727374=known<1>(r.reset);
  p.seed_valid=known<1>(r.valid[0]);p.copy_valid=known<1>(r.valid[1]);p.check_valid=known<1>(r.valid[2]);
  p.seed=decltype(p.seed)::fromPacked(known<32>(r.request[0].packed()).packed());
  p.copy=decltype(p.copy)::fromPacked(known<32>(r.request[1].packed()).packed());
  p.check=decltype(p.check)::fromPacked(known<32>(r.request[2].packed()).packed());
  p.take_seed=known<1>(r.take[0]);p.take_copy=known<1>(r.take[1]);p.take_check=known<1>(r.take[2]);return p;
}
template<class Packed> void check(const Packed &actual,const Expected&e,unsigned row) {
  auto value=packed(e),mask=packed(e,true);
  for(unsigned i=0;i<110;++i)if(mask[109-i]=='1') {
    if(!actual.knownMask().bit(i)||actual.zMask().bit(i)||actual.value().bit(i)!=(value[109-i]=='1')) {
      std::cerr<<"DMA row "<<row<<" bit "<<i<<" expected "<<value<<'\n';require(false);
    }
  }
}
struct Context {
  pyc_dut &dut;const std::vector<Row>&rows;Oracle oracle;unsigned sampled=0;
  static void initialize(void*p){require(drive(p,0));}
  static bool drive(void*p,std::uint64_t epoch){auto&c=*static_cast<Context*>(p);if(epoch==c.rows.size())return false;require(epoch<c.rows.size());c.dut.drive(inputs(c.rows[epoch]));return true;}
  static void sample(void*p,std::uint64_t epoch){auto&c=*static_cast<Context*>(p);require(epoch==c.sampled+1);auto e=c.oracle.output();check(c.dut.sample().result.packed(),e,c.sampled);std::cout<<"WORK "<<c.sampled<<' '<<packed(e)<<'\n';c.oracle.commit(c.rows[c.sampled++]);}
};
void writeRows(const std::vector<Row>&rows) {
  std::ofstream file("dma-oracle.rows");require(bool(file));Oracle o;
  for(auto&r:rows) {file<<unsigned(r.clock)<<' '<<unsigned(r.reset);for(bool v:r.valid)file<<' '<<unsigned(v);for(auto p:r.request)file<<' '<<std::hex<<p.packed()<<std::dec;for(bool t:r.take)file<<' '<<unsigned(t);auto e=o.output();file<<' '<<packed(e)<<' '<<packed(e,true)<<'\n';o.commit(r);}
  require(bool(file));
}
void nativeRollback(unsigned workers,const std::vector<Row>&rows) {
  gfsim::WorkExecutor pool(workers);pyc_root root("dma-rollback",&pool);root.Build();
  auto drive=[&](const pyc_dut::Inputs&p){root.pyc_7079635f636c6b=p.pyc_7079635f636c6b;root.pyc_7079635f727374=p.pyc_7079635f727374;root.seed_valid=p.seed_valid;root.seed=p.seed;root.copy_valid=p.copy_valid;root.copy=p.copy;root.check_valid=p.check_valid;root.check=p.check;root.take_seed=p.take_seed;root.take_copy=p.take_copy;root.take_check=p.take_check;};
  drive(inputs({}));root.Reset();root.Xfer();Oracle o;unsigned probes=0;
  for(unsigned i=0;i<rows.size();++i) {
    const auto&r=rows[i];auto e=o.output();
    bool cross=r.clock&&!o.last&&!r.reset&&e.accepted[1]==2&&o.room(Oracle::SeedRequest)&&probes<3;
    if(cross) {
      drive(inputs(r));root.Work();check(root.result.packed(),e,i);root.DiscardNext();root.Xfer();
      auto bad=inputs(r);bad.seed_valid=gfsim::wire<gfsim::Bits<1>>::unknown();drive(bad);
      bool rejected=false;try{root.Work();}catch(const gfsim::FourStateViolation&){rejected=true;}
      require(rejected);root.DiscardNext();root.Xfer();++probes;
    }
    drive(inputs(r));root.Work();check(root.result.packed(),e,i);root.Xfer();o.commit(r);
  }
  require(probes==3);require(o.pending()==0);
  std::cout<<"FAILURE cross_ram_discard_failure_retry "<<probes<<'\n';
}
int main(int argc,char**argv) {
  gfsim::SystemRunner runner(argc,argv);if(!runner.ready())return 2;
  auto plan=stimulus();writeRows(plan.rows);pyc_dut dut(runner.workers());Context c{dut,plan.rows};
  gfsim::RunnerCallbacks callbacks{&c,&Context::initialize,&Context::drive,&Context::sample};
  int status=runner.Run(dut.system(),dut.observations(),{},callbacks);require(status==0&&c.sampled==plan.rows.size());
  nativeRollback(runner.workers(),plan.rows);
  std::cout<<"CASES original_global_deadline full_packet priority stalls all_queues concurrent_ram held_falling reset_retained "<<c.oracle.edge<<' '<<c.oracle.sent<<' '<<c.oracle.retired<<' '<<c.oracle.canceled<<'\n';
  return status;
}
