// Test adapter for the automatically generated entry; no authored functional
// DUT.
#include "pycircuit_system.hpp"
#include <array>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

using Root = pyc_root;
using RequestWire = decltype(std::declval<Root>().request);
auto flag(bool value) {
  return gfsim::wire<gfsim::Bits<1>>::known(gfsim::Bits<1>{value});
}
void require(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}
RequestWire decode(const std::string &text) {
  require(text.size() == RequestWire::width, "request width");
  using Mask = RequestWire::packed_type::Mask;
  Mask value{0};
  for (unsigned i = 0; i < RequestWire::width; ++i)
    if (text[RequestWire::width - i - 1] == '1')
      value.setWord(i / 64, value.word(i / 64) | (uint64_t{1} << (i % 64)));
  return RequestWire::fromPacked(RequestWire::packed_type::known(value));
}
class System final : public gfsim::SimSystem {
public:
  explicit System(Root &root) : root_(root) {}

private:
  bool Precheck() noexcept override {
    return root_.pyc_validate_checks(failure_);
  }
  Root &root_;
  gfsim::SimFailureInfo failure_{};
};
std::array<uint64_t, 4> state(Root &root) {
  require(root.snapshot_0.isFullyKnown() && root.snapshot_1.isFullyKnown() &&
              root.snapshot_2.isFullyKnown() && root.snapshot_3.isFullyKnown(),
          "snapshot must be known");
  return {root.snapshot_0.value().value(), root.snapshot_1.value().value(),
          root.snapshot_2.value().value(), root.snapshot_3.value().value()};
}
int main(int argc, char **argv) {
  try {
    require(argc == 4, "probe needs packed baseline, enable bit and data bit");
    auto baseline = decode(argv[1]);
    unsigned enabled = std::stoul(argv[2]), data = std::stoul(argv[3]);
    for (unsigned scenario = 0; scenario < 4; ++scenario) {
      Root root("generated-entry");
      root.request = baseline;
      root.active = flag(false);
      root.pyc_7079635f636c6b = flag(false);
      root.pyc_7079635f727374 = flag(true);
#ifdef PYC_TIMING
      root.consume = flag(true);
#endif
      System system(root);
      require(system.AddModule(root), "register");
      system.Build();
      system.Reset();
      root.pyc_7079635f727374 = flag(false);
      root.active = flag(true);
      root.pyc_7079635f636c6b = flag(true);
      require(system.Step() == gfsim::SimStepResult::Running,
              "baseline acceptance");
      root.active = flag(false);
      root.pyc_7079635f636c6b = flag(false);
      require(system.Step() == gfsim::SimStepResult::Running, "baseline low");
#ifdef PYC_TIMING
      // A timing entry must drain the older accepted request before probing an
      // inadmissible new offer. This is not functional wait-cycle execution.
      for (unsigned cycle = 0; cycle < 6; ++cycle) {
        root.pyc_7079635f636c6b = flag(true);
        require(system.Step() == gfsim::SimStepResult::Running, "timing drain");
        root.pyc_7079635f636c6b = flag(false);
        require(system.Step() == gfsim::SimStepResult::Running, "timing low");
      }
#endif
      const std::array<uint64_t, 4> expected{1, 3, 1, 0};
      require(state(root) == expected, "independent baseline state");
      root.request = baseline;
      root.active = flag(true);
      if (scenario == 0) {
        root.active = flag(false);
        root.request = RequestWire::unknown();
      } else if (scenario == 1) {
        auto packed = baseline.packed();
        packed.template assignSlice<1>(data, gfsim::FourState<1>::unknown());
        root.request = RequestWire::fromPacked(packed);
      } else if (scenario == 2) {
        root.active = decltype(root.active)::unknown();
      } else {
        auto packed = RequestWire::packed_type::unknown();
        packed.template assignSlice<1>(
            enabled, gfsim::FourState<1>::known(gfsim::Bits<1>{0}));
        root.request = RequestWire::fromPacked(packed);
      }
      root.pyc_7079635f636c6b = flag(true);
      const auto result = system.Step();
      require(result == ((scenario == 1 || scenario == 2)
                             ? gfsim::SimStepResult::Failed
                             : gfsim::SimStepResult::Running),
              "unknown/inactive admission behavior");
      // Work is the public generated API. A read-only inactive Work after
      // discard exposes committed Q without resetting or advancing the failed
      // System. No host variable restores or supplies any DUT state.
      root.active = flag(false);
      root.request = RequestWire::unknown();
      root.pyc_7079635f636c6b = flag(false);
      root.Work();
      root.DiscardNext();
      require(state(root) == expected,
              "inadmissible/inactive offer changed committed state");
      std::cout << "PROBE scenario=" << scenario << " held_state=1,3,1,0\n";
    }
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
