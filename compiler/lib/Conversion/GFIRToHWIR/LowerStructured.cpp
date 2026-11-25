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
#include "mlir/IR/SymbolTable.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Pass/Pass.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/APSInt.h"
#include <functional>
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

LogicalResult materializeDefinition(const gfir::StructuredDefinition &facts,
                                    const gfir::StructuredProgram &program,
                                    mlir::ModuleOp package) {
  auto source = facts.module;
  OpBuilder builder(package.getContext());
  builder.setInsertionPointToEnd(package.getBody());
  OperationState definition(source.getLoc(), ac::ModuleOp::getOperationName());
  for (StringRef name : {"sym_name", "source_owner"})
    definition.addAttribute(name, source->getAttr(name));
  SmallVector<Type> inputTypes(source.getFunctionType().getInputs());
  SmallVector<Type> outputTypes(source.getFunctionType().getResults());
  SmallVector<Attribute> inputNames(source.getInputNames().getValue());
  SmallVector<Attribute> outputNames(source.getOutputNames().getValue());
  Type bit;
  if (!facts.inputAvailability.empty() || !facts.outputAvailability.empty() ||
      llvm::any_of(program.definitions, [](const auto &entry) { return !entry.inputAvailability.empty(); })) {
    auto owner = source.getSourceOwner();
    auto occurrence = builder.getDictionaryAttr({
        builder.getNamedAttr("site", builder.getDictionaryAttr({
            builder.getNamedAttr("definition", FlatSymbolRefAttr::get(source.getSymNameAttr())),
            builder.getNamedAttr("ast_path", builder.getArrayAttr({}))})),
        builder.getNamedAttr("expansion", builder.getArrayAttr({}))});
    auto location = builder.getDictionaryAttr({
        builder.getNamedAttr("path", owner.getAs<StringAttr>("path")),
        builder.getNamedAttr("line", builder.getI64IntegerAttr(1)),
        builder.getNamedAttr("column", builder.getI64IntegerAttr(1)),
        builder.getNamedAttr("end_line", builder.getI64IntegerAttr(1)),
        builder.getNamedAttr("end_column", builder.getI64IntegerAttr(2))});
    auto width = ac::StaticExprAttr::get(builder.getContext(), builder.getDictionaryAttr({
        builder.getNamedAttr("kind", builder.getStringAttr("literal")),
        builder.getNamedAttr("value", builder.getDictionaryAttr({
            builder.getNamedAttr("kind", builder.getStringAttr("integer")),
            builder.getNamedAttr("value", ac::MathIntAttr::get(builder.getContext(), llvm::APSInt(llvm::APInt(64, 1), true)))})),
        builder.getNamedAttr("origin", occurrence), builder.getNamedAttr("location", location)}));
    bit = ac::BitsType::get(builder.getContext(), width);
    SmallVector<Attribute> inputValidity(inputTypes.size(), builder.getI64IntegerAttr(-1));
    for (const auto &input : facts.inputAvailability) {
      inputValidity[input.port] = builder.getI64IntegerAttr(inputTypes.size());
      inputTypes.push_back(bit);
      inputNames.push_back(input.name);
    }
    SmallVector<Attribute> outputValidity(outputTypes.size(), builder.getI64IntegerAttr(-1));
    for (const auto &output : facts.outputAvailability) {
      outputValidity[output.port] = builder.getI64IntegerAttr(outputTypes.size());
      outputTypes.push_back(bit);
      outputNames.push_back(output.name);
    }
    if (!facts.inputAvailability.empty())
      definition.addAttribute("ac.input_validity", builder.getArrayAttr(inputValidity));
    if (!facts.outputAvailability.empty())
      definition.addAttribute("ac.output_validity", builder.getArrayAttr(outputValidity));
    SmallVector<Attribute> defaults(facts.inputAvailability.size(), builder.getI64IntegerAttr(1));
    definition.addAttribute("ac.internal_ports", builder.getDictionaryAttr({
        builder.getNamedAttr("source_inputs", builder.getI64IntegerAttr(source.getFunctionType().getNumInputs())),
        builder.getNamedAttr("source_outputs", builder.getI64IntegerAttr(source.getFunctionType().getNumResults())),
        builder.getNamedAttr("root_inputs", builder.getArrayAttr(defaults))}));
  }
  definition.addAttribute("function_type", TypeAttr::get(builder.getFunctionType(inputTypes, outputTypes)));
  definition.addAttribute("input_names", builder.getArrayAttr(inputNames));
  definition.addAttribute("output_names", builder.getArrayAttr(outputNames));
  definition.addAttribute("parameters", builder.getArrayAttr({}));
  definition.addAttribute("type_parameters", builder.getArrayAttr({}));
  definition.addRegion();
  auto module = cast<ac::ModuleOp>(builder.create(definition));
  auto *block = new Block;
  module.getBody().push_back(block);
  IRMapping mapping;
  for (BlockArgument input : source.getBody().front().getArguments())
    mapping.map(input, block->addArgument(input.getType(), input.getLoc()));
  llvm::DenseMap<unsigned, Value> inputCompanions;
  for (const auto &input : facts.inputAvailability)
    inputCompanions.try_emplace(input.port, block->addArgument(bit, source.getLoc()));
  llvm::DenseMap<std::pair<Operation *, unsigned>, Value> childCompanions;
  auto childFor = [&](gfir::CallOp call) -> const gfir::StructuredDefinition * {
    auto receiver = call.getReceiverAttr().getRootReference().getValue();
    for (auto instance : source.getBody().front().getOps<gfir::InstanceOp>()) {
      if (instance.getSymName() != receiver) continue;
      for (const auto &child : program.definitions) {
        auto module = child.module;
        if (module.getSymName() == instance.getCalleeAttr().getValue()) return &child;
      }
    }
    return nullptr;
  };

  struct Instance {
    gfir::InstanceOp declaration;
    unsigned ordinal;
  };
  llvm::StringMap<Instance> instances;
  for (auto [ordinal, operation] : llvm::enumerate(source.getBody().front()))
    if (auto instance = dyn_cast<gfir::InstanceOp>(operation))
      instances.try_emplace(instance.getSymName(),
                            Instance{instance, static_cast<unsigned>(ordinal)});
  llvm::DenseMap<Operation *, std::pair<unsigned, unsigned>> domainInputs;
  for (const auto &call : facts.clockedCalls)
    if (call.domainInputs)
      domainInputs.try_emplace(call.operation, *call.domainInputs);

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
      auto found = instances.find(call.getReceiverAttr().getRootReference().getValue());
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
      if (const auto *child = childFor(call)) {
        auto target = cast<ac::ModuleOp>(SymbolTable::lookupSymbolIn(package, instance.getCalleeAttr()));
        for (const auto &output : child->outputAvailability) {
          auto position = target.getFunctionType().getNumResults() - child->outputAvailability.size();
          state.addTypes(target.getFunctionType().getResult(position++));
        }
      }
    } else if (!isa<gfir::YieldOp>(operation)) {
      state.addAttributes(operation.getAttrs());
    }
    for (unsigned index = 0; index < operation.getNumRegions(); ++index)
      state.addRegion();
    Operation *lowered = builder.create(state);
    if (auto call = dyn_cast<gfir::CallOp>(operation))
      if (const auto *child = childFor(call)) {
        unsigned result = operation.getNumResults();
        for (const auto &output : child->outputAvailability)
          childCompanions.try_emplace(std::make_pair(&operation, output.port), lowered->getResult(result++));
      }
    operations.emplace_back(&operation, lowered);
    for (auto [original, result] :
         llvm::zip(operation.getResults(), lowered->getResults()))
      mapping.map(original, result);
  }
  Operation *yield = block->getTerminator();
  builder.setInsertionPoint(yield);
  auto make = [&](StringRef name, ValueRange operands, TypeRange types,
                  ArrayRef<NamedAttribute> attributes = {}) {
    OperationState state(source.getLoc(), name);
    state.addOperands(operands);
    state.addTypes(types);
    state.addAttributes(attributes);
    return builder.create(state);
  };
  Value one;
  auto truth = [&]() -> Value {
    if (!one)
      one = make(ac::BitsConstant4Op::getOperationName(), {}, {bit}, {
          builder.getNamedAttr("pattern", builder.getStringAttr("1"))})->getResult(0);
    return one;
  };
  ac::HardwareAnalysis analysis(package);
  auto project = [&](Value value, ArrayRef<StringAttr> path) -> FailureOr<Value> {
    for (auto field : path) {
      auto declaration = analysis.lookupStruct(cast<ac::StructType>(value.getType()));
      auto position = llvm::find_if(declaration.getFields(), [&](Attribute raw) {
        return cast<DictionaryAttr>(raw).getAs<StringAttr>("name") == field;
      });
      if (position == declaration.getFields().end()) return failure();
      auto type = cast<DictionaryAttr>(*position).getAs<TypeAttr>("type").getValue();
      value = make(ac::StructGetOp::getOperationName(), {value}, {type}, {
          builder.getNamedAttr("field", field)})->getResult(0);
    }
    return value;
  };
  llvm::DenseMap<unsigned, Value> selections;
  std::function<FailureOr<Value>(const gfir::StructuredAvailability &)> availability;
  availability = [&](const gfir::StructuredAvailability &sources) -> FailureOr<Value> {
    Value result;
    for (const auto &item : sources) {
      Value value;
      switch (item.kind) {
      case gfir::StructuredAvailabilitySource::Kind::Input:
        value = inputCompanions.lookup(item.index); break;
      case gfir::StructuredAvailabilitySource::Kind::PureOutput: {
        const auto &wire = facts.wires[item.index];
        const auto &child = program.definitions[wire.call->definition];
        value = childCompanions.lookup({wire.endpoint.value.getDefiningOp(),
                                       child.outputs[wire.call->output].port.port});
        break;
      }
      case gfir::StructuredAvailabilitySource::Kind::Select: {
        if (auto cached = selections.find(item.index); cached != selections.end()) {
          value = cached->second;
          break;
        }
        const auto &select = facts.availabilitySelects[item.index];
        const auto &endpoint = facts.wires[select.selector].endpoint;
        auto condition = project(mapping.lookup(endpoint.value), endpoint.path);
        auto yes = availability(select.yes), no = availability(select.no);
        if (failed(condition) || failed(yes) || failed(no)) return failure();
        value = make(ac::BitsSelectOp::getOperationName(), {*condition, *yes, *no}, {bit})->getResult(0);
        selections.try_emplace(item.index, value);
        break;
      }
      default:
        return source.emitOpError("pure Module availability requires ordinary inputs and pure calls");
      }
      if (!value) return failure();
      result = result ? make(ac::BitsBinaryOp::getOperationName(), {result, value}, {bit}, {
          builder.getNamedAttr("opcode", builder.getStringAttr("and"))})->getResult(0) : value;
    }
    return result ? result : truth();
  };
  for (auto [sourceOp, targetOp] : operations) {
    SmallVector<Value> operands;
    for (Value operand : sourceOp->getOperands()) {
      Value mapped = mapping.lookupOrNull(operand);
      if (!mapped)
        return sourceOp->emitOpError()
               << "operand outside the verified structured module graph";
      operands.push_back(mapped);
    }
    if (auto domain = domainInputs.find(sourceOp); domain != domainInputs.end()) {
      for (unsigned wire : {domain->second.first, domain->second.second}) {
        const auto &endpoint = facts.wires[wire].endpoint;
        Value value = mapping.lookupOrNull(endpoint.value);
        if (!value || !endpoint.path.empty())
          return sourceOp->emitOpError("missing verified explicit domain input");
        operands.push_back(value);
      }
    }
    if (auto call = dyn_cast<gfir::CallOp>(sourceOp))
      if (const auto *child = childFor(call))
        for (const auto &input : child->inputAvailability) {
          const auto &leaves = facts.valueWires.find(call.getInputs()[input.port])->second;
          const auto &sources = facts.wireAvailability.empty()
              ? gfir::StructuredAvailability{} : facts.wireAvailability[leaves.front()];
          auto available = availability(sources);
          if (failed(available)) return failure();
          operands.push_back(*available);
        }
    if (isa<gfir::YieldOp>(sourceOp))
      for (const auto &output : facts.outputAvailability) {
        auto available = availability(output.sources);
        if (failed(available)) return failure();
        operands.push_back(*available);
      }
    targetOp->setOperands(operands);
    for (auto [sourceRegion, targetRegion] :
         llvm::zip(sourceOp->getRegions(), targetOp->getRegions()))
      sourceRegion.cloneInto(&targetRegion, mapping);
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
                   ? materializeStructuredClocked(definition, *program, *candidate)
                   : materializeDefinition(definition, *program, *candidate)))
      return failure();
  if (program->root.getKind() == "system") {
    Operation *root = SymbolTable::lookupSymbolIn(*candidate, program->root.getCalleeAttr());
    root->setAttr("ac.root_kind", builder.getStringAttr("system"));
    if (auto samples = program->root.getSystemSamplesAttr())
      root->setAttr("ac.system_samples", samples);
  }
  builder.setInsertionPointToEnd(candidate->getBody());
  OperationState system(program->root.getLoc(), ac::SystemOp::getOperationName());
  system.addAttribute("entry", builder.getDictionaryAttr({
      builder.getNamedAttr("callee", program->root.getCalleeAttr()),
      builder.getNamedAttr("parameters", builder.getArrayAttr({})),
      builder.getNamedAttr("type_arguments", builder.getArrayAttr({}))}));
  system.addAttribute("domain", builder.getStringAttr("default"));
  if (auto policy = program->root.getExecutionPolicyAttr())
    system.addAttribute("ac.execution_policy", policy);
  const auto &root = program->definitions[program->rootIndex];
  if (root.clocked || !root.clockedCalls.empty())
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
