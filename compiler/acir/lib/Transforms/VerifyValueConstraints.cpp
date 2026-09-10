#include "acir/Transforms/Passes.h"

#include "acir/Analysis/VariableAnalysis.h"
#include "acir/Dialect/ACIR/ACIROps.h"

#include "mlir/IR/SymbolTable.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/StringSet.h"
#include "llvm/ADT/StringMap.h"

using namespace mlir;

namespace acir {
namespace {

template <typename Declaration>
Declaration resolveFlatDeclaration(Operation *operation,
                                   FlatSymbolRefAttr reference) {
  for (Operation *ancestor = operation->getParentOp(); ancestor;
       ancestor = ancestor->getParentOp()) {
    for (Region &region : ancestor->getRegions())
      for (Block &block : region)
        for (Declaration declaration : block.getOps<Declaration>())
          if (declaration.getSymName() == reference.getValue())
            return declaration;
  }
  return {};
}

std::string constraintText(const ValueConstraint &constraint) {
  std::string text;
  llvm::raw_string_ostream stream(text);
  constraint.print(stream);
  return text;
}

LogicalResult verifyIndex(ACDataFlowAnalyzer &analysis, Operation *operation,
                          Value index, uint64_t extent, StringRef resource) {
  if (extent == 0)
    return operation->emitOpError() << resource << " extent must be positive";
  if (analysis.provesWithin(index, 0, extent - 1))
    return success();
  ValueConstraint constraint = analysis.lookupConstraint(index);
  return operation->emitOpError()
         << "cannot prove " << resource << " index is within [0, "
         << extent - 1 << "]; inferred " << constraintText(constraint);
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

static bool fieldsAreDisjoint(ArrayAttr left, ArrayAttr right) {
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

static StringRef writerStableId(Operation *scope) {
  if (auto rule = dyn_cast<ac::RuleOp>(scope))
    return rule.getStableId();
  if (auto firing = dyn_cast<ac::FiringOp>(scope))
    return firing.getStableId();
  return {};
}

static WriterEndpoint tableWriterEndpoint(Operation *operation,
                                          ac::TableOp table) {
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

static LogicalResult verifyWriterArbitration(
    ModuleOp model, ACDataFlowAnalyzer &analysis) {
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
      result = operation->emitOpError("writer endpoint requires a resolved owner");
      return;
    }
    endpoints.push_back(std::move(endpoint));
  });
  if (failed(result))
    return result;

  // A rank is an owner-local semantic choice.  Repeated proposal operations
  // belonging to the same stable endpoint share that choice; distinct stable
  // endpoints may never tie.
  llvm::DenseMap<Operation *, llvm::StringMap<int64_t>> endpointRanksByOwner;
  llvm::DenseMap<Operation *, llvm::StringMap<Operation *>> endpointScopesByOwner;
  llvm::DenseMap<Operation *, llvm::StringMap<std::string>> rankOwners;
  for (WriterEndpoint endpoint : endpoints) {
    if (!endpoint.request)
      continue;
    const int64_t rank = endpoint.request.getRank();
    Operation *owner = endpoint.owner.getOperation();
    auto &endpointScopes = endpointScopesByOwner[owner];
    auto knownScope = endpointScopes.find(endpoint.endpointStableId);
    if (knownScope != endpointScopes.end() &&
        knownScope->second != endpoint.scope)
      return endpoint.operation->emitOpError()
             << "duplicate stable writer endpoint identity '"
             << endpoint.endpointStableId << "' for owner @"
             << endpoint.owner.getSymName();
    endpointScopes[endpoint.endpointStableId] = endpoint.scope;
    auto &endpointRanks = endpointRanksByOwner[owner];
    auto known = endpointRanks.find(endpoint.endpointStableId);
    if (known != endpointRanks.end() && known->second != rank)
      return endpoint.operation->emitOpError(
          "stable writer endpoint has inconsistent declared ranks");
    endpointRanks[endpoint.endpointStableId] = rank;
    std::string rankKey = std::to_string(rank);
    auto &owners = rankOwners[owner];
    auto tied = owners.find(rankKey);
    if (tied != owners.end() && tied->second != endpoint.endpointStableId)
      return endpoint.operation->emitOpError()
             << "duplicate writer priority rank " << rank
             << " for owner @" << endpoint.owner.getSymName();
    owners[rankKey] = endpoint.endpointStableId;
  }

  // One firing may be a member of priority policies for several Tables.  The
  // owner-local orders must compose into one acyclic global prepare order;
  // otherwise no source-order-independent winner selection exists.  Reject
  // the contradiction here, before QueueGraph extraction or mutation.
  SmallVector<Operation *> precedenceNodes;
  llvm::DenseMap<Operation *, unsigned> precedenceNodeIndex;
  for (const WriterEndpoint &endpoint : endpoints) {
    if (!endpoint.request)
      continue;
    auto [iterator, inserted] = precedenceNodeIndex.try_emplace(
        endpoint.scope, precedenceNodes.size());
    if (inserted)
      precedenceNodes.push_back(endpoint.scope);
  }
  SmallVector<SmallVector<bool>> precedenceEdges(
      precedenceNodes.size(), SmallVector<bool>(precedenceNodes.size(), false));
  SmallVector<unsigned> indegree(precedenceNodes.size(), 0);
  for (size_t leftIndex = 0; leftIndex < endpoints.size(); ++leftIndex) {
    const WriterEndpoint &left = endpoints[leftIndex];
    if (!left.request)
      continue;
    for (size_t rightIndex = leftIndex + 1; rightIndex < endpoints.size();
         ++rightIndex) {
      const WriterEndpoint &right = endpoints[rightIndex];
      if (!right.request || left.owner != right.owner ||
          left.scope == right.scope)
        continue;
      const unsigned leftNode = precedenceNodeIndex.lookup(left.scope);
      const unsigned rightNode = precedenceNodeIndex.lookup(right.scope);
      const unsigned before = left.request.getRank() < right.request.getRank()
                                  ? leftNode
                                  : rightNode;
      const unsigned after = before == leftNode ? rightNode : leftNode;
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
  if (visited != precedenceNodes.size())
    return model.emitError(
        "writer arbitration precedence contains a cross-owner cycle");

  for (size_t rightIndex = 1; rightIndex < endpoints.size(); ++rightIndex) {
    for (size_t leftIndex = 0; leftIndex < rightIndex; ++leftIndex) {
      WriterEndpoint left = endpoints[leftIndex];
      WriterEndpoint right = endpoints[rightIndex];
      if (left.owner != right.owner ||
          fieldsAreDisjoint(left.fields, right.fields) ||
          (!left.masked && !right.masked && left.index && right.index &&
           analysis.provesDisjoint(left.index, right.index)) ||
          (left.presence && right.presence &&
           analysis.provesMutuallyExclusive(left.presence, right.presence)))
        continue;
      const bool leftAllocation = isa<ac::TableWriteOp>(left.operation) &&
                                  left.mode == "replace";
      const bool rightAllocation = isa<ac::TableWriteOp>(right.operation) &&
                                   right.mode == "replace";
      if ((leftAllocation && right.mode == "field") ||
          (rightAllocation && left.mode == "field"))
        continue;
      if (left.scope == right.scope)
        return right.operation->emitOpError(
            "one stable writer endpoint has unresolved overlapping proposals");
      if (left.endpointStableId.empty() || right.endpointStableId.empty())
        return right.operation->emitOpError(
            "overlapping writers require stable ac.endpoint_id identity");
      if (!left.request || !right.request)
        return right.operation->emitOpError()
               << "same-field overlap on owner @" << right.owner.getSymName()
               << " requires explicit priority on every writer endpoint";
    }
  }
  return success();
}

#define GEN_PASS_DEF_VERIFYVALUECONSTRAINTSPASS
#include "acir/Transforms/Passes.h.inc"

struct VerifyValueConstraintsPass
    : impl::VerifyValueConstraintsPassBase<VerifyValueConstraintsPass> {
  void runOnOperation() override {
    if (failed(verifyValueConstraints(getOperation())))
      signalPassFailure();
  }
};

} // namespace

LogicalResult verifyValueConstraints(ModuleOp model) {
  ACDataFlowAnalyzer analysis(model.getOperation());
  if (failed(analysis.run()))
    return model.emitError("AC value constraint analysis failed");

  LogicalResult result = success();
  model.walk([&](Operation *operation) {
    if (failed(result))
      return WalkResult::interrupt();
    if (auto read = dyn_cast<ac::VarReadElementOp>(operation)) {
      auto variable = resolveFlatDeclaration<ac::VarDeclOp>(
          read, read.getVariableAttr());
      if (!variable || !variable.getShapeAttr())
        return WalkResult::advance();
      result = verifyIndex(analysis, read, read.getIndex(),
                           variable.getShapeAttr().asArrayRef().front(),
                           "shaped ac.var");
    } else if (auto assign = dyn_cast<ac::VarAssignElementOp>(operation)) {
      auto variable = resolveFlatDeclaration<ac::VarDeclOp>(
          assign, assign.getVariableAttr());
      if (!variable || !variable.getShapeAttr())
        return WalkResult::advance();
      result = verifyIndex(analysis, assign, assign.getIndex(),
                           variable.getShapeAttr().asArrayRef().front(),
                           "shaped ac.var");
    } else if (auto read = dyn_cast<ac::TableGetOp>(operation)) {
      auto table = resolveFlatDeclaration<ac::TableOp>(read,
                                                       read.getTableAttr());
      if (table)
        result = verifyIndex(analysis, read, read.getIndex(),
                             table.getEntries(), "Table");
    } else if (auto read = dyn_cast<ac::TableReadOp>(operation)) {
      auto table = resolveFlatDeclaration<ac::TableOp>(read,
                                                       read.getTableAttr());
      if (table && read.getAddress().hasOneBlock())
        result = verifyIndex(
            analysis, read,
            cast<ac::TableYieldOp>(read.getAddress().front().getTerminator())
                .getValue(),
            table.getEntries(), "Table read address");
    } else if (auto write = dyn_cast<ac::TableWriteOp>(operation)) {
      auto table = resolveFlatDeclaration<ac::TableOp>(write,
                                                       write.getTableAttr());
      if (table && write.getAddress().hasOneBlock())
        result = verifyIndex(
            analysis, write,
            cast<ac::TableYieldOp>(write.getAddress().front().getTerminator())
                .getValue(),
            table.getEntries(), "Table write address");
    } else if (auto proposal = dyn_cast<ac::TableProposeOp>(operation)) {
      auto table = resolveFlatDeclaration<ac::TableOp>(
          proposal, proposal.getTableAttr());
      if (table)
        result = verifyIndex(analysis, proposal, proposal.getIndex(),
                             table.getEntries(), "Table");
    } else if (auto snapshot = dyn_cast<ac::StateSnapshotOp>(operation)) {
      if (!snapshot.getIndex())
        return WalkResult::advance();
      auto table = resolveFlatDeclaration<ac::TableOp>(
          snapshot, snapshot.getTableAttr());
      if (table)
        result = verifyIndex(analysis, snapshot, snapshot.getIndex(),
                             table.getEntries(), "Table snapshot");
    }
    return succeeded(result) ? WalkResult::advance()
                             : WalkResult::interrupt();
  });
  if (failed(result))
    return result;

  return verifyWriterArbitration(model, analysis);
}

std::unique_ptr<Pass> createVerifyValueConstraintsPass() {
  return std::make_unique<VerifyValueConstraintsPass>();
}

} // namespace acir
