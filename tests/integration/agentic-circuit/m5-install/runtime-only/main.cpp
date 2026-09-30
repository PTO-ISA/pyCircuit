#include "gfsim/SimExecutor.h"

#include <array>
#include <cstdint>
#include <string_view>

class EmptySystem final : public gfsim::SimSystem {
protected:
  bool Precommit(std::uint64_t) noexcept override { return true; }
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
          config.size()) != AGENTIC_MODEL_STATUS_V1_OK ||
      executor.Reset() != AGENTIC_MODEL_STATUS_V1_OK)
    return 4;

  AgenticModelStepResultV1 result{sizeof(AgenticModelStepResultV1)};
  if (executor.Step(&result) != AGENTIC_MODEL_STATUS_V1_OK ||
      result.state != AGENTIC_MODEL_STEP_V1_QUIESCENT || result.epoch_time != 0 ||
      executor.cycles() != 0)
    return 5;
  return 0;
}
