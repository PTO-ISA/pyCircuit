#include "gfsim/SimExecutor.h"
#include "pycircuit_system.hpp"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <type_traits>

constexpr unsigned width = @WIDTH@;
void require(bool condition) { if (!condition) std::abort(); }
template <unsigned W> auto wire(const std::string &bits) {
  require(bits.size() == W);
  gfsim::Bits<W> value{0}, known{0}, z{0};
  for (unsigned i = 0; i < W; ++i) {
    unsigned bit = W - i - 1;
    auto mask = std::uint64_t{1} << (bit % 64);
    char c = bits[i];
    require(c == '0' || c == '1' || c == 'x' || c == 'z');
    if (c == '0' || c == '1') known.setWord(bit / 64, known.word(bit / 64) | mask);
    if (c == '1') value.setWord(bit / 64, value.word(bit / 64) | mask);
    if (c == 'z') z.setWord(bit / 64, z.word(bit / 64) | mask);
  }
  return gfsim::wire<gfsim::Bits<W>>::fromPacked(
      gfsim::FourState<W>::fromMasks(value, known, z));
}
template <class Wire> void print(const Wire &v) {
  const auto packed = v.packed();
  for (unsigned bit = Wire::width; bit-- > 0;)
    std::cout << (packed.zMask().bit(bit) ? 'z' : !packed.knownMask().bit(bit)
                  ? 'x' : packed.value().bit(bit) ? '1' : '0');
}
template <class Storage, class Wire, class Enable>
void probe(const Storage &storage, const Wire &data, const Enable &enable) {
  require(storage.size() == 1);
  std::cout << ' '; print(storage.current(0).q);
  std::cout << ' '; print(data.element(0));
  std::cout << ' '; print(enable.element(0));
  std::cout << ' ' << storage.current(0).clock;
}
template <class Storage, class Pin>
void inject(Storage &storage, Pin &pin, const std::string &bits) {
  // Test stimulus changes the real committed payload, never a calculated DUT
  // result. The object is mutable; current() is its existing read-only view.
  using Current = std::remove_cvref_t<decltype(storage.current(0))>;
  auto &current = const_cast<Current &>(storage.current(0));
  current.q = wire<width>(bits);
  pin.element(0) = current.q;
}
int main(int argc, char **argv) {
  require(argc == 3);
  pyc_dut dut(std::strtoul(argv[1], nullptr, 10));
  gfsim::SimExecutor executor(dut.system(), dut.observations(), {});
  constexpr std::string_view config =
      "{\"deadlock_window\":null,\"max_domain_cycles\":{},\"max_ticks\":64,"
      "\"schema\":\"pycircuit-model-config\",\"version\":\"1\"}";
  require(executor.ConfigureJson(reinterpret_cast<const std::uint8_t *>(config.data()),
                                config.size()) == PYCIRCUIT_MODEL_STATUS_V1_OK);
  require(executor.Reset() == PYCIRCUIT_MODEL_STATUS_V1_OK);
  const auto root = dut.root_;
  const auto family = root->pyc_implementation;
  std::ifstream stimuli(argv[2]);
  require(stimuli.good());
  unsigned clock, reset, discard, row = 0;
  std::string count, payload;
  while (stimuli >> clock >> reset >> discard >> count >> payload) {
    require(row < 64 && clock <= 1 && reset <= 1 && discard <= 1);
    @INJECT@
    pyc_dut::Inputs in;
    in.pyc_7079635f636c6b = wire<1>(clock ? "1" : "0");
    in.pyc_7079635f727374 = wire<1>(reset ? "1" : "0");
    dut.drive(in);
    root->Work();
    std::cout << "WORK " << row;
    @PROBES@
    std::cout << '\n';
    if (discard) {
      root->DiscardNext();
      root->Xfer();
    } else {
      PycircuitModelStepResultV1 step{sizeof(step)};
      require(executor.Step(&step) == PYCIRCUIT_MODEL_STATUS_V1_OK);
    }
    ++row;
  }
  require(stimuli.eof() && row != 0);
}
