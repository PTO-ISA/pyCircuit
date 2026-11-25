#include "pycircuit/Dialect/GFIR/StructuredProgram.h"
#include "pycircuit/Dialect/GFIR/StructuredTableMap.h"
#include "llvm/ADT/StringSet.h"
#include "mlir/IR/Builders.h"
#include "mlir/Interfaces/SideEffectInterfaces.h"
#include <bit>

using namespace mlir;

namespace acir::gfir {
namespace {

std::optional<unsigned> findWire(const StructuredDefinition &definition,
                                 Value value, ArrayRef<StringAttr> path = {}) {
  auto found = definition.valueWires.find(value);
  if (found != definition.valueWires.end())
    for (unsigned index : found->second)
      if (llvm::equal(definition.wires[index].endpoint.path, path))
        return index;
  return std::nullopt;
}

struct RowUpdate {
  unsigned scope = 0;
  Value index;
  Value data;
  SmallVector<Value, 3> conditions;
  bool operator==(const RowUpdate &) const = default;
};

// A successful trace with no index is the original committed table. Every
// other successful trace represents one replacement on that same table.
class TableTrace {
public:
  struct Scope {
    const StructuredDefinition *definition;
    CallOp call;
  };

  TableTrace(const StructuredProgram &program, const ac::HardwareAnalysis &analysis,
             unsigned state, StringAttr field, ac::TableType table, uint64_t size)
      : program(program), analysis(analysis), state(state), field(field),
        table(table), size(size) {
    scopes.push_back({&program.definitions[program.rootIndex], {}});
  }

