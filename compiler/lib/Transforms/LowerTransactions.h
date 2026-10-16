#ifndef PYCIRCUIT_LOWER_TRANSACTIONS_H
#define PYCIRCUIT_LOWER_TRANSACTIONS_H

#include "mlir/IR/IRMapping.h"
#include "mlir/IR/OwningOpRef.h"
#include "pycircuit/Dialect/ACIR/SourceTransactionStateAnalysis.h"

namespace acir::ac::detail {
class SourceCheckBudget;
}

namespace acir::detail {
using RoutingKey = std::pair<mlir::ArrayAttr, mlir::Value>;

struct Route {
  llvm::SmallVector<ac::InstanceOp> path;
  mlir::Value value;
  mlir::Type type, enableType;
};

struct DefinitionRouting {
  ac::ModuleOp module;
  llvm::SmallVector<Route> upward, downward;
  llvm::DenseMap<RoutingKey, unsigned> upwardIndex, downwardIndex;
  llvm::SmallVector<unsigned> transactionIndices, stateIndices;
  unsigned sourceInputs = 0, sourceOutputs = 0;
  mlir::DictionaryAttr expectedWiring;
};

struct TransactionCandidatePreparation {
  ac::OwnerVersionBinding binding;
  uint64_t dataResult, enableResult;
};

struct TransactionPreparation {
  const ac::TransactionStatePlan *plan = nullptr;
  llvm::SmallVector<ac::BoundValueId> liveIns;
  llvm::SmallVector<TransactionCandidatePreparation> outputs;
  uint64_t clockCapture = 0, resetCapture = 0, fireResult = 0;
  uint64_t checkBase = 0, totalResults = 0;
  llvm::SmallVector<uint64_t, 0> observationPathResults;
  mlir::DictionaryAttr expectedAdmission;
  mlir::ArrayAttr expectedRequiredChecks;
};

/// Immutable preparation data borrowing the original package and analysis.
/// Both must outlive materialization and verification of the detached
/// candidate.
struct PreparedTransactionLowering {
  mlir::ModuleOp original;
  llvm::SmallVector<TransactionPreparation, 0> transactions;
  llvm::SmallVector<ac::SourceStateOp> states;
  llvm::SmallVector<DefinitionRouting, 0> definitions;
  llvm::DenseMap<mlir::Operation *, unsigned> definitionIndex;
  ac::ModuleImportOp dffe;
};

struct MaterializedVersion {
  mlir::Value data, rawIntent;
};
struct MaterializedBranchResult {
  ac::BitsSelectOp select;
};

struct MaterializedAdmissionControl {
  mlir::Value raw, selectedPath, reached, accepted, bypass;
  ac::SourceExpectOp terminal;
};
struct MaterializedSelection {
  mlir::Value whenTrue, whenFalse;
};

struct MaterializedTransaction {
  ac::RuleOp rule;
  llvm::SmallVector<mlir::Value, 0> prefixes;
  llvm::SmallVector<MaterializedAdmissionControl, 0> controls;
  llvm::SmallVector<MaterializedSelection, 0> selections;
  llvm::SmallVector<mlir::Value, 0> boundValues;
  llvm::SmallVector<MaterializedVersion, 0> versions;
  llvm::SmallVector<MaterializedBranchResult, 0> branchResults;
  llvm::SmallVector<mlir::IRMapping, 0> frames;
  llvm::SmallVector<ac::SourceExpectOp, 0> bodyChecks;
  llvm::SmallVector<ac::SourceObserveOp, 0> observations;
};

/// The mappings locate actual surviving candidate objects; they do not grant
/// semantic authority. The production verifier compares them to original IR.
struct MaterializedTransactionLowering {
  mlir::OwningOpRef<mlir::ModuleOp> module;
  mlir::IRMapping packageMap;
  llvm::SmallVector<MaterializedTransaction, 0> transactions;
};

mlir::FailureOr<PreparedTransactionLowering>
prepareTransactionLowering(mlir::ModuleOp original,
                           const ac::SourceTransactionStateAnalysis &analysis,
                           ac::detail::SourceCheckBudget &budget);
mlir::FailureOr<MaterializedTransactionLowering>
materializeTransactionLowering(const PreparedTransactionLowering &prepared,
                               ac::detail::SourceCheckBudget &budget);
mlir::LogicalResult verifyTransactionMaterialization(
    const PreparedTransactionLowering &prepared,
    const MaterializedTransactionLowering &candidate,
    ac::detail::SourceCheckBudget &budget);
} // namespace acir::detail
#endif
