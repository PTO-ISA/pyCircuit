#include "pycircuit_system.hpp"
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>

void require(bool condition) {
  if (!condition) { std::cerr << "queue fault oracle failed\n"; std::abort(); }
}
template <class Wire> void wire(std::ostream &out, const Wire &value) {
  const auto packed = value.packed();
  for (unsigned bit = Wire::width; bit-- > 0;)
    out << (packed.zMask().bit(bit) ? 'z' : !packed.knownMask().bit(bit) ? 'x' : packed.value().bit(bit) ? '1' : '0');
  out << '|';
}
template <class Storage> void save(std::ostream &out, const Storage &storage) {
  for (std::size_t lane = 0; lane < storage.size(); ++lane) {
    const auto &state = storage.current(lane);
    out << state.clock << '|';
    if constexpr (requires { state.q; }) wire(out, state.q);
    if constexpr (requires { state.storage; }) {
      out << state.rd << '|' << state.wr << '|' << state.count << '|' << state.initialized << '|';
      for (const auto &token : state.storage) wire(out, token);
      if constexpr (requires { state.timing.tick; }) {
        out << state.timing.tick << '|' << state.timing.mature_ptr << '|' << state.timing.eligible_count << '|';
        for (auto deadline : state.timing.deadline) out << deadline << '|';
      }
    }
  }
}
std::string snapshot(pyc_dut &dut) {
  std::ostringstream out;
  @SNAPSHOT@
  return out.str();
}
int main() {
  const auto bit = [](bool value) { return gfsim::wire<gfsim::Bits<1>>::known(gfsim::Bits<1>{value ? 1u : 0u}); };
  constexpr unsigned edge = @EDGE@;
  constexpr std::string_view message = "@MESSAGE@";
  @EXPECTED@
  for (unsigned workers : {1u, 2u}) {
    pyc_dut dut(workers);
    dut.drive({bit(false), bit(false)});
    dut.system().Build();
    for (unsigned retry = 0; retry < 2; ++retry) {
      dut.drive({bit(false), bit(false)});
      dut.system().Reset();
      require(dut.system().cycle() == 0);
      for (unsigned epoch = 0; epoch < edge; ++epoch) {
        for (bool clock : {false, true}) {
          dut.drive({bit(clock), bit(false)});
          require(dut.system().Step() == gfsim::SimStepResult::Running);
          std::ostringstream observed;
          wire(observed, @RESULT@);
          require(observed.str() == expected[epoch * 2 + unsigned(clock)]);
          require(dut.observations().Events().empty());
        }
      }
      if (!message.empty()) {
        const auto before = snapshot(dut);
        const auto cycle = dut.system().cycle();
        dut.drive({bit(edge == 0), bit(false)});
        if constexpr (edge == 0) {
          // Direct Work permits read-only inspection of real prepared updates;
          // the existing system Step repeats Work and owns checking/discard.
          dut.root_->Work();
          const auto sibling = @SIBLING@;
          const auto counter = sibling->pyc_instance_value_state;
          const auto queue = sibling->@SIBLING_QUEUE@;
          const auto &register_pending = counter->pending_[0];
          const auto &queue_pending = queue->pending_[0];
          require(counter->current(0).q.value().value() == 0 && !counter->current(0).clock);
          require(register_pending.valid && register_pending.clock && register_pending.q.value().value() == 1);
          require(queue->current(0).count == 0 && queue->current(0).timing.tick == 0 && !queue->current(0).clock);
          require(queue_pending.valid && queue_pending.clock && queue_pending.push && !queue_pending.pop);
          require(queue_pending.token.value().value() == 0 && queue_pending.advance_time);
          require(snapshot(dut) == before);
        }
        require(dut.system().Step() == gfsim::SimStepResult::Failed);
        const auto info = dut.system().failureInfo();
        require(info.phase == gfsim::SimFailurePhase::Check);
        require(info.code == "source_check_failed" && info.message == message);
        require(!info.instance.empty() && !info.sourceJson.empty() && !info.checkIdJson.empty());
        require(snapshot(dut) == before && dut.system().cycle() == cycle);
        const auto sibling = @SIBLING@;
        require(sibling->pyc_instance_value_en.element(0).value().toBool());
        require(sibling->pyc_instance_value_d.element(0).value().value() == edge + 1);
        require(sibling->pyc_instance_value_clk.element(0).value().toBool() == (edge == 0));
        require(dut.observations().Events().empty());
        require(dut.system().Step() == gfsim::SimStepResult::Failed);
        require(snapshot(dut) == before && dut.system().cycle() == cycle);
        bool unavailable = false;
        try { (void)dut.sample(); } catch (const std::logic_error &) { unavailable = true; }
        require(unavailable);
      }
    }
    std::cout << "QUEUE_FAULT_ATOMIC_OK workers=" << workers << '\n';
  }
}
