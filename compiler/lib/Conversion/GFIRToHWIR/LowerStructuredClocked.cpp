#include "LowerStructuredClocked.h"

#include "mlir/IR/Builders.h"
#include "mlir/IR/IRMapping.h"
#include "mlir/IR/SymbolTable.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/MathExtras.h"
#include <memory>

using namespace mlir;

namespace acir::detail {
namespace {

class ClockedMaterializer {
public:
  ClockedMaterializer(const gfir::StructuredDefinition &definition,
                      const gfir::StructuredProgram &program,
                      mlir::ModuleOp package)
      : definition(definition), program(program), plan(*definition.clocked), package(package),
        source(definition.module), builder(package.getContext()),
        hardware(source->getParentOfType<mlir::ModuleOp>()) {}
  LogicalResult run();

private:
  struct MemoryProposal {
    gfir::CallOp operation;
    gfir::StructuredMemoryUse use;
    Value rawPath;
    SmallVector<Value> arguments;
    Value reachedPrefix = {};
    unsigned pathResult = 0;
  };

  struct SampledContext {
    DictionaryAttr registration;
    ArrayRef<gfir::StructuredCheckSite> plannedChecks;
    SmallVector<Attribute> invocation, controls, checks;
    SmallVector<Value> predicates, reaches;
    SmallVector<Attribute> requirements, resources;
    SmallVector<MemoryProposal> memoryProposals;
    llvm::DenseMap<unsigned, SmallVector<Value>> memoryOutputs;
    llvm::DenseMap<unsigned, Value> guardAvailability;
    llvm::DenseMap<Operation *, Value> assertionConditions;
    llvm::DenseMap<Operation *, SmallVector<Value>> observationValues;
    Value prefix, path;
    DictionaryAttr pathIdentity;
  };

  Operation *make(StringRef name, ValueRange operands, TypeRange types,
                   ArrayRef<NamedAttribute> attributes = {});
  ac::StaticExprAttr literal(uint64_t value);
  Type bits(unsigned width);
  DictionaryAttr occurrence(StringRef kind, unsigned ordinal);
  LogicalResult addPrimitive(StringRef kind);
  Value constant(bool one);
  Value binary(StringRef opcode, Value left, Value right);
  Value merge(Value base, Value guard, Value value);
  Value known(Value value, bool high);
  FailureOr<unsigned> addControl(StringRef kind, Operation *site, Value raw);
  LogicalResult lowerMemory(gfir::CallOp call,
                            const gfir::StructuredMemoryUse &use,
                            IRMapping &mapping);
  DictionaryAttr memoryOwner();
  LogicalResult lowerSampled(unsigned index);
  void bindTransactionWiring();
  FailureOr<Value> zero(Type type);
  LogicalResult makeModule();
  LogicalResult allocate();
  LogicalResult cloneGraph();
  LogicalResult bindGraph();
  LogicalResult lowerRules();
  FailureOr<SmallVector<Value>> lowerBody(Block &body, ValueRange inputs,
                                         SmallVector<Value> &candidates,
                                         SmallVector<Value> &intents,
                                         IRMapping &mapping,
                                         std::optional<uint64_t> ordinal = {});
  FailureOr<Value> indexedWrite(Value previous, Value index, Value replacement);
  LogicalResult bindStorage();
  LogicalResult bindObservations();
  FailureOr<SmallVector<Value>> lowerChildMethod(gfir::CallOp call, unsigned methodIndex,
      SmallVector<Value> &local, SmallVector<Value> &localIntents, IRMapping &mapping);
  DictionaryAttr localStateID(Operation *storage);
  DictionaryAttr childStateID();

