#ifndef GFSIM_MODEL_INPUT_H
#define GFSIM_MODEL_INPUT_H

#include <cstdint>
#include <map>
#include <optional>

#include <string>
#include <string_view>

namespace gfsim {

struct RuntimeLimits {
  std::optional<uint64_t> deadlockWindow;
  std::optional<uint64_t> maxTicks;
  std::map<std::string, uint64_t> maxDomainCycles;
};

bool parseModelConfigJson(std::string_view input, RuntimeLimits &limits,
                          std::string &error);

} // namespace gfsim

#endif // GFSIM_MODEL_INPUT_H
