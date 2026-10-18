#ifndef PYCIRCUIT_LOWER_TRANSACTIONS_H
#define PYCIRCUIT_LOWER_TRANSACTIONS_H

#include "llvm/ADT/DenseSet.h"
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

struct QueueRoute {
  llvm::SmallVector<ac::InstanceOp> path;
  ac::InstanceOp allocation;
  bool push = false;
  mlir::Type payloadType, predicateType;
};
struct QueuePopSummaryRoute {
  llvm::SmallVector<ac::InstanceOp> path;
  ac::InstanceOp allocation;
  mlir::Type predicateType;
};
using QueueRoutingKey = std::pair<RoutingKey, unsigned>;

struct DefinitionRouting {
  ac::ModuleOp module;
  llvm::SmallVector<Route> upward, downward;
  llvm::DenseMap<RoutingKey, unsigned> upwardIndex, downwardIndex;
  llvm::SmallVector<unsigned> transactionIndices, stateIndices, queueIndices;
  llvm::SmallVector<QueueRoute, 0> queueIncoming;
  llvm::DenseMap<QueueRoutingKey, unsigned> queueIncomingIndex;
  llvm::SmallVector<QueuePopSummaryRoute, 0> queuePopSummaries;
  llvm::DenseMap<RoutingKey, unsigned> queuePopSummaryIndex;
  unsigned sourceInputs = 0, sourceOutputs = 0;
  mlir::DictionaryAttr expectedWiring, expectedQueueWiring;
};

struct TransactionCandidatePreparation {
  ac::OwnerVersionBinding binding;
  uint64_t dataResult, enableResult;
};

struct TransactionForwardingPreparation {
  ac::StateOwnerId owner;
  uint64_t predecessor, output;
  uint64_t dataCapture, enableCapture;
};

struct TransactionValueRequirement {
  mlir::Operation *site = nullptr;
  ac::InvocationFrameId frame = 0;
  ac::BoundValueId payload = 0;
  std::optional<ac::BoundValueId> rawPath;
  ac::TransactionPoint point;
  mlir::StringAttr kind;
  uint64_t operand = 0, control = 0;
  mlir::DictionaryAttr occurrence, location;
};
struct TransactionQueueResource {
  ac::QueueAccessId access = 0;
  uint64_t control = 0;
  ac::BoundValueId predicate = 0;
  mlir::StringAttr kind;
  llvm::SmallVector<uint64_t, 0> priorPops;
  std::optional<uint64_t> externalPop;
};
struct TransactionQueueExternalPop {
  ac::QueueOwnerId owner = 0;
  uint64_t capture = 0;
};
struct TransactionQueueProposal {
  ac::QueueAccessId access = 0;
  uint64_t rawResult = 0, pathResult = 0;
  std::optional<uint64_t> dataResult, resource;
  bool unreachable = false;
  llvm::SmallVector<uint64_t, 0> excludedControls;
};
struct TransactionWriteDomain {
  ac::StateWriteId write = 0;
  uint64_t candidate = 0, result = 0;
  mlir::ArrayAttr selectors, prefix, suffix;
  ac::TableType table;
  std::optional<ac::BoundValueId> index;
  mlir::Type maskType;
};

struct TransactionPreparedControl {
  std::optional<ac::AdmissionControlId> source;
  std::optional<uint64_t> requirement;
  std::optional<uint64_t> resource;
};

struct TransactionPreparation {
  const ac::TransactionStatePlan *plan = nullptr;
  llvm::SmallVector<ac::BoundValueId> liveIns;
  llvm::SmallVector<TransactionCandidatePreparation> outputs;
  llvm::SmallVector<TransactionForwardingPreparation> forwarding;
  llvm::SmallVector<ac::StateOwnerId> entryOwners;
  llvm::SmallVector<TransactionValueRequirement, 0> requirements;
  llvm::SmallVector<TransactionPreparedControl, 0> controls;
  llvm::SmallVector<TransactionQueueResource, 0> resources;
  llvm::SmallVector<TransactionQueueExternalPop, 0> externalPops;
  llvm::SmallVector<TransactionQueueProposal, 0> queueProposals;
  llvm::SmallVector<TransactionWriteDomain, 0> writeDomains;
  llvm::SmallVector<uint64_t, 0> sourceControls;
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
  llvm::SmallVector<ac::InstanceOp> queues;
  llvm::SmallVector<uint64_t> transactionSchedule;
  llvm::SmallVector<DefinitionRouting, 0> definitions;
  llvm::DenseMap<mlir::Operation *, unsigned> definitionIndex;
  ac::ModuleImportOp dffe, dff;
  /// Scalar states whose verified local update is unconditional and which
  /// have no hierarchy incoming route. These lower to the plain DFF leaf.
  llvm::DenseSet<mlir::Operation *> dffStates;
  bool valueAvailability = false;
};

