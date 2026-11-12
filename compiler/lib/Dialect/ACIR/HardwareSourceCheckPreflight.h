#ifndef PYCIRCUIT_HARDWARE_SOURCE_CHECK_PREFLIGHT_H
#define PYCIRCUIT_HARDWARE_SOURCE_CHECK_PREFLIGHT_H

#include "HardwareSourceChecks.h"
#include "PackageVerification.h"
#include "SourceCheckBudget.h"

namespace acir::ac::detail {
/// Share the plan builder's immutable native-verification invocation with
/// ModuleOp's parameterless ODS verifier. The scope is synchronous and bound to
/// one exact package; nested scopes restore the prior context on every exit.
/// Standalone Module verification has no scope and keeps its independent proof.
class NativeSourceCheckScope {
public:
  using Definitions = llvm::DenseMap<mlir::Operation *,
                                    llvm::SmallVector<HardwareCheckBinding>>;
  NativeSourceCheckScope(const HardwareAnalysis &analysis,
                        SampledAdmissionValidation &validation,
                        SourceCheckBudget &budget, Definitions &definitions);
  ~NativeSourceCheckScope();
  NativeSourceCheckScope(const NativeSourceCheckScope &) = delete;
  NativeSourceCheckScope &operator=(const NativeSourceCheckScope &) = delete;
  static std::optional<mlir::LogicalResult> verify(ModuleOp definition);
private:
  static thread_local NativeSourceCheckScope *active;
  NativeSourceCheckScope *previous;
  const HardwareAnalysis &analysis;
  SampledAdmissionValidation &validation;
  SourceCheckBudget &budget;
  Definitions &definitions;
};
mlir::FailureOr<HardwareSourceCheckPlan>
buildSourceCheckPlan(const HardwareAnalysis &analysis,
                     HardwareSourceCheckLimits limits,
                     const PackageStagePolicy &policy =
                         PackageStagePolicy::hardware());
} // namespace acir::ac::detail
#endif
