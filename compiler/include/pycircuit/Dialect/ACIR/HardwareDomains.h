#ifndef PYCIRCUIT_DIALECT_ACIR_HARDWAREDOMAINS_H
#define PYCIRCUIT_DIALECT_ACIR_HARDWAREDOMAINS_H
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "pycircuit/Dialect/ACIR/HardwareClockSources.h"
namespace acir::ac {
struct HardwareDomainOccurrence {
  ModuleOp definition;
  mlir::Operation *allocation = nullptr;
  std::optional<unsigned> parent;
  llvm::SmallVector<mlir::DictionaryAttr> instancePath;
  llvm::StringMap<unsigned> clocks;
  llvm::DenseMap<unsigned, unsigned> inputClocks;
};
struct HardwareDomainPlan {
  ModuleOp root;
  llvm::SmallVector<unsigned> clockInputs;
  llvm::SmallVector<HardwareDomainOccurrence, 0> occurrences;
  /// With derived sources clocks/inputClocks contain source IDs, not ordinals.
  std::optional<HardwareClockSourcePlan> clockSources;
};
struct HardwarePortDomains {
  llvm::SmallVector<mlir::StringAttr> inputs, outputs;
};
/// Effective domain names indexed by physical signature ordinal. Public
/// availability companions inherit the domain of the associated payload.
mlir::FailureOr<HardwarePortDomains> getHardwarePortDomains(ModuleOp module);
/// A table read can forward a control only when it selects an in-range lane
/// for every runtime input, including X/Z on other signals.
mlir::FailureOr<bool>
isKnownInRangeTableSelection(mlir::Value table, mlir::Value index,
                             const HardwareAnalysis &analysis,
                             const HardwareBindings &bindings,
                             mlir::Operation *site);
mlir::FailureOr<HardwareDomainPlan>
getHardwareDomainPlan(mlir::ModuleOp package,
                      HardwareSourceCheckLimits limits = {});
mlir::LogicalResult
verifyHardwareDomains(mlir::ModuleOp package,
                      HardwareSourceCheckLimits limits = {});
mlir::ArrayAttr serializeHardwareDomainBindings(const HardwareDomainPlan &plan);
} // namespace acir::ac
#endif
