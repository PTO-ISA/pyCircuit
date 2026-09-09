#include "gfsim/model_api.h"

#include <cstdint>
#include <string_view>

int main() {
  const AgenticModelApiV1 *api = agentic_model_query_v1();
  if (!api || api->abi_version != 1 || api->struct_size != sizeof(*api))
    return 1;
  AgenticModelV1 *model = nullptr;
  if (api->create(&model) != AGENTIC_MODEL_STATUS_V1_OK || !model)
    return 2;
  const auto bytes = [](std::string_view value) {
    return reinterpret_cast<const uint8_t *>(value.data());
  };
  constexpr std::string_view config = "{}\n";
  if (api->configure_json(model, bytes(config), config.size()) !=
          AGENTIC_MODEL_STATUS_V1_OK ||
      api->reset(model) != AGENTIC_MODEL_STATUS_V1_OK) {
    api->destroy(model);
    return 3;
  }
  AgenticModelStepResultV1 step{};
  step.struct_size = sizeof(step);
  if (api->step(model, &step) != AGENTIC_MODEL_STATUS_V1_OK) {
    api->destroy(model);
    return 4;
  }
  AgenticModelBufferV1 output{};
  if (api->statistics_json(model, &output) != AGENTIC_MODEL_STATUS_V1_OK) {
    api->destroy(model);
    return 5;
  }
  api->destroy(model);
  return 0;
}
