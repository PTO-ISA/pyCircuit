#ifndef PYCIRCUIT_DIALECT_ACIR_HARDWAREDOMAINS_H
#define PYCIRCUIT_DIALECT_ACIR_HARDWAREDOMAINS_H
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
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
};
mlir::FailureOr<HardwareDomainPlan>
getHardwareDomainPlan(mlir::ModuleOp package,
                      HardwareSourceCheckLimits limits = {});
mlir::LogicalResult
verifyHardwareDomains(mlir::ModuleOp package,
                      HardwareSourceCheckLimits limits = {});
mlir::FailureOr<mlir::ArrayAttr>
serializeHardwareDomainBindings(mlir::ModuleOp package,
                                HardwareSourceCheckLimits limits = {});
} // namespace acir::ac
#endif
