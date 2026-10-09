#include "gfsim/SystemRunner.h"
#include "gfsim/SimExecutor.h"
#include "pycircuit_system.hpp"
#include "queue_source_vectors.hpp"

#include <cstdlib>
#include <iostream>
#include <source_location>
#include <string_view>
#include <type_traits>

void require(bool condition,
             std::source_location at = std::source_location::current()) {
  if (!condition) {
    std::cerr << "queue source oracle line " << at.line() << '\n';
    std::abort();
  }
}

template <unsigned Width> auto bits(std::string_view text) {
  require(text.size() == Width);
  gfsim::Bits<Width> value{0};
  for (unsigned bit = 0; bit < Width; ++bit) {
    require(text[Width - bit - 1] == '0' || text[Width - bit - 1] == '1');
    if (text[Width - bit - 1] == '1') {
      const unsigned word = bit / 64;
      value.setWord(word, value.word(word) | (std::uint64_t{1} << (bit % 64)));
    }
  }
  return value;
}

template <unsigned Width>
auto packed(std::string_view value, std::string_view known,
            std::string_view z) {
  return gfsim::FourState<Width>::fromMasks(bits<Width>(value),
                                            bits<Width>(known), bits<Width>(z));
}
template <class Wire>
void same(const Wire &actual, std::string_view value, std::string_view known,
          std::string_view z) {
  constexpr auto width = Wire::width;
  const auto expected = packed<width>(value, known, z);
  require(actual.packed().value() == expected.value());
  require(actual.packed().knownMask() == expected.knownMask());
  require(actual.packed().zMask() == expected.zMask());
}

template <unsigned Width> auto known(unsigned value) {
  return gfsim::wire<gfsim::Bits<Width>>::known(gfsim::Bits<Width>{value});
}

struct Context {
  pyc_dut &dut;
  unsigned sampled = 0;

  static void initialize(void *opaque) { require(drive(opaque, 0)); }

  static bool drive(void *opaque, std::uint64_t epoch) {
    auto &context = *static_cast<Context *>(opaque);
    if (epoch == std::size(rows))
      return false;
    require(epoch < std::size(rows));
    const auto &row = rows[epoch];
    pyc_dut::Inputs inputs;
    inputs.pyc_7079635f636c6b = known<1>(row.clk);
    inputs.pyc_7079635f727374 = known<1>(row.rst);
    inputs.valid = known<1>(row.valid);
    inputs.take = known<1>(row.take);
    inputs.data = std::remove_cvref_t<decltype(inputs.data)>::fromPacked(
        packed<input_bits>(row.data, row.data_known, row.data_z));
    context.dut.drive(inputs);
    return true;
  }

  static void sample(void *opaque, std::uint64_t epoch) {
    auto &context = *static_cast<Context *>(opaque);
    require(epoch == context.sampled + 1);
    context.compare(context.sampled);
    std::cout << "WORK " << context.sampled << '\n';
    ++context.sampled;
  }

  void compare(unsigned index) {
    auto &context = *this;
    auto output = context.dut.sample();
#ifdef Q4_MAPPING
    require(output.ready.isFullyKnown() && output.available.isFullyKnown());
    require(output.ready.packed().value().bit(0) ==
            (rows[index].expected[0] == '1'));
    require(output.available.packed().value().bit(0) ==
            (rows[index].expected[1] == '1'));
    same(output.head, rows[index].expected.substr(2),
         rows[index].expected_known.substr(2),
         rows[index].expected_z.substr(2));
#else
    same(output.result, rows[index].execution_expected,
         rows[index].execution_expected_known,
         rows[index].execution_expected_z);
#endif
  }
};

