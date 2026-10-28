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
  // Independent runtime semantics: duplicated NOT preserves every symbol;
  // double NOT does not preserve raw Z. No coverage-proof helper supplies this.
  using Bit = gfsim::Bits<1>;
  using State = gfsim::FourState<1>;
  for (char symbol : {'0', '1', 'x', 'z'}) {
    auto raw = symbol == '0' ? State::known(Bit{0}) :
               symbol == '1' ? State::known(Bit{1}) :
               symbol == 'x' ? State::unknown() : State::highImpedance();
    auto first = gfsim::bit_not(raw), second = gfsim::bit_not(raw);
    require(first.parityEquivalent(second), "same-input cloned NOT differs");
    require(first.zMask() == Bit{0}, "NOT must turn Z into X");
    if (symbol == '0' || symbol == '1') {
      require(first.isFullyKnown() && first.value() == Bit{symbol == '0' ? 1u : 0u},
              "known NOT truth table");
      require(gfsim::bit_not(first).parityEquivalent(raw), "known double NOT");
    } else {
      require(first.knownMask() == Bit{0}, "unknown NOT truth table");
    }
    require(gfsim::bit_not(first).parityEquivalent(raw) == (symbol != 'z'),
            "raw Z and double NOT must stay distinct");
  }
  pyc_dut dut(std::strtoul(argv[1], nullptr, 10));
  auto drive = [&](bool clock) { dut.drive({bit(clock), bit(false)}); };
  drive(false);
  dut.system().Build();
  dut.system().Reset();
  auto root = dut.root_->pyc_implementation;
  auto ram = root->@REMOTE@->@STORE@->@MEMORY@_state;
  auto other = root->@OTHER_MEMORY@_state;
  auto worker = root->@BRANCH@->@WORKER@;
  auto snapshot = [&]() {
    std::vector<std::uint64_t> values;
    for (const auto &storage : {ram, other}) {
      const auto &current = storage->current(0);
      for (auto byte : current.memory) values.push_back(byte);
      values.push_back(current.clock);
    }
    auto add = [&](const auto &storage) {
      const auto &current = storage->current(0);
      values.insert(values.end(), {current.q.packed().value().value(),
          current.q.packed().knownMask().value(), current.q.packed().zMask().value(),
          current.clock});
    };
    add(worker->@PROGRESS@_state);
    add(worker->@HELD@_state);
    return values;
  };
  auto initial = snapshot();
  require(initial.size() == 18 && initial[1] == 0 && initial[5] == 0 && initial[10] == 0,
          "initial Memory/state");
  require(initial[14] == 0x83 && initial[15] == 0xcf && initial[16] == 0x10,
          "transported X/Z state");
  require(dut.system().Step() == gfsim::SimStepResult::Running, "first low sample");
  drive(true);
  require(dut.system().Step() == gfsim::SimStepResult::Running, "first rising sample");
  auto state = snapshot();
  require(state[1] == 3 && state[5] == 5 && state[10] == 1, "successful RAM/state proposals");
  require(state[4] == 1 && state[9] == 1 && state[13] == 1 && state[17] == 1,
          "successful Memory/state clock history");
  auto epoch = dut.system().cycle();
  const auto events = std::vector(dut.observations().Events().begin(), dut.observations().Events().end());
  require(!events.empty(), "prior observations published");
  drive(false);
  require(dut.system().Step() == gfsim::SimStepResult::Failed, "late Memory source failure required");
  require(dut.system().failureInfo().code == "source_check_failed", "source failure attribution");
  require(dut.system().failureInfo().message == "memory_routed_late_failure", "failure message");
  require(snapshot() == state && dut.system().cycle() == epoch, "failure committed RAM/state/history/epoch");
  require(std::vector(dut.observations().Events().begin(), dut.observations().Events().end()) == events,
          "failed Work replaced prior observations");
  require(dut.system().Step() == gfsim::SimStepResult::Failed && snapshot() == state,
          "failure must remain sticky");
  drive(false);
  dut.system().Reset();
  auto reset = snapshot();
  require(reset[1] == 3 && reset[5] == 5 && reset[10] == 0 && dut.system().cycle() == 0,
          "explicit reset must retain RAM and reset state/epoch");
  require(reset[4] == 0 && reset[9] == 0 && reset[13] == 0 && reset[17] == 0,
          "explicit reset clock history");
  drive(true);
  dut.root_->Work();
  dut.root_->DiscardNext();
  dut.observations().DiscardNext();
  require(snapshot() == reset && dut.system().cycle() == 0 && dut.observations().Events().empty(),
          "discard committed RAM/state/history/epoch/effects");
  require(dut.system().Step() == gfsim::SimStepResult::Running, "retry after Memory discard");
  auto retried = snapshot();
  require(retried[1] == 6 && retried[5] == 10 && retried[10] == 1,
          "retry Memory/state differs");
  require(retried[4] == 1 && retried[9] == 1 && retried[13] == 1 && retried[17] == 1,
          "retry Memory/state clock history");
  require(retried[14] == 0x83 && retried[15] == 0xcf && retried[16] == 0x10,
          "retry changed transported X/Z");
  std::cout << "ROUTED_ZERO_COMMIT_OK\n";
}
