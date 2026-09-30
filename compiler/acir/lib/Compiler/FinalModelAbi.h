#pragma once
#include <string>

namespace acir::compiler {

struct FinalModelAbiParts {
  std::string header;
  std::string source;
};

FinalModelAbiParts buildFinalModelAbiParts();

} // namespace acir::compiler
