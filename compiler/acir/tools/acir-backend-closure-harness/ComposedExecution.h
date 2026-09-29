#ifndef ACIR_COMPOSED_EXECUTION_H
#define ACIR_COMPOSED_EXECUTION_H
#include "Compiler/FinalProgram.h"
#include "llvm/Support/JSON.h"
mlir::FailureOr<llvm::json::Object>
executeComposedFixture(const acir::compiler::FinalProgram &program,
                       llvm::StringRef backend, uint64_t maxTicks,
                       bool resetRerun, acir::ac::detail::EmitError error);
mlir::FailureOr<std::string>
executeComposedPair(const acir::compiler::FinalProgram &program,
                    llvm::StringRef identity, uint64_t maxTicks,
                    acir::ac::detail::EmitError error);
#endif
