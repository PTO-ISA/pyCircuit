#include "model.hpp"
#include "observer.hpp"
using namespace ac_generated;
using namespace observe;

static std::uint8_t byte(CPU &m, unsigned address) {
    return m.memory.refs[address >> 8]->peek().words[(address >> 2) & 63] >> ((address & 3) * 8);
}
static Snapshot snapshot(CPU &m, unsigned extent) {
    Snapshot s;
    s.add("pc",m.pc.peek()); s.add("head",m.head.peek()); s.add("tail",m.tail.peek());
    for(unsigned i=0;i<32;++i) s.add("x"+std::to_string(i),m.registers.refs[i]->peek());
    for(unsigned i=0;i<32;++i) {
        const auto &r=m.rename.refs[i]->peek();
        s.group("rat"+std::to_string(i),{r.busy,r.busy ? r.tag : 0});
    }
    for(unsigned i=0;i<8;++i) {
        auto *q=m.rob.refs[i];
        auto r=q->empty() ? ROBEntry{} : q->peek();
        s.group("rob"+std::to_string(i+1),{!q->empty(),r.ins.word,r.ins.opcode,r.dest,
                r.ready,r.ready ? r.value : 0,r.ins.opcode==0x63 ? r.predicted : 0});
        s.add("retained_dest"+std::to_string(i+1),m.retained_dest.refs[i]->peek());
    }
    for(unsigned i=0;i<10;++i) {
        auto *q=m.stations.refs[i];
        auto r=q->empty() ? Station{} : q->peek();
        s.group("rs"+std::to_string(i),{!q->empty(),r.op,r.left.tag,r.right.tag,
                r.left.tag ? 0 : r.left.value,r.right.tag ? 0 : r.right.value,
                i>=4 ? r.address : 0,r.rob,r.pc});
    }
    for(unsigned i=4;i<10;++i) {
        auto &r=m.stages.refs[i]->peek();
        s.group("lsu"+std::to_string(i),{r.phase,i>=7 && r.phase==2 ? r.buffer : 0});
    }
    for(unsigned pc=0;pc<extent;pc+=4) {
        auto &p=m.predictor.refs[pc >> 8]->peek();
        auto i=pc & 255;
        s.group("predictor"+std::to_string(pc),{p.history[i],p.counters[i],p.counters[i+1],p.counters[i+2],p.counters[i+3]});
    }
    return s;
}
static Commit commit(CPU &m) {
    auto id=m.head.peek();
    auto *q=m.rob.refs[id-1];
    if(q->empty() || !q->peek().ready) return {};
    auto &r=q->peek();
    return {id,r.pc,r.ins.word,r.ins.opcode,r.dest,r.value,r.predicted,r.ins.imm};
}
int main(int argc,char **argv) try {
    Options options(argc,argv);
    Image image(options.image);
    std::vector<MemoryPage> pages(MEMORY_PAGES);
    for(unsigned a=0;a<memorySize;++a) pages[a >> 8].words[(a >> 2)&63] |= std::uint32_t(image.bytes[a]) << ((a&3)*8);
    auto started=Clock::now();
    auto owner=std::make_unique<CPU>(pages,options.reverse);
    auto construct=elapsed(started);
    auto &m=*owner;
    if(!options.benchmark) trace(0,snapshot(m,image.extent),{});
    started=Clock::now();
    if(options.benchmark) {
        for(std::uint64_t i=0;i<options.fixed;++i) m.sim.step();
    } else {
        for(std::uint64_t i=0;i<(options.fixed ? options.fixed : options.limit);++i) {
            auto retired=commit(m);
            m.sim.step();
            trace(m.sim.tick(),snapshot(m,image.extent),retired);
            if(!options.fixed && byte(m,stopAddress)) break;
        }
    }
    auto run=elapsed(started);
    final(image,m.sim.tick(),run,construct,
          [&](unsigned i){return m.registers.refs[i]->peek();},
          [&](unsigned a){return byte(m,a);},
          [&](unsigned a){return m.predictor.refs[a>>8]->peek().history[a&255];},
          [&](unsigned a){return m.predictor.refs[a>>8]->peek().counters[a&255];});
    return byte(m,stopAddress) || options.fixed ? 0 : 2;
} catch(const std::exception &e) {std::cerr << e.what() << '\n';return 1;}
