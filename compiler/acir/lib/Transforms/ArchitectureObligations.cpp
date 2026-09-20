#include "acir/Transforms/Passes.h"

#include "acir/Analysis/VariableAnalysis.h"
#include "acir/Analysis/WriterArbitrationAnalysis.h"
#include "acir/Dialect/ACIR/ACIROps.h"

#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/StringSet.h"
#include "llvm/Support/FormatVariadic.h"

#include <algorithm>

using namespace mlir;

namespace acir {
namespace {

ac::ModuleOp owningModule(Operation *operation);
FailureOr<SymbolRefAttr> moduleOwnedReference(Operation *endpoint,
                                              FlatSymbolRefAttr local);

WriterConflictProof canonicalWriterProof(WriterConflictProof proof) {
  if (proof.rightEndpoint < proof.leftEndpoint) {
    std::swap(proof.leftOperation, proof.rightOperation);
    std::swap(proof.leftEndpoint, proof.rightEndpoint);
    std::swap(proof.leftRule, proof.rightRule);
  }
  return proof;
}

std::string writerObligationId(const WriterConflictProof &proof) {
  StringRef left = proof.leftRule;
  StringRef right = proof.rightRule;
  if (right < left)
    std::swap(left, right);
  StringRef leftEndpoint = proof.leftEndpoint;
  StringRef rightEndpoint = proof.rightEndpoint;
  if (rightEndpoint < leftEndpoint)
    std::swap(leftEndpoint, rightEndpoint);
  return llvm::formatv("single_writer:{0}:{1}:{2}:{3}:{4}",
                       proof.ownerStableId, left, right, leftEndpoint,
                       rightEndpoint)
      .str();
}

std::string writerExpressionScope(const WriterConflictProof &proof) {
  return proof.leftRule + "::" + writerObligationId(proof);
}

std::string scopedId(ac::ModuleOp module, StringRef id) {
  return (module.getSymName() + "\x1f" + id).str();
}

DictionaryAttr operationNode(Builder &builder, StringRef operation,
                             ArrayRef<int64_t> operands) {
  NamedAttrList attributes;
  attributes.set("operation", builder.getStringAttr(operation));
  attributes.set("result_ordinal", builder.getI64IntegerAttr(0));
  NamedAttrList node;
  node.set("opcode", ac::RuleExpressionOpcodeAttr::get(
                         builder.getContext(),
                         ac::RuleExpressionOpcode::Operation));
  node.set("result_type",
           TypeAttr::get(ac::VarType::get(builder.getContext(),
                                         builder.getI1Type())));
  node.set("operands", builder.getDenseI64ArrayAttr(operands));
  node.set("attributes", builder.getDictionaryAttr(attributes));
  return builder.getDictionaryAttr(node);
}

struct ExpectedWriterCondition {
  ArrayAttr nodes;
  int64_t leftRoot;
  int64_t rightRoot;
  int64_t conditionRoot;
};

FailureOr<ExpectedWriterCondition>
writerCondition(const WriterConflictProof &proof) {
  auto left = dyn_cast<ac::TableProposeOp>(proof.leftOperation);
  auto right = dyn_cast<ac::TableProposeOp>(proof.rightOperation);
  if (!left || !right || proof.leftRule != proof.rightRule || !left.getWhen() ||
      !right.getWhen())
    return failure();
  Operation *scope = left->getParentOfType<ac::RuleOp>();
  if (!scope)
    scope = left->getParentOfType<ac::FiringOp>();
  if (!scope || !scope->isAncestor(right))
    return failure();
  FailureOr<ac::NormalizedRuleExpressions> normalized =
      ac::normalizeRuleExpressions(scope, {left.getWhen(), right.getWhen()});
  if (failed(normalized))
    return failure();
  Builder builder(scope->getContext());
  SmallVector<Attribute> nodes(normalized->expressionDAG.getValue());
  int64_t andRoot = nodes.size();
  nodes.push_back(operationNode(builder, "ac.var.and", normalized->roots));
  int64_t conditionRoot = nodes.size();
  nodes.push_back(operationNode(builder, "ac.var.not", {andRoot}));
  return ExpectedWriterCondition{builder.getArrayAttr(nodes),
                                 normalized->roots[0], normalized->roots[1],
                                 conditionRoot};
}

DictionaryAttr expressionReference(Builder &builder, StringRef scope,
                                   int64_t node) {
  NamedAttrList reference;
  reference.set("table", builder.getStringAttr("ac.arch_expression_table"));
  reference.set("rule", builder.getStringAttr(scope));
  reference.set("node", builder.getI64IntegerAttr(node));
  return builder.getDictionaryAttr(reference);
}

ArrayAttr sourceProvenance(Builder &builder, Operation *operation) {
  if (auto source = operation->getAttrOfType<ArrayAttr>("ac.source_provenance"))
    return source;
  NamedAttrList frame;
  frame.set("file", builder.getStringAttr("compiler/inferred.py"));
  frame.set("line", builder.getI64IntegerAttr(1));
  frame.set("column", builder.getI64IntegerAttr(1));
  frame.set("kind", builder.getStringAttr("statement"));
  NamedAttrList origin;
  origin.set("frames",
             builder.getArrayAttr({builder.getDictionaryAttr(frame)}));
  return builder.getArrayAttr({builder.getDictionaryAttr(origin)});
}

DictionaryAttr proofCertificate(Builder &builder,
                                const WriterConflictProof &proof,
                                const ExpectedWriterCondition &condition) {
  NamedAttrList certificate;
  certificate.set("kind", builder.getStringAttr("predicate_exclusive"));
  certificate.set("left_endpoint", builder.getStringAttr(proof.leftEndpoint));
  certificate.set("right_endpoint", builder.getStringAttr(proof.rightEndpoint));
  certificate.set("left_rule", builder.getStringAttr(proof.leftRule));
  certificate.set("right_rule", builder.getStringAttr(proof.rightRule));
  certificate.set("owner_path", builder.getStringAttr(proof.ownerPath));
  certificate.set("owner_stable_id",
                  builder.getStringAttr(proof.ownerStableId));
  certificate.set("assumption", builder.getStringAttr("exact_path_predicates"));
  certificate.set("left_condition_root",
                  builder.getI64IntegerAttr(condition.leftRoot));
  certificate.set("right_condition_root",
                  builder.getI64IntegerAttr(condition.rightRoot));
  certificate.set("property_root",
                  builder.getI64IntegerAttr(condition.conditionRoot));
  return builder.getDictionaryAttr(certificate);
}

ArrayAttr expectedWriterOwners(Builder &builder,
                               const WriterConflictProof &proof) {
  auto proposal = cast<ac::TableProposeOp>(proof.leftOperation);
  FailureOr<SymbolRefAttr> resource =
      moduleOwnedReference(proposal, proposal.getTableAttr());
  assert(succeeded(resource) && "writer proof must resolve its typed owner");
  NamedAttrList owner;
  owner.set("resource", *resource);
  owner.set("owner_path", builder.getStringAttr(proof.ownerPath));
  owner.set("owner_stable_id", builder.getStringAttr(proof.ownerStableId));
  return builder.getArrayAttr({builder.getDictionaryAttr(owner)});
}

DictionaryAttr expectedWriterSampling(Builder &builder) {
  NamedAttrList sampling;
  sampling.set("kind", ac::ArchitectureSamplingKindAttr::get(
                           builder.getContext(),
                           ac::ArchitectureSamplingKind::TickObservation));
  sampling.set("edge", ac::ArchitectureSamplingEdgeAttr::get(
                           builder.getContext(),
                           ac::ArchitectureSamplingEdge::None));
  sampling.set("monitor_only", builder.getBoolAttr(true));
  return builder.getDictionaryAttr(sampling);
}

LogicalResult writerProofs(ModuleOp model,
                           llvm::StringMap<WriterConflictProof> &proofs) {
  ACDataFlowAnalyzer analysis(model.getOperation());
  if (failed(analysis.run()))
    return model.emitError(
        "AC dataflow analysis failed for architecture obligations");
  WriterArbitrationAnalysis arbitration;
  if (failed(analyzeWriterArbitration(model, analysis, &arbitration)))
    return failure();
  for (const WriterConflictProof &rawProof : arbitration.conflicts) {
    WriterConflictProof proof = canonicalWriterProof(rawProof);
    if (!proof.leftRule.empty() && !proof.rightRule.empty() &&
        proof.kind == WriterConflictProofKind::PredicateExclusive &&
        proof.leftRule == proof.rightRule && owningModule(proof.leftOperation) &&
        owningModule(proof.leftOperation) == owningModule(proof.rightOperation)) {
      ac::ModuleOp owner = owningModule(proof.leftOperation);
      const std::string identity = scopedId(owner, writerObligationId(proof));
      if (!proofs.try_emplace(identity, proof).second)
        return proof.leftOperation->emitError(
            "duplicate predicate-exclusive architecture proof identity");
    }
  }
  return success();
}

ac::ModuleOp owningModule(Operation *operation) {
  return operation ? operation->getParentOfType<ac::ModuleOp>()
                   : ac::ModuleOp();
}

FailureOr<SymbolRefAttr> moduleOwnedReference(Operation *endpoint,
                                              FlatSymbolRefAttr local) {
  Operation *target = ac::lookupRuntimeSymbol(endpoint, local);
  ac::ModuleOp module = owningModule(endpoint);
  if (!target || !module)
    return failure();
  SmallVector<StringAttr> path;
  for (Operation *current = target; current && current != module.getOperation();
       current = current->getParentOp()) {
    auto symbol =
        current->getAttrOfType<StringAttr>(SymbolTable::getSymbolAttrName());
    if (symbol)
      path.push_back(symbol);
  }
  if (path.empty())
    return failure();
  std::reverse(path.begin(), path.end());
  SmallVector<FlatSymbolRefAttr> nested;
  for (StringAttr component : ArrayRef(path).drop_front())
    nested.push_back(FlatSymbolRefAttr::get(component));
  return SymbolRefAttr::get(path.front(), nested);
}

LogicalResult inferArchitectureObligations(ModuleOp model) {
  llvm::StringMap<WriterConflictProof> expected;
  if (failed(writerProofs(model, expected)))
    return failure();
  llvm::StringSet<> existing;
  model.walk([&](ac::ArchitectureObligationOp obligation) {
    existing.insert(scopedId(obligation->getParentOfType<ac::ModuleOp>(),
                             obligation.getId()));
  });
  for (const auto &entry : expected) {
    if (existing.contains(entry.getKey()))
      continue;
    const WriterConflictProof &proof = entry.getValue();
    const std::string id = writerObligationId(proof);
    ac::ModuleOp owner = owningModule(proof.leftOperation);
    if (!owner || owningModule(proof.rightOperation) != owner)
      return model.emitError(
          "predicate-exclusive proof crosses architecture module ownership");
    Builder builder(model.getContext());
    ArrayAttr table =
        owner->getAttrOfType<ArrayAttr>("ac.arch_expression_table");
    SmallVector<Attribute> scopes(table ? table.getValue()
                                        : ArrayRef<Attribute>{});
    FailureOr<ExpectedWriterCondition> condition = writerCondition(proof);
    if (failed(condition))
      return proof.leftOperation->emitError(
          "cannot normalize predicate-exclusive obligation condition");
    NamedAttrList scope;
    const std::string expressionScope = writerExpressionScope(proof);
    scope.set("rule", builder.getStringAttr(expressionScope));
    scope.set("owner_rule", builder.getStringAttr(proof.leftRule));
    scope.set("nodes", condition->nodes);
    for (Attribute raw : scopes) {
      auto prior = cast<DictionaryAttr>(raw);
      if (prior.getAs<StringAttr>("rule").getValue() == expressionScope) {
        if (prior.getAs<ArrayAttr>("nodes") != condition->nodes)
          return owner.emitOpError(
              "architecture-expression rule scope is duplicated with "
              "different nodes");
        scope = {};
        break;
      }
    }
    if (!scope.empty())
      scopes.push_back(builder.getDictionaryAttr(scope));
    llvm::sort(scopes, [](Attribute left, Attribute right) {
      return cast<DictionaryAttr>(left).getAs<StringAttr>("rule").getValue() <
             cast<DictionaryAttr>(right).getAs<StringAttr>("rule").getValue();
    });
    owner->setAttr("ac.arch_expression_table", builder.getArrayAttr(scopes));

    auto proposal = dyn_cast<ac::TableProposeOp>(proof.leftOperation);
    if (!proposal)
      return proof.leftOperation->emitError(
          "predicate-exclusive obligation requires a typed Table endpoint");
    auto ownerCase = proof.leftOperation->getParentOfType<ac::ModuleCaseOp>();
    if (!ownerCase)
      return proof.leftOperation->emitError(
          "architecture obligation has no owning module case");
    OpBuilder insertion(ownerCase.getBody().front().getTerminator());
    OperationState state(proof.leftOperation->getLoc(),
                         ac::ArchitectureObligationOp::getOperationName());
    state.addAttribute(SymbolTable::getSymbolAttrName(),
                       builder.getStringAttr(id));
    state.addAttribute("id", builder.getStringAttr(id));
    state.addAttribute(
        "kind",
        ac::ArchitectureObligationKindAttr::get(
            model.getContext(), ac::ArchitectureObligationKind::SingleWriter));
    state.addAttribute(
        "severity",
        ac::ArchitectureObligationSeverityAttr::get(
            model.getContext(), ac::ArchitectureObligationSeverity::Error));
    state.addAttribute(
        "status",
        ac::ArchitectureObligationStatusAttr::get(
            model.getContext(), ac::ArchitectureObligationStatus::Pending));
    state.addAttribute("condition", expressionReference(
                                        builder, expressionScope,
                                        condition->conditionRoot));
    SmallVector<StringRef> rules{proof.leftRule, proof.rightRule};
    llvm::sort(rules);
    rules.erase(std::unique(rules.begin(), rules.end()), rules.end());
    state.addAttribute("source_rules", builder.getStrArrayAttr(rules));
    state.addAttribute("state_owners", expectedWriterOwners(builder, proof));
    state.addAttribute("runtime_targets", builder.getArrayAttr({}));
    state.addAttribute("materializations", builder.getArrayAttr({}));
    state.addAttribute("sampling", expectedWriterSampling(builder));
    state.addAttribute("message",
                       builder.getStringAttr("same-field writers must remain "
                                             "predicate-exclusive"));
    state.addAttribute("source_provenance",
                       sourceProvenance(builder, proof.leftOperation));
    state.addAttribute("ndf_ids", builder.getStrArrayAttr({}));
    insertion.create(state);
  }
  return success();
}

LogicalResult proveArchitectureObligations(ModuleOp model) {
  llvm::StringMap<WriterConflictProof> expected;
  if (failed(writerProofs(model, expected)))
    return failure();
  Builder builder(model.getContext());
  model.walk([&](ac::ArchitectureObligationOp obligation) {
    if (obligation.getStatus() != ac::ArchitectureObligationStatus::Pending)
      return;
    auto proof = expected.find(scopedId(
        obligation->getParentOfType<ac::ModuleOp>(), obligation.getId()));
    if (proof == expected.end()) {
      return;
    }
    FailureOr<ExpectedWriterCondition> condition =
        writerCondition(proof->getValue());
    if (failed(condition))
      return;
    obligation->setAttr("proof_certificate",
                        proofCertificate(builder, proof->getValue(),
                                         *condition));
    obligation->setAttr(
        "status",
        ac::ArchitectureObligationStatusAttr::get(
            model.getContext(), ac::ArchitectureObligationStatus::Proved));
  });
  return success();
}

struct RangeProgram {
  int64_t inputOrdinal;
  int64_t maximum;
};

DictionaryAttr expectedRangeMaterialization(ac::ArchitectureObligationOp op,
                                            const RangeProgram &program) {
  Builder builder(op.getContext());
  auto module = op->getParentOfType<ac::ModuleOp>();
  NamedAttrList expected;
  expected.set("module", FlatSymbolRefAttr::get(module.getSymNameAttr()));
  expected.set("target", ac::ArchitectureRuntimeTargetAttr::get(
                             op.getContext(),
                             ac::ArchitectureRuntimeTarget::Gfsim));
  expected.set("firing",
               op.getSampling().getAs<StringAttr>("sample_anchor"));
  expected.set("input_ordinal",
               builder.getI64IntegerAttr(program.inputOrdinal));
  expected.set("maximum", builder.getI64IntegerAttr(program.maximum));
  expected.set("sampling", op.getSampling());
  expected.set("condition", op.getCondition());
  expected.set("severity", op.getSeverityAttr());
  expected.set("active_predicate",
               op.getSampling().get("active_predicate")
                   ? op.getSampling().get("active_predicate")
                   : builder.getUnitAttr());
  expected.set("reset_recovery_disable",
               op.getSampling().get("reset_recovery_disable")
                   ? op.getSampling().get("reset_recovery_disable")
                   : builder.getUnitAttr());
  return builder.getDictionaryAttr(expected);
}

FailureOr<RangeProgram> decodeRangeProgram(ac::ArchitectureObligationOp op) {
  auto rule = op.getCondition().getAs<StringAttr>("rule");
  auto root = op.getCondition().getAs<IntegerAttr>("node");
  auto owner = op->getParentOfType<ac::ModuleOp>();
  auto table = owner->getAttrOfType<ArrayAttr>("ac.arch_expression_table");
  ArrayAttr nodes;
  for (Attribute raw : table) {
    auto scope = cast<DictionaryAttr>(raw);
    if (scope.getAs<StringAttr>("rule") == rule) {
      nodes = scope.getAs<ArrayAttr>("nodes");
      break;
    }
  }
  if (!nodes || root.getInt() < 0 ||
      root.getInt() >= static_cast<int64_t>(nodes.size()))
    return failure();
  auto comparison = cast<DictionaryAttr>(nodes[root.getInt()]);
  auto comparisonOpcode =
      comparison.getAs<ac::RuleExpressionOpcodeAttr>("opcode");
  auto comparisonAttrs = comparison.getAs<DictionaryAttr>("attributes");
  auto operation = comparisonAttrs
                       ? comparisonAttrs.getAs<StringAttr>("operation")
                       : StringAttr();
  auto predicate = comparisonAttrs
                       ? comparisonAttrs.getAs<StringAttr>("predicate")
                       : StringAttr();
  auto operands = comparison.getAs<DenseI64ArrayAttr>("operands");
  if (!comparisonOpcode ||
      comparisonOpcode.getValue() != ac::RuleExpressionOpcode::Operation ||
      !operation || operation.getValue() != "ac.var.cmp" || !predicate ||
      predicate.getValue() != "ule" || !operands || operands.size() != 2)
    return failure();
  if (operands[0] < 0 || operands[1] < 0 ||
      operands[0] >= static_cast<int64_t>(nodes.size()) ||
      operands[1] >= static_cast<int64_t>(nodes.size()))
    return failure();
  auto input = dyn_cast<DictionaryAttr>(nodes[operands[0]]);
  auto constant = dyn_cast<DictionaryAttr>(nodes[operands[1]]);
  if (!input || !constant)
    return failure();
  auto inputOpcode = input.getAs<ac::RuleExpressionOpcodeAttr>("opcode");
  auto constantOpcode = constant.getAs<ac::RuleExpressionOpcodeAttr>("opcode");
  auto inputAttrs = input.getAs<DictionaryAttr>("attributes");
  auto constantAttrs = constant.getAs<DictionaryAttr>("attributes");
  auto inputOrdinal =
      inputAttrs ? inputAttrs.getAs<IntegerAttr>("ordinal") : IntegerAttr();
  auto maximum =
      constantAttrs ? constantAttrs.getAs<IntegerAttr>("value") : IntegerAttr();
  if (!inputOpcode ||
      inputOpcode.getValue() != ac::RuleExpressionOpcode::RuleInput ||
      !constantOpcode ||
      constantOpcode.getValue() != ac::RuleExpressionOpcode::Constant ||
      !inputOrdinal || inputOrdinal.getInt() < 0 || !maximum ||
      maximum.getInt() < 0)
    return failure();
  return RangeProgram{inputOrdinal.getInt(), maximum.getInt()};
}

LogicalResult materializeArchitectureObligations(ModuleOp model) {
  Builder builder(model.getContext());
  LogicalResult result = success();
  model.walk([&](ac::ArchitectureObligationOp obligation) {
    if (failed(result) ||
        obligation.getStatus() != ac::ArchitectureObligationStatus::Pending)
      return;
    if (obligation.getKind() != ac::ArchitectureObligationKind::Range ||
        obligation.getRuntimeTargets().size() != 1 ||
        !isa<ac::ArchitectureRuntimeTargetAttr>(
            obligation.getRuntimeTargets()[0]) ||
        cast<ac::ArchitectureRuntimeTargetAttr>(
            obligation.getRuntimeTargets()[0])
                .getValue() != ac::ArchitectureRuntimeTarget::Gfsim) {
      obligation->setAttr(
          "status",
          ac::ArchitectureObligationStatusAttr::get(
              model.getContext(), ac::ArchitectureObligationStatus::Rejected));
      return;
    }
    FailureOr<RangeProgram> program = decodeRangeProgram(obligation);
    auto anchor = obligation.getSampling().getAs<StringAttr>("sample_anchor");
    if (failed(program) || !anchor) {
      obligation->setAttr(
          "status",
          ac::ArchitectureObligationStatusAttr::get(
              model.getContext(), ac::ArchitectureObligationStatus::Rejected));
      return;
    }
    obligation->setAttr(
        "materializations",
        builder.getArrayAttr(
            {expectedRangeMaterialization(obligation, *program)}));
    obligation->setAttr("status",
                        ac::ArchitectureObligationStatusAttr::get(
                            model.getContext(),
                            ac::ArchitectureObligationStatus::RuntimeChecked));
  });
  return result;
}

#define GEN_PASS_DEF_INFERARCHITECTUREOBLIGATIONSPASS
#define GEN_PASS_DEF_PROVEARCHITECTUREOBLIGATIONSPASS
#define GEN_PASS_DEF_MATERIALIZEARCHITECTUREOBLIGATIONSPASS
#include "acir/Transforms/Passes.h.inc"

template <typename Base, LogicalResult (*Implementation)(ModuleOp)>
struct ObligationPass : Base {
  void runOnOperation() override {
    if (failed(Implementation(this->getOperation())))
      this->signalPassFailure();
  }
};

struct InferArchitectureObligationsPass
    : ObligationPass<impl::InferArchitectureObligationsPassBase<
                         InferArchitectureObligationsPass>,
                     inferArchitectureObligations> {};
struct ProveArchitectureObligationsPass
    : ObligationPass<impl::ProveArchitectureObligationsPassBase<
                         ProveArchitectureObligationsPass>,
                     proveArchitectureObligations> {};
struct MaterializeArchitectureObligationsPass
    : ObligationPass<impl::MaterializeArchitectureObligationsPassBase<
                         MaterializeArchitectureObligationsPass>,
                     materializeArchitectureObligations> {};

} // namespace

LogicalResult verifyArchitectureObligations(ModuleOp model,
                                            bool requireClosed) {
  llvm::StringMap<WriterConflictProof> expected;
  if (failed(writerProofs(model, expected)))
    return failure();
  llvm::StringSet<> observed;
  Builder builder(model.getContext());
  LogicalResult result = success();
  model.walk([&](ac::ArchitectureObligationOp obligation) {
    if (failed(result))
      return;
    if (failed(obligation.verify())) {
      result = failure();
      return;
    }
    ac::ModuleOp owner = obligation->getParentOfType<ac::ModuleOp>();
    std::string identity = scopedId(owner, obligation.getId());
    if (!observed.insert(identity).second) {
      result = obligation.emitOpError("duplicates a live obligation ID");
      return;
    }
    if (requireClosed &&
        (obligation.getStatus() == ac::ArchitectureObligationStatus::Pending ||
         obligation.getStatus() ==
             ac::ArchitectureObligationStatus::Rejected)) {
      result = obligation.emitOpError(
          "pending or rejected architecture obligation blocks backend emit");
      return;
    }
    if (obligation.getStatus() == ac::ArchitectureObligationStatus::Proved &&
        !expected.contains(identity)) {
      result = obligation.emitOpError(
          "proved obligation is not in the recomputed inferred set");
      return;
    }
    llvm::StringSet<> liveRules;
    owner.walk([&](Operation *operation) {
      if (auto rule = dyn_cast<ac::RuleOp>(operation))
        liveRules.insert(rule.getStableId());
      else if (auto firing = dyn_cast<ac::FiringOp>(operation))
        liveRules.insert(firing.getStableId());
      else if (auto transform = dyn_cast<ac::TransformOp>(operation))
        if (auto id = transform->getAttrOfType<StringAttr>("ac.rule_stable_id"))
          liveRules.insert(id.getValue());
    });
    for (Attribute raw : obligation.getSourceRules())
      if (!liveRules.contains(cast<StringAttr>(raw).getValue())) {
        result = obligation.emitOpError(
            "source-rule reference does not resolve in the owning module");
        return;
      }
    if (obligation.getStatus() == ac::ArchitectureObligationStatus::Proved) {
      auto proof = expected.find(identity);
      assert(proof != expected.end());
      FailureOr<ExpectedWriterCondition> condition =
          writerCondition(proof->getValue());
      ArrayAttr expressionNodes;
      auto table = owner->getAttrOfType<ArrayAttr>("ac.arch_expression_table");
      if (table)
        for (Attribute rawScope : table) {
          auto scope = cast<DictionaryAttr>(rawScope);
          if (scope.getAs<StringAttr>("rule").getValue() ==
              writerExpressionScope(proof->getValue()))
            expressionNodes = scope.getAs<ArrayAttr>("nodes");
        }
      const auto expectedRules =
          builder.getStrArrayAttr({proof->getValue().leftRule});
      if (failed(condition) || expressionNodes != condition->nodes ||
          obligation.getKind() != ac::ArchitectureObligationKind::SingleWriter ||
          obligation.getSeverity() !=
              ac::ArchitectureObligationSeverity::Error ||
          obligation.getCondition() !=
              expressionReference(builder,
                                  writerExpressionScope(proof->getValue()),
                                  condition->conditionRoot) ||
          obligation.getSourceRules() != expectedRules ||
          obligation.getStateOwners() !=
              expectedWriterOwners(builder, proof->getValue()) ||
          obligation.getSampling() != expectedWriterSampling(builder) ||
          !obligation.getNdfIds().empty() ||
          obligation.getMessage() !=
              "same-field writers must remain predicate-exclusive" ||
          obligation.getProofCertificate() !=
              proofCertificate(builder, proof->getValue(), *condition))
        result = obligation.emitOpError(
            "proved obligation has a removed, dangling, or forged closed "
            "certificate");
      return;
    }
    if (obligation.getStatus() ==
        ac::ArchitectureObligationStatus::RuntimeChecked) {
      FailureOr<RangeProgram> program = decodeRangeProgram(obligation);
      if (failed(program) || obligation.getMaterializations().size() != 1 ||
          obligation.getMaterializations()[0] !=
              expectedRangeMaterialization(obligation, *program))
        result = obligation.emitOpError(
            "runtime materialization does not exactly match condition, "
            "module, firing, input, maximum, target, sampling, predicates, "
            "or severity");
      return;
    }
  });
  if (failed(result))
    return failure();
  for (const auto &entry : expected)
    if (!observed.contains(entry.getKey()))
      return model.emitError() << "expected inferred architecture obligation '"
                               << entry.getKey() << "' was removed";
  bool transient = false;
  model.walk([&](ac::PendingObligationMarkerOp) { transient = true; });
  if (requireClosed && transient)
    return model.emitError(
        "transient rule obligation marker reached architecture closure");
  return success();
}

std::unique_ptr<Pass> createInferArchitectureObligationsPass() {
  return std::make_unique<InferArchitectureObligationsPass>();
}
std::unique_ptr<Pass> createProveArchitectureObligationsPass() {
  return std::make_unique<ProveArchitectureObligationsPass>();
}
std::unique_ptr<Pass> createMaterializeArchitectureObligationsPass() {
  return std::make_unique<MaterializeArchitectureObligationsPass>();
}

} // namespace acir
