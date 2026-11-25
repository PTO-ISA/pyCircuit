#include "BehavioralEmitCpp.h"
#include "FinalCppNames.h"
#include "HardwareEmitCommon.h"
#include "pycircuit/Dialect/GFIR/StructuredProgram.h"
#include "llvm/ADT/BitVector.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/STLFunctionalExtras.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/Support/JSON.h"
#include "llvm/Support/raw_ostream.h"
#include "mlir/Interfaces/SideEffectInterfaces.h"
#include <algorithm>
#include <bit>
#include <limits>
#include <map>

using namespace mlir;

namespace acir::compiler {
namespace {

#include "BehavioralCppSupport.inc"

std::string wireName(unsigned index) { return "value_" + std::to_string(index); }
std::string moduleName(unsigned index) { return "pyc_gfir_module_" + std::to_string(index); }
std::string functionName(unsigned index) { return "output_" + std::to_string(index); }
std::string carrier(unsigned width) { return "gfsim::FourState<" + std::to_string(width) + ">"; }
std::string stateName(unsigned index) { return "pyc_state_" + std::to_string(index); }
std::string initialName(unsigned index) { return "pyc_initial_" + std::to_string(index); }
std::string frameName(unsigned index) { return "pyc_frame_" + std::to_string(index); }
std::string ruleName(unsigned index) { return "pyc_rule_" + std::to_string(index); }
std::string payloadName(unsigned index) { return "pyc_payload_" + std::to_string(index); }
std::string recipientName(unsigned index) { return "pyc_recipient_" + std::to_string(index); }
std::string resultName(unsigned index) { return "pyc_result_" + std::to_string(index); }
std::string preparedName(unsigned index) { return "pyc_prepared_" + std::to_string(index); }
std::string wholeOutputName(unsigned port) { return "output_value_" + std::to_string(port); }
std::string wholeSnapshotName(unsigned port) { return "pyc_output_value_" + std::to_string(port); }
std::string availabilityName(unsigned port) { return "pyc_available_" + std::to_string(port); }
std::string fieldAvailabilityName(unsigned field) {
  return "pyc_field_available_" + std::to_string(field);
}
std::string memoryPinName(unsigned memory, unsigned port, unsigned argument) {
  return "pyc_memory_" + std::to_string(memory) + "_" + std::to_string(port) + "_" + std::to_string(argument);
}
std::string intentName(unsigned index) { return "pyc_intent_" + std::to_string(index); }
std::string childCandidateName(unsigned receiver) {
  return "pyc_child_candidate_" + std::to_string(receiver);
}
std::string observationName(unsigned slot) {
  return "pyc_observation_" + std::to_string(slot);
}

bool isClocked(const gfir::StructuredDefinition &definition) {
  return definition.clocked.has_value() || !definition.clockedCalls.empty();
}

bool inlineRootWork(const gfir::StructuredProgram &program) {
  auto root = program.root;
  if (program.definitions.size() != 1 || root.getKind() != "module")
    return false;
  const auto &definition = program.definitions.front();
  if (!definition.clocked || definition.hasChecks() || definition.explicitDomains)
    return false;
  const auto &plan = *definition.clocked;
  if (!plan.fifos.empty() || !plan.memories.empty())
    return false;
  // Limit repeated compilation of inline Work bodies in downstream TUs.
  constexpr size_t budget = 128;
  size_t cost = definition.order.size();
  if (cost >= budget)
    return false;
  auto module = definition.module;
  return !module->walk([&](Operation *) {
    return ++cost <= budget ? WalkResult::advance() : WalkResult::interrupt();
  }).wasInterrupted();
}

bool hasAssertions(const gfir::StructuredDefinition &definition) {
  return definition.clocked && llvm::any_of(definition.clocked->rules,
      [](const gfir::StructuredRule &rule) { return !rule.assertions.empty(); });
}

bool hasObservations(const gfir::StructuredDefinition &definition) {
  return definition.clocked && llvm::any_of(definition.clocked->rules,
      [](const gfir::StructuredRule &rule) { return !rule.observations.empty(); });
}

std::string assertionName(unsigned rule, unsigned check) {
  return "pyc_assert_failed_" + std::to_string(rule) + "_" + std::to_string(check);
}

bool needsDomain(const gfir::StructuredClocked &clocked) {
  return !clocked.variables.empty() || llvm::any_of(clocked.rules,
      [](const gfir::StructuredRule &rule) { return rule.sampled; });
}

constexpr StringLiteral levelName = "pyc_behavior::level";
constexpr StringLiteral noCheck = "std::numeric_limits<unsigned>::max()";
constexpr unsigned minScalarOperations = 8;
constexpr unsigned maxScalarOperations = 64;

std::string cppString(StringRef value) {
  std::string text;
  llvm::raw_string_ostream out(text);
  out << '"';
  out.write_escaped(value, false);
  out << '"';
  return text;
}

std::string concatenate(ArrayRef<std::string> fields) {
  if (fields.size() == 1)
    return fields.front();
  return "pyc_behavior::concatenate(" + join(fields, ", ") + ")";
}

std::string pack(StringRef value) {
  return "pyc_behavior::pack(" + value.str() + ")";
}

std::string unpack(StringRef wire, StringRef value) {
  return "pyc_behavior::unpack<" + wire.str() + ">(" + value.str() + ")";
}

std::string bitWords(const llvm::APInt &plane) {
  SmallVector<std::string> words;
  for (unsigned i = 0; i < plane.getNumWords(); ++i)
    words.push_back(std::to_string(plane.getRawData()[i]) + "ULL");
  return "gfsim::Bits<" + std::to_string(plane.getBitWidth()) + ">{" +
         join(words, ", ") + "}";
}

std::string constant4(StringRef pattern) {
  unsigned width = pattern.size();
  llvm::APInt value(width, 0), known(width, 0), z(width, 0);
  for (auto [index, symbol] : llvm::enumerate(pattern)) {
    unsigned bit = width - 1 - index;
    if (symbol == '0' || symbol == '1')
      known.setBit(bit);
    if (symbol == '1')
      value.setBit(bit);
    if (symbol == 'z')
      z.setBit(bit);
  }
  return carrier(width) + "::fromMasks(" + bitWords(value) + ", " +
         bitWords(known) + ", " + bitWords(z) + ")";
}

FailureOr<llvm::APSInt> integer(ac::StaticExprAttr expression,
                               ac::HardwareAnalysis &analysis, Operation *op) {
  auto value = analysis.evaluateStatic(expression, {}, op);
  if (failed(value))
    return failure();
  auto number = dyn_cast<ac::MathIntAttr>(*value);
  if (!number)
    return op->emitOpError("behavioral bit computation requires a closed integer");
  return llvm::APSInt(number.getCanonicalValue());
}

FailureOr<std::string> qualifiedName(StringRef symbol, Operation *site) {
  SmallVector<StringRef> parts;
  symbol.split(parts, '.');
  SmallVector<std::string> names;
  for (StringRef part : parts) {
    auto name = legalizeIdentifier(part, [&] { return site->emitOpError(); });
    if (failed(name))
      return failure();
    names.push_back(std::move(*name));
  }
  return "::" + join(names, "::");
}

FailureOr<std::string> instanceName(gfir::InstanceOp instance) {
  auto name = legalizeIdentifier(instance.getSymName(),
                                 [&] { return instance.emitOpError(); });
  if (failed(name))
    return failure();
  return "pyc_child_" + *name;
}

using TableWireTypes = llvm::DenseMap<Type, std::string>;

std::string graphType(const gfir::StructuredWire &wire,
                      const TableWireTypes &tables) {
  auto type = tables.find(wire.type);
  return type == tables.end() ? carrier(wire.width) : type->second;
}

void signature(raw_ostream &out, const gfir::StructuredDefinition &definition,
               const TableWireTypes &tables, unsigned result, StringRef owner = {}) {
  const auto &output = definition.outputs[result];
  out << graphType(definition.wires[output.wire], tables) << ' ' << owner
      << functionName(result) << '(';
  ArrayRef<unsigned> inputs = output.requiresPreparation ? ArrayRef<unsigned>{}
                                                         : ArrayRef<unsigned>(output.inputs);
  llvm::interleaveComma(inputs, out, [&](unsigned input) {
    out << "const " << graphType(definition.wires[input], tables) << " &"
        << wireName(input);
  });
  out << ") const";
}

llvm::BitVector wireClosure(const gfir::StructuredDefinition &definition,
                            ArrayRef<unsigned> outputs,
                            llvm::function_ref<bool(unsigned, unsigned)> skip = {}) {
  llvm::BitVector needed(definition.order.size());
  for (unsigned output : outputs)
    needed.set(output);
  for (unsigned index : llvm::reverse(definition.order))
    if (needed.test(index))
      for (unsigned input : definition.dependencies(index))
        if (!skip || !skip(index, input))
          needed.set(input);
  return needed;
}

std::optional<unsigned> packedStateTable(const gfir::StructuredDefinition &definition,
                                         unsigned index) {
  // Record construction and projection forward a leaf without changing it.
  while (true) {
    const auto &wire = definition.wires[index];
    if (wire.state && isa<ac::StructType>(wire.endpoint.value.getType()))
      return index;
    if (wire.dependencies.size() != 1 ||
        !isa_and_nonnull<ac::StructCreateOp, ac::StructGetOp>(
            wire.endpoint.value.getDefiningOp()))
      return std::nullopt;
    index = wire.dependencies.front();
  }
}

bool canBorrowTableRead(gfir::ReadOp read) {
  Value table = read.getValue();
  if (!isa<ac::TableType>(table.getType()) || !table.hasOneUse())
    return false;
  auto lookup = dyn_cast<ac::TableGetOp>(*table.getUsers().begin());
  if (!lookup || lookup->getBlock() != read->getBlock() ||
      !read->isBeforeInBlock(lookup))
    return false;
  // The lookup consumes the rows before this snapshot could be invalidated.
  for (auto *op = read->getNextNode(); op != lookup.getOperation();
       op = op->getNextNode())
    if (!isMemoryEffectFree(op))
      return false;
  return true;
}

struct OutputGroup {
  SmallVector<unsigned> outputs;
  SmallVector<unsigned> inputs;
};

struct SelectGroup {
  unsigned condition;
  SmallVector<unsigned> selects;
  SmallVector<std::pair<unsigned, unsigned>> forwarded;
};

struct SelectSchedule {
  llvm::DenseMap<unsigned, SelectGroup> groups;
  llvm::BitVector delayed;
};

std::optional<unsigned> forwardedBits(
    const gfir::StructuredDefinition &definition, unsigned index) {
  const auto &wire = definition.wires[index];
  if (!isa<ac::BitsType>(wire.type))
    return std::nullopt;
  auto *op = wire.endpoint.value.getDefiningOp();
  unsigned input;
  if (wire.dependencies.size() == 1 &&
      isa_and_nonnull<ac::StructCreateOp, ac::StructGetOp>(op)) {
    input = wire.dependencies.front();
  } else if (auto select = dyn_cast_or_null<ac::BitsSelectOp>(op);
             select && wire.dependencies.size() == 3 &&
             select.getTrueValue() == select.getFalseValue()) {
    input = wire.dependencies[1];
  } else {
    return std::nullopt;
  }
  const auto &source = definition.wires[input];
  if (isa<ac::BitsType>(source.type) && wire.width == source.width)
    return input;
  return std::nullopt;
}

SelectSchedule planSelects(const gfir::StructuredDefinition &definition,
                           const llvm::BitVector &needed) {
  const unsigned count = definition.order.size();
  SelectSchedule result;
  result.delayed.resize(count);
  SmallVector<unsigned> origin(count), position(count), firstUse(count, count);
  for (auto [at, index] : llvm::enumerate(definition.order)) {
    position[index] = at;
    origin[index] = index;
    if (needed.test(index))
      if (auto source = forwardedBits(definition, index))
        origin[index] = origin[*source];
  }
  for (unsigned index : definition.order) {
    if (!needed.test(index) || origin[index] != index)
      continue;
    for (unsigned input : definition.dependencies(index)) {
      auto &use = firstUse[origin[input]];
      use = std::min(use, position[index]);
    }
  }

  struct Pending {
    SmallVector<unsigned> selects;
    unsigned deadline;
  };
  llvm::DenseMap<unsigned, Pending> pending;
  SmallVector<unsigned> scheduledAt(count, count);
  auto finish = [&](unsigned condition, Pending &group) {
    if (group.selects.size() >= 3) {
      unsigned last = group.selects.back();
      for (unsigned index : group.selects)
        scheduledAt[index] = last;
      result.groups.try_emplace(last, SelectGroup{condition, std::move(group.selects), {}});
    }
    group.selects.clear();
    group.deadline = count;
  };
  for (unsigned index : definition.order) {
    if (!needed.test(index) || origin[index] != index)
      continue;
    const auto &wire = definition.wires[index];
    if (wire.dependencies.size() != 3 ||
        !wire.endpoint.value.getDefiningOp<ac::BitsSelectOp>())
      continue;
    unsigned condition = origin[wire.dependencies[0]];
    auto &group = pending.try_emplace(condition, Pending{{}, count}).first->second;
    // Every member's operands will exist at the last select, and its result
    // must be published before the first consumer that actually computes.
    if (position[index] >= group.deadline)
      finish(condition, group);
    group.selects.push_back(index);
    group.deadline = std::min(group.deadline, firstUse[index]);
  }
  for (auto &entry : pending)
    finish(entry.first, entry.second);

  for (unsigned index : definition.order) {
    if (!needed.test(index))
      continue;
    unsigned source = origin[index];
    unsigned at = scheduledAt[source];
    if (at == count || position[index] >= position[at])
      continue;
    result.delayed.set(index);
    if (source != index)
      result.groups.find(at)->second.forwarded.emplace_back(index, source);
  }
  return result;
}

void emitSelectGroup(raw_ostream &out,
                     const gfir::StructuredDefinition &definition,
                     const SelectGroup &group) {
  for (unsigned index : group.selects)
    out << "    auto " << wireName(index) << " = "
        << carrier(definition.wires[index].width) << "::unknown();\n";
  const auto predicate = wireName(group.condition);
  out << "    if (" << predicate << ".isFullyKnown()) {\n"
      << "      if (" << predicate << ".value().toBool()) {\n";
  for (unsigned arm : {1u, 2u}) {
    if (arm == 2)
      out << "      } else {\n";
    for (unsigned index : group.selects)
      out << "        " << wireName(index) << " = "
          << wireName(definition.wires[index].dependencies[arm]) << ";\n";
  }
  out << "      }\n    } else {\n";
  for (unsigned index : group.selects) {
    const auto &inputs = definition.wires[index].dependencies;
    out << "      " << wireName(index) << " = gfsim::select(" << predicate << ", "
        << wireName(inputs[1]) << ", " << wireName(inputs[2]) << ");\n";
  }
  out << "    }\n";
  for (auto [index, source] : group.forwarded)
    out << "    const auto &" << wireName(index) << " = " << wireName(source) << ";\n";
}

struct WholeOutput {
  std::string type;
  // A local Rule result must outlive its frame. Committed state and a child's
  // prepared output already have the lifetime of this Work invocation.
  std::string source;
  bool snapshot;
};

std::string outputGroupName(unsigned index) {
  return "output_group_" + std::to_string(index);
}

std::string outputGroupType(unsigned index) {
  return "OutputGroup" + std::to_string(index);
}

void groupSignature(raw_ostream &out, const gfir::StructuredDefinition &definition,
                    const TableWireTypes &tables, const OutputGroup &group,
                    unsigned index, StringRef owner = {}) {
  out << owner << outputGroupType(index) << ' ' << owner
      << outputGroupName(index) << '(';
  llvm::interleaveComma(group.inputs, out, [&](unsigned input) {
    out << "const " << graphType(definition.wires[input], tables) << " &" << wireName(input);
  });
  out << ") const";
}

void preparationSignature(raw_ostream &out,
                          const gfir::StructuredDefinition &definition,
                          const TableWireTypes &tables,
                          StringRef owner = {}) {
  out << "void " << owner << "prepare(";
  bool comma = !definition.explicitDomains;
  if (comma)
    out << "const pyc_behavior::Level &pyc_clock, "
           "const pyc_behavior::Level &pyc_reset";
  for (unsigned input : definition.preparationInputs) {
    if (comma)
      out << ", ";
    out << "const " << graphType(definition.wires[input], tables) << " &" << wireName(input);
    comma = true;
  }
  if (definition.incomingState)
    out << ", const ChildTxnState &pyc_incoming, bool pyc_incoming_enabled";
  if (hasObservations(definition))
    out << ", const pyc_observation_sink *pyc_sink, std::size_t pyc_observation_base";
  out << ')';
}

class BehavioralEmitter {
public:
  BehavioralEmitter(ModuleOp package, const gfir::StructuredProgram &program)
      : program(program), analysis(package), common(package, analysis) {}
  FailureOr<FinalCppSourceParts> emit();

private:
  FailureOr<std::string> bitExpression(Operation *op, unsigned width,
                                        ArrayRef<std::string> args);
  FailureOr<std::string> expression(const gfir::StructuredWire &wire,
                                    const gfir::StructuredDefinition &definition,
                                    StringRef instancePrefix,
                                    const gfir::StructuredWire *packedTable = nullptr);
  LogicalResult emitWires(raw_ostream &out,
                          const gfir::StructuredDefinition &definition,
                          ArrayRef<unsigned> outputs, StringRef instancePrefix = {},
                          bool groupSelects = false);
  std::optional<unsigned> internOutputGroup(unsigned definition, ArrayRef<unsigned> outputs);
  FailureOr<bool> emitOutputGroup(raw_ostream &out,
                                  const gfir::StructuredDefinition &definition,
                                  unsigned first, ArrayRef<unsigned> candidates,
                                  llvm::BitVector &emitted, StringRef instancePrefix);
  LogicalResult emitOutputGroups();
  LogicalResult planWholeOutputs();
  void planEvaluationMemoization();
  LogicalResult declareModule(raw_ostream &out, unsigned index);
  LogicalResult emitModule(raw_ostream &out, unsigned index, raw_ostream *header);
  LogicalResult emitRoot(FinalCppSourceGroup &group, bool inlineWork);
  FailureOr<std::string> emitObservationMetadata(
      const gfir::StructuredDefinition &definition);
  FailureOr<FinalCppSourceGroup *> getGroup(gfir::ModuleOp module);
  FailureOr<std::string> typedWire(Type type, Operation *site);
  FailureOr<unsigned> fieldOffset(ac::StructGetOp get);
  std::string graphLeaf(const gfir::StructuredWire &wire, StringRef value) const;
  FailureOr<std::string> graphWire(Value value,
                                   const gfir::StructuredDefinition &definition,
                                   Operation *site,
                                   std::optional<ArrayRef<unsigned>> available = {});
  FailureOr<std::string> valueExpression(Value result, ArrayRef<std::string> args);
  FailureOr<std::string> tableMapExpression(ac::TableMapOp map, unsigned result,
                                           ArrayRef<std::string> args);
  bool isNativeScalar(Operation &op);
  FailureOr<std::string> nativeScalarExpression(Operation &op, unsigned width,
                                                ArrayRef<std::string> args);
  FailureOr<bool> emitScalarRegion(raw_ostream &out, ArrayRef<Operation *> operations,
                                   llvm::DenseMap<Value, std::string> &values);
  LogicalResult emitInitial(raw_ostream &out, gfir::VarOp variable, unsigned index);
  LogicalResult emitBody(raw_ostream &out, Block &body,
                         const gfir::StructuredDefinition &definition,
                         const gfir::StructuredRule *rule = nullptr,
                         const llvm::DenseMap<Value, std::string> *outer = nullptr,
                         ArrayRef<std::string> stepTargets = {});
  LogicalResult emitLoop(raw_ostream &out, gfir::LoopOp loop,
                         const gfir::StructuredDefinition &definition,
                         const llvm::DenseMap<Value, std::string> &values,
                         ArrayRef<std::string> results);
  LogicalResult emitBranch(raw_ostream &out, gfir::IfOp branch,
                           const gfir::StructuredDefinition &definition,
                           const llvm::DenseMap<Value, std::string> &values,
                           ArrayRef<std::string> results);
  LogicalResult emitBlockedReturn(raw_ostream &out, Block &body, bool ruleResults);
  LogicalResult declareClocked(raw_ostream &out, const gfir::StructuredDefinition &definition);
  FailureOr<std::string> recipient(const gfir::StructuredDefinition &definition,
                                    const gfir::StructuredFifo &fifo,
                                    std::optional<unsigned> rule = {}, bool ownPop = false);
  LogicalResult emitGrant(raw_ostream &out, const gfir::StructuredDefinition &definition,
                          unsigned grant, StringRef instancePrefix);
  FailureOr<SmallVector<std::string>>
  captureExpressions(const gfir::StructuredDefinition &definition,
                     const gfir::StructuredRule &rule);
  LogicalResult emitMemoizedRule(raw_ostream &out,
                                 const gfir::StructuredDefinition &definition);
  FailureOr<std::string> availabilityExpression(
      const gfir::StructuredDefinition &definition,
      ArrayRef<gfir::StructuredAvailabilitySource> sources,
      StringRef instancePrefix = {});
  LogicalResult emitClockedCall(raw_ostream &out,
                                const gfir::StructuredDefinition &definition,
                                unsigned index, StringRef instancePrefix);
  LogicalResult emitTemporalWork(raw_ostream &out,
                                 unsigned definition);
  LogicalResult emitPreparation(raw_ostream &out, const gfir::StructuredDefinition &definition,
                                StringRef clock);
  LogicalResult prepareMemory(raw_ostream &out, const gfir::StructuredDefinition &definition,
                              const gfir::StructuredMemory &memory, StringRef clock);
  LogicalResult emitLifecycle(raw_ostream &out, const gfir::StructuredDefinition &definition,
                              StringRef name, bool memoized, raw_ostream *header);

