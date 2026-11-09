#include "design.hpp"
#include <cstdlib>
#include <iostream>
#include <source_location>
#include <sstream>
#include <string>

void require(bool condition,
             std::source_location at = std::source_location::current()) {
  if (!condition) {
    std::cerr << "queue fault oracle failed at line " << at.line() << '\n';
    std::abort();
  }
}
template <class Wire> void wire(std::ostream &out, const Wire &value) {
  const auto packed = value.packed();
  for (unsigned bit = Wire::width; bit-- > 0;)
    out << (packed.zMask().bit(bit) ? 'z' : !packed.knownMask().bit(bit) ? 'x' : packed.value().bit(bit) ? '1' : '0');
  out << '|';
}
template <class Storage> void save(std::ostream &out, const Storage &storage) {
  const auto &state = [&]() -> const auto & {
    if constexpr (requires { storage.currentState(); })
      return storage.currentState();
    else
      return storage.current_;
  }();
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
          // The failure contract observes the Work view, before the successful
          // transaction commits.  The pipeline child refreshes its temporal
          // output during Xfer, so reading that internal port after Step would
          // incorrectly compare the next committed view with this row.
          dut.root().Work();
          std::ostringstream observed;
          wire(observed, @RESULT@);
          require(observed.str() == expected[epoch * 2 + unsigned(clock)]);
          require(dut.system().Step() == gfsim::SimStepResult::Running);
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
          dut.root().Work();
          const auto &sibling = @SIBLING@;
          const auto &counter = sibling.pyc_value;
          const auto &queue = sibling.@SIBLING_QUEUE@;
          const auto &register_pending = counter.pending_;
          const auto &queue_pending = queue.pending_;
          require(counter.currentState().q.value().value() == 0 && !counter.currentState().clock);
          require(register_pending.valid && register_pending.clock && register_pending.q.value().value() == 1);
          require(queue.current_.count == 0 && queue.current_.timing.tick == 0 && !queue.current_.clock);
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
        const auto &sibling = @SIBLING@;
        // The concrete plan may refresh sibling pins during check/discard;
        // committed storage and clock history are the atomicity authority.
        require(sibling.pyc_value.clk.value().toBool() == (edge == 0));
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
