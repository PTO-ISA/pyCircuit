#ifndef PYCIRCUIT_LIB_DIALECT_ACIR_SOURCETRANSACTIONSUMMARY_H
#define PYCIRCUIT_LIB_DIALECT_ACIR_SOURCETRANSACTIONSUMMARY_H

#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "pycircuit/Dialect/ACIR/SourceMethodAnalysis.h"

namespace acir::ac::detail {
/// One proven low-zero, one-bit-to-one-bit extract edge. Empty Value means
/// unsupported or unproved; failure means an actual static evaluation error.
mlir::FailureOr<mlir::Value>
getSourcePredicateIdentityInput(mlir::Value value,
                                const HardwareAnalysis &hardware);
/// One-bit literal proof through only the bounded identity edges above.
mlir::FailureOr<mlir::BoolAttr>
getSourceLiteralPredicate(mlir::Value value, const HardwareAnalysis &hardware);
mlir::FailureOr<mlir::DictionaryAttr>
deriveSourceTransactionSummary(ModuleOp definition);
} // namespace acir::ac::detail
#endif
