#include "pycircuit_system.hpp"
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <vector>

void require(bool condition, const char *message) {
  if (!condition) throw std::runtime_error(message);
}
auto bit(bool value) {
  return gfsim::wire<gfsim::Bits<1>>::known(gfsim::Bits<1>{value});
}
int main(int argc, char **argv) {
  require(argc == 2, "worker count required");
  pyc_dut dut(std::strtoul(argv[1], nullptr, 10));
  auto drive = [&](bool clock) { dut.drive({bit(clock), bit(false)}); };
  drive(false);
  dut.system().Build();
  dut.system().Reset();
  auto root = dut.root_->pyc_implementation;
  auto snapshot = [&]() {
    std::vector<std::uint64_t> values;
    auto add = [&](const auto &storage) {
      const auto &current = storage->current(0);
      values.insert(values.end(), {current.q.packed().value().value(),
          current.q.packed().knownMask().value(), current.q.packed().zMask().value(),
          current.clock});
    };
    add(root->@PEER@->@COUNT@_state);
    add(root->@WORKER@->@PROGRESS@_state);
    add(root->@SIBLING@_state);
    return values;
  };
  auto initial = snapshot();
  require(initial[0] == 13 && initial[4] == 0 && initial[8] == 29, "initial state");
  require(dut.system().Step() == gfsim::SimStepResult::Running, "first low sample");
  drive(true);
  require(dut.system().Step() == gfsim::SimStepResult::Running, "first rising sample");
  auto state = snapshot();
  require(state[0] == 16 && state[4] == 1 && state[8] == 34, "successful proposals");
  require(state[3] == 1 && state[7] == 1 && state[11] == 1, "successful clock history");
  auto epoch = dut.system().cycle();
  const auto events = std::vector(dut.observations().Events().begin(), dut.observations().Events().end());
  require(!events.empty(), "prior observations published");
  drive(false);
  require(dut.system().Step() == gfsim::SimStepResult::Failed, "late failure required");
  require(dut.system().failureInfo().code == "source_check_failed", "source failure attribution");
  require(dut.system().failureInfo().message == "routed_late_failure", "failure message");
  require(snapshot() == state && dut.system().cycle() == epoch, "failure committed storage/history/epoch");
  require(std::vector(dut.observations().Events().begin(), dut.observations().Events().end()) == events,
          "failed Work replaced prior observations");
  require(dut.system().Step() == gfsim::SimStepResult::Failed && snapshot() == state,
          "failure must remain sticky");
  drive(false);
  dut.system().Reset();
  require(snapshot() == initial && dut.system().cycle() == 0, "explicit reset");
  drive(true);
  dut.root_->Work();
  dut.root_->DiscardNext();
  dut.observations().DiscardNext();
  require(snapshot() == initial && dut.system().cycle() == 0 && dut.observations().Events().empty(),
          "discard committed storage/history/epoch/effects");
  require(dut.system().Step() == gfsim::SimStepResult::Running, "retry after discard");
  require(snapshot() == state, "retry state/history differs");
  std::cout << "ROUTED_ZERO_COMMIT_OK\n";
}
