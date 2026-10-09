#ifndef PYCIRCUIT_HARDWARE_EMIT_CPP_CHECKS_H
#define PYCIRCUIT_HARDWARE_EMIT_CPP_CHECKS_H

#include "FinalCppSourceParts.h"
#include "HardwareEmitCommon.h"
#include <functional>

namespace acir::compiler {
struct CppSourceCheckSite {
  const ac::HardwareCheckBinding *binding;
  size_t ordinal;
};
llvm::SmallVector<CppSourceCheckSite>
cppDefinitionChecks(const HardwareEmitContext &context,
                    ac::ModuleOp definition);
void emitCppCheckStorage(HardwareEmitContext &context, ac::ModuleOp definition,
                         llvm::raw_ostream &out);
void emitCppCheckClear(HardwareEmitContext &context, ac::ModuleOp definition,
                       llvm::raw_ostream &out);
mlir::LogicalResult emitCppCheckCapture(
    HardwareEmitContext &context, ac::ModuleOp definition,
    llvm::StringRef object, llvm::StringRef count,
    const std::function<mlir::FailureOr<std::string>(mlir::Value)> &value,
    llvm::raw_ostream &out);
bool cppHasRootChecks(const HardwareEmitContext &context,
                      ac::ModuleOp definition);
mlir::LogicalResult emitCppCheckInstanceNames(HardwareEmitContext &context,
                                              llvm::raw_ostream &out);
mlir::LogicalResult emitCppRootCheckValidator(HardwareEmitContext &context,
                                              llvm::raw_ostream &out);
/// Test-owned/private entry using the same complete emitter and verified
/// preparation as the public wrapper, without lifting its admission guard.
mlir::FailureOr<FinalCppSourceParts>
emitPreparedCppSourceParts(HardwareEmitContext &context);
} // namespace acir::compiler
#endif
