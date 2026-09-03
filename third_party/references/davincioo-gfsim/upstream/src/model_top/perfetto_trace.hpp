#pragma once

#include <filesystem>

#include "davincioo/model/pto_inst.hpp"

namespace davincioo::model_top {

void WritePerfettoTrace(const std::filesystem::path& path, const SimulationResult& result);

}  // namespace davincioo::model_top
