#include "pycircuit_system.hpp"
#include <cstdlib>
#include <vector>

static void require(bool value) {
  if (!value) std::abort();
}

int main(int argc, char **argv) {
  pyc_dut dut(argc > 1 ? std::atoi(argv[1]) : 1);
  pyc_dut::Inputs inputs{};
  auto bit = [](unsigned value) {
    return gfsim::wire<gfsim::Bits<1>>::known(gfsim::Bits<1>{value});
  };
  inputs.pyc_7079635f636c6b = bit(0);
  inputs.pyc_7079635f727374 = bit(0);
  dut.drive(inputs);
  auto &system = dut.system();
  system.Build();
  // Repeating only after public Reset verifies no pending events survive.
  for (unsigned restart = 0; restart != 2; ++restart) {
    system.Reset();
    require(system.state() == gfsim::SimSystemState::Ready);
    require(system.cycle() == 0 && dut.observations().Events().empty());
    std::vector<gfsim::CommittedEventSlot> lastEvents;
    for (unsigned epoch = 0; epoch != 7; ++epoch) {
      inputs.pyc_7079635f636c6b = bit(epoch & 1);
      dut.drive(inputs);
      const auto result = system.Step();
      if (epoch != 6) {
        require(result == gfsim::SimStepResult::Running);
        require(system.cycle() == epoch + 1);
        const auto events = dut.observations().Events();
        lastEvents.assign(events.begin(), events.end());
      } else {
        require(result == gfsim::SimStepResult::Failed);
        require(system.state() == gfsim::SimSystemState::Failed);
        require(system.cycle() == 6);
        require(system.failureInfo().phase == gfsim::SimFailurePhase::Check);
        // Failed epochs do not publish even unrelated pending observations.
        const auto events = dut.observations().Events();
        require(std::equal(lastEvents.begin(), lastEvents.end(),
                           events.begin(), events.end()));
      }
    }
  }
}
