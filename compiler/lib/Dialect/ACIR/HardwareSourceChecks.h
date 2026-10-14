#ifndef PYCIRCUIT_HARDWARE_SOURCE_CHECKS_H
#define PYCIRCUIT_HARDWARE_SOURCE_CHECKS_H

#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include <functional>

namespace acir::ac::detail {
class SourceCheckBudget;
class SampledAdmissionValidation;
struct ObservationLocalKey {
  mlir::DictionaryAttr identity;
  mlir::DictionaryAttr owner;
  mlir::StringAttr reportName;
};
mlir::LogicalResult verifySourceObservationStructure(SourceObserveOp observe);
mlir::LogicalResult verifyModuleObservationIdentities(ModuleOp definition);
struct ResolvedObservationOwner {
  ModuleOp terminal;
  HardwareBindings bindings;
  llvm::SmallVector<InstanceOp> path;
};
mlir::FailureOr<ResolvedObservationOwner> resolveObservationOwner(
    const HardwareAnalysis &analysis, SourceObserveOp observe,
    const HardwareBindings &physicalBindings,
    const std::function<mlir::LogicalResult(uint64_t, mlir::Operation *)>
        &debit);
mlir::LogicalResult
verifyHardwarePackageEnvelope(const HardwareAnalysis &analysis);
mlir::LogicalResult verifySourceCheckDependencies(
    const HardwareAnalysis &analysis,
    llvm::ArrayRef<std::pair<ModuleOp, HardwareBindings>> scopes,
    const std::function<mlir::LogicalResult(uint64_t, mlir::Operation *)>
        &debit);
mlir::FailureOr<llvm::SmallVector<HardwareCheckBinding>>
definitionSourceChecks(const HardwareAnalysis &analysis, ModuleOp definition,
                       const HardwareBindings &bindings,
                       SampledAdmissionValidation &validation,
                       SourceCheckBudget &budget);
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
