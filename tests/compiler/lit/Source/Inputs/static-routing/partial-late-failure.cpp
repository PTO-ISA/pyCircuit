#include "pycircuit_system.hpp"
#include <algorithm>
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
  auto bank = root->@REMOTE@->@BANK@;
  auto snapshot = [&]() {
    std::vector<std::uint64_t> values;
    auto add = [&](const auto &storage) {
      for (std::size_t lane = 0; lane < storage->size(); ++lane) {
        const auto &current = storage->current(lane);
        values.insert(values.end(), {current.q.packed().value().value(),
            current.q.packed().knownMask().value(), current.q.packed().zMask().value(),
            current.clock});
      }
    };
    add(bank->@PACKET@_state);
    add(bank->@ENTRIES@_state);
    add(root->@LEFT@->@WORKER@->@PROGRESS@_state);
    add(root->@RIGHT@->@WORKER@->@PROGRESS@_state);
    add(root->@RIGHT@->@DISABLED@->@PROGRESS@_state);
    add(root->@PHASE@_state);
    add(root->@DISABLED_GATE@_state);
    require(values.size() == 40, "ten physical state lanes required");
    return values;
  };
  const auto initial = snapshot();
  require(initial[4] == 5 && initial[8] == 9 && initial[16] == 23, "Table initial values");
  require(initial[12] == 0x83 && initial[13] == 0xcf && initial[14] == 0x10, "Table X/Z initializer");
  require(dut.system().Step() == gfsim::SimStepResult::Running, "first low sample");
  drive(true);
  require(dut.system().Step() == gfsim::SimStepResult::Running, "first rising sample");
  const auto state = snapshot();
  require(state[0] != initial[0], "Struct contributors did not commit");
  require(state[4] == 12 && state[8] == 20 && state[16] == 23, "merged Table state");
  require(state[12] == 0x83 && state[13] == 0xcf && state[14] == 0x10, "disabled raw mask changed held element");
  require(state[20] == 1 && state[24] == 1 && state[28] == 0 && state[32] == 1 && state[36] == 0,
          "active/disabled contributor state");
  for (std::size_t index = 3; index < state.size(); index += 4)
    require(state[index] == 1, "successful clock history");
  const auto epoch = dut.system().cycle();
  const auto events = std::vector(dut.observations().Events().begin(), dut.observations().Events().end());
  // Events() exposes scalar slots. The authored logs remain exactly three
  // complete groups: eleven left values, eleven right values, seven Bank values.
  require(events.size() == 29, "prior complete scalar observation set");
  std::vector<unsigned> groupSizes;
  for (std::size_t begin = 0; begin < events.size();) {
    const auto &first = events[begin].descriptor;
    require(first.groupIndex == 0 && first.groupSize > 0 &&
            first.groupSize <= events.size() - begin, "complete observation group start");
    for (unsigned member = 0; member < first.groupSize; ++member) {
      const auto &event = events[begin + member];
      const auto &descriptor = event.descriptor;
      require(event.epoch == epoch && descriptor.kind == gfsim::ObservationKind::Event &&
              descriptor.payloadWidth == 8 && descriptor.groupIndex == member &&
              descriptor.groupSize == first.groupSize &&
              descriptor.ownerKey == first.ownerKey &&
              descriptor.registrationKey == first.registrationKey &&
              descriptor.siteKey == first.siteKey,
              "observation group member identity/epoch");
    }
    groupSizes.push_back(first.groupSize);
    begin += first.groupSize;
  }
  std::sort(groupSizes.begin(), groupSizes.end());
  require(groupSizes == std::vector<unsigned>{7, 11, 11}, "prior three complete observation groups");
  // Match the authored finite samples: the third sample holds clock high.
  drive(true);
  require(dut.system().Step() == gfsim::SimStepResult::Failed, "late check must fail");
  require(dut.system().failureInfo().code == "source_check_failed" &&
          dut.system().failureInfo().message == "partial_late_failure", "source failure attribution");
  require(snapshot() == state && dut.system().cycle() == epoch, "failed sample committed state/history/epoch");
  require(std::vector(dut.observations().Events().begin(), dut.observations().Events().end()) == events,
          "failed sample replaced prior observations");
  require(dut.system().Step() == gfsim::SimStepResult::Failed && snapshot() == state, "sticky failure");
  drive(false);
  dut.system().Reset();
  require(snapshot() == initial && dut.system().cycle() == 0, "explicit reset");
  drive(true);
  dut.root_->Work();
  dut.root_->DiscardNext();
  dut.observations().DiscardNext();
  require(snapshot() == initial && dut.system().cycle() == 0 && dut.observations().Events().empty(),
          "discard committed partial proposals");
  require(dut.system().Step() == gfsim::SimStepResult::Running && snapshot() == state, "discard/retry differs");
  std::cout << "ROUTED_ZERO_COMMIT_OK\n";
}
