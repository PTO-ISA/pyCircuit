#include "pycircuit_system.hpp"
#include <cstdint>
#include <iostream>
#include <string>
#include <tuple>

template <unsigned W> auto known(std::uint64_t v) {
  return gfsim::wire<gfsim::Bits<W>>::known(gfsim::Bits<W>{v});
}
int failures = 0;
void require(bool ok, const std::string &what) {
  std::cout << (ok ? "ok   " : "FAIL ") << what << "\n";
  if (!ok) ++failures;
}
int main() {
  pyc_dut dut(1);
  pyc_dut::Inputs in;
  in.value = known<8>(0x12);
  in.second = known<4>(0x5);
  in.ok = known<1>(1);
  dut.drive(in);
  dut.system().Build();
  dut.system().Reset();
  auto &obs = dut.observations();

  // The descriptor table is configured, ordered and unique.
  require(obs.Descriptors().size() == 5, "five descriptors configured");
  bool increasing = true, unique = true;
  for (std::size_t i = 1; i < obs.Descriptors().size(); ++i) {
    const auto &p = obs.Descriptors()[i - 1], &c = obs.Descriptors()[i];
    increasing &= std::tie(p.ownerKey, p.registrationKey, p.siteKey,
                           p.stableOrdinal, p.kind) <
                  std::tie(c.ownerKey, c.registrationKey, c.siteKey,
                           c.stableOrdinal, c.kind);
    unique &= p.stableOrdinal != c.stableOrdinal;
  }
  require(increasing, "descriptors strictly increase by key");
  require(unique, "stable ordinals are globally unique");
  require(obs.Descriptors()[0].kind == gfsim::ObservationKind::Gauge &&
              obs.Descriptors()[1].kind == gfsim::ObservationKind::Gauge,
          "report maps to Gauge");
  require(obs.Descriptors()[2].kind == gfsim::ObservationKind::Event &&
              obs.Descriptors()[3].kind == gfsim::ObservationKind::Event &&
              obs.Descriptors()[4].kind == gfsim::ObservationKind::Event,
          "log maps to Event");
  require(obs.Descriptors()[0].stableOrdinal == 0 &&
              obs.Descriptors()[4].stableOrdinal == 4,
          "stable ordinals are the configured slot indices");

  // Two epochs each publish exactly their own evaluation epoch.
  require(dut.system().Step() == gfsim::SimStepResult::Running, "epoch 1 running");
  require(obs.Gauges().size() == 2, "two gauges");
  require(obs.Gauges()[0].value.bits == 0x12 && obs.Gauges()[0].lastUpdate == 1,
          "gauge_a committed 0x12 at epoch 1");
  require(obs.Gauges()[1].value.bits == 0x5 && obs.Gauges()[1].lastUpdate == 1,
          "gauge_b committed 0x5 at epoch 1");
  require(obs.Events().size() == 3, "three events committed at epoch 1");
  bool epochs = true;
  for (auto e : obs.Events())
    epochs &= e.epoch == 1 && e.epoch == dut.system().cycle();
  require(epochs, "commit_epoch == evaluation_epoch + 1 == 1");
  require(obs.Events()[0].value.bits == 0x12 &&
              obs.Events()[1].value.bits == 0x5 &&
              obs.Events()[2].value.bits == 1,
          "event values are the in-DUT staged values");

  in.value = known<8>(0x34);
  in.second = known<4>(0xA);
  dut.drive(in);
  require(dut.system().Step() == gfsim::SimStepResult::Running, "epoch 2 running");
  require(dut.system().cycle() == 2, "cycle 2 committed");
  require(obs.Gauges()[0].value.bits == 0x34 && obs.Gauges()[0].lastUpdate == 2,
          "gauge_a advanced to 0x34 at epoch 2");
  require(obs.Events().size() == 3 && obs.Events()[0].epoch == 2 &&
              obs.Events()[0].value.bits == 0x34,
          "event batch is the epoch 2 batch with commit_epoch 2");

  // One failed epoch keeps the previously committed batch and does not advance
  // any gauge, and discards the batch the failing epoch had already staged.
  in.value = known<8>(0x77);
  in.second = known<4>(0x1);
  dut.drive(in);
  require(obs.Stage(0, gfsim::SlotValue::Unsigned(0x99), true, true, 99),
          "fault injected into slot 0");
  require(dut.system().Step() == gfsim::SimStepResult::Failed, "epoch 3 failed");
  require(dut.system().failureInfo().phase == gfsim::SimFailurePhase::Check,
          "failure phase is Check, so Work staged its observations first");
  require(obs.Events().size() == 3 && obs.Events()[0].epoch == 2 &&
              obs.Events()[0].value.bits == 0x34,
          "Events keeps the previously committed batch");
  require(obs.Gauges()[0].lastUpdate == 2 && obs.Gauges()[0].value.bits == 0x34 &&
              obs.Gauges()[1].lastUpdate == 2,
          "Gauge lastUpdate did not advance");
  require(obs.Stage(0, gfsim::SlotValue::Unsigned(1), true, true, 2),
          "slot 0 is no longer pending, so DiscardNext cleared the batch");
  std::cout << (failures ? "OBSERVATION-DRIVER-FAILED\n"
                         : "OBSERVATION-DRIVER-OK\n");
  return failures != 0;
}
