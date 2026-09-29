#ifndef ACIR_LIB_COMPILER_PROPOSALGRAPH_H
#define ACIR_LIB_COMPILER_PROPOSALGRAPH_H

#include "ModuleGraph.h"

namespace acir::compiler {

struct CheckGraph;

struct ProposalUseFact {
  mlir::DictionaryAttr useID;
  mlir::DictionaryAttr sourceID;
  mlir::Value value;
  mlir::Value valid;
  mlir::Value path;
  ac::SourceUseOp use;
  ac::ValueUseOp valueUse;
};

enum class ProposalConflictKind {
  MutuallyExclusive,
  GuaranteedOverlap,
  PossibleOverlap,
};

enum class CommitPairKind {
  Hold,
  Forward,
  ExclusiveMerge,
};

struct ProposalContribution {
  mlir::DictionaryAttr stateID;
  mlir::DictionaryAttr logicalType;
  mlir::Value data;
  mlir::Value enabled;
  mlir::Value value;
  mlir::Value valid;
  mlir::Value path;
  mlir::DictionaryAttr useID;
  mlir::DictionaryAttr sourceID;
  InstanceView *owner = nullptr;
  ac::RuleOp rule;
  ac::SourceUseOp use;
  ac::ValueUseOp valueUse;
  ac::InstanceOp child;
  // Compositional rules have one physical proposal per output. These rows
  // retain the original source/value-use facts that were reduced to its yield.
  llvm::SmallVector<ProposalUseFact> sourceFacts;
  bool composition = false;
};

struct ProposalConflict {
  size_t left = 0;
  size_t right = 0;
  ProposalConflictKind kind = ProposalConflictKind::PossibleOverlap;
};

struct CommitPair {
  mlir::DictionaryAttr stateID;
  CommitPairKind kind = CommitPairKind::Hold;
  mlir::Value data;
  mlir::Value enable;
  llvm::SmallVector<size_t> contributions;
  bool permitAlways = true;
};

struct StateProposals {
  mlir::DictionaryAttr stateID;
  mlir::DictionaryAttr logicalType;
  llvm::SmallVector<size_t> contributions;
  llvm::SmallVector<ProposalConflict> conflicts;
  CommitPair commit;
};

struct ProposalGraph {
  const ModuleGraph *modules = nullptr;
  const CheckGraph *checks = nullptr;
  llvm::SmallVector<ProposalContribution> contributions;
  llvm::SmallVector<StateProposals> states;
  bool globalPermitAlways = true;
};

mlir::FailureOr<ProposalGraph>
buildSourceProposalGraph(const ModuleGraph &modules,
                         ac::detail::EmitError emitError);
mlir::FailureOr<ProposalGraph>
buildSourceProposalGraph(const ModuleGraph &modules, const CheckGraph *checks,
                         ac::detail::EmitError emitError);
mlir::LogicalResult verifySourceProposals(const ProposalGraph &graph,
                                          ac::detail::EmitError emitError);

} // namespace acir::compiler

#endif // ACIR_LIB_COMPILER_PROPOSALGRAPH_H
