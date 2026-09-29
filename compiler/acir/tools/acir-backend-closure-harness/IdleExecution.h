#ifndef ACIR_BACKEND_IDLE_EXECUTION_H
#define ACIR_BACKEND_IDLE_EXECUTION_H
#include "Compiler/FinalProgram.h"
mlir::FailureOr<std::string>
executeIdleFixture(const acir::compiler::FinalProgram &program,
                   llvm::StringRef backend,
                   acir::ac::detail::EmitError emitError);
#endif
