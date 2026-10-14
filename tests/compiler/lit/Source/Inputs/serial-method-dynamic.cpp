#include "gfsim/SimExecutor.h"
#include "pycircuit_system.hpp"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>

void require(bool condition) {
  if (!condition)
    std::abort();
}

template <unsigned W> auto wire(const std::string &bits) {
  require(bits.size() == W);
  gfsim::Bits<W> value{0}, known{0}, z{0};
  for (unsigned i = 0; i < W; ++i) {
    const unsigned bit = W - i - 1;
    const auto mask = std::uint64_t{1} << (bit % 64);
    const char c = bits[i];
    require(c == '0' || c == '1' || c == 'x' || c == 'z');
    if (c == '0' || c == '1')
      known.setWord(bit / 64, known.word(bit / 64) | mask);
    if (c == '1')
      value.setWord(bit / 64, value.word(bit / 64) | mask);
    if (c == 'z')
      z.setWord(bit / 64, z.word(bit / 64) | mask);
  }
  return gfsim::wire<gfsim::Bits<W>>::fromPacked(
      gfsim::FourState<W>::fromMasks(value, known, z));
}

template <class Wire> void print(const Wire &value) {
  const auto packed = value.packed();
  for (unsigned bit = Wire::width; bit-- > 0;)
    std::cout << (packed.zMask().bit(bit)        ? 'z'
                  : !packed.knownMask().bit(bit) ? 'x'
                  : packed.value().bit(bit)      ? '1'
                                                 : '0');
}

template <class Storage> void probe(const Storage &storage) {
  require(storage.size() == 1);
  std::cout << ' ';
  print(storage.current(0).q);
  std::cout << ' ' << storage.current(0).clock;
}

template <unsigned W, class Storage, class Pin>
void inject(Storage &storage, Pin &pin, const std::string &bits) {
  require(storage.size() == 1);
  // Stimulus changes the real committed gate and its Q pin before Work.
  using Current = std::remove_cvref_t<decltype(storage.current(0))>;
  auto &current = const_cast<Current &>(storage.current(0));
  current.q = wire<W>(bits);
  pin.element(0) = current.q;
}

std::string_view failure_status(const gfsim::SimFailureInfo &failure) {
  if (failure.code == "source_check_failed")
    return "guard";
  require(failure.code == "runtime_failure");
  return "physical";
}

std::string_view abi_status(PycircuitModelStatusV1 status) {
  switch (status) {
  case PYCIRCUIT_MODEL_STATUS_V1_OK:
    return "ok";
  case PYCIRCUIT_MODEL_STATUS_V1_RUNTIME_FAILURE:
    return "runtime_failure";
  case PYCIRCUIT_MODEL_STATUS_V1_INVALID_STATE:
    return "invalid_state";
  default:
    std::abort();
  }
}

int main(int argc, char **argv) {
  require(argc == 3);
  const unsigned workers = std::strtoul(argv[1], nullptr, 10);
  require(workers == 1 || workers == 2);
  pyc_dut dut(workers);
  const auto root = dut.root_;
  const auto family = root->pyc_implementation;
  gfsim::SimExecutor executor(dut.system(), dut.observations(), {});
  constexpr std::string_view config =
      "{\"deadlock_window\":null,\"max_domain_cycles\":{},\"max_ticks\":256,"
      "\"schema\":\"pycircuit-model-config\",\"version\":\"1\"}";
  require(executor.ConfigureJson(
              reinterpret_cast<const std::uint8_t *>(config.data()),
              config.size()) == PYCIRCUIT_MODEL_STATUS_V1_OK);
  require(executor.Reset() == PYCIRCUIT_MODEL_STATUS_V1_OK);
  std::ifstream stimuli(argv[2]);
  require(stimuli.good());
  unsigned row = 0;
  std::string line;
  while (std::getline(stimuli, line)) {
    require(row < 256);
    std::istringstream input(line);
    std::string op, clock, reset, lf, ls, rf, rs, sf, ss, extra;
    require(static_cast<bool>(input >> op >> clock >> reset >> lf >> ls >> rf >>
                              rs >> sf >> ss));
    require(!(input >> extra));
    require(op.size() == 1 &&
            (op == "S" || op == "R" || op == "D" || op == "F" || op == "T"));
    for (const auto *token : {&clock, &reset, &lf, &ls, &rf, &rs, &sf, &ss})
      (void)wire<1>(*token);

    require(executor.cycles() == dut.system().cycle());
    const auto cycleBefore = executor.cycles();
    // Direct Work lifecycle operations have no C ABI result status.
    std::string_view resultStatus = "direct";
    std::string_view status = "ok";
    if (op == "T") {
      require(executor.state() == gfsim::SimExecutorState::Failed &&
              dut.system().state() == gfsim::SimSystemState::Failed);
      // Make the pins valid without injecting gates or altering committed
      // state/history. Failed execution still requires explicit Reset.
      pyc_dut::Inputs pins;
      pins.pyc_7079635f636c6b = wire<1>("0");
      pins.pyc_7079635f727374 = wire<1>("0");
      dut.drive(pins);
      std::cout << "NATIVE_PRE " << row;
      @PROBES@
      std::cout << '\n';
      PycircuitModelStepResultV1 result{sizeof(result)};
      const auto resultCode = executor.Step(&result);
      require(resultCode == PYCIRCUIT_MODEL_STATUS_V1_INVALID_STATE);
      resultStatus = abi_status(resultCode);
      std::cout << "NATIVE_POST " << row << " sticky";
      @PROBES@
      std::cout << '\n';
    } else {
      @INJECT@
      pyc_dut::Inputs pins;
      pins.pyc_7079635f636c6b = wire<1>(clock);
      pins.pyc_7079635f727374 = wire<1>(reset);
      dut.drive(pins);
      std::cout << "PRE " << row;
      @PROBES@
      std::cout << '\n';
      if (op == "R") {
        const auto resultCode = executor.Reset();
        require(resultCode == PYCIRCUIT_MODEL_STATUS_V1_OK);
        resultStatus = abi_status(resultCode);
        status = "reset";
      } else if (op == "S") {
        PycircuitModelStepResultV1 result{sizeof(result)};
        const auto resultCode = executor.Step(&result);
        resultStatus = abi_status(resultCode);
        if (resultCode == PYCIRCUIT_MODEL_STATUS_V1_INVALID_STATE)
          status = "sticky";
        else if (resultCode != PYCIRCUIT_MODEL_STATUS_V1_OK) {
          require(resultCode == PYCIRCUIT_MODEL_STATUS_V1_RUNTIME_FAILURE);
          status = failure_status(dut.system().failureInfo());
        }
      } else {
        bool worked = false;
        try {
          root->Work();
          worked = true;
        } catch (...) {
          status = "physical";
        }
        if (op == "D") {
          root->DiscardNext();
          root->Xfer();
          if (worked)
            status = "discard";
        } else {
          pins.pyc_7079635f636c6b = wire<1>("x");
          pins.pyc_7079635f727374 = wire<1>("x");
          dut.drive(pins);
          gfsim::SimFailureInfo failure{};
          if (worked && root->__pyc_validate_checks(failure))
            root->Xfer();
          else {
            if (worked)
              status = failure_status(failure);
            root->DiscardNext();
          }
        }
      }
      std::cout << "POST " << row << ' ' << status;
      @PROBES@
      std::cout << '\n';
    }
    require(executor.cycles() == dut.system().cycle());
    std::cout << "NATIVE " << row << ' ' << cycleBefore << ' '
              << executor.cycles() << ' ' << resultStatus << '\n';
    ++row;
  }
  require(stimuli.eof() && row != 0);
}