  const gfir::StructuredProgram &program;
  ac::HardwareAnalysis analysis;
  HardwareEmitContext common;
  TableWireTypes tableWireTypes;
  llvm::StringMap<unsigned> groups;
  FinalCppSourceParts parts;
  unsigned nextBranch = 0;
  unsigned nextLoop = 0;
  SmallVector<SmallVector<OutputGroup>> outputGroups;
  SmallVector<llvm::DenseMap<unsigned, WholeOutput>> wholeOutputs;
  SmallVector<unsigned> memoKeyWidths;
  SmallVector<uint64_t> groupWorkRemaining;
};

FailureOr<std::string>
BehavioralEmitter::expression(const gfir::StructuredWire &wire,
                             const gfir::StructuredDefinition &definition,
                             StringRef instancePrefix,
                             const gfir::StructuredWire *packedTable) {
  auto *op = wire.endpoint.value.getDefiningOp();
  SmallVector<std::string> args;
  for (unsigned dep : wire.dependencies)
    args.push_back(wireName(dep));
  if (wire.call) {
    auto member = instanceName(wire.call->receiver);
    if (failed(member))
      return failure();
    if (program.definitions[wire.call->definition].outputs[wire.call->output].requiresPreparation)
      args.clear();
    return instancePrefix.str() + *member + "." + functionName(wire.call->output) +
           "(" + join(args, ", ") + ")";
  }
  if (wire.fifoQuery) {
    auto [index, query] = *wire.fifoQuery;
    const auto &fifo = definition.clocked->fifos[index];
    auto name = instanceName(fifo.instance);
    if (failed(name))
      return failure();
    std::string read = instancePrefix.str() + *name;
    if (query == gfir::StructuredFifoQuery::Data)
      return graphLeaf(wire, read + ".data()");
    if (query == gfir::StructuredFifoQuery::Valid)
      return read + ".valid().packed()";
    auto ready = fifo.downstreamPop
        ? recipient(definition, fifo)
        : FailureOr<std::string>(levelName.str() + "(false)");
    if (failed(ready))
      return failure();
    return read + ".ready(" + *ready + ").packed()";
  }
  if (wire.memory) {
    const auto &memory = definition.clocked->memories[*wire.memory];
    auto name = instanceName(memory.instance);
    if (failed(name))
      return failure();
    std::string read = instancePrefix.str() + *name;
    if (memory.asynchronous()) {
      auto raw = memory.raw;
      auto address = graphWire(raw.getInputs().front(), definition, raw);
      if (failed(address))
        return failure();
      read += ".read(" + *address + ")";
    } else {
      unsigned port = cast<OpResult>(wire.endpoint.value).getResultNumber();
      read += ".result(" + std::to_string(port) + ")";
    }
    return graphLeaf(wire, read);
  }
  if (wire.state)
    return graphLeaf(wire, instancePrefix.str() + stateName(*wire.state) + ".value()");
  if (wire.ruleResult) {
    auto [rule, result] = *wire.ruleResult;
    return graphLeaf(wire, frameName(rule) + "." + resultName(result));
  }
  if (packedTable) {
    auto get = cast<ac::TableGetOp>(op);
    auto key = graphWire(get.getIndex(), definition, get, wire.dependencies);
    if (failed(key))
      return failure();
    auto type = graphType(*packedTable, tableWireTypes);
    auto state = instancePrefix.str() + stateName(*packedTable->state) + ".value()";
    auto value = "std::get<0>(pyc_behavior::lookupPacked<" + type + ">(" +
                 pack(state) + ", " + std::to_string(packedTable->low) + ", " +
                 *key + "))";
    return graphLeaf(wire, value);
  }
  if (isa<ac::TableSplatOp, ac::TableGetOp, ac::TableMapOp>(op)) {
    SmallVector<std::string> operands;
    for (Value value : op->getOperands()) {
      auto operand = graphWire(value, definition, op, wire.dependencies);
      if (failed(operand))
        return failure();
      operands.push_back(std::move(*operand));
    }
    auto value = valueExpression(wire.endpoint.value, operands);
    if (failed(value))
      return failure();
    return graphLeaf(wire, *value);
  }
  if (isa<ac::StructCreateOp, ac::StructGetOp>(op))
    return args.front();
  return bitExpression(op, wire.width, args);
}

FailureOr<std::string>
BehavioralEmitter::bitExpression(Operation *op, unsigned bitWidth,
                                 ArrayRef<std::string> args) {
  if (auto constant = dyn_cast<ac::BitsConstant4Op>(op))
    return constant4(constant.getPattern());
  if (auto constant = dyn_cast<ac::BitsConstantOp>(op)) {
    auto number = integer(constant.getValue(), analysis, op);
    if (failed(number))
      return failure();
    return carrier(bitWidth) + "::known(" +
           bitWords(number->zextOrTrunc(bitWidth)) + ")";
  }
  auto call = [&](StringRef name) {
    return "gfsim::" + name.str() + "(" + join(args, ", ") + ")";
  };
  if (isa<ac::BitsUnaryOp>(op))
    return call("bit_not");
  if (auto binary = dyn_cast<ac::BitsBinaryOp>(op))
    return call(llvm::StringSwitch<StringRef>(binary.getOpcode())
                    .Case("and", "bit_and")
                    .Case("or", "bit_or")
                    .Case("xor", "bit_xor")
                    .Default(binary.getOpcode()));
  if (auto compare = dyn_cast<ac::BitsCompareOp>(op))
    return call("compare<gfsim::ComparePredicate::" +
                compare.getPredicate().upper() + ">");
  if (auto select = dyn_cast<ac::BitsSelectOp>(op)) {
    if (select.getTrueValue() == select.getFalseValue())
      return args[1];
    return call("select");
  }
  if (isa<ac::BitsConcatOp>(op))
    return concatenate(args);
  std::string width = std::to_string(bitWidth);
  if (auto extract = dyn_cast<ac::BitsExtractOp>(op)) {
    auto low = integer(extract.getLow(), analysis, op);
    if (failed(low))
      return failure();
    return "pyc_behavior::slice<" + width + ">(" + args.front() + ", " +
           std::to_string(low->getZExtValue()) + ")";
  }
  if (auto resize = dyn_cast<ac::BitsResizeOp>(op)) {
    auto mode = llvm::StringSwitch<StringRef>(resize.getMode())
                    .Case("trunc", "truncate")
                    .Case("zext", "zero_extend")
                    .Case("sext", "sign_extend")
                    .Default("");
    return call(mode.str() + "<" + width + ">");
  }
  return op->emitOpError("operation lacks behavioral C++ emission");
}

bool BehavioralEmitter::isNativeScalar(Operation &op) {
  if (auto constant = dyn_cast<ac::BitsConstant4Op>(op)) {
    if (constant.getPattern().find_first_not_of("01") != StringRef::npos)
      return false;
  } else if (auto binary = dyn_cast<ac::BitsBinaryOp>(op)) {
    if (!llvm::is_contained(
            ArrayRef<StringRef>{"add", "sub", "mul", "and", "or", "xor"},
            binary.getOpcode()))
      return false;
  } else if (!isa<ac::BitsConstantOp, ac::BitsUnaryOp, ac::BitsCompareOp,
                  ac::BitsSelectOp, ac::BitsConcatOp, ac::BitsExtractOp,
                  ac::BitsResizeOp>(op)) {
    return false;
  }
  auto fits = [&](Type type) {
    auto width = analysis.getPackedWidth(type, {}, &op);
    return succeeded(width) && *width > 0 && *width <= 64;
  };
  return llvm::all_of(op.getResultTypes(), fits) &&
         llvm::all_of(op.getOperandTypes(), fits);
}

FailureOr<std::string> BehavioralEmitter::nativeScalarExpression(
    Operation &op, unsigned width, ArrayRef<std::string> args) {
  auto operandWidth = [&](Value operand) {
    return analysis.getPackedWidth(operand.getType(), {}, &op);
  };
  if (auto constant = dyn_cast<ac::BitsConstantOp>(op)) {
    auto value = integer(constant.getValue(), analysis, &op);
    if (failed(value))
      return failure();
    return std::to_string(value->zextOrTrunc(width).getZExtValue()) + "ULL";
  }
  if (auto constant = dyn_cast<ac::BitsConstant4Op>(op)) {
    uint64_t value;
    if (constant.getPattern().getAsInteger(2, value))
      return constant.emitOpError("invalid known scalar literal");
    return std::to_string(value) + "ULL";
  }
  if (isa<ac::BitsUnaryOp>(op))
    return "~" + args.front();
  if (auto binary = dyn_cast<ac::BitsBinaryOp>(op)) {
    auto token = llvm::StringSwitch<StringRef>(binary.getOpcode())
                     .Case("add", "+").Case("sub", "-").Case("mul", "*")
                     .Case("and", "&").Case("or", "|").Case("xor", "^").Default("");
    return args[0] + " " + token.str() + " " + args[1];
  }
  if (auto compare = dyn_cast<ac::BitsCompareOp>(op)) {
    auto predicate = compare.getPredicate();
    std::string lhs = args[0], rhs = args[1];
    if (predicate.starts_with("s")) {
      auto bits = operandWidth(compare.getLhs());
      if (failed(bits))
        return failure();
      auto sign = "(std::uint64_t{1} << " + std::to_string(*bits - 1) + ")";
      lhs = "(" + lhs + " ^ " + sign + ")";
      rhs = "(" + rhs + " ^ " + sign + ")";
    }
    if (predicate.starts_with("u") || predicate.starts_with("s"))
      predicate = predicate.drop_front();
    auto token = llvm::StringSwitch<StringRef>(predicate)
                     .Cases("eq", "case_eq", "==").Case("ne", "!=")
                     .Case("lt", "<").Case("le", "<=").Case("gt", ">").Case("ge", ">=")
                     .Default("");
    return lhs + " " + token.str() + " " + rhs;
  }
  if (auto select = dyn_cast<ac::BitsSelectOp>(op)) {
    if (select.getTrueValue() == select.getFalseValue())
      return args[1];
    return args[0] + " ? " + args[1] + " : " + args[2];
  }
  if (auto concat = dyn_cast<ac::BitsConcatOp>(op)) {
    std::string result = args.front();
    for (unsigned i = 1; i < args.size(); ++i) {
      auto bits = operandWidth(concat.getOperand(i));
      if (failed(bits))
        return failure();
      result = "((" + result + " << " + std::to_string(*bits) + ") | " + args[i] + ")";
    }
    return result;
  }
  if (auto extract = dyn_cast<ac::BitsExtractOp>(op)) {
    auto low = integer(extract.getLow(), analysis, &op);
    if (failed(low))
      return failure();
    return args.front() + " >> " + std::to_string(low->getZExtValue());
  }
  auto resize = cast<ac::BitsResizeOp>(op);
  if (resize.getMode() == "sext") {
    auto bits = operandWidth(resize.getInput());
    if (failed(bits))
      return failure();
    return "pyc_behavior::extendSign<" + std::to_string(*bits) + ">(" + args.front() + ")";
  }
  return args.front();
}

FailureOr<bool> BehavioralEmitter::emitScalarRegion(
    raw_ostream &out, ArrayRef<Operation *> operations,
    llvm::DenseMap<Value, std::string> &values) {
  llvm::SmallDenseSet<Operation *> members(operations.begin(), operations.end());
  llvm::SmallDenseSet<Value> seenInputs;
  SmallVector<Value> inputs, outputs;
  for (Operation *op : operations) {
    for (Value input : op->getOperands())
      if (!members.contains(input.getDefiningOp()) && seenInputs.insert(input).second)
        inputs.push_back(input);
    Value result = op->getResult(0);
    if (llvm::any_of(result.getUsers(),
                     [&](Operation *user) { return !members.contains(user); }))
      outputs.push_back(result);
  }
  if (inputs.empty() || outputs.empty())
    return false;

  llvm::DenseMap<Value, std::string> local;
  SmallVector<std::string> parameters, arguments, known, exported;
  for (auto [index, input] : llvm::enumerate(inputs)) {
    auto name = "pyc_scalar_input_" + std::to_string(index);
    parameters.push_back("const auto &" + name);
    arguments.push_back(pack(values.lookup(input)));
    known.push_back(name + ".isFullyKnown()");
    local.try_emplace(input, std::move(name));
  }
  for (Value output : outputs) {
    exported.push_back(wireName(values.size()));
    values.try_emplace(output, exported.back());
  }
  llvm::DenseMap<Value, unsigned> widths;
  for (Operation *op : operations) {
    Value result = op->getResult(0);
    auto width = analysis.getPackedWidth(result.getType(), {}, op);
    if (failed(width))
      return failure();
    widths.try_emplace(result, *width);
    local.try_emplace(result, "pyc_scalar_" + std::to_string(local.size()));
  }

  auto emit = [&](bool native) -> LogicalResult {
    llvm::DenseMap<Value, std::string> singleUse;
    for (Operation *op : operations) {
      Value result = op->getResult(0);
      unsigned width = widths.lookup(result);
      SmallVector<std::string> operands;
      for (Value input : op->getOperands()) {
        if (auto expression = singleUse.find(input); expression != singleUse.end()) {
          operands.push_back(expression->second);
          continue;
        }
        auto argument = local.lookup(input);
        if (native && seenInputs.contains(input))
          argument += ".value().value()";
        operands.push_back(std::move(argument));
      }
      auto expression = native ? nativeScalarExpression(*op, width, operands)
                               : bitExpression(op, width, operands);
      if (failed(expression))
        return failure();
      if (native) {
        *expression = "pyc_behavior::limitBits<" + std::to_string(width) + ">(" +
                      *expression + ")";
        // Expressions used only in one arm stay inside that arm's conditional.
        if (result.hasOneUse() && members.contains(*result.getUsers().begin()) &&
            !llvm::is_contained(outputs, result)) {
          singleUse.try_emplace(result, std::move(*expression));
          continue;
        }
      }
      out << "        const auto " << local.lookup(result) << " = " << *expression << ";\n";
    }
    SmallVector<std::string> returned;
    for (Value result : outputs) {
      unsigned width = widths.lookup(result);
      auto value = local.lookup(result);
      if (native)
        value = carrier(width) + "::known(gfsim::Bits<" + std::to_string(width) + ">{" +
                value + "})";
      auto type = typedWire(result.getType(), result.getDefiningOp());
      if (failed(type))
        return failure();
      returned.push_back(unpack(*type, value));
    }
    out << "        return std::tuple{" << join(returned, ", ") << "};\n";
    return success();
  };

  out << "    const auto [" << join(exported, ", ") << "] = []("
      << join(parameters, ", ") << ") {\n"
      << "      if (" << join(known, " && ") << ") {\n";
  if (failed(emit(true)))
    return failure();
  out << "      }\n      return pyc_behavior::evaluateFourState([]("
      << join(parameters, ", ") << ") {\n";
  if (failed(emit(false)))
    return failure();
  SmallVector<std::string> names;
  for (Value input : inputs)
    names.push_back(local.lookup(input));
  out << "      }, " << join(names, ", ") << ");\n"
      << "    }(" << join(arguments, ", ") << ");\n";
  return true;
}

LogicalResult BehavioralEmitter::emitWires(
    raw_ostream &out, const gfir::StructuredDefinition &definition,
    ArrayRef<unsigned> outputs, StringRef instancePrefix, bool groupSelects) {
  llvm::DenseMap<unsigned, unsigned> packedLookups;
  auto needed = wireClosure(definition, outputs, [&](unsigned index, unsigned input) {
    if (index >= definition.wires.size() || input >= definition.wires.size() ||
        !isa<ac::TableType>(definition.wires[input].type))
      return false;
    auto result = dyn_cast<OpResult>(definition.wires[index].endpoint.value);
    if (!result || result.getResultNumber() != 0)
      return false;
    auto get = dyn_cast<ac::TableGetOp>(result.getOwner());
    if (!get || definition.wires[input].endpoint.value != get.getInput())
      return false;
    if (auto origin = packedStateTable(definition, input)) {
      packedLookups.try_emplace(index, *origin);
      return true;
    }
    return false;
  });
  SelectSchedule selectSchedule;
  if (groupSelects)
    selectSchedule = planSelects(definition, needed);
  llvm::BitVector emitted(definition.order.size());
  for (auto [index, wire] : llvm::enumerate(definition.wires))
    if (isa<BlockArgument>(wire.endpoint.value))
      emitted.set(index);
  llvm::DenseMap<Operation *, SmallVector<unsigned>> callMembers;
  for (unsigned index : definition.order)
    if (index < definition.wires.size() && needed.test(index))
      if (const auto &call = definition.wires[index].call)
        callMembers[call->receiver].push_back(index);
  for (unsigned index : definition.order) {
    if (!needed.test(index) || emitted.test(index))
      continue;
    if (groupSelects && selectSchedule.delayed.test(index))
      continue;
    if (auto group = selectSchedule.groups.find(index); group != selectSchedule.groups.end()) {
      emitSelectGroup(out, definition, group->second);
      for (unsigned member : group->second.selects)
        emitted.set(member);
      for (const auto &alias : group->second.forwarded)
        emitted.set(alias.first);
      continue;
    }
    if (index >= definition.firstClockedCallNode()) {
      if (failed(emitClockedCall(out, definition,
                                 index - definition.firstClockedCallNode(), instancePrefix)))
        return failure();
      emitted.set(index);
      continue;
    }
    if (index >= definition.wires.size()) {
      if (failed(emitGrant(out, definition, index, instancePrefix)))
        return failure();
      emitted.set(index);
      continue;
    }
    const auto &wire = definition.wires[index];
    if (isa<BlockArgument>(wire.endpoint.value))
      continue;
    auto candidates = wire.call ? ArrayRef<unsigned>(callMembers[wire.call->receiver])
                                : ArrayRef<unsigned>{};
    auto grouped = emitOutputGroup(out, definition, index, candidates, emitted, instancePrefix);
    if (failed(grouped))
      return failure();
    if (*grouped)
      continue;
    auto lookup = packedLookups.find(index);
    auto value = expression(wire, definition, instancePrefix,
                            lookup == packedLookups.end() ? nullptr
                                                         : &definition.wires[lookup->second]);
    if (failed(value))
      return failure();
    const bool borrow =
        (groupSelects && forwardedBits(definition, index).has_value()) ||
        (isa<ac::TableType>(wire.type) &&
         isa_and_nonnull<ac::StructCreateOp, ac::StructGetOp, ac::TableMapOp>(
             wire.endpoint.value.getDefiningOp()));
    out << "    const auto " << (borrow ? "&" : "") << wireName(index)
        << " = " << *value << ";\n";
    emitted.set(index);
  }
  return success();
}

std::optional<unsigned> BehavioralEmitter::internOutputGroup(
    unsigned index, ArrayRef<unsigned> outputs) {
  auto &groups = outputGroups[index];
  for (auto [ordinal, group] : llvm::enumerate(groups))
    if (llvm::equal(group.outputs, outputs))
      return ordinal;
  const auto &definition = program.definitions[index];
  // Bound specialization count and total added closure size by the existing
  // singleton surface; a refused specialization keeps ordinary emission.
  if (groups.size() >= definition.outputs.size())
    return std::nullopt;
  OutputGroup group;
  group.outputs.append(outputs.begin(), outputs.end());
  SmallVector<unsigned> roots;
  for (unsigned output : outputs) {
    const auto &field = definition.outputs[output];
    roots.push_back(field.wire);
    group.inputs.append(field.inputs.begin(), field.inputs.end());
  }
  auto work = wireClosure(definition, roots).count();
  if (work > groupWorkRemaining[index])
    return std::nullopt;
  groupWorkRemaining[index] -= work;
  llvm::sort(group.inputs);
  group.inputs.erase(std::unique(group.inputs.begin(), group.inputs.end()), group.inputs.end());
  groups.push_back(std::move(group));
  return groups.size() - 1;
}

FailureOr<bool> BehavioralEmitter::emitOutputGroup(
    raw_ostream &out, const gfir::StructuredDefinition &definition,
    unsigned first, ArrayRef<unsigned> candidates, llvm::BitVector &emitted,
    StringRef instancePrefix) {
  const auto &initial = definition.wires[first];
  if (!initial.call)
    return false;
  const auto &call = *initial.call;
  const auto &child = program.definitions[call.definition];
  if (isClocked(child) || child.hasChecks())
    return false;
  SmallVector<unsigned> members;
  SmallVector<unsigned> outputs;
  std::map<unsigned, unsigned> frontier;
  for (unsigned index : candidates) {
    if (emitted.test(index))
      continue;
    const auto &wire = definition.wires[index];
    if (!wire.call || wire.call->receiver != call.receiver || wire.call->definition != call.definition)
      continue;
    const auto &output = child.outputs[wire.call->output];
    if (output.requiresPreparation || output.inputs.size() != wire.dependencies.size() ||
        !llvm::all_of(wire.dependencies, [&](unsigned input) { return emitted.test(input); }))
      continue;
    // Actual SSA bindings, rather than port names or declaration order, own
    // the union frontier. Different invocation bindings must remain separate.
    for (auto [input, actual] : llvm::zip(output.inputs, wire.dependencies)) {
      auto [found, inserted] = frontier.emplace(input, actual);
      if (!inserted && found->second != actual)
        return false;
    }
    members.push_back(index);
    outputs.push_back(wire.call->output);
  }
  llvm::sort(outputs);
  outputs.erase(std::unique(outputs.begin(), outputs.end()), outputs.end());
  if (outputs.size() < 2 || !llvm::is_contained(members, first))
    return false;
  auto group = internOutputGroup(call.definition, outputs);
  if (!group)
    return false;
  auto member = instanceName(call.receiver);
  if (failed(member))
    return failure();
  std::map<unsigned, unsigned> bindings;
  for (unsigned index : members)
    bindings.try_emplace(definition.wires[index].call->output, index);
  SmallVector<std::string> results, arguments;
  for (unsigned output : outputs)
    results.push_back(wireName(bindings.at(output)));
  for (unsigned input : outputGroups[call.definition][*group].inputs)
    arguments.push_back(wireName(frontier.at(input)));
  out << "    const auto [" << join(results, ", ") << "] = " << instancePrefix << *member
      << '.' << outputGroupName(*group) << '(' << join(arguments, ", ") << ");\n";
  for (unsigned index : members) {
    unsigned original = bindings.at(definition.wires[index].call->output);
    if (index != original)
      out << "    const auto &" << wireName(index) << " = " << wireName(original) << ";\n";
    emitted.set(index);
  }
  return true;
}

LogicalResult BehavioralEmitter::emitOutputGroups() {
  // Definitions are callee-before-caller. Descending discovery allows each
  // helper body to register nested groups before its child is emitted.
  for (unsigned index = program.definitions.size(); index-- != 0;) {
    const auto &definition = program.definitions[index];
    auto group = getGroup(definition.module);
    if (failed(group))
      return failure();
    llvm::raw_string_ostream out((*group)->source);
    bool groupSelects = !isClocked(definition) &&
        llvm::none_of(definition.wires, [](const gfir::StructuredWire &wire) {
          return wire.call.has_value();
        });
    for (unsigned ordinal = 0; ordinal < outputGroups[index].size(); ++ordinal) {
      const auto &outputs = outputGroups[index][ordinal];
      groupSignature(out, definition, tableWireTypes, outputs, ordinal,
                     moduleName(index) + "::");
      out << " {\n";
      SmallVector<unsigned> roots;
      SmallVector<std::string> values;
      for (unsigned output : outputs.outputs) {
        unsigned wire = definition.outputs[output].wire;
        roots.push_back(wire);
        values.push_back(wireName(wire));
      }
      if (failed(emitWires(out, definition, roots, {}, groupSelects)))
        return failure();
      out << "    return {" << join(values, ", ") << "};\n}\n";
    }
  }
  return success();
}

LogicalResult BehavioralEmitter::planWholeOutputs() {
  wholeOutputs.resize(program.definitions.size());
  for (auto [index, definition] : llvm::enumerate(program.definitions)) {
    auto module = definition.module;
    auto yield = cast<gfir::YieldOp>(module.getBody().front().getTerminator());
    for (auto [port, value] : llvm::enumerate(yield.getValues())) {
      const auto resultType = module.getFunctionType().getResult(port);
      if (!isa<ac::StructType>(resultType) || resultType != value.getType())
        continue;
      const auto &leaves = definition.valueWires.find(value)->second;
      const auto &first = definition.wires[leaves.front()];
      std::string source;
      bool snapshot = false;
      if (first.state && llvm::all_of(leaves, [&](unsigned leaf) {
            return definition.wires[leaf].state == first.state;
          })) {
        source = stateName(*first.state) + ".value()";
      } else if (first.ruleResult && llvm::all_of(leaves, [&](unsigned leaf) {
                   return definition.wires[leaf].ruleResult == first.ruleResult;
                 })) {
        auto [rule, result] = *first.ruleResult;
        source = frameName(rule) + "." + resultName(result);
        snapshot = true;
      } else if (first.call) {
        const auto &call = *first.call;
        const auto &child = program.definitions[call.definition];
        const auto &output = child.outputs[call.output];
        const unsigned childPort = output.port.port;
        auto callee = child.module;
        // A prepared child value may only be forwarded through a single
        // preparation of this receiver. Field availability is still separate.
        if (!output.requiresPreparation ||
            !wholeOutputs[call.definition].contains(childPort) ||
            callee.getFunctionType().getResult(childPort) != resultType ||
            llvm::count_if(definition.clockedCalls, [&](const auto &prepare) {
              return prepare.receiver == call.receiver;
            }) != 1 ||
            !llvm::all_of(leaves, [&](unsigned leaf) {
              const auto &wire = definition.wires[leaf];
              if (!wire.call || wire.call->receiver != call.receiver ||
                  wire.call->definition != call.definition)
                return false;
              const auto &field = child.outputs[wire.call->output];
              return field.requiresPreparation && field.port.port == childPort &&
                     field.port.path == wire.endpoint.path;
            }))
          continue;
        auto member = instanceName(call.receiver);
        if (failed(member))
          return failure();
        source = *member + "." + wholeOutputName(childPort) + "()";
      } else {
        continue;
      }
      auto type = typedWire(resultType, module);
      if (failed(type))
        return failure();
      wholeOutputs[index].try_emplace(port, WholeOutput{std::move(*type),
                                                       std::move(source), snapshot});
    }
  }
  return success();
}

void BehavioralEmitter::planEvaluationMemoization() {
  memoKeyWidths.resize(program.definitions.size());
  for (auto [index, definition] : llvm::enumerate(program.definitions)) {
    if (!definition.clocked || definition.explicitDomains || definition.hasChecks() ||
        definition.incomingState || !definition.clockedCalls.empty() ||
        !definition.childStates.empty())
      continue;
    const auto &plan = *definition.clocked;
    if (plan.rules.size() != 1 || plan.variables.empty() || !plan.fifos.empty() ||
        !plan.memories.empty() || !plan.childMethodCalls.empty())
      continue;
    const auto &rule = plan.rules.front();
    if (rule.sampled || rule.resultUses.empty() || !rule.requirements.empty() ||
        !rule.guardCaptures.empty() || !rule.assertions.empty() ||
        !rule.observations.empty())
      continue;
    // Other outputs may read committed state via the ordinary getters.
    if (llvm::any_of(definition.outputs, [&](const gfir::StructuredOutput &output) {
          return output.requiresPreparation && !wholeOutputs[index].contains(output.port.port);
        }))
      continue;

    uint64_t keyWidth = 0;
    for (unsigned input : definition.preparationInputs) {
      const auto &wire = definition.wires[input];
      if (isa<ac::TableType>(wire.type)) {
        keyWidth = 0;
        break;
      }
      keyWidth += wire.width;
    }
    if (!keyWidth || keyWidth > std::numeric_limits<unsigned>::max())
      continue;

    // Limit the first profile to call-heavy previews. In a split Rule, only
    // the captures are reusable because pyc_active still affects its frame.
    bool hasCall = false;
    if (rule.afterSnapshot) {
      auto closure = wireClosure(definition, {unsigned(definition.wires.size())});
      for (auto [wire, value] : llvm::enumerate(definition.wires))
        hasCall |= closure.test(wire) && value.call.has_value();
    } else {
      auto operation = rule.operation;
      hasCall = operation->walk([&](gfir::CallOp call) {
        return plan.methodCalls.contains(call) ? WalkResult::interrupt()
                                               : WalkResult::advance();
      }).wasInterrupted();
    }
    if (hasCall)
      memoKeyWidths[index] = keyWidth;
  }
}

LogicalResult BehavioralEmitter::declareModule(raw_ostream &out, unsigned index) {
  const auto &definition = program.definitions[index];
  out << "struct " << moduleName(index) << " {\n";
  for (unsigned result = 0; result < definition.outputs.size(); ++result) {
    out << "  ";
    signature(out, definition, tableWireTypes, result);
    out << ";\n";
  }
  for (auto [ordinal, group] : llvm::enumerate(outputGroups[index])) {
    out << "  struct " << outputGroupType(ordinal) << " {\n";
    for (unsigned output : group.outputs)
      out << "    " << graphType(definition.wires[definition.outputs[output].wire], tableWireTypes)
          << ' ' << resultName(output) << ";\n";
    out << "  };\n";
    out << "  ";
    groupSignature(out, definition, tableWireTypes, group, ordinal);
    out << ";\n";
  }
  for (const auto &output : definition.outputAvailability)
    out << "  bool " << availabilityName(output.port) << " = false;\n";
  for (auto [index, field] : llvm::enumerate(definition.fieldOutputs))
    if (!field.sources.empty())
      out << "  bool " << fieldAvailabilityName(index) << " = false;\n";
  if (definition.incomingState) {
    unsigned state = *definition.incomingState;
    auto variable = definition.clocked->variables[state];
    auto type = typedWire(variable.getInitial().getType(), variable);
    if (failed(type))
      return failure();
    out << "  struct ChildTxnState {\n"
        << "    std::optional<" << *type << "> " << stateName(state) << ";\n"
        << "  };\n";
  }
  if (isClocked(definition)) {
    out << "  ";
    preparationSignature(out, definition, tableWireTypes);
    out << ";\n  void reset() noexcept;\n  void discard() noexcept;\n"
           "  void commit() noexcept;\n";
  }
  if (definition.hasChecks())
    out << "  void configure_checks(std::string instance);\n"
           "  bool validate_sampling(gfsim::SimFailureInfo &failure) const noexcept;\n"
           "  bool validate_checks(gfsim::SimFailureInfo &failure) const noexcept;\n";
  if (hasAssertions(definition))
    out << "  bool validate_assertions(gfsim::SimFailureInfo &failure) const noexcept;\n";
  llvm::SmallDenseSet<Operation *> seen;
  for (const auto &wire : definition.wires) {
    if (!wire.call || !seen.insert(wire.call->receiver).second)
      continue;
    auto member = instanceName(wire.call->receiver);
    if (failed(member))
      return failure();
    out << "  " << moduleName(wire.call->definition) << " " << *member << ";\n";
  }
  for (const auto &call : definition.clockedCalls) {
    if (!seen.insert(call.receiver).second)
      continue;
    auto member = instanceName(call.receiver);
    if (failed(member))
      return failure();
    out << "  " << moduleName(call.definition) << ' ' << *member << ";\n";
  }
  if (definition.clocked && failed(declareClocked(out, definition)))
    return failure();
  const auto &wide = wholeOutputs[index];
  auto module = definition.module;
  for (unsigned port = 0, count = module.getFunctionType().getNumResults();
       port < count; ++port) {
    auto whole = wide.find(port);
    if (whole == wide.end())
      continue;
    const auto &value = whole->second;
    out << "  const " << value.type << " &" << wholeOutputName(port)
        << "() const noexcept { return "
        << (value.snapshot ? wholeSnapshotName(port) : value.source) << "; }\n";
  }
  if (isClocked(definition)) {
    out << "private:\n";
    if (unsigned width = memoKeyWidths[index]) {
      const auto &rule = definition.clocked->rules.front();
      out << "  std::optional<" << carrier(width) << "> pyc_memo_key;\n";
      if (rule.afterSnapshot) {
        auto operation = rule.operation;
        SmallVector<std::string> types;
        for (Value value : operation.getCaptures()) {
          auto type = typedWire(value.getType(), operation);
          if (failed(type))
            return failure();
          types.push_back(std::move(*type));
        }
        out << "  std::optional<std::tuple<" << join(types, ", ") << ">> pyc_memo_values;\n";
      } else {
        out << "  std::optional<TxnFrame0> pyc_memo_frame;\n";
      }
    }
    if (definition.hasChecks())
      out << "  bool pyc_checks_valid = false;\n"
             "  gfsim::SampledClockFault pyc_sampling_fault = gfsim::SampledClockFault::None;\n"
             "  std::string pyc_check_instance;\n"
          << "  unsigned pyc_failed_rule = " << noCheck << ";\n"
          << "  unsigned pyc_failed_check = " << noCheck << ";\n";
    if (hasAssertions(definition))
      for (auto [ruleIndex, rule] : llvm::enumerate(definition.clocked->rules))
        for (auto assertion : rule.assertions)
          out << "  bool " << assertionName(ruleIndex,
              definition.clocked->checkOffsets.lookup(assertion)) << " = false;\n";
    for (auto [result, output] : llvm::enumerate(definition.outputs)) {
      if (!output.requiresPreparation || wide.contains(output.port.port))
        continue;
      auto type = graphType(definition.wires[output.wire], tableWireTypes);
      out << "  " << type << ' ' << preparedName(result) << " = " << type
          << "::unknown();\n";
    }
    for (unsigned port = 0, count = module.getFunctionType().getNumResults();
         port < count; ++port) {
      auto whole = wide.find(port);
      if (whole != wide.end() && whole->second.snapshot)
        out << "  " << whole->second.type << ' ' << wholeSnapshotName(port) << ";\n";
    }
  }
  out << "};\n";
  return success();
}

LogicalResult BehavioralEmitter::emitModule(raw_ostream &out, unsigned index,
                                            raw_ostream *header) {
  const auto &definition = program.definitions[index];
  const auto &wide = wholeOutputs[index];
  for (auto [result, output] : llvm::enumerate(definition.outputs)) {
    signature(out, definition, tableWireTypes, result, moduleName(index) + "::");
    out << " {\n";
    if (wide.contains(output.port.port)) {
      auto value = graphLeaf(definition.wires[output.wire],
                             wholeOutputName(output.port.port) + "()");
      out << "    return " << value << ";\n}\n";
    } else if (output.requiresPreparation) {
      out << "    return " << preparedName(result) << ";\n}\n";
    } else {
      if (failed(emitWires(out, definition, {output.wire})))
        return failure();
      out << "    return " << wireName(output.wire) << ";\n}\n";
    }
  }
  if (isClocked(definition)) {
    auto &prepare = header ? *header : out;
    if (header)
      prepare << "inline ";
    preparationSignature(prepare, definition, tableWireTypes, moduleName(index) + "::");
    prepare << " {\n";
    if (failed(emitTemporalWork(prepare, index)))
      return failure();
    prepare << "}\n";
    if (failed(emitLifecycle(out, definition, moduleName(index),
                             memoKeyWidths[index] != 0, header)))
      return failure();
  }
  if (definition.hasChecks()) {
    out << "void " << moduleName(index) << "::configure_checks(std::string instance) {\n"
           "  pyc_check_instance = std::move(instance);\n";
    for (const auto &call : definition.clockedCalls) {
      if (!program.definitions[call.definition].hasChecks())
        continue;
      auto member = instanceName(call.receiver);
      if (failed(member))
        return failure();
      auto receiver = call.receiver;
      out << "  " << *member << ".configure_checks(pyc_check_instance + "
          << cppString("." + receiver.getSymName().str()) << ");\n";
    }
    out << "}\n";
    out << "bool " << moduleName(index)
        << "::validate_sampling(gfsim::SimFailureInfo &failure) const noexcept {\n"
           "  if (!pyc_checks_valid) {\n"
           "    failure = {gfsim::SimFailurePhase::Check, \"runtime_failure\", "
           "\"model check failed\", {}, {}, {}};\n    return false;\n  }\n"
           "  if (pyc_sampling_fault != gfsim::SampledClockFault::None) {\n"
           "    failure = {gfsim::SimFailurePhase::Check, \"runtime_failure\", "
           "pyc_sampling_fault == gfsim::SampledClockFault::UnknownClock ? "
           "\"sampled clock is unknown\" : \"sampled reset is unknown\", {}, {}, {}};\n"
           "    return false;\n  }\n";
    for (const auto &call : definition.clockedCalls) {
      if (!program.definitions[call.definition].hasChecks())
        continue;
      auto member = instanceName(call.receiver);
      if (failed(member))
        return failure();
      out << "  if (!" << *member << ".validate_sampling(failure)) return false;\n";
    }
    out << "  return true;\n}\n";
    out << "bool " << moduleName(index)
        << "::validate_checks(gfsim::SimFailureInfo &failure) const noexcept {\n";
    if (definition.clocked)
      for (auto [ruleIndex, rule] : llvm::enumerate(definition.clocked->rules)) {
        if (!rule.sampled)
          continue;
        out << "  if (pyc_failed_rule == " << ruleIndex << ") {\n"
               "    switch (pyc_failed_check) {\n";
        auto operation = rule.operation;
        for (auto [ordinal, check] : llvm::enumerate(
                 ArrayRef<gfir::StructuredCheckSite>(rule.checks)
                     .drop_back(rule.assertions.size()))) {
          auto id = emissionMetadataJson(check.id, operation);
          auto location = emissionMetadataJson(check.location, operation);
          if (failed(id) || failed(location))
            return failure();
          out << "    case " << ordinal << ":\n"
                 "      failure = {gfsim::SimFailurePhase::Check, \"source_check_failed\", "
                 "\"source guard_known check failed\", pyc_check_instance, "
              << cppString(*location) << ", " << cppString(*id)
              << "};\n      return false;\n";
        }
        out << "    }\n  }\n";
      }
    for (const auto &call : definition.clockedCalls) {
      if (!program.definitions[call.definition].hasChecks())
        continue;
      auto member = instanceName(call.receiver);
      if (failed(member))
        return failure();
      out << "  if (!" << *member << ".validate_checks(failure)) return false;\n";
    }
    out << "  return true;\n}\n";
  }

  if (hasAssertions(definition)) {
    out << "bool " << moduleName(index)
        << "::validate_assertions(gfsim::SimFailureInfo &failure) const noexcept {\n";
    for (auto [ruleIndex, rule] : llvm::enumerate(definition.clocked->rules))
      for (auto assertion : rule.assertions) {
        unsigned ordinal = definition.clocked->checkOffsets.lookup(assertion);
        const auto &check = rule.checks[ordinal];
        auto id = emissionMetadataJson(check.id, assertion);
        auto location = emissionMetadataJson(check.location, assertion);
        if (failed(id) || failed(location)) return failure();
        auto message = assertion.getMessageAttr();
        out << "  if (" << assertionName(ruleIndex, ordinal) << ") {\n"
               "    failure = {gfsim::SimFailurePhase::Check, \"source_check_failed\", "
            << cppString(message ? message.getValue() : "source assert check failed")
            << ", pyc_check_instance, " << cppString(*location) << ", "
            << cppString(*id) << "};\n    return false;\n  }\n";
      }
    out << "  return true;\n}\n";
  }

  return success();
}

FailureOr<std::string> BehavioralEmitter::typedWire(Type type, Operation *site) {
  auto payload = common.cppType(type, site);
  if (failed(payload))
    return failure();
  return "gfsim::wire<" + *payload + ">";
}

FailureOr<unsigned> BehavioralEmitter::fieldOffset(ac::StructGetOp get) {
  auto leaves = analysis.getLeaves(get.getValue().getType(), {}, get);
  if (failed(leaves))
    return failure();
  auto field = llvm::find_if(llvm::reverse(*leaves), [&](const ac::PackedLeaf &leaf) {
    return !leaf.path.empty() && leaf.path.front().getValue() == get.getField();
  });
  if (field == leaves->rend() || field->low > std::numeric_limits<unsigned>::max())
    return get.emitOpError("behavioral field has no packed layout");
  return static_cast<unsigned>(field->low);
}

std::string BehavioralEmitter::graphLeaf(const gfir::StructuredWire &wire,
                                         StringRef value) const {
  const bool table = isa<ac::TableType>(wire.type);
  if (table && isa<ac::TableType>(wire.endpoint.value.getType()))
    return value.str();
  auto slice = "pyc_behavior::slice<" + std::to_string(wire.width) + ">(" +
               pack(value) + ", " + std::to_string(wire.low) + ")";
  return table ? unpack(graphType(wire, tableWireTypes), slice) : slice;
}

FailureOr<std::string>
BehavioralEmitter::graphWire(Value value,
                             const gfir::StructuredDefinition &definition,
                             Operation *site,
                             std::optional<ArrayRef<unsigned>> available) {
  auto values = definition.valueWires.find(value);
  if (values == definition.valueWires.end())
    return failure();
  if (isa<ac::TableType>(value.getType())) {
    unsigned index = values->second.front();
    if (!available || llvm::is_contained(*available, index))
      return wireName(index);
    return graphType(definition.wires[index], tableWireTypes) + "::unknown()";
  }
  auto type = typedWire(value.getType(), site);
  if (failed(type))
    return failure();
  SmallVector<unsigned> ordered(values->second);
  llvm::sort(ordered, [&](unsigned a, unsigned b) {
    return definition.wires[a].low > definition.wires[b].low;
  });
  SmallVector<std::string> fields;
  for (unsigned at : ordered) {
    const auto &field = definition.wires[at];
    if (available && !llvm::is_contained(*available, at))
      fields.push_back(carrier(field.width) + "::unknown()");
    else
      fields.push_back(isa<ac::TableType>(field.type) ? pack(wireName(at)) : wireName(at));
  }
  return unpack(*type, concatenate(fields));
}

FailureOr<std::string>
BehavioralEmitter::valueExpression(Value result, ArrayRef<std::string> args) {
  auto *op = result.getDefiningOp();
  auto type = result.getType();
  if (isa<ac::TableSplatOp>(op)) {
    auto size = analysis.getTableSize(cast<ac::TableType>(type), {}, op);
    if (failed(size))
      return failure();
    return "pyc_behavior::splat<" + std::to_string(*size) + ">(" + args.front() + ")";
  }
  if (auto map = dyn_cast<ac::TableMapOp>(op))
    return tableMapExpression(map, cast<OpResult>(result).getResultNumber(), args);
  if (isa<ac::TableGetOp>(op)) {
    unsigned ordinal = cast<OpResult>(result).getResultNumber();
    if (ordinal == 0)
      return "std::get<0>(pyc_behavior::lookup(" + join(args, ", ") + "))";
    auto size = analysis.getTableSize(cast<ac::TableType>(op->getOperand(0).getType()), {}, op);
    if (failed(size))
      return failure();
    return "pyc_behavior::Level::fromPacked(pyc_behavior::inRange<" +
           std::to_string(*size) + ">(" + args[1] + "))";
  }
  auto wire = typedWire(type, op);
  auto width = analysis.getPackedWidth(type, {}, op);
  if (failed(wire) || failed(width) || *width > std::numeric_limits<unsigned>::max())
    return failure();
  SmallVector<std::string> packed;
  for (const auto &arg : args)
    packed.push_back(pack(arg));
  if (isa<ac::StructCreateOp>(op))
    return unpack(*wire, concatenate(packed));
  if (auto get = dyn_cast<ac::StructGetOp>(op)) {
    auto low = fieldOffset(get);
    if (failed(low))
      return failure();
    return unpack(*wire, "pyc_behavior::slice<" + std::to_string(*width) + ">(" +
                             packed.front() + ", " + std::to_string(*low) + ")");
  }
  auto computed = bitExpression(op, *width, packed);
  if (failed(computed))
    return failure();
  return unpack(*wire, *computed);
}

FailureOr<std::string> BehavioralEmitter::tableMapExpression(
    ac::TableMapOp map, unsigned result, ArrayRef<std::string> args) {
  auto tableType = cast<ac::TableType>(map.getResult(result).getType());
  auto type = typedWire(tableType, map);
  auto size = analysis.getTableSize(tableType, {}, map);
  if (failed(type) || failed(size))
    return failure();
  auto &body = map.getBody().front();
  auto yielded = cast<ac::YieldOp>(body.getTerminator()).getValues()[result];
  auto inputTable = [&](Value row) -> std::optional<unsigned> {
    auto argument = dyn_cast<BlockArgument>(row);
    if (argument && argument.getOwner() == &body && argument.getArgNumber() > 0 &&
        argument.getArgNumber() <= map.getTables().size())
      return argument.getArgNumber() - 1;
    return std::nullopt;
  };
  auto forwarded = inputTable(yielded);
  if (auto select = yielded.getDefiningOp<ac::BitsSelectOp>()) {
    auto yes = inputTable(select.getTrueValue());
    auto no = inputTable(select.getFalseValue());
    // A four-state mux of the same row preserves all three masks.
    if (yes && no && map.getTables()[*yes] == map.getTables()[*no])
      forwarded = yes;
    if (tableType.getShape().size() == 1 && isa<ac::BitsType>(tableType.getElementType())) {
      auto uniform = [&](Value row) -> std::string {
        auto blockArg = dyn_cast<BlockArgument>(row);
        if (!blockArg || blockArg.getOwner() != &body || blockArg.getArgNumber() == 0)
          return {};
        unsigned index = blockArg.getArgNumber() - 1;
        if (index >= map.getTables().size())
          return args[index];
        if (map.getTables()[index].getDefiningOp<ac::TableSplatOp>())
          return args[index] + ".element(0)";
        return {};
      };
      auto condition = uniform(select.getCondition());
      if (!forwarded && yes && no && !condition.empty())
        return "pyc_behavior::selectRows(" + condition + ", " + args[*yes] + ", " +
               args[*no] + ")";

      auto compare = select.getCondition().getDefiningOp<ac::BitsCompareOp>();
      auto replacement = uniform(select.getTrueValue());
      if (no && compare && compare.getPredicate() == "eq" && !replacement.empty()) {
        auto ordinal = body.getArgument(0);
        auto matchesOrdinal = [&](Value value) {
          if (auto resize = value.getDefiningOp<ac::BitsResizeOp>();
              resize && resize.getMode() == "zext")
            value = resize.getInput();
          return value == ordinal;
        };
        std::string index;
        if (matchesOrdinal(compare.getLhs()))
          index = uniform(compare.getRhs());
        else if (matchesOrdinal(compare.getRhs()))
          index = uniform(compare.getLhs());
        if (!index.empty()) {
          auto width = analysis.getPackedWidth(ordinal.getType(), {}, map);
          if (failed(width))
            return failure();
          // A wrapped row ordinal can match more than one row.
          if (std::bit_width(*size - 1) <= *width)
            return "pyc_behavior::updatedTable(" + args[*no] + ", " + index + ", " +
                   replacement + ")";
        }
      }
    }
  }
  if (forwarded && map.getTables()[*forwarded].getType() == tableType)
    return args[*forwarded];

  llvm::SmallDenseSet<Value> needed;
  SmallVector<Value> pending{yielded};
  while (!pending.empty()) {
    auto value = pending.pop_back_val();
    if (!needed.insert(value).second)
      continue;
    if (auto *op = value.getDefiningOp())
      pending.append(op->getOperands().begin(), op->getOperands().end());
  }
  std::string text;
  llvm::raw_string_ostream out(text);
  out << "([&]() {\n    " << *type << " pyc_mapped;\n";
  for (auto [index, argument] : llvm::enumerate(body.getArguments().drop_front()))
    if (needed.contains(argument))
      out << "    const auto &pyc_map_input_" << index << " = " << args[index] << ";\n";
  out << "    for (std::size_t pyc_row = 0; pyc_row < " << *size << "; ++pyc_row) {\n";
  llvm::DenseMap<Value, std::string> values;
  for (auto [index, argument] : llvm::enumerate(body.getArguments())) {
    if (!needed.contains(argument))
      continue;
    auto name = "pyc_map_arg_" + std::to_string(index);
    out << "      const auto " << name << " = ";
    if (index == 0) {
      auto wire = typedWire(argument.getType(), map);
      auto width = analysis.getPackedWidth(argument.getType(), {}, map);
      if (failed(wire) || failed(width))
        return failure();
      out << *wire << "::known(gfsim::Bits<" << *width << ">{pyc_row})";
    } else {
      out << "pyc_map_input_" << index - 1;
      if (index <= map.getTables().size())
        out << ".element(pyc_row)";
    }
    out << ";\n";
    values.try_emplace(argument, std::move(name));
  }
  for (Operation &op : body) {
    if (isa<ac::YieldOp>(op))
      break;
    for (Value value : op.getResults()) {
      if (!needed.contains(value))
        continue;
      SmallVector<std::string> operands;
      for (Value input : op.getOperands())
        operands.push_back(values.lookup(input));
      auto expression = valueExpression(value, operands);
      if (failed(expression))
        return failure();
      auto name = "pyc_map_value_" + std::to_string(values.size());
      out << "      const auto " << name << " = " << *expression << ";\n";
      values.try_emplace(value, std::move(name));
    }
  }
  out << "      pyc_mapped.element(pyc_row) = " << values.lookup(yielded)
      << ";\n    }\n    return pyc_mapped;\n  }())";
  return text;
}

LogicalResult BehavioralEmitter::emitInitial(raw_ostream &out, gfir::VarOp variable,
                                             unsigned index) {
  out << "  static const auto &" << initialName(index) << "() noexcept {\n"
      << "    static const auto init = [] {\n";
  Value initial = variable.getInitial();
  SmallVector<std::pair<Value, bool>> pending{{initial, false}};
  llvm::DenseMap<Value, std::string> values;
  while (!pending.empty()) {
    auto [value, ready] = pending.pop_back_val();
    if (values.contains(value))
      continue;
    auto *op = value.getDefiningOp();
    if (!ready) {
      pending.emplace_back(value, true);
      for (Value input : llvm::reverse(op->getOperands()))
        pending.emplace_back(input, false);
      continue;
    }
    SmallVector<std::string> args;
    for (Value input : op->getOperands())
      args.push_back(values.lookup(input));
    auto text = valueExpression(value, args);
    if (failed(text))
      return failure();
    auto name = wireName(values.size());
    out << "      const auto " << name << " = " << *text << ";\n";
    values.try_emplace(value, std::move(name));
  }
  out << "      return " << values.lookup(initial) << ";\n    }();\n"
      << "    return init;\n  }\n";
  return success();
}

FailureOr<std::string> methodName(gfir::MethodOp method) {
  auto name = legalizeIdentifier(method.getSymName(),
                                 [&] { return method.emitOpError(); });
  if (failed(name))
    return failure();
  return "pyc_method_" + *name;
}

LogicalResult BehavioralEmitter::emitBlockedReturn(raw_ostream &out, Block &body,
                                                   bool ruleResults) {
  SmallVector<std::string> values;
  if (!ruleResults)
    for (Value value : cast<gfir::YieldOp>(body.getTerminator()).getValues()) {
      auto type = typedWire(value.getType(), body.getParentOp());
      if (failed(type))
        return failure();
      values.push_back(*type + "{}");
    }
  out << "return";
  if (values.size() == 1)
    out << ' ' << values.front();
  else if (!values.empty())
    out << " std::tuple{" << join(values, ", ") << '}';
  out << ";\n";
  return success();
}

LogicalResult BehavioralEmitter::emitBody(raw_ostream &out, Block &body,
                                          const gfir::StructuredDefinition &definition,
                                          const gfir::StructuredRule *rule,
                                          const llvm::DenseMap<Value, std::string> *outer,
                                          ArrayRef<std::string> stepTargets) {
  const auto &clocked = *definition.clocked;
  bool ruleResults = rule != nullptr;
  unsigned nextObservation = 0;
  llvm::DenseMap<Value, std::string> values = outer ? *outer : llvm::DenseMap<Value, std::string>{};
  for (Value arg : body.getArguments())
    values.try_emplace(arg, wireName(values.size()));
  auto snapshot = [&] {
    auto yield = cast<gfir::YieldOp>(body.getTerminator());
    for (auto [index, value] : llvm::enumerate(yield.getValues()))
      out << "    txn." << resultName(index) << " = " << values.lookup(value) << ";\n";
  };
  auto requireKnownOne = [&](StringRef predicate, Operation *site) -> LogicalResult {
    out << "    if (!pyc_behavior::isOne(" << predicate << ")) {\n"
        << "      if (txn.pyc_check_active && !" << predicate << ".isFullyKnown()) "
        << "txn.pyc_failed_check = pyc_check_base + " << clocked.checkOffsets.lookup(site) << ";\n"
           "      txn.pyc_running = false;\n      ";
    if (failed(emitBlockedReturn(out, body, ruleResults)))
      return failure();
    out << "    }\n";
    return success();
  };
  Operation *snapshotBoundary = rule && rule->assertions.empty() ? rule->afterSnapshot : nullptr;
  auto nextRegion = body.begin();
  for (auto iterator = body.begin(); iterator != body.end();) {
    auto current = iterator++;
    Operation &op = *current;
    if (snapshotBoundary == &op) {
      snapshot();
      out << "    if (!pyc_active) return;\n";
    }
    if (current == nextRegion) {
      SmallVector<Operation *, 16> scalar;
      for (; nextRegion != body.end() &&
             scalar.size() < maxScalarOperations &&
             (nextRegion == current || &*nextRegion != snapshotBoundary) &&
             isNativeScalar(*nextRegion); ++nextRegion)
        scalar.push_back(&*nextRegion);
      if (scalar.empty()) {
        nextRegion = iterator;
      } else if (scalar.size() >= minScalarOperations) {
        auto emitted = emitScalarRegion(out, scalar, values);
        if (failed(emitted))
          return failure();
        if (*emitted) {
          iterator = nextRegion;
          continue;
        }
      }
    }
    SmallVector<std::string> args, results;
    for (Value input : op.getOperands())
      args.push_back(values.lookup(input));
    for (Value result : op.getResults()) {
      results.push_back(wireName(values.size()));
      values.try_emplace(result, results.back());
    }
    std::string text;
    bool borrow = false;
    if (auto effect = dyn_cast<gfir::EffectOp>(op)) {
      if (effect.getKind() == "assert") {
        out << "    txn.pyc_assert_condition_" << clocked.checkOffsets.lookup(effect)
            << " = " << args.front() << ";\n";
      } else {
        unsigned slot = rule->observations[nextObservation++].firstSlot;
        for (const auto &argument : args)
          out << "    txn." << observationName(slot++) << " = " << argument << ";\n";
      }
      continue;
    } else if (auto loop = dyn_cast<gfir::LoopOp>(op)) {
      if (failed(emitLoop(out, loop, definition, values, results)))
        return failure();
      continue;
    } else if (auto ordinal = dyn_cast<gfir::OrdinalBitsOp>(op)) {
      auto type = typedWire(ordinal.getValue().getType(), ordinal);
      auto width = analysis.getPackedWidth(ordinal.getValue().getType(), {}, ordinal);
      if (failed(type) || failed(width))
        return failure();
      text = *type + "::known(gfsim::Bits<" + std::to_string(*width) + ">{" +
             values.lookup(ordinal.getOrdinal()) + "})";
    } else if (isa<gfir::StepOp>(op)) {
      if (!stepTargets.empty())
        out << "    {\n      const auto pyc_next = std::tuple{" << join(args, ", ")
            << "};\n      std::tie(" << join(stepTargets, ", ") << ") = pyc_next;\n    }\n";
      continue;
    } else if (auto guard = dyn_cast<gfir::GuardOp>(op)) {
      if (auto capture = clocked.guardedCaptureUses.find(guard);
          capture != clocked.guardedCaptureUses.end()) {
        out << "    if (!txn.pyc_capture_available_" << capture->second
            << ") { txn.pyc_running = false; ";
        if (failed(emitBlockedReturn(out, body, ruleResults)))
          return failure();
        out << " }\n";
      }
      if (failed(requireKnownOne(values.lookup(guard.getPredicate()), guard)))
        return failure();
      continue;
    } else if (auto branch = dyn_cast<gfir::IfOp>(op)) {
      if (clocked.checkedBranches.contains(branch)) {
        auto predicate = values.lookup(branch.getPredicate());
        out << "    if (!" << predicate << ".isFullyKnown()) {\n"
            << "      if (txn.pyc_check_active) txn.pyc_failed_check = pyc_check_base + "
            << clocked.checkOffsets.lookup(branch) << ";\n"
               "      txn.pyc_running = false;\n      ";
        if (failed(emitBlockedReturn(out, body, ruleResults)))
          return failure();
        out << "    }\n";
      }
      if (failed(emitBranch(out, branch, definition, values, results)))
        return failure();
      if (clocked.checkedBranches.contains(branch)) {
        out << "    if (!txn.pyc_running) { ";
        if (failed(emitBlockedReturn(out, body, ruleResults)))
          return failure();
        out << "    }\n";
      }
      continue;
    } else if (auto remote = clocked.childMethodCalls.find(&op);
               remote != clocked.childMethodCalls.end()) {
      unsigned receiver = remote->second.receiver;
      const auto &binding = definition.childStates[receiver];
      const auto &child = *program.definitions[binding.definition].clocked;
      auto instance = instanceName(binding.receiver);
      auto method = methodName(child.methods[remote->second.method]);
      if (failed(instance) || failed(method))
        return failure();
      args.insert(args.begin(), "0");
      args.insert(args.begin(), "txn." + childCandidateName(receiver));
      text = *instance + "." + *method + "(" + join(args, ", ") + ")";
    } else if (auto call = dyn_cast<gfir::CallOp>(op)) {
      auto memoryUse = clocked.transactionMemoryCalls.find(call);
      if (memoryUse != clocked.transactionMemoryCalls.end()) {
        const auto &use = memoryUse->second;
        const auto &memory = clocked.memories[use.memory];
        auto name = instanceName(memory.instance);
        if (failed(name))
          return failure();
        if (use.access == gfir::StructuredMemoryAccess::Read) {
          out << "    txn." << memoryPinName(use.memory, 0, 0) << " = " << args.front() << ";\n"
              << "    txn." << memoryPinName(use.memory, 0, 3) << " = true;\n";
          text = *name + ".read(" + args.front() + ")";
        } else if (use.access == gfir::StructuredMemoryAccess::Result) {
          out << "    if (!pyc_behavior::isOne(" << *name << ".live(" << use.port
              << "))) { txn.pyc_running = false; ";
          if (failed(emitBlockedReturn(out, body, ruleResults)))
            return failure();
          out << " }\n";
          text = *name + ".result(" + std::to_string(use.port) + ")";
        } else {
          // Typed local arguments are total. Payload knownness belongs to the
          // kernel, after the entire transaction has admitted these pins.
          unsigned port = use.access == gfir::StructuredMemoryAccess::Write ? 2 : use.port;
          for (auto [argument, value] : llvm::enumerate(args))
            out << "    txn." << memoryPinName(use.memory, port, argument) << " = " << value << ";\n";
          out << "    txn." << memoryPinName(use.memory, port, 3) << " = true;\n";
          continue;
        }
      } else {
        auto use = clocked.fifoCalls.find(call);
        if (use != clocked.fifoCalls.end()) {
          unsigned fifoIndex = use->second.fifo;
          const auto &fifo = clocked.fifos[fifoIndex];
          bool isInput = use->second.access == gfir::StructuredFifoAccess::In;
          auto name = instanceName(fifo.instance);
          if (failed(name))
            return failure();
          if (rule && rule->sampled) {
            // Before initialization all local FIFOs share unknown committed
            // controls. Once initialized, unknown readiness can only come
            // from an effective raw pop, which the FIFO reports during Work.
            auto available = "pyc_fifo_available_" + std::to_string(clocked.checkOffsets.lookup(call));
            out << "    const auto " << available << " = " << *name;
            if (isInput) {
              out << ".ready(";
              if (fifo.downstreamPop)
                out << "txn." << recipientName(fifoIndex);
              else
                out << levelName << "(false)";
              out << ')';
            } else {
              out << ".valid()";
            }
            out << ";\n";
            if (failed(requireKnownOne(available, call)))
              return failure();
          }
          // Reads use old Q. Payload writes are staged in the caller's frame.
          if (isInput)
            text = "txn." + payloadName(fifoIndex) + " = " + args.front();
          else
            text = *name + ".data()";
        } else {
          auto callee = methodName(clocked.methods[clocked.methodCalls.lookup(call)]);
          if (failed(callee))
            return failure();
          args.insert(args.begin(), "pyc_check_base + " + std::to_string(clocked.checkOffsets.lookup(call)));
          args.insert(args.begin(), "txn");
          text = *callee + "(" + join(args, ", ") + ")";
        }
      }
    } else if (auto read = dyn_cast<gfir::ReadOp>(op)) {
      auto state = stateName(clocked.stateAccesses.lookup(read));
      text = "txn." + state + " ? *txn." + state + " : " + state + ".value()";
      borrow = canBorrowTableRead(read);
    } else if (auto write = dyn_cast<gfir::WriteOp>(op)) {
      unsigned index = clocked.stateAccesses.lookup(write);
      auto state = stateName(index);
      std::string frame = "txn." + state;
      if (clocked.conditionalWrites.contains(index))
        out << "    txn." << intentName(index) << " = " << levelName << "(true);\n";
      if (auto path = clocked.writePaths.find(write); path != clocked.writePaths.end()) {
        out << "    {\n      if (!" << frame << ") " << frame << ".emplace(" << state
            << ".value());\n";
        if (write.getIndex()) {
          auto type = typedWire(path->second.types.back(), write);
          if (failed(type))
            return failure();
          out << "      pyc_behavior::replacePackedRows<" << *type << ">(*" << frame
              << ", " << path->second.low << ", "
              << values.lookup(write.getIndex()) << ", "
              << values.lookup(write.getValue()) << ");\n";
        } else {
          out << "      " << frame << "->assignSlice(" << path->second.low << ", "
              << pack(values.lookup(write.getValue())) << ");\n";
        }
        out << "    }\n";
        continue;
      }
      if (write.getIndex()) {
        out << "    if (!" << frame << ") " << frame << ".emplace(" << state
            << ".value());\n";
        text = "pyc_behavior::replace(*" + frame + ", " +
               values.lookup(write.getIndex()) + ", " + values.lookup(write.getValue()) + ")";
      } else {
        text = frame + " = " + values.lookup(write.getValue());
      }
    } else if (isa<gfir::YieldOp>(op)) {
      if (ruleResults) {
        if (!rule->afterSnapshot || !rule->assertions.empty())
          snapshot();
      } else if (args.size() == 1)
        out << "    return " << args.front() << ";\n";
      else if (!args.empty())
        out << "    return std::tuple{" << join(args, ", ") << "};\n";
      continue;
    } else if (isa<ac::TableGetOp>(op)) {
      text = "pyc_behavior::lookup(" + join(args, ", ") + ")";
    } else {
      auto value = valueExpression(op.getResult(0), args);
      if (failed(value))
        return failure();
      text = *value;
    }
    out << "    ";
    if (results.size() == 1)
      out << "const auto " << (borrow ? "&" : "") << results.front() << " = ";
    else if (!results.empty())
      out << "const auto [" << join(results, ", ") << "] = ";
    out << text << ";\n";
    auto method = clocked.methodCalls.find(&op);
    if (method != clocked.methodCalls.end() &&
        clocked.sampledMethods.contains(method->second)) {
      out << "    if (!txn.pyc_running) { ";
      if (failed(emitBlockedReturn(out, body, ruleResults)))
        return failure();
      out << "    }\n";
    }
  }
  return success();
}

LogicalResult BehavioralEmitter::emitLoop(
    raw_ostream &out, gfir::LoopOp loop, const gfir::StructuredDefinition &definition,
    const llvm::DenseMap<Value, std::string> &values,
    ArrayRef<std::string> results) {
  for (auto [result, initial, name] :
       llvm::zip(loop.getResults().drop_back(), loop.getInitial(),
                 results.drop_back())) {
    auto type = typedWire(result.getType(), loop);
    if (failed(type))
      return failure();
    out << "    " << *type << ' ' << name << " = " << values.lookup(initial) << ";\n";
  }
  uint64_t count = loop.getTripCountAttr().getInt();
  if (count) {
    auto ordinal = "pyc_ordinal_" + std::to_string(nextLoop++);
    out << "    for (std::uint64_t " << ordinal << " = 0; " << ordinal << " < "
        << count << "ULL; ++" << ordinal << ") {\n";
    auto &body = loop.getBody().front();
    llvm::DenseMap<Value, std::string> iteration(values);
    iteration.try_emplace(body.getArgument(0), ordinal);
    for (auto [argument, name] :
         llvm::zip(body.getArguments().drop_front(), results.drop_back()))
      iteration.try_emplace(argument, name);
    if (failed(emitBody(out, body, definition, nullptr, &iteration,
                        results.drop_back())))
      return failure();
    out << "    }\n";
  }
  out << "    const auto " << results.back() << " = " << levelName << "(true);\n";
  return success();
}

LogicalResult BehavioralEmitter::emitBranch(
    raw_ostream &out, gfir::IfOp branch, const gfir::StructuredDefinition &definition,
    const llvm::DenseMap<Value, std::string> &values,
    ArrayRef<std::string> results) {
  const auto &clocked = *definition.clocked;
  const auto suffix = std::to_string(nextBranch++);
  const auto thenName = "pyc_then_" + suffix;
  const auto elseName = "pyc_else_" + suffix;
  const auto thenFrame = "pyc_then_frame_" + suffix;
  const auto elseFrame = "pyc_else_frame_" + suffix;
  const auto thenValue = "pyc_then_value_" + suffix;
  const auto elseValue = "pyc_else_value_" + suffix;
  const auto predicate = values.lookup(branch.getPredicate());
  for (auto [arm, name] : {std::pair{&branch.getThenBody(), thenName},
                          std::pair{&branch.getElseBody(), elseName}}) {
    out << "    const auto " << name << " = [&](auto &txn) {\n";
    if (failed(emitBody(out, arm->front(), definition, nullptr, &values)))
      return failure();
    out << "    };\n";
  }
  for (auto [index, result] : llvm::enumerate(branch.getResults())) {
    auto type = typedWire(result.getType(), branch);
    if (failed(type))
      return failure();
    out << "    " << *type << ' ' << results[index] << ";\n";
  }
  auto assign = [&](StringRef arm) {
    out << "      ";
    if (results.size() == 1)
      out << results.front() << " = ";
    else if (!results.empty())
      out << "std::tie(" << join(results, ", ") << ") = ";
    out << arm << "(txn);\n";
  };
  out << "    if (" << predicate << ".isFullyKnown()) {\n"
      << "      if (" << predicate << ".value().toBool()) {\n";
  assign(thenName);
  out << "      } else {\n";
  assign(elseName);
  if (clocked.checkedBranches.contains(branch)) {
    out << "      }\n    }\n";
    return success();
  }
  out << "      }\n    } else {\n"
      << "      auto " << thenFrame << " = txn;\n"
      << "      auto " << elseFrame << " = txn;\n"
      << "      ";
  if (!results.empty())
    out << "const auto " << thenValue << " = ";
  out << thenName << '(' << thenFrame << ");\n      ";
  if (!results.empty())
    out << "const auto " << elseValue << " = ";
  out << elseName << '(' << elseFrame << ");\n";
  for (unsigned state : clocked.branchWrites.find(branch)->second) {
    auto variable = clocked.variables[state];
    auto type = typedWire(variable.getInitial().getType(), variable);
    if (failed(type))
      return failure();
    auto candidate = [&](StringRef frame) {
      auto slot = frame.str() + '.' + stateName(state);
      return pack(slot + " ? *" + slot + " : " + stateName(state) + ".value()");
    };
    auto selected = "gfsim::select(" + pack(predicate) + ", " +
                    candidate(thenFrame) + ", " + candidate(elseFrame) + ")";
    out << "      txn." << stateName(state) << " = " << unpack(*type, selected) << ";\n"
        << "      txn." << intentName(state)
        << " = pyc_behavior::Level::fromPacked(gfsim::select(" << predicate
        << ".packed(), " << thenFrame << '.' << intentName(state) << ".packed(), "
        << elseFrame << '.' << intentName(state) << ".packed()));\n";
  }
  for (auto [index, result] : llvm::enumerate(branch.getResults())) {
    auto type = typedWire(result.getType(), branch);
    if (failed(type))
      return failure();
    auto returned = [&](StringRef arm) {
      return results.size() == 1 ? arm.str()
          : "std::get<" + std::to_string(index) + ">(" + arm.str() + ")";
    };
    auto selected = "gfsim::select(" + pack(predicate) + ", " +
                    pack(returned(thenValue)) + ", " + pack(returned(elseValue)) + ")";
    out << "      " << results[index] << " = " << unpack(*type, selected) << ";\n";
  }
  out << "    }\n";
  return success();
}

LogicalResult
BehavioralEmitter::declareClocked(raw_ostream &out,
                                 const gfir::StructuredDefinition &definition) {
  const auto &clocked = *definition.clocked;
  if (needsDomain(clocked))
    out << "  pyc_behavior::SampledDomain pyc_domain;\n";
  for (const auto &fifo : clocked.fifos) {
    auto name = instanceName(fifo.instance);
    auto type = common.cppType(fifo.element, fifo.instance);
    if (failed(name) || failed(type))
      return failure();
    out << "  pyc_behavior::Fifo<" << *type << ", " << fifo.depth
        << ", gfsim::QueueReadyPolicy::"
        << (fifo.downstreamPop ? "DownstreamPop" : "LocalOccupancy") << ", "
        << fifo.latency << "> " << *name << ";\n";
  }
  for (const auto &memory : clocked.memories) {
    auto name = instanceName(memory.instance);
    auto type = common.cppType(memory.element, memory.instance);
    if (failed(name) || failed(type))
      return failure();
    out << "  pyc_behavior::" << (memory.asynchronous() ? "ByteMemory" : "SyncMemory")
        << '<' << *type << ", " << memory.addressWidth << ", " << memory.depth;
    if (!memory.asynchronous())
      out << ", " << memory.readPorts();
    out << "> " << *name << ";\n";
  }
  for (unsigned index = 0; index < clocked.variables.size(); ++index) {
    auto variable = clocked.variables[index];
    auto type = common.cppType(variable.getInitial().getType(), variable);
    if (failed(type) || failed(emitInitial(out, variable, index)))
      return failure();
    out << "  pyc_behavior::State<" << *type << "> " << stateName(index) << ";\n";
  }
  if (!clocked.variables.empty()) {
    out << "  void pyc_reset_state() noexcept {\n";
    for (unsigned index = 0; index < clocked.variables.size(); ++index)
      out << "    " << stateName(index) << ".reset(" << initialName(index) << "());\n";
    out << "  }\n";
  }
  for (unsigned index : clocked.methodOrder) {
    auto method = clocked.methods[index];
    auto name = methodName(method);
    if (failed(name))
      return failure();
    out << "  template <class Frame> auto " << *name
        << "(Frame &txn, unsigned pyc_check_base";
    for (auto [index, type] : llvm::enumerate(method.getFunctionType().getInputs())) {
      auto wire = typedWire(type, method);
      if (failed(wire))
        return failure();
      out << ", const " << *wire << " &" << wireName(index);
    }
    out << ") {\n";
    if (failed(emitBody(out, method.getBody().front(), definition)))
      return failure();
    out << "  }\n";
  }
  for (auto [index, rule] : llvm::enumerate(clocked.rules)) {
    auto name = "TxnFrame" + std::to_string(index);
    out << "  struct " << name << " {\n";
    for (unsigned receiver : rule.childStates)
      out << "    " << moduleName(definition.childStates[receiver].definition)
          << "::ChildTxnState " << childCandidateName(receiver) << ";\n";
    for (const auto &capture : rule.guardCaptures)
      out << "    bool pyc_capture_available_" << capture.capture << " = false;\n";
    if (rule.sampled)
      out << "    bool pyc_running = true;\n"
          << "    unsigned pyc_failed_check = " << noCheck << ";\n"
          << "    bool pyc_check_active = false;\n";
    for (auto assertion : rule.assertions)
      out << "    pyc_behavior::Level pyc_assert_condition_"
          << clocked.checkOffsets.lookup(assertion) << ";\n";
    for (const auto &observation : rule.observations) {
      auto effect = observation.operation;
      unsigned slot = observation.firstSlot;
      for (Value value : effect.getValues()) {
        auto type = typedWire(value.getType(), effect);
        if (failed(type))
          return failure();
        out << "    " << *type << ' ' << observationName(slot++) << ";\n";
      }
    }
    for (unsigned state : rule.states) {
      auto variable = clocked.variables[state];
      auto wire = typedWire(variable.getInitial().getType(), variable);
      if (failed(wire))
        return failure();
      out << "    std::optional<" << *wire << "> " << stateName(state) << ";\n";
      if (clocked.conditionalWrites.contains(state))
        out << "    pyc_behavior::Level " << intentName(state)
            << " = " << levelName << "(false);\n";
    }
    for (auto [fifoId, fifo] : llvm::enumerate(clocked.fifos)) {
      if (fifo.producer != index)
        continue;
      auto type = typedWire(fifo.element, fifo.instance);
      if (failed(type))
        return failure();
      out << "    " << *type << " " << payloadName(fifoId) << ";\n";
      if (rule.sampled && fifo.downstreamPop)
        out << "    pyc_behavior::Level " << recipientName(fifoId) << ";\n";
    }
    for (auto [memoryIndex, memory] : llvm::enumerate(clocked.memories)) {
      auto declare = [&](gfir::CallOp call, unsigned port) -> LogicalResult {
        if (!call || clocked.transactionMemoryCalls.lookup(call).rule != index)
          return success();
        for (auto [argument, type] : llvm::enumerate(call.getInputs().getTypes())) {
          auto wire = typedWire(type, call);
          if (failed(wire))
            return failure();
          out << "    " << *wire << ' ' << memoryPinName(memoryIndex, port, argument) << ";\n";
        }
        out << "    bool " << memoryPinName(memoryIndex, port, 3) << " = false;\n";
        return success();
      };
      for (unsigned port = 0; port < memory.readPorts(); ++port)
        if (failed(declare(memory.requests[port], port)))
          return failure();
      if (failed(declare(memory.reader, 0)) || failed(declare(memory.writer, 2)))
        return failure();
    }
    auto operation = rule.operation;
    for (auto [result, type] : llvm::enumerate(operation.getFunctionType().getResults())) {
      auto wire = typedWire(type, operation);
      if (failed(wire))
        return failure();
      out << "    " << *wire << ' ' << resultName(result) << ";\n";
    }
    out << "  };\n  void " << ruleName(index) << "(" << name << " &txn";
    if (rule.afterSnapshot)
      out << ", bool pyc_active";
    for (auto [input, type] : llvm::enumerate(operation.getFunctionType().getInputs())) {
      auto wire = typedWire(type, operation);
      if (failed(wire))
        return failure();
      out << ", const " << *wire << " &" << wireName(input);
    }
    out << ") {\n";
    out << "    constexpr unsigned pyc_check_base = 0;\n";
    if (failed(emitBody(out, operation.getBody().front(), definition, &rule)))
      return failure();
    out << "  }\n";
  }
  return success();
}

FailureOr<std::string>
BehavioralEmitter::recipient(const gfir::StructuredDefinition &definition,
                             const gfir::StructuredFifo &fifo,
                             std::optional<unsigned> rule, bool ownPop) {
  if (fifo.rawOutput) {
    auto call = fifo.rawOutput;
    auto wires = definition.valueWires.find(call.getInputs().front());
    if (wires == definition.valueWires.end() || wires->second.size() != 1)
      return call.emitOpError("FIFO recipient has no single-bit graph value");
    return "pyc_behavior::Level::fromPacked(" + wireName(wires->second.front()) + ")";
  }
  if (fifo.consumer && fifo.consumer != rule)
    return wireName(definition.wires.size() + *fifo.consumer);
  return levelName.str() + (ownPop ? "(true)" : "(false)");
}

FailureOr<std::string> BehavioralEmitter::availabilityExpression(
    const gfir::StructuredDefinition &definition,
    ArrayRef<gfir::StructuredAvailabilitySource> sources,
    StringRef instancePrefix) {
  SmallVector<std::string> terms;
  for (const auto &source : sources) {
    if (source.kind == gfir::StructuredAvailabilitySource::Kind::Rule) {
      terms.push_back(frameName(source.index) + ".pyc_running");
      continue;
    }
    auto member = instanceName(definition.clockedCalls[source.index].receiver);
    if (failed(member))
      return failure();
    auto result = source.kind == gfir::StructuredAvailabilitySource::Kind::ChildField
                      ? fieldAvailabilityName(source.port)
                      : availabilityName(source.port);
    terms.push_back(instancePrefix.str() + *member + "." + result);
  }
  return join(terms, " && ");
}

LogicalResult BehavioralEmitter::emitGrant(raw_ostream &out,
                                            const gfir::StructuredDefinition &definition,
                                            unsigned grant, StringRef instancePrefix) {
  unsigned index = grant - definition.wires.size();
  const auto &rule = definition.clocked->rules[index];
  bool orderedFifo = rule.sampled && !rule.requirements.empty();
  if (!orderedFifo) {
    SmallVector<std::string> checks;
    for (const auto &requirement : rule.requirements) {
      const auto &fifo = definition.clocked->fifos[requirement.fifo];
      auto member = instanceName(fifo.instance);
      if (failed(member))
        return failure();
      std::string query = instancePrefix.str() + *member;
      if (requirement.requiresReady) {
        auto ready = fifo.downstreamPop
            ? recipient(definition, fifo, index, requirement.ownPop)
            : FailureOr<std::string>(levelName.str() + "(false)");
        if (failed(ready))
          return failure();
        query += ".ready(" + *ready + ")";
      } else {
        query += ".valid()";
      }
      checks.push_back(query + ".packed()");
    }
    std::string grantExpr = levelName.str() + "(true)";
    if (!checks.empty()) {
      std::string combined = checks.front();
      for (const auto &check : ArrayRef<std::string>(checks).drop_front())
        combined = "gfsim::bit_and(" + combined + ", " + check + ")";
      grantExpr = "pyc_behavior::Level::fromPacked(" + combined + ")";
    }
    out << "    const auto " << wireName(grant) << " = " << grantExpr << ";\n";
  }
  out << "    TxnFrame" << index << ' ' << frameName(index) << ";\n";
  if (rule.sampled)
    out << "    " << frameName(index) << ".pyc_check_active = pyc_active;\n";
  if (orderedFifo) {
    for (const auto &requirement : rule.requirements) {
      const auto &fifo = definition.clocked->fifos[requirement.fifo];
      if (!requirement.requiresReady || !fifo.downstreamPop)
        continue;
      auto ready = recipient(definition, fifo, index);
      if (failed(ready))
        return failure();
      out << "    " << frameName(index) << '.' << recipientName(requirement.fifo)
          << " = " << *ready << ";\n";
    }
  }
  for (const auto &capture : rule.guardCaptures) {
    auto available = availabilityExpression(definition, capture.sources, instancePrefix);
    if (failed(available))
      return failure();
    out << "    " << frameName(index) << ".pyc_capture_available_" << capture.capture
        << " = " << *available << ";\n";
  }
  out << "    ";
  if (!orderedFifo) {
    out << "if (";
    if (needsDomain(*definition.clocked) && rule.resultUses.empty() &&
        !rule.readsByteMemory && rule.assertions.empty() && rule.observations.empty())
      out << "pyc_active && ";
    out << "pyc_behavior::isOne(" << wireName(grant) << ")) ";
  }
  out << instancePrefix << ruleName(index) << '(' << frameName(index);
  if (rule.afterSnapshot)
    out << ", pyc_active";
  auto captures = captureExpressions(definition, rule);
  if (failed(captures))
    return failure();
  for (const auto &value : *captures)
    out << ", " << value;
  out << ");\n";
  if (orderedFifo)
    out << "    const auto " << wireName(grant) << " = " << levelName << '('
        << frameName(index) << ".pyc_running);\n";
  for (auto assertion : rule.assertions) {
    unsigned ordinal = definition.clocked->checkOffsets.lookup(assertion);
    out << "    {\n      const auto pyc_error = gfsim::bit_and(gfsim::bit_and("
        << levelName << '(' << frameName(index) << ".pyc_running).packed(), "
           "gfsim::bit_not(pyc_reset.packed())), gfsim::bit_not("
        << frameName(index) << ".pyc_assert_condition_" << ordinal << ".packed()));\n"
        << "      " << assertionName(index, ordinal)
        << " = !pyc_error.isFullyKnown() || pyc_error.value() != gfsim::Bits<1>{0};\n    }\n";
  }
  if (!rule.observations.empty()) {
    auto frame = frameName(index);
    out << "    if (" << frame
        << ".pyc_running && pyc_sink && pyc_sink->slots && pyc_sink->system) {\n";
    for (const auto &observation : rule.observations) {
      auto effect = observation.operation;
      unsigned slot = observation.firstSlot;
      auto stage = [&](StringRef packed, bool eventBool) {
        out << "      pyc_sink->slots->StagePacked(pyc_observation_base + " << slot++
            << ", " << packed << ", " << (eventBool ? "true" : "false")
            << ", true, true, pyc_sink->system->cycle());\n";
      };
      if (effect.getValues().empty()) {
        stage("pyc_behavior::level(true).packed()", true);
        continue;
      }
      for (Value value : effect.getValues()) {
        auto width = analysis.getPackedWidth(value.getType(), {}, effect);
        if (failed(width))
          return failure();
        stage(frame + "." + observationName(slot) + ".packed()",
              effect.getKind() == "log" && *width == 1);
      }
    }
    out << "    }\n";
  }
  if (rule.sampled)
    out << "    if (" << frameName(index) << ".pyc_failed_check != " << noCheck
        << " && " << index << " < pyc_failed_rule) {\n"
        << "      pyc_failed_rule = " << index << ";\n"
        << "      pyc_failed_check = " << frameName(index) << ".pyc_failed_check;\n"
        << "    }\n";
  return success();
}

FailureOr<SmallVector<std::string>> BehavioralEmitter::captureExpressions(
    const gfir::StructuredDefinition &definition, const gfir::StructuredRule &rule) {
  auto operation = rule.operation;
  SmallVector<std::string> values;
  for (Value capture : operation.getCaptures()) {
    auto value = graphWire(capture, definition, operation);
    if (failed(value))
      return failure();
    values.push_back(std::move(*value));
  }
  return values;
}

LogicalResult BehavioralEmitter::emitMemoizedRule(
    raw_ostream &out, const gfir::StructuredDefinition &definition) {
  const auto &rule = definition.clocked->rules.front();
  const unsigned grant = definition.wires.size();
  SmallVector<std::string> inputs;
  for (unsigned input : definition.preparationInputs)
    inputs.push_back(wireName(input));
  out << "    const auto " << wireName(grant) << " = " << levelName << "(true);\n"
      << "    const auto pyc_key = " << concatenate(inputs) << ";\n"
      << "    if (!pyc_memo_key || !pyc_behavior::sameMasks(*pyc_memo_key, pyc_key)) {\n"
         "      pyc_memo_key.reset();\n";
  if (failed(emitWires(out, definition, rule.dependencies)))
    return failure();
  auto captures = captureExpressions(definition, rule);
  if (failed(captures))
    return failure();
  if (rule.afterSnapshot) {
    out << "      pyc_memo_values.emplace(" << join(*captures, ", ") << ");\n";
  } else {
    out << "      " << ruleName(0) << "(pyc_memo_frame.emplace()";
    for (const auto &capture : *captures)
      out << ", " << capture;
    out << ");\n";
  }
  out << "      pyc_memo_key = pyc_key;\n    }\n";
  if (rule.afterSnapshot) {
    out << "    TxnFrame0 " << frameName(0) << ";\n"
        << "    " << ruleName(0) << '(' << frameName(0) << ", pyc_active";
    for (unsigned i = 0; i < captures->size(); ++i)
      out << ", std::get<" << i << ">(*pyc_memo_values)";
    out << ");\n";
  } else {
    out << "    const auto &" << frameName(0) << " = *pyc_memo_frame;\n";
  }
  return success();
}

LogicalResult BehavioralEmitter::emitClockedCall(
    raw_ostream &out, const gfir::StructuredDefinition &definition,
    unsigned index, StringRef instancePrefix) {
  const auto &call = definition.clockedCalls[index];
  const auto &child = program.definitions[call.definition];
  auto operation = call.operation;
  auto member = instanceName(call.receiver);
  if (failed(member))
    return failure();
  out << "    " << instancePrefix << *member << ".prepare(";
  if (call.domainInputs) {
    auto entry = program.root;
    auto policy = entry.getExecutionPolicyAttr();
    bool ifOne = policy && policy.getAs<StringAttr>("reset").getValue() == "if_one";
    out << "pyc_behavior::Level::fromPacked(" << wireName(call.domainInputs->first)
        << "), gfsim::interpretReset(pyc_behavior::Level::fromPacked("
        << wireName(call.domainInputs->second) << "), gfsim::ResetSemantics::"
        << (ifOne ? "IfOne" : "Strict") << ')';
  } else {
    out << "pyc_clock, pyc_reset";
  }
  for (unsigned input : child.preparationInputs) {
    const auto &endpoint = child.wires[input].endpoint;
    unsigned argument = cast<BlockArgument>(endpoint.value).getArgNumber();
    auto actual = operation.getInputs()[argument];
    const auto &leaves = definition.valueWires.find(actual)->second;
    auto leaf = llvm::find_if(leaves, [&](unsigned wire) {
      return definition.wires[wire].endpoint.path == endpoint.path;
    });
    if (leaf == leaves.end())
      return operation.emitOpError("clocked child preparation input is unresolved");
    out << ", " << wireName(*leaf);
  }
  if (child.incomingState) {
    auto writer = call.childState ? definition.childStates[*call.childState].writer
                                 : std::nullopt;
    if (writer) {
      auto frame = frameName(*writer);
      out << ", " << frame << '.' << childCandidateName(*call.childState)
          << ", " << frame << ".pyc_running";
    } else {
      out << ", {}, false";
    }
  }
  out << ");\n";
  return success();
}

LogicalResult BehavioralEmitter::emitTemporalWork(
    raw_ostream &out, unsigned module) {
  const auto &definition = program.definitions[module];
  const auto &wide = wholeOutputs[module];
  SmallVector<unsigned> needed;
  if (definition.hasChecks())
    out << "    pyc_checks_valid = false;\n"
        << "    pyc_failed_rule = " << noCheck << ";\n"
        << "    pyc_failed_check = " << noCheck << ";\n"
        << "    pyc_sampling_fault = gfsim::SampledClockFault::None;\n";
  for (const auto &output : definition.outputs)
    if (output.requiresPreparation && !wide.contains(output.port.port))
      needed.push_back(output.wire);
  if (definition.clocked) {
    const auto &plan = *definition.clocked;
    if (needsDomain(plan)) {
      out << "    const auto pyc_sample = pyc_domain.evaluate(pyc_clock, pyc_reset);\n";
      if (definition.hasChecks())
        out << "    pyc_sampling_fault = pyc_sample.fault;\n";
      if (llvm::any_of(plan.rules, [](const gfir::StructuredRule &rule) {
            return rule.sampled || rule.resultUses.empty() || rule.afterSnapshot;
          }))
        out << "    const bool pyc_active = pyc_sample.rising && !pyc_sample.resetActive && "
               "pyc_sample.fault == gfsim::SampledClockFault::None;\n";
    }
    for (unsigned index = 0; index < plan.rules.size(); ++index)
      needed.push_back(definition.wires.size() + index);
    auto needInputs = [&](gfir::CallOp call) {
      if (call)
        for (Value input : call.getInputs())
          needed.append(definition.valueWires.find(input)->second);
    };
    for (const auto &fifo : plan.fifos)
      for (auto call : {fifo.rawInput, fifo.rawOutput})
        needInputs(call);
    for (const auto &memory : plan.memories)
      needInputs(memory.raw);
  }
  for (unsigned index = 0; index < definition.clockedCalls.size(); ++index)
    needed.push_back(definition.firstClockedCallNode() + index);
  auto emitted = memoKeyWidths[module] ? emitMemoizedRule(out, definition)
                                       : emitWires(out, definition, needed);
  if (failed(emitted))
    return failure();
  if (definition.clocked && failed(emitPreparation(out, definition, "pyc_clock")))
    return failure();
  for (auto [index, output] : llvm::enumerate(definition.outputs))
    if (output.requiresPreparation && !wide.contains(output.port.port))
      out << "    " << preparedName(index) << " = " << wireName(output.wire) << ";\n";
  auto operation = definition.module;
  for (unsigned port = 0, count = operation.getFunctionType().getNumResults();
       port < count; ++port) {
    auto whole = wide.find(port);
    if (whole != wide.end() && whole->second.snapshot)
      out << "    " << wholeSnapshotName(port) << " = " << whole->second.source << ";\n";
  }
  auto assignAvailability = [&](StringRef member,
      ArrayRef<gfir::StructuredAvailabilitySource> sources) -> LogicalResult {
    auto available = availabilityExpression(definition, sources);
    if (failed(available))
      return failure();
    out << "    " << member << " = " << *available << ";\n";
    return success();
  };
  for (const auto &output : definition.outputAvailability)
    if (failed(assignAvailability(availabilityName(output.port), output.sources)))
      return failure();
  for (auto [index, field] : llvm::enumerate(definition.fieldOutputs))
    if (!field.sources.empty() &&
        failed(assignAvailability(fieldAvailabilityName(index), field.sources)))
      return failure();
  if (definition.hasChecks())
    out << "    pyc_checks_valid = true;\n";
  return success();
}

LogicalResult BehavioralEmitter::emitPreparation(raw_ostream &out,
                                                  const gfir::StructuredDefinition &definition,
                                                  StringRef clock) {
  const auto &plan = *definition.clocked;
  const bool hasState = !plan.variables.empty();
  // All Rules have finished issuing requests. Only prepare borrowed inputs
  // here; no owner is committed until SimSystem has accepted the sample.
  for (auto [index, fifo] : llvm::enumerate(plan.fifos)) {
    auto name = instanceName(fifo.instance);
    auto type = typedWire(fifo.element, fifo.instance);
    auto outReady = recipient(definition, fifo);
    if (failed(name) || failed(type) || failed(outReady))
      return failure();
    std::string enabled = levelName.str() + "(false)";
    std::string data = *type + "{}";
    if (auto raw = fifo.rawInput) {
      auto valid = graphWire(raw.getInputs()[0], definition, raw);
      auto payload = graphWire(raw.getInputs()[1], definition, raw);
      if (failed(valid) || failed(payload))
        return failure();
      enabled = *valid;
      data = *payload;
    } else if (fifo.producer) {
      enabled = wireName(definition.wires.size() + *fifo.producer);
      data = frameName(*fifo.producer) + "." + payloadName(index);
    }
    out << "    " << *name << ".prepare(" << clock << ", pyc_reset, "
        << enabled << ", " << data << ", " << *outReady << ");\n";
  }
  for (const auto &memory : plan.memories)
    if (failed(prepareMemory(out, definition, memory, clock)))
      return failure();
  if (!hasState) {
    if (needsDomain(plan))
      out << "    if (pyc_sample.fault == gfsim::SampledClockFault::None) "
             "pyc_domain.prepare(pyc_sample);\n";
    return success();
  }

  // FIFO preparation precedes state validation, including when a resource
  // and the sampled domain would both reject the same inputs.
  out << "    pyc_domain.prepare(pyc_sample);\n"
      << "    if (pyc_sample.rising) {\n"
      << "      if (pyc_sample.resetActive) {\n"
      << "        pyc_reset_state();\n"
      << "      } else {\n";
  llvm::DenseMap<unsigned, unsigned> writers;
  for (auto [index, rule] : llvm::enumerate(plan.rules))
    for (unsigned state : rule.writes)
      writers.try_emplace(state, index);
  for (unsigned index = 0; index < plan.variables.size(); ++index) {
    out << "        " << stateName(index);
    if (auto writer = writers.find(index); writer != writers.end()) {
      std::string enabled = wireName(definition.wires.size() + writer->second);
      if (plan.rules[writer->second].sampled)
        enabled = "pyc_behavior::Level::fromPacked(gfsim::bit_and(" + enabled +
                  ".packed(), " + levelName.str() + "(" + frameName(writer->second) + ".pyc_running).packed()))";
      if (plan.conditionalWrites.contains(index))
        enabled = "pyc_behavior::Level::fromPacked(gfsim::bit_and(" + enabled +
                  ".packed(), " + frameName(writer->second) + '.' + intentName(index) + ".packed()))";
      out << ".prepare(" << enabled
          << ", " << frameName(writer->second) << '.' << stateName(index) << ");\n";
    } else if (definition.incomingState == index) {
      out << ".prepare(" << levelName << "(pyc_incoming_enabled), pyc_incoming."
          << stateName(index) << ");\n";
    } else {
      out << ".hold();\n";
    }
  }
  out << "      }\n    }\n";
  return success();
}

LogicalResult BehavioralEmitter::prepareMemory(
    raw_ostream &out, const gfir::StructuredDefinition &definition,
    const gfir::StructuredMemory &memory, StringRef clock) {
  auto name = instanceName(memory.instance);
  if (failed(name))
    return failure();
  if (memory.transactional) {
    const auto &plan = *definition.clocked;
    unsigned memoryIndex = &memory - plan.memories.data();
    auto zero = [&](unsigned width) {
      return "gfsim::wire<gfsim::Bits<" + std::to_string(width) + ">>::known(gfsim::Bits<" + std::to_string(width) + ">{0})";
    };
    auto enabled = [&](gfir::CallOp call, unsigned port) {
      if (!call)
        return std::string("false");
      auto frame = frameName(plan.transactionMemoryCalls.lookup(call).rule);
      return "(" + frame + ".pyc_running && " + frame + "." + memoryPinName(memoryIndex, port, 3) + ")";
    };
    auto pin = [&](gfir::CallOp call, unsigned port, unsigned argument, unsigned width) {
      if (!call)
        return zero(width);
      auto frame = frameName(plan.transactionMemoryCalls.lookup(call).rule);
      return "(" + enabled(call, port) + " ? " + frame + "." +
             memoryPinName(memoryIndex, port, argument) + " : " + zero(width) + ")";
    };
    SmallVector<std::string> reads, addresses;
    for (unsigned port = 0; port < memory.readPorts(); ++port) {
      reads.push_back(levelName.str() + "(" + enabled(memory.requests[port], port) + ")");
      addresses.push_back(pin(memory.requests[port], port, 0, memory.addressWidth));
    }
    auto width = analysis.getPackedWidth(memory.element, {}, memory.instance);
    if (failed(width))
      return failure();
    if (memory.asynchronous()) {
      std::string address = zero(memory.addressWidth);
      if (memory.reader) {
        auto frame = frameName(plan.transactionMemoryCalls.lookup(memory.reader).rule);
        address = "(" + frame + "." + memoryPinName(memoryIndex, 0, 3) + " ? " +
                  frame + "." + memoryPinName(memoryIndex, 0, 0) + " : " + address + ")";
      }
      out << "    " << *name << ".prepare(" << clock << ", pyc_reset, " << address
          << ", " << levelName << '(' << enabled(memory.writer, 2) << "), "
          << pin(memory.writer, 2, 0, memory.addressWidth) << ", "
          << pin(memory.writer, 2, 1, *width) << ", " << pin(memory.writer, 2, 2, memory.maskWidth)
          << ");\n";
      return success();
    }
    out << "    " << *name << ".prepare(" << clock << ", pyc_reset, {" << join(reads, ", ")
        << "}, {" << join(addresses, ", ") << "}, " << levelName << '(' << enabled(memory.writer, 2)
        << "), " << pin(memory.writer, 2, 0, memory.addressWidth) << ", "
        << pin(memory.writer, 2, 1, *width) << ", " << pin(memory.writer, 2, 2, memory.maskWidth)
        << ");\n";
    return success();
  }
  auto raw = memory.raw;
  SmallVector<std::string> inputs;
  for (Value value : raw.getInputs()) {
    auto input = graphWire(value, definition, raw);
    if (failed(input))
      return failure();
    inputs.push_back(std::move(*input));
  }
  out << "    " << *name << ".prepare(" << clock << ", pyc_reset, ";
  if (memory.asynchronous()) {
    out << inputs.front() << ", ";
  } else {
    SmallVector<std::string> enabled, addresses;
    for (unsigned port = 0; port < memory.readPorts(); ++port) {
      enabled.push_back(inputs[2 * port]);
      addresses.push_back(inputs[2 * port + 1]);
    }
    out << '{' << join(enabled, ", ") << "}, {" << join(addresses, ", ") << "}, ";
  }
  out << join(ArrayRef<std::string>(inputs).drop_front(memory.writeOffset()), ", ")
      << ");\n";
  return success();
}

LogicalResult BehavioralEmitter::emitLifecycle(
    raw_ostream &source, const gfir::StructuredDefinition &definition,
    StringRef name, bool memoized, raw_ostream *header) {
  for (StringRef action : {"reset", "discard", "commit"}) {
    const bool inlineCommit = header && action == "commit";
    auto &out = inlineCommit ? *header : source;
    if (inlineCommit)
      out << "inline ";
    out << "void " << name << "::" << action << "() noexcept {\n";
    if (memoized && action != "commit")
      out << "  pyc_memo_key.reset();\n";
    if (definition.hasChecks())
      out << "  pyc_checks_valid = false;\n";
    for (const auto &call : definition.clockedCalls) {
      auto member = instanceName(call.receiver);
      if (failed(member))
        return failure();
      out << "  " << *member << '.' << action << "();\n";
    }
    if (definition.clocked) {
      const auto &plan = *definition.clocked;
      for (const auto &fifo : plan.fifos) {
        auto member = instanceName(fifo.instance);
        if (failed(member))
          return failure();
        out << "  " << *member << '.' << action << "();\n";
      }
      for (const auto &memory : plan.memories) {
        auto member = instanceName(memory.instance);
        if (failed(member))
          return failure();
        out << "  " << *member << '.' << action << "();\n";
      }
      if (needsDomain(plan)) {
        if (action == "commit") {
          out << "  if (pyc_domain.commit()) {\n";
          if (memoized)
            out << "    pyc_memo_key.reset();\n";
          for (unsigned index = 0; index < plan.variables.size(); ++index)
            out << "    " << stateName(index) << ".commit();\n";
          out << "  }\n";
        } else {
          out << "  pyc_domain." << action << "();\n";
          if (action == "reset" && !plan.variables.empty())
            out << "  pyc_reset_state();\n";
        }
      }
    }
    out << "}\n";
  }
  return success();
}

FailureOr<FinalCppSourceGroup *> BehavioralEmitter::getGroup(gfir::ModuleOp module) {
  auto owner = module.getSourceOwner();
  auto diagnostic = [&] { return module.emitOpError(); };
  auto names = sourceOwnerComponents(owner, diagnostic);
  if (failed(names))
    return failure();
  auto path = sourcePathName(names->filePath, ".hpp", diagnostic);
  if (failed(path))
    return failure();
  auto [entry, inserted] = groups.try_emplace(*path, parts.sourceGroups.size());
  if (inserted) {
    auto source = sourcePathName(names->filePath, ".cpp", diagnostic);
    if (failed(source))
      return failure();
    parts.sourceGroups.push_back(
        {owner, *path, *source,
         "#pragma once\n#include \"pycircuit_support.hpp\"\n",
         "#include \"" + *path + "\"\n"});
  }
  auto &group = parts.sourceGroups[entry->second];
  if (group.sourceOwner != owner)
    return module.emitOpError("C++ source owner header path collision");
  return &group;
}

FailureOr<std::string> BehavioralEmitter::emitObservationMetadata(
    const gfir::StructuredDefinition &definition) {
  if (!hasObservations(definition)) {
    parts.systemHeader += "inline std::span<const gfsim::RunnerObservation> "
                          "pyc_observation_metadata() { return {}; }\n";
    return std::string("observations_.Configure({}, 0)");
  }
  std::string descriptors, metadata;
  llvm::raw_string_ostream descriptorStream(descriptors), metadataStream(metadata);
  uint64_t registration = 0, site = 0, events = 0, slots = 0;
  for (const auto &rule : definition.clocked->rules) {
    if (rule.observations.empty())
      continue;
    for (const auto &observation : rule.observations) {
      auto effect = observation.operation;
      bool gauge = effect.getKind() == "report";
      bool carriesValue = !effect.getValues().empty();
      unsigned count = std::max<std::size_t>(1, effect.getValues().size());
      for (unsigned index = 0; index < count; ++index) {
        uint64_t width = 1;
        if (carriesValue) {
          auto payload = analysis.getPackedWidth(effect.getValues()[index].getType(), {}, effect);
          if (failed(payload))
            return failure();
          width = *payload;
        }
        uint64_t slot = observation.firstSlot + index;
        appendCppObservationDescriptor(descriptorStream,
            {slot, 0, registration, site, gauge ? 1u : 0u}, index, count, width);
        if (failed(appendCppObservationMetadata(metadataStream, effect, observation.id,
                effect.getSpecAttr(), effect.getKind(), "root", slot, carriesValue, index, count)))
          return failure();
      }
      slots += count;
      if (!gauge)
        events += count;
      ++site;
    }
    ++registration;
  }
  llvm::raw_string_ostream header(parts.systemHeader);
  header << "static constexpr gfsim::ObservationDescriptor pyc_observation_descriptors["
         << slots << "] = {" << descriptors << "\n};\n"
         << "inline std::span<const gfsim::RunnerObservation> "
            "pyc_observation_metadata() {\n"
            "  static const gfsim::RunnerObservation entries[] = {\n"
         << metadata << "  };\n  return entries;\n}\n";
  return "observations_.Configure(pyc_observation_descriptors, " +
         std::to_string(events) + ")";
}

LogicalResult BehavioralEmitter::emitRoot(FinalCppSourceGroup &group, bool inlineWork) {
  const auto &definition = program.definitions[program.rootIndex];
  auto root = definition.module;
  auto entry = program.root;
  auto policy = entry.getExecutionPolicyAttr();
  const bool resetIfOne = policy && policy.getAs<StringAttr>("reset").getValue() == "if_one";
  const bool system = entry.getKind() == "system";
  const bool observes = hasObservations(definition);
  auto name = qualifiedName(root.getSymName(), root);
  if (failed(name))
    return failure();
  parts.rootCppName = *name;
  parts.rootHeaderPath = group.headerPath;
  parts.sourceCheckCount = definition.sourceCheckCount;
  SmallVector<StringRef> components;
  StringRef(*name).drop_front(2).split(components, "::");
  llvm::raw_string_ostream header(group.header);
  for (auto component : ArrayRef<StringRef>(components).drop_back())
    header << "namespace " << component << " {\n";
  auto className = components.back();
  header << "class " << className << " final : public gfsim::SimModule {\n"
         << "public:\n  explicit " << className
         << "(std::string name, gfsim::WorkExecutor* = nullptr";
  if (observes)
    header << ", pyc_observation_sink *sink = nullptr, std::size_t base = 0";
  header << ")\n      : gfsim::SimModule(std::move(name))";
  if (observes)
    header << ", pyc_sink_(sink), pyc_observation_base_(base)";
  header << " {\n";
  if (definition.hasChecks())
    header << "    pyc_body.configure_checks(std::string(this->name()));\n";
  header << "  }\n"
         << "  bool HasWork() const noexcept override { return true; }\n"
         << "  " << (inlineWork ? "inline " : "") << "void Work() override;\n";
  if (definition.hasChecks())
    header << "  bool pyc_validate_checks(gfsim::SimFailureInfo &failure) const noexcept;\n";
  else
    header << "  bool pyc_validate_checks(gfsim::SimFailureInfo &) const noexcept "
              "{ return true; }\n";
  if (isClocked(definition))
    header << "  void Reset() noexcept override;\n"
           << "  void DiscardNext() noexcept override;\n"
           << "  " << (inlineWork ? "inline " : "") << "void Xfer() noexcept override;\n";
  SmallVector<std::string> inputs, outputs;
  llvm::json::Array inputRows, outputRows, controls;
  SmallVector<std::pair<unsigned, std::string>> companions;
  auto ports = [&](TypeRange types, ArrayAttr names, SmallVectorImpl<std::string> &wires,
                   llvm::json::Array &rows, bool output) -> LogicalResult {
    for (auto [index, pair] : llvm::enumerate(llvm::zip(types, names))) {
      auto [type, raw] = pair;
      auto original = cast<StringAttr>(raw).getValue();
      auto wire = legalizeIdentifier(original, [&] { return root.emitOpError(); });
      auto carrierType = common.cppType(type, root);
      auto leaves = analysis.getLeaves(type, {}, root);
      if (failed(wire) || failed(carrierType) || failed(leaves))
        return failure();
      header << "  gfsim::wire<" << *carrierType << "> " << *wire << ";\n";
      wires.push_back(*wire);
      llvm::json::Array fields;
      uint64_t width = 0;
      for (const auto &leaf : *leaves) {
        llvm::json::Array path;
        for (auto field : leaf.path)
          path.emplace_back(field.getValue());
        fields.emplace_back(llvm::json::Object{
            {"path", std::move(path)}, {"low", leaf.low}, {"width", leaf.width}});
        width += leaf.width;
      }
      llvm::json::Object row{{"ordinal", index}, {"name", original},
                             {"wire", *wire}, {"width", width}, {"fields", std::move(fields)}};
      if (output)
        row["availability"] = nullptr;
      rows.emplace_back(std::move(row));
    }
    return success();
  };
  auto type = root.getFunctionType();
  if (failed(ports(type.getInputs(), root.getInputNames(), inputs, inputRows, false)) ||
      failed(ports(type.getResults(), root.getOutputNames(), outputs, outputRows, true)))
    return failure();
  for (const auto &output : definition.outputAvailability) {
    auto wire = legalizeIdentifier(output.name.getValue(), [&] { return root.emitOpError(); });
    if (failed(wire))
      return failure();
    header << "  pyc_behavior::Level " << *wire << ";\n";
    (*outputRows[output.port].getAsObject())["availability"] = *wire;
    controls.emplace_back(llvm::json::Object{{"direction", "output"},
                                             {"role", "availability"},
                                             {"wire", *wire}, {"width", 1}});
    outputs.push_back(*wire);
    companions.emplace_back(output.port, std::move(*wire));
  }
  llvm::json::Object domains{{"clock", nullptr}, {"reset", nullptr}};
  std::string clock, reset;
  if (isClocked(definition) && !definition.explicitDomains) {
    for (auto [role, port] : {std::pair{"clock", "pyc_clk"}, {"reset", "pyc_rst"}}) {
      auto pin = legalizeIdentifier(port, [&] { return root.emitOpError(); });
      if (failed(pin))
        return failure();
      domains[role] = inputs.size();
      inputRows.emplace_back(llvm::json::Object{
          {"ordinal", inputs.size()}, {"name", port}, {"wire", *pin}, {"width", 1},
          {"fields", llvm::json::Array{llvm::json::Object{
              {"path", llvm::json::Array{}}, {"low", 0}, {"width", 1}}}}});
      inputs.push_back(*pin);
      header << "  pyc_behavior::Level " << *pin << ";\n";
      (StringRef(role) == "clock" ? clock : reset) = *pin;
    }
  }

  header << "private:\n  " << moduleName(program.rootIndex) << " pyc_body;\n";
  if (observes)
    header << "  pyc_observation_sink *pyc_sink_;\n"
           << "  std::size_t pyc_observation_base_;\n";
  if (system)
    header << "  pyc_system_sampled_clock pyc_system_clock;\n";
  header << "};\n";
  for (size_t i = 1; i < components.size(); ++i)
    header << "}\n";

  llvm::raw_string_ostream source(group.source);
  StringRef scopedName = StringRef(*name).drop_front(2);
  auto &work = inlineWork ? header : source;
  if (inlineWork)
    work << "inline ";
  work << "void " << scopedName << "::Work() {\n";
  if (isClocked(definition)) {
    work << "  try {\n";
    if (!definition.explicitDomains)
      work << "    const auto pyc_reset = gfsim::interpretReset(" << reset
           << ", gfsim::ResetSemantics::" << (resetIfOne ? "IfOne" : "Strict") << ");\n";
  }

  llvm::SmallDenseSet<unsigned> declaredInputs;
  for (const auto &output : definition.outputs) {
    for (unsigned input : output.inputs)
      declaredInputs.insert(input);
  }
  for (unsigned input : definition.preparationInputs)
    declaredInputs.insert(input);

  for (auto [index, wire] : llvm::enumerate(definition.wires)) {
    if (!isa<BlockArgument>(wire.endpoint.value) ||
        !declaredInputs.contains(index))
      continue;
    auto arg = cast<BlockArgument>(wire.endpoint.value).getArgNumber();
    const bool directTable = isa<ac::TableType>(wire.endpoint.value.getType());
    work << "    const auto " << (directTable ? "&" : "") << wireName(index)
           << " = " << graphLeaf(wire, inputs[arg]) << ";\n";
  }
  if (isClocked(definition)) {
    work << "    pyc_body.prepare(";
    bool comma = !definition.explicitDomains;
    if (comma)
      work << clock << ", pyc_reset";
    for (unsigned input : definition.preparationInputs) {
      if (comma)
        work << ", ";
      work << wireName(input);
      comma = true;
    }
    if (observes)
      work << ", pyc_sink_, pyc_observation_base_";
    work << ");\n";
  }
  if (system)
    work << "    pyc_system_clock.prepare(" << clock << ", " << reset
           << ", gfsim::ResetSemantics::" << (resetIfOne ? "IfOne" : "Strict") << ");\n";

  const auto &wide = wholeOutputs[program.rootIndex];
  llvm::SmallDenseSet<unsigned> assigned;
  for (auto [index, output] : llvm::enumerate(definition.outputs)) {
    if (wide.contains(output.port.port)) {
      if (assigned.insert(output.port.port).second)
        work << "    " << outputs[output.port.port] << " = pyc_body."
               << wholeOutputName(output.port.port) << "();\n";
      continue;
    }
    // The source leaf may be a scalar projected from a wider result port.
    // Use the layout of that port, not the layout of the producing value.
    auto leaves = analysis.getLeaves(type.getResult(output.port.port), {}, root);
    if (failed(leaves))
      return failure();
    auto leaf = llvm::find_if(*leaves, [&](const ac::PackedLeaf &item) {
      return item.path == output.port.path;
    });
    if (leaf == leaves->end())
      return root.emitOpError("unresolved behavioral output layout");
    SmallVector<std::string> actuals;
    if (!output.requiresPreparation)
      for (unsigned input : output.inputs)
        actuals.push_back(wireName(input));
    auto value = "pyc_body." + functionName(index) + '(' + join(actuals, ", ") + ')';
    if (isa<ac::TableType>(type.getResult(output.port.port))) {
      work << "    " << outputs[output.port.port] << " = " << value << ";\n";
    } else {
      if (isa<ac::TableType>(leaf->type))
        value = pack(value);
      work << "    " << outputs[output.port.port] << ".assignSlice(" << leaf->low
             << ", " << value << ");\n";
    }
  }
  for (const auto &[port, wire] : companions)
    work << "    " << wire << " = " << levelName << "(pyc_body."
           << availabilityName(port) << ");\n";
  if (isClocked(definition))
    work << "  } catch (...) {\n    DiscardNext();\n    throw;\n  }\n";
  work << "}\n";
  if (isClocked(definition)) {
    for (auto [hook, action] : {std::pair{"Reset", "reset"},
                              {"DiscardNext", "discard"}, {"Xfer", "commit"}}) {
      const bool inlineXfer = inlineWork && StringRef(hook) == "Xfer";
      auto &target = inlineXfer ? header : source;
      if (inlineXfer)
        target << "inline ";
      target << "void " << scopedName << "::" << hook << "() noexcept {\n"
             << "  pyc_body." << action << "();\n";
      if (system)
        target << "  pyc_system_clock." << action << "();\n";
      target << "}\n";
    }
  }

  if (definition.hasChecks()) {
    source << "bool " << scopedName
           << "::pyc_validate_checks(gfsim::SimFailureInfo &failure) const noexcept {\n"
              "  failure = {};\n"
              "  return pyc_body.validate_sampling(failure) && pyc_body.validate_checks(failure)";
    if (hasAssertions(definition))
      source << " && pyc_body.validate_assertions(failure)";
    source << ";\n}\n";
  }

  parts.systemHeader = "#pragma once\n#include \"" + parts.rootHeaderPath +
                       "\"\nusing pyc_root = " + *name + ";\n";
  std::string configure = "observations_.Configure({}, 0)";
  if (system) {
    parts.systemHeader += "#include <gfsim/SystemRunner.h>\n";
    auto observations = emitObservationMetadata(definition);
    if (failed(observations))
      return failure();
    configure = std::move(*observations);
    addCppSystemDriver(parts, clock, reset, program.samples, resetIfOne);
  }
  appendCppDutHeader(parts, inputs, outputs, configure,
                     /*hasChecks=*/definition.hasChecks(), observes);
  llvm::raw_string_ostream map(parts.portMap);
  map << llvm::json::Value(llvm::json::Object{
      {"schema_version", 1}, {"target", "cpp"}, {"inputs", std::move(inputRows)},
      {"outputs", std::move(outputRows)}, {"controls", std::move(controls)},
      {"domains", std::move(domains)}});
  return success();
}

FailureOr<FinalCppSourceParts> BehavioralEmitter::emit() {
  CppNameScope names;
  for (auto record : analysis.getStructs()) {
    auto name = qualifiedName(record.getSymName(), record);
    if (failed(name) || failed(names.insert(*name, record)))
      return failure();
  }
  for (auto enumeration : analysis.getPackage().getOps<ac::EnumOp>()) {
    auto name = qualifiedName(enumeration.getSymName(), enumeration);
    if (failed(name) || failed(names.insert(*name, enumeration)))
      return failure();
  }
  for (const auto &definition : program.definitions) {
    auto module = definition.module;
    auto name = qualifiedName(module.getSymName(), module);
    if (failed(name) || failed(names.insert(*name, module)))
      return failure();
    for (const auto &wire : definition.wires) {
      if (!isa<ac::TableType>(wire.type) || tableWireTypes.contains(wire.type))
        continue;
      auto type = typedWire(wire.type, module);
      if (failed(type))
        return failure();
      tableWireTypes.try_emplace(wire.type, std::move(*type));
    }
  }
  if (failed(planWholeOutputs()))
    return failure();
  planEvaluationMemoization();
  outputGroups.resize(program.definitions.size());
  groupWorkRemaining.resize(program.definitions.size());
  for (auto [index, definition] : llvm::enumerate(program.definitions))
    for (const auto &output : definition.outputs) {
      uint64_t work = wireClosure(definition, {output.wire}).count();
      groupWorkRemaining[index] =
          work > std::numeric_limits<uint64_t>::max() - groupWorkRemaining[index]
              ? std::numeric_limits<uint64_t>::max()
              : groupWorkRemaining[index] + work;
    }
  parts.supportHeader = "#pragma once\n#include <gfsim/wire.h>\n#include <tuple>\n"
                        "#include <gfsim/SimModule.h>\n#include <gfsim/WorkExecutor.h>\n";
  if (llvm::any_of(program.definitions, [](const gfir::StructuredDefinition &definition) {
        return definition.hasChecks();
      }))
    parts.supportHeader += "#include <gfsim/SimSystem.h>\n#include <limits>\n#include <string>\n";
  else
    parts.supportHeader += "namespace gfsim { struct SimFailureInfo; }\n";
  if (hasObservations(program.definitions[program.rootIndex]))
    parts.supportHeader += cppObservationSupport().str();
  parts.supportHeader += behavioralValueSupport.str();
  bool usesModelSupport = llvm::any_of(program.definitions, [](const auto &definition) {
    return isClocked(definition) || llvm::any_of(definition.wires,
        [](const gfir::StructuredWire &wire) {
          return isa_and_nonnull<ac::TableSplatOp, ac::TableGetOp, ac::TableMapOp>(
              wire.endpoint.value.getDefiningOp());
        });
  });
  auto root = program.root;
  bool system = root.getKind() == "system";
  if (usesModelSupport || system)
    parts.supportHeader += "#include <gfsim/sampled_clock.h>\n";
  if (usesModelSupport)
    parts.supportHeader += behavioralCppSupport.str();
  if (system)
    parts.supportHeader += cppSystemClockSupport().str();
  llvm::raw_string_ostream support(parts.supportHeader);
  if (failed(emitCppRecords(common, support)))
    return failure();
  const bool inlineWork = inlineRootWork(program);
  for (auto [index, definition] : llvm::enumerate(program.definitions)) {
    auto group = getGroup(definition.module);
    if (failed(group))
      return failure();
    llvm::raw_string_ostream source((*group)->source);
    llvm::raw_string_ostream header((*group)->header);
    if (failed(emitModule(source, index, inlineWork ? &header : nullptr)))
      return failure();
  }
  auto rootGroup = getGroup(program.definitions[program.rootIndex].module);
  if (failed(rootGroup) || failed(emitRoot(**rootGroup, inlineWork)))
    return failure();
  if (failed(emitOutputGroups()))
    return failure();
  for (unsigned index = 0; index < program.definitions.size(); ++index)
    if (failed(declareModule(support, index)))
      return failure();
  return std::move(parts);
}

} // namespace

FailureOr<FinalCppSourceParts>
emitBehavioralCpp(ModuleOp package, const gfir::StructuredProgram &program) {
  return BehavioralEmitter(package, program).emit();
}

} // namespace acir::compiler
