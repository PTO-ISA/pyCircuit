#include "LowerStructuredClocked.h"

#include "mlir/IR/Builders.h"
#include "mlir/IR/IRMapping.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/MathExtras.h"
#include <memory>

using namespace mlir;

namespace acir::detail {
namespace {

class ClockedMaterializer {
public:
  ClockedMaterializer(const gfir::StructuredDefinition &definition,
                      mlir::ModuleOp package)
      : definition(definition), plan(*definition.clocked), package(package),
        source(definition.module), builder(package.getContext()),
        hardware(source->getParentOfType<mlir::ModuleOp>()) {}
  LogicalResult run();

private:
  struct SampledContext {
    DictionaryAttr registration;
    SmallVector<Attribute> invocation, controls, checks;
    SmallVector<Value> predicates, reaches;
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
  DictionaryAttr sourceLocation(Operation *operation);
  DictionaryAttr lexicalOccurrence(Operation *operation);
  unsigned addControl(StringRef kind, Operation *site, Value raw);
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
                                         IRMapping &mapping);
  FailureOr<Value> indexedWrite(Value previous, Value index, Value replacement);
  LogicalResult bindStorage();

  const gfir::StructuredDefinition &definition;
  const gfir::StructuredClocked &plan;
  mlir::ModuleOp package;
  gfir::ModuleOp source;
  OpBuilder builder;
  ac::HardwareAnalysis hardware;
  ac::ModuleOp target;
  Type bit;
  Value clock, reset, one, none;
  IRMapping graph;
  SmallVector<Operation *> fifos, memories, states;
  SmallVector<Value> grants, candidates, intents, fifoData;
  SmallVector<Value> sampledEnables;
  SmallVector<Attribute> transactionWriters;
  llvm::DenseMap<Operation *, unsigned> lexicalOrdinals;
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

DictionaryAttr ClockedMaterializer::sourceLocation(Operation *operation) {
  if (auto location = dyn_cast<FileLineColLoc>(operation->getLoc()))
    return builder.getDictionaryAttr({
        builder.getNamedAttr("path", source.getSourceOwner().getAs<StringAttr>("path")),
        builder.getNamedAttr("line", builder.getI64IntegerAttr(location.getLine())),
        builder.getNamedAttr("column", builder.getI64IntegerAttr(location.getColumn())),
        builder.getNamedAttr("end_line", builder.getI64IntegerAttr(location.getLine())),
        builder.getNamedAttr("end_column", builder.getI64IntegerAttr(location.getColumn() + 1))});
  return cast<DictionaryAttr>(literal(0).getTree().get("location"));
}

DictionaryAttr ClockedMaterializer::lexicalOccurrence(Operation *operation) {
  return occurrence("ordered", lexicalOrdinals.lookup(operation));
}

unsigned ClockedMaterializer::addControl(StringRef kind, Operation *site, Value raw) {
  unsigned ordinal = sampled->controls.size();
  auto id = builder.getDictionaryAttr({
      builder.getNamedAttr("registration", sampled->registration),
      builder.getNamedAttr("check", lexicalOccurrence(site)),
      builder.getNamedAttr("obligation", builder.getI64IntegerAttr(ordinal))});
  sampled->controls.push_back(builder.getDictionaryAttr({
      builder.getNamedAttr("kind", builder.getStringAttr(kind)),
      builder.getNamedAttr("id", id),
      builder.getNamedAttr("invocation", builder.getArrayAttr(sampled->invocation)),
      builder.getNamedAttr("predecessor", builder.getI64IntegerAttr(int64_t(ordinal) - 1)),
      builder.getNamedAttr("path", sampled->pathIdentity)}));
  sampled->checks.push_back(builder.getDictionaryAttr({
      builder.getNamedAttr("id", id),
      builder.getNamedAttr("kind", builder.getStringAttr("guard_known")),
      builder.getNamedAttr("location", sourceLocation(site))}));
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
  for (StringRef name : {"sym_name", "source_owner", "output_names"})
    state.addAttribute(name, source->getAttr(name));
  SmallVector<Type> inputs(source.getFunctionType().getInputs());
  inputs.append({bit, bit});
  state.addAttribute("function_type", TypeAttr::get(
      builder.getFunctionType(inputs, source.getFunctionType().getResults())));
  SmallVector<Attribute> names(source.getInputNames().getValue());
  names.append({builder.getStringAttr("pyc_clk"), builder.getStringAttr("pyc_rst")});
  state.addAttribute("input_names", builder.getArrayAttr(names));
  state.addAttribute("parameters", builder.getArrayAttr({}));
  state.addAttribute("type_parameters", builder.getArrayAttr({}));
  state.addAttribute("ac.domain_inputs", builder.getDictionaryAttr({
      builder.getNamedAttr("clock", builder.getI64IntegerAttr(inputs.size() - 2)),
      builder.getNamedAttr("reset", builder.getI64IntegerAttr(inputs.size() - 1))}));
  state.addRegion();
  target = cast<ac::ModuleOp>(builder.create(state));
  auto *body = new Block;
  target.getBody().push_back(body);
  for (Type type : inputs)
    body->addArgument(type, source.getLoc());
  for (auto [from, to] : llvm::zip(source.getBody().front().getArguments(),
                                   body->getArguments()))
    graph.map(from, to);
  clock = body->getArgument(inputs.size() - 2);
  reset = body->getArgument(inputs.size() - 1);
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
    } else {
      state.addAttributes(op.getAttrs());
    }
    auto *copy = builder.create(state);
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
    }
    copy->setOperands(operands);
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
ClockedMaterializer::lowerBody(Block &body, ValueRange inputs,
                               SmallVector<Value> &local,
                               SmallVector<Value> &localIntents,
                               IRMapping &mapping) {
  for (auto [argument, input] : llvm::zip(body.getArguments(), inputs))
    mapping.map(argument, input);
  for (Operation &op : body) {
    if (auto yield = dyn_cast<gfir::YieldOp>(op)) {
      SmallVector<Value> values;
      for (Value value : yield.getValues())
        values.push_back(mapping.lookup(value));
      return values;
    }
    if (auto read = dyn_cast<gfir::ReadOp>(op)) {
      mapping.map(read.getValue(), local[plan.stateAccesses.lookup(read)]);
    } else if (auto write = dyn_cast<gfir::WriteOp>(op)) {
      unsigned target = plan.stateAccesses.lookup(write);
      Value value = mapping.lookup(write.getValue());
      if (write.getIndex()) {
        auto next = indexedWrite(local[target], mapping.lookup(write.getIndex()), value);
        if (failed(next))
          return failure();
        value = *next;
      }
      local[target] = value;
      localIntents[target] = one;
    } else if (auto guard = dyn_cast<gfir::GuardOp>(op)) {
      addControl("ordered_guard", guard, mapping.lookup(guard.getPredicate()));
    } else if (auto branch = dyn_cast<gfir::IfOp>(op)) {
      Value predicate = mapping.lookup(branch.getPredicate());
      Value parentPath;
      DictionaryAttr parentPathIdentity;
      std::optional<unsigned> selection;
      if (sampled && plan.checkedBranches.contains(branch)) {
        parentPath = sampled->path;
        parentPathIdentity = sampled->pathIdentity;
        selection = addControl("branch_select", branch, predicate);
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
      SmallVector<Value> results;
      if (!call.getReceiverAttr()) {
        SmallVector<Value> arguments;
        for (Value input : call.getInputs())
          arguments.push_back(mapping.lookup(input));
        IRMapping nested;
        auto method = plan.methods[plan.methodCalls.lookup(call)];
        if (sampled) {
          sampled->invocation.push_back(builder.getDictionaryAttr({
              builder.getNamedAttr("call", lexicalOccurrence(call)),
              builder.getNamedAttr("receiver", builder.getDictionaryAttr({
                  builder.getNamedAttr("instance_path", builder.getArrayAttr({}))}))}));
          addControl("callable_guard", method, one);
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
  context.registration = lexicalOccurrence(rule);
  context.pathIdentity = builder.getDictionaryAttr({});
  context.prefix = one;
  sampled = &context;
  addControl("callable_guard", rule, one);

  SmallVector<Value> local(states.size()), localIntents(states.size(), none);
  for (auto [ordinal, state] : llvm::enumerate(plan.rules[index].states))
    local[state] = body->getArgument(clockCapture + 2 + ordinal);
  IRMapping mapping;
  auto results = lowerBody(rule.getBody().front(),
      body->getArguments().take_front(clockCapture), local, localIntents, mapping);
  if (failed(results)) {
    sampled = nullptr;
    one = outerOne;
    none = outerNone;
    return failure();
  }
  SmallVector<Value> yielded;
  SmallVector<Attribute> candidateRows;
  for (auto [ordinal, state] : llvm::enumerate(plan.rules[index].writes)) {
    yielded.push_back(local[state]);
    yielded.push_back(binary("and", context.prefix, localIntents[state]));
    auto stateID = builder.getDictionaryAttr({
        builder.getNamedAttr("owner", builder.getDictionaryAttr({
            builder.getNamedAttr("instance_path", builder.getArrayAttr({}))})),
        builder.getNamedAttr("declaration", states[state]->getAttr("occurrence")),
        builder.getNamedAttr("element", builder.getArrayAttr({}))});
    candidateRows.push_back(builder.getDictionaryAttr({
        builder.getNamedAttr("state", stateID),
        builder.getNamedAttr("data", builder.getI64IntegerAttr(2 * ordinal)),
        builder.getNamedAttr("enable", builder.getI64IntegerAttr(2 * ordinal + 1))}));
  }
  unsigned fireResult = yielded.size();
  yielded.push_back(context.prefix);
  for (auto [raw, reached] : llvm::zip(context.predicates, context.reaches))
    yielded.append({raw, reached});
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
  operation.addAttribute("ac.sampled_admission", builder.getDictionaryAttr({
      builder.getNamedAttr("clock_capture", builder.getI64IntegerAttr(clockCapture)),
      builder.getNamedAttr("reset_capture", builder.getI64IntegerAttr(clockCapture + 1)),
      builder.getNamedAttr("fire_result", builder.getI64IntegerAttr(fireResult)),
      builder.getNamedAttr("controls", builder.getArrayAttr(context.controls)),
      builder.getNamedAttr("body_checks", builder.getArrayAttr({})),
      builder.getNamedAttr("observations", builder.getArrayAttr({}))}));
  operation.addRegion(std::move(region));
  auto *lowered = builder.create(operation);
  for (auto [ordinal, state] : llvm::enumerate(plan.rules[index].writes)) {
    candidates[state] = lowered->getResult(2 * ordinal);
    sampledEnables[state] = lowered->getResult(2 * ordinal + 1);
  }
  for (auto [ordinal, raw] : llvm::enumerate(context.checks)) {
    auto check = cast<DictionaryAttr>(raw);
    make(ac::SourceExpectOp::getOperationName(), {
        lowered->getResult(fireResult + 1 + 2 * ordinal),
        lowered->getResult(fireResult + 2 + 2 * ordinal), clock, reset}, {}, {
        builder.getNamedAttr("kind", builder.getStringAttr("guard_known")),
        builder.getNamedAttr("location", check.get("location")),
        builder.getNamedAttr("ac.check_id", check.get("id"))});
  }
  transactionWriters.push_back(builder.getDictionaryAttr({
      builder.getNamedAttr("registration", context.registration),
      builder.getNamedAttr("ordinary_results", builder.getI64IntegerAttr(0)),
      builder.getNamedAttr("candidates", builder.getArrayAttr(candidateRows))}));
  grants[index] = one;
  return success();
}

void ClockedMaterializer::bindTransactionWiring() {
  if (transactionWriters.empty())
    return;
  SmallVector<Attribute> declarations;
  for (auto [index, state] : llvm::enumerate(states))
    if (sampledEnables[index])
      declarations.push_back(state->getAttr("occurrence"));
  target->setAttr("ac.transaction_wiring", builder.getDictionaryAttr({
      builder.getNamedAttr("states", builder.getArrayAttr(declarations)),
      builder.getNamedAttr("incoming", builder.getArrayAttr({})),
      builder.getNamedAttr("writers", builder.getArrayAttr(transactionWriters))}));
}

LogicalResult ClockedMaterializer::lowerRules() {
  grants.resize(plan.rules.size());
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

LogicalResult ClockedMaterializer::bindStorage() {
  for (auto [index, memory] : llvm::enumerate(plan.memories)) {
    SmallVector<Value> inputs{clock, reset};
    auto raw = memory.raw;
    for (Value pin : raw.getInputs())
      inputs.push_back(graph.lookup(pin));
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
    SmallVector<Value> inputs{clock, reset, enable, candidates[index],
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
  make(ac::YieldOp::getOperationName(), outputs, {});
  return success();
}

LogicalResult ClockedMaterializer::run() {
  source.walk([&](Operation *operation) {
    lexicalOrdinals.try_emplace(operation, lexicalOrdinals.size());
  });
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
      failed(lowerRules()) || failed(bindGraph()) || failed(bindStorage()))
    return failure();
  bindTransactionWiring();
  return success();
}

} // namespace

LogicalResult
materializeStructuredClocked(const gfir::StructuredDefinition &definition,
                              mlir::ModuleOp package) {
  return ClockedMaterializer(definition, package).run();
}

} // namespace acir::detail
