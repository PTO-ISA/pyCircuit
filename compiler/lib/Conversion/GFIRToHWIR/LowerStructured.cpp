// Structured GFIR retains module instances and typed call bindings. This
// refinement turns those bindings into the common hardware graph directly.
#include "pycircuit/Conversion/GFIRToHWIR.h"
#include "LowerStructuredClocked.h"

#include "pycircuit/Dialect/ACIR/ACIRDialect.h"
#include "pycircuit/Dialect/ACIR/ACIROps.h"
#include "pycircuit/Dialect/ACIR/HWIRPackageValidation.h"
#include "pycircuit/Dialect/GFIR/GFIRDialect.h"
#include "pycircuit/Dialect/GFIR/StructuredProgram.h"

#include "mlir/IR/Builders.h"
#include "mlir/IR/IRMapping.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Pass/Pass.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringMap.h"
#include <utility>

using namespace mlir;

namespace acir {
namespace {

DictionaryAttr instanceOccurrence(gfir::ModuleOp parent,
                                  gfir::InstanceOp instance, unsigned ordinal,
                                  Builder &builder) {
  if (auto source = instance->getAttrOfType<DictionaryAttr>("occurrence"))
    return source;
  auto component = builder.getDictionaryAttr({
      builder.getNamedAttr("kind", builder.getStringAttr("index")),
      builder.getNamedAttr("value", builder.getI64IntegerAttr(ordinal))});
  auto site = builder.getDictionaryAttr({
      builder.getNamedAttr("definition",
                           FlatSymbolRefAttr::get(parent.getSymNameAttr())),
      builder.getNamedAttr("ast_path", builder.getArrayAttr({component}))});
  return builder.getDictionaryAttr({
      builder.getNamedAttr("site", site),
      builder.getNamedAttr("expansion", builder.getArrayAttr({}))});
}

LogicalResult materializeDefinition(gfir::ModuleOp source,
                                    mlir::ModuleOp package) {
  OpBuilder builder(package.getContext());
  builder.setInsertionPointToEnd(package.getBody());
  OperationState definition(source.getLoc(), ac::ModuleOp::getOperationName());
  for (StringRef name : {"sym_name", "source_owner", "function_type",
                         "input_names", "output_names"})
    definition.addAttribute(name, source->getAttr(name));
  definition.addAttribute("parameters", builder.getArrayAttr({}));
  definition.addAttribute("type_parameters", builder.getArrayAttr({}));
  definition.addRegion();
  auto module = cast<ac::ModuleOp>(builder.create(definition));
  auto *block = new Block;
  module.getBody().push_back(block);
  IRMapping mapping;
  for (BlockArgument input : source.getBody().front().getArguments())
    mapping.map(input, block->addArgument(input.getType(), input.getLoc()));

  struct Instance {
    gfir::InstanceOp declaration;
    unsigned ordinal;
  };
  llvm::StringMap<Instance> instances;
  for (auto [ordinal, operation] : llvm::enumerate(source.getBody().front()))
    if (auto instance = dyn_cast<gfir::InstanceOp>(operation))
      instances.try_emplace(instance.getSymName(),
                            Instance{instance, static_cast<unsigned>(ordinal)});

  // Make every result before binding any operand. A legal field-level graph
  // can have forward references and even a cycle at whole-operation granularity;
  // scheduling operations topologically here would incorrectly reject it.
  SmallVector<std::pair<Operation *, Operation *>> operations;
  builder.setInsertionPointToEnd(block);
  for (Operation &operation : source.getBody().front()) {
    if (isa<gfir::InstanceOp>(operation))
      continue;
    StringRef name = operation.getName().getStringRef();
    if (isa<gfir::CallOp>(operation))
      name = ac::InstanceOp::getOperationName();
    else if (isa<gfir::YieldOp>(operation))
      name = ac::YieldOp::getOperationName();
    OperationState state(operation.getLoc(), name);
    state.addTypes(operation.getResultTypes());
    if (auto call = dyn_cast<gfir::CallOp>(operation)) {
      auto found = instances.find(call.getReceiverAttr().getValue());
      // The shared analysis proves this binding; keep construction total if
      // its admitted profile changes independently of this refinement.
      if (found == instances.end())
        return call.emitOpError() << "missing verified instance declaration";
      auto instance = found->second.declaration;
      state.addAttribute("instance_name", instance.getSymNameAttr());
      state.addAttribute("callee", instance.getCalleeAttr());
      state.addAttribute("parameters", builder.getArrayAttr({}));
      state.addAttribute("type_arguments", builder.getArrayAttr({}));
      state.addAttribute("occurrence",
                         instanceOccurrence(source, instance,
                                            found->second.ordinal, builder));
    } else if (!isa<gfir::YieldOp>(operation)) {
      state.addAttributes(operation.getAttrs());
    }
    Operation *lowered = builder.create(state);
    operations.emplace_back(&operation, lowered);
    for (auto [original, result] :
         llvm::zip(operation.getResults(), lowered->getResults()))
      mapping.map(original, result);
  }
  for (auto [sourceOp, targetOp] : operations) {
    SmallVector<Value> operands;
    for (Value operand : sourceOp->getOperands()) {
      Value mapped = mapping.lookupOrNull(operand);
      if (!mapped)
        return sourceOp->emitOpError()
               << "operand outside the verified structured module graph";
      operands.push_back(mapped);
    }
    targetOp->setOperands(operands);
  }
  return success();
}

class LowerStructuredGFIRPass
    : public PassWrapper<LowerStructuredGFIRPass,
                         OperationPass<mlir::ModuleOp>> {
public:
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(LowerStructuredGFIRPass)
  void getDependentDialects(DialectRegistry &registry) const override {
    registry.insert<ac::ACIRDialect, gfir::GFIRDialect>();
  }
  StringRef getArgument() const final { return "gfir-to-hwir"; }
  StringRef getDescription() const final {
    return "Materialize a verified structured GFIR graph as final hardware";
  }
  void runOnOperation() final {
    auto input = getOperation();
    auto candidate = detail::materializeStructuredHardware(input);
    if (failed(candidate)) {
      signalPassFailure();
      return;
    }
    // Native and HWIR validation both precede the first input mutation.
    input->setAttrs((**candidate)->getAttrs());
    input.getBodyRegion().takeBody((*candidate)->getBodyRegion());
  }
};

} // namespace

namespace detail {
FailureOr<OwningOpRef<mlir::ModuleOp>>
materializeStructuredHardware(mlir::ModuleOp package) {
  auto program = gfir::analyzeStructured(package);
  if (failed(program))
    return failure();
  OwningOpRef<mlir::ModuleOp> candidate =
      mlir::ModuleOp::create(package.getLoc());
  OpBuilder builder(package.getContext());
  candidate->getOperation()->setAttr("ac.stage", builder.getStringAttr("final"));
  builder.setInsertionPointToEnd(candidate->getBody());
  for (Operation &operation : *package.getBody())
    if (isa<ac::StructOp, ac::EnumOp>(operation))
      builder.clone(operation);
  for (const auto &definition : program->definitions)
    if (failed(definition.clocked
                   ? materializeStructuredClocked(definition, *candidate)
                   : materializeDefinition(definition.module, *candidate)))
      return failure();
  builder.setInsertionPointToEnd(candidate->getBody());
  OperationState system(program->root.getLoc(), ac::SystemOp::getOperationName());
  system.addAttribute("entry", builder.getDictionaryAttr({
      builder.getNamedAttr("callee", program->root.getCalleeAttr()),
      builder.getNamedAttr("parameters", builder.getArrayAttr({})),
      builder.getNamedAttr("type_arguments", builder.getArrayAttr({}))}));
  system.addAttribute("domain", builder.getStringAttr("default"));
  if (auto policy = program->root.getExecutionPolicyAttr())
    system.addAttribute("ac.execution_policy", policy);
  if (program->definitions[program->rootIndex].clocked)
    system.addAttribute("ac.managed_sampling", builder.getUnitAttr());
  builder.create(system);
  if (failed(verify(*candidate)) || failed(ac::verifyHardwarePackage(*candidate)))
    return failure();
  return std::move(candidate);
}
} // namespace detail

std::unique_ptr<Pass> createLowerStructuredGFIRPass() {
  return std::make_unique<LowerStructuredGFIRPass>();
}
} // namespace acir
