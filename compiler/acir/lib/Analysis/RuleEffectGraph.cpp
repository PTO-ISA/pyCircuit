#include "acir/Analysis/RuleEffectGraph.h"

#include "acir/Analysis/VariableAnalysis.h"
#include "acir/Analysis/WriterArbitrationAnalysis.h"
#include "acir/Dialect/ACIR/ACIROps.h"

#include "llvm/ADT/SmallSet.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/Support/JSON.h"

#include <algorithm>
#include <map>
#include <optional>
#include <set>
#include <tuple>

using namespace mlir;

namespace acir {
namespace {

std::string attributeText(Attribute attribute) {
  std::string text;
  llvm::raw_string_ostream stream(text);
  attribute.print(stream);
  return text;
}

std::string stableRuleId(Operation *operation) {
  if (auto rule = dyn_cast<ac::RuleOp>(operation))
    return rule.getStableId().str();
  return cast<ac::FiringOp>(operation).getStableId().str();
}

ArrayAttr ruleArray(Operation *operation, StringRef transientName,
                    StringRef firingName) {
  return operation->getAttrOfType<ArrayAttr>(
      isa<ac::RuleOp>(operation) ? transientName : firingName);
}

void addNode(RuleEffectGraph &graph, StringRef id, StringRef kind,
             StringRef label, StringRef status = "present") {
  graph.nodes.push_back({id.str(), kind.str(), label.str(), status.str()});
}

void addEdge(RuleEffectGraph &graph, StringRef from, StringRef to,
             StringRef kind, StringRef result = {}, StringRef proof = {},
             StringRef detail = {}) {
  graph.edges.push_back({from.str(), to.str(), kind.str(), result.str(),
                         proof.str(), detail.str()});
}

std::string fieldText(ArrayAttr fields, bool wholeEntry) {
  if (wholeEntry)
    return "$entry";
  SmallVector<std::string> names;
  for (Attribute raw : fields)
    names.push_back(cast<StringAttr>(raw).getValue().str());
  llvm::sort(names);
  return llvm::join(names, ",");
}

std::string dotEscape(StringRef text) {
  std::string result;
  static constexpr char hex[] = "0123456789abcdef";
  for (unsigned char value : text) {
    if (value == '\\' || value == '"') {
      result.push_back('\\');
      result.push_back(value);
    } else if (value == '\n') {
      result.append("\\n");
    } else if (value == '\r') {
      result.append("\\r");
    } else if (value == '\t') {
      result.append("\\t");
    } else if (value < 0x20 || value == 0x7f) {
      result.append("\\x");
      result.push_back(hex[value >> 4]);
      result.push_back(hex[value & 0xf]);
    } else {
      result.push_back(static_cast<char>(value));
    }
  }
  return result;
}

struct GraphFootprint {
  std::string ruleId;
  std::string ruleNode;
  std::string nodeId;
  std::string ownerPath;
  std::string ownerStableId;
  std::string ownerIdentity;
  std::string access;
  std::string fieldsText;
  std::string indexExpression;
  std::string predicateExpression;
  ArrayAttr fields;
  bool wholeEntry = false;
  ac::ExactRuleFootprintInput live;
};

} // namespace

FailureOr<std::vector<int64_t>> canonicalRuleExpressionRanks(ArrayAttr dag) {
  std::vector<int64_t> ranks(dag.size(), -1);
  std::vector<unsigned> depths(dag.size(), 0);
  std::vector<std::string> tuples(dag.size());
  unsigned maxDepth = 0;
  for (auto [ordinal, raw] : llvm::enumerate(dag)) {
    DictionaryAttr node = cast<DictionaryAttr>(raw);
    unsigned depth = 0;
    SmallVector<int64_t> operands(
        cast<DenseI64ArrayAttr>(node.get("operands")).asArrayRef());
    for (int64_t operand : operands) {
      if (operand < 0 || operand >= static_cast<int64_t>(ordinal))
        return failure();
      depth = std::max(depth, depths[operand] + 1);
    }
    depths[ordinal] = depth;
    maxDepth = std::max(maxDepth, depth);
  }

  int64_t nextRank = 0;
  for (unsigned depth = 0; depth <= maxDepth; ++depth) {
    SmallVector<size_t> layer;
    SmallVector<std::string> unique;
    for (size_t ordinal = 0; ordinal < dag.size(); ++ordinal) {
      if (depths[ordinal] != depth)
        continue;
      DictionaryAttr node = cast<DictionaryAttr>(dag[ordinal]);
      std::string tuple =
          "opcode=" + attributeText(node.get("opcode")) +
          ";type=" + attributeText(node.get("result_type")) +
          ";attributes=" + attributeText(node.get("attributes")) +
          ";operands=[";
      bool first = true;
      for (int64_t operand :
           cast<DenseI64ArrayAttr>(node.get("operands")).asArrayRef()) {
        if (!first)
          tuple.push_back(',');
        first = false;
        tuple.append(std::to_string(ranks[operand]));
      }
      tuple.push_back(']');
      tuples[ordinal] = tuple;
      layer.push_back(ordinal);
      unique.push_back(std::move(tuple));
    }
    llvm::sort(unique);
    unique.erase(std::unique(unique.begin(), unique.end()), unique.end());
    for (size_t ordinal : layer)
      ranks[ordinal] = nextRank + llvm::lower_bound(unique, tuples[ordinal]) -
                       unique.begin();
    nextRank += unique.size();
  }
  return ranks;
}

namespace {

struct ResourceIdentity {
  Operation *operation = nullptr;
  std::string ownerPath;
  std::string stableId;
};

std::string caseScopeIdentity(Operation *operation) {
  auto moduleCase = operation->getParentOfType<ac::ModuleCaseOp>();
  if (!moduleCase)
    return {};
  auto family = dyn_cast_or_null<ac::ModuleOp>(moduleCase->getParentOp());
  if (!family)
    return {};
  return "module=" + family.getSymName().str() +
         ";arguments=" + attributeText(moduleCase.getArguments());
}

using IdentityScopes = std::map<std::string, std::set<std::string>>;

std::string scopedDebugIdentity(StringRef localIdentity, StringRef scope,
                                const IdentityScopes &identityScopes) {
  auto found = identityScopes.find(localIdentity.str());
  if (found == identityScopes.end() || found->second.size() <= 1)
    return localIdentity.str();
  return "case={" + (scope.empty() ? std::string("model") : scope.str()) +
         "}:" + localIdentity.str();
}

ResourceIdentity resourceIdentity(Operation *scope,
                                  FlatSymbolRefAttr reference) {
  Operation *resource = ac::lookupRuntimeSymbol(scope, reference);
  if (!resource)
    return {};
  auto owner = resource->getAttrOfType<StringAttr>("owner");
  auto stable = resource->getAttrOfType<StringAttr>("stable_id");
  return {resource, owner ? owner.getValue().str() : "",
          stable ? stable.getValue().str() : ""};
}

Operation *resourceByStableIdentity(Operation *scope, StringRef stableId) {
  Operation *result = nullptr;
  auto consider = [&](Operation *operation) {
    if (result || !operation->getAttrOfType<StringAttr>("owner"))
      return;
    auto stable = operation->getAttrOfType<StringAttr>("stable_id");
    if (stable && stable.getValue() == stableId)
      result = operation;
  };
  if (auto moduleCase = scope->getParentOfType<ac::ModuleCaseOp>()) {
    moduleCase.walk(consider);
    return result;
  }
  auto model = scope->getParentOfType<ModuleOp>();
  model.walk([&](Operation *operation) {
    if (caseScopeIdentity(operation).empty())
      consider(operation);
  });
  return result;
}

} // namespace

LogicalResult buildRuleEffectGraph(ModuleOp model, RuleEffectGraph &graph) {
  graph = {};
  ACDataFlowAnalyzer dataFlow(model.getOperation());
  if (failed(dataFlow.run()))
    return model.emitError(
        "AC value constraint analysis failed before rule effect graph");

  SmallVector<Operation *> rules;
  IdentityScopes ruleScopes;
  IdentityScopes resourceScopes;
  model.walk([&](Operation *operation) {
    if (isa<ac::RuleOp, ac::FiringOp>(operation)) {
      rules.push_back(operation);
      ruleScopes[stableRuleId(operation)].insert(caseScopeIdentity(operation));
    }
    auto owner = operation->getAttrOfType<StringAttr>("owner");
    auto stable = operation->getAttrOfType<StringAttr>("stable_id");
    if (owner && stable)
      resourceScopes[stable.getValue().str()].insert(
          caseScopeIdentity(operation));
  });
  auto ruleIdentity = [&](Operation *operation) {
    return scopedDebugIdentity(stableRuleId(operation),
                               caseScopeIdentity(operation), ruleScopes);
  };
  auto ownerIdentity = [&](const ResourceIdentity &resource) {
    return scopedDebugIdentity(resource.stableId,
                               caseScopeIdentity(resource.operation),
                               resourceScopes);
  };
  llvm::sort(rules, [&](Operation *left, Operation *right) {
    return ruleIdentity(left) < ruleIdentity(right);
  });

  llvm::SmallSet<std::string, 32> nodeIds;
  std::vector<GraphFootprint> graphFootprints;
  auto uniqueNode = [&](StringRef id, StringRef kind, StringRef label,
                        StringRef status = "present") {
    if (nodeIds.insert(id.str()).second)
      addNode(graph, id, kind, label, status);
  };

  for (Operation *operation : rules) {
    const std::string localRuleId = stableRuleId(operation);
    const std::string ruleId = ruleIdentity(operation);
    const std::string ruleNode = "rule:" + ruleId;
    ArrayAttr dag =
        ruleArray(operation, "ac.rule.expression_dag", "ac.expression_dag");
    ArrayAttr footprints =
        ruleArray(operation, "ac.rule.footprints_exact", "ac.footprints_exact");
    if (!dag || !footprints)
      return operation->emitOpError(
          "rule effect graph requires verified exact rule summary");
    if (failed(ac::verifyExactRuleEffectSummary(operation, dag, footprints)))
      return failure();
    SmallVector<ac::ExactRuleFootprintInput> liveFootprints =
        ac::collectExactRuleFootprintInputs(operation);
    FailureOr<std::vector<int64_t>> expressionRanks =
        canonicalRuleExpressionRanks(dag);
    if (failed(expressionRanks))
      return operation->emitOpError(
          "cannot rank malformed exact rule-expression DAG");
    if (liveFootprints.size() != footprints.size())
      return operation->emitOpError(
          "exact footprint/live endpoint count changed after verification");
    const std::string ruleScope = caseScopeIdentity(operation);
    uniqueNode(ruleNode, "rule",
               ruleScopes[localRuleId].size() > 1
                   ? localRuleId + " [" + ruleScope + "]"
                   : localRuleId);

    for (auto [raw, live] : llvm::zip_equal(footprints, liveFootprints)) {
      DictionaryAttr footprint = cast<DictionaryAttr>(raw);
      const std::string owner =
          cast<StringAttr>(footprint.get("owner")).getValue().str();
      const std::string ownerStable =
          cast<StringAttr>(footprint.get("owner_stable_id")).getValue().str();
      auto resourceReference =
          cast<FlatSymbolRefAttr>(footprint.get("resource"));
      const std::string resource = resourceReference.getValue().str();
      const ResourceIdentity resolvedResource =
          resourceIdentity(operation, resourceReference);
      if (!resolvedResource.operation ||
          resolvedResource.stableId != ownerStable)
        return operation->emitOpError(
            "exact footprint resource lacks matching canonical owner identity");
      const std::string qualifiedOwner = ownerIdentity(resolvedResource);
      const std::string access =
          cast<StringAttr>(footprint.get("access")).getValue().str();
      const bool wholeEntry =
          cast<BoolAttr>(footprint.get("whole_entry")).getValue();
      ArrayAttr fields = cast<ArrayAttr>(footprint.get("fields"));
      const bool allEntries =
          cast<BoolAttr>(footprint.get("all_entries")).getValue();
      const std::string indexExpression =
          allEntries ? "all_entries"
                     : "r" + std::to_string(
                                 (*expressionRanks)[cast<IntegerAttr>(
                                                        footprint.get("index"))
                                                        .getInt()]);
      const std::string predicateExpression =
          "r" +
          std::to_string(
              (*expressionRanks)[cast<IntegerAttr>(footprint.get("predicate"))
                                     .getInt()]);
      const std::string source =
          attributeText(footprint.get("source_provenance"));
      const std::string ownerNode = "state_owner:" + qualifiedOwner;
      const std::string resourceScope =
          caseScopeIdentity(resolvedResource.operation);
      uniqueNode(ownerNode, "state_owner",
                 owner + " [" + ownerStable + "]" +
                     (resourceScopes[ownerStable].size() > 1
                          ? " [" + resourceScope + "]"
                          : ""));
      const std::string footprintNode =
          "footprint:" + qualifiedOwner + ":rule=" + ruleId +
          ":resource=" + resource + ":" + access + ":index={" +
          indexExpression + "}:predicate={" + predicateExpression +
          "}:f=" + fieldText(fields, wholeEntry) + ":endpoint=" +
          cast<StringAttr>(footprint.get("endpoint")).getValue().str();
      uniqueNode(footprintNode, "state_footprint", resource + ":" + access);
      addEdge(graph, ruleNode, footprintNode, "state_" + access, "exact",
              "Decision0271.exact_summary",
              "index={" + indexExpression + "};predicate={" +
                  predicateExpression + "};fields=" +
                  fieldText(fields, wholeEntry) + ";source=" + source);
      addEdge(graph, footprintNode, ownerNode, "owned_by", "exact",
              "Decision0271.owner_identity",
              "owner_path=" + owner + ";owner_stable_id=" + ownerStable);
      graphFootprints.push_back(
          {localRuleId, ruleNode, footprintNode, owner, ownerStable,
           qualifiedOwner, access, fieldText(fields, wholeEntry),
           indexExpression, predicateExpression, fields, wholeEntry, live});
    }

    ArrayAttr resources = ruleArray(operation, "ac.rule.transaction_resources",
                                    "ac.transaction_resources");
    if (!resources)
      return operation->emitOpError(
          "rule effect graph requires exact transaction resources");
    for (Attribute raw : resources) {
      DictionaryAttr resource = cast<DictionaryAttr>(raw);
      auto kind = cast<ac::ActivationResourceKindAttr>(resource.get("kind"));
      std::string resourceKind =
          stringifyActivationResourceKind(kind.getValue()).str();
      std::string identity;
      std::string resourceDetail;
      if (auto ordinal = resource.getAs<IntegerAttr>("ordinal")) {
        identity = ruleId + ":" + resourceKind + ":" +
                   std::to_string(ordinal.getInt());
        resourceDetail = "rule_stable_id=" + ruleId +
                         ";ordinal=" + std::to_string(ordinal.getInt());
      } else {
        auto reference = cast<FlatSymbolRefAttr>(resource.get("resource"));
        ResourceIdentity resolved = resourceIdentity(operation, reference);
        if (resolved.stableId.empty())
          return operation->emitOpError(
              "transaction resource lacks canonical owner identity");
        identity = ownerIdentity(resolved);
        resourceDetail = "owner_path=" + resolved.ownerPath +
                         ";owner_stable_id=" + resolved.stableId;
      }
      const std::string node = "resource:" + resourceKind + ":" + identity;
      uniqueNode(node, resourceKind, identity);
      const StringRef edgeKind =
          kind.getValue() == ac::ActivationResourceKind::InputQueue
              ? "queue_consume"
              : (kind.getValue() == ac::ActivationResourceKind::OutputQueue
                     ? "queue_produce"
                     : (kind.getValue() == ac::ActivationResourceKind::Slot
                            ? "slot_resource"
                            : "state_resource"));
      addEdge(graph, ruleNode, node, edgeKind, "exact",
              "ac.transaction_resources", resourceKind + ";" + resourceDetail);
    }

    ArrayAttr arbitration =
        ruleArray(operation, "ac.rule.arbitration_membership",
                  "ac.arbitration_membership");
    if (!arbitration)
      return operation->emitOpError(
          "rule effect graph requires arbitration membership");
    for (Attribute raw : arbitration) {
      DictionaryAttr membership = cast<DictionaryAttr>(raw);
      const std::string owner =
          cast<FlatSymbolRefAttr>(membership.get("owner")).getValue().str();
      ResourceIdentity resolved = resourceIdentity(
          operation, cast<FlatSymbolRefAttr>(membership.get("owner")));
      if (resolved.stableId.empty())
        return operation->emitOpError(
            "arbitration domain lacks canonical owner identity");
      const int64_t rank =
          cast<IntegerAttr>(membership.get("declared_rank")).getInt();
      const std::string qualifiedOwner = ownerIdentity(resolved);
      const std::string domain = "arbitration:" + qualifiedOwner;
      uniqueNode(domain, "arbitration_domain",
                 owner + " [" + resolved.stableId + "]");
      addEdge(graph, ruleNode, domain, "arbitration", "priority",
              "VerifyValueConstraints.explicitPriority",
              "owner_path=" + resolved.ownerPath + ";owner_stable_id=" +
                  resolved.stableId + ";rank=" + std::to_string(rank) +
                  ";policy=priority;resolution=winner_takes_transaction");
    }

    const std::string recovery = "recovery:" + ruleId;
    uniqueNode(recovery, "recovery_domain", "absent", "absent");
    addEdge(graph, ruleNode, recovery, "recovery", "absent", {},
            "not_declared");
    const std::string obligation = "obligation_slot:" + ruleId;
    uniqueNode(obligation, "obligation_linkage", "reserved", "absent");
    addEdge(graph, ruleNode, obligation, "obligation", "absent", {},
            "reserved_for_F3");
  }

  WriterArbitrationAnalysis arbitration;
  LogicalResult writerResult =
      analyzeWriterArbitration(model, dataFlow, &arbitration);
  graph.arbitrationCycleRejected = arbitration.hasCrossOwnerCycle;

  llvm::sort(graphFootprints,
             [](const GraphFootprint &left, const GraphFootprint &right) {
               return left.nodeId < right.nodeId;
             });
  for (size_t leftIndex = 0; leftIndex < graphFootprints.size(); ++leftIndex) {
    const GraphFootprint &left = graphFootprints[leftIndex];
    for (size_t rightIndex = leftIndex + 1; rightIndex < graphFootprints.size();
         ++rightIndex) {
      const GraphFootprint &right = graphFootprints[rightIndex];
      if (left.ownerIdentity != right.ownerIdentity)
        continue;
      const FootprintRelationProofKind relation = proveFootprintRelation(
          dataFlow,
          {left.fields, left.live.index, left.live.predicate, left.wholeEntry,
           false},
          {right.fields, right.live.index, right.live.predicate,
           right.wholeEntry, false});
      const bool leftRead = left.access == "read";
      const bool rightRead = right.access == "read";
      std::string result;
      std::string proof;
      std::string provenance;
      if (!leftRead && !rightRead) {
        const WriterConflictProof *outcome = nullptr;
        for (const WriterConflictProof &candidate : arbitration.conflicts)
          if ((candidate.leftOperation == left.live.endpoint &&
               candidate.rightOperation == right.live.endpoint) ||
              (candidate.leftOperation == right.live.endpoint &&
               candidate.rightOperation == left.live.endpoint)) {
            outcome = &candidate;
            break;
          }
        result = outcome ? outcome->result : "rejected";
        proof = outcome ? stringifyWriterConflictProofKind(outcome->kind)
                        : "writer_analysis_rejected";
        provenance = outcome ? outcome->provenance
                             : "VerifyValueConstraints.analysisFailure";
      } else if (relation != FootprintRelationProofKind::Overlap) {
        result = "coexist";
        proof = stringifyFootprintRelationProofKind(relation);
        provenance =
            relation == FootprintRelationProofKind::FieldDisjoint
                ? "VerifyValueConstraints.fieldsAreDisjoint"
                : (relation == FootprintRelationProofKind::IndexDisjoint
                       ? "ACDataFlowAnalyzer.provesDisjoint"
                       : "ACDataFlowAnalyzer.provesMutuallyExclusive");
      } else if (leftRead || rightRead) {
        result = "committed_old_state";
        proof = leftRead && rightRead ? "read_read_committed_old_state"
                                      : "read_write_committed_old_state";
        provenance = "Decision0271.committedOldState";
      }
      const std::string relationNode = "interaction:" + left.ownerIdentity +
                                       ":left={" + left.nodeId + "}:right={" +
                                       right.nodeId + "}";
      uniqueNode(relationNode, "interaction",
                 left.ownerIdentity + ":" + left.ruleId + ":" + right.ruleId,
                 result);
      const std::string detail =
          "owner_path=" + left.ownerPath +
          ";owner_stable_id=" + left.ownerStableId +
          ";left_access=" + left.access + ";right_access=" + right.access +
          ";left_fields=" + left.fieldsText +
          ";right_fields=" + right.fieldsText + ";left_index={" +
          left.indexExpression + "};left_predicate={" +
          left.predicateExpression + "};right_index={" + right.indexExpression +
          "};right_predicate={" + right.predicateExpression +
          "};provenance=" + provenance;
      addEdge(graph, left.nodeId, relationNode, "interaction", result, proof,
              detail);
      addEdge(graph, right.nodeId, relationNode, "interaction", result, proof,
              detail);
    }
  }

  // Direct declarative writers have no rule exact summaries. Preserve their
  // shared verifier proof records as endpoint relations. Rule-owned footprint
  // pairs above are keyed by exact footprint identity instead.
  for (const WriterConflictProof &conflict : arbitration.conflicts) {
    if (!conflict.leftRule.empty() && !conflict.rightRule.empty())
      continue;
    ResourceIdentity conflictOwner;
    Operation *conflictOperation = conflict.leftOperation
                                       ? conflict.leftOperation
                                       : conflict.rightOperation;
    if (conflictOperation)
      if (Operation *resource = resourceByStableIdentity(
              conflictOperation, conflict.ownerStableId))
        conflictOwner = {resource, conflict.ownerPath, conflict.ownerStableId};
    const std::string conflictOwnerIdentity = conflictOwner.operation
                                                  ? ownerIdentity(conflictOwner)
                                                  : conflict.ownerStableId;
    auto endpointNode = [&](Operation *operation, StringRef rule,
                            StringRef endpoint) -> std::optional<std::string> {
      if (rule.empty())
        return "writer_endpoint:" + conflictOwnerIdentity + ":" +
               endpoint.str();
      for (const GraphFootprint &footprint : graphFootprints)
        if (footprint.live.endpoint == operation)
          return footprint.nodeId;
      return std::nullopt;
    };
    std::optional<std::string> leftNode = endpointNode(
        conflict.leftOperation, conflict.leftRule, conflict.leftEndpoint);
    std::optional<std::string> rightNode = endpointNode(
        conflict.rightOperation, conflict.rightRule, conflict.rightEndpoint);
    if (!leftNode || !rightNode)
      return model.emitError(
          "writer arbitration report cannot resolve an exact rule footprint");
    std::string firstIdentity = *leftNode;
    std::string secondIdentity = *rightNode;
    if (secondIdentity < firstIdentity)
      std::swap(firstIdentity, secondIdentity);
    const std::string conflictNode = "conflict:" + conflictOwnerIdentity +
                                     ":left={" + firstIdentity + "}:right={" +
                                     secondIdentity + "}";
    uniqueNode(conflictNode, "conflict",
               conflictOwnerIdentity + ":" + firstIdentity + ":" +
                   secondIdentity,
               conflict.result);
    if (conflict.leftRule.empty())
      uniqueNode(*leftNode, "writer_endpoint", conflict.leftEndpoint);
    if (conflict.rightRule.empty())
      uniqueNode(*rightNode, "writer_endpoint", conflict.rightEndpoint);
    addEdge(graph, *leftNode, conflictNode, "conflict", conflict.result,
            stringifyWriterConflictProofKind(conflict.kind),
            "owner_path=" + conflict.ownerPath + ";owner_stable_id=" +
                conflict.ownerStableId + ";provenance=" + conflict.provenance);
    addEdge(graph, *rightNode, conflictNode, "conflict", conflict.result,
            stringifyWriterConflictProofKind(conflict.kind),
            "owner_path=" + conflict.ownerPath + ";owner_stable_id=" +
                conflict.ownerStableId + ";provenance=" + conflict.provenance);
  }
  std::set<std::tuple<std::string, std::string, std::string>> emittedOrdering;
  for (const WriterOrderingProof &ordering : arbitration.ordering) {
    if (ordering.beforeRule.empty() || ordering.afterRule.empty())
      continue;
    std::set<std::tuple<std::string, std::string, std::string>> scopedRules;
    for (const GraphFootprint &before : graphFootprints) {
      if (before.ownerPath != ordering.ownerPath ||
          before.ownerStableId != ordering.ownerStableId ||
          before.ruleId != ordering.beforeRule)
        continue;
      for (const GraphFootprint &after : graphFootprints)
        if (after.ownerIdentity == before.ownerIdentity &&
            after.ruleId == ordering.afterRule)
          scopedRules.insert(
              {before.ruleNode, after.ruleNode, before.ownerIdentity});
    }
    for (const auto &[beforeNode, afterNode, scopedOwner] : scopedRules) {
      if (!emittedOrdering.insert({beforeNode, afterNode, scopedOwner}).second)
        continue;
      addEdge(graph, beforeNode, afterNode, "ordering",
              arbitration.hasCrossOwnerCycle ? "rejected" : "priority",
              arbitration.hasCrossOwnerCycle
                  ? "cross_owner_priority_cycle_rejected"
                  : "VerifyValueConstraints.crossOwnerAcyclicity",
              "owner_path=" + ordering.ownerPath +
                  ";owner_stable_id=" + ordering.ownerStableId +
                  ";before_endpoint=" + ordering.beforeEndpoint +
                  ";after_endpoint=" + ordering.afterEndpoint +
                  ";ranks=" + std::to_string(ordering.beforeRank) + "<" +
                  std::to_string(ordering.afterRank));
    }
  }
  if (arbitration.ordering.empty()) {
    uniqueNode("ordering:absent", "ordering", "absent", "absent");
  }

  llvm::sort(graph.nodes, [](const auto &left, const auto &right) {
    return std::tie(left.id, left.kind, left.label, left.status) <
           std::tie(right.id, right.kind, right.label, right.status);
  });
  llvm::sort(graph.edges, [](const auto &left, const auto &right) {
    return std::tie(left.from, left.to, left.kind, left.result, left.proof,
                    left.detail) < std::tie(right.from, right.to, right.kind,
                                            right.result, right.proof,
                                            right.detail);
  });
  return writerResult;
}

void printRuleEffectGraphJSON(const RuleEffectGraph &graph,
                              llvm::raw_ostream &output) {
  llvm::json::Array nodes;
  for (const RuleEffectGraphNode &node : graph.nodes)
    nodes.push_back(llvm::json::Object{{"id", node.id},
                                       {"kind", node.kind},
                                       {"label", node.label},
                                       {"status", node.status}});
  llvm::json::Array edges;
  for (const RuleEffectGraphEdge &edge : graph.edges)
    edges.push_back(llvm::json::Object{{"from", edge.from},
                                       {"to", edge.to},
                                       {"kind", edge.kind},
                                       {"result", edge.result},
                                       {"proof", edge.proof},
                                       {"detail", edge.detail}});
  llvm::json::Object root{
      {"format", "ac-rule-effect-graph-debug-v1"},
      {"identity_format", false},
      {"arbitration_cycle_rejected", graph.arbitrationCycleRejected},
      {"nodes", std::move(nodes)},
      {"edges", std::move(edges)}};
  output << llvm::formatv("{0:2}\n", llvm::json::Value(std::move(root)));
}

void printRuleEffectGraphDOT(const RuleEffectGraph &graph,
                             llvm::raw_ostream &output) {
  output << "digraph rule_effect_graph {\n";
  output
      << "  // Debug/evidence view only; not an identity or release format.\n";
  for (const RuleEffectGraphNode &node : graph.nodes)
    output << "  \"" << dotEscape(node.id) << "\" [label=\""
           << dotEscape(node.label) << "\\n"
           << dotEscape(node.kind) << "\\n"
           << dotEscape(node.status) << "\"];\n";
  for (const RuleEffectGraphEdge &edge : graph.edges)
    output << "  \"" << dotEscape(edge.from) << "\" -> \"" << dotEscape(edge.to)
           << "\" [label=\"" << dotEscape(edge.kind)
           << (edge.result.empty() ? "" : ":" + dotEscape(edge.result))
           << (edge.proof.empty() ? "" : "\\n" + dotEscape(edge.proof))
           << "\"];\n";
  output << "}\n";
}

} // namespace acir
