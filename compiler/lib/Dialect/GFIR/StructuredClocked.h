#ifndef PYCIRCUIT_DIALECT_GFIR_STRUCTUREDCLOCKED_H
#define PYCIRCUIT_DIALECT_GFIR_STRUCTUREDCLOCKED_H

#include "Dialect/ACIR/SourceCheckBudget.h"
#include "pycircuit/Dialect/GFIR/StructuredProgram.h"

namespace acir::gfir::detail {

mlir::LogicalResult verifyAttributes(mlir::Operation *op,
                                     llvm::ArrayRef<llvm::StringRef> allowed);
bool isCombinational(mlir::Operation *op);
bool isStructuredLeaf(mlir::Type type);
bool matchesSignature(CallOp call, mlir::FunctionType type);
bool hasGeneratedDomain(RootOp root);
mlir::LogicalResult verifyFifoPrimitive(PrimitiveOp primitive,
                                        const ac::HardwareAnalysis &hardware);
mlir::FailureOr<StructuredMemory>
describeRawMemory(PrimitiveOp primitive, const ac::HardwareAnalysis &hardware);
mlir::FailureOr<StructuredClocked>
analyzeClocked(ModuleOp module, const ac::HardwareAnalysis &hardware,
               ac::detail::SourceCheckBudget &budget,
               llvm::DenseMap<mlir::Operation *, StructuredChildMethod> childMethods,
               llvm::ArrayRef<StructuredChildState> childStates);
mlir::LogicalResult buildClockedChecks(ModuleOp module, StructuredClocked &plan,
                                      ac::detail::SourceCheckBudget &budget);

} // namespace acir::gfir::detail

#endif
