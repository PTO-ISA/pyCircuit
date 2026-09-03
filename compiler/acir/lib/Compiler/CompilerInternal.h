#ifndef ACIR_COMPILER_INTERNAL_H
#define ACIR_COMPILER_INTERNAL_H

#include "acir/Compiler/Driver.h"

#include "llvm/Support/Error.h"

#include <vector>

namespace acir::compiler::detail {

llvm::Expected<std::vector<CompilerStage>>
selectPipeline(const CompilerRequest &request);

} // namespace acir::compiler::detail

#endif // ACIR_COMPILER_INTERNAL_H
