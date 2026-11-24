#ifndef PYCIRCUIT_DIALECT_GFIR_STRUCTUREDCLOCKED_H
#define PYCIRCUIT_DIALECT_GFIR_STRUCTUREDCLOCKED_H

#include "Dialect/ACIR/SourceCheckBudget.h"
#include "pycircuit/Dialect/GFIR/StructuredProgram.h"

namespace acir::gfir::detail {

mlir::LogicalResult verifyAttributes(mlir::Operation *op,
                                     llvm::ArrayRef<llvm::StringRef> allowed);
bool isCombinational(mlir::Operation *op);
bool hasGeneratedDomain(RootOp root);
mlir::LogicalResult verifyFifoPrimitive(PrimitiveOp primitive,
                                        const ac::HardwareAnalysis &hardware);
mlir::FailureOr<StructuredClocked>
analyzeClocked(ModuleOp module, const ac::HardwareAnalysis &hardware,
               ac::detail::SourceCheckBudget &budget);

} // namespace acir::gfir::detail

#endif
