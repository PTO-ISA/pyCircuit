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
  if (!ok) throw std::runtime_error("external pop oracle line " + std::to_string(at.line()));
}
template<class Storage, class Output>
void inject(Storage &storage, Output &output, unsigned s) {
  using Current = std::remove_cvref_t<decltype(storage->current(0))>;
  auto &q = const_cast<Current &>(storage->current(0));
  q.q = four(s); output.element(0) = q.q;
}
int main() {
  unsigned cases = 0;
  for (unsigned workers : {1u, 2u}) {
    pyc_dut dut(workers);
    auto drive = [&](bool clk, bool rst) { dut.drive({four(clk), four(rst)}); };
    drive(false, false); dut.system().Build();
    auto root = dut.root_->pyc_implementation;
    auto snapshot = [&] {
      const auto &q = root->pyc_queue_tokens_state->current(0);
      std::vector<std::uint64_t> result{q.rd, q.wr, q.count, q.clock, q.initialized,
        q.storage[0].packed().value().value(), q.storage[0].packed().knownMask().value(),
        q.storage[0].packed().zMask().value()};
      auto add = [&](const auto &storage) {
        const auto &state = storage->current(0);
        result.insert(result.end(), {state.q.packed().value().value(),
          state.q.packed().knownMask().value(), state.q.packed().zMask().value(), state.clock});
      };
      add(root->pyc_instance_count_state);
      add(root->pyc_instance_allow_state);
      add(root->pyc_instance_take_state);
      return result;
    };
    auto start = [&](unsigned timing, unsigned allow, unsigned take, unsigned full) {
      drive(false, false); dut.system().Reset();
      if (timing == 2 || timing == 3) {
        drive(true, true);
        require(dut.system().Step() == gfsim::SimStepResult::Running);
      }
      using Current = std::remove_cvref_t<decltype(root->pyc_queue_tokens_state->current(0))>;
      auto &q = const_cast<Current &>(root->pyc_queue_tokens_state->current(0));
      q.count = full; q.rd = q.wr = 0;
      q.storage[0] = gfsim::wire<gfsim::Bits<8>>::known(gfsim::Bits<8>{11});
      inject(root->pyc_instance_allow_state, root->pyc_instance_allow_q, allow);
      inject(root->pyc_instance_take_state, root->pyc_instance_take_q, take);
      drive(timing == 1 || timing == 2 || timing == 4, timing == 4);
    };
    auto step = [&](bool fails) {
      auto old = snapshot();
      auto epoch = dut.system().cycle();
      auto events = std::vector(dut.observations().Events().begin(), dut.observations().Events().end());
      auto gauges = std::vector(dut.observations().Gauges().begin(), dut.observations().Gauges().end());
      auto result = dut.system().Step();
      if (fails) {
        require(result == gfsim::SimStepResult::Failed);
        require(dut.system().failureInfo().phase == gfsim::SimFailurePhase::Check);
        require(snapshot() == old && dut.system().cycle() == epoch);
        require(std::vector(dut.observations().Events().begin(), dut.observations().Events().end()) == events);
        require(std::vector(dut.observations().Gauges().begin(), dut.observations().Gauges().end()) == gauges);
        require(dut.system().Step() == gfsim::SimStepResult::Failed && snapshot() == old);
      } else require(result == gfsim::SimStepResult::Running);
      ++cases;
    };
    for (unsigned timing = 0; timing < 5; ++timing)
      for (unsigned allow = 0; allow < 4; ++allow)
        for (unsigned take = 0; take < 4; ++take)
          for (unsigned full = 0; full < 2; ++full) {
            start(timing, allow, take, full);
            bool fails = timing == 1 && (allow >= 2 || (allow == 1 && take >= 2));
            step(fails);
            if (fails) continue;
            const auto &q = root->pyc_queue_tokens_state->current(0);
            bool pop = allow == 1 && take == 1 && full;
            bool rising = timing == 1;
            unsigned expectedCount = timing == 4 ? 0 : rising ? 1 : full;
            unsigned expectedValue = rising && (!full || pop) ? 23 : 11;
            require(q.count == expectedCount && q.storage[0].isFullyKnown());
            require(q.storage[0].value().value() == expectedValue);
            require(root->pyc_instance_count_state->current(0).q.value().value() == rising);
            require(q.clock == (timing == 1 || timing == 2 || timing == 4));
          }
    // A full queue needs the other Rule's accepted pop. Discard must preserve
    // all storage, clock histories, observations and the retry's rising edge.
    start(1, 1, 1, 1);
    auto old = snapshot();
    auto epoch = dut.system().cycle();
    dut.root_->Work(); dut.root_->DiscardNext(); dut.observations().DiscardNext();
    require(snapshot() == old && dut.system().cycle() == epoch);
    require(dut.observations().Events().empty());
    step(false);
    require(root->pyc_queue_tokens_state->current(0).count == 1);
    require(root->pyc_queue_tokens_state->current(0).storage[0].value().value() == 23);
    require(root->pyc_instance_count_state->current(0).q.value().value() == 1);
  }
  std::cout << "EXTERNAL_POP_CPP_OK " << cases << '\n';
}