#ifdef Q4_CHECKS
void checkedRows(pyc_dut &dut, Context &context) {
  gfsim::SimExecutor executor(dut.system(), dut.observations(), {});
  require(executor.ConfigureJson(
              reinterpret_cast<const std::uint8_t *>(configuration.data()),
              configuration.size()) == PYCIRCUIT_MODEL_STATUS_V1_OK);
  require(Context::drive(&context, 0));
  require(executor.Reset() == PYCIRCUIT_MODEL_STATUS_V1_OK);
  bool failed = false;
  for (unsigned index = 0; index < std::size(rows); ++index) {
    require(Context::drive(&context, index));
    if (rows[index].host_reset) {
      // Recovery is a separately labeled host operation. The physical row and
      // its original pre-reset oracle value are retained in the vector record.
      require(executor.Reset() == PYCIRCUIT_MODEL_STATUS_V1_OK);
      failed = false;
      std::cout << "HOST_RESET " << index << '\n';
    }
    const auto before = executor.cycles();
    PycircuitModelStepResultV1 result{sizeof(result)};
    const auto status = executor.Step(&result);
    if (rows[index].execution_failure) {
      // The first failed Step reports runtime failure. Later API calls reject
      // the already failed execution without evaluating another Work.
      require(status == (failed ? PYCIRCUIT_MODEL_STATUS_V1_INVALID_STATE
                                : PYCIRCUIT_MODEL_STATUS_V1_RUNTIME_FAILURE));
      failed = true;
      require(executor.cycles() == before && dut.system().cycle() == before);
      const auto info = dut.system().failureInfo();
      require(info.phase == gfsim::SimFailurePhase::Check);
      require(info.code == "source_check_failed");
      require(!info.message.empty() && !info.instance.empty() &&
              !info.sourceJson.empty() && !info.checkIdJson.empty());
      bool unavailable = false;
      try {
        (void)dut.sample();
      } catch (const std::logic_error &) {
        unavailable = true;
      }
      require(unavailable);
      std::cout << "FAILED " << index << '\n';
    } else {
      require(!failed);
      require(status == PYCIRCUIT_MODEL_STATUS_V1_OK);
      context.compare(index);
      std::cout << "WORK " << index << '\n';
    }
    require(dut.observations().Events().empty());
  }
}
#endif

#ifdef Q6_DEAD
void deadQueueMustAge(unsigned workers) {
  pyc_dut dut(workers);
  gfsim::SimExecutor executor(dut.system(), dut.observations(), {});
  constexpr std::string_view config =
      R"({"deadlock_window":null,"max_domain_cycles":{},"max_ticks":32,"schema":"pycircuit-model-config","version":"1"})";
  require(executor.ConfigureJson(
              reinterpret_cast<const std::uint8_t *>(config.data()),
              config.size()) == PYCIRCUIT_MODEL_STATUS_V1_OK);
  pyc_dut::Inputs inputs;
  inputs.pyc_7079635f636c6b = known<1>(0);
  inputs.pyc_7079635f727374 = known<1>(0);
  inputs.valid = known<1>(0);
  inputs.take = known<1>(0);
  inputs.data = known<13>(71);
  dut.drive(inputs);
  require(executor.Reset() == PYCIRCUIT_MODEL_STATUS_V1_OK);
  PycircuitModelStepResultV1 result{sizeof(result)};
  auto step = [&] {
    dut.drive(inputs);
    require(executor.Step(&result) == PYCIRCUIT_MODEL_STATUS_V1_OK);
  };
  step();
  inputs.valid = known<1>(1);
  inputs.pyc_7079635f636c6b = known<1>(1);
  step(); // E0 reserves the sole slot, though no queue result is observed.
  inputs.valid = gfsim::wire<gfsim::Bits<1>>::unknown();
  inputs.take = gfsim::wire<gfsim::Bits<1>>::unknown();
  for (unsigned edge = 1; edge <= 2; ++edge) {
    inputs.pyc_7079635f636c6b = known<1>(0);
    step();
    inputs.pyc_7079635f636c6b = known<1>(1);
    step(); // Full ineligible masks both unknowns; E2 Xfer must mature.
  }
  inputs.pyc_7079635f636c6b = known<1>(0);
  step();
  const auto epoch = executor.cycles();
  inputs.pyc_7079635f636c6b = known<1>(1);
  dut.drive(inputs);
  require(executor.Step(&result) == PYCIRCUIT_MODEL_STATUS_V1_RUNTIME_FAILURE);
  require(executor.cycles() == epoch && dut.system().cycle() == epoch);
  bool unavailable = false;
  try {
    (void)dut.sample();
  } catch (const std::logic_error &) {
    unavailable = true;
  }
  require(unavailable);
  std::cout << "DEAD queue age witnessed by effective unknown pop workers="
            << workers << '\n';
}
#endif

int main(int argc, char **argv) {
  gfsim::SystemRunner runner(argc, argv);
  if (!runner.ready())
    return 2;
  pyc_dut dut(runner.workers());
  Context context{dut};
#ifdef Q4_CHECKS
  checkedRows(dut, context);
  return 0;
#else
  gfsim::RunnerCallbacks callbacks{&context, &Context::initialize,
                                   &Context::drive, &Context::sample};
  int status = runner.Run(dut.system(), dut.observations(), {}, callbacks);
  require(status == 0 && context.sampled == std::size(rows));
#ifdef Q6_DEAD
  deadQueueMustAge(runner.workers());
#endif
  return status;
#endif
}
