#ifndef PYCIRCUIT_HARDWARE_SOURCE_CHECKS_H
#define PYCIRCUIT_HARDWARE_SOURCE_CHECKS_H

#include "ACIRSourceContracts.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include <functional>

namespace acir::ac::detail {
enum class PackageVerificationProfile { Routed, Hardware };
class SourceCheckBudget;
class SampledAdmissionValidation;
mlir::LogicalResult verifyVerificationObserverUses(
    const HardwareAnalysis &analysis, ModuleOp definition,
    const HardwareBindings &bindings,
    const std::function<mlir::LogicalResult(uint64_t, mlir::Operation *)> &debit);
mlir::LogicalResult verifySourceObservationSpec(
    mlir::StringAttr kind, mlir::DictionaryAttr spec, uint64_t valueCount,
    EmitError emitError);
struct ObservationLocalKey {
  mlir::DictionaryAttr identity;
  mlir::DictionaryAttr owner;
  mlir::StringAttr reportName;
};
mlir::LogicalResult verifySourceObservationStructure(SourceObserveOp observe);
mlir::LogicalResult verifyModuleObservationIdentities(ModuleOp definition);
mlir::LogicalResult verifyRoutedRegistration(
    const HardwareAnalysis &analysis, RuleOp rule,
    const std::function<mlir::LogicalResult(uint64_t, mlir::Operation *)> &debit);
mlir::LogicalResult verifyTransactionReturns(
    const HardwareAnalysis &analysis,
    const std::function<mlir::LogicalResult(uint64_t, mlir::Operation *)> &debit);
struct ResolvedSourceEffectOwner {
  ModuleOp terminal;
  HardwareBindings bindings;
  llvm::SmallVector<InstanceOp> path;
};
mlir::FailureOr<ResolvedSourceEffectOwner> resolveSourceEffectOwner(
    const HardwareAnalysis &analysis, ModuleOp physical,
    const HardwareBindings &physicalBindings,
    mlir::DictionaryAttr registration, mlir::DictionaryAttr site,
    mlir::ArrayAttr invocation, mlir::DictionaryAttr location,
    mlir::Operation *diagnosticSite,
    const std::function<mlir::LogicalResult(uint64_t, mlir::Operation *)> &debit);
mlir::FailureOr<ResolvedSourceEffectOwner> resolveObservationOwner(
    const HardwareAnalysis &analysis, SourceObserveOp observe,
    const HardwareBindings &physicalBindings,
    const std::function<mlir::LogicalResult(uint64_t, mlir::Operation *)>
        &debit);
mlir::LogicalResult
verifyHardwarePackageStage(const HardwareAnalysis &analysis,
                           PackageVerificationProfile profile);
mlir::LogicalResult
verifyHardwarePackageEnvelope(
    const HardwareAnalysis &analysis, PackageVerificationProfile profile,
    const std::function<mlir::LogicalResult(uint64_t, mlir::Operation *)> &debit);
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
mlir::FailureOr<HardwareBindings> resolveHardwareEntryBindings(
    const HardwareAnalysis &analysis, mlir::DictionaryAttr entry);
mlir::FailureOr<HardwareBindings>
resolveHardwareLogicalRootBindings(const HardwareAnalysis &analysis);
} // namespace acir::ac::detail
#endif
