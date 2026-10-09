#include "gfsim/SystemRunner.h"
#include "pycircuit_system.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <cstdlib>
#include <deque>
#include <fstream>
#include <iostream>
#include <optional>
#include <source_location>
#include <string>
#include <vector>

void require(bool condition, std::source_location at=std::source_location::current()) {
  if(!condition) {std::cerr<<"memory_banks independent oracle line "<<at.line()<<'\n';std::abort();}
}
struct Packet {
  unsigned bank=0,offset=0,write=0,data=0,tag=0;
  std::uint32_t packed() const {return (bank<<29)|(offset<<25)|(write<<24)|(data<<8)|tag;}
  bool operator==(const Packet&) const=default;
};
struct Row {bool clock=0,reset=0,valid=0;Packet request{};bool take=1;};
struct Expected {
  bool ready=0,valid=0;
  Packet response{};
  unsigned route=0,accepted=0,enqueued=0,responseValid=0,merged=0;
  std::uint64_t packed() const {
    return (std::uint64_t(ready)<<52)|(std::uint64_t(valid)<<51)|
      (std::uint64_t(valid?response.packed():0)<<20)|(route<<16)|
      (accepted<<12)|(enqueued<<8)|(responseValid<<4)|merged;
  }
  std::uint64_t mask() const {return (3ULL<<51)|((valid?0x7fffffffULL:0)<<20)|0xfffffULL;}
};
// Root Q8, four request Q2, four response Q2, merge Q2. Each bank has its
// own RAM and at most one job, whose response deadline is an absolute edge.
// Record old data and apply writes at acceptance; never model DUT countdowns.
struct Oracle {
  static constexpr unsigned Root=0,Merge=9;
  static constexpr std::array<unsigned,10> capacity{8,2,2,2,2,2,2,2,2,2};
  struct Job {Packet response;std::uint64_t due;};
  std::array<std::deque<Packet>,10> queues;
  std::array<std::array<unsigned,16>,4> ram{};
  std::array<std::optional<Job>,4> jobs;
  std::array<std::deque<Packet>,4> completionOrder;
  std::array<unsigned,10> peak{};
  std::array<unsigned,4> blocked{},acceptedCount{},completedCount{};
  std::array<bool,256> seen{},retiredTags{};
  std::vector<Packet> retiredPackets;
  unsigned sent=0,retired=0,canceled=0,resets=0,held=0,hol=0,multiHeadMerge=0,
    fourBusy=0,multiAccepted=0,multiEnqueued=0,rootStalls=0;
  std::uint64_t edge=0;
  bool last=0;
  bool room(unsigned q) const {return queues[q].size()<capacity[q];}
  Expected output() const {
    Expected e;e.ready=room(Root);e.valid=!queues[Merge].empty();
    if(e.valid)e.response=queues[Merge].front();
    if(!queues[Root].empty()) {unsigned b=queues[Root].front().bank;if(room(1+b))e.route=1U<<b;}
    for(unsigned b=0;b<4;++b) {
      if(!jobs[b]&&!queues[1+b].empty())e.accepted|=1U<<b;
      if(jobs[b]&&edge>=jobs[b]->due&&room(5+b))e.enqueued|=1U<<b;
      if(!queues[5+b].empty())e.responseValid|=1U<<b;
    }
    if(room(Merge)&&e.responseValid)e.merged=e.responseValid&(~e.responseValid+1);
    return e;
  }
  unsigned pending() const {
    unsigned n=0;for(auto&q:queues)n+=q.size();for(auto&j:jobs)n+=unsigned(bool(j));return n;
  }
  void commit(const Row&r) {
    if(r.clock&&!last) {
      auto e=output();
      if(r.reset) {
        canceled+=pending();for(auto&q:queues)q.clear();jobs={};for(auto&q:completionOrder)q.clear();++resets;
      } else {
        const auto old=queues;
        rootStalls+=r.valid&&!e.ready;
        multiAccepted+=std::popcount(e.accepted)>1;
        multiEnqueued+=std::popcount(e.enqueued)>1;
        if(!old[Root].empty()&&!e.route) {
          for(unsigned i=1;i<old[Root].size();++i)
            if(old[Root][i].bank!=old[Root].front().bank&&room(1+old[Root][i].bank)) {++hol;break;}
        }
        if(e.valid&&r.take) {
          auto p=e.response;require(!retiredTags[p.tag]);retiredTags[p.tag]=1;
          require(!completionOrder[p.bank].empty()&&completionOrder[p.bank].front()==p);
          completionOrder[p.bank].pop_front();queues[Merge].pop_front();retiredPackets.push_back(p);++retired;
        }
        if(e.merged) {
          unsigned b=std::countr_zero(e.merged);
          require(e.merged==(1U<<b));
          if(std::popcount(e.responseValid)>1)++multiHeadMerge;
          queues[Merge].push_back(old[5+b].front());queues[5+b].pop_front();
        }
        for(unsigned b=0;b<4;++b) {
          if(e.accepted&(1U<<b)) {
            require(!jobs[b]&&!(e.enqueued&(1U<<b)));
            auto p=old[1+b].front();queues[1+b].pop_front();unsigned oldData=ram[b][p.offset];
            if(p.write)ram[b][p.offset]=p.data;
            p.data=oldData;jobs[b]=Job{p,edge+2};completionOrder[b].push_back(p);++acceptedCount[b];
          } else if(e.enqueued&(1U<<b)) {
            require(jobs[b].has_value());queues[5+b].push_back(jobs[b]->response);jobs[b].reset();++completedCount[b];
          } else if(jobs[b]&&edge>=jobs[b]->due)++blocked[b];
        }
        if(e.route) {unsigned b=std::countr_zero(e.route);queues[1+b].push_back(old[Root].front());queues[Root].pop_front();}
        if(r.valid&&e.ready) {require(r.request.tag<256&&!seen[r.request.tag]);seen[r.request.tag]=1;queues[Root].push_back(r.request);++sent;}
        unsigned busy=0;for(auto&j:jobs)busy+=unsigned(bool(j));fourBusy+=busy==4;
        for(unsigned q=0;q<10;++q) {peak[q]=std::max(peak[q],unsigned(queues[q].size()));require(queues[q].size()<=capacity[q]);}
        require(sent==retired+canceled+pending());require(pending()<=30);
      }
      ++edge;
    } else ++held;
    last=r.clock;
  }
};
std::string binary(std::uint64_t value,unsigned width) {
  std::string s;for(unsigned b=width;b;--b)s+=((value>>(b-1))&1)?'1':'0';return s;
}
struct Plan {
  Oracle oracle;std::vector<Row> rows;
  void add(Row r) {rows.push_back(r);oracle.commit(r);}
  void tick(Row r={}) {r.clock=1;add(r);r.clock=0;add(r);}
  void reset() {Row r;r.reset=1;tick(r);}
  void drain() {for(unsigned n=0;oracle.pending();++n){require(n<1000);tick();}}
  void send(Packet p) {unsigned n=0;while(!oracle.output().ready){require(n++<1000);tick();}Row r;r.valid=1;r.request=p;tick(r);}
  Packet retiredPacket(unsigned tag) const {for(auto p:oracle.retiredPackets)if(p.tag==tag)return p;require(false);return {};}
};
Plan stimulus() {
  Plan p;p.add({});p.reset();
  // Original five complete requests enter through public ready/valid.
  const std::array<Packet,5> original{{{0,3,1,41,1},{1,3,1,91,2},{0,3,0,0,3},{1,3,0,0,4},{2,3,0,0,5}}};
  for(auto request:original)p.send(request);p.drain();
  constexpr unsigned oldData[]{0,0,41,91,0};
  require(p.oracle.retiredPackets.size()==5);
  for(unsigned i=0;i<5;++i){auto expected=original[i];expected.data=oldData[i];require(p.retiredPacket(i+1)==expected);}
  // Bank3, equal offsets in all four physical RAMs, and full field retention.
  p.send({3,3,1,0x3333,6});p.send({3,3,0,0xffff,7});
  for(unsigned b=0;b<3;++b)p.send({b,3,0,0xaaaa,8+b});p.drain();
  require(p.retiredPacket(6)==Packet{3,3,1,0,6});
  require(p.retiredPacket(7)==Packet{3,3,0,0x3333,7});
  require(p.retiredPacket(8).data==41&&p.retiredPacket(9).data==91&&p.retiredPacket(10).data==0);
  // A long stopped sink saturates all queues and controllers. Round-robin
  // offers reach a full selected bank and cannot bypass the old root head.
  std::deque<Packet> offers;
  for(unsigned i=0;i<84;++i)offers.push_back({i%4,(i/4)%16,unsigned(i%3!=1),0x5000+i,20+i});
  for(unsigned n=0;n<650;++n) {
    Row r;r.take=n>=180;
    if(!offers.empty()){r.valid=1;r.request=offers.front();if(p.oracle.output().ready)offers.pop_front();}
    p.tick(r);
    if(n==12||n==95) {
      r.valid=0;r.clock=0;r.reset=1;p.add(r); // No reset on a held-low level.
      r.clock=1;r.reset=0;p.add(r);
      r.reset=1;p.add(r); // No reset on a held-high level.
      r.clock=0;r.reset=0;p.add(r);
    }
    if(n>=180&&offers.empty()&&!p.oracle.pending())break;
    require(n<649);
  }
  p.drain();
  // Commit one write in each bank, cancel all four pending responses, then
  // independently read back the newly written retained values.
  for(unsigned b=0;b<4;++b)p.send({b,15,1,0x8100+b,120+b});
  for(unsigned n=0;;++n) {require(n<64);bool ready=true;for(unsigned b=0;b<4;++b)ready&=p.oracle.ram[b][15]==0x8100+b;if(ready)break;Row r;r.take=0;p.tick(r);}
  require(p.oracle.pending()>0);p.reset();require(p.oracle.pending()==0);
  for(unsigned b=0;b<4;++b)p.send({b,15,0,0x1234,124+b});p.drain();
  for(unsigned b=0;b<4;++b)require(p.retiredPacket(124+b)==Packet{b,15,0,0x8100+b,124+b});
  // Saturate once more and reset every queue class plus all four outstanding
  // banks. A second drain must emit none of the canceled tags.
  offers.clear();for(unsigned i=0;i<40;++i)offers.push_back({i%4,5,1,0x9000+i,150+i});
  for(unsigned n=0;n<100;++n){Row r;r.take=0;if(!offers.empty()){r.valid=1;r.request=offers.front();if(p.oracle.output().ready)offers.pop_front();}p.tick(r);}
  for(unsigned q=0;q<10;++q)require(p.oracle.queues[q].size()==Oracle::capacity[q]);
  for(auto&j:p.oracle.jobs)require(j.has_value());
  p.reset();p.drain();
  for(unsigned b=0;b<4;++b){unsigned expected=p.oracle.ram[b][5];p.send({b,5,0,0,200+b});p.drain();require(p.retiredPacket(200+b)==Packet{b,5,0,expected,200+b});}
  auto&o=p.oracle;
  for(unsigned q=0;q<10;++q)require(o.peak[q]==Oracle::capacity[q]);
  for(unsigned b=0;b<4;++b)require(o.blocked[b]>4&&o.acceptedCount[b]>0&&o.completedCount[b]>0);
  require(o.hol>0&&o.multiHeadMerge>0&&o.fourBusy>0&&o.multiAccepted>0&&o.multiEnqueued>0&&o.rootStalls>0);
  require(o.canceled>0&&o.resets>=3&&o.sent==o.retired+o.canceled&&o.pending()==0);
  return p;
}
template<unsigned W>auto known(unsigned v){return gfsim::wire<gfsim::Bits<W>>::known(gfsim::Bits<W>{v});}
pyc_dut::Inputs inputs(const Row&r) {
  pyc_dut::Inputs p;p.pyc_7079635f636c6b=known<1>(r.clock);p.pyc_7079635f727374=known<1>(r.reset);
  p.valid=known<1>(r.valid);p.request=decltype(p.request)::fromPacked(known<31>(r.request.packed()).packed());p.take=known<1>(r.take);return p;
}
template<class Packed>void check(const Packed&actual,const Expected&e,unsigned row) {
  const auto value=e.packed(),mask=e.mask();
  for(unsigned b=0;b<53;++b)if((mask>>b)&1)
    if(!actual.knownMask().bit(b)||actual.zMask().bit(b)||actual.value().bit(b)!=bool((value>>b)&1)) {
      std::cerr<<"memory_banks row "<<row<<" bit "<<b<<" expected "<<binary(value,53)<<'\n';require(false);
    }
}
struct Context {
  pyc_dut&dut;const std::vector<Row>&rows;Oracle oracle;unsigned sampled=0;
  static void initialize(void*p){require(drive(p,0));}
  static bool drive(void*p,std::uint64_t epoch){auto&c=*static_cast<Context*>(p);if(epoch==c.rows.size())return false;require(epoch<c.rows.size());c.dut.drive(inputs(c.rows[epoch]));return true;}
  static void sample(void*p,std::uint64_t epoch){auto&c=*static_cast<Context*>(p);require(epoch==c.sampled+1);auto e=c.oracle.output();check(c.dut.sample().result.packed(),e,c.sampled);std::cout<<"WORK "<<c.sampled<<' '<<binary(e.packed(),53)<<'\n';c.oracle.commit(c.rows[c.sampled++]);}
};
void writeRows(const std::vector<Row>&rows) {
  std::ofstream file("memory-banks-oracle.rows");require(bool(file));Oracle o;
  for(auto&r:rows){auto e=o.output();file<<unsigned(r.clock)<<' '<<unsigned(r.reset)<<' '<<unsigned(r.valid)<<' '<<std::hex<<r.request.packed()<<std::dec<<' '<<unsigned(r.take)<<' '<<binary(e.packed(),53)<<' '<<binary(e.mask(),53)<<'\n';o.commit(r);}require(bool(file));
}
void nativeRollback(unsigned workers,const std::vector<Row>&rows) {
  gfsim::WorkExecutor pool(workers);pyc_root root("banks-rollback",&pool);root.Build();
  auto drive=[&](const pyc_dut::Inputs&p){root.pyc_7079635f636c6b=p.pyc_7079635f636c6b;root.pyc_7079635f727374=p.pyc_7079635f727374;root.valid=p.valid;root.request=p.request;root.take=p.take;};
  drive(inputs({}));root.Reset();root.Xfer();Oracle o;std::vector<unsigned> probeTags;
  for(unsigned i=0;i<rows.size();++i) {
    const auto&r=rows[i];auto e=o.output();unsigned writeTag=0;
    if(r.clock&&!o.last&&!r.reset&&e.merged&&e.valid&&r.take&&probeTags.size()<3)
      for(unsigned b=0;b<4;++b)if(e.accepted&(1U<<b)) {
        const auto&p=o.queues[1+b].front();if(p.write&&p.data!=o.ram[b][p.offset])writeTag=p.tag;
      }
    if(writeTag) {
      drive(inputs(r));root.Work();check(root.result.packed(),e,i);root.DiscardNext();root.Xfer();
      auto bad=inputs(r);bad.take=gfsim::wire<gfsim::Bits<1>>::unknown();drive(bad);
      bool rejected=false;try{root.Work();}catch(const gfsim::FourStateViolation&){rejected=true;}
      require(rejected);root.DiscardNext();root.Xfer();probeTags.push_back(writeTag);
    }
    drive(inputs(r));root.Work();check(root.result.packed(),e,i);root.Xfer();o.commit(r);
  }
  require(probeTags.size()==3);for(unsigned tag:probeTags)require(o.retiredTags[tag]);require(o.pending()==0);
  std::cout<<"FAILURE merge_prepared_discard_unknown_take_retry "<<probeTags.size()<<'\n';
}
int main(int argc,char**argv) {
  gfsim::SystemRunner runner(argc,argv);if(!runner.ready())return 2;
  auto plan=stimulus();writeRows(plan.rows);pyc_dut dut(runner.workers());Context c{dut,plan.rows};
  gfsim::RunnerCallbacks callbacks{&c,&Context::initialize,&Context::drive,&Context::sample};
  int status=runner.Run(dut.system(),dut.observations(),{},callbacks);require(status==0&&c.sampled==plan.rows.size());
  nativeRollback(runner.workers(),plan.rows);
  std::cout<<"CASES original5 full_packet unique_tags per_bank_order four_ram hol priority capacity26 stall hold reset_retained "<<c.oracle.edge<<' '<<c.oracle.sent<<' '<<c.oracle.retired<<' '<<c.oracle.canceled<<'\n';
  return status;
}
