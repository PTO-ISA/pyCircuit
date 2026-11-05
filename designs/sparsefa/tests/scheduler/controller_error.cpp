#include "pycircuit_system.hpp"
#include <cstdlib>
#include <iostream>
#include <source_location>

void require(bool ok, std::source_location at = std::source_location::current()) {
  if (!ok) {
    std::cerr << "controller error oracle line " << at.line() << '\n';
    std::abort();
  }
}
template<unsigned W> auto known(unsigned value) {
  return gfsim::wire<gfsim::Bits<W>>::known(gfsim::Bits<W>{value});
}
using Command = fa2::fa2_interfaces::EngineCommand;
using Reply = fa2::fa2_interfaces::MemoryReply;
using State = fa2::fa2_engine::EngineState;
using Datapath = fa2::fa2_engine::DatapathResult;

struct Case {
  const char *name;
  unsigned phase, valid, error, dataKind, data, cancel;
  unsigned expectedPhase, expectedCode;
  bool expectedKnown = true;
};
// Independent protocol oracle: failed responses terminate with code5;
// successful COUNT accepts0..4 and INDEX accepts0..3 for key_length4.
const Case cases[] = {
  {"count-error-x", 2,1,1,1,0,0,16,5},
  {"count-error-z", 2,1,1,2,0,0,16,5},
  {"count-error-legal", 2,1,1,0,1,0,16,5},
  {"count-error-invalid", 2,1,1,0,5,0,16,5},
  {"index-error-x", 4,1,1,1,0,0,16,5},
  {"index-error-z", 4,1,1,2,0,0,16,5},
  {"index-error-legal", 4,1,1,0,1,0,16,5},
  {"index-error-invalid", 4,1,1,0,5,0,16,5},
  {"count-zero", 2,1,0,0,0,0,6,0},
  {"count-legal", 2,1,0,0,1,0,3,0},
  {"count-limit", 2,1,0,0,4,0,3,0},
  {"count-invalid", 2,1,0,0,5,0,16,2},
  {"index-zero", 4,1,0,0,0,0,25,0},
  {"index-legal", 4,1,0,0,3,0,25,0},
  {"index-invalid", 4,1,0,0,4,0,16,3},
  {"count-noerror-x", 2,1,0,1,0,0,0,0,false},
  {"index-noerror-x", 4,1,0,1,0,0,0,0,false},
  {"count-noresponse-x", 2,0,1,1,0,0,2,0},
  {"index-noresponse-x", 4,0,1,1,0,0,4,0},
  {"count-cancel-error-x", 2,1,1,1,0,1,16,6},
  {"index-cancel-error-x", 4,1,1,1,0,1,16,6},
};

int main(int argc, char **argv) {
  require(argc == 2);
  pyc_dut dut(std::atoi(argv[1]));
  pyc_dut::Inputs input{};
  Command command{};
  command.key_length = gfsim::Bits<32>{4};
  command.dimensions = gfsim::Bits<7>{1};
  input.command = input.saved = gfsim::wire<Command>::known(command);
  input.progress = gfsim::wire<State>::known(State{});
  Datapath datapath{};
  datapath.waiting = gfsim::Bits<1>{1};
  input.datapath = gfsim::wire<Datapath>::known(datapath);
  input.completion_ready = input.cancel = known<1>(0);
  input.trie = gfsim::wire<fa2::fa2_engine::TrieStatus>::known({});
  input.phase = known<6>(2);
  input.code = known<4>(0);
  input.memory = gfsim::wire<Reply>::known(Reply{});
  dut.drive(input);
  dut.system().Build();
  dut.system().Reset();
  for (const auto &test : cases) {
    Reply reply{};
    reply.valid = gfsim::Bits<1>{test.valid};
    reply.error = gfsim::Bits<1>{test.error};
    reply.data = gfsim::Bits<32>{test.data};
    // Generated Struct ABI: request_ready, valid, data[32], error, MSB first.
    static_assert(gfsim::hardware_traits<Reply>::width == 35);
    using Packed = gfsim::Bits<35>;
    constexpr auto dataMask = 0xffffffffULL << 1;
    constexpr auto fullMask = (1ULL << 35) - 1;
    input.memory = gfsim::wire<Reply>::fromPacked(gfsim::FourState<35>::fromMasks(
        gfsim::hardware_traits<Reply>::pack(reply),
        Packed{test.dataKind ? fullMask ^ dataMask : fullMask},
        Packed{test.dataKind == 2 ? dataMask : 0}));
    input.phase = known<6>(test.phase);
    input.cancel = known<1>(test.cancel);
    dut.drive(input);
    auto before = dut.system().cycle();
    require(dut.system().Step() == gfsim::SimStepResult::Running);
    require(dut.system().cycle() == before + 1);
    auto result = dut.sample().result.packed();
    require(result.invariantHolds());
    bool fullyKnown = result.knownMask().value() == 1023 && result.zMask().value() == 0;
    require(fullyKnown == test.expectedKnown);
    if (test.expectedKnown) {
      require(result.value().value() == ((test.expectedPhase << 4) | test.expectedCode));
    }
    std::cout << test.name << " PASS\n";
  }
  std::cout << "controller_error_pass " << std::size(cases) << '\n';
}
