#ifndef ACIR_COMPOSED_OBSERVATIONS_H
#define ACIR_COMPOSED_OBSERVATIONS_H
#include "Compiler/FinalProgram.h"
#include "llvm/Support/JSON.h"
mlir::FailureOr<llvm::json::Object>
composedObservation(const acir::compiler::FinalProgram &program,
                    llvm::StringRef tag, uint64_t ordinal, uint64_t epoch,
                    uint64_t valueKind, llvm::StringRef bits, uint64_t owner,
                    uint64_t registration, uint64_t site,
                    acir::ac::detail::EmitError error);
mlir::LogicalResult
verifyComposedCompletion(const acir::compiler::FinalProgram &program,
                         const llvm::json::Object &run,
                         acir::ac::detail::EmitError error);
mlir::FailureOr<llvm::json::Array>
composedRtlStatistics(const acir::compiler::FinalProgram &program,
                      uint64_t epoch, const llvm::json::Array &gauges,
                      acir::ac::detail::EmitError error);
#endif