  const gfir::StructuredDefinition &definition;
  const gfir::StructuredProgram &program;
  const gfir::StructuredClocked &plan;
  mlir::ModuleOp package;
  gfir::ModuleOp source;
  OpBuilder builder;
  ac::HardwareAnalysis hardware;
  ac::ModuleOp target;
  Type bit;
  Value clock, reset, one, none;
  Value incomingData, incomingEnable, childQ, childData, childEnable;
  Operation *childInstance = nullptr, *childStorage = nullptr;
  IRMapping graph;
  SmallVector<Operation *> fifos, memories, states;
  SmallVector<Value> grants, candidates, intents, fifoData;
  SmallVector<Value> sampledEnables;
  SmallVector<Attribute> transactionWriters;
  SmallVector<Operation *> sampledRules;
  struct MemoryIssue {
    Value path;
    SmallVector<Value> arguments;
  };
  llvm::DenseMap<Operation *, MemoryIssue> memoryIssues;
  SampledContext *sampled = nullptr;
  SmallVector<std::pair<Operation *, Operation *>> graphOperations;
};

Operation *ClockedMaterializer::make(StringRef name, ValueRange operands,
                                     TypeRange types,
                                     ArrayRef<NamedAttribute> attributes) {
  OperationState state(source.getLoc(), name);
  state.addOperands(operands);
  state.addTypes(types);
  state.addAttributes(attributes);
  return builder.create(state);
}

DictionaryAttr ClockedMaterializer::occurrence(StringRef kind, unsigned ordinal) {
  auto component = builder.getDictionaryAttr({
      builder.getNamedAttr("kind", builder.getStringAttr("field")),
      builder.getNamedAttr("name", builder.getStringAttr(kind))});
  auto index = builder.getDictionaryAttr({
      builder.getNamedAttr("kind", builder.getStringAttr("index")),
      builder.getNamedAttr("value", builder.getI64IntegerAttr(ordinal))});
  auto site = builder.getDictionaryAttr({
      builder.getNamedAttr("definition", FlatSymbolRefAttr::get(source.getSymNameAttr())),
      builder.getNamedAttr("ast_path", builder.getArrayAttr({component, index}))});
  return builder.getDictionaryAttr({
      builder.getNamedAttr("site", site),
      builder.getNamedAttr("expansion", builder.getArrayAttr({}))});
}

ac::StaticExprAttr ClockedMaterializer::literal(uint64_t value) {
  auto owner = source.getSourceOwner();
  auto location = builder.getDictionaryAttr({
      builder.getNamedAttr("path", owner.getAs<StringAttr>("path")),
      builder.getNamedAttr("line", builder.getI64IntegerAttr(1)),
      builder.getNamedAttr("column", builder.getI64IntegerAttr(1)),
      builder.getNamedAttr("end_line", builder.getI64IntegerAttr(1)),
      builder.getNamedAttr("end_column", builder.getI64IntegerAttr(1))});
  auto number = ac::MathIntAttr::get(
      builder.getContext(), llvm::APSInt(llvm::APInt(64, value), true));
  return ac::StaticExprAttr::get(builder.getContext(), builder.getDictionaryAttr({
      builder.getNamedAttr("kind", builder.getStringAttr("literal")),
      builder.getNamedAttr("value", builder.getDictionaryAttr({
          builder.getNamedAttr("kind", builder.getStringAttr("integer")),
          builder.getNamedAttr("value", number)})),
      builder.getNamedAttr("origin", occurrence("literal", 0)),
      builder.getNamedAttr("location", location)}));
}

Type ClockedMaterializer::bits(unsigned width) {
  return ac::BitsType::get(builder.getContext(), literal(width));
}

LogicalResult ClockedMaterializer::addPrimitive(StringRef kind) {
  std::string name = ("pycircuit.__builtins__." + kind).str();
  for (Operation &op : *package.getBody()) {
    auto symbol = op.getAttrOfType<StringAttr>("sym_name");
    if (!symbol || symbol.getValue() != name)
      continue;
    if (isa<ac::ModuleImportOp>(op) &&
        op.getAttrOfType<StringAttr>("primitive_kind").getValue() == kind)
      return success();
    return source.emitOpError("definition collides with a canonical hardware primitive");
  }
  OpBuilder::InsertionGuard guard(builder);
  builder.setInsertionPointToStart(package.getBody());
  auto ref = FlatSymbolRefAttr::get(builder.getContext(), name);
  Type payload = ac::TypeParamType::get(builder.getContext(), ref,
                                       builder.getStringAttr("T"));
  bool memory = kind == "sync_mem" || kind == "sync_mem_dp" || kind == "byte_mem";
  bool fifo = kind == "fifo_local" || kind == "fifo_downstream_pop";
  SmallVector<Type> inputs = fifo ? SmallVector<Type>{bit, bit, bit, payload, bit}
                                  : SmallVector<Type>{bit, bit, bit, payload, payload};
  SmallVector<Type> outputs = fifo ? SmallVector<Type>{bit, bit, payload, bit}
                                   : SmallVector<Type>{payload};
  SmallVector<StringRef> inputNames = fifo
      ? SmallVector<StringRef>{"clk", "rst", "in_valid", "in_data", "out_ready"}
      : SmallVector<StringRef>{"clk", "rst", "en", "d", "init"};
  SmallVector<StringRef> outputNames = fifo
      ? SmallVector<StringRef>{"ready", "valid", "data", "local_capacity"}
      : SmallVector<StringRef>{"q"};
  if (memory) {
    // Keep the canonical primitive's address and byte-mask widths symbolic.
    auto provenance = literal(0).getTree();
    auto expression = [&](StringRef kind, ArrayRef<NamedAttribute> fields) {
      SmallVector<NamedAttribute> attrs(fields);
      attrs.push_back(builder.getNamedAttr("kind", builder.getStringAttr(kind)));
      attrs.push_back(builder.getNamedAttr("origin", provenance.get("origin")));
      attrs.push_back(builder.getNamedAttr("location", provenance.get("location")));
      return ac::StaticExprAttr::get(builder.getContext(), builder.getDictionaryAttr(attrs));
    };
    auto addressWidth = expression("reference", {
        builder.getNamedAttr("ref", builder.getDictionaryAttr({
            builder.getNamedAttr("kind", builder.getStringAttr("parameter")),
            builder.getNamedAttr("owner", ref),
            builder.getNamedAttr("name", builder.getStringAttr("ADDR_WIDTH"))}))});
    auto payloadWidth = expression("type_width", {
        builder.getNamedAttr("type", TypeAttr::get(payload))});
    auto arithmetic = [&](StringRef opcode, ac::StaticExprAttr left,
                           ac::StaticExprAttr right) {
      return expression("binary", {
          builder.getNamedAttr("operator", builder.getStringAttr(opcode)),
          builder.getNamedAttr("lhs", left), builder.getNamedAttr("rhs", right)});
    };
    bool asynchronous = kind == "byte_mem";
    bool dual = kind == "sync_mem_dp";
    auto numerator = asynchronous ? payloadWidth : arithmetic("add", payloadWidth, literal(7));
    Type address = ac::BitsType::get(builder.getContext(), addressWidth);
    Type mask = ac::BitsType::get(builder.getContext(),
                                  arithmetic("floordiv", numerator, literal(8)));
    inputs = {bit, bit};
    inputNames = {"clk", "rst"};
    if (dual) {
      inputs.append({bit, address, bit, address});
      inputNames.append({"ren0", "raddr0", "ren1", "raddr1"});
    } else if (asynchronous) {
      inputs.push_back(address);
      inputNames.push_back("raddr");
    } else {
      inputs.append({bit, address});
      inputNames.append({"ren", "raddr"});
    }
    inputs.append({bit, address, payload, mask});
    inputNames.append({"wvalid", "waddr", "wdata", "wstrb"});
    outputs = {payload};
    outputNames = {"rdata"};
    if (dual) {
      outputs.append({payload, bit, bit});
      outputNames = {"rdata0", "rdata1", "rlive0", "rlive1"};
    } else if (!asynchronous) {
      outputs.push_back(bit);
      outputNames.push_back("rlive");
    }
  }
  SmallVector<Attribute> dependencies;
  for (unsigned output = 0; output < outputs.size(); ++output) {
    SmallVector<Attribute> incoming;
    if (kind == "fifo_downstream_pop" && output == 0)
      incoming.push_back(builder.getDictionaryAttr({
          builder.getNamedAttr("port", builder.getI64IntegerAttr(4)),
          builder.getNamedAttr("path", builder.getArrayAttr({}))}));
    if (kind == "byte_mem")
      incoming.push_back(builder.getDictionaryAttr({
          builder.getNamedAttr("port", builder.getI64IntegerAttr(2)),
          builder.getNamedAttr("path", builder.getArrayAttr({}))}));
    dependencies.push_back(builder.getDictionaryAttr({
        builder.getNamedAttr("output", builder.getDictionaryAttr({
            builder.getNamedAttr("port", builder.getI64IntegerAttr(output)),
            builder.getNamedAttr("path", builder.getArrayAttr({}))})),
        builder.getNamedAttr("inputs", builder.getArrayAttr(incoming))}));
  }
  SmallVector<Attribute> parameters;
  if (fifo || memory) {
    SmallVector<StringRef> names = memory
        ? SmallVector<StringRef>{"ADDR_WIDTH", "DEPTH"}
        : SmallVector<StringRef>{"DEPTH", "AVAILABILITY_LATENCY"};
    for (StringRef parameter : names)
      parameters.push_back(builder.getDictionaryAttr({
          builder.getNamedAttr("name", builder.getStringAttr(parameter)),
          builder.getNamedAttr("type", TypeAttr::get(ac::MathIntType::get(builder.getContext())))}));
  }
  make(ac::ModuleImportOp::getOperationName(), {}, {}, {
      builder.getNamedAttr("sym_name", builder.getStringAttr(name)),
      builder.getNamedAttr("semantic_kind", builder.getStringAttr("hardware")),
      builder.getNamedAttr("source_owner", builder.getDictionaryAttr({
          builder.getNamedAttr("package", builder.getStringAttr("pycircuit")),
          builder.getNamedAttr("path", builder.getStringAttr("__builtins__.py"))})),
      builder.getNamedAttr("function_type", TypeAttr::get(builder.getFunctionType(inputs, outputs))),
      builder.getNamedAttr("input_names", builder.getStrArrayAttr(inputNames)),
      builder.getNamedAttr("output_names", builder.getStrArrayAttr(outputNames)),
      builder.getNamedAttr("parameters", builder.getArrayAttr(parameters)),
      builder.getNamedAttr("type_parameters", builder.getStrArrayAttr({"T"})),
      builder.getNamedAttr("primitive_kind", builder.getStringAttr(kind)),
      builder.getNamedAttr("dependency_summary", builder.getArrayAttr(dependencies))});
  return success();
}

Value ClockedMaterializer::constant(bool value) {
  return make(ac::BitsConstant4Op::getOperationName(), {}, {bit}, {
      builder.getNamedAttr("pattern", builder.getStringAttr(value ? "1" : "0"))})->getResult(0);
}

Value ClockedMaterializer::binary(StringRef opcode, Value left, Value right) {
  return make(ac::BitsBinaryOp::getOperationName(), {left, right}, {bit}, {
      builder.getNamedAttr("opcode", builder.getStringAttr(opcode))})->getResult(0);
}

Value ClockedMaterializer::merge(Value base, Value guard, Value value) {
  return make(ac::ValueMergeOp::getOperationName(), {base, guard, value},
              {base.getType(), bit}, {
      builder.getNamedAttr("paths", builder.getArrayAttr({builder.getArrayAttr({})})),
      builder.getNamedAttr("operandSegmentSizes", builder.getDenseI32ArrayAttr({1, 1, 1}))})->getResult(0);
}

Value ClockedMaterializer::known(Value value, bool high) {
  return make(ac::BitsCompareOp::getOperationName(), {value, high ? one : none},
              {bit}, {builder.getNamedAttr("predicate", builder.getStringAttr("case_eq"))})
      ->getResult(0);
}

FailureOr<unsigned> ClockedMaterializer::addControl(StringRef kind, Operation *site,
                                                    Value raw) {
  unsigned ordinal = sampled->controls.size();
  if (ordinal >= sampled->plannedChecks.size() ||
      sampled->plannedChecks[ordinal].id.get("check") !=
          plan.orderedOccurrences.lookup(site))
    return site->emitOpError("sampled refinement disagrees with the verified check order");
  const auto &check = sampled->plannedChecks[ordinal];
  sampled->controls.push_back(builder.getDictionaryAttr({
      builder.getNamedAttr("kind", builder.getStringAttr(kind)),
      builder.getNamedAttr("id", check.id),
      builder.getNamedAttr("invocation", builder.getArrayAttr(sampled->invocation)),
      builder.getNamedAttr("predecessor", builder.getI64IntegerAttr(int64_t(ordinal) - 1)),
      builder.getNamedAttr("path", sampled->pathIdentity)}));
  sampled->checks.push_back(builder.getDictionaryAttr({
      builder.getNamedAttr("id", check.id),
      builder.getNamedAttr("kind", builder.getStringAttr("guard_known")),
      builder.getNamedAttr("location", check.location)}));
  Value accepted = known(raw, true);
  if (kind == "branch_select")
    accepted = binary("or", known(raw, false), accepted);
  Value reached = sampled->prefix;
  if (sampled->path) {
    reached = binary("and", reached, sampled->path);
    accepted = make(ac::BitsSelectOp::getOperationName(),
                    {sampled->path, accepted, one}, {bit})->getResult(0);
  }
  sampled->predicates.push_back(raw);
  sampled->reaches.push_back(reached);
  sampled->prefix = binary("and", sampled->prefix, accepted);
  return ordinal;
}

DictionaryAttr ClockedMaterializer::memoryOwner() {
  return builder.getDictionaryAttr({
      builder.getNamedAttr("instance_path", builder.getArrayAttr({}))});
}

LogicalResult ClockedMaterializer::lowerMemory(
    gfir::CallOp call, const gfir::StructuredMemoryUse &use,
    IRMapping &mapping) {
  if (!sampled)
    return call.emitOpError("transaction Memory requires sampled Rule refinement");
  // This slice has direct ordered calls, without branch ancestry or an
  // additional authored path. Retain the canonical raw-statement path shape
  // required by the independent resource and pin proofs.
  Value rawPath = binary("and", one, one);
  const auto &memory = plan.memories[use.memory];
  if (use.access == gfir::StructuredMemoryAccess::Result) {
    auto outputs = sampled->memoryOutputs.find(use.memory);
    if (outputs == sampled->memoryOutputs.end())
      return call.emitOpError("missing canonical Memory response captures");
    Value data = outputs->second[use.port];
    Value live = outputs->second[memory.readPorts() + use.port];
    Value selected = make(ac::BitsSelectOp::getOperationName(),
                          {rawPath, live, one}, {bit})->getResult(0);
    auto control = addControl("resource_available", call, selected);
    if (failed(control))
      return failure();
    const auto &check = sampled->plannedChecks[*control];
    sampled->resources.push_back(builder.getDictionaryAttr({
        builder.getNamedAttr("control", builder.getI64IntegerAttr(*control)),
        builder.getNamedAttr("kind", builder.getStringAttr("memory_live")),
        builder.getNamedAttr("owner", memoryOwner()),
        builder.getNamedAttr("declaration", memories[use.memory]->getAttr("occurrence")),
        builder.getNamedAttr("occurrence", check.id.get("check")),
        builder.getNamedAttr("location", check.location),
        builder.getNamedAttr("port", builder.getI64IntegerAttr(use.port))}));
    mapping.map(call.getOutputs().front(), data);
    return success();
  }

  MemoryProposal proposal{call, use, rawPath, {}};
  for (auto [ordinal, input] : llvm::enumerate(call.getInputs())) {
    Value payload = mapping.lookup(input);
    unsigned control = sampled->controls.size();
    if (control >= sampled->plannedChecks.size())
      return call.emitOpError("missing verified Memory argument obligation");
    const auto &check = sampled->plannedChecks[control];
    // Primitive responses and the admitted ordinary captures have total
    // availability. A synchronous response's live prerequisite was admitted at
    // its result call. Payload X/Z remains intact for native enabled-pin checks.
    Value available = make(ac::ValueRequireOp::getOperationName(),
        {payload, one, rawPath}, {bit}, {
        builder.getNamedAttr("occurrence", check.id.get("check")),
        builder.getNamedAttr("source_location", check.location)})->getResult(0);
    if (failed(addControl("value_available", call, available)))
      return failure();
    sampled->requirements.push_back(builder.getDictionaryAttr({
        builder.getNamedAttr("control", builder.getI64IntegerAttr(control)),
        builder.getNamedAttr("kind", builder.getStringAttr("memory_argument")),
        builder.getNamedAttr("operand", builder.getI64IntegerAttr(ordinal)),
        builder.getNamedAttr("occurrence", check.id.get("check")),
        builder.getNamedAttr("location", check.location)}));
    proposal.arguments.push_back(payload);
  }
  if (use.access == gfir::StructuredMemoryAccess::Read) {
    // The asynchronous address belongs to this reached point, before loaded
    // data can influence a later guard or final atomic write admission.
    proposal.reachedPrefix = sampled->prefix;
    auto outputs = sampled->memoryOutputs.find(use.memory);
    if (outputs == sampled->memoryOutputs.end())
      return call.emitOpError("missing canonical asynchronous Memory data capture");
    mapping.map(call.getOutputs().front(), outputs->second[0]);
  }
  sampled->memoryProposals.push_back(std::move(proposal));
  return success();
}

FailureOr<Value> ClockedMaterializer::zero(Type type) {
  if (isa<ac::BitsType>(type)) {
    auto width = hardware.getPackedWidth(type, {}, source);
    if (failed(width))
      return failure();
    return make(ac::BitsConstant4Op::getOperationName(), {}, {type}, {
        builder.getNamedAttr("pattern", builder.getStringAttr(std::string(*width, '0')))})->getResult(0);
  }
  auto structure = hardware.lookupStruct(cast<ac::StructType>(type));
  SmallVector<Value> fields;
  for (Attribute raw : structure.getFields()) {
    auto field = cast<DictionaryAttr>(raw);
    auto value = zero(field.getAs<TypeAttr>("type").getValue());
    if (failed(value))
      return failure();
    fields.push_back(*value);
  }
  return make(ac::StructCreateOp::getOperationName(), fields, {type})->getResult(0);
}

LogicalResult ClockedMaterializer::makeModule() {
  builder.setInsertionPointToEnd(package.getBody());
  OperationState state(source.getLoc(), ac::ModuleOp::getOperationName());
  for (StringRef name : {"sym_name", "source_owner"})
    state.addAttribute(name, source->getAttr(name));
  SmallVector<Type> inputs(source.getFunctionType().getInputs());
  inputs.append({bit, bit});
  unsigned sourceInputs = inputs.size();
  Type incomingType;
  if (definition.incomingState) {
    auto variable = plan.variables[*definition.incomingState];
    incomingType = variable.getInitial().getType();
    inputs.append({incomingType, bit});
  }
  SmallVector<Type> outputs(source.getFunctionType().getResults());
  SmallVector<Attribute> outputNames(source.getOutputNames().getValue());
  SmallVector<Attribute> names(source.getInputNames().getValue());
  names.append({builder.getStringAttr("pyc_clk"), builder.getStringAttr("pyc_rst")});
  auto privateName = [&](StringRef stem) {
    std::string name = stem.str();
    while (llvm::is_contained(names, builder.getStringAttr(name)) ||
           llvm::is_contained(outputNames, builder.getStringAttr(name))) name += "_";
    return builder.getStringAttr(name);
  };
  if (!definition.outputAvailability.empty()) {
    SmallVector<Attribute> validity(outputs.size(), builder.getI64IntegerAttr(-1));
    for (const auto &output : definition.outputAvailability) {
      validity[output.port] = builder.getI64IntegerAttr(outputs.size());
      outputs.push_back(bit);
      outputNames.push_back(output.name);
    }
    state.addAttribute("ac.internal_ports", builder.getDictionaryAttr({
        builder.getNamedAttr("source_inputs", builder.getI64IntegerAttr(inputs.size())),
        builder.getNamedAttr("source_outputs", builder.getI64IntegerAttr(validity.size())),
        builder.getNamedAttr("root_inputs", builder.getArrayAttr({}))}));
    state.addAttribute("ac.output_validity", builder.getArrayAttr(validity));
  }
  if (definition.incomingState) {
    outputs.push_back(incomingType);
    outputNames.push_back(privateName("__ac_state_q"));
    names.push_back(privateName("__ac_state_data"));
    names.push_back(privateName("__ac_state_enable"));
    state.addAttribute("ac.internal_ports", builder.getDictionaryAttr({
        builder.getNamedAttr("source_inputs", builder.getI64IntegerAttr(sourceInputs)),
        builder.getNamedAttr("source_outputs", builder.getI64IntegerAttr(source.getFunctionType().getNumResults())),
        builder.getNamedAttr("root_inputs", builder.getArrayAttr({
            builder.getI64IntegerAttr(0), builder.getI64IntegerAttr(0)}))}));
  }
  state.addAttribute("output_names", builder.getArrayAttr(outputNames));
  state.addAttribute("function_type", TypeAttr::get(
      builder.getFunctionType(inputs, outputs)));
  state.addAttribute("input_names", builder.getArrayAttr(names));
  state.addAttribute("parameters", builder.getArrayAttr({}));
  state.addAttribute("type_parameters", builder.getArrayAttr({}));
  state.addAttribute("ac.domain_inputs", builder.getDictionaryAttr({
      builder.getNamedAttr("clock", builder.getI64IntegerAttr(sourceInputs - 2)),
      builder.getNamedAttr("reset", builder.getI64IntegerAttr(sourceInputs - 1))}));
  state.addRegion();
  target = cast<ac::ModuleOp>(builder.create(state));
  auto *body = new Block;
  target.getBody().push_back(body);
  for (Type type : inputs)
    body->addArgument(type, source.getLoc());
  for (auto [from, to] : llvm::zip(source.getBody().front().getArguments(),
                                   body->getArguments()))
    graph.map(from, to);
  clock = body->getArgument(sourceInputs - 2);
  reset = body->getArgument(sourceInputs - 1);
  if (definition.incomingState) {
    incomingData = body->getArgument(sourceInputs);
    incomingEnable = body->getArgument(sourceInputs + 1);
  }
  builder.setInsertionPointToEnd(body);
  one = constant(true);
  none = constant(false);
  return success();
}

LogicalResult ClockedMaterializer::allocate() {
  for (auto [index, fifo] : llvm::enumerate(plan.fifos)) {
    auto declaration = fifo.instance;
    StringRef kind = fifo.downstreamPop ? "fifo_downstream_pop" : "fifo_local";
    auto name = ("pycircuit.__builtins__." + kind).str();
    auto *instance = make(ac::InstanceOp::getOperationName(), {},
        {bit, bit, fifo.element, bit}, {
        builder.getNamedAttr("instance_name", declaration.getSymNameAttr()),
        builder.getNamedAttr("occurrence", occurrence("fifo", index)),
        builder.getNamedAttr("callee", FlatSymbolRefAttr::get(builder.getContext(), name)),
        builder.getNamedAttr("parameters", builder.getArrayAttr({literal(fifo.depth), literal(fifo.latency)})),
        builder.getNamedAttr("type_arguments", builder.getArrayAttr({TypeAttr::get(fifo.element)}))});
    fifos.push_back(instance);
  }
  for (auto [index, memory] : llvm::enumerate(plan.memories)) {
    auto instance = memory.instance;
    SmallVector<Type> results(memory.readPorts(), memory.element);
    if (!memory.asynchronous())
      results.append(memory.readPorts(), bit);
    auto name = ("pycircuit.__builtins__." + memory.kind).str();
    memories.push_back(make(ac::InstanceOp::getOperationName(), {}, results, {
        builder.getNamedAttr("instance_name", instance.getSymNameAttr()),
        builder.getNamedAttr("occurrence", occurrence("memory", index)),
        builder.getNamedAttr("callee", FlatSymbolRefAttr::get(builder.getContext(), name)),
        builder.getNamedAttr("parameters", builder.getArrayAttr({
            literal(memory.addressWidth), literal(memory.depth)})),
        builder.getNamedAttr("type_arguments", builder.getArrayAttr({TypeAttr::get(memory.element)}))}));
  }
  for (auto [index, declaration] : llvm::enumerate(plan.variables)) {
    auto var = declaration;
    Type type = var.getInitial().getType();
    auto table = dyn_cast<ac::TableType>(type);
    SmallVector<NamedAttribute> attrs{
        builder.getNamedAttr("instance_name", var.getSymNameAttr()),
        builder.getNamedAttr("occurrence", occurrence("state", index)),
        builder.getNamedAttr("callee", FlatSymbolRefAttr::get(builder.getContext(), "pycircuit.__builtins__.dffe")),
        builder.getNamedAttr("parameters", builder.getArrayAttr({})),
        builder.getNamedAttr("type_arguments", builder.getArrayAttr({
            TypeAttr::get(table ? table.getElementType() : type)}))};
    if (table)
      attrs.push_back(builder.getNamedAttr("shape", table.getShape()));
    states.push_back(make(table ? ac::CollectionOp::getOperationName()
                                 : ac::InstanceOp::getOperationName(), {}, {type}, attrs));
    candidates.push_back(states.back()->getResult(0));
    intents.push_back(none);
  }
  fifoData.resize(fifos.size());
  sampledEnables.resize(states.size());
  return success();
}

LogicalResult ClockedMaterializer::cloneGraph() {
  for (Operation &op : source.getBody().front()) {
    if (isa<gfir::InstanceOp, gfir::VarOp, gfir::RuleOp, gfir::MethodOp,
            gfir::ResultOp, gfir::YieldOp>(op))
      continue;
    if (auto read = dyn_cast<gfir::ReadOp>(op)) {
      graph.map(read.getValue(), states[plan.stateAccesses.lookup(read)]->getResult(0));
      continue;
    }
    if (auto call = dyn_cast<gfir::CallOp>(op); call && call.getInterfaceAttr()) {
      if (auto memory = plan.memoryCalls.find(call); memory != plan.memoryCalls.end()) {
        for (auto [index, output] : llvm::enumerate(call.getOutputs()))
          graph.map(output, memories[memory->second]->getResult(index));
        continue;
      }
      auto use = plan.fifoCalls.lookup(call);
      auto *fifo = fifos[use.fifo];
      unsigned result = use.access == gfir::StructuredFifoAccess::RawOut ? 1 : 0;
      for (Value value : call.getOutputs())
        graph.map(value, fifo->getResult(result++));
      continue;
    }
    OperationState state(op.getLoc(), op.getName().getStringRef());
    state.addTypes(op.getResultTypes());
    if (auto call = dyn_cast<gfir::CallOp>(op)) {
      gfir::InstanceOp instance;
      for (auto candidate : source.getBody().front().getOps<gfir::InstanceOp>())
        if (candidate.getSymName() == call.getReceiverAttr().getValue())
          instance = candidate;
      state.name = OperationName(ac::InstanceOp::getOperationName(), builder.getContext());
      state.addAttribute("instance_name", instance.getSymNameAttr());
      state.addAttribute("callee", instance.getCalleeAttr());
      state.addAttribute("parameters", builder.getArrayAttr({}));
      state.addAttribute("type_arguments", builder.getArrayAttr({}));
      state.addAttribute("occurrence", occurrence("child", graphOperations.size()));
      if (definition.childState && instance == definition.childState->receiver) {
        const auto &child = program.definitions[definition.childState->definition];
        auto childSource = child.module;
        auto variable = child.clocked->variables[definition.childState->state];
        auto childModule = cast<ac::ModuleOp>(SymbolTable::lookupSymbolIn(package,
            FlatSymbolRefAttr::get(childSource.getSymNameAttr())));
        for (auto storage : childModule.getBody().front().getOps<ac::InstanceOp>())
          if (storage.getInstanceName() == variable.getSymName())
            childStorage = storage;
        if (!childStorage)
          return call.emitOpError("missing materialized child state allocation");
        state.addTypes(childStorage->getResult(0).getType());
      }
    } else {
      state.addAttributes(op.getAttrs());
    }
    for (unsigned index = 0; index < op.getNumRegions(); ++index)
      state.addRegion();
    auto *copy = builder.create(state);
    auto childReceiver = definition.childState ? definition.childState->receiver : gfir::InstanceOp();
    if (auto call = dyn_cast<gfir::CallOp>(op);
        call && definition.childState &&
        call.getReceiverAttr().getValue() == childReceiver.getSymName()) {
      childInstance = copy;
      childQ = copy->getResults().back();
      childData = make(ac::BitsConstantOp::getOperationName(), {}, {childQ.getType()}, {
          builder.getNamedAttr("value", literal(0))})->getResult(0);
      childEnable = make(ac::BitsConstantOp::getOperationName(), {}, {bit}, {
          builder.getNamedAttr("value", literal(0))})->getResult(0);
    }
    graphOperations.push_back({&op, copy});
    for (auto [from, to] : llvm::zip(op.getResults(), copy->getResults()))
      graph.map(from, to);
  }
  return success();
}

LogicalResult ClockedMaterializer::bindGraph() {
  for (auto [original, copy] : graphOperations) {
    SmallVector<Value> operands;
    for (Value operand : original->getOperands()) {
      auto value = graph.lookupOrNull(operand);
      if (!value)
        return original->emitOpError("missing verified graph value in clocked refinement");
      operands.push_back(value);
    }
    if (isa<gfir::CallOp>(original)) {
      auto callee = copy->getAttrOfType<FlatSymbolRefAttr>("callee");
      auto *child = hardware.lookupDefinition(callee.getValue());
      auto domains = child->getAttrOfType<ArrayAttr>("domains");
      if (domains && !domains.empty())
        operands.append({clock, reset});
      if (copy == childInstance)
        operands.append({childData, childEnable});
    }
    copy->setOperands(operands);
    for (auto [sourceRegion, targetRegion] :
         llvm::zip(original->getRegions(), copy->getRegions()))
      sourceRegion.cloneInto(&targetRegion, graph);
  }
  return success();
}

FailureOr<Value> ClockedMaterializer::indexedWrite(Value previous, Value index,
                                                   Value replacement) {
  auto table = cast<ac::TableType>(previous.getType());
  auto size = hardware.getTableSize(table, {}, source);
  auto indexWidth = hardware.getPackedWidth(index.getType(), {}, source);
  if (failed(size) || failed(indexWidth))
    return failure();
  OperationState state(source.getLoc(), ac::TableMapOp::getOperationName());
  state.addOperands({previous, index, replacement});
  state.addTypes({table});
  state.addAttribute("shape", table.getShape());
  state.addAttribute("operandSegmentSizes", builder.getDenseI32ArrayAttr({1, 2}));
  state.addRegion();
  auto *mapped = builder.create(state);
  auto *body = new Block;
  mapped->getRegion(0).push_back(body);
  unsigned ordinalWidth = std::max(1u, llvm::Log2_64_Ceil(*size));
  Type ordinalType = bits(ordinalWidth);
  Value ordinal = body->addArgument(ordinalType, source.getLoc());
  Value old = body->addArgument(table.getElementType(), source.getLoc());
  Value actual = body->addArgument(index.getType(), source.getLoc());
  Value value = body->addArgument(replacement.getType(), source.getLoc());
  OpBuilder::InsertionGuard guard(builder);
  builder.setInsertionPointToEnd(body);
  Type comparedType = *indexWidth > ordinalWidth ? index.getType() : ordinalType;
  auto widen = [&](Value input) -> Value {
    if (input.getType() == comparedType)
      return input;
    if (*indexWidth == ordinalWidth)
      return make(ac::BitsExtractOp::getOperationName(), {input}, {comparedType}, {
          builder.getNamedAttr("low", literal(0))})->getResult(0);
    return make(ac::BitsResizeOp::getOperationName(), {input}, {comparedType}, {
        builder.getNamedAttr("mode", builder.getStringAttr("zext"))})->getResult(0);
  };
  ordinal = widen(ordinal);
  actual = widen(actual);
  Value equal = make(ac::BitsCompareOp::getOperationName(), {ordinal, actual}, {bit}, {
      builder.getNamedAttr("predicate", builder.getStringAttr("eq"))})->getResult(0);
  Value next = merge(old, equal, value);
  make(ac::YieldOp::getOperationName(), {next}, {});
  return mapped->getResult(0);
}

FailureOr<SmallVector<Value>>
ClockedMaterializer::lowerChildMethod(gfir::CallOp call, unsigned methodIndex,
    SmallVector<Value> &local, SmallVector<Value> &localIntents, IRMapping &mapping) {
  const auto &child = program.definitions[definition.childState->definition];
  auto method = child.clocked->methods[methodIndex];
  IRMapping nested;
  for (auto [argument, input] : llvm::zip(method.getBody().front().getArguments(), call.getInputs()))
    nested.map(argument, mapping.lookup(input));
  for (Operation &op : method.getBody().front()) {
    if (auto yield = dyn_cast<gfir::YieldOp>(op)) {
      SmallVector<Value> results;
      for (Value value : yield.getValues()) results.push_back(nested.lookup(value));
      return results;
    }
    if (auto read = dyn_cast<gfir::ReadOp>(op)) {
      if (child.clocked->stateAccesses.lookup(read) != definition.childState->state)
        return read.emitOpError("child Method read differs from the verified state owner");
      nested.map(read.getValue(), local[states.size()]);
    } else if (auto write = dyn_cast<gfir::WriteOp>(op)) {
      if (child.clocked->stateAccesses.lookup(write) != definition.childState->state ||
          write.getIndex() || write->hasAttr("fields"))
        return write.emitOpError("child Method requires its verified whole Bits state");
      local[states.size()] = nested.lookup(write.getValue());
      localIntents[states.size()] = one;
    } else if (auto invoke = dyn_cast<gfir::CallOp>(op)) {
      auto results = lowerChildMethod(invoke, child.clocked->methodCalls.lookup(invoke),
                                      local, localIntents, nested);
      if (failed(results))
        return failure();
      for (auto [result, value] : llvm::zip(invoke.getOutputs(), *results))
        nested.map(result, value);
    } else {
      builder.clone(op, nested);
    }
  }
  return method.emitOpError("verified child Method has no yield");
}

DictionaryAttr ClockedMaterializer::localStateID(Operation *storage) {
  return builder.getDictionaryAttr({
      builder.getNamedAttr("owner", builder.getDictionaryAttr({
          builder.getNamedAttr("instance_path", builder.getArrayAttr({}))})),
      builder.getNamedAttr("declaration", storage->getAttr("occurrence")),
      builder.getNamedAttr("element", builder.getArrayAttr({}))});
}

DictionaryAttr ClockedMaterializer::childStateID() {
  NamedAttrList id(localStateID(childStorage));
  id.set("owner", builder.getDictionaryAttr({builder.getNamedAttr("instance_path",
      builder.getArrayAttr({childInstance->getAttr("occurrence")}))}));
  return id.getDictionary(builder.getContext());
}

FailureOr<SmallVector<Value>>
ClockedMaterializer::lowerBody(Block &body, ValueRange inputs,
                               SmallVector<Value> &local,
                               SmallVector<Value> &localIntents,
                               IRMapping &mapping,
                               std::optional<uint64_t> ordinal) {
  for (auto [argument, input] : llvm::zip(body.getArguments(), inputs))
    mapping.map(argument, input);
  for (Operation &op : body) {
    if (isa<gfir::YieldOp, gfir::StepOp>(op)) {
      SmallVector<Value> values;
      for (Value value : op.getOperands())
        values.push_back(mapping.lookup(value));
      return values;
    }
    if (auto converted = dyn_cast<gfir::OrdinalBitsOp>(op)) {
      if (!ordinal)
        return converted.emitOpError("missing verified loop ordinal in hardware refinement");
      Value value = make(ac::BitsConstantOp::getOperationName(), {},
          {converted.getValue().getType()}, {
          builder.getNamedAttr("value", literal(*ordinal))})->getResult(0);
      mapping.map(converted.getValue(), value);
    } else if (auto loop = dyn_cast<gfir::LoopOp>(op)) {
      SmallVector<Value> carried;
      for (Value initial : loop.getInitial())
        carried.push_back(mapping.lookup(initial));
      auto &body = loop.getBody().front();
      uint64_t count = loop.getTripCountAttr().getInt();
      for (uint64_t iteration = 0; iteration < count; ++iteration) {
        // Every iteration starts with the enclosing immutable SSA bindings.
        // Candidates and write intent belong to the same transaction and flow
        // through the complete sequence of iterations.
        IRMapping nested(mapping);
        for (auto [argument, value] :
             llvm::zip(body.getArguments().drop_front(), carried))
          nested.map(argument, value);
        auto next = lowerBody(body, {}, local, localIntents, nested, iteration);
        if (failed(next))
          return failure();
        carried = std::move(*next);
      }
      for (auto [result, value] :
           llvm::zip(loop.getResults().drop_back(), carried))
        mapping.map(result, value);
      mapping.map(loop.getResults().back(), one);
    } else if (auto read = dyn_cast<gfir::ReadOp>(op)) {
      mapping.map(read.getValue(), local[plan.stateAccesses.lookup(read)]);
    } else if (auto write = dyn_cast<gfir::WriteOp>(op)) {
      unsigned target = plan.stateAccesses.lookup(write);
      Value value = mapping.lookup(write.getValue());
      Value previous = local[target];
      auto path = plan.writePaths.find(write);
      if (path != plan.writePaths.end())
        for (auto [field, type] : llvm::zip(path->second.fields, path->second.types))
          previous = make(ac::StructGetOp::getOperationName(), {previous}, {type}, {
              builder.getNamedAttr("field", field)})->getResult(0);
      if (write.getIndex()) {
        auto next = indexedWrite(previous, mapping.lookup(write.getIndex()), value);
        if (failed(next))
          return failure();
        value = *next;
      }
      if (path != plan.writePaths.end())
        value = make(ac::ValueMergeOp::getOperationName(), {local[target], one, value},
            {local[target].getType(), bit}, {
            builder.getNamedAttr("paths", builder.getArrayAttr({path->second.fields})),
            builder.getNamedAttr("operandSegmentSizes", builder.getDenseI32ArrayAttr({1, 1, 1}))})
                    ->getResult(0);
      local[target] = value;
      localIntents[target] = one;
    } else if (auto effect = dyn_cast<gfir::EffectOp>(op)) {
      if (effect.getKind() == "assert") {
        sampled->assertionConditions.try_emplace(effect,
            mapping.lookup(effect.getValues().front()));
      } else {
        auto &values = sampled->observationValues[effect];
        for (Value value : effect.getValues()) values.push_back(mapping.lookup(value));
      }
    } else if (auto guard = dyn_cast<gfir::GuardOp>(op)) {
      if (auto captured = plan.guardedCaptureUses.find(guard);
          captured != plan.guardedCaptureUses.end()) {
        unsigned control = sampled->controls.size();
        if (control >= sampled->plannedChecks.size())
          return guard.emitOpError("missing verified guard availability obligation");
        const auto &check = sampled->plannedChecks[control];
        Value availability = sampled->guardAvailability.lookup(captured->second);
        if (!availability)
          return guard.emitOpError("missing sampled producer availability capture");
        Value required = make(ac::ValueRequireOp::getOperationName(),
            {mapping.lookup(guard.getPredicate()), availability, one}, {bit}, {
            builder.getNamedAttr("occurrence", check.id.get("check")),
            builder.getNamedAttr("source_location", check.location)})->getResult(0);
        if (failed(addControl("value_available", guard, required)))
          return failure();
        sampled->requirements.push_back(builder.getDictionaryAttr({
            builder.getNamedAttr("control", builder.getI64IntegerAttr(control)),
            builder.getNamedAttr("kind", builder.getStringAttr("guard")),
            builder.getNamedAttr("operand", builder.getI64IntegerAttr(0)),
            builder.getNamedAttr("occurrence", check.id.get("check")),
            builder.getNamedAttr("location", check.location)}));
      }
      if (failed(addControl("ordered_guard", guard, mapping.lookup(guard.getPredicate()))))
        return failure();
    } else if (auto branch = dyn_cast<gfir::IfOp>(op)) {
      Value predicate = mapping.lookup(branch.getPredicate());
      Value parentPath;
      DictionaryAttr parentPathIdentity;
      std::optional<unsigned> selection;
      if (sampled && plan.checkedBranches.contains(branch)) {
        parentPath = sampled->path;
        parentPathIdentity = sampled->pathIdentity;
        auto control = addControl("branch_select", branch, predicate);
        if (failed(control))
          return failure();
        selection = *control;
        sampled->path = known(predicate, true);
        if (parentPath)
          sampled->path = binary("and", parentPath, sampled->path);
        sampled->pathIdentity = builder.getDictionaryAttr({
            builder.getNamedAttr("selection", builder.getI64IntegerAttr(*selection)),
            builder.getNamedAttr("when_true", builder.getBoolAttr(true))});
      }
      SmallVector<Value> thenState(local), thenIntents(localIntents);
      auto thenValues = lowerBody(branch.getThenBody().front(), {}, thenState,
                                  thenIntents, mapping);
      if (failed(thenValues))
        return failure();
      if (selection) {
        sampled->path = known(predicate, false);
        if (parentPath)
          sampled->path = binary("and", parentPath, sampled->path);
        sampled->pathIdentity = builder.getDictionaryAttr({
            builder.getNamedAttr("selection", builder.getI64IntegerAttr(*selection)),
            builder.getNamedAttr("when_true", builder.getBoolAttr(false))});
      }
      SmallVector<Value> elseState(local), elseIntents(localIntents);
      auto elseValues = lowerBody(branch.getElseBody().front(), {}, elseState,
                                  elseIntents, mapping);
      if (failed(elseValues))
        return failure();
      if (selection) {
        sampled->path = parentPath;
        sampled->pathIdentity = parentPathIdentity;
      }
      for (unsigned state : plan.branchWrites.find(branch)->second) {
        local[state] = merge(elseState[state], predicate, thenState[state]);
        localIntents[state] = merge(elseIntents[state], predicate, thenIntents[state]);
      }
      for (auto [result, yes, no] : llvm::zip(branch.getResults(), *thenValues, *elseValues))
        mapping.map(result, merge(no, predicate, yes));
    } else if (auto call = dyn_cast<gfir::CallOp>(op)) {
      if (auto child = plan.childMethodCalls.find(call); child != plan.childMethodCalls.end()) {
        auto results = lowerChildMethod(call, child->second, local, localIntents, mapping);
        if (failed(results)) return failure();
        for (auto [result, value] : llvm::zip(call.getOutputs(), *results))
          mapping.map(result, value);
        continue;
      }
      if (auto memory = plan.transactionMemoryCalls.find(call);
          memory != plan.transactionMemoryCalls.end()) {
        if (failed(lowerMemory(call, memory->second, mapping)))
          return failure();
        continue;
      }
      SmallVector<Value> results;
      if (!call.getReceiverAttr()) {
        SmallVector<Value> arguments;
        for (Value input : call.getInputs())
          arguments.push_back(mapping.lookup(input));
        IRMapping nested;
        auto method = plan.methods[plan.methodCalls.lookup(call)];
        if (sampled) {
          sampled->invocation.push_back(builder.getDictionaryAttr({
              builder.getNamedAttr("call", plan.orderedOccurrences.lookup(call)),
              builder.getNamedAttr("receiver", builder.getDictionaryAttr({
                  builder.getNamedAttr("instance_path", builder.getArrayAttr({}))}))}));
          if (failed(addControl("callable_guard", method, one)))
            return failure();
        }
        auto values = lowerBody(method.getBody().front(),
                                 arguments, local, localIntents, nested);
        if (sampled)
          sampled->invocation.pop_back();
        if (failed(values))
          return failure();
        results = std::move(*values);
      } else {
        auto use = plan.fifoCalls.lookup(call);
        if (use.access == gfir::StructuredFifoAccess::In)
          fifoData[use.fifo] = mapping.lookup(call.getInputs().front());
        else
          results.push_back(fifos[use.fifo]->getResult(2));
      }
      for (auto [from, to] : llvm::zip(call.getOutputs(), results))
        mapping.map(from, to);
    } else {
      builder.clone(op, mapping);
    }
  }
  return body.getParentOp()->emitOpError("ordered GFIR body has no yield");
}

LogicalResult ClockedMaterializer::lowerSampled(unsigned index) {
  auto rule = plan.rules[index].operation;
  SmallVector<Value> captures;
  for (Value input : rule.getCaptures())
    captures.push_back(graph.lookup(input));
  unsigned clockCapture = captures.size();
  captures.append({clock, reset});
  for (unsigned state : plan.rules[index].states)
    captures.push_back(states[state]->getResult(0));
  std::optional<unsigned> childCapture;
  if (plan.rules[index].usesChildState) {
    if (!childQ) return rule.emitOpError("missing actual child Q route");
    childCapture = captures.size();
    captures.push_back(childQ);
  }
  SmallVector<std::pair<unsigned, unsigned>> guardCaptures;
  for (auto [capture, producer] : plan.rules[index].guardCaptures) {
    if (!grants[producer])
      return rule.emitOpError("sampled producer must precede its guard consumer");
    guardCaptures.emplace_back(capture, captures.size());
    captures.push_back(grants[producer]);
  }
  SmallVector<std::pair<unsigned, unsigned>> memoryCaptures;
  for (Operation &operation : rule.getBody().front()) {
    auto use = plan.transactionMemoryCalls.find(&operation);
    if (use == plan.transactionMemoryCalls.end() ||
        (use->second.access != gfir::StructuredMemoryAccess::Result &&
         use->second.access != gfir::StructuredMemoryAccess::Read) ||
        llvm::any_of(memoryCaptures, [&](const auto &capture) {
          return capture.first == use->second.memory;
        }))
      continue;
    unsigned memory = use->second.memory;
    memoryCaptures.emplace_back(memory, captures.size());
    llvm::append_range(captures, memories[memory]->getResults());
  }

  auto region = std::make_unique<Region>();
  auto *body = new Block;
  region->push_back(body);
  for (Value capture : captures)
    body->addArgument(capture.getType(), rule.getLoc());
  OpBuilder::InsertionGuard insertion(builder);
  builder.setInsertionPointToEnd(body);
  Value outerOne = one, outerNone = none;
  auto controlLiteral = [&](uint64_t value) {
    return make(ac::BitsConstantOp::getOperationName(), {}, {bit}, {
        builder.getNamedAttr("value", literal(value))})->getResult(0);
  };
  one = controlLiteral(1);
  none = controlLiteral(0);
  SampledContext context;
  context.registration = plan.orderedOccurrences.lookup(rule);
  context.plannedChecks = plan.rules[index].checks;
  context.pathIdentity = builder.getDictionaryAttr({});
  context.prefix = one;
  for (auto [capture, ordinal] : guardCaptures)
    context.guardAvailability.try_emplace(capture, body->getArgument(ordinal));
  for (auto [memory, first] : memoryCaptures)
    for (unsigned result = 0; result < memories[memory]->getNumResults(); ++result)
      context.memoryOutputs[memory].push_back(body->getArgument(first + result));
  sampled = &context;
  if (failed(addControl("callable_guard", rule, one)))
    return failure();

  SmallVector<Value> local(states.size()), localIntents(states.size(), none);
  for (auto [ordinal, state] : llvm::enumerate(plan.rules[index].states))
    local[state] = body->getArgument(clockCapture + 2 + ordinal);
  if (childCapture) {
    local.push_back(body->getArgument(*childCapture));
    localIntents.push_back(none);
  }
  IRMapping mapping;
  auto results = lowerBody(rule.getBody().front(),
      body->getArguments().take_front(clockCapture), local, localIntents, mapping);
  if (failed(results)) {
    sampled = nullptr;
    one = outerOne;
    none = outerNone;
    return failure();
  }
  const auto &assertions = plan.rules[index].assertions;
  if (context.controls.size() + assertions.size() != context.plannedChecks.size())
    return rule.emitOpError("sampled refinement did not emit all verified checks");
  SmallVector<Value> yielded;
  const auto &resultUses = plan.rules[index].resultUses;
  if (!resultUses.empty())
    yielded.append(*results);
  unsigned ordinaryResults = yielded.size();
  SmallVector<Attribute> candidateRows;
  for (auto [ordinal, state] : llvm::enumerate(plan.rules[index].writes)) {
    yielded.push_back(local[state]);
    yielded.push_back(binary("and", context.prefix, localIntents[state]));
    auto stateID = localStateID(states[state]);
    candidateRows.push_back(builder.getDictionaryAttr({
        builder.getNamedAttr("state", stateID),
        builder.getNamedAttr("data", builder.getI64IntegerAttr(ordinaryResults + 2 * ordinal)),
        builder.getNamedAttr("enable", builder.getI64IntegerAttr(ordinaryResults + 2 * ordinal + 1))}));
  }
  std::optional<unsigned> childCandidate;
  if (definition.childState && definition.childState->writer == index) {
    childCandidate = yielded.size();
    yielded.push_back(local[states.size()]);
    yielded.push_back(binary("and", context.prefix, localIntents[states.size()]));
    candidateRows.push_back(builder.getDictionaryAttr({
        builder.getNamedAttr("state", childStateID()),
        builder.getNamedAttr("data", builder.getI64IntegerAttr(*childCandidate)),
        builder.getNamedAttr("enable", builder.getI64IntegerAttr(*childCandidate + 1))}));
  }
  unsigned fireResult = yielded.size();
  yielded.push_back(context.prefix);
  SmallVector<Attribute> observations;
  for (const auto &observation : plan.rules[index].observations) {
    auto effect = observation.operation;
    auto found = context.observationValues.find(effect);
    if (found == context.observationValues.end())
      return effect.emitOpError("missing ordered observation snapshot");
    observations.push_back(builder.getDictionaryAttr({
        builder.getNamedAttr("id", observation.id),
        builder.getNamedAttr("path", builder.getDictionaryAttr({})),
        builder.getNamedAttr("value_count", builder.getI64IntegerAttr(found->second.size()))}));
    yielded.push_back(context.prefix);
    yielded.append(found->second);
  }
  unsigned checkBase = yielded.size();
  if (!context.resources.empty()) {
    // Memory snapshots do not suppress previously reached diagnostics. With
    // no Queue demands or resource branches, the existing demand forest folds
    // to H=one. Keep its private canonical possible-execution wrapper on every
    // reached check, independently of transaction fire and physical proposals.
    Value mayExecute = make(ac::BitsUnaryOp::getOperationName(),
        {known(one, false)}, {bit}, {
        builder.getNamedAttr("opcode", builder.getStringAttr("not"))})->getResult(0);
    for (Value &reached : context.reaches)
      reached = binary("and", reached, mayExecute);
  }
  for (auto [raw, reached] : llvm::zip(context.predicates, context.reaches))
    yielded.append({raw, reached});
  SmallVector<Attribute> bodyChecks;
  for (auto assertion : assertions) {
    unsigned ordinal = context.checks.size();
    const auto &check = context.plannedChecks[ordinal];
    Value condition = context.assertionConditions.lookup(assertion);
    if (!condition)
      return assertion.emitOpError("missing ordered assertion condition");
    context.checks.push_back(builder.getDictionaryAttr({
        builder.getNamedAttr("id", check.id),
        builder.getNamedAttr("kind", builder.getStringAttr("assert")),
        builder.getNamedAttr("location", check.location)}));
    bodyChecks.push_back(builder.getDictionaryAttr({
        builder.getNamedAttr("id", check.id),
        builder.getNamedAttr("invocation", builder.getArrayAttr({})),
        builder.getNamedAttr("path", builder.getDictionaryAttr({}))}));
    yielded.append({condition, context.prefix});
  }
  SmallVector<Attribute> memoryProposals;
  for (auto &proposal : context.memoryProposals) {
    unsigned rawResult = yielded.size();
    yielded.push_back(proposal.rawPath);
    proposal.pathResult = yielded.size();
    Value prefix = proposal.reachedPrefix ? proposal.reachedPrefix : context.prefix;
    yielded.push_back(binary("and", prefix, proposal.rawPath));
    SmallVector<Attribute> arguments;
    for (Value argument : proposal.arguments) {
      arguments.push_back(builder.getI64IntegerAttr(yielded.size()));
      yielded.push_back(argument);
    }
    auto memory = builder.getDictionaryAttr({
        builder.getNamedAttr("owner", memoryOwner()),
        builder.getNamedAttr("declaration",
                             memories[proposal.use.memory]->getAttr("occurrence"))});
    unsigned firstCheck = plan.checkOffsets.lookup(proposal.operation);
    const auto &check = context.plannedChecks[firstCheck];
    StringRef kind = "request";
    if (proposal.use.access == gfir::StructuredMemoryAccess::Write)
      kind = "write";
    else if (proposal.use.access == gfir::StructuredMemoryAccess::Read)
      kind = "read";
    memoryProposals.push_back(builder.getDictionaryAttr({
        builder.getNamedAttr("memory", memory),
        builder.getNamedAttr("kind", builder.getStringAttr(kind)),
        builder.getNamedAttr("port", builder.getI64IntegerAttr(proposal.use.port)),
        builder.getNamedAttr("occurrence", check.id.get("check")),
        builder.getNamedAttr("location", check.location),
        builder.getNamedAttr("raw_path", builder.getI64IntegerAttr(rawResult)),
        builder.getNamedAttr("path", builder.getI64IntegerAttr(proposal.pathResult)),
        builder.getNamedAttr("arguments", builder.getArrayAttr(arguments))}));
  }
  make(ac::YieldOp::getOperationName(), yielded, {});
  sampled = nullptr;
  one = outerOne;
  none = outerNone;

  builder.setInsertionPointToEnd(&target.getBody().front());
  OperationState operation(rule.getLoc(), ac::RuleOp::getOperationName());
  operation.addOperands(captures);
  for (Value value : yielded)
    operation.addTypes(value.getType());
  operation.addAttribute("name", rule.getSymNameAttr());
  operation.addAttribute("occurrence", context.registration);
  operation.addAttribute("ac.required_checks", builder.getArrayAttr(context.checks));
  NamedAttrList admission({
      builder.getNamedAttr("clock_capture", builder.getI64IntegerAttr(clockCapture)),
      builder.getNamedAttr("reset_capture", builder.getI64IntegerAttr(clockCapture + 1)),
      builder.getNamedAttr("fire_result", builder.getI64IntegerAttr(fireResult)),
      builder.getNamedAttr("controls", builder.getArrayAttr(context.controls)),
      builder.getNamedAttr("body_checks", builder.getArrayAttr(bodyChecks)),
      builder.getNamedAttr("observations", builder.getArrayAttr(observations))});
  if (!context.requirements.empty())
    admission.append("requirements", builder.getArrayAttr(context.requirements));
  if (!context.resources.empty())
    admission.append("resources", builder.getArrayAttr(context.resources));
  if (!memoryProposals.empty())
    admission.append("memory_proposals", builder.getArrayAttr(memoryProposals));
  operation.addAttribute("ac.sampled_admission", admission.getDictionary(builder.getContext()));
  operation.addRegion(std::move(region));
  auto *lowered = builder.create(operation);
  sampledRules[index] = lowered;
  for (auto &proposal : context.memoryProposals) {
    MemoryIssue issue;
    issue.path = lowered->getResult(proposal.pathResult);
    for (unsigned argument = 0; argument < proposal.arguments.size(); ++argument)
      issue.arguments.push_back(lowered->getResult(proposal.pathResult + 1 + argument));
    memoryIssues.try_emplace(proposal.operation, std::move(issue));
  }
  for (auto result : resultUses)
    graph.map(result.getValue(), lowered->getResult(result.getOrdinalAttr().getInt()));
  for (auto [ordinal, state] : llvm::enumerate(plan.rules[index].writes)) {
    candidates[state] = lowered->getResult(ordinaryResults + 2 * ordinal);
    sampledEnables[state] = lowered->getResult(ordinaryResults + 2 * ordinal + 1);
  }
  if (childCandidate) {
    childData = lowered->getResult(*childCandidate);
    childEnable = lowered->getResult(*childCandidate + 1);
  }
  for (auto [ordinal, raw] : llvm::enumerate(
           ArrayRef<Attribute>(context.checks).take_front(context.controls.size()))) {
    auto check = cast<DictionaryAttr>(raw);
    make(ac::SourceExpectOp::getOperationName(), {
        lowered->getResult(checkBase + 2 * ordinal),
        lowered->getResult(checkBase + 1 + 2 * ordinal), clock, reset}, {}, {
        builder.getNamedAttr("kind", builder.getStringAttr("guard_known")),
        builder.getNamedAttr("location", check.get("location")),
        builder.getNamedAttr("ac.check_id", check.get("id"))});
  }
  for (auto [index, assertion] : llvm::enumerate(assertions)) {
    unsigned ordinal = context.controls.size() + index;
    const auto &check = context.plannedChecks[ordinal];
    SmallVector<NamedAttribute> attrs{
        builder.getNamedAttr("kind", builder.getStringAttr("assert")),
        builder.getNamedAttr("location", check.location),
        builder.getNamedAttr("ac.check_id", check.id)};
    if (auto message = assertion->getAttr("message"))
      attrs.push_back(builder.getNamedAttr("ac.message", message));
    make(ac::SourceExpectOp::getOperationName(), {
        lowered->getResult(checkBase + 2 * ordinal),
        lowered->getResult(checkBase + 1 + 2 * ordinal)}, {}, attrs);
  }
  transactionWriters.push_back(builder.getDictionaryAttr({
      builder.getNamedAttr("registration", context.registration),
      builder.getNamedAttr("ordinary_results", builder.getI64IntegerAttr(ordinaryResults)),
      builder.getNamedAttr("candidates", builder.getArrayAttr(candidateRows))}));
  grants[index] = lowered->getResult(fireResult);
  return success();
}

void ClockedMaterializer::bindTransactionWiring() {
  if (transactionWriters.empty() && !definition.incomingState)
    return;
  SmallVector<Attribute> declarations;
  for (auto [index, state] : llvm::enumerate(states))
    if (sampledEnables[index] || definition.incomingState == index)
      declarations.push_back(state->getAttr("occurrence"));
  SmallVector<Attribute> incoming;
  if (definition.incomingState)
    incoming.push_back(builder.getDictionaryAttr({
        builder.getNamedAttr("state", localStateID(states[*definition.incomingState])),
        builder.getNamedAttr("data", builder.getI64IntegerAttr(cast<BlockArgument>(incomingData).getArgNumber())),
        builder.getNamedAttr("enable", builder.getI64IntegerAttr(cast<BlockArgument>(incomingEnable).getArgNumber()))}));
  target->setAttr("ac.transaction_wiring", builder.getDictionaryAttr({
      builder.getNamedAttr("states", builder.getArrayAttr(declarations)),
      builder.getNamedAttr("incoming", builder.getArrayAttr(incoming)),
      builder.getNamedAttr("writers", builder.getArrayAttr(transactionWriters))}));
}

LogicalResult ClockedMaterializer::lowerRules() {
  grants.resize(plan.rules.size());
  sampledRules.resize(plan.rules.size());
  for (unsigned node : definition.order) {
    if (node < definition.wires.size() || node >= definition.firstClockedCallNode())
      continue;
    unsigned index = node - definition.wires.size();
    if (plan.rules[index].sampled) {
      if (failed(lowerSampled(index)))
        return failure();
      continue;
    }
    Value grant = one;
    for (const auto &required : plan.rules[index].requirements) {
      auto *fifo = fifos[required.fifo];
      const auto &definition = plan.fifos[required.fifo];
      Value available = fifo->getResult(required.requiresReady ? 0 : 1);
      if (required.requiresReady && definition.downstreamPop &&
          definition.consumer == index) {
        available = fifo->getResult(3);
        if (required.ownPop)
          available = binary("or", available, fifo->getResult(1));
      }
      grant = binary("and", grant, available);
    }
    grants[index] = grant;
    SmallVector<Value> local;
    SmallVector<Value> localIntents(states.size(), none);
    for (Operation *state : states)
      local.push_back(state->getResult(0));
    if (plan.rules[index].usesChildState) {
      local.push_back(childQ);
      localIntents.push_back(none);
    }
    IRMapping mapping;
    auto rule = plan.rules[index].operation;
    SmallVector<Value> inputs;
    for (Value capture : rule.getCaptures()) {
      Value input = graph.lookupOrNull(capture);
      if (!input)
        return rule.emitOpError("missing verified Rule capture in clocked refinement");
      inputs.push_back(input);
    }
    auto results = lowerBody(rule.getBody().front(), inputs, local,
                             localIntents, mapping);
    if (failed(results))
      return failure();
    for (gfir::ResultOp result : plan.rules[index].resultUses)
      graph.map(result.getValue(), (*results)[result.getOrdinalAttr().getInt()]);
    for (unsigned state : plan.rules[index].writes) {
      candidates[state] = local[state];
      intents[state] = localIntents[state];
    }
  }
  return success();
}

LogicalResult ClockedMaterializer::bindObservations() {
  builder.setInsertionPoint(target.getBody().front().getTerminator());
  // Rule evaluation may follow dependencies. Terminal order instead follows
  // source registration, which owns the existing runner's observation order.
  for (auto [index, rule] : llvm::enumerate(plan.rules)) {
    if (rule.observations.empty()) continue;
    Operation *lowered = sampledRules[index];
    if (!lowered)
      return source.emitOpError("observation requires its materialized sampled Rule");
    auto admission = lowered->getAttrOfType<DictionaryAttr>("ac.sampled_admission");
    unsigned slot = admission.getAs<IntegerAttr>("fire_result").getInt() + 1;
    for (const auto &observation : rule.observations) {
      auto effect = observation.operation;
      SmallVector<Value> values;
      values.push_back(lowered->getResult(slot++));
      for (unsigned value = 0; value < effect.getValues().size(); ++value)
        values.push_back(lowered->getResult(slot++));
      auto *terminal = make(ac::SourceObserveOp::getOperationName(), values, {}, {
          builder.getNamedAttr("kind", effect.getKindAttr()),
          builder.getNamedAttr("spec", effect.getSpecAttr()),
          builder.getNamedAttr("ac.location", observation.location),
          builder.getNamedAttr("ac.observation_id", observation.id)});
      terminal->setLoc(effect.getLoc());
    }
  }
  return success();
}

LogicalResult ClockedMaterializer::bindStorage() {
  for (auto [index, memory] : llvm::enumerate(plan.memories)) {
    SmallVector<Value> inputs{clock, reset};
    if (!memory.transactional) {
      auto raw = memory.raw;
      for (Value pin : raw.getInputs())
        inputs.push_back(graph.lookup(pin));
    } else {
      auto pinLiteral = [&](Type type, unsigned value) {
        return make(ac::BitsConstantOp::getOperationName(), {}, {type}, {
            builder.getNamedAttr("value", literal(value))})->getResult(0);
      };
      Value pinZero = pinLiteral(bit, 0), pinOne = pinLiteral(bit, 1);
      Value addressZero = pinLiteral(bits(memory.addressWidth), 0);
      Value dataZero = pinLiteral(memory.element, 0);
      Value maskZero = pinLiteral(bits(memory.maskWidth), 0);
      auto select = [&](Value path, Value payload, Value idle) {
        return make(ac::BitsSelectOp::getOperationName(), {path, payload, idle},
                    {payload.getType()})->getResult(0);
      };
      if (memory.asynchronous()) {
        auto read = memoryIssues.find(memory.reader);
        inputs.push_back(read == memoryIssues.end() ? addressZero :
            select(read->second.path, read->second.arguments[0], addressZero));
      } else {
        for (unsigned port = 0; port < memory.readPorts(); ++port) {
          auto issue = memoryIssues.find(memory.requests[port]);
          if (issue == memoryIssues.end()) {
            inputs.append({pinZero, addressZero});
          } else {
            inputs.append({select(issue->second.path, pinOne, pinZero),
                           select(issue->second.path, issue->second.arguments[0], addressZero)});
          }
        }
      }
      auto writer = memoryIssues.find(memory.writer);
      if (writer == memoryIssues.end()) {
        inputs.append({pinZero, addressZero, dataZero, maskZero});
      } else {
        const auto &issue = writer->second;
        inputs.append({select(issue.path, pinOne, pinZero),
                       select(issue.path, issue.arguments[0], addressZero),
                       select(issue.path, issue.arguments[1], dataZero),
                       select(issue.path, issue.arguments[2], maskZero)});
      }
    }
    memories[index]->setOperands(inputs);
  }
  for (auto [index, fifo] : llvm::enumerate(plan.fifos)) {
    Value inputValid = fifo.producer ? grants[*fifo.producer] : none;
    Value inputData = fifoData[index];
    Value outputReady = fifo.consumer ? grants[*fifo.consumer] : none;
    auto rawInput = fifo.rawInput;
    auto rawOutput = fifo.rawOutput;
    if (rawInput) {
      inputValid = graph.lookup(rawInput.getInputs()[0]);
      inputData = graph.lookup(rawInput.getInputs()[1]);
    }
    if (rawOutput)
      outputReady = graph.lookup(rawOutput.getInputs()[0]);
    if (!inputData) {
      auto value = zero(fifo.element);
      if (failed(value))
        return failure();
      inputData = *value;
    }
    fifos[index]->setOperands({clock, reset, inputValid, inputData, outputReady});
  }
  for (auto [index, declaration] : llvm::enumerate(plan.variables)) {
    auto var = declaration;
    Value enable = none;
    for (auto [rule, definition] : llvm::enumerate(plan.rules))
      if (llvm::is_contained(definition.writes, index))
        enable = grants[rule];
    if (sampledEnables[index])
      enable = sampledEnables[index];
    else if (plan.conditionalWrites.contains(index))
      enable = binary("and", enable, intents[index]);
    Value data = candidates[index];
    if (definition.incomingState == index) {
      enable = incomingEnable;
      // Each incoming stage closes its proposal over the actual local old Q.
      // The whole Bits owner has the same literal-one leaf mask used by
      // materializeDomainCommit; keep AND(enable, mask) explicit for the
      // independent staged-commit proof. The raw D/E pair remains the declared
      // incoming route and the parent's exact sampled writer candidate.
      Value mask = make(ac::BitsConstantOp::getOperationName(), {}, {bit}, {
          builder.getNamedAttr("value", literal(1))})->getResult(0);
      data = merge(states[index]->getResult(0),
                   binary("and", incomingEnable, mask), incomingData);
    }
    SmallVector<Value> inputs{clock, reset, enable, data,
                              graph.lookup(var.getInitial())};
    if (auto table = dyn_cast<ac::TableType>(var.getInitial().getType()))
      for (unsigned control = 0; control < 3; ++control)
        inputs[control] = make(ac::TableSplatOp::getOperationName(), {inputs[control]},
            {ac::TableType::get(builder.getContext(), table.getShape(), bit)}, {
            builder.getNamedAttr("shape", table.getShape())})->getResult(0);
    states[index]->setOperands(inputs);
  }
  SmallVector<Value> outputs;
  for (Value value : cast<gfir::YieldOp>(source.getBody().front().getTerminator()).getValues())
    outputs.push_back(graph.lookup(value));
  for (const auto &output : definition.outputAvailability) {
    Value available = grants[output.rules.front()];
    for (unsigned rule : ArrayRef<unsigned>(output.rules).drop_front())
      available = binary("and", available, grants[rule]);
    outputs.push_back(available);
  }
  if (definition.incomingState)
    outputs.push_back(states[*definition.incomingState]->getResult(0));
  make(ac::YieldOp::getOperationName(), outputs, {});
  return success();
}

LogicalResult ClockedMaterializer::run() {
  bit = bits(1);
  if (!plan.variables.empty() && failed(addPrimitive("dffe")))
    return failure();
  for (const auto &fifo : plan.fifos)
    if (failed(addPrimitive(fifo.downstreamPop ? "fifo_downstream_pop" : "fifo_local")))
      return failure();
  for (const auto &memory : plan.memories)
    if (failed(addPrimitive(memory.kind)))
      return failure();
  if (failed(makeModule()) || failed(allocate()) || failed(cloneGraph()) ||
      failed(lowerRules()) || failed(bindGraph()) || failed(bindStorage()) ||
      failed(bindObservations()))
    return failure();
  bindTransactionWiring();
  return success();
}

} // namespace

LogicalResult
materializeStructuredClocked(const gfir::StructuredDefinition &definition,
                              const gfir::StructuredProgram &program,
                              mlir::ModuleOp package) {
  return ClockedMaterializer(definition, program, package).run();
}

} // namespace acir::detail
