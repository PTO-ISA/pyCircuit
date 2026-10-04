// Observer for the clean pinned core. Simulation always calls Session::tick().
#include "Common/Session.h"
#include "Pipeline/Issue.h"
#include "Pipeline/OoOExecute.h"
#include "Module/BranchPrediction.h"
#include "Module/LoadStoreUnit.h"
#include "observer.hpp"
using namespace observe;

static Snapshot snapshot(Session &m,unsigned extent) {
    Snapshot s;
    auto &e=*m.e;
    s.add("pc",m.i->pc); s.add("head",e.rob_front); s.add("tail",e.rob_rear);
    for(unsigned i=0;i<32;++i) s.add("x"+std::to_string(i),m.rf.read(i));
    for(unsigned i=0;i<32;++i) s.group("rat"+std::to_string(i),{e.Busy[i],e.Busy[i] ? e.Reorder[i].read() : 0});
    std::array<bool,9> occupied{};
    for(unsigned i=e.rob_front;i!=e.rob_rear;i=OoOExecute::next_rob_entry(i)) occupied[i]=true;
    for(unsigned i=1;i<=8;++i) {
        auto &r=e.rob[i];
        auto ins=r.Inst.read();
        if(occupied[i]) s.group("rob"+std::to_string(i),{1,ins.inst,ins.opcode,r.Dest,r.Ready,
                r.Ready ? r.Value.read() : 0,ins.opcode==0x63 ? r.Tag.read() : 0});
        else s.group("rob"+std::to_string(i),{0,0,0,0,0,0,0});
        s.add("retained_dest"+std::to_string(i),r.Dest);
    }
    for(unsigned i=0;i<10;++i) {
        auto &r=*e.get_rs(static_cast<RSID>(ADD1+i));
        if(r.Busy) s.group("rs"+std::to_string(i),{1,r.Op,r.Qj,r.Qk,
                r.Qj==0 ? r.Vj.read() : 0,r.Qk==0 ? r.Vk.read() : 0,i>=4 ? r.A.read() : 0,r.Dest,r.Tag});
        else s.group("rs"+std::to_string(i),{0,0,0,0,0,0,0,0,0});
    }
    for(unsigned i=4;i<10;++i) {
        auto &l=*e.loadStoreUnit;
        unsigned phase=i<7 ? l.store_cnt[i-4].read() : l.load_cnt[i-7].read();
        s.group("lsu"+std::to_string(i),{phase,i>=7 && phase==2 ? l.load_buffer[i-7].read() : 0});
    }
    for(unsigned pc=0;pc<extent;pc+=4)
        s.group("predictor"+std::to_string(pc),{m.branch->mux[pc],m.branch->two_bits[pc],m.branch->two_bits[pc+1],m.branch->two_bits[pc+2],m.branch->two_bits[pc+3]});
    return s;
}
int main(int argc,char **argv) try {
    Options options(argc,argv);
    Image image(options.image);
    auto started=Clock::now();
    auto owner=std::make_unique<Session>(false);
    auto construct=elapsed(started);
    auto &m=*owner;
    std::copy(image.bytes.begin(),image.bytes.end(),m.memory.mem);
    std::array<unsigned,9> instructionPC{}; // Observer metadata, never fed to core.
    if(!options.benchmark) trace(0,snapshot(m,image.extent),{});
    started=Clock::now();
    if(options.benchmark) {
        for(std::uint64_t i=0;i<options.fixed;++i) m.tick();
    } else {
        for(std::uint64_t i=0;i<(options.fixed ? options.fixed : options.limit);++i) {
            unsigned head=m.e->rob_front,tail=m.e->rob_rear,pc=m.i->pc;
            auto flushes=m.e->stat.flush_cycle;
            Commit retired;
            auto &r=m.e->rob[head];
            if(r.Ready) {
                auto ins=r.Inst.read();
                retired={head,instructionPC[head],ins.inst,ins.opcode,r.Dest,r.Value,
                         ins.opcode==0x63 ? r.Tag.read() : 0,ins.imm};
            }
            m.tick();
            if(m.e->stat.flush_cycle==flushes)
                for(unsigned id=tail;id!=m.e->rob_rear;id=OoOExecute::next_rob_entry(id)) instructionPC[id]=pc;
            trace(m.stat.cycle,snapshot(m,image.extent),retired);
            if(!options.fixed && m.memory[stopAddress]) break;
        }
    }
    auto run=elapsed(started);
    final(image,m.stat.cycle,run,construct,[&](unsigned i){return m.rf.read(i);},
          [&](unsigned a){return m.memory.mem[a];},[&](unsigned a){return m.branch->mux[a];},
          [&](unsigned a){return m.branch->two_bits[a];});
    return m.memory[stopAddress] || options.fixed ? 0 : 2;
} catch(const std::exception &e) {std::cerr << e.what() << '\n';return 1;}
