#include "gfsim/SimExecutor.h"
#include "pycircuit_system.hpp"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <source_location>
#include <string>
#include <string_view>
#include <type_traits>

constexpr unsigned width = @WIDTH@;
void require(bool condition, std::source_location at = std::source_location::current()) {
  if (!condition) {
    std::cerr << "serial Method probe failure at " << at.line() << '\n';
    std::abort();
  }
}
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
template <class Storage, class Wire>
void probe(const Storage &storage, const Wire &data, bool) {
  // A plain DFF has no authored enable pin; its commit condition is constant
  // one, which is what the paired RTL probe prints.
  require(storage.size() == 1);
  std::cout << ' '; print(storage.current(0).q);
  std::cout << ' '; print(data.element(0));
  std::cout << " 1";
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
template <class Wire> void require_zero(const Wire &value) {
  const auto packed = value.packed();
  for (unsigned bit = 0; bit < Wire::width; ++bit)
    require(packed.knownMask().bit(bit) && !packed.zMask().bit(bit) &&
            !packed.value().bit(bit));
}
template <class Root> void check_defaults(const Root &root) {
  @DEFAULT_CHECKS@
}
template <class Root> class DirectSystem final : public gfsim::SimSystem {
public:
  explicit DirectSystem(Root &root) : root_(root) {}
private:
  bool Precheck() noexcept override {
    if constexpr (requires { root_.__pyc_validate_checks(failure_); })
      return root_.__pyc_validate_checks(failure_);
    return true;
  }
  Root &root_;
  gfsim::SimFailureInfo failure_{};
};
#ifndef PYC_NOMINAL_JOIN
int main(int argc, char **argv) {
  require(argc == 3 || (argc == 4 && std::string_view(argv[3]) == "direct"));
  const bool direct = argc == 4;
  const unsigned workers = std::strtoul(argv[1], nullptr, 10);
  gfsim::WorkExecutor direct_executor(workers);
  std::unique_ptr<pyc_dut> dut;
  std::shared_ptr<pyc_root> root;
  std::unique_ptr<DirectSystem<pyc_root>> direct_system;
  std::unique_ptr<gfsim::SimExecutor> executor;
  constexpr std::string_view config =
      "{\"deadlock_window\":null,\"max_domain_cycles\":{},\"max_ticks\":64,"
      "\"schema\":\"pycircuit-model-config\",\"version\":\"1\"}";
  if (direct) {
    root = std::make_shared<pyc_root>("direct-root", &direct_executor);
    check_defaults(*root); // Constructor must bind before any facade/Work.
    root->pyc_7079635f636c6b = wire<1>("0");
    root->pyc_7079635f727374 = wire<1>("1");
    root->Reset();
    check_defaults(*root);
    root->DiscardNext();
    direct_system = std::make_unique<DirectSystem<pyc_root>>(*root);
    require(direct_system->AddModule(*root));
    direct_system->Build();
    direct_system->Reset();
    require(direct_system->state() == gfsim::SimSystemState::Ready);
  } else {
    dut = std::make_unique<pyc_dut>(workers);
    root = dut->root_;
    check_defaults(*root);
    executor = std::make_unique<gfsim::SimExecutor>(dut->system(), dut->observations(),
        std::span<const gfsim::ReportGaugeDescriptor>{});
    require(executor->ConfigureJson(reinterpret_cast<const std::uint8_t *>(config.data()),
                                   config.size()) == PYCIRCUIT_MODEL_STATUS_V1_OK);
    require(executor->Reset() == PYCIRCUIT_MODEL_STATUS_V1_OK);
  }
  check_defaults(*root);
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
    if (direct) {
      root->pyc_7079635f636c6b = in.pyc_7079635f636c6b;
      root->pyc_7079635f727374 = in.pyc_7079635f727374;
    } else {
      dut->drive(in);
    }
    root->Work();
    check_defaults(*root);
    std::cout << "WORK " << row;
    @PROBES@
    std::cout << '\n';
    if (discard) {
      root->DiscardNext();
      root->Xfer();
    } else if (direct) {
      require(direct_system->Step() == gfsim::SimStepResult::Running);
    } else {
      PycircuitModelStepResultV1 step{sizeof(step)};
      require(executor->Step(&step) == PYCIRCUIT_MODEL_STATUS_V1_OK);
    }
    ++row;
  }
  require(stimuli.eof() && row != 0);
}

#else
int main(int argc, char **argv) {
  require(argc == 3);
  pyc_dut dut(std::strtoul(argv[1], nullptr, 10));
  gfsim::SimExecutor executor(dut.system(), dut.observations(), {});
  constexpr std::string_view config =
      "{\"deadlock_window\":null,\"max_domain_cycles\":{},\"max_ticks\":64,"
      "\"schema\":\"pycircuit-model-config\",\"version\":\"1\"}";
  require(executor.ConfigureJson(reinterpret_cast<const std::uint8_t *>(config.data()), config.size()) == PYCIRCUIT_MODEL_STATUS_V1_OK);
  require(executor.Reset() == PYCIRCUIT_MODEL_STATUS_V1_OK);
  std::ifstream stimuli(argv[2]); require(stimuli.good());
  std::string flag, a, b, ta, tb;
  unsigned row = 0;
  while (stimuli >> flag >> a >> b >> ta >> tb) {
    require(row < 128);
    pyc_dut::Inputs in;
    in.pyc_7079635f727374 = wire<1>("0");
    in.flag = wire<1>(flag); in.a = wire<8>(a); in.b = wire<8>(b);
    in.ta = wire<8>(ta); in.tb = wire<8>(tb);
    // Every selector, including X/Z, reaches an active rising sample. A pure
    // nominal join must not acquire a selection-known admission obligation.
    for (unsigned level = 0; level != 2; ++level) {
      in.pyc_7079635f636c6b = wire<1>(level ? "1" : "0");
      dut.drive(in);
      PycircuitModelStepResultV1 step{sizeof(step)};
      require(executor.Step(&step) == PYCIRCUIT_MODEL_STATUS_V1_OK);
    }
    auto out = dut.sample();
    std::cout << "NOMINAL " << row++ << ' '; print(out.result);
    std::cout << '\n';
  }
  require(stimuli.eof() && row != 0);
}
#endif
