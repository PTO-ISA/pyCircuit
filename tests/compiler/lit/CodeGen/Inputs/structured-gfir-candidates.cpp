#include "pycircuit_system.hpp"
#include <array>
#include <cstdlib>
#include <iostream>

auto bit(bool value) {
  return gfsim::wire<gfsim::Bits<1>>::known(gfsim::Bits<1>{value});
}

template <class Outputs>
void expect(const Outputs &root, unsigned total, unsigned snapshot) {
  const auto check = [](const auto &port, unsigned expected) {
    return port.isFullyKnown() && port.value() == gfsim::Bits<8>{expected};
  };
  if (!check(root.total, total) || !check(root.snapshot, snapshot)) {
    std::cerr << "Expected committed total " << total << " and prior total "
              << snapshot << '\n';
    std::abort();
  }
}

void require(bool condition) {
  if (!condition)
    std::abort();
}

void checkSystem() {
  pyc_dut dut;
  auto &system = dut.system();
  auto &slots = dut.observations();
  system.Build();
  system.Reset();
  const auto drive = [&](auto clock) {
    pyc_dut::Inputs inputs;
    inputs.pyc_7079635f636c6b = clock;
    inputs.pyc_7079635f727374 = bit(false);
    dut.drive(inputs);
  };
  drive(bit(true));
  require(system.Step() == gfsim::SimStepResult::Running);
  expect(dut.sample(), 0, 0);
  require(system.cycle() == 1);

  drive(decltype(bit(false))::unknown());
  require(system.Step() == gfsim::SimStepResult::Failed);
  require(system.Step() == gfsim::SimStepResult::Failed);
  require(system.cycle() == 1);

  // Attached slots remain part of admission after their public owner changes.
  system.Reset();
  slots = gfsim::ObservationSlots{};
  drive(bit(true));
  require(system.Step() == gfsim::SimStepResult::Failed);
  require(system.cycle() == 0);

  system.Reset();
  const std::array descriptors{
      gfsim::ObservationDescriptor{0, 1, 1, 1, gfsim::ObservationKind::Event, 0, 2},
      gfsim::ObservationDescriptor{1, 1, 1, 1, gfsim::ObservationKind::Event, 1, 2}};
  require(slots.Configure(descriptors, 2));
  require(slots.Stage(0, gfsim::SlotValue::Unsigned(10), true, true, 0));
  require(slots.Stage(1, gfsim::SlotValue::Unsigned(20), true, true, 0));
  require(system.Step() == gfsim::SimStepResult::Running);
  expect(dut.sample(), 0, 0);
  require(slots.Events().size() == 2);
  drive(bit(false));
  require(system.Step() == gfsim::SimStepResult::Running);
  expect(dut.sample(), 2, 0);
  require(slots.Events().empty());

  // An incomplete observation group rejects the whole rising sample.
  require(slots.Stage(0, gfsim::SlotValue::Unsigned(30), true, true, 2));
  drive(bit(true));
  require(system.Step() == gfsim::SimStepResult::Failed);
  require(system.cycle() == 2);
  require(slots.Events().empty());
}

void checkAdditionalRoot() {
  struct Observer final : gfsim::SimModule {
    Observer() : SimModule("observer") {}
    unsigned works = 0, commits = 0;
    void Work() override { ++works; }
    void Xfer() noexcept override { ++commits; }
  } observer;
  pyc_dut dut;
  require(dut.system().AddModule(observer));
  dut.system().Build();
  dut.system().Reset();
  pyc_dut::Inputs inputs;
  inputs.pyc_7079635f636c6b = bit(true);
  inputs.pyc_7079635f727374 = bit(false);
  dut.drive(inputs);
  require(dut.system().Step() == gfsim::SimStepResult::Running);
  expect(dut.sample(), 0, 0);
  require(observer.works == 1 && observer.commits == 2);
}

int main() {
  gfsim::WorkExecutor executor;
  pyc_root root("candidates", &executor);
  root.pyc_7079635f636c6b = bit(false);
  root.pyc_7079635f727374 = bit(false);
  root.Build();
  root.Reset();
  root.Xfer();

  const auto sample = [&](bool clock, bool reset, unsigned total, unsigned old) {
    root.pyc_7079635f636c6b = bit(clock);
    root.pyc_7079635f727374 = bit(reset);
    root.Work();
    expect(root, total, old);
    root.Xfer();
  };

  sample(false, false, 0, 0);
  root.pyc_7079635f636c6b = bit(true);
  root.Work();
  expect(root, 0, 0);
  root.DiscardNext();
  root.Xfer();

  // A discarded edge must be retryable. Both nested calls share one
  // candidate; the independent Rule sees the committed value.
  sample(true, false, 0, 0);
  sample(true, false, 2, 0);
  sample(false, false, 2, 0);
  sample(true, false, 2, 0);
  sample(true, false, 4, 2);

  sample(false, false, 4, 2);
  sample(true, true, 4, 2);
  sample(true, false, 0, 0);

  // A later Work replaces the pending edge before either value or clock commits.
  sample(false, false, 0, 0);
  root.pyc_7079635f636c6b = bit(true);
  root.Work();
  root.pyc_7079635f636c6b = bit(false);
  root.Work();
  expect(root, 0, 0);
  root.Xfer();
  sample(true, false, 0, 0);
  sample(true, false, 2, 0);

  checkSystem();
  checkAdditionalRoot();
}
