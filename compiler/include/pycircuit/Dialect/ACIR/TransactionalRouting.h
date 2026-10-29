#ifndef PYCIRCUIT_DIALECT_ACIR_TRANSACTIONALROUTING_H
#define PYCIRCUIT_DIALECT_ACIR_TRANSACTIONALROUTING_H

#include "mlir/IR/OwningOpRef.h"
#include "pycircuit/Dialect/ACIR/TransactionalAnalysis.h"

namespace acir::ac {
mlir::FailureOr<mlir::OwningOpRef<mlir::ModuleOp>>
routeTransactionalPackage(mlir::ModuleOp source, llvm::StringRef top,
                          TransactionalDebit debit);
// Replays source semantics against actual Routed or final graph endpoints.
// Serialized source is authority; generated IDs only locate candidate objects.
mlir::LogicalResult verifyTransactionalRouting(mlir::ModuleOp package,
                                               TransactionalDebit debit);
} // namespace acir::ac
#endif