struct MaterializedVersion {
  mlir::Value data, rawIntent;
};
struct MaterializedBranchResult {
  mlir::Value value;
  llvm::SmallVector<mlir::Operation *, 0> generated;
};

struct MaterializedSelectedWrite {
  mlir::Value data, mask, rawPath;
  llvm::SmallVector<mlir::Operation *, 0> generated;
};

struct MaterializedAdmissionControl {
  mlir::Value raw, selectedPath, reached, accepted, bypass;
  ac::SourceExpectOp terminal;
};
struct MaterializedResourceDemand {
  mlir::Value demand, mayExecute;
  llvm::SmallVector<mlir::Value, 0> reached;
  llvm::SmallVector<mlir::Operation *, 0> generated;
};
struct MaterializedSelection {
  mlir::Value whenTrue, whenFalse;
};

struct MaterializedCommit {
  MaterializedBranchResult data;
  mlir::Value enable;
  llvm::SmallVector<mlir::Operation *, 0> enableGenerated;
  bool domainMerge = false;
};

struct MaterializedTransaction {
  ac::RuleOp rule;
  llvm::SmallVector<mlir::Value, 0> prefixes;
  llvm::SmallVector<MaterializedAdmissionControl, 0> controls;
  llvm::SmallVector<MaterializedSelection, 0> selections;
  llvm::SmallVector<mlir::Value, 0> boundValues;
  llvm::SmallVector<MaterializedVersion, 0> versions;
  llvm::SmallVector<MaterializedBranchResult, 0> branchResults, stateJoins, forwarding,
      conditionalWrites;
  llvm::SmallVector<MaterializedCommit, 0> commits;
  llvm::SmallVector<mlir::IRMapping, 0> frames;
  llvm::SmallVector<ac::SourceExpectOp, 0> bodyChecks;
  llvm::SmallVector<ac::SourceObserveOp, 0> observations;
  llvm::SmallVector<ac::ValueRequireOp, 0> requirements;
  llvm::SmallVector<ac::BitsSelectOp, 0> resources;
  llvm::SmallVector<mlir::Value, 0> queuePaths;
  llvm::SmallVector<MaterializedSelectedWrite, 0> selectedWrites;
  MaterializedResourceDemand resourceDemand;
  llvm::SmallVector<std::pair<uint64_t, uint64_t>, 0> validityCaptures;
};

/// The mappings locate actual surviving candidate objects; they do not grant
/// semantic authority. The production verifier compares them to original IR.
struct MaterializedQueueMerge {
  ac::ModuleOp module;
  QueueRoutingKey key;
  llvm::SmallVector<MaterializedBranchResult, 0> data;
  llvm::SmallVector<mlir::Value, 0> enables;
};

struct MaterializedIncomingCommit {
  ac::ModuleOp module;
  RoutingKey key;
  MaterializedBranchResult data;
};

struct MaterializedTransactionLowering {
  mlir::OwningOpRef<mlir::ModuleOp> module;
  mlir::IRMapping packageMap;
  llvm::SmallVector<MaterializedTransaction, 0> transactions;
  llvm::SmallVector<MaterializedIncomingCommit, 0> incomingCommits;
  llvm::SmallVector<mlir::Operation *, 0> availabilityOperations;
  llvm::SmallVector<MaterializedQueueMerge, 0> queueMerges;
  llvm::SmallVector<mlir::Value, 0> queueDisabled;
};

mlir::FailureOr<PreparedTransactionLowering>
prepareTransactionLowering(mlir::ModuleOp original,
                           const ac::SourceTransactionStateAnalysis &analysis,
                           ac::detail::SourceCheckBudget &budget);
mlir::FailureOr<MaterializedTransactionLowering>
materializeTransactionLowering(PreparedTransactionLowering &prepared,
                               ac::detail::SourceCheckBudget &budget);
mlir::LogicalResult verifyTransactionMaterialization(
    const PreparedTransactionLowering &prepared,
    const MaterializedTransactionLowering &candidate,
    ac::detail::SourceCheckBudget &budget);
} // namespace acir::detail
#endif
