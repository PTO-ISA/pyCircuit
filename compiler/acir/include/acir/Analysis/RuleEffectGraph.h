#ifndef ACIR_ANALYSIS_RULEEFFECTGRAPH_H
#define ACIR_ANALYSIS_RULEEFFECTGRAPH_H

#include "mlir/IR/BuiltinOps.h"
#include "mlir/Support/LogicalResult.h"
#include "llvm/Support/raw_ostream.h"

#include <string>
#include <vector>

namespace acir {

struct RuleEffectGraphNode {
  std::string id;
  std::string kind;
  std::string label;
  std::string status;
};

struct RuleEffectGraphEdge {
  std::string from;
  std::string to;
  std::string kind;
  std::string result;
  std::string proof;
  std::string detail;
};

struct RuleEffectGraph {
  std::vector<RuleEffectGraphNode> nodes;
  std::vector<RuleEffectGraphEdge> edges;
  bool arbitrationCycleRejected = false;
};

/// Assign compact source-order-independent ranks to exact expression DAG
/// nodes by interning closed structural tuples bottom-up. Ranks are local debug
/// references, never identity outside one graph construction.
mlir::FailureOr<std::vector<int64_t>>
canonicalRuleExpressionRanks(mlir::ArrayAttr expressionDAG);

/// Build Decision 0271's deterministic, read-only whole-design graph from
/// independently verified exact summaries and the shared writer proof engine.
mlir::LogicalResult buildRuleEffectGraph(mlir::ModuleOp model,
                                         RuleEffectGraph &graph);

void printRuleEffectGraphJSON(const RuleEffectGraph &graph,
                              llvm::raw_ostream &output);
void printRuleEffectGraphDOT(const RuleEffectGraph &graph,
                             llvm::raw_ostream &output);

} // namespace acir

#endif // ACIR_ANALYSIS_RULEEFFECTGRAPH_H