  std::optional<RowUpdate> trace(unsigned scope, unsigned index, unsigned depth = 0);
  Scope getScope(unsigned index) const { return scopes[index]; }

private:
  std::optional<RowUpdate> traceWire(unsigned scope, unsigned index, unsigned depth);
  std::optional<RowUpdate> traceMap(unsigned scope, ac::TableMapOp map,
                                    unsigned result, unsigned depth);
  const StructuredProgram &program;
  const ac::HardwareAnalysis &analysis;
  unsigned state;
  StringAttr field;
  ac::TableType table;
  uint64_t size;
  SmallVector<Scope> scopes;
  llvm::DenseMap<Operation *, unsigned> callScopes;
  llvm::DenseMap<std::pair<unsigned, unsigned>, std::optional<RowUpdate>> traced;
};

std::optional<RowUpdate> TableTrace::trace(unsigned scope, unsigned index,
                                           unsigned depth) {
  // Do not grow the compiler stack on long checked graphs.
  constexpr unsigned maxDepth = 256;
  if (depth == maxDepth)
    return std::nullopt;
  std::pair key(scope, index);
  auto inserted = traced.try_emplace(key, std::nullopt);
  if (!inserted.second)
    return inserted.first->second;
  auto result = traceWire(scope, index, depth + 1);
  traced.find(key)->second = result;
  return result;
}

std::optional<RowUpdate> TableTrace::traceWire(unsigned scope, unsigned index,
                                               unsigned depth) {
  auto binding = scopes[scope];
  if (index >= binding.definition->wires.size())
    return std::nullopt;
  const auto &wire = binding.definition->wires[index];
  if (!ac::areEquivalentHardwareTypes(wire.type, table))
    return std::nullopt;
  if (!scope && wire.state == state && llvm::equal(wire.endpoint.path, ArrayRef{field}))
    return RowUpdate{};
  if (auto argument = dyn_cast<BlockArgument>(wire.endpoint.value)) {
    if (!binding.call)
      return std::nullopt;
    auto input = binding.call.getInputs()[argument.getArgNumber()];
    auto parent = findWire(*scopes.front().definition, input, wire.endpoint.path);
    return parent ? trace(0, *parent, depth) : std::nullopt;
  }
  if (wire.call) {
    if (scope)
      return std::nullopt;
    auto call = wire.endpoint.value.getDefiningOp<CallOp>();
    const auto &callee = program.definitions[wire.call->definition];
    if (!call || callee.clocked || !callee.clockedCalls.empty() || callee.hasChecks())
      return std::nullopt;
    auto [entry, inserted] = callScopes.try_emplace(call, scopes.size());
    if (inserted)
      scopes.push_back({&callee, call});
    return trace(entry->second, callee.outputs[wire.call->output].wire, depth);
  }
  if (auto map = wire.endpoint.value.getDefiningOp<ac::TableMapOp>())
    return traceMap(scope, map, cast<OpResult>(wire.endpoint.value).getResultNumber(), depth);
  if (wire.dependencies.size() == 1 &&
      isa_and_nonnull<ac::StructCreateOp, ac::StructGetOp>(wire.endpoint.value.getDefiningOp()))
    return trace(scope, wire.dependencies.front(), depth);
  return std::nullopt;
}

std::optional<RowUpdate> TableTrace::traceMap(unsigned scope, ac::TableMapOp map,
                                              unsigned result, unsigned depth) {
  detail::TableMapRow row(map, result);
  auto traceInput = [&](unsigned operand) -> std::optional<RowUpdate> {
    auto input = findWire(*scopes[scope].definition, map.getTables()[operand]);
    return input ? trace(scope, *input, depth) : std::nullopt;
  };
  if (auto input = row.tableOperand(row.getValue()))
    return traceInput(*input);
  auto select = row.getValue().getDefiningOp<ac::BitsSelectOp>();
  if (!select)
    return std::nullopt;
  auto oldOperand = row.tableOperand(select.getFalseValue());
  if (!oldOperand)
    return std::nullopt;
  auto old = traceInput(*oldOperand);
  if (!old)
    return std::nullopt;
  auto index = row.matchingIndex(select.getCondition());
  auto data = row.uniform(select.getTrueValue());
  if (index && data) {
    if (old->index ||
        !ac::areEquivalentHardwareTypes(data->value.getType(), table.getElementType()))
      return std::nullopt;
    auto width = analysis.getPackedWidth(index->value.getType(), {}, map);
    auto ordinal = map.getBody().front().getArgument(0);
    auto ordinalWidth = analysis.getPackedWidth(ordinal.getType(), {}, map);
    // The index must fit the table, and the map ordinal must not wrap.
    if (failed(width) || failed(ordinalWidth) || *width >= 64 ||
        (uint64_t{1} << *width) > size || std::bit_width(size - 1) > *ordinalWidth)
      return std::nullopt;
    return RowUpdate{scope, index->value, data->value, {}};
  }
  if (auto newOperand = row.tableOperand(select.getTrueValue())) {
    auto updated = traceInput(*newOperand);
    if (!updated)
      return std::nullopt;
    if (*old == *updated)
      return old;
    auto condition = row.uniform(select.getCondition());
    if (old->index || !updated->index || updated->scope != scope || !condition)
      return std::nullopt;
    // A chain of muxes with the same false arm is an AND of its conditions:
    // any known false keeps Old, and repeated unknown merges are idempotent.
    updated->conditions.push_back(condition->value);
    return updated;
  }
  return std::nullopt;
}

struct Recovery {
  RuleOp rule;
  WriteOp write;
  unsigned capture;
  StringAttr tableField;
  SmallVector<std::pair<StringAttr, Type>> otherFields;
  RowUpdate row;
  TableTrace::Scope producer;
};

std::optional<Recovery> planRecovery(mlir::ModuleOp package,
                                     const StructuredProgram &program) {
  auto selected = program.root;
  const auto &root = program.definitions[program.rootIndex];
  if (selected.getKind() != "module" || !root.clocked || root.incomingState ||
      !root.clockedCalls.empty() || root.hasChecks())
    return std::nullopt;
  const auto &clocked = *root.clocked;
  if (clocked.rules.size() != 1 || !clocked.fifos.empty() || !clocked.memories.empty())
    return std::nullopt;
  const auto &plan = clocked.rules.front();
  if (plan.sampled || !plan.requirements.empty() || !plan.guardCaptures.empty())
    return std::nullopt;

  auto rule = plan.operation;
  WriteOp write;
  for (Operation &op : rule.getBody().front()) {
    if (auto candidate = dyn_cast<WriteOp>(op)) {
      if (write || candidate.getFieldsAttr() || candidate.getIndex())
        return std::nullopt;
      write = candidate;
    } else if (!isa<ReadOp>(op) && !isMemoryEffectFree(&op)) {
      return std::nullopt;
    }
  }
  if (!write)
    return std::nullopt;
  auto argument = dyn_cast<BlockArgument>(write.getValue());
  auto type = dyn_cast<ac::StructType>(write.getValue().getType());
  if (!argument || argument.getOwner() != &rule.getBody().front() ||
      !argument.hasOneUse() || !type)
    return std::nullopt;

  ac::HardwareAnalysis analysis(package);
  auto record = analysis.lookupStruct(type);
  if (!record)
    return std::nullopt;
  Recovery recovery{rule, write, argument.getArgNumber(), {}, {}, {}, {}};
  ac::TableType table;
  for (Attribute attr : record.getFields()) {
    auto definition = cast<DictionaryAttr>(attr);
    auto name = definition.getAs<StringAttr>("name");
    auto fieldType = definition.getAs<TypeAttr>("type").getValue();
    if (auto candidate = dyn_cast<ac::TableType>(fieldType)) {
      if (table || candidate.getShape().size() != 1 || !isa<ac::BitsType>(candidate.getElementType()))
        return std::nullopt;
      table = candidate;
      recovery.tableField = name;
    } else {
      recovery.otherFields.emplace_back(name, fieldType);
    }
  }
  // A non-table field establishes the original unconditional write intent
  // before the table branch, even when that branch's condition is X or Z.
  if (!table || recovery.otherFields.empty())
    return std::nullopt;
  auto next = rule.getCaptures()[recovery.capture];
  auto wire = findWire(root, next, ArrayRef{recovery.tableField});
  if (!wire)
    return std::nullopt;
  auto size = analysis.getTableSize(table, {}, write);
  if (failed(size))
    return std::nullopt;
  TableTrace trace(program, analysis, clocked.stateAccesses.lookup(write), recovery.tableField,
                   table, *size);
  auto update = trace.trace(0, *wire);
  if (!update || !update->index)
    return std::nullopt;
  recovery.row = std::move(*update);
  recovery.producer = trace.getScope(recovery.row.scope);
  if (recovery.producer.call) {
    // Extending a private Module signature requires a matching call for every
    // occurrence in the package. Only extend it when there is one occurrence.
    auto producer = recovery.producer.definition->module;
    unsigned occurrences = 0;
    package.walk([&](InstanceOp instance) {
      if (instance.getCallee() == producer.getSymName())
        ++occurrences;
    });
    if (occurrences != 1)
      return std::nullopt;
  }
  return recovery;
}

Operation *makeOperation(OpBuilder &builder, Location loc, StringRef name,
                         ValueRange inputs = {}, TypeRange outputs = {},
                         ArrayRef<NamedAttribute> attrs = {}) {
  OperationState state(loc, name);
  state.addOperands(inputs);
  state.addTypes(outputs);
  state.addAttributes(attrs);
  return builder.create(state);
}

SmallVector<Value, 3> exportRow(const Recovery &recovery) {
  auto producer = recovery.producer.definition->module;
  auto yield = cast<YieldOp>(producer.getBody().front().getTerminator());
  OpBuilder builder(yield);
  auto loc = yield.getLoc();
  const auto &row = recovery.row;
  SmallVector<Value, 3> values{row.index, row.data};
  if (!row.conditions.empty()) {
    Value condition = row.conditions.front();
    for (Value next : ArrayRef(row.conditions).drop_front())
      condition = makeOperation(builder, loc, ac::BitsBinaryOp::getOperationName(),
          {condition, next}, {condition.getType()},
          {builder.getNamedAttr("opcode", builder.getStringAttr("and"))})->getResult(0);
    values.push_back(condition);
  }
  auto call = recovery.producer.call;
  if (!call)
    return values;

  SmallVector<Value> outputs(yield.getValues());
  auto outputNames = producer.getOutputNames();
  SmallVector<Attribute> names(outputNames.begin(), outputNames.end());
  llvm::StringSet<> used;
  for (ArrayAttr list : {producer.getInputNames(), producer.getOutputNames()})
    for (Attribute name : list)
      used.insert(cast<StringAttr>(name).getValue());
  SmallVector<unsigned, 3> exported;
  const StringRef labels[] = {"row_index", "row_data", "row_enable"};
  for (auto [label, value] : llvm::zip(labels, values)) {
    auto found = llvm::find(outputs, value);
    exported.push_back(found - outputs.begin());
    if (found != outputs.end())
      continue;
    outputs.push_back(value);
    std::string name = label.str();
    for (unsigned suffix = 0; !used.insert(name).second; ++suffix)
      name = label.str() + "_" + std::to_string(suffix);
    names.push_back(builder.getStringAttr(name));
  }
  Operation *result = call;
  if (outputs.size() != call.getNumResults()) {
    SmallVector<Type> types;
    llvm::append_range(types, ValueRange(outputs).getTypes());
    producer->setAttr("function_type", TypeAttr::get(
        builder.getFunctionType(producer.getFunctionType().getInputs(), types)));
    producer->setAttr("output_names", builder.getArrayAttr(names));
    yield->setOperands(outputs);
    builder.setInsertionPoint(call);
    result = makeOperation(builder, call.getLoc(), CallOp::getOperationName(),
                           call.getInputs(), types, call->getAttrs());
    call->replaceAllUsesWith(result->getResults().take_front(call.getNumResults()));
    call.erase();
  }
  for (auto [i, index] : llvm::enumerate(exported))
    values[i] = result->getResult(index);
  return values;
}

void applyRecovery(const Recovery &recovery) {
  auto rule = recovery.rule;
  auto write = recovery.write;
  auto row = exportRow(recovery);
  auto loc = write.getLoc();
  SmallVector<Value> captures(rule.getCaptures());
  Value next = captures[recovery.capture];
  Block &body = rule.getBody().front();
  auto capture = [&](Value source) {
    captures.push_back(source);
    return body.addArgument(source.getType(), loc);
  };
  OpBuilder graph(rule), ordered(write);
  auto assign = [&](OpBuilder &builder, StringAttr field, Value value, Value index = {}) {
    SmallVector<Value, 2> inputs{value};
    if (index)
      inputs.push_back(index);
    makeOperation(builder, loc, WriteOp::getOperationName(), inputs, {},
                   {builder.getNamedAttr("state", write.getStateAttr()),
                    builder.getNamedAttr("fields", builder.getArrayAttr({field}))});
  };
  for (auto [field, type] : recovery.otherFields) {
    auto projected = makeOperation(graph, loc, ac::StructGetOp::getOperationName(), {next}, {type},
                                   {graph.getNamedAttr("field", field)})->getResult(0);
    assign(ordered, field, capture(projected));
  }
  auto index = capture(row[0]);
  auto data = capture(row[1]);
  if (row.size() == 2) {
    assign(ordered, recovery.tableField, data, index);
  } else {
    OperationState branch(loc, IfOp::getOperationName());
    branch.addOperands(capture(row[2]));
    branch.addRegion();
    branch.addRegion();
    auto *choice = ordered.create(branch);
    OpBuilder arm(rule.getContext());
    for (unsigned i = 0; i != 2; ++i) {
      auto *block = new Block();
      choice->getRegion(i).push_back(block);
      arm.setInsertionPointToEnd(block);
      if (i == 0)
        assign(arm, recovery.tableField, data, index);
      makeOperation(arm, loc, YieldOp::getOperationName());
    }
  }
  write.erase();
  body.eraseArgument(recovery.capture);
  captures.erase(captures.begin() + recovery.capture);
  rule->setOperands(captures);
  rule->setAttr("function_type", TypeAttr::get(
      graph.getFunctionType(body.getArgumentTypes(), rule.getFunctionType().getResults())));
}

} // namespace

bool recoverStructuredRowWrite(mlir::ModuleOp package, const StructuredProgram &program) {
  auto recovery = planRecovery(package, program);
  if (!recovery)
    return false;
  applyRecovery(*recovery);
  return true;
}

} // namespace acir::gfir
