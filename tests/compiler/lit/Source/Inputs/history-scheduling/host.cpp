#include "pycircuit_system.hpp"
#include "scheduling_vectors.hpp"
#include <cstdlib>
#include <iostream>
#include <source_location>
#include <sstream>
#include <string>
#include <type_traits>

void require(bool condition, std::source_location at = std::source_location::current()) {
  if (!condition) { std::cerr << "scheduler oracle line " << at.line() << '\n'; std::abort(); }
}
template <unsigned Width> auto known(std::uint64_t value) {
  return gfsim::wire<gfsim::Bits<Width>>::known(gfsim::Bits<Width>{value});
}
template <class Wire> void save_wire(std::ostream &out, const Wire &wire) {
  const auto packed = wire.packed();
  for (unsigned bit = Wire::width; bit-- > 0;)
    out << (packed.zMask().bit(bit) ? 'z' : !packed.knownMask().bit(bit) ? 'x' : packed.value().bit(bit) ? '1' : '0');
  out << '|';
}
template <class Storage> void save(std::ostream &out, const Storage &storage) {
  for (std::size_t lane = 0; lane < storage.size(); ++lane) {
    const auto &state = storage.current(lane);
    out << state.clock << '|';
    if constexpr (requires { state.q; }) save_wire(out, state.q);
    if constexpr (requires { state.storage; }) {
      out << state.rd << '|' << state.wr << '|' << state.count << '|' << state.initialized << '|';
      for (const auto &token : state.storage) save_wire(out, token);
      if constexpr (requires { state.timing.tick; }) {
        out << state.timing.tick << '|' << state.timing.mature_ptr << '|' << state.timing.eligible_count << '|';
        for (auto deadline : state.timing.deadline) out << deadline << '|';
      }
    }
  }
}
std::string snapshot(pyc_dut &dut) {
  std::ostringstream out;
  @STATE_SNAPSHOTS@
  return out.str();
}
int main() {
  for (unsigned workers : {1u, 2u}) {
    pyc_dut dut(workers);
    pyc_dut::Inputs inputs;
    inputs.valid = known<1>(0); inputs.take = known<1>(0);
    inputs.data = std::remove_cvref_t<decltype(inputs.data)>::fromPacked(known<input_bits>(0).packed());
    inputs.pyc_7079635f636c6b = known<1>(0); inputs.pyc_7079635f727374 = known<1>(0);
    dut.drive(inputs); dut.system().Build();
    for (const auto &history : histories) {
      inputs.pyc_7079635f636c6b = known<1>(0); dut.drive(inputs); dut.system().Reset();
      for (std::size_t index = history.begin; index < history.end; ++index) {
        const auto &row = rows[index];
        inputs.valid = known<1>(row.valid); inputs.take = known<1>(row.take);
        inputs.data = std::remove_cvref_t<decltype(inputs.data)>::fromPacked(known<input_bits>(row.token).packed());
        inputs.pyc_7079635f636c6b = known<1>(row.clock);
        dut.drive(inputs);
        if (row.host_reset) dut.system().Reset();
        const auto cycle = dut.system().cycle();
        const auto before = row.failure ? snapshot(dut) : std::string{};
        const auto status = dut.system().Step();
        if (row.failure) {
          require(status == gfsim::SimStepResult::Failed);
          require(dut.system().cycle() == cycle && snapshot(dut) == before);
          const auto info = dut.system().failureInfo();
          require(info.phase == gfsim::SimFailurePhase::Check && info.code == "source_check_failed");
          require(info.message == row.message && !info.instance.empty() && !info.sourceJson.empty() && !info.checkIdJson.empty());
          bool unavailable = false;
          try { (void)dut.sample(); } catch (const std::logic_error &) { unavailable = true; }
          require(unavailable);
        } else {
          require(status == gfsim::SimStepResult::Running);
          const auto result = dut.sample().result;
          if (!result.isFullyKnown() || result.packed().value() != gfsim::Bits<output_bits>{row.expected}) {
            std::cerr << history.name << " view " << index - history.begin << " expected " << row.expected << '\n';
            require(false);
          }
        }
        require(dut.observations().Events().empty());
      }
      std::cout << "SCHEDULING_HISTORY_OK " << history.name << " views=" << history.end-history.begin << " workers=" << workers << '\n';
    }
  }
}
