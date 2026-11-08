#ifndef PYCIRCUIT_DIALECT_ACIR_GFIRPACKAGEVALIDATION_H
#define PYCIRCUIT_DIALECT_ACIR_GFIRPACKAGEVALIDATION_H

#include "pycircuit/Dialect/ACIR/HardwareAnalysisContext.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/Support/LogicalResult.h"
#include <memory>

namespace mlir {
class Pass;
}

namespace acir::ac {
/// Verify the versioned routed graph without requiring physical state refinement
/// or an implementation for a declared ordinary external combinational module.
mlir::LogicalResult verifyRoutedPackage(mlir::ModuleOp package,
                                        HardwareSourceCheckLimits limits = {});
} // namespace acir::ac

namespace acir {
std::unique_ptr<mlir::Pass> createVerifyRoutedPass();
/// Register the ac-verify-routed pass from the GFIR validation target.
void registerGFIRValidationPasses();
} // namespace acir
#endif
