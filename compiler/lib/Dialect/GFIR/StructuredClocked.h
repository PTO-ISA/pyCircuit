#ifndef PYCIRCUIT_DIALECT_GFIR_STRUCTUREDCLOCKED_H
#define PYCIRCUIT_DIALECT_GFIR_STRUCTUREDCLOCKED_H

#include "Dialect/ACIR/SourceCheckBudget.h"
#include "pycircuit/Dialect/GFIR/StructuredProgram.h"
#include "llvm/ADT/STLFunctionalExtras.h"

namespace acir::gfir::detail {

mlir::LogicalResult verifyAttributes(mlir::Operation *op,
                                     llvm::ArrayRef<llvm::StringRef> allowed);
bool isCombinational(mlir::Operation *op);
bool isStructuredLeaf(mlir::Type type);
mlir::FailureOr<StructuredWritePath>
describeStateFields(const ac::HardwareAnalysis &hardware,
                    ac::detail::SourceCheckBudget &budget, mlir::Type type,
                    mlir::ArrayAttr fields, mlir::Operation *site);
bool matchesSignature(CallOp call, mlir::FunctionType type);
bool hasGeneratedDomain(RootOp root);
mlir::LogicalResult verifyFifoPrimitive(PrimitiveOp primitive,
                                        const ac::HardwareAnalysis &hardware);
mlir::FailureOr<StructuredMemory>
describeRawMemory(PrimitiveOp primitive, const ac::HardwareAnalysis &hardware);
using ClockedCallBinder = llvm::function_ref<
    mlir::FailureOr<llvm::ArrayRef<StructuredChildState>>(StructuredClocked &)>;
mlir::FailureOr<StructuredClocked>
analyzeClocked(ModuleOp module, const ac::HardwareAnalysis &hardware,
               ac::detail::SourceCheckBudget &budget,
               ClockedCallBinder bindCalls);
mlir::LogicalResult buildClockedChecks(ModuleOp module, StructuredClocked &plan,
                                      ac::detail::SourceCheckBudget &budget);

} // namespace acir::gfir::detail

#endif
