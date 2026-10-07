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
void require(bool ok, std::source_location at = std::source_location::current()) {
  if (!ok) { std::cerr << "select independent oracle at " << at.line() << '\n'; std::abort(); }
}
constexpr std::uint64_t All = UINT64_MAX;
struct Planes {
  std::uint64_t value = 0, known = All, z = 0;
  bool operator==(const Planes &) const = default;
};
Planes data(std::uint64_t value = 0) { return {value,All,0}; }
Planes route(bool value = false) { return {value,1,0}; }
std::string visible(Planes p) {
  std::string s;
  for (unsigned b = 64; b; --b)
    s += (p.z >> (b-1)) & 1 ? 'z' : !((p.known >> (b-1)) & 1) ? 'x' :
         (p.value >> (b-1)) & 1 ? '1' : '0';
  return s;
}
template<unsigned W> auto wire(Planes p) {
  return gfsim::wire<gfsim::Bits<W>>::fromPacked(gfsim::FourState<W>::fromMasks(
      gfsim::Bits<W>{p.value},gfsim::Bits<W>{p.known},gfsim::Bits<W>{p.z}));
}
auto bit(bool v) { return wire<1>(route(v)); }
template<class P> void same(const P &p, Planes e) {
  require(p.value() == gfsim::Bits<64>{e.value} &&
          p.knownMask() == gfsim::Bits<64>{e.known} && p.zMask() == gfsim::Bits<64>{e.z});
}
Planes marker(unsigned n) { return data(0x8123456789abcdefULL ^ (std::uint64_t{n} * 0x0101010101010101ULL)); }
std::vector<Planes> vectors(bool four) {
  std::vector<Planes> a;
  if (!four) {
    for(unsigned b=0;b<64;++b) a.push_back(data(std::uint64_t{1}<<b));
    for(unsigned b=0;b<64;++b) a.push_back(data(All^(std::uint64_t{1}<<b)));
    for(unsigned length=0;length<=64;++length)
      a.push_back(data(length==64 ? All : (std::uint64_t{1}<<length)-1));
    for(auto v : std::array<std::uint64_t,8>{0ULL,All,0xfffffffffffffff0ULL,0x7fffffffffffffffULL,
                  0x8000000000000000ULL,0x8000000000000001ULL,0x5555555555555555ULL,0xaaaaaaaaaaaaaaaaULL})
      a.push_back(data(v));
    for(unsigned n=0;n<17;++n) a.push_back(marker(n));
  } else {
    for(unsigned b=0;b<64;++b)
      for(bool z : {false,true}) for(bool latent : {false,true}) {
        auto m=std::uint64_t{1}<<b;
        a.push_back({(0xa55aa55aa55aa55aULL&~m)|(latent?m:0),All^m,z?m:0});
      }
    for(unsigned pattern=0;pattern<4;++pattern) for(bool latent : {false,true}) {
      Planes p{0,0,0};
      for(unsigned b=0;b<64;++b) {
        char c=pattern==0?'x':pattern==1?'z':pattern==2?"xz"[b%2]:"01xz"[b%4];
        p.value |= std::uint64_t(c=='1'||((c=='x'||c=='z')&&latent))<<b;
        p.known |= std::uint64_t(c=='0'||c=='1')<<b; p.z |= std::uint64_t(c=='z')<<b;
      }
      a.push_back(p);
    }
  }
  require(a.size()==(four?264u:218u)); return a;
}
struct Row {
  bool clock=false, reset=false, cv=false, v0=false, v1=false, take=false;
  Planes control=route(), d0=data(), d1=data();
};
struct Token { std::uint64_t id=0,birth=0; Planes data; unsigned lane=0; std::uint64_t cid=0; };
struct Expected { std::array<bool,3> ready; bool valid,move,pop; unsigned selected; Planes data; };
struct Golden {
  std::array<std::deque<Token>,4> q;
  std::array<std::uint64_t,3> next{};
  bool clock=false; std::uint64_t edge=0;
  unsigned size() const { unsigned n=0; for(const auto &a:q)n+=a.size(); return n; }
  Expected read(const Row &r) const {
    const bool pop=!q[3].empty()&&r.take, room=q[3].size()<2||pop;
    unsigned selected=0; bool move=false;
    if(!q[0].empty()) {
      const auto p=q[0].front().data;
      if((p.known&1)&&!(p.z&1)) { selected=p.value&1; move=room&&!q[1+selected].empty(); }
      else require(!room||(q[1].empty()&&q[2].empty()));
    }
    return {{q[0].size()<2||move,q[1].size()<2||(move&&selected==0),q[2].size()<2||(move&&selected==1)},
            !q[3].empty(),move,pop,selected,q[3].empty()?data():q[3].front().data};
  }
  void commit(const Row &r) {
    const auto e=read(r);
    if(r.clock&&!clock) {
      if(r.reset) { for(auto &a:q)a.clear(); edge=0; }
      else {
        if(e.pop)q[3].pop_front();
        if(e.move) {
          auto t=q[1+e.selected].front(); t.cid=q[0].front().id; t.lane=e.selected;
          t.birth=std::max(t.birth,q[0].front().birth);
          q[3].push_back(t);q[0].pop_front();q[1+e.selected].pop_front();
        }
        const bool offered[3]={r.cv,r.v0,r.v1}; const Planes values[3]={r.control,r.d0,r.d1};
        for(unsigned i=0;i<3;++i) if(offered[i]&&e.ready[i])q[i].push_back({next[i]++,edge,values[i]});
        ++edge;
      }
      for(const auto &a:q)require(a.size()<=2);require(size()<=8);
    }
    clock=r.clock;
  }
};
std::vector<Row> stimulus(bool four) {
  std::vector<Row> rows; Golden g;
  auto row=[&](Row r){g.commit(r);rows.push_back(r);};
  auto edge=[&](bool cv,bool r,bool v0,bool v1,bool take,Planes a=data(),Planes b=data(),bool reset=false){
    Row x{false,reset,cv,v0,v1,take,route(r),a,b};row(x);x.clock=true;row(x);
  };
  auto drain=[&]{for(unsigned i=0;i<8;++i)edge(false,false,false,false,true);};
  row({false,true});edge(false,false,false,false,false,data(),data(),true);
  // Empty start: E0 captures, E1 joins old heads, E2 first retires.
  edge(true,false,true,false,true,marker(0));edge(false,false,false,false,true);edge(false,false,false,false,true);drain();
  // An available unselected lane cannot unblock two older route0 controls.
  edge(false,false,false,true,false,data(),marker(1));edge(false,false,false,true,false,data(),marker(2));
  edge(true,false,false,false,false);edge(true,false,false,false,false);edge(true,true,false,false,false);
  edge(false,true,true,false,true,marker(3));edge(false,true,false,false,true);
  edge(false,true,true,false,true,marker(4));edge(false,true,false,false,true);
  edge(true,true,false,false,true);edge(true,true,false,false,true);drain();require(g.size()==0);
  auto fill=[&]{
    edge(true,false,true,true,false,marker(10),marker(11));
    edge(true,true,true,true,false,marker(12),marker(13));
    edge(true,false,true,false,false,marker(14));
    edge(true,true,false,true,false,data(),marker(15));require(g.size()==8);
  };
  fill();edge(true,false,true,true,false,marker(16),marker(17));
  row({true,true,true,true,true,true,route(true),marker(18),marker(19)});
  row({true,false,true,true,true,false,route(false),marker(20),marker(21)});require(g.size()==8);
  for(unsigned i=0;i<12;++i)edge(true,i%2,true,true,true,marker(30+i),marker(50+i));
  drain();edge(true,false,false,false,true);edge(true,true,false,false,true);drain();require(g.size()==0);
  fill();edge(false,false,false,false,false,data(),data(),true);require(g.size()==0);
  unsigned n=0;
  for(const auto &p:vectors(four))for(bool selected:{false,true}) {
    // Two independently admitted lanes and two controls preserve both FIFO
    // orders. The first selection leaves the other lane untouched.
    auto a=selected?marker(100+n):p,b=selected?p:marker(100+n);
    edge(true,selected,true,true,true,a,b);edge(true,!selected,false,false,true);
    edge(false,false,false,false,true);edge(false,false,false,false,true);
    require(g.size()==0);++n;
  }
  fill();edge(false,false,false,false,true,data(),data(),true);drain();require(g.size()==0);
  return rows;
}
pyc_dut::Inputs ports(const Row &r) {
  pyc_dut::Inputs p;
  static_assert(decltype(p.control)::width==1 && decltype(p.lane0_data)::width==64 && decltype(p.lane1_data)::width==64);
  p.pyc_7079635f636c6b=bit(r.clock);p.pyc_7079635f727374=bit(r.reset);
  p.control_valid=bit(r.cv);p.control=decltype(p.control)::fromPacked(wire<1>(r.control).packed());
  p.lane0_valid=bit(r.v0);p.lane1_valid=bit(r.v1);p.take=bit(r.take);
  p.lane0_data=wire<64>(r.d0);p.lane1_data=wire<64>(r.d1);return p;
}
template<class P> void check(const P &p,const Expected &e) {
  for(unsigned i=0;i<4;++i)require(p.knownMask().bit(67-i)&&!p.zMask().bit(67-i));
  for(unsigned i=0;i<3;++i)require(p.value().bit(67-i)==e.ready[i]);
  require(p.value().bit(64)==e.valid);same(gfsim::extract<64>(p,0),e.data);
}
struct Context {
  pyc_dut &dut;std::vector<Row> rows;Golden golden;
  std::array<std::deque<Token>,3> admitted;std::deque<Token> joined;
  std::array<unsigned,3> accepted{},dropped{};std::array<unsigned,2> retired{};
  unsigned sampled=0,peak=0,replacements=0,blocked=0,hol=0,fullReset=0;
  void observe(std::string_view label) {
    const auto &r=rows[sampled];auto e=golden.read(r);const auto out=dut.sample().result.packed();
    static_assert(decltype(dut.sample().result)::width==68);check(out,e);
    std::cout<<label<<' '<<sampled<<' '<<out.value().bit(67)<<' '<<out.value().bit(66)<<' '<<out.value().bit(65)<<' '<<out.value().bit(64)<<' '<<visible(e.data)<<'\n';
    if(r.clock&&!golden.clock) {
      if(r.reset) {
        fullReset+=golden.size()==8;
        for(unsigned i=0;i<3;++i){dropped[i]+=admitted[i].size();admitted[i].clear();}
        for(auto &t:joined){++dropped[0];++dropped[1+t.lane];}joined.clear();
      } else {
        blocked+=golden.q[3].size()==2&&!r.take;
        hol+=!golden.q[0].empty()&&golden.q[1+e.selected].empty()&&golden.q[2-e.selected].size()==2;
        replacements+=e.move&&golden.q[0].size()==2&&golden.q[1+e.selected].size()==2&&golden.q[3].size()==2&&r.take;
        if(out.value().bit(64)&&r.take) {
          require(!joined.empty());const auto t=joined.front();same(gfsim::extract<64>(out,0),t.data);
          require(golden.edge>=t.birth+2);++retired[t.lane];joined.pop_front();
        }
        // Internal join is inferred from the independent old-slot model;
        // admission/retirement are qualified by actual public DUT handshakes.
        if(e.move) {
          require(!admitted[0].empty()&&!admitted[1+e.selected].empty());
          require((admitted[0].front().data.value&1)==e.selected);
          auto t=admitted[1+e.selected].front();t.cid=admitted[0].front().id;t.lane=e.selected;
          t.birth=std::max(t.birth,admitted[0].front().birth);joined.push_back(t);
          admitted[0].pop_front();admitted[1+e.selected].pop_front();
        }
        const bool offered[3]={r.cv,r.v0,r.v1};const Planes values[3]={r.control,r.d0,r.d1};
        for(unsigned i=0;i<3;++i)if(out.value().bit(67-i)&&offered[i])admitted[i].push_back({accepted[i]++,golden.edge,values[i]});
      }
    }
    golden.commit(r);peak=std::max(peak,golden.size());
    for(unsigned i=0;i<3;++i)require(admitted[i].size()==golden.q[i].size());require(joined.size()==golden.q[3].size());++sampled;
  }
  static void initialize(void *p){require(drive(p,0));}
  static bool drive(void *p,std::uint64_t epoch){auto &c=*static_cast<Context*>(p);if(epoch==c.rows.size())return false;require(epoch<c.rows.size());c.dut.drive(ports(c.rows[epoch]));return true;}
  static void sample(void *p,std::uint64_t epoch){auto &c=*static_cast<Context*>(p);require(epoch==c.sampled+1);c.observe("WORK");}
  void finish(){require(sampled==rows.size()&&golden.size()==0&&peak==8&&fullReset>=2&&blocked>=2&&hol>=2&&replacements>=8);
    require(accepted[0]==retired[0]+retired[1]+dropped[0]);
    for(unsigned i=0;i<2;++i)require(accepted[1+i]==retired[i]+dropped[1+i]);
    std::cout<<"HISTORY "<<accepted[0]<<' '<<accepted[1]<<' '<<accepted[2]<<' '<<retired[0]<<' '<<retired[1]<<' '<<dropped[0]<<' '<<dropped[1]<<' '<<dropped[2]<<' '<<peak<<' '<<replacements<<' '<<blocked<<' '<<hol<<' '<<fullReset<<'\n';}
};
constexpr std::string_view config=R"({"deadlock_window":null,"max_domain_cycles":{},"max_ticks":6000,"schema":"pycircuit-model-config","version":"1"})";
void configure(gfsim::SimExecutor &e){require(e.ConfigureJson(reinterpret_cast<const std::uint8_t*>(config.data()),config.size())==PYCIRCUIT_MODEL_STATUS_V1_OK);}
void driveRoot(pyc_root &root,const Row &r){auto p=ports(r);root.pyc_7079635f636c6b=p.pyc_7079635f636c6b;root.pyc_7079635f727374=p.pyc_7079635f727374;root.control_valid=p.control_valid;root.control=p.control;root.lane0_valid=p.lane0_valid;root.lane1_valid=p.lane1_valid;root.lane0_data=p.lane0_data;root.lane1_data=p.lane1_data;root.take=p.take;}
void owner(unsigned workers,bool fail){
  gfsim::WorkExecutor pool(workers);pyc_root root("owner",&pool);root.Build();Golden g;
  Row r;driveRoot(root,r);root.Reset();root.Xfer();
  auto step=[&](Row x){driveRoot(root,x);root.Work();check(root.result.packed(),g.read(x));root.Xfer();g.commit(x);};
  auto edge=[&](Row x){x.clock=false;step(x);x.clock=true;step(x);};
  edge({false,false,true,true,true,false,route(false),marker(200),marker(201)});
  edge({false,false,true,true,false,false,route(true),marker(202),data()});
  r={false,false,true,true,true,true,route(false),marker(203),marker(204)};step(r);r.clock=true;driveRoot(root,r);
  if(fail)root.lane1_valid=wire<1>({0,0,0});
  bool failed=false;try{root.Work();}catch(const gfsim::FourStateViolation &){failed=true;}require(failed==fail);
  root.DiscardNext();root.Xfer();
  // Neither clock nor any of the four queue owners committed. Reprepare the
  // same edge with no new offers; the old result and selected pair remain.
  r.cv=r.v0=r.v1=false;step(r);
  for(unsigned i=0;i<3;++i)edge({false,false,false,false,false,true});
  edge({false,false,true,false,false,true,route(false)});
  for(unsigned i=0;i<4;++i)edge({false,false,false,false,false,true});require(g.size()==0);
  // A discarded host reset also preserves state; a successful one clears it.
  edge({false,false,true,true,false,false,route(false),marker(205)});
  root.Reset();root.DiscardNext();root.Xfer();step({false,false,false,false,false,false});
  // Empty output and available input slots alone cannot prove that the hidden
  // D2 heads survived a discarded reset. Join and retire the saved token,
  // then drain additional edges to reject loss, duplicates and stale heads.
  unsigned retainedRetirements=0;
  for(unsigned i=0;i<4;++i){
    Row x{false,false,false,false,false,true};step(x);x.clock=true;step(x);
    const auto out=root.result.packed();
    if(out.value().bit(64)){
      same(gfsim::extract<64>(out,0),marker(205));++retainedRetirements;
    }
  }
  require(retainedRetirements==1&&g.size()==0);
  check(root.result.packed(),g.read({}));
  root.Reset();root.Xfer();g=Golden{};step({});
  std::cout<<"OWNER "<<fail<<" whole-four-owner discard/reprepare/reset passed\n";
}
void safe(unsigned workers){
  for(unsigned mode=0;mode<2;++mode)for(bool z:{false,true})for(bool latent:{false,true}){
    gfsim::WorkExecutor pool(workers);pyc_root root("safe",&pool);root.Build();Golden g;Row r;
    driveRoot(root,r);root.Reset();root.Xfer();
    auto step=[&](Row x){driveRoot(root,x);root.Work();check(root.result.packed(),g.read(x));root.Xfer();g.commit(x);};
    auto edge=[&](Row x){x.clock=false;step(x);x.clock=true;step(x);};
    if(mode==0){
      edge({false,false,true,true,false,false,route(false),marker(300)});
      edge({false,false,true,true,false,false,route(false),marker(301)});
      edge({false,false,false,false,false,false});require(g.q[3].size()==2);
    }
    r={false,false,true,false,false,false,{latent?1u:0u,0,z?1u:0u}};edge(r);
    // Safe table row: Ro0, or both lanes empty. In the latter case an
    // independent lane push is allowed before the next effective failure.
    r={false,false,false,true,true,false,route(false),marker(302),marker(303)};edge(r);
    root.Reset();root.Xfer();g=Golden{};step({});
    std::cout<<"SAFE "<<mode<<' '<<z<<' '<<latent<<" masked selector and independent pushes passed\n";
  }
}
void terminal(unsigned workers){
  for(unsigned lanes=1;lanes<=4;++lanes)for(bool z:{false,true})for(bool latent:{false,true}){
    pyc_dut dut(workers);gfsim::SimExecutor e(dut.system(),dut.observations(),{});configure(e);
    auto step=[&](Row x){dut.drive(ports(x));PycircuitModelStepResultV1 s{sizeof(s)};require(e.Step(&s)==PYCIRCUIT_MODEL_STATUS_V1_OK);};
    dut.drive(ports({}));require(e.Reset()==PYCIRCUIT_MODEL_STATUS_V1_OK);step({});
    Row r{true,false,true,false,false,false,{latent?1u:0u,0,z?1u:0u},marker(400),lanes==4?marker(400):marker(401)};step(r);
    r.clock=false;r.cv=false;r.control=route(false);step(r);
    r.clock=true;r.v0=lanes!=2;r.v1=lanes!=1;step(r); // Both old lanes empty: independent pushes commit safely.
    r={false,false,false,false,false,false,route(false)};step(r);const auto epoch=e.cycles();
    // Changing external route to known0 cannot repair the queued X/Z head.
    r.clock=true;dut.drive(ports(r));PycircuitModelStepResultV1 status{sizeof(status)};
    require(e.Step(&status)==PYCIRCUIT_MODEL_STATUS_V1_RUNTIME_FAILURE&&status.state==PYCIRCUIT_MODEL_STEP_V1_FAILED&&e.cycles()==epoch&&dut.system().cycle()==epoch);
    require(e.Step(&status)==PYCIRCUIT_MODEL_STATUS_V1_INVALID_STATE);
    bool unavailable=false;try{(void)dut.sample();}catch(const std::logic_error &){unavailable=true;}require(unavailable);
    dut.drive(ports({}));require(e.Reset()==PYCIRCUIT_MODEL_STATUS_V1_OK);step({});
    step({true,false,true,true,false,true,route(false),marker(402)});step({false,false,false,false,false,true});
    step({true,false,false,false,false,true});step({false,false,false,false,false,true});
    require(dut.sample().result.packed().value().bit(64));same(gfsim::extract<64>(dut.sample().result.packed(),0),marker(402));
    step({true,false,false,false,false,true});step({false,false,false,false,false,true});require(!dut.sample().result.packed().value().bit(64));
    std::cout<<"NEGATIVE "<<lanes<<' '<<z<<' '<<latent<<" queued unknown selector terminal; Reset execution recovery\n";
  }
}
int main(int argc,char **argv){
  std::string mode;std::vector<char*> arguments{argv[0]};
  for(int i=1;i<argc;++i)if(std::string_view(argv[i])=="--probe-mode"){require(i+1<argc&&mode.empty());mode=argv[++i];}else arguments.push_back(argv[i]);
  arguments.push_back(nullptr);gfsim::SystemRunner runner(static_cast<int>(arguments.size())-1,arguments.data());if(!runner.ready())return 2;
  if(!mode.empty()){require(mode=="terminal");terminal(runner.workers());return 0;}
  pyc_dut dut(runner.workers());Context c{dut,stimulus(false)};const gfsim::RunnerCallbacks callbacks{&c,&Context::initialize,&Context::drive,&Context::sample};
  const int status=runner.Run(dut.system(),dut.observations(),{},callbacks);require(status==0);c.finish();
  pyc_dut four(runner.workers());gfsim::SimExecutor e(four.system(),four.observations(),{});configure(e);Context f{four,stimulus(true)};
  four.drive(ports(f.rows[0]));require(e.Reset()==PYCIRCUIT_MODEL_STATUS_V1_OK);
  for(const auto &r:f.rows){four.drive(ports(r));PycircuitModelStepResultV1 s{sizeof(s)};require(e.Step(&s)==PYCIRCUIT_MODEL_STATUS_V1_OK);f.observe("FOUR");}f.finish();
  owner(runner.workers(),false);owner(runner.workers(),true);safe(runner.workers());return status;
}
