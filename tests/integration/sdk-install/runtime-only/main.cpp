#include "gfsim/SimExecutor.h"
#include "gfsim/dff.h"

#include <array>
#include <cstdint>
#include <string_view>

class EmptySystem final : public gfsim::SimSystem {
protected:
  bool Precheck() noexcept override { return true; }
};

int main() {
  const std::array explicitDescriptors{gfsim::ObservationDescriptor{
      0, 1, 1, 1, gfsim::ObservationKind::Event, 0, 1}};
  gfsim::ObservationSlots explicitObservations;
  if (!explicitObservations.Configure(explicitDescriptors, 1) ||
      !explicitObservations.Stage(0, gfsim::SlotValue::Bool(true), true, true,
                                  0) ||
      !explicitObservations.Check() || !explicitObservations.Precommit(0))
    return 1;
  explicitObservations.Xfer(1);
  if (explicitObservations.Events().size() != 1 ||
      explicitObservations.Events()[0].value != gfsim::SlotValue::Bool(true))
    return 2;

  gfsim::ObservationSlots observations;
  if (!observations.Configure({}, 0))
    return 3;
  EmptySystem system;
  if (!system.AttachObservations(observations))
    return 4;
  gfsim::SimExecutor executor(system, observations, {});
  if (!executor.created())
    return 5;
  constexpr std::string_view config = "{}";
  if (executor.ConfigureJson(
          reinterpret_cast<const std::uint8_t *>(config.data()),
          config.size()) != PYCIRCUIT_MODEL_STATUS_V1_OK ||
      executor.Reset() != PYCIRCUIT_MODEL_STATUS_V1_OK)
    return 6;

  PycircuitModelStepResultV1 result{sizeof(PycircuitModelStepResultV1)};
  if (executor.Step(&result) != PYCIRCUIT_MODEL_STATUS_V1_OK ||
      result.state != PYCIRCUIT_MODEL_STEP_V1_QUIESCENT ||
      result.epoch_time != 0 || executor.cycles() != 0)
    return 7;

  using Bit = gfsim::Bits<1>;
  using Word = gfsim::Bits<9>;
  auto control = [](bool value) {
    return gfsim::wire<Bit>::known(Bit{value ? 1u : 0u});
  };
  auto word = [](unsigned value) {
    return gfsim::wire<Word>::known(Word{value});
  };
  gfsim::dffe<Word> reg("runtime-only.reg");
  reg.init = word(3);
  reg.Reset();
  reg.Xfer();
  if (!reg.q.isFullyKnown() || reg.q.value() != Word{3})
    return 8;
  reg.clk = control(true);
  reg.rst = control(false);
  reg.en = control(true);
  reg.d = word(17);
  reg.Work();
  reg.Xfer();
  if (!reg.q.isFullyKnown() || reg.q.value() != Word{17})
    return 9;
  return 0;
}
