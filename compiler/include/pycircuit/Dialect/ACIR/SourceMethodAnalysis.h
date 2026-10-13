#ifndef PYCIRCUIT_DIALECT_ACIR_SOURCEMETHODANALYSIS_H
#define PYCIRCUIT_DIALECT_ACIR_SOURCEMETHODANALYSIS_H

#include "pycircuit/Dialect/ACIR/ACIROps.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/FunctionExtras.h"
#include "llvm/ADT/SmallVector.h"
namespace acir::ac {

using SourceModuleLookup =
    llvm::function_ref<mlir::Operation *(mlir::FlatSymbolRefAttr)>;

/// A checked, borrowed Method contract from a definition or interface row.
struct SourceMethodView {
  mlir::Operation *module = nullptr;
  SourceMethodOp definition;
  mlir::DictionaryAttr row;
  mlir::FunctionType getFunctionType() const;
  mlir::ArrayAttr getArgumentNames() const;
  mlir::ArrayAttr getParameters() const;
  mlir::ArrayAttr getResultConstraints() const;
  mlir::StringAttr getReturnForm() const;
  mlir::DictionaryAttr getDeclaration() const;
  mlir::SymbolRefAttr getSymbol() const;
};

struct ResolvedSourceMethod {
  SourceMethodView method;
  HardwareBindings bindings;
  mlir::DictionaryAttr receiver;
};

mlir::FailureOr<SourceMethodView>
lookupSourceMethod(mlir::Operation *module, llvm::StringRef name,
                   mlir::Operation *diagnosticSite);
mlir::FailureOr<ResolvedSourceMethod>
resolveSourceMethodCall(SourceMethodCallOp call);
/// Resolve an actual call under bindings for its lexical caller module.
/// The lookup is borrowed only for this synchronous resolution. The receiver
/// occurrence path in the result remains relative to that caller.
mlir::FailureOr<ResolvedSourceMethod> resolveSourceMethodCall(
    SourceMethodCallOp call, const HardwareBindings &parentBindings,
    SourceModuleLookup lookup);
mlir::FailureOr<ResolvedSourceMethod> resolveSourceMethodCall(
    mlir::Operation *callerModule, mlir::DictionaryAttr callRow,
    const HardwareBindings &parentBindings, SourceModuleLookup lookup,
    mlir::Operation *diagnosticSite);

struct SourceMethodSummary {
  bool mayBlock = true;
  llvm::SmallVector<mlir::SymbolRefAttr> callees;
};

/// Facts derived from the current typed operations, never imported proof flags.
/// Body and interface views share call closure, recursion and totality proof.
/// Publication derivation remains a separate fallible request.
class SourceMethodSummariesAnalysis {
public:
  explicit SourceMethodSummariesAnalysis(mlir::Operation *package);
  /// The caller supplies native-verified canonical declarations, after checking
  /// primitive catalog authority, snapshot equality and interface dependencies.
  /// The lookup callback is borrowed only during this synchronous construction.
  SourceMethodSummariesAnalysis(
      llvm::ArrayRef<mlir::Operation *> moduleDeclarations,
      SourceModuleLookup lookup);
  bool isValid() const { return valid; }
  const SourceMethodSummary *lookup(SourceMethodOp method) const;
  const SourceMethodSummary *lookup(mlir::SymbolRefAttr method) const;
  bool isTotal(SourceTransactionOp transaction) const;

  /// Fallible direct publication facts; independent of totality admission.
  /// The cache, including failures, belongs to this analysis of the current IR.
  mlir::FailureOr<mlir::DictionaryAttr>
  getPublicationSummary(ModuleOp definition);
  mlir::FailureOr<bool> requiresTransactionPublication(ModuleOp definition);

private:
  llvm::DenseMap<mlir::Attribute, SourceMethodSummary> methods;
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
