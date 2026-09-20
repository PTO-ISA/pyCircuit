#include "acir/Analysis/WriterArbitrationAnalysis.h"

#include "acir/Analysis/VariableAnalysis.h"
#include "acir/Dialect/ACIR/ACIROps.h"

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/StringSet.h"

#include <optional>

using namespace mlir;

namespace acir {
namespace {

template <typename Declaration>
Declaration resolveFlatDeclaration(Operation *operation,
                                   FlatSymbolRefAttr reference) {
  for (Operation *ancestor = operation->getParentOp(); ancestor;
       ancestor = ancestor->getParentOp())
    for (Region &region : ancestor->getRegions())
      for (Block &block : region)
        for (Declaration declaration : block.getOps<Declaration>())
          if (declaration.getSymName() == reference.getValue())
            return declaration;
  return {};
}

struct WriterEndpoint {
  Operation *operation = nullptr;
  ac::TableOp owner;
  Operation *scope = nullptr;
  std::string endpointStableId;
  ac::WriterPriorityAttr request;
  Value index;
  Value presence;
  ArrayAttr fields;
  StringRef mode;
  bool masked = false;
};

bool fieldsAreDisjoint(ArrayAttr left, bool leftWhole, ArrayAttr right,
                       bool rightWhole) {
  if (leftWhole || rightWhole)
    return false;
  llvm::StringSet<> fields;
  for (Attribute raw : left) {
    StringRef field = cast<StringAttr>(raw).getValue();
    if (field == "$entry")
      return false;
    fields.insert(field);
  }
  for (Attribute raw : right) {
    StringRef field = cast<StringAttr>(raw).getValue();
    if (field == "$entry" || fields.contains(field))
      return false;
  }
  return true;
}

StringRef writerStableId(Operation *scope) {
  if (auto rule = dyn_cast<ac::RuleOp>(scope))
    return rule.getStableId();
  if (auto firing = dyn_cast<ac::FiringOp>(scope))
    return firing.getStableId();
  return {};
}

std::string endpointRule(WriterEndpoint &endpoint) {
  StringRef stable = writerStableId(endpoint.scope);
  return stable.str();
}

WriterEndpoint tableWriterEndpoint(Operation *operation, ac::TableOp table) {
  WriterEndpoint endpoint;
  endpoint.operation = operation;
  endpoint.owner = table;
  endpoint.request =
      operation->getAttrOfType<ac::WriterPriorityAttr>("ac.arbitration");
  if (auto proposal = dyn_cast<ac::TableProposeOp>(operation)) {
    endpoint.scope = proposal->getParentOp();
    endpoint.endpointStableId = writerStableId(endpoint.scope).str();
    endpoint.index = proposal.getIndex();
    endpoint.presence = proposal.getWhen();
    endpoint.fields = proposal.getWriteFieldsAttr();
    endpoint.mode = proposal.getMode();
  } else if (auto write = dyn_cast<ac::TableWriteOp>(operation)) {
    endpoint.scope = operation;
    auto stable = write->getAttrOfType<StringAttr>("ac.endpoint_id");
    endpoint.endpointStableId = stable ? stable.getValue().str() : "";
    endpoint.index =
        cast<ac::TableYieldOp>(write.getAddress().front().getTerminator())
            .getValue();
    endpoint.presence =
        cast<ac::TableYieldOp>(write.getEnable().front().getTerminator())
            .getValue();
    endpoint.fields = write.getWriteFieldsAttr();
    endpoint.mode = write.getMode();
  } else if (auto write = dyn_cast<ac::TableMaskedWriteOp>(operation)) {
    endpoint.scope = operation;
    auto stable = write->getAttrOfType<StringAttr>("ac.endpoint_id");
    endpoint.endpointStableId = stable ? stable.getValue().str() : "";
    endpoint.presence =
        cast<ac::TableYieldOp>(write.getEnable().front().getTerminator())
            .getValue();
    endpoint.fields = write.getWriteFieldsAttr();
    endpoint.mode = write.getMode();
    endpoint.masked = true;
  }
  return endpoint;
}

void recordConflict(WriterArbitrationAnalysis *report, WriterEndpoint &left,
                    WriterEndpoint &right, WriterConflictProofKind kind,
                    StringRef result, StringRef provenance) {
  if (!report)
    return;
  report->conflicts.push_back(
      {left.operation, right.operation, left.endpointStableId,
       right.endpointStableId, endpointRule(left), endpointRule(right),
       left.owner.getOwner().str(), left.owner.getStableId().str(), kind,
       result.str(), provenance.str()});
}

} // namespace

const char *stringifyWriterConflictProofKind(WriterConflictProofKind kind) {
  switch (kind) {
  case WriterConflictProofKind::FieldDisjoint:
    return "field_disjoint";
  case WriterConflictProofKind::IndexDisjoint:
    return "index_disjoint";
  case WriterConflictProofKind::PredicateExclusive:
    return "predicate_exclusive";
  case WriterConflictProofKind::DeclarativeAllocationOrder:
    return "declarative_allocation_order";
  case WriterConflictProofKind::ExplicitPriority:
    return "explicit_priority";
  case WriterConflictProofKind::DuplicateRankRejected:
    return "duplicate_rank_rejected";
  case WriterConflictProofKind::DuplicateIdentityRejected:
    return "duplicate_identity_rejected";
  case WriterConflictProofKind::InconsistentRankRejected:
    return "inconsistent_rank_rejected";
  case WriterConflictProofKind::SameEndpointRejected:
    return "same_endpoint_rejected";
  case WriterConflictProofKind::MissingIdentityRejected:
    return "missing_identity_rejected";
  case WriterConflictProofKind::MissingPriorityRejected:
    return "missing_priority_rejected";
  }
  llvm_unreachable("unknown writer conflict proof kind");
}

const char *
stringifyFootprintRelationProofKind(FootprintRelationProofKind kind) {
  switch (kind) {
  case FootprintRelationProofKind::Overlap:
    return "overlap";
  case FootprintRelationProofKind::FieldDisjoint:
    return "field_disjoint";
  case FootprintRelationProofKind::IndexDisjoint:
    return "index_disjoint";
  case FootprintRelationProofKind::PredicateExclusive:
    return "predicate_exclusive";
  }
  llvm_unreachable("unknown footprint relation proof kind");
}

FootprintRelationProofKind
proveFootprintRelation(ACDataFlowAnalyzer &analysis,
                       const FootprintRelationInput &left,
                       const FootprintRelationInput &right) {
  if (fieldsAreDisjoint(left.fields, left.wholeEntry, right.fields,
                        right.wholeEntry))
    return FootprintRelationProofKind::FieldDisjoint;
  if (!left.masked && !right.masked && left.index && right.index &&
      analysis.provesDisjoint(left.index, right.index))
    return FootprintRelationProofKind::IndexDisjoint;
  if (left.predicate && right.predicate &&
      analysis.provesMutuallyExclusive(left.predicate, right.predicate))
    return FootprintRelationProofKind::PredicateExclusive;
  return FootprintRelationProofKind::Overlap;
}

LogicalResult analyzeWriterArbitration(ModuleOp model,
                                       ACDataFlowAnalyzer &analysis,
                                       WriterArbitrationAnalysis *report) {
  if (report)
    *report = {};
  SmallVector<WriterEndpoint> endpoints;
  LogicalResult result = success();
  model.walk([&](Operation *operation) {
    if (failed(result))
      return;
    FlatSymbolRefAttr reference;
    if (auto proposal = dyn_cast<ac::TableProposeOp>(operation))
      reference = proposal.getTableAttr();
    else if (auto write = dyn_cast<ac::TableWriteOp>(operation))
      reference = write.getTableAttr();
    else if (auto write = dyn_cast<ac::TableMaskedWriteOp>(operation))
      reference = write.getTableAttr();
    else
      return;
    ac::TableOp table =
        resolveFlatDeclaration<ac::TableOp>(operation, reference);
    WriterEndpoint endpoint = tableWriterEndpoint(operation, table);
    if (!table) {
      result =
          operation->emitOpError("writer endpoint requires a resolved owner");
      return;
    }
    endpoints.push_back(std::move(endpoint));
  });
  if (failed(result))
    return result;

  llvm::DenseMap<Operation *, llvm::StringMap<int64_t>> endpointRanksByOwner;
  llvm::DenseMap<Operation *, llvm::StringMap<Operation *>>
      endpointScopesByOwner;
  llvm::DenseMap<Operation *, llvm::StringMap<std::string>> rankOwners;
  auto findEndpoint = [&](Operation *owner, StringRef stableId,
                          std::optional<int64_t> rank) -> WriterEndpoint * {
    for (WriterEndpoint &candidate : endpoints)
      if (candidate.operation != nullptr &&
          candidate.owner.getOperation() == owner &&
          candidate.endpointStableId == stableId && candidate.request &&
          (!rank || candidate.request.getRank() == *rank))
        return &candidate;
    return nullptr;
  };
  for (WriterEndpoint &endpoint : endpoints) {
    if (!endpoint.request)
      continue;
    const int64_t rank = endpoint.request.getRank();
    Operation *owner = endpoint.owner.getOperation();
    auto &endpointScopes = endpointScopesByOwner[owner];
    auto knownScope = endpointScopes.find(endpoint.endpointStableId);
    if (knownScope != endpointScopes.end() &&
        knownScope->second != endpoint.scope) {
      if (WriterEndpoint *previous =
              findEndpoint(owner, endpoint.endpointStableId, std::nullopt))
        recordConflict(report, *previous, endpoint,
                       WriterConflictProofKind::DuplicateIdentityRejected,
                       "rejected", "VerifyValueConstraints.duplicateIdentity");
      return endpoint.operation->emitOpError()
             << "duplicate stable writer endpoint identity '"
             << endpoint.endpointStableId << "' for owner @"
             << endpoint.owner.getSymName();
    }
    endpointScopes[endpoint.endpointStableId] = endpoint.scope;
    auto &endpointRanks = endpointRanksByOwner[owner];
    auto known = endpointRanks.find(endpoint.endpointStableId);
    if (known != endpointRanks.end() && known->second != rank) {
      if (WriterEndpoint *previous =
              findEndpoint(owner, endpoint.endpointStableId, known->second))
        recordConflict(report, *previous, endpoint,
                       WriterConflictProofKind::InconsistentRankRejected,
                       "rejected", "VerifyValueConstraints.inconsistentRank");
      return endpoint.operation->emitOpError(
          "stable writer endpoint has inconsistent declared ranks");
    }
    endpointRanks[endpoint.endpointStableId] = rank;
    std::string rankKey = std::to_string(rank);
    auto &owners = rankOwners[owner];
    auto tied = owners.find(rankKey);
    if (tied != owners.end() && tied->second != endpoint.endpointStableId) {
      if (WriterEndpoint *previous = findEndpoint(owner, tied->second, rank))
        recordConflict(report, *previous, endpoint,
                       WriterConflictProofKind::DuplicateRankRejected,
                       "rejected", "VerifyValueConstraints.duplicateRank");
      return endpoint.operation->emitOpError()
             << "duplicate writer priority rank " << rank << " for owner @"
             << endpoint.owner.getSymName();
    }
    owners[rankKey] = endpoint.endpointStableId;
  }

  SmallVector<Operation *> precedenceNodes;
  llvm::DenseMap<Operation *, unsigned> precedenceNodeIndex;
  for (WriterEndpoint &endpoint : endpoints) {
    if (!endpoint.request)
      continue;
    auto [iterator, inserted] =
        precedenceNodeIndex.try_emplace(endpoint.scope, precedenceNodes.size());
    if (inserted)
      precedenceNodes.push_back(endpoint.scope);
  }
  SmallVector<SmallVector<bool>> precedenceEdges(
      precedenceNodes.size(), SmallVector<bool>(precedenceNodes.size(), false));
  SmallVector<unsigned> indegree(precedenceNodes.size(), 0);
  llvm::StringSet<> recordedOrdering;
  for (size_t leftIndex = 0; leftIndex < endpoints.size(); ++leftIndex) {
    WriterEndpoint &left = endpoints[leftIndex];
    if (!left.request)
      continue;
    for (size_t rightIndex = leftIndex + 1; rightIndex < endpoints.size();
         ++rightIndex) {
      WriterEndpoint &right = endpoints[rightIndex];
      if (!right.request || left.owner != right.owner ||
          left.scope == right.scope)
        continue;
      const bool leftBefore = left.request.getRank() < right.request.getRank();
      const unsigned before =
          precedenceNodeIndex.lookup(leftBefore ? left.scope : right.scope);
      const unsigned after =
          precedenceNodeIndex.lookup(leftBefore ? right.scope : left.scope);
      WriterEndpoint &beforeEndpoint = leftBefore ? left : right;
      WriterEndpoint &afterEndpoint = leftBefore ? right : left;
      if (report) {
        const std::string proofKey =
            beforeEndpoint.owner.getStableId().str() + "\x1f" +
            beforeEndpoint.endpointStableId + "\x1f" +
            afterEndpoint.endpointStableId + "\x1f" +
            std::to_string(beforeEndpoint.request.getRank()) + "\x1f" +
            std::to_string(afterEndpoint.request.getRank());
        if (recordedOrdering.insert(proofKey).second)
          report->ordering.push_back(
              {endpointRule(beforeEndpoint), endpointRule(afterEndpoint),
               beforeEndpoint.endpointStableId, afterEndpoint.endpointStableId,
               beforeEndpoint.owner.getOwner().str(),
               beforeEndpoint.owner.getStableId().str(),
               beforeEndpoint.request.getRank(),
               afterEndpoint.request.getRank()});
      }
      if (!precedenceEdges[before][after]) {
        precedenceEdges[before][after] = true;
        ++indegree[after];
      }
    }
  }
  SmallVector<unsigned> ready;
  for (auto [index, degree] : llvm::enumerate(indegree))
    if (degree == 0)
      ready.push_back(index);
  size_t visited = 0;
  while (!ready.empty()) {
    const unsigned node = ready.pop_back_val();
    ++visited;
    for (size_t successor = 0; successor < precedenceEdges.size(); ++successor)
      if (precedenceEdges[node][successor] && --indegree[successor] == 0)
        ready.push_back(successor);
  }
  if (visited != precedenceNodes.size()) {
    if (report)
      report->hasCrossOwnerCycle = true;
    return model.emitError(
        "writer arbitration precedence contains a cross-owner cycle");
  }

  for (size_t rightIndex = 1; rightIndex < endpoints.size(); ++rightIndex) {
    for (size_t leftIndex = 0; leftIndex < rightIndex; ++leftIndex) {
      WriterEndpoint &left = endpoints[leftIndex];
      WriterEndpoint &right = endpoints[rightIndex];
      if (left.owner != right.owner)
        continue;
      const FootprintRelationProofKind relation = proveFootprintRelation(
          analysis,
          {left.fields, left.index, left.presence,
           llvm::is_contained(left.fields,
                              StringAttr::get(model.getContext(), "$entry")),
           left.masked},
          {right.fields, right.index, right.presence,
           llvm::is_contained(right.fields,
                              StringAttr::get(model.getContext(), "$entry")),
           right.masked});
      if (relation == FootprintRelationProofKind::FieldDisjoint) {
        recordConflict(report, left, right,
                       WriterConflictProofKind::FieldDisjoint, "coexist",
                       "VerifyValueConstraints.fieldsAreDisjoint");
        continue;
      }
      if (relation == FootprintRelationProofKind::IndexDisjoint) {
        recordConflict(report, left, right,
                       WriterConflictProofKind::IndexDisjoint, "coexist",
                       "ACDataFlowAnalyzer.provesDisjoint");
        continue;
      }
      if (relation == FootprintRelationProofKind::PredicateExclusive) {
        recordConflict(report, left, right,
                       WriterConflictProofKind::PredicateExclusive, "coexist",
                       "ACDataFlowAnalyzer.provesMutuallyExclusive");
        continue;
      }
      const bool leftAllocation =
          isa<ac::TableWriteOp>(left.operation) && left.mode == "replace";
      const bool rightAllocation =
          isa<ac::TableWriteOp>(right.operation) && right.mode == "replace";
      const bool leftDeclarativeField =
          (isa<ac::TableWriteOp>(left.operation) ||
           isa<ac::TableMaskedWriteOp>(left.operation)) &&
          left.mode == "field";
      const bool rightDeclarativeField =
          (isa<ac::TableWriteOp>(right.operation) ||
           isa<ac::TableMaskedWriteOp>(right.operation)) &&
          right.mode == "field";
      if ((leftAllocation && rightDeclarativeField) ||
          (rightAllocation && leftDeclarativeField)) {
        recordConflict(report, left, right,
                       WriterConflictProofKind::DeclarativeAllocationOrder,
                       "ordered", "Decision0156.declarativeAllocationOrder");
        continue;
      }
      if (left.scope == right.scope) {
        recordConflict(report, left, right,
                       WriterConflictProofKind::SameEndpointRejected,
                       "rejected", "VerifyValueConstraints.writerEndpoint");
        return right.operation->emitOpError(
            "one stable writer endpoint has unresolved overlapping proposals");
      }
      if (left.endpointStableId.empty() || right.endpointStableId.empty()) {
        recordConflict(report, left, right,
                       WriterConflictProofKind::MissingIdentityRejected,
                       "rejected", "VerifyValueConstraints.stableIdentity");
        return right.operation->emitOpError(
            "overlapping writers require stable ac.endpoint_id identity");
      }
      if (!left.request || !right.request) {
        recordConflict(report, left, right,
                       WriterConflictProofKind::MissingPriorityRejected,
                       "rejected", "VerifyValueConstraints.explicitPriority");
        return right.operation->emitOpError()
               << "same-field overlap on owner @" << right.owner.getSymName()
               << " requires explicit priority on every writer endpoint";
      }
      recordConflict(report, left, right,
                     WriterConflictProofKind::ExplicitPriority, "ordered",
                     "VerifyValueConstraints.explicitPriority");
    }
  }
  return success();
}

} // namespace acir
