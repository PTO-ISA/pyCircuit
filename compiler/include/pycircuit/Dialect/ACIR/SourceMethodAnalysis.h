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
/// Totality covers call closure and recursion; native publication is a separate
/// fallible request and does not enable imported Method admission.
class SourceMethodSummariesAnalysis {
public:
  explicit SourceMethodSummariesAnalysis(mlir::Operation *package);
  bool isValid() const { return valid; }
  const SourceMethodSummary *lookup(SourceMethodOp method) const;
  bool isTotal(SourceTransactionOp transaction) const;

  /// Fallible direct publication facts; independent of totality admission.
  /// The cache, including failures, belongs to this analysis of the current IR.
  mlir::FailureOr<mlir::DictionaryAttr>
  getPublicationSummary(ModuleOp definition);

private:
  llvm::DenseMap<mlir::Operation *, SourceMethodSummary> methods;
  llvm::DenseMap<mlir::Operation *, bool> transactions;
  llvm::DenseMap<mlir::Operation *, mlir::DictionaryAttr> publicationSummaries;
  mlir::Operation *package = nullptr;
  bool valid = false;
};

/// Check closed schema and local scope without consulting provider bodies.
mlir::LogicalResult
verifySourceTransactionSummary(mlir::Operation *moduleDeclaration,
                               mlir::DictionaryAttr summary);

/// Require the exact canonical summary rederived from the current provider.
mlir::LogicalResult
verifyPublishedTransactionSummary(ModuleOp definition,
                                  mlir::DictionaryAttr published,
                                  SourceMethodSummariesAnalysis &analysis);

} // namespace acir::ac

#endif
