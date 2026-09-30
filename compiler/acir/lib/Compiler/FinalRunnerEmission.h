#ifndef ACIR_LIB_COMPILER_FINALRUNNEREMISSION_H
#define ACIR_LIB_COMPILER_FINALRUNNEREMISSION_H

#include "FinalProgram.h"
#include <string>

namespace acir::compiler {
struct FinalRunnerParts {
  std::string metadataHeader;
  std::string mainSource;
  std::string rtlBridge;
  std::string rtlAdapter;
};

mlir::FailureOr<FinalRunnerParts>
emitFinalRunnerParts(const FinalProgram &program,
                     ac::detail::EmitError emitError);
mlir::FailureOr<FinalRunnerParts>
emitFinalRunnerPartsBody(const FinalProgram &program,
                         ac::detail::EmitError emitError);
} // namespace acir::compiler
#endif
