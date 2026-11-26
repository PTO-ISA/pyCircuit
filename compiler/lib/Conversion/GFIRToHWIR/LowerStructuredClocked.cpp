#include "LowerStructuredClocked.h"
#include "LowerStructuredInputs.h"

#include "mlir/IR/Builders.h"
#include "mlir/IR/IRMapping.h"
#include "mlir/IR/SymbolTable.h"
#include "llvm/ADT/ScopeExit.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/MathExtras.h"
#include <functional>
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
  struct QueueProposal {
    unsigned fifo, control;
    Value rawPath;
    Value data = {};
    unsigned pathResult = 0;
  };

  struct FifoCapture {
    unsigned first;
    Value capacity, valid, data;
    std::optional<unsigned> external;
  };

  struct MemoryProposal {
    gfir::CallOp operation;
    gfir::StructuredMemoryUse use;
    Value rawPath;
    SmallVector<Value> arguments;
    Value reachedPrefix = {};
    unsigned pathResult = 0;
  };

  struct EffectSnapshot {
    SmallVector<Value> values;
    Value path;
    DictionaryAttr pathIdentity;
  };

  struct SampledContext {
    DictionaryAttr registration;
    ArrayRef<gfir::StructuredCheckSite> plannedChecks;
    SmallVector<Attribute> invocation, controls, checks;
    SmallVector<Value> predicates, reaches;
    SmallVector<Attribute> requirements, resources;
    SmallVector<QueueProposal> queueProposals;
    llvm::DenseMap<unsigned, FifoCapture> fifoOutputs;
    llvm::DenseMap<unsigned, Value> queueDemands;
    SmallVector<MemoryProposal> memoryProposals;
    llvm::DenseMap<unsigned, SmallVector<Value>> memoryOutputs;
    llvm::DenseMap<unsigned, Value> guardAvailability;
    llvm::DenseMap<Operation *, EffectSnapshot> effects;
    SmallVector<Value> rawAncestors;
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
  FailureOr<Value> closeIncoming(Value previous, Value candidate, Value enable, Value mask);
  Value known(Value value, bool high);
  Value rawStatementPath();
  FailureOr<unsigned> addControl(StringRef kind, Operation *site, Value raw);
  LogicalResult lowerFifo(gfir::CallOp call,
                         const gfir::StructuredFifoUse &use,
                         IRMapping &mapping);
  LogicalResult lowerMemory(gfir::CallOp call,
                            const gfir::StructuredMemoryUse &use,
                            IRMapping &mapping);
  DictionaryAttr localResourceOwner();
  DictionaryAttr localQueueID(unsigned fifo);
  LogicalResult lowerSampled(unsigned index);
  LogicalResult bindTransactionWiring();
  void bindQueueWiring();
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
  struct MethodPlace {
    unsigned slot;
    const gfir::StructuredWritePath *projection = nullptr;
    Value index;
  };
  FailureOr<SmallVector<Value>> lowerChildMethod(
      gfir::CallOp call, const gfir::StructuredChildMethod &invocation, unsigned methodIndex,
      SmallVector<Value> &local, SmallVector<Value> &localIntents, IRMapping &mapping,
      ArrayRef<std::pair<unsigned, MethodPlace>> forwarded = {});
  DictionaryAttr localStateID(Operation *storage);
  FailureOr<Value> availability(const gfir::StructuredAvailability &sources);
  FailureOr<Value> projectField(Value value, ArrayRef<StringAttr> path);

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
  SmallVector<Value> incomingData, incomingEnable, incomingQ;
  FailureOr<DictionaryAttr> routeStateID(const gfir::StructuredStateRoute &route);
  std::optional<unsigned> incomingLocal(unsigned state) const;
  struct ChildRoute {
    Value q, data, enable;
  };
  SmallVector<ChildRoute> childRoutes;
  struct ChildInput { Value data, enable; std::optional<unsigned> candidate; };
  llvm::DenseMap<Operation *, SmallVector<ChildInput>> childInputs;
  llvm::DenseMap<unsigned, unsigned> childSlots;
  llvm::DenseMap<std::pair<unsigned, unsigned>, Value> childCompanions;
  llvm::DenseMap<std::pair<unsigned, unsigned>, Value> childFieldCompanions;
  IRMapping graph;
  SmallVector<Operation *> fifos, memories, states;
  SmallVector<Value> grants, candidates, intents, fifoData;
  SmallVector<Value> fifoPopPaths, fifoPushPaths;
  SmallVector<Value> fifoIncomingPops, fifoPopUnions;
  SmallVector<unsigned> fifoIncomingOrder;
  SmallVector<Attribute> queueWriters, externalPops;
  llvm::DenseMap<unsigned, Value> selectAvailability;
  llvm::DenseMap<unsigned, Value> inputCompanions;
  llvm::DenseMap<unsigned, Value> fieldInputCompanions;
  llvm::DenseMap<std::pair<Operation *, unsigned>, Value> pureCompanions;
  bool sampledQueues = false;
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

// Struct fields close separately. An incoming Table route carries the whole value.
FailureOr<Value> ClockedMaterializer::closeIncoming(
    Value previous, Value candidate, Value enable, Value mask) {
  Type type = previous.getType();
  if (auto structure = dyn_cast<ac::StructType>(type)) {
    auto declaration = hardware.lookupStruct(structure);
    SmallVector<Value> fields;
    for (Attribute raw : declaration.getFields()) {
      auto field = cast<DictionaryAttr>(raw);
      auto name = field.getAs<StringAttr>("name");
      auto fieldType = field.getAs<TypeAttr>("type").getValue();
      auto project = [&](Value value) {
        return make(ac::StructGetOp::getOperationName(), {value}, {fieldType}, {
            builder.getNamedAttr("field", name)})->getResult(0);
      };
      auto next = closeIncoming(project(previous), project(candidate), enable, mask);
      if (failed(next)) return failure();
      fields.push_back(*next);
    }
    return make(ac::StructCreateOp::getOperationName(), fields, {type})->getResult(0);
  }
  return merge(previous, binary("and", enable, mask), candidate);
}

Value ClockedMaterializer::known(Value value, bool high) {
  return make(ac::BitsCompareOp::getOperationName(), {value, high ? one : none},
              {bit}, {builder.getNamedAttr("predicate", builder.getStringAttr("case_eq"))})
      ->getResult(0);
}

Value ClockedMaterializer::rawStatementPath() {
  Value path = one;
  // The canonical resource witness walks from the innermost source arm to
  // its ancestors, then appends the local authored path (one in this profile).
  for (Value ancestor : llvm::reverse(sampled->rawAncestors))
    path = binary("and", path, ancestor);
  return binary("and", path, one);
}

FailureOr<unsigned> ClockedMaterializer::addControl(StringRef kind, Operation *site,
                                                    Value raw) {
  unsigned ordinal = sampled->controls.size();
  if (ordinal >= sampled->plannedChecks.size() ||
      sampled->plannedChecks[ordinal].operation != site)
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

DictionaryAttr ClockedMaterializer::localResourceOwner() {
  return builder.getDictionaryAttr({
      builder.getNamedAttr("instance_path", builder.getArrayAttr({}))});
}

DictionaryAttr ClockedMaterializer::localQueueID(unsigned fifo) {
  return builder.getDictionaryAttr({
      builder.getNamedAttr("owner", localResourceOwner()),
      builder.getNamedAttr("declaration", fifos[fifo]->getAttr("occurrence"))});
}

LogicalResult ClockedMaterializer::lowerFifo(
    gfir::CallOp call, const gfir::StructuredFifoUse &use,
    IRMapping &mapping) {
  if (use.access != gfir::StructuredFifoAccess::Out &&
      use.access != gfir::StructuredFifoAccess::View &&
      use.access != gfir::StructuredFifoAccess::In)
    return call.emitOpError("sampled FIFO refinement requires ordered in, out or view");
  auto captured = sampled->fifoOutputs.find(use.fifo);
  if (captured == sampled->fifoOutputs.end())
    return call.emitOpError("missing committed FIFO response captures");
  Value rawPath = rawStatementPath();
  const auto &capture = captured->second;
  bool push = use.access == gfir::StructuredFifoAccess::In;
  Value predicate = push ? capture.capacity : capture.valid;
  NamedAttrList prospective;
  if (push) {
    prospective.append("capacity", builder.getI64IntegerAttr(capture.first));
    if (plan.fifos[use.fifo].downstreamPop) {
      if (!capture.external)
        return call.emitOpError("downstream FIFO in requires its actual raw pop capture");
      Value external = builder.getBlock()->getArgument(*capture.external);
      SmallVector<Attribute> priorPops;
      Value credit;
      for (auto [ordinal, proposal] : llvm::enumerate(sampled->queueProposals)) {
        if (proposal.fifo != use.fifo || proposal.data)
          continue;
        priorPops.push_back(builder.getI64IntegerAttr(ordinal));
        credit = credit ? binary("or", credit, proposal.rawPath) : proposal.rawPath;
      }
      credit = credit ? binary("or", credit, external) : external;
      predicate = binary("or", predicate, binary("and", capture.valid, credit));
      prospective.append("valid", builder.getI64IntegerAttr(capture.first + 1));
      prospective.append("external_pop", builder.getI64IntegerAttr(*capture.external));
      prospective.append("prior_pops", builder.getArrayAttr(priorPops));
    }
  }
  Value selected = make(ac::BitsSelectOp::getOperationName(),
                       {rawPath, predicate, one}, {bit})->getResult(0);
  auto control = addControl("resource_available", call, selected);
  if (failed(control))
    return failure();
  const auto &check = sampled->plannedChecks[*control];
  NamedAttrList resource({
      builder.getNamedAttr("control", builder.getI64IntegerAttr(*control)),
      builder.getNamedAttr("kind", builder.getStringAttr(push ? "queue_ready" : "queue_valid")),
      builder.getNamedAttr("owner", localResourceOwner()),
      builder.getNamedAttr("declaration", fifos[use.fifo]->getAttr("occurrence")),
      builder.getNamedAttr("occurrence", check.id.get("check")),
      builder.getNamedAttr("location", check.location)});
  if (push)
    resource.append("prospective", prospective.getDictionary(builder.getContext()));
  sampled->resources.push_back(resource.getDictionary(builder.getContext()));
  sampled->queueDemands.try_emplace(*control, predicate);
  if (push)
    sampled->queueProposals.push_back({use.fifo, *control, rawPath,
                                      mapping.lookup(call.getInputs().front())});
  else {
    if (use.access == gfir::StructuredFifoAccess::Out)
      sampled->queueProposals.push_back({use.fifo, *control, rawPath});
    mapping.map(call.getOutputs().front(), capture.data);
  }
  return success();
}

LogicalResult ClockedMaterializer::lowerMemory(
    gfir::CallOp call, const gfir::StructuredMemoryUse &use,
    IRMapping &mapping) {
  if (!sampled)
    return call.emitOpError("transaction Memory requires sampled Rule refinement");
  Value rawPath = rawStatementPath();
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
        builder.getNamedAttr("owner", localResourceOwner()),
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
  StructuredInputLayout inputLayout(definition, builder);
  auto inputs = std::move(inputLayout.types);
  auto names = std::move(inputLayout.names);
  inputs.append({bit, bit});
  unsigned sourceInputs = inputs.size();
  for (const auto &incoming : definition.incomingStates) {
    const auto &route = incoming.route;
    auto variable = program.definitions[route.definition].clocked->variables[route.state];
    if (incoming.writable) inputs.append({variable.getInitial().getType(), bit});
  }
  SmallVector<Type> outputs(source.getFunctionType().getResults());
  SmallVector<Attribute> outputNames(source.getOutputNames().getValue());
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
    state.addAttribute("ac.output_validity", builder.getArrayAttr(validity));
  }
  SmallVector<Attribute> routedValidity;
  for (const auto &field : definition.fieldOutputs) {
    unsigned payload = outputs.size();
    const auto &output = definition.outputs[field.output];
    outputs.push_back(definition.wires[output.wire].type);
    outputNames.push_back(field.name);
    if (!field.sources.empty()) {
      routedValidity.push_back(builder.getDictionaryAttr({
          builder.getNamedAttr("payload", builder.getI64IntegerAttr(payload)),
          builder.getNamedAttr("validity", builder.getI64IntegerAttr(outputs.size()))}));
      outputs.push_back(bit);
      outputNames.push_back(field.availabilityName);
    }
  }
  if (!routedValidity.empty())
    state.addAttribute("ac.routed_output_validity", builder.getArrayAttr(routedValidity));
  for (const auto &incoming : definition.incomingStates) {
    const auto &route = incoming.route;
    auto variable = program.definitions[route.definition].clocked->variables[route.state];
    outputs.push_back(variable.getInitial().getType());
    outputNames.push_back(privateName("__ac_state_q"));
    if (incoming.writable) {
      names.push_back(privateName("__ac_state_data"));
      names.push_back(privateName("__ac_state_enable"));
    }
  }
  // Keep incoming demand order identical to the actual sampled Rule/resource
  // inventory. These root-default-zero ports follow the existing state suffix.
  fifoIncomingPops.resize(plan.fifos.size());
  fifoPopUnions.resize(plan.fifos.size());
  SmallVector<std::pair<unsigned, unsigned>> fifoInputs;
  for (unsigned node : definition.order) {
    if (node < definition.wires.size() || node >= definition.firstClockedCallNode())
      continue;
    auto rule = plan.rules[node - definition.wires.size()].operation;
    rule.walk([&](gfir::CallOp call) {
      auto use = plan.fifoCalls.find(call);
      if (use == plan.fifoCalls.end() ||
          use->second.access != gfir::StructuredFifoAccess::In ||
          !plan.fifos[use->second.fifo].localPopCredit)
        return;
      unsigned fifo = use->second.fifo;
      fifoIncomingOrder.push_back(fifo);
      fifoInputs.emplace_back(fifo, inputs.size());
      inputs.push_back(bit);
      names.push_back(privateName("__ac_queue_pop_" + std::to_string(fifo)));
    });
  }
  SmallVector<std::pair<unsigned, unsigned>> availabilityInputs;
  SmallVector<Attribute> inputValidity(sourceInputs, builder.getI64IntegerAttr(-1));
  for (auto [ordinal, input] : llvm::enumerate(definition.inputAvailability)) {
    inputValidity[inputLayout.companionPayloads[ordinal]] = builder.getI64IntegerAttr(inputs.size());
    availabilityInputs.emplace_back(ordinal, inputs.size());
    inputs.push_back(bit);
    names.push_back(input.name);
  }
  if (!availabilityInputs.empty())
    state.addAttribute("ac.input_validity", builder.getArrayAttr(inputValidity));
  if (!definition.incomingStates.empty() || !definition.outputAvailability.empty() ||
      !definition.fieldOutputs.empty() || !fifoInputs.empty() || !availabilityInputs.empty()) {
    SmallVector<Attribute> defaults(inputs.size() - sourceInputs,
                                    builder.getI64IntegerAttr(0));
    for (auto [port, input] : availabilityInputs)
      defaults[input - sourceInputs] = builder.getI64IntegerAttr(1);
    state.addAttribute("ac.internal_ports", builder.getDictionaryAttr({
        builder.getNamedAttr("source_inputs", builder.getI64IntegerAttr(sourceInputs)),
        builder.getNamedAttr("source_outputs", builder.getI64IntegerAttr(source.getFunctionType().getNumResults())),
        builder.getNamedAttr("root_inputs", builder.getArrayAttr(defaults))}));
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
  builder.setInsertionPointToEnd(body);
  if (failed(inputLayout.mapInputs(*body, builder, hardware, graph)))
    return failure();
  clock = body->getArgument(sourceInputs - 2);
  reset = body->getArgument(sourceInputs - 1);
  incomingQ.resize(definition.incomingStates.size());
  incomingData.resize(definition.incomingStates.size());
  incomingEnable.resize(definition.incomingStates.size());
  unsigned nextIncoming = sourceInputs;
  for (unsigned index = 0; index < definition.incomingStates.size(); ++index)
    if (definition.incomingStates[index].writable) {
      incomingData[index] = body->getArgument(nextIncoming++);
      incomingEnable[index] = body->getArgument(nextIncoming++);
    }
  for (auto [fifo, input] : fifoInputs)
    fifoIncomingPops[fifo] = body->getArgument(input);
  for (auto [ordinal, input] : availabilityInputs) {
    const auto &endpoint = definition.inputAvailability[ordinal];
    if (endpoint.path.empty()) inputCompanions.try_emplace(endpoint.port, body->getArgument(input));
    else fieldInputCompanions.try_emplace(ordinal, body->getArgument(input));
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
  fifoPopPaths.resize(fifos.size());
  fifoPushPaths.resize(fifos.size());
  sampledEnables.resize(states.size());
  return success();
}

LogicalResult ClockedMaterializer::cloneGraph() {
  childRoutes.resize(definition.childStates.size());
  llvm::DenseMap<Operation *, const gfir::StructuredClockedCall *> plannedCalls;
  for (const auto &call : definition.clockedCalls)
    plannedCalls.try_emplace(call.operation, &call);
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
    gfir::InstanceOp routedInstance;
    const gfir::StructuredClockedCall *planned = nullptr;
    const gfir::StructuredDefinition *pureChild = nullptr;
    if (auto call = dyn_cast<gfir::CallOp>(op)) {
      gfir::InstanceOp instance;
      for (auto candidate : source.getBody().front().getOps<gfir::InstanceOp>())
        if (candidate.getSymName() == call.getReceiverAttr().getRootReference().getValue())
          instance = candidate;
      routedInstance = instance;
      state.name = OperationName(ac::InstanceOp::getOperationName(), builder.getContext());
      state.addAttribute("instance_name", instance.getSymNameAttr());
      state.addAttribute("callee", instance.getCalleeAttr());
      state.addAttribute("parameters", builder.getArrayAttr({}));
      state.addAttribute("type_arguments", builder.getArrayAttr({}));
      state.addAttribute("occurrence", occurrence("child", graphOperations.size()));
      planned = plannedCalls.lookup(&op);
      if (!planned) {
        for (const auto &child : program.definitions) {
          auto module = child.module;
          if (module.getSymName() == instance.getCalleeAttr().getValue()) {
            pureChild = &child;
            break;
          }
        }
        if (pureChild)
          for (const auto &output : pureChild->outputAvailability)
            state.addTypes(bit);
      }
      if (planned)
        for (unsigned i = 0; i < program.definitions[planned->definition].outputAvailability.size(); ++i)
          state.addTypes(bit);
      if (planned) {
        const auto &child = program.definitions[planned->definition];
        for (const auto &field : child.fieldOutputs) {
          state.addTypes(child.wires[child.outputs[field.output].wire].type);
          if (!field.sources.empty()) state.addTypes(bit);
        }
      }
      if (planned)
        for (const auto &incoming : program.definitions[planned->definition].incomingStates) {
          const auto &route = incoming.route;
          auto variable = program.definitions[route.definition].clocked->variables[route.state];
          state.addTypes(variable.getInitial().getType());
        }
    } else {
      state.addAttributes(op.getAttrs());
    }
    for (unsigned index = 0; index < op.getNumRegions(); ++index)
      state.addRegion();
    auto *copy = builder.create(state);
    if (pureChild) {
      unsigned result = op.getNumResults();
      for (const auto &output : pureChild->outputAvailability)
        pureCompanions.try_emplace(std::make_pair(&op, output.port), copy->getResult(result++));
    }
    llvm::DenseMap<unsigned, Value> reassembled;
    if (planned) {
      unsigned call = planned - definition.clockedCalls.data();
      unsigned result = op.getNumResults();
      const auto &child = program.definitions[planned->definition];
      for (const auto &output : child.outputAvailability)
        childCompanions.try_emplace(std::make_pair(call, output.port), copy->getResult(result++));
      llvm::DenseMap<std::pair<unsigned, ArrayAttr>, Value> leaves;
      SmallVector<unsigned> ports;
      for (auto [ordinal, field] : llvm::enumerate(child.fieldOutputs)) {
        const auto &output = child.outputs[field.output];
        SmallVector<Attribute> path(output.port.path.begin(), output.port.path.end());
        leaves.try_emplace(std::make_pair(output.port.port, builder.getArrayAttr(path)),
                           copy->getResult(result++));
        if (ports.empty() || ports.back() != output.port.port)
          ports.push_back(output.port.port);
        if (!field.sources.empty())
          childFieldCompanions.try_emplace(std::make_pair(call, ordinal), copy->getResult(result++));
      }
      if (!ports.empty()) {
        std::function<FailureOr<Value>(Type, unsigned, SmallVector<Attribute> &)> rebuild =
            [&](Type type, unsigned port, SmallVector<Attribute> &path) -> FailureOr<Value> {
          if (auto structure = dyn_cast<ac::StructType>(type)) {
            auto declaration = hardware.lookupStruct(structure);
            SmallVector<Value> fields;
            for (Attribute raw : declaration.getFields()) {
              auto field = cast<DictionaryAttr>(raw);
              path.push_back(field.getAs<StringAttr>("name"));
              auto value = rebuild(field.getAs<TypeAttr>("type").getValue(), port, path);
              path.pop_back();
              if (failed(value)) return failure();
              fields.push_back(*value);
            }
            return make(ac::StructCreateOp::getOperationName(), fields, {type})->getResult(0);
          }
          Value value = leaves.lookup({port, builder.getArrayAttr(path)});
          if (!value) return op.emitOpError("missing verified private child field payload");
          return value;
        };
        for (unsigned port : ports) {
          SmallVector<Attribute> path;
          auto value = rebuild(op.getResult(port).getType(), port, path);
          if (failed(value)) return failure();
          reassembled.try_emplace(port, *value);
        }
      }
    }
    if (planned) {
      const auto &routes = program.definitions[planned->definition].incomingStates;
      unsigned firstQ = copy->getNumResults() - routes.size();
      Value disabled, wholeOwnerMask;
      for (auto [ordinal, suffix] : llvm::enumerate(routes)) {
        auto route = suffix.route;
        route.path.insert(route.path.begin(), routedInstance);
        Value q = copy->getResult(firstQ + ordinal);
        if (suffix.writable && !disabled)
          disabled = make(ac::BitsConstantOp::getOperationName(), {}, {bit}, {
              builder.getNamedAttr("value", literal(0))})->getResult(0);
        // Unaffected instances send their old Q and literal-zero intent.
        // This also covers composite payloads without synthesizing a value.
        ChildInput input{q, disabled, {}};
        for (auto [index, candidate] : llvm::enumerate(definition.childStates))
          if (candidate.route == route) {
            childRoutes[index] = ChildRoute{q, q, disabled};
            if (candidate.writer) input.candidate = index;
          }
        for (auto [index, incoming] : llvm::enumerate(definition.incomingStates))
          if (incoming.route == route) {
            incomingQ[index] = q;
            if (incoming.writable) {
              input.enable = incomingEnable[index];
              if (!wholeOwnerMask)
                wholeOwnerMask = make(ac::BitsConstantOp::getOperationName(), {}, {bit}, {
                    builder.getNamedAttr("value", literal(1))})->getResult(0);
              auto closed = closeIncoming(q, incomingData[index], input.enable, wholeOwnerMask);
              if (failed(closed))
                return failure();
              input.data = *closed;
            }
          }
        if (suffix.writable) childInputs[copy].push_back(input);
      }
    }
    graphOperations.push_back({&op, copy});
    for (auto [ordinal, from] : llvm::enumerate(op.getResults())) {
      Value value = reassembled.lookup(ordinal);
      graph.map(from, value ? value : copy->getResult(ordinal));
    }
  }
  return success();
}

LogicalResult ClockedMaterializer::bindGraph() {
  for (auto [original, copy] : graphOperations) {
    SmallVector<Value> operands;
    const gfir::StructuredDefinition *logicalChild = nullptr;
    if (isa<gfir::CallOp>(original)) {
      auto callee = copy->getAttrOfType<FlatSymbolRefAttr>("callee");
      auto child = llvm::find_if(program.definitions, [&](const auto &child) {
        auto module = child.module;
        return module.getSymName() == callee.getValue();
      });
      if (child == program.definitions.end())
        return original->emitOpError("missing verified callee definition");
      logicalChild = &*child;
    }
    for (auto [port, operand] : llvm::enumerate(original->getOperands())) {
      auto value = graph.lookupOrNull(operand);
      if (!value)
        return original->emitOpError("missing verified graph value in clocked refinement");
      bool flattened = logicalChild && llvm::any_of(logicalChild->inputAvailability, [&](const auto &input) {
        return input.port == port && !input.path.empty();
      });
      if (flattened) {
        for (const auto &input : logicalChild->inputAvailability) {
          if (input.port != port) continue;
          auto field = projectField(value, input.path);
          if (failed(field)) return failure();
          operands.push_back(*field);
        }
      } else operands.push_back(value);
    }
    if (logicalChild) {
      auto domains = logicalChild->module->getAttrOfType<ArrayAttr>("domains");
      if (domains && !domains.empty())
        operands.append({clock, reset});
      for (const auto &input : childInputs[copy]) {
        if (input.candidate) {
          const auto &child = childRoutes[*input.candidate];
          operands.append({child.data, child.enable});
        } else {
          operands.append({input.data, input.enable});
        }
      }
      for (const auto &input : logicalChild->inputAvailability) {
        auto wires = definition.valueWires.find(original->getOperand(input.port));
        if (wires == definition.valueWires.end())
          return original->emitOpError("missing verified input wires");
        const auto &leaves = wires->second;
        auto leaf = llvm::find_if(leaves, [&](unsigned index) {
          return definition.wires[index].endpoint.path == input.path;
        });
        if (leaf == leaves.end())
          return original->emitOpError("missing verified input companion field");
        const auto &sources = definition.wireAvailability.empty()
            ? gfir::StructuredAvailability{} : definition.wireAvailability[*leaf];
        auto available = availability(sources);
        if (failed(available))
          return failure();
        operands.push_back(*available);
      }
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

FailureOr<SmallVector<Value>> ClockedMaterializer::lowerChildMethod(
    gfir::CallOp call, const gfir::StructuredChildMethod &invocation, unsigned methodIndex,
    SmallVector<Value> &local, SmallVector<Value> &localIntents, IRMapping &mapping,
    ArrayRef<std::pair<unsigned, MethodPlace>> forwarded) {
  const auto &child = *program.definitions[invocation.definition].clocked;
  auto method = child.methods[methodIndex];
  auto boundPlace = [&](unsigned ordinal) -> std::optional<MethodPlace> {
    if (!call.getBindingsAttr())
      return std::nullopt;
    if (!forwarded.empty()) {
      for (const auto &[formal, place] : forwarded)
        if (formal == ordinal)
          return place;
      return std::nullopt;
    }
    for (const auto &bound : invocation.formals) {
      if (bound.formal != ordinal)
        continue;
      return MethodPlace{bound.local ? bound.slot : childSlots.lookup(bound.slot),
                         bound.projection ? &*bound.projection : nullptr,
                         bound.indexOperand ? mapping.lookup(call.getInputs()[*bound.indexOperand])
                                            : Value{}};
    }
    return std::nullopt;
  };
  auto statePlace = [&](Operation *op) -> std::optional<MethodPlace> {
    auto formal = child.formalAccesses.find(op);
    if (formal != child.formalAccesses.end())
      return boundPlace(formal->second);
    if (invocation.path.empty())
      return MethodPlace{child.stateAccesses.lookup(op), nullptr, {}};
    for (auto [ordinal, receiver] : invocation.states)
      if (ordinal == child.stateAccesses.lookup(op))
        return MethodPlace{childSlots.lookup(receiver), nullptr, {}};
    return std::nullopt;
  };
  auto project = [&](Value value, const gfir::StructuredWritePath *path) {
    if (path)
      for (auto [field, type] : llvm::zip(path->fields, path->types))
        value = make(ac::StructGetOp::getOperationName(), {value}, {type}, {
            builder.getNamedAttr("field", field)})->getResult(0);
    return value;
  };
  auto readPlace = [&](const MethodPlace &place) {
    Value value = project(local[place.slot], place.projection);
    if (place.index)
      value = make(ac::TableGetOp::getOperationName(), {value, place.index},
                   {cast<ac::TableType>(value.getType()).getElementType(), bit})->getResult(0);
    return value;
  };
  IRMapping nested;
  unsigned supplied = 0;
  for (BlockArgument argument : method.getBody().front().getArguments()) {
    auto place = boundPlace(argument.getArgNumber());
    nested.map(argument, place ? readPlace(*place) : mapping.lookup(call.getInputs()[supplied++]));
  }
  for (Operation &op : method.getBody().front()) {
    if (auto yield = dyn_cast<gfir::YieldOp>(op)) {
      SmallVector<Value> results;
      for (Value value : yield.getValues())
        results.push_back(nested.lookup(value));
      return results;
    }
    if (auto read = dyn_cast<gfir::ReadOp>(op)) {
      auto place = statePlace(read);
      if (!place)
        return read.emitOpError("missing verified child state slot");
      nested.map(read.getValue(), readPlace(*place));
    } else if (auto write = dyn_cast<gfir::WriteOp>(op)) {
      auto place = statePlace(write);
      if (!place)
        return write.emitOpError("missing verified child state slot");
      if (auto path = child.writePaths.find(write); path != child.writePaths.end())
        place->projection = &path->second;
      if (write.getIndex())
        place->index = nested.lookup(write.getIndex());
      auto &current = local[place->slot];
      Value value = nested.lookup(write.getValue());
      if (place->index) {
        auto next = indexedWrite(project(current, place->projection), place->index, value);
        if (failed(next))
          return failure();
        value = *next;
      }
      if (place->projection)
        value = make(ac::ValueMergeOp::getOperationName(), {current, one, value},
            {current.getType(), bit}, {
            builder.getNamedAttr("paths", builder.getArrayAttr({place->projection->fields})),
            builder.getNamedAttr("operandSegmentSizes", builder.getDenseI32ArrayAttr({1, 1, 1}))})->getResult(0);
      current = value;
      localIntents[place->slot] = one;
    } else if (auto invoke = dyn_cast<gfir::CallOp>(op)) {
      SmallVector<std::pair<unsigned, MethodPlace>> nextBindings;
      if (auto forwarding = child.forwardedFormals.find(invoke);
          forwarding != child.forwardedFormals.end()) {
        for (auto [formal, actual] : forwarding->second) {
          auto place = boundPlace(actual);
          if (!place)
            return invoke.emitOpError("missing verified forwarded formal slot");
          nextBindings.emplace_back(formal, *place);
        }
      }
      auto results = lowerChildMethod(invoke, invocation, child.methodCalls.lookup(invoke),
                                      local, localIntents, nested, nextBindings);
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

std::optional<unsigned> ClockedMaterializer::incomingLocal(unsigned state) const {
  for (auto [index, incoming] : llvm::enumerate(definition.incomingStates))
    if (incoming.writable && incoming.route.path.empty() && incoming.route.state == state)
      return index;
  return {};
}

FailureOr<DictionaryAttr>
ClockedMaterializer::routeStateID(const gfir::StructuredStateRoute &route) {
  ac::ModuleOp owner = target;
  SmallVector<Attribute> path;
  for (auto original : route.path) {
    ac::InstanceOp selected;
    for (auto instance : owner.getBody().front().getOps<ac::InstanceOp>())
      if (instance.getInstanceName() == original.getSymName()) selected = instance;
    if (!selected)
      return original.emitOpError("state route requires an allocated instance");
    path.push_back(selected->getAttr("occurrence"));
    owner = dyn_cast_or_null<ac::ModuleOp>(
        SymbolTable::lookupSymbolIn(package, selected.getCalleeAttr()));
    if (!owner)
      return original.emitOpError("state route requires a lowered Module");
  }
  auto variable = program.definitions[route.definition].clocked->variables[route.state];
  Operation *storage = nullptr;
  for (auto &allocation : owner.getBody().front())
    if (isa<ac::InstanceOp, ac::CollectionOp>(allocation) &&
        allocation.getAttrOfType<StringAttr>("instance_name") == variable.getSymNameAttr())
      storage = &allocation;
  if (!storage)
    return variable.emitOpError("state route requires allocated storage");
  NamedAttrList id(localStateID(storage));
  id.set("owner", builder.getDictionaryAttr({builder.getNamedAttr("instance_path", builder.getArrayAttr(path))}));
  return id.getDictionary(builder.getContext());
}

FailureOr<Value> ClockedMaterializer::availability(
    const gfir::StructuredAvailability &sources) {
  Value result;
  for (const auto &source : sources) {
    Value value;
    switch (source.kind) {
    case gfir::StructuredAvailabilitySource::Kind::FieldInput:
      value = fieldInputCompanions.lookup(source.index); break;
    case gfir::StructuredAvailabilitySource::Kind::PureOutput: {
      const auto &wire = definition.wires[source.index];
      const auto &child = program.definitions[wire.call->definition];
      unsigned port = child.outputs[wire.call->output].port.port;
      value = pureCompanions.lookup({wire.endpoint.value.getDefiningOp(), port});
      break;
    }
    case gfir::StructuredAvailabilitySource::Kind::Input:
      value = inputCompanions.lookup(source.index); break;
    case gfir::StructuredAvailabilitySource::Kind::Rule:
      value = grants[source.index]; break;
    case gfir::StructuredAvailabilitySource::Kind::ChildOutput:
      value = childCompanions.lookup({source.index, source.port}); break;
    case gfir::StructuredAvailabilitySource::Kind::ChildField:
      value = childFieldCompanions.lookup({source.index, source.port}); break;
    case gfir::StructuredAvailabilitySource::Kind::Select: {
      if (auto cached = selectAvailability.find(source.index); cached != selectAvailability.end()) {
        value = cached->second;
        break;
      }
      const auto &select = definition.availabilitySelects[source.index];
      const auto &endpoint = definition.wires[select.selector].endpoint;
      auto condition = projectField(graph.lookup(endpoint.value), endpoint.path);
      auto yes = availability(select.yes), no = availability(select.no);
      if (failed(condition) || failed(yes) || failed(no))
        return failure();
      value = make(ac::BitsSelectOp::getOperationName(), {*condition, *yes, *no}, {bit})->getResult(0);
      selectAvailability.try_emplace(source.index, value);
      break;
    }
    }
    if (!value)
      return this->source.emitOpError("missing verified availability source");
    result = result ? binary("and", result, value) : value;
  }
  return result ? result : one;
}

FailureOr<Value> ClockedMaterializer::projectField(Value value, ArrayRef<StringAttr> path) {
  for (auto name : path) {
    auto declaration = hardware.lookupStruct(cast<ac::StructType>(value.getType()));
    auto field = llvm::find_if(declaration.getFields(), [&](Attribute raw) {
      return cast<DictionaryAttr>(raw).getAs<StringAttr>("name") == name;
    });
    if (field == declaration.getFields().end())
      return source.emitOpError("missing verified output field layout");
    Type type = cast<DictionaryAttr>(*field).getAs<TypeAttr>("type").getValue();
    value = make(ac::StructGetOp::getOperationName(), {value}, {type}, {
        builder.getNamedAttr("field", name)})->getResult(0);
  }
  return value;
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
    if (sampled) {
      if (auto uses = plan.dataCaptureUses.find(&op); uses != plan.dataCaptureUses.end()) {
        for (const auto &use : uses->second) {
          unsigned control = sampled->controls.size();
          if (control >= sampled->plannedChecks.size())
            return op.emitOpError("missing required data-use obligation");
          const auto &check = sampled->plannedChecks[control];
          Value payload = isa<gfir::WriteOp>(op)
              ? cast<gfir::WriteOp>(op).getValue() : op.getOperand(use.operand);
          Value available;
          for (unsigned capture : use.captures) {
            Value next = sampled->guardAvailability.lookup(capture);
            if (!next) return op.emitOpError("missing required data-use availability");
            available = available ? binary("and", available, next) : next;
          }
          if (!available)
            return op.emitOpError("missing required data-use availability");
          Value required = make(ac::ValueRequireOp::getOperationName(),
              {mapping.lookup(payload), available, sampled->path ? sampled->path : one}, {bit}, {
              builder.getNamedAttr("occurrence", check.id.get("check")),
              builder.getNamedAttr("source_location", check.location)})->getResult(0);
          if (failed(addControl("value_available", &op, required)))
            return failure();
          sampled->requirements.push_back(builder.getDictionaryAttr({
              builder.getNamedAttr("control", builder.getI64IntegerAttr(control)),
              builder.getNamedAttr("kind", builder.getStringAttr(use.kind == gfir::StructuredDataCaptureUse::Kind::Return ? "return" :
                  use.kind == gfir::StructuredDataCaptureUse::Kind::Observation ? "log" : "state_write")),
              builder.getNamedAttr("operand", builder.getI64IntegerAttr(use.operand)),
              builder.getNamedAttr("occurrence", check.id.get("check")),
              builder.getNamedAttr("location", check.location)}));
        }
      }
    }
    if (isa<gfir::YieldOp, gfir::StepOp>(op)) {
      SmallVector<Value> values;
      ValueRange returned = isa<gfir::StepOp>(op)
          ? ValueRange(cast<gfir::StepOp>(op).getValues()) : ValueRange(op.getOperands());
      for (Value value : returned)
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
      auto step = cast<gfir::StepOp>(body.getTerminator());
      uint64_t count = loop.getTripCountAttr().getInt();
      Value continuation = one;
      Value parentPath;
      DictionaryAttr parentPathIdentity;
      if (step.getBreakIf() && count) {
        if (!sampled)
          return loop.emitOpError("break_if requires sampled Rule refinement");
        parentPath = sampled->path;
        parentPathIdentity = sampled->pathIdentity;
      }
      for (uint64_t iteration = 0; iteration < count; ++iteration) {
        // Every iteration starts with the enclosing immutable SSA bindings.
        // Candidates and write intent belong to the same transaction and flow
        // through the complete sequence of iterations.
        IRMapping nested(mapping);
        for (auto [argument, value] :
             llvm::zip(body.getArguments().drop_front(), carried))
          nested.map(argument, value);
        SmallVector<Value> previous;
        if (step.getBreakIf() && iteration)
          previous.assign(local.begin(), local.end());
        auto next = lowerBody(body, {}, local, localIntents, nested, iteration);
        if (failed(next))
          return failure();
        if (step.getBreakIf()) {
          // Later unrolled bodies still have immutable SSA definitions, but
          // only iterations reached before a stop may replace candidates or
          // carried values. Every body write is direct in this bounded profile,
          // so its intent is already present after the first reached iteration.
          if (iteration) {
            for (unsigned state = 0; state < local.size(); ++state)
              if (local[state] && local[state] != previous[state])
                local[state] = merge(previous[state], continuation, local[state]);
            for (auto [value, old] : llvm::zip(*next, carried))
              value = merge(old, continuation, value);
          }
          Value stop = nested.lookup(step.getBreakIf());
          auto selection = addControl("branch_select", step, stop);
          if (failed(selection))
            return failure();
          Value keepGoing = known(stop, false);
          continuation = binary("and", continuation, keepGoing);
          sampled->path = sampled->path
              ? binary("and", sampled->path, keepGoing) : keepGoing;
          sampled->pathIdentity = builder.getDictionaryAttr({
              builder.getNamedAttr("selection", builder.getI64IntegerAttr(*selection)),
              builder.getNamedAttr("when_true", builder.getBoolAttr(false))});
        }
        carried = std::move(*next);
      }
      if (step.getBreakIf() && count) {
        sampled->path = parentPath;
        sampled->pathIdentity = parentPathIdentity;
      }
      for (auto [result, value] :
           llvm::zip(loop.getResults().drop_back(), carried))
        mapping.map(result, value);
      mapping.map(loop.getResults().back(), step.getBreakIf() ? continuation : one);
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
      auto &snapshot = sampled->effects[effect];
      snapshot.path = sampled->path;
      snapshot.pathIdentity = sampled->pathIdentity;
      for (Value value : effect.getValues())
        snapshot.values.push_back(mapping.lookup(value));
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
        sampled->rawAncestors.push_back(predicate);
      }
      SmallVector<Value> thenState(local), thenIntents(localIntents);
      auto thenValues = lowerBody(branch.getThenBody().front(), {}, thenState,
                                  thenIntents, mapping);
      if (failed(thenValues))
        return failure();
      if (selection) {
        sampled->rawAncestors.back() = make(ac::BitsUnaryOp::getOperationName(),
            {predicate}, {bit}, {
            builder.getNamedAttr("opcode", builder.getStringAttr("not"))})->getResult(0);
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
        sampled->rawAncestors.pop_back();
        sampled->path = parentPath;
        sampled->pathIdentity = parentPathIdentity;
      }
      for (unsigned state : plan.branchWrites.find(branch)->second) {
        local[state] = merge(elseState[state], predicate, thenState[state]);
        localIntents[state] = make(ac::BitsSelectOp::getOperationName(),
            {predicate, thenIntents[state], elseIntents[state]}, {bit})->getResult(0);
      }
      for (auto [result, yes, no] : llvm::zip(branch.getResults(), *thenValues, *elseValues))
        mapping.map(result, merge(no, predicate, yes));
    } else if (auto call = dyn_cast<gfir::CallOp>(op)) {
      if (auto child = plan.childMethodCalls.find(call); child != plan.childMethodCalls.end()) {
        auto results = lowerChildMethod(call, child->second, child->second.method,
                                        local, localIntents, mapping);
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
        if (sampled) {
          if (failed(lowerFifo(call, use, mapping)))
            return failure();
          continue;
        }
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
  SmallVector<std::pair<unsigned, unsigned>> childCaptures;
  for (unsigned receiver : plan.rules[index].childStates) {
    Value q = childRoutes[receiver].q;
    if (!q) return rule.emitOpError("missing actual child Q route");
    childCaptures.emplace_back(receiver, captures.size());
    captures.push_back(q);
  }
  SmallVector<std::pair<unsigned, unsigned>> guardCaptures;
  for (const auto &capture : plan.rules[index].guardCaptures) {
    auto available = availability(capture.sources);
    if (failed(available)) return failure();
    guardCaptures.emplace_back(capture.capture, captures.size());
    captures.push_back(*available);
  }
  SmallVector<std::pair<unsigned, unsigned>> memoryCaptures;
  SmallVector<std::pair<unsigned, unsigned>> fifoCaptures;
  // Capture physical snapshots once in lexical source order, including Memory
  // responses consumed by selected nested arms.
  rule.walk([&](gfir::CallOp operation) {
    if (auto use = plan.fifoCalls.find(operation);
        use != plan.fifoCalls.end() &&
        llvm::none_of(fifoCaptures, [&](const auto &capture) {
          return capture.first == use->second.fifo;
        })) {
      unsigned fifo = use->second.fifo;
      fifoCaptures.emplace_back(fifo, captures.size());
      captures.append({fifos[fifo]->getResult(3), fifos[fifo]->getResult(1),
                       fifos[fifo]->getResult(2)});
    }
    auto use = plan.transactionMemoryCalls.find(operation);
    if (use == plan.transactionMemoryCalls.end() ||
        (use->second.access != gfir::StructuredMemoryAccess::Result &&
         use->second.access != gfir::StructuredMemoryAccess::Read) ||
        llvm::any_of(memoryCaptures, [&](const auto &capture) {
          return capture.first == use->second.memory;
        }))
      return;
    unsigned memory = use->second.memory;
    memoryCaptures.emplace_back(memory, captures.size());
    llvm::append_range(captures, memories[memory]->getResults());
  });
  // External pop controls are dedicated contiguous captures. Use the raw
  // recipient or the distinct consumer's authenticated pop proposal path.
  SmallVector<std::pair<unsigned, unsigned>> externalCaptures;
  auto capturedExternal = rule.walk([&](gfir::CallOp operation) {
    auto use = plan.fifoCalls.find(operation);
    if (use == plan.fifoCalls.end() ||
        use->second.access != gfir::StructuredFifoAccess::In ||
        !plan.fifos[use->second.fifo].downstreamPop ||
        llvm::any_of(externalCaptures, [&](const auto &capture) {
          return capture.first == use->second.fifo;
        }))
      return WalkResult::advance();
    unsigned fifo = use->second.fifo;
    auto raw = plan.fifos[fifo].rawOutput;
    Value external;
    if (plan.fifos[fifo].localPopCredit) {
      if (!fifoIncomingPops[fifo]) {
        rule.emitOpError("missing verified local FIFO pop-credit dependency");
        return WalkResult::interrupt();
      }
      external = fifoIncomingPops[fifo];
      if (plan.fifos[fifo].consumer != index) {
        // A distinct consumer is ordered first by readyDependency. Own prior
        // pops are instead raw proposal paths inside this Rule's readiness.
        if (!fifoPopPaths[fifo]) {
          rule.emitOpError("missing verified distinct FIFO pop path");
          return WalkResult::interrupt();
        }
        fifoPopUnions[fifo] = binary("or", external, fifoPopPaths[fifo]);
        external = fifoPopUnions[fifo];
      }
    } else if (raw) {
      external = graph.lookup(raw.getInputs().front());
    }
    if (!external) {
      rule.emitOpError("downstream FIFO in requires authored raw output");
      return WalkResult::interrupt();
    }
    externalCaptures.emplace_back(fifo, captures.size());
    captures.push_back(external);
    return WalkResult::advance();
  });
  if (capturedExternal.wasInterrupted()) return failure();

  auto region = std::make_unique<Region>();
  auto *body = new Block;
  region->push_back(body);
  for (Value capture : captures)
    body->addArgument(capture.getType(), rule.getLoc());
  OpBuilder::InsertionGuard insertion(builder);
  builder.setInsertionPointToEnd(body);
  Value outerOne = one, outerNone = none;
  auto *outerSampled = sampled;
  auto restoreOuter = [&] {
    sampled = outerSampled;
    one = outerOne;
    none = outerNone;
  };
  auto restoreOnExit = llvm::scope_exit(restoreOuter);
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
  for (auto [fifo, first] : fifoCaptures)
    context.fifoOutputs.try_emplace(fifo, FifoCapture{first,
        body->getArgument(first), body->getArgument(first + 1),
        body->getArgument(first + 2), {}});
  for (auto [fifo, capture] : externalCaptures) {
    context.fifoOutputs.find(fifo)->second.external = capture;
    externalPops.push_back(builder.getDictionaryAttr({
        builder.getNamedAttr("registration", context.registration),
        builder.getNamedAttr("queue", localQueueID(fifo)),
        builder.getNamedAttr("capture", builder.getI64IntegerAttr(capture))}));
  }
  for (auto [memory, first] : memoryCaptures)
    for (unsigned result = 0; result < memories[memory]->getNumResults(); ++result)
      context.memoryOutputs[memory].push_back(body->getArgument(first + result));
  sampled = &context;
  if (failed(addControl("callable_guard", rule, one)))
    return failure();

  SmallVector<Value> local(states.size()), localIntents(states.size(), none);
  for (auto [ordinal, state] : llvm::enumerate(plan.rules[index].states))
    local[state] = body->getArgument(clockCapture + 2 + ordinal);
  childSlots.clear();
  for (auto [receiver, capture] : childCaptures) {
    childSlots.try_emplace(receiver, local.size());
    local.push_back(body->getArgument(capture));
    localIntents.push_back(none);
  }
  IRMapping mapping;
  auto results = lowerBody(rule.getBody().front(),
      body->getArguments().take_front(clockCapture), local, localIntents, mapping);
  if (failed(results))
    return failure();
  const auto &assertions = plan.rules[index].assertions;
  if (context.controls.size() + assertions.size() != context.plannedChecks.size())
    return rule.emitOpError("sampled refinement did not emit all verified checks");
  SmallVector<Value> yielded;
  auto resultUses = llvm::make_filter_range(
      plan.rules[index].resultUses,
      [](gfir::ResultOp result) { return !result.getValue().use_empty(); });
  // lowerBody has already emitted the checks for reached return values.
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
  SmallVector<std::pair<unsigned, unsigned>> childCandidates;
  for (unsigned receiver : plan.rules[index].childStates) {
    const auto &child = definition.childStates[receiver];
    if (child.writer != index)
      continue;
    auto stateID = routeStateID(child.route);
    if (failed(stateID))
      return failure();
    unsigned childCandidate = yielded.size();
    unsigned slot = childSlots.lookup(receiver);
    childCandidates.emplace_back(receiver, childCandidate);
    yielded.push_back(local[slot]);
    yielded.push_back(binary("and", context.prefix, localIntents[slot]));
    candidateRows.push_back(builder.getDictionaryAttr({
        builder.getNamedAttr("state", *stateID),
        builder.getNamedAttr("data", builder.getI64IntegerAttr(childCandidate)),
        builder.getNamedAttr("enable", builder.getI64IntegerAttr(childCandidate + 1))}));
  }
  unsigned fireResult = yielded.size();
  yielded.push_back(context.prefix);
  SmallVector<Attribute> observations;
  for (const auto &observation : plan.rules[index].observations) {
    auto effect = observation.operation;
    auto found = context.effects.find(effect);
    if (found == context.effects.end())
      return effect.emitOpError("missing ordered observation snapshot");
    const auto &snapshot = found->second;
    observations.push_back(builder.getDictionaryAttr({
        builder.getNamedAttr("id", observation.id),
        builder.getNamedAttr("path", snapshot.pathIdentity),
        builder.getNamedAttr("value_count", builder.getI64IntegerAttr(snapshot.values.size()))}));
    yielded.push_back(snapshot.path ? binary("and", context.prefix, snapshot.path)
                                   : context.prefix);
    yielded.append(snapshot.values);
  }
  unsigned checkBase = yielded.size();
  if (!context.resources.empty()) {
    // Fold demand backwards independently of final fire. An unavailable Queue
    // can make an earlier diagnostic unreachable; an explicit control preserves
    // the possibility of execution until its own known acceptance is established.
    // Memory snapshots contribute no Queue demand and retain neutral H=one.
    SmallVector<SmallVector<unsigned>> blocks(1 + 2 * context.controls.size());
    for (auto [ordinal, raw] : llvm::enumerate(context.controls)) {
      auto path = cast<DictionaryAttr>(raw).getAs<DictionaryAttr>("path");
      auto selection = path.getAs<IntegerAttr>("selection");
      unsigned block = selection ? 1 + 2 * selection.getInt() +
          (path.getAs<BoolAttr>("when_true").getValue() ? 0 : 1) : 0;
      blocks[block].push_back(ordinal);
    }
    auto select = [&](Value raw, Value yes, Value no) {
      return yes == no ? yes : make(ac::BitsSelectOp::getOperationName(),
          {raw, yes, no}, {bit})->getResult(0);
    };
    std::function<Value(unsigned, Value)> fold = [&](unsigned block, Value demand) {
      for (unsigned ordinal : llvm::reverse(blocks[block])) {
        auto kind = cast<DictionaryAttr>(context.controls[ordinal])
                        .getAs<StringAttr>("kind").getValue();
        if (kind == "branch_select") {
          Value yes = fold(1 + 2 * ordinal, demand);
          Value no = fold(2 + 2 * ordinal, demand);
          demand = select(context.predicates[ordinal], yes, no);
        } else if (auto queue = context.queueDemands.find(ordinal);
                   queue != context.queueDemands.end()) {
          demand = make(ac::BitsSelectOp::getOperationName(),
              {one, binary("and", queue->second, demand), demand}, {bit})->getResult(0);
        } else if (demand != one && kind != "resource_available") {
          demand = select(known(context.predicates[ordinal], true), demand, one);
        }
      }
      return demand;
    };
    Value demand = fold(0, one);
    Value mayExecute = make(ac::BitsUnaryOp::getOperationName(),
        {known(demand, false)}, {bit}, {
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
    auto found = context.effects.find(assertion);
    if (found == context.effects.end())
      return assertion.emitOpError("missing ordered assertion condition");
    const auto &snapshot = found->second;
    context.checks.push_back(builder.getDictionaryAttr({
        builder.getNamedAttr("id", check.id),
        builder.getNamedAttr("kind", builder.getStringAttr("assert")),
        builder.getNamedAttr("location", check.location)}));
    bodyChecks.push_back(builder.getDictionaryAttr({
        builder.getNamedAttr("id", check.id),
        builder.getNamedAttr("invocation", builder.getArrayAttr({})),
        builder.getNamedAttr("path", snapshot.pathIdentity)}));
    yielded.append({snapshot.values.front(), snapshot.path
        ? binary("and", context.prefix, snapshot.path) : context.prefix});
  }
  SmallVector<Attribute> queueProposals, queueWriterProposals;
  for (auto &proposal : context.queueProposals) {
    unsigned rawResult = yielded.size();
    yielded.push_back(proposal.rawPath);
    proposal.pathResult = yielded.size();
    yielded.push_back(binary("and", context.prefix, proposal.rawPath));
    auto queue = localQueueID(proposal.fifo);
    const auto &check = context.plannedChecks[proposal.control];
    StringRef kind = proposal.data ? "push" : "pop";
    NamedAttrList row({
        builder.getNamedAttr("queue", queue),
        builder.getNamedAttr("kind", builder.getStringAttr(kind)),
        builder.getNamedAttr("occurrence", check.id.get("check")),
        builder.getNamedAttr("location", check.location),
        builder.getNamedAttr("raw_path", builder.getI64IntegerAttr(rawResult)),
        builder.getNamedAttr("path", builder.getI64IntegerAttr(proposal.pathResult)),
        builder.getNamedAttr("resource_control", builder.getI64IntegerAttr(proposal.control))});
    NamedAttrList writer({
        builder.getNamedAttr("queue", queue),
        builder.getNamedAttr("kind", builder.getStringAttr(kind)),
        builder.getNamedAttr("path", builder.getI64IntegerAttr(proposal.pathResult))});
    if (proposal.data) {
      auto data = builder.getI64IntegerAttr(yielded.size());
      yielded.push_back(proposal.data);
      row.append("data", data);
      writer.append("data", data);
    }
    queueProposals.push_back(row.getDictionary(builder.getContext()));
    queueWriterProposals.push_back(writer.getDictionary(builder.getContext()));
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
        builder.getNamedAttr("owner", localResourceOwner()),
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
  restoreOuter();
  restoreOnExit.release();

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
  if (!queueProposals.empty())
    admission.append("queue_proposals", builder.getArrayAttr(queueProposals));
  if (!memoryProposals.empty())
    admission.append("memory_proposals", builder.getArrayAttr(memoryProposals));
  operation.addAttribute("ac.sampled_admission", admission.getDictionary(builder.getContext()));
  operation.addRegion(std::move(region));
  auto *lowered = builder.create(operation);
  sampledRules[index] = lowered;
  for (const auto &proposal : context.queueProposals) {
    Value &path = proposal.data ? fifoPushPaths[proposal.fifo] : fifoPopPaths[proposal.fifo];
    if (path)
      return rule.emitOpError("multiple sampled proposals for one FIFO direction");
    path = lowered->getResult(proposal.pathResult);
    if (proposal.data)
      fifoData[proposal.fifo] = lowered->getResult(proposal.pathResult + 1);
    else {
      const auto &fifo = plan.fifos[proposal.fifo];
      if (fifo.localPopCredit && fifo.producer == index && fifo.consumer == index)
        fifoPopUnions[proposal.fifo] = binary("or", fifoIncomingPops[proposal.fifo], path);
    }
  }
  if (!queueWriterProposals.empty())
    queueWriters.push_back(builder.getDictionaryAttr({
        builder.getNamedAttr("registration", context.registration),
        builder.getNamedAttr("proposals", builder.getArrayAttr(queueWriterProposals))}));
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
  for (auto [receiver, candidate] : childCandidates) {
    childRoutes[receiver].data = lowered->getResult(candidate);
    childRoutes[receiver].enable = lowered->getResult(candidate + 1);
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

LogicalResult ClockedMaterializer::bindTransactionWiring() {
  if (transactionWriters.empty() && definition.fieldOutputs.empty() &&
      llvm::none_of(definition.incomingStates, [](const auto &entry) { return entry.writable; }))
    return success();
  SmallVector<Attribute> declarations;
  for (auto [index, state] : llvm::enumerate(states))
    if (sampledEnables[index] || incomingLocal(index))
      declarations.push_back(state->getAttr("occurrence"));
  SmallVector<Attribute> incoming;
  for (auto [index, entry] : llvm::enumerate(definition.incomingStates)) {
    if (!entry.writable)
      continue;
    auto stateID = routeStateID(entry.route);
    if (failed(stateID))
      return failure();
    incoming.push_back(builder.getDictionaryAttr({
        builder.getNamedAttr("state", *stateID),
        builder.getNamedAttr("data", builder.getI64IntegerAttr(cast<BlockArgument>(incomingData[index]).getArgNumber())),
        builder.getNamedAttr("enable", builder.getI64IntegerAttr(cast<BlockArgument>(incomingEnable[index]).getArgNumber()))}));
  }
  target->setAttr("ac.transaction_wiring", builder.getDictionaryAttr({
      builder.getNamedAttr("states", builder.getArrayAttr(declarations)),
      builder.getNamedAttr("incoming", builder.getArrayAttr(incoming)),
      builder.getNamedAttr("writers", builder.getArrayAttr(transactionWriters))}));
  return success();
}

void ClockedMaterializer::bindQueueWiring() {
  if (!sampledQueues)
    return;
  SmallVector<Attribute> queues, incoming;
  for (auto [index, fifo] : llvm::enumerate(plan.fifos))
    queues.push_back(builder.getDictionaryAttr({
        builder.getNamedAttr("declaration", fifos[index]->getAttr("occurrence")),
        builder.getNamedAttr("unbound_push", builder.getBoolAttr(!fifo.rawInput)),
        builder.getNamedAttr("unbound_pop", builder.getBoolAttr(!fifo.rawOutput))}));
  for (unsigned fifo : fifoIncomingOrder)
    incoming.push_back(builder.getDictionaryAttr({
        builder.getNamedAttr("queue", localQueueID(fifo)),
        builder.getNamedAttr("kind", builder.getStringAttr("pop")),
        builder.getNamedAttr("path", builder.getI64IntegerAttr(
            cast<BlockArgument>(fifoIncomingPops[fifo]).getArgNumber()))}));
  NamedAttrList wiring({
      builder.getNamedAttr("queues", builder.getArrayAttr(queues)),
      builder.getNamedAttr("incoming", builder.getArrayAttr(incoming)),
      builder.getNamedAttr("writers", builder.getArrayAttr(queueWriters))});
  if (!externalPops.empty())
    wiring.append("external_pops", builder.getArrayAttr(externalPops));
  target->setAttr("ac.queue_wiring", wiring.getDictionary(builder.getContext()));
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
    childSlots.clear();
    for (unsigned receiver : plan.rules[index].childStates) {
      if (!childRoutes[receiver].q)
        return source.emitOpError("missing actual child Q route");
      childSlots.try_emplace(receiver, local.size());
      local.push_back(childRoutes[receiver].q);
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
  Value fifoIdle = none;
  if (sampledQueues)
    fifoIdle = make(ac::BitsConstantOp::getOperationName(), {}, {bit}, {
        builder.getNamedAttr("value", literal(0))})->getResult(0);
  for (auto [index, fifo] : llvm::enumerate(plan.fifos)) {
    Value inputValid = fifo.producer ? grants[*fifo.producer] : none;
    Value inputData = fifoData[index];
    Value outputReady = fifo.consumer ? grants[*fifo.consumer] : none;
    if (sampledQueues) {
      inputValid = fifoPushPaths[index] ? fifoPushPaths[index] : fifoIdle;
      outputReady = fifoPopPaths[index] ? fifoPopPaths[index] : fifoIdle;
      if ((fifo.producer && !fifoPushPaths[index]) ||
          (fifo.consumer && !fifoPopPaths[index]))
        return source.emitOpError("FIFO wiring requires sampled transactional writers");
      if (fifo.localPopCredit) {
        if (!fifoPopUnions[index])
          return source.emitOpError("missing verified local FIFO pop-credit route");
        outputReady = fifoPopUnions[index];
      }
    }
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
    if (auto incoming = incomingLocal(index)) {
      enable = incomingEnable[*incoming];
      // Each incoming stage closes its proposal over the actual local old Q.
      // The whole owner has the same literal-one leaf mask used by
      // materializeDomainCommit; keep AND(enable, mask) explicit for the
      // independent staged-commit proof. The raw D/E pair remains the declared
      // incoming route and the parent's exact sampled writer candidate.
      Value mask = make(ac::BitsConstantOp::getOperationName(), {}, {bit}, {
          builder.getNamedAttr("value", literal(1))})->getResult(0);
      auto closed = closeIncoming(states[index]->getResult(0), incomingData[*incoming],
                                  incomingEnable[*incoming], mask);
      if (failed(closed)) return failure();
      data = *closed;
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
    auto available = availability(output.sources);
    if (failed(available)) return failure();
    outputs.push_back(*available);
  }
  for (const auto &field : definition.fieldOutputs) {
    const auto &output = definition.outputs[field.output];
    auto value = projectField(outputs[output.port.port], output.port.path);
    if (failed(value)) return failure();
    outputs.push_back(*value);
    if (!field.sources.empty()) {
      auto available = availability(field.sources);
      if (failed(available)) return failure();
      outputs.push_back(*available);
    }
  }
  for (auto [index, entry] : llvm::enumerate(definition.incomingStates)) {
    Value q = entry.route.path.empty() ? states[entry.route.state]->getResult(0) : incomingQ[index];
    if (!q) return source.emitOpError("missing forwarded state Q route");
    outputs.push_back(q);
  }
  make(ac::YieldOp::getOperationName(), outputs, {});
  return success();
}

LogicalResult ClockedMaterializer::run() {
  bit = bits(1);
  sampledQueues = llvm::any_of(plan.rules, [](const auto &rule) {
    return rule.sampled && !rule.requirements.empty();
  });
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
      failed(bindObservations()) || failed(bindTransactionWiring()))
    return failure();
  bindQueueWiring();
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
