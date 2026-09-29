#include "generated/dut.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <optional>

bool check_unpaired_input() {
  gfsim::SimSystem system{"unpaired_input"};
  ac_generated::QueueAdderSystem model;
  std::array<gfsim::TimeDomainRuntime, 1> domains{{{"cycle", 1, 0, 1}}};
  if (!system.root().attachChild(model) || !system.setTimeDomains(domains) ||
      !model.configure_activation_scheduler(system) ||
      !model.offer_left(system, gfsim::UInt<32>{5}))
    return false;

  system.step();  // Commit the left token.
  system.step();  // Try the adder rule without a right token.
  return model.left().canProposePop() &&
         !model.result_0().canProposePop();
}

std::optional<uint32_t> run_pair(uint32_t left, uint32_t right) {
  gfsim::SimSystem system{"queue_adder"};
  ac_generated::QueueAdderSystem model;
  std::array<gfsim::TimeDomainRuntime, 1> domains{{{"cycle", 1, 0, 1}}};
  if (!system.root().attachChild(model) || !system.setTimeDomains(domains) ||
      !model.configure_activation_scheduler(system) ||
      !model.offer_left(system, gfsim::UInt<32>{left}) ||
      !model.offer_right(system, gfsim::UInt<32>{right}))
    return std::nullopt;

  for (int tick = 0; tick < 8 && system.step(); ++tick)
    if (auto result = model.try_take_result_0(system))
      return static_cast<uint32_t>(result->value());
  return std::nullopt;
}

int main() {
  const auto first = run_pair(5, 7);
  const auto second = run_pair(UINT32_MAX, 2);
  if (!check_unpaired_input() || first != 12 || second != 1)
    return 1;
  std::cout << "sums=" << *first << "," << *second << '\n';
  return 0;
}
