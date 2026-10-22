#include "pycircuit_system.hpp"
#include <iostream>
#include <source_location>
#include <stdexcept>
#include <type_traits>
#include <vector>
using Bit = gfsim::wire<gfsim::Bits<1>>;
static Bit four(unsigned s) {
  if (s < 2) return Bit::known(gfsim::Bits<1>{s});
  return Bit::fromPacked(s == 2 ? gfsim::FourState<1>::unknown()
                               : gfsim::FourState<1>::highImpedance());
}
static void require(bool ok, std::source_location at = std::source_location::current()) {
  if (!ok) throw std::runtime_error("loop else oracle line " + std::to_string(at.line()));
}
template<class Storage, class Output>
void inject(Storage &storage, Output &output, unsigned value) {
  using Current = std::remove_cvref_t<decltype(storage->current(0))>;
  auto &q = const_cast<Current &>(storage->current(0));
  q.q = four(value); output.element(0) = q.q;
}
int main() {
  unsigned cases = 0;
  for (unsigned workers : {1u, 2u}) {
    pyc_dut dut(workers);
    auto drive = [&](bool clock) { dut.drive({four(clock), four(0)}); };
    drive(false); dut.system().Build();
    auto root = dut.root_->pyc_implementation;
    auto snapshot = [&] {
      std::vector<std::uint64_t> result;
      auto add = [&](const auto &storage) {
        const auto &q = storage->current(0);
        result.insert(result.end(), {q.q.packed().value().value(),
          q.q.packed().knownMask().value(), q.q.packed().zMask().value(), q.clock});
      };
      add(root->pyc_instance_total_state); add(root->pyc_instance_peer_state);
      add(root->pyc_instance_stop_state); add(root->pyc_instance_allow_state);
      return result;
    };
    for (unsigned stop = 0; stop < 4; ++stop)
      for (unsigned allow = 0; allow < 2; ++allow) {
        drive(false); dut.system().Reset();
        inject(root->pyc_instance_stop_state, root->pyc_instance_stop_q, stop);
        inject(root->pyc_instance_allow_state, root->pyc_instance_allow_q, allow);
        drive(true);
        auto old = snapshot();
        auto epoch = dut.system().cycle();
        auto events = std::vector(dut.observations().Events().begin(), dut.observations().Events().end());
        auto gauges = std::vector(dut.observations().Gauges().begin(), dut.observations().Gauges().end());
        auto result = dut.system().Step();
        if (stop >= 2 || !allow) {
          require(result == gfsim::SimStepResult::Failed);
          require(dut.system().failureInfo().phase == gfsim::SimFailurePhase::Check);
          require(snapshot() == old && dut.system().cycle() == epoch);
          require(std::vector(dut.observations().Events().begin(), dut.observations().Events().end()) == events);
          require(std::vector(dut.observations().Gauges().begin(), dut.observations().Gauges().end()) == gauges);
        } else {
          require(result == gfsim::SimStepResult::Running);
          require(root->pyc_instance_total_state->current(0).q.value().value() == (stop ? 6 : 28));
          require(root->pyc_instance_peer_state->current(0).q.value().value() == 8);
          // Observation slots contain one event per logged scalar value.
          require(dut.observations().Events().size() == (stop ? 3 : 9));
          require(dut.observations().Gauges().size() == 1);
        }
        ++cases;
      }
  }
  std::cout << "LOOP_ELSE_CPP_OK " << cases << '\n';
}
