#include "gfsim/SimExecutor.h"
#include "gfsim/SystemRunner.h"
#include "pycircuit_system.hpp"
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <source_location>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
void require(bool ok,std::source_location at=std::source_location::current()){
  if(!ok){std::cerr<<"feedback independent oracle at "<<at.line()<<'\n';std::abort();}
}
constexpr std::uint64_t All=(std::uint64_t{1}<<36)-1,ValueMask=All^15;
struct Item{std::uint64_t value=0,known=All,z=0;bool computed=false;bool operator==(const Item &)const=default;};
Item item(std::uint32_t value=0,unsigned remaining=0){return{(std::uint64_t{value}<<4)|remaining,All,0};}
std::string visible(Item p){std::string s;for(unsigned b=36;b;--b)s+=(p.z>>(b-1))&1?'z':!((p.known>>(b-1))&1)?'x':(p.value>>(b-1))&1?'1':'0';return s;}
template<unsigned W>auto wire(std::uint64_t value,std::uint64_t known,std::uint64_t z){return gfsim::wire<gfsim::Bits<W>>::fromPacked(gfsim::FourState<W>::fromMasks(gfsim::Bits<W>{value},gfsim::Bits<W>{known},gfsim::Bits<W>{z}));}
auto bit(bool b){return wire<1>(b,1,0);}
Item advance(Item p){
  require((p.known&15)==15&&(p.z&15)==0&&(p.value&15)>0);
  const auto remaining=(p.value&15)-1;
  if((p.known&ValueMask)==ValueMask&&(p.z&ValueMask)==0)
    return item(static_cast<std::uint32_t>((p.value>>4)+1),remaining);
  return{remaining,15,0,true}; // Computed arithmetic-X value has no latent contract.
}
Item completed(Item p){
  const unsigned n=p.value&15;
  if(!n)return p;
  if((p.known&ValueMask)==ValueMask&&(p.z&ValueMask)==0)
    return item(static_cast<std::uint32_t>((p.value>>4)+n));
  return{0,15,0,true};
}
template<class P>void same(const P &p,Item e){
  // Transported X/Z uses all raw value bits; arithmetic-X has unspecified
  // latent values and no Z. The input ledger keeps which obligation applies.
  const auto mask=e.computed?e.known:All;
  require((p.value()&gfsim::Bits<36>{mask})==gfsim::Bits<36>{e.value&mask});
  require(p.knownMask()==gfsim::Bits<36>{e.known}&&p.zMask()==gfsim::Bits<36>{e.z});
}
std::vector<Item> vectors(bool four){
  std::vector<Item>a;
  if(!four){
    for(auto value:std::array<std::uint32_t,12>{0,1,15,16,255,256,0x7fffffff,0x80000000,0xffffffff,0xfffffffe,0x55555555,0xaaaaaaaa})
      for(unsigned n=0;n<16;++n)a.push_back(item(value,n));
    for(unsigned b=0;b<32;++b)for(bool invert:{false,true})for(unsigned n:{0u,15u})
      a.push_back(item((std::uint32_t{1}<<b)^(invert?UINT32_MAX:0),n));
  }else{
    for(unsigned b=0;b<32;++b)for(bool z:{false,true})for(bool latent:{false,true})for(unsigned n:{0u,1u,15u}){
      auto p=item(0xa55aa55a,n);const auto m=std::uint64_t{1}<<(b+4);
      p.value=(p.value&~m)|(latent?m:0);p.known^=m;p.z=z?m:0;a.push_back(p);
    }
    for(unsigned pattern=0;pattern<4;++pattern)for(bool latent:{false,true})for(unsigned n:{0u,1u,15u}){
      Item p{n,15,0};for(unsigned b=0;b<32;++b){char c=pattern==0?'x':pattern==1?'z':pattern==2?"xz"[b%2]:"01xz"[b%4];
        p.value|=std::uint64_t(c=='1'||((c=='x'||c=='z')&&latent))<<(b+4);
        p.known|=std::uint64_t(c=='0'||c=='1')<<(b+4);p.z|=std::uint64_t(c=='z')<<(b+4);}
      a.push_back(p);
    }
  }
  require(a.size()==(four?408u:320u));return a;
}
struct Row{bool clock=false,reset=false,valid=false,take=false;Item data;};
struct Token{unsigned id=0;Item data;std::uint64_t birth=0;unsigned original=0;};
struct Expected{bool ready,valid,pop,sourcePop,update,exit,feedbackPop;Token chosen;Item data;};
struct Golden{
  std::array<std::deque<Token>,3>q;bool clock=false;unsigned next=0;std::uint64_t edge=0;
  unsigned size()const{return q[0].size()+q[1].size()+q[2].size();}
  Expected read(const Row &r)const{
    bool fb=!q[1].empty(),available=fb||!q[0].empty(),pop=!q[2].empty()&&r.take,room=q[2].empty()||pop;
    Token chosen=fb?q[1].front():q[0].empty()?Token{}:q[0].front();
    require(!available||((chosen.data.known&15)==15&&(chosen.data.z&15)==0));
    bool continuing=available&&(chosen.data.value&15)>0,h=continuing||room;
    bool sourcePop=!fb&&h&&!q[0].empty();
    return{q[0].size()<2||sourcePop,!q[2].empty(),pop,sourcePop,available&&continuing,available&&!continuing&&room,fb&&h,chosen,q[2].empty()?item():q[2].front().data};
  }
  void commit(const Row &r){auto e=read(r);if(r.clock&&!clock){if(r.reset){for(auto &a:q)a.clear();edge=0;}else{
    if(e.pop)q[2].pop_front();if(e.feedbackPop)q[1].pop_front();
    if(e.update){auto t=e.chosen;t.data=advance(t.data);q[1].push_back(t);}if(e.exit)q[2].push_back(e.chosen);
    if(e.sourcePop)q[0].pop_front();if(r.valid&&e.ready)q[0].push_back({next++,r.data,edge,unsigned(r.data.value&15)});++edge;
  }require(q[0].size()<=2&&q[1].size()<=1&&q[2].size()<=1&&size()<=4);}clock=r.clock;}
};
std::vector<Row> stimulus(bool four){
  std::vector<Row>a;Golden g;
  auto row=[&](Row r){g.commit(r);a.push_back(r);};
  auto edge=[&](Item p=item(),bool valid=false,bool take=true,bool reset=false){Row r{false,reset,valid,take,p};row(r);r.clock=true;row(r);};
  auto drain=[&]{for(unsigned i=0;i<70;++i)edge();require(g.size()==0);};
  row({false,true});edge(item(),false,false,true);
  for(unsigned n=0;n<16;++n){const auto birth=g.edge;edge(item(UINT32_MAX-n,n),true);
    for(unsigned i=0;i<n+2;++i)edge();require(g.size()==0&&g.edge==birth+n+3);}
  auto fill=[&]{edge(item(10),true,false);edge(item(20,3),true,false);edge(item(30,1),true,false);edge(item(40,2),true,false);require(g.size()==4);};
  fill();edge(item(50,4),true,false);edge(item(60,4),true,false);
  // Continuation reaches zero while the old result remains blocked.
  edge(item(70),true,false);require(g.q[1].front().data==item(23));
  row({true,true,true,true,item(80)});row({true,false,true,false,item(90)});require(g.size()==4);
  edge(item(100),true,true);require(g.q[0].size()==2&&g.q[1].empty());
  // A resident exit cannot process the old source head or replace a full
  // source slot in its own edge. Processing resumes at the next edge.
  drain();fill();edge(item(),false,true,true);require(g.size()==0);drain();
  for(const auto &p:vectors(four)){unsigned n=p.value&15;const auto birth=g.edge;edge(p,true);
    for(unsigned i=0;i<n+2;++i)edge();require(g.size()==0&&g.edge==birth+n+3);}
  edge(item(UINT32_MAX,1),true);for(unsigned i=0;i<4;++i)edge();
  fill();edge(item(),true,true,true);drain();return a;
}
pyc_dut::Inputs ports(const Row &r){pyc_dut::Inputs p;static_assert(decltype(p.data)::width==36);
  p.pyc_7079635f636c6b=bit(r.clock);p.pyc_7079635f727374=bit(r.reset);p.valid=bit(r.valid);p.take=bit(r.take);
  p.data=decltype(p.data)::fromPacked(wire<36>(r.data.value,r.data.known,r.data.z).packed());return p;}
