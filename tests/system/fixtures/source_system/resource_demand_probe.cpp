#include "pycircuit_system.hpp"
#include <iostream>
#include <stdexcept>
#include <type_traits>
#include <vector>
using Bit = gfsim::wire<gfsim::Bits<1>>;
static Bit four(unsigned s) {
  if (s < 2) return Bit::known(gfsim::Bits<1>{s});
  return Bit::fromPacked(s == 2 ? gfsim::FourState<1>::unknown()
                               : gfsim::FourState<1>::highImpedance());
}
static void require(bool ok, const char *message) {
  if (!ok) throw std::runtime_error(message);
}
template<class Storage, class Output>
void inject(Storage &storage, Output &output, unsigned s) {
  using Current = std::remove_cvref_t<decltype(storage->current(0))>;
  auto &state = const_cast<Current &>(storage->current(0));
  state.q = four(s); output.element(0) = state.q;
}
template<class Storage> void seed(Storage &storage, unsigned count) {
  using Current = std::remove_cvref_t<decltype(storage->current(0))>;
  auto &q = const_cast<Current &>(storage->current(0));
  q.count=count; q.rd=q.wr=0;
  q.storage[0]=gfsim::wire<gfsim::Bits<8>>::known(gfsim::Bits<8>{11});
}
int main() {
  unsigned cases=0;
  for (unsigned workers : {1u,2u}) {
    pyc_dut dut(workers);
    auto drive=[&](bool clk,bool rst){dut.drive({four(clk),four(rst)});};
    drive(false,false); dut.system().Build();
    auto root=dut.root_->pyc_implementation;
    auto selector=[&](unsigned v){inject(root->pyc_instance_selector_state,root->pyc_instance_selector_q,v);};
    auto early=[&](unsigned v){inject(root->pyc_instance_early_state,root->pyc_instance_early_q,v);};
    auto later=[&](unsigned v){inject(root->pyc_instance_later_state,root->pyc_instance_later_q,v);};
    auto sendLeft=[&](unsigned v){inject(root->pyc_instance_send_left_state,root->pyc_instance_send_left_q,v);};
    auto sendRight=[&](unsigned v){inject(root->pyc_instance_send_right_state,root->pyc_instance_send_right_q,v);};
    auto counts=[&](){return std::vector<std::uint64_t>{root->pyc_queue_left_state->current(0).count,
      root->pyc_queue_right_state->current(0).count,root->pyc_queue_pyc_6f7574707574_state->current(0).count,
      root->pyc_instance_count_state->current(0).q.value().value()};};
    auto snapshot=[&](){auto result=counts();
      auto addQueue=[&](const auto &storage){const auto &q=storage->current(0);
        result.insert(result.end(),{q.rd,q.wr,q.clock,q.initialized,q.storage[0].packed().value().value(),q.storage[0].packed().knownMask().value(),q.storage[0].packed().zMask().value()});};
      auto addState=[&](const auto &storage){const auto &q=storage->current(0);
        result.insert(result.end(),{q.q.packed().value().value(),q.q.packed().knownMask().value(),q.q.packed().zMask().value(),q.clock});};
      addQueue(root->pyc_queue_left_state);addQueue(root->pyc_queue_right_state);addQueue(root->pyc_queue_pyc_6f7574707574_state);
      addState(root->pyc_instance_count_state);addState(root->pyc_instance_selector_state);
      addState(root->pyc_instance_early_state);addState(root->pyc_instance_later_state);
      addState(root->pyc_instance_send_left_state);addState(root->pyc_instance_send_right_state);addState(root->pyc_instance_take_state);
      return result;};
    auto start=[&](unsigned timing,unsigned s,unsigned l,unsigned r,unsigned o){
      drive(false,false);dut.system().Reset();
      if(timing==2||timing==3){drive(true,true);require(dut.system().Step()==gfsim::SimStepResult::Running,"setup history");}
      seed(root->pyc_queue_left_state,l);seed(root->pyc_queue_right_state,r);seed(root->pyc_queue_pyc_6f7574707574_state,o);
      selector(s);early(1);later(1);sendLeft(0);sendRight(0);
      drive(timing==1||timing==2||timing==4,timing==4);};
    auto step=[&](bool fails){auto old=snapshot();auto epoch=dut.system().cycle();
      auto events=std::vector(dut.observations().Events().begin(),dut.observations().Events().end());
      auto result=dut.system().Step();
      if(fails){require(result==gfsim::SimStepResult::Failed,"expected diagnostic failure");
        require(dut.system().failureInfo().phase==gfsim::SimFailurePhase::Check,"check before commit");
        require(snapshot()==old&&dut.system().cycle()==epoch,"zero queue/state/history/epoch commit");
        require(std::vector(dut.observations().Events().begin(),dut.observations().Events().end())==events,"events preserved");
        require(dut.system().Step()==gfsim::SimStepResult::Failed&&snapshot()==old,"sticky failure");
      }else require(result==gfsim::SimStepResult::Running,"unexpected diagnostic failure");
      ++cases;};
    for(unsigned timing=0;timing<5;++timing)
      for(unsigned s=0;s<4;++s)for(unsigned l=0;l<2;++l)for(unsigned r=0;r<2;++r)for(unsigned o=0;o<2;++o){
        start(timing,s,l,r,o);
        bool fail=timing==1&&s>=2&&!o&&(l||r);
        step(fail);if(fail)continue;
        bool active=timing==1&&s<2&&!o&&(s?l:r);
        std::vector<std::uint64_t> expected=timing==4?std::vector<std::uint64_t>{0,0,0,0}:
          std::vector<std::uint64_t>{l-(active&&s==1),r-(active&&s==0),o+active,timing==1};
        require(counts()==expected,"independent transfer/count oracle");
      }
    for(unsigned u:{2u,3u}){
      for(unsigned blocked:{0u,1u}){
        start(1,u,0,0,blocked);sendLeft(1);sendRight(1);step(false);
        require(counts()==std::vector<std::uint64_t>{1,1,blocked,1},"independent input pushes commit on resource stall");
      }
      start(1,1,0,0,0);early(u);later(0);step(true);
      start(1,1,1,1,0);early(0);later(u);step(false);
      require(counts()==std::vector<std::uint64_t>{1,1,0,1},"earlier false suppresses later unknown");
    }
    start(1,1,1,1,0);auto old=snapshot();
    dut.root_->Work();dut.root_->DiscardNext();dut.observations().DiscardNext();
    require(snapshot()==old,"Discard preserves committed queue/state/history");
    step(false);require(counts()==std::vector<std::uint64_t>{0,1,1,1},"reprepare commits exact selected lane");
  }
  std::cout<<"RESOURCE_DEMAND_CPP_OK "<<cases<<"\n";
}
