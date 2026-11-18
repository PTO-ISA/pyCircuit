#include "pycircuit_system.hpp"
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

static void require(bool ok, const std::string &message) {
  if (!ok) throw std::runtime_error(message);
}
template <unsigned W> auto known(std::uint64_t value) {
  return gfsim::wire<gfsim::Bits<W>>::known(gfsim::Bits<W>{value});
}
struct Snapshot {
  std::uint32_t total=0, row=0, column=0, remaining=0, slot_elapsed=0;
  unsigned phase=0, source=0, stage=0;
  bool done=false;
};
struct Span {
  std::uint32_t begin=0, duration=0;
  unsigned phase=0, source=0, stage=0;
};

int main(int argc, char **argv) {
  try {
    require(argc==3, "usage: timing_runner CONFIG.txt OUTPUT.json");
    require(!std::filesystem::exists(argv[2]), "refusing to replace existing timing receipt");
    std::ifstream config_file(argv[1]);
    require(bool(config_file), "cannot open timing config");
    std::array<std::uint32_t,13> config{};
    std::uint64_t host_bound=100;
    for(auto &field:config) {
      std::uint64_t value=0;
      require(bool(config_file>>value)&&value>0&&value<=1000000,
              "timing config requires thirteen positive integers at most 1000000");
      field=std::uint32_t(value);host_bound+=32*value;
    }
    std::string extra;
    require(!(config_file>>extra), "unexpected extra timing config field");
    gfsim::Bits<416> packed{};
    for(unsigned i=0;i<config.size();++i)
      for(unsigned bit=0;bit<32;++bit)
        if((config[i]>>bit)&1) packed=gfsim::detail::setBit(packed,32*(12-i)+bit);
    pyc_dut dut(1);
    pyc_dut::Inputs input{};
    input.pyc_636f6e666967=decltype(input.pyc_636f6e666967)::fromPacked(gfsim::FourState<416>::known(packed));
    input.start=known<1>(0);
    input.pyc_7079635f636c6b=known<1>(0);
    input.pyc_7079635f727374=known<1>(0);
    dut.drive(input);dut.system().Build();dut.system().Reset();
    require(dut.system().state()==gfsim::SimSystemState::Ready,"timing DUT initialization failed");
    std::uint64_t steps=0;
    auto step=[&](bool start,unsigned clock) {
      input.start=known<1>(start);
      input.pyc_7079635f636c6b=known<1>(clock);
      dut.drive(input);
      ++steps;
      if(dut.system().Step()!=gfsim::SimStepResult::Running) {
        auto failure=dut.system().failureInfo();
        throw std::runtime_error("timing DUT checked step failed: "+std::string(failure.code)+
                                 " "+std::string(failure.message)+" "+std::string(failure.sourceJson));
      }
    };
    auto sample=[&]() {
      auto output=dut.sample();
      // ports.json declares this unguarded state output always available (null
      // availability); every payload bit is still checked for known/non-Z data.
      auto bits=output.result.packed();
      auto get=[&](unsigned offset,unsigned width) {
        std::uint32_t value=0;
        for(unsigned bit=0;bit<width;++bit) {
          require(bits.knownMask().bit(offset+bit)&&!bits.zMask().bit(offset+bit),
                  "unknown timing result");
          value|=std::uint32_t(bits.value().bit(offset+bit))<<bit;
        }
        return value;
      };
      Snapshot result;
      result.done=get(0,1);result.slot_elapsed=get(1,32);result.remaining=get(33,32);
      result.stage=get(65,4);result.source=get(69,3);result.phase=get(72,1);
      result.column=get(73,32);result.row=get(105,32);result.total=get(137,32);
      return result;
    };
    step(true,0);require(sample().stage==0,"DUT did not begin idle");
    step(true,1); // Capture configuration at t=0; no timing tick is consumed.
    step(false,0);auto state=sample();
    require(state.total==0&&state.phase==0&&state.stage==1,"unexpected timing launch state");
    std::uint32_t ticks=0;
    std::array<std::array<std::uint32_t,11>,2> stage_ticks{};
    std::vector<Span> spans;
    while(!state.done) {
      require(ticks<host_bound,"timing execution exceeded host safety bound");
      require(state.stage>=1&&state.stage<=9&&state.remaining>0,"invalid active timing stage");
      if(spans.empty()||spans.back().phase!=state.phase||spans.back().source!=state.source||
         spans.back().stage!=state.stage)
        spans.push_back({ticks,0,state.phase,state.source,state.stage});
      ++spans.back().duration;++stage_ticks[state.phase][state.stage];
      step(false,1);++ticks;
      step(false,0);state=sample();
      require(state.total==ticks&&state.row+state.column==ticks,
              "DUT elapsed counters disagree with observed timing ticks");
    }
    require(state.stage==10&&state.phase==1&&state.source==3,"wrong terminal timing state");
    const std::array<const char*,11> names={"idle","alignment","arm","ub_read","tx_crossing",
      "network","rx_crossing","ub_write","guard","slot_idle","done"};
    std::ofstream report(argv[2]);require(bool(report),"cannot create timing receipt");
    report<<"{\n  \"status\":\"EXECUTED\",\n  \"model\":\"conservative_packet_timing_projection\",\n"
          <<"  \"backend\":\"cpp\",\n  \"workers\":1,\n  \"steps\":"<<steps<<",\n"
          <<"  \"total_cycles\":"<<state.total<<",\n  \"row_cycles\":"<<state.row
          <<",\n  \"column_cycles\":"<<state.column<<",\n  \"stage_ticks\":[";
    for(unsigned phase=0;phase<2;++phase) {
      if(phase)report<<',';report<<'{';
      for(unsigned stage=1;stage<=9;++stage) {
        if(stage>1)report<<',';
        report<<'"'<<names[stage]<<"\":"<<stage_ticks[phase][stage];
      }
      report<<'}';
    }
    report<<"],\n  \"spans\":[";
    for(unsigned i=0;i<spans.size();++i) {
      if(i)report<<',';const auto &span=spans[i];
      report<<"{\"begin_cycle\":"<<span.begin<<",\"duration\":"<<span.duration
            <<",\"phase\":"<<span.phase<<",\"source_rank\":"<<span.source
            <<",\"stage\":\""<<names[span.stage]<<"\"}";
    }
    report<<"]\n}\n";require(bool(report),"cannot write timing receipt");
    std::cout<<"EXECUTED timing projection: "<<state.total<<" NoC cycles\n";
    return 0;
  } catch(const std::exception &error) {
    std::cerr<<error.what()<<'\n';return 1;
  }
}
