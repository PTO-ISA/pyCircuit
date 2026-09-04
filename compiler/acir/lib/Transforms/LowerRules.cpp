#include "acir/Transforms/Passes.h"

#include "acir/Dialect/ACIR/ACIROps.h"
#include "mlir/IR/Builders.h"
#include "mlir/Pass/PassManager.h"
#include "mlir/Pass/PassRegistry.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringSet.h"

using namespace mlir;

namespace acir {
namespace {

template <typename Marker> SmallVector<Marker> collectMarkers(ModuleOp model) {
  SmallVector<Marker> markers;
  model.walk([&](Marker marker) { markers.push_back(marker); });
  return markers;
}

template <typename Marker> void dischargeMarker(Marker marker) {
  marker.getResult().replaceAllUsesWith(marker.getInput());
  marker.erase();
}

LogicalResult requireNoTypeMarkers(ModuleOp model, StringRef stage) {
  LogicalResult result = success();
  model.walk([&](ac::TypeConstraintMarkerOp marker) {
    if (failed(result))
      return;
    result = marker.emitOpError() << "must be resolved before " << stage;
  });
  return result;
}

LogicalResult requireRuleAttribute(ac::RuleOp rule, StringRef name,
                                   StringRef stage) {
  if (rule->hasAttr(name))
    return success();
  return rule.emitOpError() << "requires '" << name << "' before " << stage;
}

LogicalResult inferRuleTypes(ModuleOp model) {
  for (ac::TypeConstraintMarkerOp marker :
       collectMarkers<ac::TypeConstraintMarkerOp>(model)) {
    if (marker.getConstraint() != ac::TypeConstraintKind::QueuePayload)
      return marker.emitOpError("unsupported type-constraint kind");
    ac::RuleOp rule = marker->getParentOfType<ac::RuleOp>();
    if (!rule || marker.getInput() != rule.getBody().front().getArgument(0))
      return marker.emitOpError(
          "phase-one Queue payload inference must refine the unique rule input");
    // The ACIR Var wrapper already carries the exact element type.  This first
    // inference lane therefore refines unknown/constrained Queue payload facts
    // monotonically to exact before removing the marker.
    dischargeMarker(marker);
  }
  return success();
}

LogicalResult inferRuleEffects(ModuleOp model) {
  if (failed(requireNoTypeMarkers(model, "rule effect inference")))
    return failure();
  for (ac::ValueFactMarkerOp marker :
       collectMarkers<ac::ValueFactMarkerOp>(model)) {
    if (marker.getFact() != ac::ValueFactKind::CommittedInput)
      return marker.emitOpError("unsupported value-fact kind");
    ac::RuleOp rule = marker->getParentOfType<ac::RuleOp>();
    if (!rule || marker.getInput() != rule.getBody().front().getArgument(0) ||
        marker.getIdentity() != "input" || marker.getPathPredicate() != "true")
      return marker.emitOpError(
          "phase-one value inference requires the unique committed input on "
          "path true with identity 'input'");
    dischargeMarker(marker);
  }
  Builder builder(model.getContext());
  model.walk([&](ac::RuleOp rule) {
    rule->setAttr("ac.rule.effects",
                  builder.getStrArrayAttr({"input.consume", "output.produce"}));
  });
  return success();
}

LogicalResult materializeRuleChecks(ModuleOp model) {
  Builder builder(model.getContext());
  LogicalResult result = success();
  model.walk([&](ac::RuleOp rule) {
    if (failed(result))
      return;
    if (failed(requireRuleAttribute(rule, "ac.rule.effects",
                                    "check materialization"))) {
      result = failure();
      return;
    }
    SmallVector<Attribute> checks;
    for (ac::PendingObligationMarkerOp marker :
         collectMarkers<ac::PendingObligationMarkerOp>(model)) {
      if (marker->getParentOfType<ac::RuleOp>() != rule ||
          marker.getResolver() != ac::ObligationResolver::Checks)
        continue;
      if (marker.getState() != ac::ObligationState::Pending) {
        marker.emitOpError("check resolver requires a pending obligation");
        result = failure();
        return;
      }
      marker.emitOpError(
          "dynamic checks are not executable in the phase-one pure rule subset");
      result = failure();
      return;
    }
    rule->setAttr("ac.rule.checks", builder.getArrayAttr(checks));
  });
  return result;
}

LogicalResult materializeRuleHandshake(ModuleOp model) {
  LogicalResult result = success();
  model.walk([&](ac::RuleOp rule) {
    if (failed(result))
      return;
    if (failed(requireRuleAttribute(rule, "ac.rule.checks",
                                    "handshake materialization"))) {
      result = failure();
      return;
    }
    bool found = false;
    for (ac::PendingObligationMarkerOp marker :
         collectMarkers<ac::PendingObligationMarkerOp>(model)) {
      if (marker->getParentOfType<ac::RuleOp>() != rule ||
          marker.getResolver() != ac::ObligationResolver::Handshake)
        continue;
      if (found || marker.getState() != ac::ObligationState::Pending ||
          marker.getPathPredicate() != "true") {
        marker.emitOpError("phase-one handshake requires one pending "
                           "unconditional obligation");
        result = failure();
        return;
      }
      if (!marker.getResult().hasOneUse() ||
          !isa<ac::RuleReturnOp>(marker.getResult().use_begin()->getOwner())) {
        marker.emitOpError("handshake obligation must wrap the value returned "
                           "by ac.rule.return");
        result = failure();
        return;
      }
      found = true;
      marker.setStateAttr(ac::ObligationStateAttr::get(
          model.getContext(), ac::ObligationState::Materialized));
    }
    if (!found) {
      rule.emitOpError("requires one handshake obligation");
      result = failure();
      return;
    }
    rule->setAttr("ac.rule.handshake",
                  StringAttr::get(model.getContext(), "ready_valid_1x1"));
  });
  return result;
}

LogicalResult dischargeRuleObligations(ModuleOp model) {
  LogicalResult result = success();
  for (ac::PendingObligationMarkerOp marker :
       collectMarkers<ac::PendingObligationMarkerOp>(model)) {
    if (marker.getState() != ac::ObligationState::Materialized) {
      marker.emitOpError(
          "must be materialized by its named resolver before discharge");
      result = failure();
      continue;
    }
    ac::RuleOp rule = marker->getParentOfType<ac::RuleOp>();
    if (!rule) {
      marker.emitOpError("has no owning rule");
      result = failure();
      continue;
    }
    if (marker.getResolver() == ac::ObligationResolver::Handshake) {
      auto handshake = rule->getAttrOfType<StringAttr>("ac.rule.handshake");
      if (!handshake || handshake.getValue() != "ready_valid_1x1" ||
          !marker.getResult().hasOneUse() ||
          !isa<ac::RuleReturnOp>(marker.getResult().use_begin()->getOwner())) {
        marker.emitOpError(
            "handshake discharge requires returned-value handshake evidence");
        result = failure();
        continue;
      }
    } else if (marker.getResolver() == ac::ObligationResolver::Checks) {
      auto checks = rule->getAttrOfType<ArrayAttr>("ac.rule.checks");
      bool found = checks && llvm::any_of(checks, [&](Attribute attribute) {
                     auto record = dyn_cast<DictionaryAttr>(attribute);
                     return record &&
                            record.getAs<StringAttr>("origin") ==
                                marker.getOriginAttr() &&
                            record.getAs<StringAttr>("path") ==
                                marker.getPathPredicateAttr();
                   });
      if (!found) {
        marker.emitOpError(
            "check discharge requires matching materialized check evidence");
        result = failure();
        continue;
      }
    } else {
      marker.emitOpError("has no implemented discharge verifier");
      result = failure();
      continue;
    }
    marker.setStateAttr(ac::ObligationStateAttr::get(
        model.getContext(), ac::ObligationState::Discharged));
    dischargeMarker(marker);
  }
  return result;
}

LogicalResult resolveRuleSchedule(ModuleOp model) {
  LogicalResult result = success();
  llvm::StringSet<> stableIds;
  model.walk([&](ac::RuleOp rule) {
    if (failed(result))
      return;
    if (failed(requireRuleAttribute(rule, "ac.rule.handshake",
                                    "schedule resolution"))) {
      result = failure();
      return;
    }
    if (!stableIds.insert(rule.getStableId()).second) {
      rule.emitOpError() << "duplicate stable rule identity '"
                         << rule.getStableId() << "'";
      result = failure();
      return;
    }
    bool unresolved = false;
    rule.getBody().walk([&](ac::PendingObligationMarkerOp marker) {
      marker.emitOpError("has no implemented named resolver");
      unresolved = true;
    });
    if (unresolved) {
      result = failure();
      return;
    }
    if (rule.getInputs().size() != 1 || rule.getOutputs().size() != 1) {
      rule.emitOpError(
          "phase-one scheduling requires one input and one output Queue");
      result = failure();
      return;
    }
    size_t consumingUses = 0;
    for (OpOperand &use : rule.getInputs().front().getUses())
      if (!isa<ac::ObserveOp, ac::ExpectOp>(use.getOwner()))
        ++consumingUses;
    if (consumingUses != 1) {
      rule.emitOpError(
          "phase-one independent scheduling requires an exclusive input Queue");
      result = failure();
      return;
    }
    rule->setAttr("ac.rule.guard", StringAttr::get(model.getContext(), "true"));
    rule->setAttr("ac.rule.schedule",
                  StringAttr::get(model.getContext(), "independent"));
  });
  return result;
}

LogicalResult lowerRulesToFiring(ModuleOp model) {
  SmallVector<ac::RuleOp> rules;
  model.walk([&](ac::RuleOp rule) { rules.push_back(rule); });
  for (ac::RuleOp rule : rules) {
    for (StringRef attribute :
         {"ac.rule.effects", "ac.rule.checks", "ac.rule.handshake",
          "ac.rule.guard", "ac.rule.schedule"})
      if (failed(requireRuleAttribute(rule, attribute, "rule lowering")))
        return failure();
    bool hasMarker = false;
    rule.getBody().walk([&](Operation *operation) {
      hasMarker |= isa<ac::TypeConstraintMarkerOp, ac::ValueFactMarkerOp,
                       ac::PendingObligationMarkerOp>(operation);
    });
    if (hasMarker)
      return rule.emitOpError("cannot lower while typed markers remain");

    auto returned =
        dyn_cast<ac::RuleReturnOp>(rule.getBody().front().getTerminator());
    if (!returned)
      return rule.emitOpError("requires ac.rule.return before rule lowering");
    OpBuilder bodyBuilder(returned);
    OperationState yieldState(returned.getLoc(),
                              ac::FiringYieldOp::getOperationName());
    yieldState.addOperands(returned.getValues());
    bodyBuilder.create(yieldState);
    returned.erase();

    OpBuilder builder(rule);
    OperationState state(rule.getLoc(), ac::FiringOp::getOperationName());
    state.addOperands(rule.getInputs());
    state.addTypes(rule.getResultTypes());
    state.addAttribute("output_depths", rule.getOutputDepthsAttr());
    state.addAttribute("output_latencies", rule.getOutputLatenciesAttr());
    state.addAttribute("stable_id", rule.getStableIdAttr());
    state.addAttribute("time_domain", rule.getTimeDomainAttr());
    state.addAttribute("functional_guard", rule->getAttr("ac.rule.guard"));
    state.addAttribute("checks", rule->getAttr("ac.rule.checks"));
    state.addAttribute("handshake", rule->getAttr("ac.rule.handshake"));
    state.addAttribute("schedule", rule->getAttr("ac.rule.schedule"));
    state.addAttribute("effects", rule->getAttr("ac.rule.effects"));
    state.addAttribute("ac.rule_definition", rule.getNameAttr());
    if (Attribute name = rule->getAttr("ac.name"))
      state.addAttribute("ac.name", name);
    state.addRegion();
    auto firing = cast<ac::FiringOp>(builder.create(state));
    firing.getBody().takeBody(rule.getBody());
    rule.replaceAllUsesWith(firing.getResults());
    rule.erase();
  }
  return success();
}

LogicalResult canonicalizePureFirings(ModuleOp model) {
  SmallVector<ac::FiringOp> firings;
  model.walk([&](ac::FiringOp firing) { firings.push_back(firing); });
  Builder attrBuilder(model.getContext());
  for (ac::FiringOp firing : firings) {
    if (firing.getInputs().size() != 1 || firing.getOutputs().size() != 1 ||
        firing.getFunctionalGuard() != "true" || !firing.getChecks().empty() ||
        firing.getHandshake() != "ready_valid_1x1" ||
        firing.getSchedule() != "independent" ||
        firing.getTimeDomain() != "cycle" ||
        firing.getEffects() !=
            attrBuilder.getStrArrayAttr({"input.consume", "output.produce"}))
      return firing.emitOpError(
          "is not proven equivalent to the phase-one pure transform subset");

    auto yielded =
        dyn_cast<ac::FiringYieldOp>(firing.getBody().front().getTerminator());
    if (!yielded)
      return firing.emitOpError(
          "requires ac.firing.yield before pure-firing canonicalization");
    OpBuilder bodyBuilder(yielded);
    OperationState yieldState(yielded.getLoc(),
                              ac::TransformYieldOp::getOperationName());
    yieldState.addOperands(yielded.getValues());
    bodyBuilder.create(yieldState);
    yielded.erase();

    OpBuilder builder(firing);
    OperationState state(firing.getLoc(), ac::TransformOp::getOperationName());
    state.addOperands(firing.getInputs());
    state.addTypes(firing.getResultTypes());
    state.addAttribute("output_depths", firing.getOutputDepthsAttr());
    state.addAttribute("output_latencies", firing.getOutputLatenciesAttr());
    for (StringRef name : {"ac.name", "ac.rule_definition"})
      if (Attribute attribute = firing->getAttr(name))
        state.addAttribute(name, attribute);
    state.addAttribute("ac.rule_stable_id", firing.getStableIdAttr());
    state.addAttribute("ac.rule_time_domain", firing.getTimeDomainAttr());
    state.addAttribute("ac.rule_guard", firing.getFunctionalGuardAttr());
    state.addAttribute("ac.rule_checks", firing.getChecksAttr());
    state.addAttribute("ac.rule_handshake", firing.getHandshakeAttr());
    state.addAttribute("ac.rule_schedule", firing.getScheduleAttr());
    state.addAttribute("ac.rule_effects", firing.getEffectsAttr());
    state.addRegion();
    auto transform = cast<ac::TransformOp>(builder.create(state));
    transform.getBody().takeBody(firing.getBody());
    firing.replaceAllUsesWith(transform.getResults());
    firing.erase();
  }
  return success();
}

#define GEN_PASS_DEF_INFERRULETYPESPASS
#define GEN_PASS_DEF_INFERRULEEFFECTSPASS
#define GEN_PASS_DEF_MATERIALIZERULECHECKSPASS
#define GEN_PASS_DEF_MATERIALIZERULEHANDSHAKEPASS
#define GEN_PASS_DEF_DISCHARGERULEOBLIGATIONSPASS
#define GEN_PASS_DEF_RESOLVERULESCHEDULEPASS
#define GEN_PASS_DEF_LOWERRULESTOFIRINGPASS
#define GEN_PASS_DEF_CANONICALIZEPUREFIRINGSPASS
#define GEN_PASS_DEF_VERIFYRULECLOSUREPASS
#include "acir/Transforms/Passes.h.inc"

template <typename Base, LogicalResult (*Implementation)(ModuleOp)>
struct RulePass : Base {
  void runOnOperation() override {
    if (failed(Implementation(this->getOperation())))
      this->signalPassFailure();
  }
};

struct InferRuleTypesPass
    : RulePass<impl::InferRuleTypesPassBase<InferRuleTypesPass>,
               inferRuleTypes> {};
struct InferRuleEffectsPass
    : RulePass<impl::InferRuleEffectsPassBase<InferRuleEffectsPass>,
               inferRuleEffects> {};
struct MaterializeRuleChecksPass
    : RulePass<impl::MaterializeRuleChecksPassBase<MaterializeRuleChecksPass>,
               materializeRuleChecks> {};
struct MaterializeRuleHandshakePass
    : RulePass<
          impl::MaterializeRuleHandshakePassBase<MaterializeRuleHandshakePass>,
          materializeRuleHandshake> {};
struct DischargeRuleObligationsPass
    : RulePass<
          impl::DischargeRuleObligationsPassBase<DischargeRuleObligationsPass>,
          dischargeRuleObligations> {};
struct ResolveRuleSchedulePass
    : RulePass<impl::ResolveRuleSchedulePassBase<ResolveRuleSchedulePass>,
               resolveRuleSchedule> {};
struct LowerRulesToFiringPass
    : RulePass<impl::LowerRulesToFiringPassBase<LowerRulesToFiringPass>,
               lowerRulesToFiring> {};
struct CanonicalizePureFiringsPass
    : RulePass<
          impl::CanonicalizePureFiringsPassBase<CanonicalizePureFiringsPass>,
          canonicalizePureFirings> {};

struct VerifyRuleClosurePass
    : impl::VerifyRuleClosurePassBase<VerifyRuleClosurePass> {
  void runOnOperation() override {
    if (failed(verifyRuleClosure(getOperation())))
      signalPassFailure();
  }
};

} // namespace

LogicalResult verifyRuleClosure(ModuleOp model) {
  LogicalResult result = success();
  llvm::StringSet<> stableIds;
  model.walk([&](Operation *operation) {
    if (failed(result))
      return;
    if (isa<ac::RuleOp, ac::TypeConstraintMarkerOp, ac::ValueFactMarkerOp,
            ac::PendingObligationMarkerOp>(operation)) {
      result = operation->emitError(
          "unresolved transient rule or typed marker before Frozen ACIR");
      return;
    }
    StringAttr identity;
    if (auto firing = dyn_cast<ac::FiringOp>(operation)) {
      if (failed(firing.verify())) {
        result = failure();
        return;
      }
      identity = firing.getStableIdAttr();
    } else if (auto transform = dyn_cast<ac::TransformOp>(operation)) {
      if (failed(ac::verifyLoweredRuleTransformContract(transform))) {
        result = failure();
        return;
      }
      identity = transform->getAttrOfType<StringAttr>("ac.rule_stable_id");
    }
    if (identity && !stableIds.insert(identity.getValue()).second)
      result = operation->emitError() << "duplicate lowered rule identity '"
                                      << identity.getValue() << "'";
  });
  return result;
}

std::unique_ptr<Pass> createInferRuleTypesPass() {
  return std::make_unique<InferRuleTypesPass>();
}
std::unique_ptr<Pass> createInferRuleEffectsPass() {
  return std::make_unique<InferRuleEffectsPass>();
}
std::unique_ptr<Pass> createMaterializeRuleChecksPass() {
  return std::make_unique<MaterializeRuleChecksPass>();
}
std::unique_ptr<Pass> createMaterializeRuleHandshakePass() {
  return std::make_unique<MaterializeRuleHandshakePass>();
}
std::unique_ptr<Pass> createDischargeRuleObligationsPass() {
  return std::make_unique<DischargeRuleObligationsPass>();
}
std::unique_ptr<Pass> createResolveRuleSchedulePass() {
  return std::make_unique<ResolveRuleSchedulePass>();
}
std::unique_ptr<Pass> createLowerRulesToFiringPass() {
  return std::make_unique<LowerRulesToFiringPass>();
}
std::unique_ptr<Pass> createCanonicalizePureFiringsPass() {
  return std::make_unique<CanonicalizePureFiringsPass>();
}
std::unique_ptr<Pass> createVerifyRuleClosurePass() {
  return std::make_unique<VerifyRuleClosurePass>();
}

void addRuleLoweringPipeline(mlir::OpPassManager &manager) {
  manager.addPass(std::make_unique<InferRuleTypesPass>());
  manager.addPass(std::make_unique<InferRuleEffectsPass>());
  manager.addPass(std::make_unique<MaterializeRuleChecksPass>());
  manager.addPass(std::make_unique<MaterializeRuleHandshakePass>());
  manager.addPass(std::make_unique<DischargeRuleObligationsPass>());
  manager.addPass(std::make_unique<ResolveRuleSchedulePass>());
  manager.addPass(std::make_unique<LowerRulesToFiringPass>());
  manager.addPass(std::make_unique<CanonicalizePureFiringsPass>());
  manager.addPass(std::make_unique<VerifyRuleClosurePass>());
}

void registerRuleLoweringPipeline() {
  static mlir::PassPipelineRegistration<> registration(
      "ac-lower-rules",
      "Infer and materialize rules into marker-free internal ACIR",
      [](mlir::OpPassManager &manager) { addRuleLoweringPipeline(manager); });
  (void)registration;
}

} // namespace acir
