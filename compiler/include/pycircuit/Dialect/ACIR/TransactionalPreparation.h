#ifndef PYCIRCUIT_DIALECT_ACIR_TRANSACTIONALPREPARATION_H
#define PYCIRCUIT_DIALECT_ACIR_TRANSACTIONALPREPARATION_H

#include "mlir/IR/OwningOpRef.h"
#include "pycircuit/Dialect/ACIR/TransactionalAnalysis.h"

namespace acir::ac {

// Expand verified pure calls in a disposable Linked package. All definitions,
// including unused ones, are verified before any template is removed.
mlir::LogicalResult inlinePureHelpers(mlir::ModuleOp package,
                                      TransactionalDebit debit);

// The original Linked package remains the provenance authority. The analysis
// borrows only the immutable expanded clone. Declaration order ensures that
// analysis is destroyed before its SSA backing storage.
class TransactionalPreparation final {
public:
  static mlir::FailureOr<std::unique_ptr<TransactionalPreparation>>
  create(mlir::ModuleOp source, llvm::StringRef top, TransactionalDebit debit);
  mlir::ModuleOp getSource() const { return source_; }
  const TransactionalAnalysis &getAnalysis() const { return *analysis_; }

private:
  explicit TransactionalPreparation(mlir::ModuleOp source) : source_(source) {}
  mlir::ModuleOp source_;
  mlir::OwningOpRef<mlir::ModuleOp> expanded_;
  std::unique_ptr<TransactionalAnalysis> analysis_;
};
} // namespace acir::ac
#endif
