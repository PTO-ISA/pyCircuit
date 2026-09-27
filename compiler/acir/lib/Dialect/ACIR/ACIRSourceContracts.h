#ifndef ACIR_LIB_DIALECT_ACIR_ACIRSOURCECONTRACTS_H
#define ACIR_LIB_DIALECT_ACIR_ACIRSOURCECONTRACTS_H

#include "acir/Dialect/ACIR/ACIRAttributes.h"

#include "mlir/IR/BuiltinAttributes.h"
#include "mlir/IR/Diagnostics.h"
#include "llvm/ADT/FunctionExtras.h"

namespace acir::ac::detail {

mlir::FailureOr<MathIntAttr>
parseMathIntAttr(mlir::MLIRContext *context, llvm::StringRef spelling,
                 llvm::function_ref<mlir::InFlightDiagnostic()> emitError);

mlir::LogicalResult
verifySourceSpan(mlir::DictionaryAttr value,
                 llvm::function_ref<mlir::InFlightDiagnostic()> emitError);

mlir::LogicalResult
verifyPathComponent(mlir::DictionaryAttr value,
                    llvm::function_ref<mlir::InFlightDiagnostic()> emitError);

mlir::LogicalResult
verifySite(mlir::DictionaryAttr value,
           llvm::function_ref<mlir::InFlightDiagnostic()> emitError);

} // namespace acir::ac::detail

#endif // ACIR_LIB_DIALECT_ACIR_ACIRSOURCECONTRACTS_H
