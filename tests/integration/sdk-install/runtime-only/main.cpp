#include "gfsim/SimExecutor.h"

#include <array>
#include <cstdint>
#include <string_view>

class EmptySystem final : public gfsim::SimSystem {
protected:
  bool Precheck() noexcept override { return true; }
};

int main() {
  gfsim::ObservationSlots observations;
  if (!observations.Configure({}, 0))
    return 1;
  EmptySystem system;
  if (!system.AttachObservations(observations))
    return 2;
  gfsim::SimExecutor executor(system, observations, {});
  if (!executor.created())
    return 3;
  constexpr std::string_view config = "{}";
  if (executor.ConfigureJson(
          reinterpret_cast<const std::uint8_t *>(config.data()),
          config.size()) != PYCIRCUIT_MODEL_STATUS_V1_OK ||
      executor.Reset() != PYCIRCUIT_MODEL_STATUS_V1_OK)
    return 4;

  PycircuitModelStepResultV1 result{sizeof(PycircuitModelStepResultV1)};
  if (executor.Step(&result) != PYCIRCUIT_MODEL_STATUS_V1_OK ||
      result.state != PYCIRCUIT_MODEL_STEP_V1_QUIESCENT || result.epoch_time != 0 ||
      executor.cycles() != 0)
    return 5;
  return 0;
}
