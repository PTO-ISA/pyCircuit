#ifndef ACIR_LIB_COMPILER_SCALARNUMERICLOWERING_H
#define ACIR_LIB_COMPILER_SCALARNUMERICLOWERING_H

#include "mlir/IR/BuiltinOps.h"
#include "mlir/Pass/Pass.h"

#include <memory>

namespace acir::compiler {

mlir::LogicalResult lowerExactInputAddTransactional(mlir::ModuleOp unit);

std::unique_ptr<mlir::Pass> createLowerExactInputAddPass();
void registerACIRScalarNumericPasses();

} // namespace acir::compiler

#endif // ACIR_LIB_COMPILER_SCALARNUMERICLOWERING_H