template<class P>void check(const P &p,const Expected &e){require(p.knownMask().bit(37)&&p.knownMask().bit(36)&&!p.zMask().bit(37)&&!p.zMask().bit(36));require(p.value().bit(37)==e.ready&&p.value().bit(36)==e.valid);same(gfsim::extract<36>(p,0),e.data);}
struct Context{
  pyc_dut &dut;std::vector<Row>rows;Golden g;std::deque<Token>ledger;
  unsigned sampled=0,accepted=0,retired=0,dropped=0,peak=0,blocked=0,blockedUpdates=0,busyExit=0,fullReset=0;
  void observe(std::string_view label){const auto &r=rows[sampled];auto e=g.read(r);const auto out=dut.sample().result.packed();static_assert(decltype(dut.sample().result)::width==38);check(out,e);
    std::cout<<label<<' '<<sampled<<' '<<out.value().bit(37)<<' '<<out.value().bit(36)<<' '<<visible(e.data)<<'\n';
    if(r.clock&&!g.clock){if(r.reset){fullReset+=g.size()==4;dropped+=ledger.size();ledger.clear();}else{
      blocked+=!g.q[2].empty()&&!r.take;blockedUpdates+=e.update&&!g.q[2].empty()&&!r.take;
      busyExit+=e.exit&&!g.q[1].empty()&&g.q[0].size()==2&&r.valid;
      if(out.value().bit(36)&&r.take){require(!ledger.empty());const auto t=ledger.front();same(gfsim::extract<36>(out,0),completed(t.data));require(g.edge>=t.birth+t.original+2);ledger.pop_front();++retired;}
      if(out.value().bit(37)&&r.valid)ledger.push_back({accepted++,r.data,g.edge,unsigned(r.data.value&15)});
    }}g.commit(r);peak=std::max(peak,g.size());require(ledger.size()==g.size());++sampled;
  }
  static void initialize(void *p){require(drive(p,0));}
  static bool drive(void *p,std::uint64_t epoch){auto &c=*static_cast<Context*>(p);if(epoch==c.rows.size())return false;require(epoch<c.rows.size());c.dut.drive(ports(c.rows[epoch]));return true;}
  static void sample(void *p,std::uint64_t epoch){auto &c=*static_cast<Context*>(p);require(epoch==c.sampled+1);c.observe("WORK");}
  void finish(){require(sampled==rows.size()&&g.size()==0&&ledger.empty()&&accepted==retired+dropped&&peak==4&&fullReset>=2&&blocked>=3&&blockedUpdates>=2&&busyExit>=1);std::cout<<"HISTORY "<<accepted<<' '<<retired<<' '<<dropped<<' '<<peak<<' '<<blocked<<' '<<blockedUpdates<<' '<<busyExit<<' '<<fullReset<<'\n';}
};
constexpr std::string_view config=R"({"deadlock_window":null,"max_domain_cycles":{},"max_ticks":12000,"schema":"pycircuit-model-config","version":"1"})";
void configure(gfsim::SimExecutor &e){require(e.ConfigureJson(reinterpret_cast<const std::uint8_t*>(config.data()),config.size())==PYCIRCUIT_MODEL_STATUS_V1_OK);}
void driveRoot(pyc_root &root,const Row &r){auto p=ports(r);root.pyc_7079635f636c6b=p.pyc_7079635f636c6b;root.pyc_7079635f727374=p.pyc_7079635f727374;root.valid=p.valid;root.data=p.data;root.take=p.take;}
void owner(unsigned workers,bool fail){
  gfsim::WorkExecutor pool(workers);pyc_root root("owner",&pool);root.Build();Golden g;Row r;
  driveRoot(root,r);root.Reset();root.Xfer();std::vector<Item>retirements;
  auto step=[&](Row x){driveRoot(root,x);root.Work();check(root.result.packed(),g.read(x));if(x.clock&&!g.clock&&x.take&&root.result.packed().value().bit(36))retirements.push_back(g.read(x).data);root.Xfer();g.commit(x);};
  auto edge=[&](Row x){x.clock=false;step(x);x.clock=true;step(x);};
  edge({false,false,true,false,item(10)});edge({false,false,true,false,item(20,2)});edge({false,false,true,false,item(30,1)});
  r={false,false,true,true,item(40,1)};step(r);r.clock=true;driveRoot(root,r);if(fail)root.valid=wire<1>(0,0,0);
  bool failed=false;try{root.Work();}catch(const gfsim::FourStateViolation &){failed=true;}require(failed==fail);root.DiscardNext();root.Xfer();
  r.valid=false;step(r);for(unsigned i=0;i<12;++i)edge({false,false,false,true});require(g.size()==0);
  require(retirements==std::vector<Item>{item(10),item(22),item(31)});
  edge({false,false,true,false,item(7,3)});root.Reset();root.DiscardNext();root.Xfer();retirements.clear();
  for(unsigned i=0;i<10;++i)edge({false,false,false,true});require(g.size()==0&&retirements==std::vector<Item>{item(10)});
  check(root.result.packed(),g.read({}));root.Reset();root.Xfer();g=Golden{};step({});
  std::cout<<"OWNER "<<fail<<" whole-three-owner zero-commit/discard/reset-retirement passed\n";
}
void terminal(unsigned workers){
  // Unknown remaining is safely queued behind a known resident feedback
  // token. It fails only when selected after that token exits; external data
  // changes cannot repair the queued command.
  for(unsigned base:{0u,15u})for(unsigned b=0;b<4;++b)for(bool z:{false,true})for(bool latent:{false,true}){
    pyc_dut dut(workers);gfsim::SimExecutor e(dut.system(),dut.observations(),{});configure(e);
    auto step=[&](Row x){dut.drive(ports(x));PycircuitModelStepResultV1 s{sizeof(s)};require(e.Step(&s)==PYCIRCUIT_MODEL_STATUS_V1_OK);};
    dut.drive(ports({}));require(e.Reset()==PYCIRCUIT_MODEL_STATUS_V1_OK);step({});step({true,false,true,true,item(7,2)});step({false,false,false,true});
    auto p=item(100,base);auto m=std::uint64_t{1}<<b;p.value=(p.value&~m)|(latent?m:0);p.known^=m;p.z=z?m:0;
    step({true,false,true,true,p});step({false,false,false,true});
    step({true,false,false,true});step({false,false,false,true});step({true,false,false,true});step({false,false,false,true});
    auto epoch=e.cycles();dut.drive(ports({true,false,false,true,item()}));PycircuitModelStepResultV1 s{sizeof(s)};
    require(e.Step(&s)==PYCIRCUIT_MODEL_STATUS_V1_RUNTIME_FAILURE&&s.state==PYCIRCUIT_MODEL_STEP_V1_FAILED&&e.cycles()==epoch&&dut.system().cycle()==epoch);
    require(e.Step(&s)==PYCIRCUIT_MODEL_STATUS_V1_INVALID_STATE);bool unavailable=false;try{(void)dut.sample();}catch(const std::logic_error &){unavailable=true;}require(unavailable);
    dut.drive(ports({}));require(e.Reset()==PYCIRCUIT_MODEL_STATUS_V1_OK);step({});step({true,false,true,true,item(UINT32_MAX,1)});
    step({false,false,false,true});step({true,false,false,true});step({false,false,false,true});step({true,false,false,true});step({false,false,false,true});
    require(dut.sample().result.packed().value().bit(36));same(gfsim::extract<36>(dut.sample().result.packed(),0),item());
    step({true,false,false,true});step({false,false,false,true});require(!dut.sample().result.packed().value().bit(36));
    std::cout<<"NEGATIVE remaining "<<base<<' '<<b<<' '<<z<<' '<<latent<<" deferred terminal; Reset execution recovery\n";
  }
  for(bool pop:{false,true}){
    pyc_dut dut(workers);gfsim::SimExecutor e(dut.system(),dut.observations(),{});configure(e);
    auto step=[&](Row x){dut.drive(ports(x));PycircuitModelStepResultV1 s{sizeof(s)};require(e.Step(&s)==PYCIRCUIT_MODEL_STATUS_V1_OK);};
    dut.drive(ports({}));require(e.Reset()==PYCIRCUIT_MODEL_STATUS_V1_OK);step({});
    if(pop){step({true,false,true,false,item(5)});step({false,false,false,false});step({true,false,false,false});step({false,false,false,false});}
    auto p=ports({true,false,false,pop});if(pop)p.take=wire<1>(0,0,0);else p.valid=wire<1>(0,0,0);dut.drive(p);auto epoch=e.cycles();PycircuitModelStepResultV1 s{sizeof(s)};
    require(e.Step(&s)==PYCIRCUIT_MODEL_STATUS_V1_RUNTIME_FAILURE&&s.state==PYCIRCUIT_MODEL_STEP_V1_FAILED&&e.cycles()==epoch);require(e.Step(&s)==PYCIRCUIT_MODEL_STATUS_V1_INVALID_STATE);
    dut.drive(ports({}));require(e.Reset()==PYCIRCUIT_MODEL_STATUS_V1_OK);step({});require(!dut.sample().result.packed().value().bit(36));
    std::cout<<"NEGATIVE protocol "<<pop<<" terminal; Reset recovery\n";
  }
}
int main(int argc,char **argv){std::string mode;std::vector<char*>args{argv[0]};for(int i=1;i<argc;++i)if(std::string_view(argv[i])=="--probe-mode"){require(i+1<argc&&mode.empty());mode=argv[++i];}else args.push_back(argv[i]);args.push_back(nullptr);gfsim::SystemRunner runner(static_cast<int>(args.size())-1,args.data());if(!runner.ready())return 2;
  if(!mode.empty()){require(mode=="terminal");terminal(runner.workers());return 0;}
  pyc_dut dut(runner.workers());Context c{dut,stimulus(false)};const gfsim::RunnerCallbacks callbacks{&c,&Context::initialize,&Context::drive,&Context::sample};int status=runner.Run(dut.system(),dut.observations(),{},callbacks);require(status==0);c.finish();
  pyc_dut four(runner.workers());gfsim::SimExecutor e(four.system(),four.observations(),{});configure(e);Context f{four,stimulus(true)};four.drive(ports(f.rows[0]));require(e.Reset()==PYCIRCUIT_MODEL_STATUS_V1_OK);
  for(auto &r:f.rows){four.drive(ports(r));PycircuitModelStepResultV1 s{sizeof(s)};require(e.Step(&s)==PYCIRCUIT_MODEL_STATUS_V1_OK);f.observe("FOUR");}f.finish();owner(runner.workers(),false);owner(runner.workers(),true);return status;
}
