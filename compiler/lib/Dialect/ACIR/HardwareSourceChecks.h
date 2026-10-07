#ifndef PYCIRCUIT_HARDWARE_SOURCE_CHECKS_H
#define PYCIRCUIT_HARDWARE_SOURCE_CHECKS_H

#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include <functional>

namespace acir::ac::detail {
mlir::LogicalResult
verifyHardwarePackageEnvelope(const HardwareAnalysis &analysis);
mlir::LogicalResult verifySourceCheckDependencies(
    const HardwareAnalysis &analysis,
    llvm::ArrayRef<std::pair<ModuleOp, HardwareBindings>> scopes,
    const std::function<mlir::LogicalResult(uint64_t, mlir::Operation *)>
        &debit);
mlir::FailureOr<llvm::SmallVector<HardwareCheckBinding>>
definitionSourceChecks(const HardwareAnalysis &analysis, ModuleOp definition,
                       const HardwareBindings &bindings);
bool sameHardwareBindings(const HardwareBindings &lhs,
                          const HardwareBindings &rhs);
mlir::LogicalResult
verifySourceExpectOperation(const HardwareAnalysis &analysis,
                            SourceExpectOp expect,
                            const HardwareBindings &bindings);
mlir::FailureOr<HardwareBindings>
resolveHardwareRootBindings(const HardwareAnalysis &analysis);
} // namespace acir::ac::detail
#endif
