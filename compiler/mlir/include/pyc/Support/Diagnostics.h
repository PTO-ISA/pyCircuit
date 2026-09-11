#ifndef PYC_SUPPORT_DIAGNOSTICS_H
#define PYC_SUPPORT_DIAGNOSTICS_H

#include "mlir/IR/Diagnostics.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/StringRef.h"

namespace mlir {
class Operation;
}

namespace pyc {

/// Metadata field understood by structured diagnostic consumers.
inline constexpr llvm::StringLiteral kDiagnosticCodeMetadata =
    "diagnostic.code";

/// Returns every diagnostic code owned by the native PYC frontend and driver.
llvm::ArrayRef<llvm::StringLiteral> registeredDiagnosticCodes();

/// Returns true when `code` is present in the native PYC diagnostic registry.
bool isRegisteredDiagnosticCode(llvm::StringRef code);

/// Emits a registered PYC error. The code is attached as MLIR diagnostic
/// metadata for machine consumers and retained in the rendered message for
/// command-line users.
mlir::InFlightDiagnostic emitError(mlir::Operation *op, llvm::StringRef code);

template <typename OpTy>
mlir::InFlightDiagnostic emitError(OpTy op, llvm::StringRef code) {
  return emitError(op.getOperation(), code);
}

} // namespace pyc

#endif // PYC_SUPPORT_DIAGNOSTICS_H
