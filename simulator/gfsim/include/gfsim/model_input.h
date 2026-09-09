#ifndef GFSIM_MODEL_INPUT_H
#define GFSIM_MODEL_INPUT_H

#include "gfsim/core.h"

#include <string>
#include <string_view>

namespace gfsim {

bool parseModelConfigJson(std::string_view input, RuntimeLimits &limits,
                          std::string &error);

} // namespace gfsim

#endif // GFSIM_MODEL_INPUT_H
