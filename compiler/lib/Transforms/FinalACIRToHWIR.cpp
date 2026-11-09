#include "pycircuit/Transforms/Passes.h"

#include "pycircuit/Dialect/ACIR/FinalHardwarePackage.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "pycircuit/Dialect/HWIR/HWIRDialect.h"
#include "pycircuit/Dialect/HWIR/HWIRVerification.h"

#include "mlir/IR/IRMapping.h"
#include "mlir/Pass/Pass.h"

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"

using namespace mlir;

namespace acir {
namespace {

StringRef mappedName(Operation *operation) {
  return llvm::StringSwitch<StringRef>(operation->getName().getStringRef())
      .Case("ac.module", "hwir.module")
      .Case("ac.system", "hwir.system")
      .Case("ac.instance", "hwir.instance")
      .Case("ac.collection", "hwir.collection")
      .Case("ac.queue", "hwir.queue")
      .Case("ac.rule", "hwir.rule")
      .Case("ac.yield", "hwir.yield")
      .Case("ac.expect", "hwir.expect")
      .Case("ac.observe", "hwir.observe")
      .Case("ac.table.create", "hwir.table.create")
      .Case("ac.table.splat", "hwir.table.splat")
      .Case("ac.table.map", "hwir.table.map")
      .Case("ac.table.view", "hwir.table.view")
      .Case("ac.table.index", "hwir.table.index")
      .Case("ac.table.get", "hwir.table.get")
      .Case("ac.table.match", "hwir.table.match")
      .Case("ac.table.choose", "hwir.table.choose")
      .Case("ac.table.fold", "hwir.table.fold")
      .Case("ac.value.merge", "hwir.value.merge")
      .Default({});
}

/// Consume final-ACIR proof markers before structural operations are renamed.
/// In particular, ValueRequire's payload authenticates a source use but must
/// never become an executable dependency in independent HWIR.
LogicalResult normalizeFinalProofOperations(ModuleOp target) {
  SmallVector<ac::ValueRequireOp> requirements;
  target.walk([&](ac::ValueRequireOp operation) {
    requirements.push_back(operation);
  });
  for (ac::ValueRequireOp requirement : requirements) {
    if (requirement.getResult().use_empty()) {
      requirement.erase();
      continue;
    }
    OpBuilder builder(requirement);
    OperationState oneState(requirement.getLoc(),
                            ac::BitsConstant4Op::getOperationName());
    oneState.addTypes(requirement.getResult().getType());
    oneState.addAttribute("pattern", builder.getStringAttr("1"));
    Value one = builder.create(oneState)->getResult(0);

    OperationState selectState(requirement.getLoc(),
                               ac::BitsSelectOp::getOperationName());
    selectState.addOperands(
        {requirement.getPath(), requirement.getAvailability(), one});
    selectState.addTypes(requirement.getResult().getType());
    Operation *select = builder.create(selectState);
    requirement.getResult().replaceAllUsesWith(select->getResult(0));
    requirement.erase();
  }
  return success();
}

FailureOr<OwningOpRef<ModuleOp>> convertFinalPackage(ModuleOp source) {
  MLIRContext *context = source.getContext();
  // All HWIR-owned facts must be attached to the converted package, never to
  // the certified final-ACIR input.  Besides preserving the pass boundary,
  // this is important because ACIR structural operations reject hwir.*
  // attributes during verification.
  OwningOpRef<ModuleOp> target = cast<ModuleOp>(source->clone());
  // Proof-only operations may be erased or replaced.  Normalize them before
  // retaining schedule/temporal-cut Operation pointers so those maps never
  // contain dangling keys.
  if (failed(normalizeFinalProofOperations(*target)))
    return failure();
  llvm::DenseMap<Operation *, uint64_t> scheduleRanks;
  llvm::DenseSet<Operation *> temporalCuts;
  llvm::DenseMap<Operation *, ArrayAttr> outputDependencies;
  ac::HardwareAnalysis analysis(*target);
  for (ac::ModuleOp module : target->getOps<ac::ModuleOp>()) {
    ac::HardwareBindings bindings;
    bindings.owner = module;
    auto schedule = analysis.analyzeModule(module, bindings);
    if (succeeded(schedule)) {
      outputDependencies.try_emplace(
          module, ac::serializeOutputDependencies(context,
                                                   schedule->dependencies));
      for (auto [ordinal, operation] : llvm::enumerate(schedule->schedule))
        scheduleRanks.try_emplace(operation, ordinal);
      for (Operation *operation : schedule->storageWork)
        temporalCuts.insert(operation);
    }
  }
  (*target)->removeAttr("ac.stage");
  (*target)->removeAttr(ac::HardwareOnlyAttrName);
  (*target)->setAttr("hwir.contract",
                     IntegerAttr::get(IntegerType::get(context, 64), 2));
  target->walk([&](Operation *operation) {
    if (!operation->hasAttr("ac.origin") && !operation->hasAttr("occurrence"))
      return;
    operation->setAttr(
        "hwir.source_operation",
        StringAttr::get(context, operation->getName().getStringRef()));
    std::string location;
    llvm::raw_string_ostream stream(location);
    operation->getLoc().print(stream);
    operation->setAttr("hwir.source_location",
                       StringAttr::get(context, location));
  });
  llvm::SmallVector<Operation *> replacements;
  target->walk<WalkOrder::PostOrder>([&](Operation *operation) {
    if (!mappedName(operation).empty())
      replacements.push_back(operation);
  });
  llvm::DenseMap<Operation *, Operation *> convertedOperations;
  for (Operation *operation : replacements) {
    OpBuilder builder(operation);
    OperationState state(operation->getLoc(), mappedName(operation));
    state.addOperands(operation->getOperands());
    state.addTypes(operation->getResultTypes());
    state.addAttributes(operation->getAttrs());
    for (unsigned index = 0; index < operation->getNumRegions(); ++index)
      state.addRegion();
    Operation *replacement = builder.create(state);
    convertedOperations.try_emplace(operation, replacement);
    for (auto [from, to] :
         llvm::zip_equal(operation->getRegions(), replacement->getRegions()))
      to.takeBody(from);
    operation->replaceAllUsesWith(replacement->getResults());
    operation->erase();
  }
  // Materialize HWIR schedule facts only after all structural ACIR operations
  // have become HWIR operations.  ACIR queues intentionally reject unknown
  // attributes, so attaching these facts earlier creates invalid transient IR.
  for (auto [operation, rank] : scheduleRanks) {
    Operation *converted = convertedOperations.lookup(operation);
    if (!converted)
      converted = operation;
    converted->setAttr(
        "hwir.schedule",
        IntegerAttr::get(IntegerType::get(context, 64), rank));
  }
  for (Operation *operation : temporalCuts) {
    Operation *converted = convertedOperations.lookup(operation);
    if (!converted)
      converted = operation;
    converted->setAttr("hwir.temporal_cut", UnitAttr::get(context));
  }
  for (auto [operation, dependencies] : outputDependencies) {
    Operation *converted = convertedOperations.lookup(operation);
    if (!converted)
      return failure();
    converted->setAttr("hwir.output_dependencies", dependencies);
  }
  // Translate the final-ACIR hardware boundary into backend-owned facts.  No
  // final proof record crosses this seam: the compact records below are
  // independently checked by the HWIR verifier after serialization.
  target->walk([&](hwir::ModuleOp module) {
    OpBuilder builder(module);
    auto signature = module.getFunctionType();
    uint64_t publicInputs = signature.getNumInputs();
    uint64_t publicOutputs = signature.getNumResults();
    ArrayAttr rootDefaults = builder.getArrayAttr({});
    if (auto internal =
            module->getAttrOfType<DictionaryAttr>("ac.internal_ports")) {
      if (auto value = internal.getAs<IntegerAttr>("source_inputs"))
        publicInputs = value.getValue().getZExtValue();
      if (auto value = internal.getAs<IntegerAttr>("source_outputs"))
        publicOutputs = value.getValue().getZExtValue();
      if (auto value = internal.getAs<ArrayAttr>("root_inputs"))
        rootDefaults = value;
    }
    module->setAttr(
        "hwir.interface",
        DictionaryAttr::get(
            context,
            {NamedAttribute(StringAttr::get(context, "public_inputs"),
                            builder.getI64IntegerAttr(publicInputs)),
             NamedAttribute(StringAttr::get(context, "public_outputs"),
                            builder.getI64IntegerAttr(publicOutputs)),
             NamedAttribute(StringAttr::get(context, "root_input_defaults"),
                            rootDefaults)}));
    if (Attribute domain = module->getAttr("ac.domain_inputs")) {
      module->setAttr("hwir.domain_inputs", domain);
      module->removeAttr("ac.domain_inputs");
    }
    if (Attribute kind = module->getAttr("ac.root_kind")) {
      module->setAttr("hwir.root_kind", kind);
      module->removeAttr("ac.root_kind");
    }
    module->removeAttr("ac.internal_ports");
  });
  target->walk([&](hwir::RuleOp rule) {
    auto sampled = rule->getAttrOfType<DictionaryAttr>("ac.sampled_admission");
    if (!sampled)
      return;
    auto clock = sampled.getAs<IntegerAttr>("clock_capture");
    auto reset = sampled.getAs<IntegerAttr>("reset_capture");
    if (clock && reset)
      rule->setAttr(
          "hwir.rule_domain",
          DictionaryAttr::get(
              context,
              {NamedAttribute(StringAttr::get(context, "clock_capture"),
                              clock),
               NamedAttribute(StringAttr::get(context, "reset_capture"),
                              reset)}));
  });
  target->walk([&](hwir::SystemOp system) {
    if (Attribute logical = system->getAttr("ac.entry_boundary")) {
      auto boundary = dyn_cast<DictionaryAttr>(logical);
      if (boundary)
        if (Attribute entry = boundary.get("logical_entry"))
          system->setAttr("hwir.logical_entry", entry);
      system->removeAttr("ac.entry_boundary");
    }
  });
  // These attributes certify and describe final-ACIR transaction lowering.
  // At this boundary their semantics have already been materialized as
  // explicit ports, occurrences, retained effects and wires. They are neither
  // part of the independent HWIR contract nor valid on hwir.module/rule after
  // ac.stage is removed.
  target->walk([](Operation *operation) {
    operation->removeAttr("ac.sampled_admission");
    operation->removeAttr("ac.transaction_wiring");
  });
  if (failed(hwir::verifyHWIRPackage(*target)))
    return failure();
  return target;
}

class FinalACIRToHWIRPass
    : public PassWrapper<FinalACIRToHWIRPass, OperationPass<ModuleOp>> {
public:
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(FinalACIRToHWIRPass)
  StringRef getArgument() const final { return "ac-final-to-hwir"; }
  StringRef getDescription() const final {
    return "Convert verified final ACIR into the independent HWIR contract";
  }
  void getDependentDialects(DialectRegistry &registry) const override {
    registry.insert<hwir::HWIRDialect>();
  }
  void runOnOperation() final {
    ModuleOp source = getOperation();
    auto stage = source->getAttrOfType<StringAttr>("ac.stage");
    if (!stage || stage.getValue() != "final" ||
        !ac::isHardwareOnlyPackage(source)) {
      source.emitOpError() << "ac-final-to-hwir requires certified final ACIR";
      signalPassFailure();
      return;
    }
    if (failed(ac::verifyHardwarePackage(source))) {
      signalPassFailure();
      return;
    }
    auto target = convertFinalPackage(source);
    if (failed(target)) {
      signalPassFailure();
      return;
    }
    source->setAttrs((**target)->getAttrs());
    source.getRegion().takeBody((**target).getRegion());
  }
};

} // namespace

std::unique_ptr<mlir::Pass> createFinalACIRToHWIRPass() {
  return std::make_unique<FinalACIRToHWIRPass>();
}

} // namespace acir
