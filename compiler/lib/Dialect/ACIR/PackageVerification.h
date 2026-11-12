#ifndef PYCIRCUIT_PACKAGE_VERIFICATION_H
#define PYCIRCUIT_PACKAGE_VERIFICATION_H

#include "mlir/Support/LogicalResult.h"
#include "llvm/ADT/StringRef.h"
#include <cstdint>
#include <functional>

namespace mlir {
class Operation;
}
namespace acir::ac {
class HardwareAnalysis;
}

namespace acir::ac::detail {
/// Stage policy for the shared bounded package-check pipeline. The policy
/// changes admission, not the source-check builder's ordering or lifetime.
struct PackageStagePolicy {
  /// Routed verification requires ac.stage to exist and equal stageValue;
  /// final hardware admits an absent stage and rejects any other value.
  bool stageRequired;
  llvm::StringRef stageValue;
  /// The closed routed walk admits operations whose REGISTERED NAME is
  /// "gfir.state"/"gfir.state.update" (string compare, not an interface
  /// check). When false, final hardware instead rejects every routed
  /// logical-state interface unconditionally.
  bool admitRoutedLogicalStateByName;
  /// Routed verification rejects unlowered source resource bindings on
  /// instances; final hardware runs no check at that site.
  bool rejectUnloweredInstanceBindings;
  /// Routed ordinary module imports require ac.external_contract.
  bool requireExternalContract;
  /// Final hardware requires an implementation for ordinary imports.
  bool requireImplementedImports;
  /// ac.anchor_provenance belongs only to the top-level final package.
  bool anchorProvenanceTopLevelOnly;
  /// Final hardware rejects unlowered transactions (envelope gate).
  bool rejectUnloweredTransactions;
  /// Routed source-check planning admits a declared ordinary external
  /// combinational module without an executable provider body.
  bool combinationalExternalAdmission;
  static const PackageStagePolicy &routed();
  static const PackageStagePolicy &hardware();
};

mlir::LogicalResult
verifyHardwarePackageStage(const HardwareAnalysis &analysis,
                           const PackageStagePolicy &policy);
mlir::LogicalResult
verifyHardwarePackageEnvelope(
    const HardwareAnalysis &analysis, const PackageStagePolicy &policy,
    const std::function<mlir::LogicalResult(uint64_t, mlir::Operation *)> &debit);
} // namespace acir::ac::detail
#endif
