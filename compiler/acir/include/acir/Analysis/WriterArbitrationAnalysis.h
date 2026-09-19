#ifndef ACIR_ANALYSIS_WRITERARBITRATIONANALYSIS_H
#define ACIR_ANALYSIS_WRITERARBITRATIONANALYSIS_H

#include "mlir/IR/BuiltinOps.h"
#include "mlir/Support/LogicalResult.h"

#include <cstdint>
#include <string>
#include <vector>

namespace acir {

class ACDataFlowAnalyzer;

enum class FootprintRelationProofKind {
  Overlap,
  FieldDisjoint,
  IndexDisjoint,
  PredicateExclusive,
};

struct FootprintRelationInput {
  mlir::ArrayAttr fields;
  mlir::Value index;
  mlir::Value predicate;
  bool wholeEntry = false;
  bool masked = false;
};

enum class WriterConflictProofKind {
  FieldDisjoint,
  IndexDisjoint,
  PredicateExclusive,
  DeclarativeAllocationOrder,
  ExplicitPriority,
  DuplicateRankRejected,
  DuplicateIdentityRejected,
  InconsistentRankRejected,
  SameEndpointRejected,
  MissingIdentityRejected,
  MissingPriorityRejected,
};

struct WriterConflictProof {
  mlir::Operation *leftOperation = nullptr;
  mlir::Operation *rightOperation = nullptr;
  std::string leftEndpoint;
  std::string rightEndpoint;
  std::string leftRule;
  std::string rightRule;
  std::string ownerPath;
  std::string ownerStableId;
  WriterConflictProofKind kind;
  std::string result;
  std::string provenance;
};

struct WriterOrderingProof {
  std::string beforeRule;
  std::string afterRule;
  std::string beforeEndpoint;
  std::string afterEndpoint;
  std::string ownerPath;
  std::string ownerStableId;
  int64_t beforeRank = 0;
  int64_t afterRank = 0;
};

struct WriterArbitrationAnalysis {
  std::vector<WriterConflictProof> conflicts;
  std::vector<WriterOrderingProof> ordering;
  bool hasCrossOwnerCycle = false;
};

/// Run the single authoritative writer overlap and arbitration proof used by
/// ACIR verification. When `report` is supplied, every accepted or rejected
/// pair and every priority edge is recorded with proof provenance.
mlir::LogicalResult
analyzeWriterArbitration(mlir::ModuleOp model, ACDataFlowAnalyzer &analysis,
                         WriterArbitrationAnalysis *report = nullptr);

const char *stringifyWriterConflictProofKind(WriterConflictProofKind kind);
const char *
stringifyFootprintRelationProofKind(FootprintRelationProofKind kind);

/// Shared field/index/predicate relation proof. Writer verification and the
/// whole-design graph both use this exact implementation.
FootprintRelationProofKind
proveFootprintRelation(ACDataFlowAnalyzer &analysis,
                       const FootprintRelationInput &left,
                       const FootprintRelationInput &right);

} // namespace acir

#endif // ACIR_ANALYSIS_WRITERARBITRATIONANALYSIS_H
