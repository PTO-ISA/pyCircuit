#include "pycircuit_system.hpp"
#include <cstdlib>
#include <iostream>
#include <source_location>

void require(bool ok, std::source_location at = std::source_location::current()) {
  if (!ok) { std::cerr << "guard order oracle line " << at.line() << '\n'; std::abort(); }
}
template<unsigned W> auto known(unsigned value) {
  return gfsim::wire<gfsim::Bits<W>>::known(gfsim::Bits<W>{value});
}
auto control(unsigned value) {
  using B = gfsim::Bits<1>;
  if (value < 2) return known<1>(value);
  return gfsim::wire<B>::fromPacked(gfsim::FourState<1>::fromMasks(
      B{0}, B{0}, B{value == 3 ? 1u : 0u}));
}
void equal(gfsim::FourState<8> value, unsigned expected) {
  require(value.invariantHolds());
  require(value.knownMask().value() == 255 && value.zMask().value() == 0);
  require(value.value().value() == expected);
}
struct TestSystem final : gfsim::SimSystem {
  pyc_root &root;
  explicit TestSystem(pyc_root &value) : root(value) {}
  bool Precheck() noexcept override {
    gfsim::SimFailureInfo failure;
    return root.pyc_validate_checks(failure);
  }
};
int main(int argc, char **argv) {
  require(argc == 2);
  unsigned workers = std::atoi(argv[1]);
  for (unsigned outer = 0; outer != 4; ++outer)
    for (unsigned inner = 0; inner != 4; ++inner) {
      gfsim::WorkExecutor pool(workers);
      pyc_root root("guard-order", &pool);
      TestSystem system(root);
      root.outer = root.inner = known<1>(1);
      root.data = known<8>(17);
      root.pyc_7079635f636c6b = root.pyc_7079635f727374 = known<1>(0);
      require(system.AddModule(root)); system.Build(); system.Reset();
      require(system.Step() == gfsim::SimStepResult::Running);
      root.pyc_7079635f636c6b = known<1>(1);
      require(system.Step() == gfsim::SimStepResult::Running);
      root.pyc_7079635f636c6b = known<1>(0);
      require(system.Step() == gfsim::SimStepResult::Running);
      equal(root.result_0.packed(), 17); equal(root.result_1.packed(), 1);
      root.outer = control(outer); root.inner = control(inner);
      root.data = known<8>(29); root.pyc_7079635f636c6b = known<1>(1);
      auto epoch = system.cycle();
      // Independent truth table: always check the reached outer guard first.
      bool fault = outer >= 2 || (outer == 1 && inner >= 2);
      auto result = system.Step();
      require(result == (fault ? gfsim::SimStepResult::Failed : gfsim::SimStepResult::Running));
      require(system.cycle() == epoch + (fault ? 0 : 1));
      // Read committed storage without another commit, including after failure.
      root.outer = root.inner = known<1>(0);
      root.pyc_7079635f636c6b = known<1>(0);
      root.Work();
      equal(root.result_0.packed(), !fault && outer == 1 && inner == 1 ? 29 : 17);
      equal(root.result_1.packed(), fault ? 1 : 2);
      root.DiscardNext(); root.Xfer();
      std::cout << outer << ' ' << inner << ' ' << fault << '\n';
    }
}
