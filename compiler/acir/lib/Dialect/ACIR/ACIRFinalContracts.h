#ifndef ACIR_LIB_DIALECT_ACIR_ACIRFINALCONTRACTS_H
#define ACIR_LIB_DIALECT_ACIR_ACIRFINALCONTRACTS_H

#include "acir/Dialect/ACIR/ACIROps.h"

namespace acir::ac {

bool isFinalImplementation(mlir::Operation *operation);
mlir::LogicalResult verifyFinalReg(RegOp op);
mlir::LogicalResult verifyFinalModule(ModuleOp op);
mlir::LogicalResult verifyFinalModuleRegion(ModuleOp op);
mlir::LogicalResult verifyFinalRule(RuleOp op);
mlir::LogicalResult verifyFinalRuleRegion(RuleOp op);
mlir::LogicalResult verifyFinalExpect(SourceExpectOp op);
mlir::LogicalResult verifyFinalObserve(SourceObserveOp op);
mlir::LogicalResult verifyFinalInstance(InstanceOp op);

} // namespace acir::ac

#endif // ACIR_LIB_DIALECT_ACIR_ACIRFINALCONTRACTS_H
