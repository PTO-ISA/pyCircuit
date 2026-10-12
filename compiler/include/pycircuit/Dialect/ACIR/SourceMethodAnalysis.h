#ifndef PYCIRCUIT_DIALECT_ACIR_SOURCEMETHODANALYSIS_H
#define PYCIRCUIT_DIALECT_ACIR_SOURCEMETHODANALYSIS_H

#include "pycircuit/Dialect/ACIR/ACIROps.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallVector.h"
namespace acir::ac {

struct SourceMethodSummary {
  bool mayBlock = true;
  llvm::SmallVector<SourceMethodOp> callees;
};

/// Facts derived from the current typed operations, never imported proof flags.
/// This first summary slice covers call closure, recursion and result totality.
class SourceMethodSummariesAnalysis {
public:
  explicit SourceMethodSummariesAnalysis(mlir::Operation *package);
  bool isValid() const { return valid; }
  const SourceMethodSummary *lookup(SourceMethodOp method) const;
  bool isTotal(SourceTransactionOp transaction) const;

private:
  llvm::DenseMap<mlir::Operation *, SourceMethodSummary> methods;
  llvm::DenseMap<mlir::Operation *, bool> transactions;
  bool valid = false;
};

} // namespace acir::ac

#endif
