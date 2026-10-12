#ifndef PYCIRCUIT_LIB_DIALECT_ACIR_SOURCETRANSACTIONSUMMARY_H
#define PYCIRCUIT_LIB_DIALECT_ACIR_SOURCETRANSACTIONSUMMARY_H

#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "pycircuit/Dialect/ACIR/SourceMethodAnalysis.h"

namespace acir::ac::detail {
/// The existing direct one-bit literal proof, shared with totality analysis.
mlir::FailureOr<mlir::BoolAttr>
getSourceLiteralPredicate(mlir::Value value, const HardwareAnalysis &hardware);
mlir::FailureOr<mlir::DictionaryAttr>
deriveSourceTransactionSummary(ModuleOp definition);
} // namespace acir::ac::detail
#endif
