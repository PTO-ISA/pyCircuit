#include "pycircuit_system.hpp"
#include <gfsim/SystemRunner.h>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <type_traits>

// Test-owned, read-only access to the emitted hierarchy. No state is initialized
// or changed here; the ordinary public Reset/Step lifecycle owns all transfers.
template <class W> W known(uint64_t value) {
  using P = typename W::packed_type;
  return W::fromPacked(P::known(gfsim::Bits<W::width>{value}));
}
template <class W> std::string bits(const W &wire) {
  const auto value = wire.packed();
  if (!value.isFullyKnown()) throw std::runtime_error("unknown snapshot plane");
  std::string result;
  for (unsigned bit = W::width; bit-- > 0;) result += value.value().bit(bit) ? '1' : '0';
  return result;
}
template <class W> uint64_t unsigned_value(const W &wire) {
  const auto value = wire.packed();
  if (!value.isFullyKnown()) throw std::runtime_error("unknown control plane");
  return value.value().word(0);
}
std::string integer_bits(uint64_t value, unsigned width) {
  std::string result;
  for (unsigned bit = width; bit-- > 0;) result += (value >> bit) & 1 ? '1' : '0';
  return result;
}
template <class Storage> std::string queue_bits(const Storage &storage, unsigned count_width) {
  const auto &current = storage.current(0);
  constexpr auto depth = std::tuple_size_v<std::remove_reference_t<decltype(current.storage)>>;
  using W = typename std::remove_reference_t<decltype(current.storage)>::value_type;
  auto result = integer_bits(current.count, count_width);
  for (std::size_t item = 0; item < depth; ++item)
    result += item < current.count ? bits(current.storage[(current.rd + item) % depth]) : std::string(W::width, '0');
  return result;
}

struct Context {
  pyc_dut &dut;
  std::function<std::string()> snapshot, controls;
  pyc_dut::Inputs input{};
  std::string before, work, low_controls;
  static void initialize(void *opaque) {
    auto &self = *static_cast<Context *>(opaque);
    self.input.pyc_7079635f636c6b = known<decltype(self.input.pyc_7079635f636c6b)>(0);
    self.input.pyc_7079635f727374 = known<decltype(self.input.pyc_7079635f727374)>(0);
    self.dut.drive(self.input);
  }
  static bool drive(void *opaque, std::uint64_t epoch) {
    auto &self = *static_cast<Context *>(opaque);
    if (epoch >= @EPOCHS@ * 2) return false;
    if ((epoch & 1) == 0) self.before = self.snapshot();
    self.input.pyc_7079635f636c6b = known<decltype(self.input.pyc_7079635f636c6b)>(epoch & 1);
    self.dut.drive(self.input);
    return true;
  }
  static void sample(void *opaque, std::uint64_t epoch) {
    auto &self = *static_cast<Context *>(opaque);
    if (epoch & 1) {
      self.work = self.snapshot();
      self.low_controls = self.controls();
    } else {
      if (self.low_controls != self.controls()) throw std::runtime_error("old-Q physical pair changed");
      std::cout << "ROW " << epoch / 2 - 1 << ' ' << self.before << ' ' << self.work << ' ' << self.snapshot() << ' ' << self.low_controls << '\n';
    }
  }
};
int main(int argc, char **argv) {
  constexpr unsigned epochs = @EPOCHS@;
  static_assert(epochs > 0 && epochs <= 256);
  gfsim::SystemRunner runner(argc, argv, R"json({"deadlock_window":null,"max_domain_cycles":{},"max_ticks":1024,"schema":"pycircuit-model-config","version":"1"})json");
  if (!runner.ready()) return 2;
  pyc_dut dut(runner.workers());
  @CONFIG@
  Context context{dut, snapshot, controls};
  const gfsim::RunnerCallbacks callbacks{&context, &Context::initialize, &Context::drive, &Context::sample};
  return runner.Run(dut.system(), dut.observations(), pyc_observation_metadata(), callbacks);
}
