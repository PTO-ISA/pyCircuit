#include "pycircuit_system.hpp"
#include <array>
#include <cstdlib>
#include <iostream>
#include <string_view>

template <class DUT> bool validate(DUT &dut) {
  if constexpr (requires { dut.pyc_get_check_valid_0(0); }) {
    if (!dut.pyc_get_check_valid_0(0) || !dut.pyc_get_check_reset_valid(0))
      return false;
    if (!dut.pyc_get_sampling_valid_0(0)) return false;
    const auto &sample = dut.pyc_get_sampling_evaluation_0(0);
    if (sample.fault != gfsim::SampledClockFault::None) return false;
    if (!sample.rising || sample.resetActive) return true;
    const auto error = gfsim::bit_and(
        gfsim::bit_and(dut.pyc_get_check_path_0(0).packed(),
                      gfsim::bit_not(dut.pyc_get_check_reset(0).packed())),
        gfsim::bit_not(dut.pyc_get_check_condition_0(0).packed()));
    return error.isFullyKnown() && error.value() == gfsim::Bits<1>{0};
  }
  return true;
}

int main(int argc, char **argv) {
  gfsim::WorkExecutor executor(argc > 1 ? std::atoi(argv[1]) : 1);
  const bool failure = argc > 2 && std::string_view(argv[2]) == "failure";
  queue_timing::timing::pyc_family_Timing<1> dut("queue", &executor);
  auto bit = [](unsigned value) {
    return gfsim::wire<gfsim::Bits<1>>::known(gfsim::Bits<1>{value});
  };
  dut.pyc_7079635f636c6b.element(0) = bit(0);
  dut.pyc_7079635f727374.element(0) = bit(0);
  dut.Reset(); dut.Xfer();
  auto sample = [&](unsigned clock, unsigned reset = 0, bool accepted = true) {
    dut.pyc_7079635f636c6b.element(0) = bit(clock);
    dut.pyc_7079635f727374.element(0) = bit(reset);
    dut.Work();
    if (validate(dut) != accepted) {
      std::cerr << "check outcome clock=" << clock << " reset=" << reset
                << " expected=" << accepted << '\n';
      std::abort();
    }
    if (accepted) dut.Xfer(); else dut.DiscardNext();
  };
  // phase, pushes, replacements, pops, last popped payload. These rows follow
  // old occupancy: empty phase0 cannot bypass; full phase4 cannot borrow room;
  // same-transaction pop+push succeeds only at middle occupancy (phases1,5).
  constexpr unsigned expected[][5] = {
    {0,0,0,0,0}, {1,1,0,0,0}, {2,1,1,0,0}, {3,2,1,0,0},
    {4,2,1,0,0}, {5,2,1,1,12}, {6,2,2,1,12}, {7,2,2,2,23},
    {8,2,2,2,23}, {9,3,2,2,23}, {10,3,2,3,22}
  };
  auto check = [&](unsigned phase) {
    // Direct kernel inspection is confined to this failure/retry adapter.
    // Independently derived pre-failure state: phase4 removed 12, leaving 22.
    if (failure && phase == 5) {
      const auto &queue = dut.QUEUE_STATE->current(0);
      if (queue.count != 1 || !queue.storage[queue.rd].isFullyKnown() ||
          queue.storage[queue.rd].value() != gfsim::Bits<8>{22}) {
        std::cerr << "Queue changed after failure: count=" << queue.count << '\n';
        std::abort();
      }
    }
    const auto actual = std::array{dut.out0.element(0), dut.out1.element(0), dut.out2.element(0), dut.out3.element(0), dut.out4.element(0)};
    for (unsigned i = 0; i < actual.size(); ++i)
      if (!actual[i].isFullyKnown() || actual[i].value() != gfsim::Bits<8>{expected[phase][i]})
        { std::cerr << "state mismatch phase=" << phase << " output=" << i << '\n'; std::abort(); }
  };
  for (unsigned pass = 0; pass < 2; ++pass) {
    for (unsigned phase = 0; phase <= (failure || pass == 0 ? 5 : 10); ++phase) {
      sample(0); check(phase);
      if (phase == 5 && failure) {
        sample(1, 0, false);
        sample(0); check(5);
        sample(1, 0, false); // A second failed attempt cannot consume or update.
        sample(0); check(5);
        break;
      }
      if (phase < (pass == 0 ? 5 : 10)) sample(1);
    }
    // Reset a nonempty Queue and all counters; the complete trace must repeat.
    sample(1, 1); sample(0); check(0);
  }
}
