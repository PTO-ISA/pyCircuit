#include "BehavioralEmitCpp.h"
#include "FinalCppNames.h"
#include "HardwareEmitCommon.h"
#include "pycircuit/Dialect/GFIR/StructuredProgram.h"
#include "pycircuit/Dialect/GFIR/StructuredTableMap.h"
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
#include <functional>
#include <limits>
#include <map>
#include <memory>

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
std::string pureAvailabilityName(unsigned output) { return "pyc_availability_output_" + std::to_string(output); }
std::string inputAvailabilityName(const gfir::StructuredInputAvailability &input, unsigned ordinal) {
  return input.path.empty() ? "pyc_input_available_" + std::to_string(input.port)
                            : "pyc_input_field_available_" + std::to_string(ordinal);
}
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
std::string incomingCandidateName(unsigned index) {
  return "candidate_" + std::to_string(index);
}
std::string formalStateName(unsigned index) {
  return "pyc_formal_" + std::to_string(index);
}

void bindMethodArguments(raw_ostream &out, gfir::MethodOp method,
                         ArrayRef<std::pair<unsigned, std::string>> places,
                         SmallVectorImpl<std::string> &arguments, StringRef prefix) {
  SmallVector<std::string> inputs, bindings;
  unsigned supplied = 0;
  for (unsigned formal = 0; formal < method.getFunctionType().getNumInputs(); ++formal) {
    auto place = llvm::find_if(places, [&](const auto &entry) { return entry.first == formal; });
    if (place == places.end()) {
      inputs.push_back(arguments[supplied++]);
      continue;
    }
    auto snapshot = prefix.str() + "_" + formalStateName(formal) + "_entry";
    out << "    const auto " << snapshot << " = " << place->second << ".value();\n";
    inputs.push_back(std::move(snapshot));
    bindings.push_back(place->second);
  }
  arguments.assign(inputs.begin(), inputs.end());
  arguments.append(bindings.begin(), bindings.end());
}
std::string observationName(unsigned slot) {
  return "pyc_observation_" + std::to_string(slot);
}
std::string observationReachedName(unsigned slot) {
  return observationName(slot) + "_reached";
}

bool isClocked(const gfir::StructuredDefinition &definition) {
  return definition.clocked.has_value() || !definition.clockedCalls.empty();
}

bool hasIncomingWrites(const gfir::StructuredDefinition &definition) {
  return llvm::any_of(definition.incomingStates, [](const auto &state) {
    return state.writable;
  });
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

enum class ResourceDemandKind { Boundary, Queue, MemoryLive, Branch };
struct OrderedFifoBranchEmission {
  struct Control {
    unsigned ordinal;
    ResourceDemandKind kind;
    std::optional<std::pair<unsigned, bool>> path;
  };
  SmallVector<Control> controls;
  std::optional<std::pair<unsigned, bool>> path;
  const gfir::StructuredRule *rule = nullptr;
};

std::string orderedFifoCredit(const OrderedFifoBranchEmission &ordered, unsigned fifo) {
  auto external = "txn." + recipientName(fifo);
  if (!llvm::any_of(ordered.rule->requirements, [&](const auto &requirement) {
        return requirement.fifo == fifo && requirement.requiresReady && requirement.ownPop;
      }))
    return external;
  return "pyc_behavior::Level::fromPacked(gfsim::bit_or(txn.pyc_raw_pops[" +
      std::to_string(fifo) + "].packed(), " + external + ".packed()))";
}

bool hasOrderedFifoBranches(const gfir::StructuredRule &rule) {
  auto operation = rule.operation;
  return rule.sampled && !rule.requirements.empty() &&
      llvm::any_of(operation.getBody().front(),
                   [](Operation &op) { return isa<gfir::IfOp>(op); });
}

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

FailureOr<std::string> instancePath(ArrayRef<gfir::InstanceOp> path) {
  SmallVector<std::string> members;
  for (auto instance : path) {
    auto name = instanceName(instance);
    if (failed(name))
      return failure();
    members.push_back(std::move(*name));
  }
  return join(members, ".");
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

struct NativeOutputEntry {
  SmallVector<unsigned> tables;
  std::string body;
  gfir::ModuleOp soleCaller;
  unsigned callSites = 0;
  bool inlineBody = false;
};

constexpr llvm::StringLiteral nativeInlineAttribute =
    "#if defined(__clang__) && defined(__OPTIMIZE__)\n"
    "[[clang::always_inline]]\n"
    "#endif\n";

struct OutputGroup {
  SmallVector<unsigned> outputs;
  SmallVector<unsigned> inputs;
  bool reuseKnownness = false;
  SmallVector<unsigned> checkedInputs;
  std::string nativeBody;
  std::unique_ptr<NativeOutputEntry> nativeEntry;
};

struct BoundOutputGroup {
  unsigned definition;
  unsigned ordinal;
  gfir::InstanceOp receiver;
  SmallVector<unsigned> inputs;
  SmallVector<unsigned> results;
};

struct CaptureGraphStep {
  unsigned wire;
  std::optional<BoundOutputGroup> group;
};

struct GroupKnownness {
  llvm::SmallDenseSet<unsigned> checked;
  llvm::DenseMap<unsigned, unsigned> nativeResults;
};

std::string inputKnownName(unsigned wire) {
  return "pyc_input_known_" + std::to_string(wire);
}

std::string resultKnownName(unsigned wire) {
  return "pyc_group_known_" + std::to_string(wire);
}

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

std::optional<unsigned> forwardedTable(
    const gfir::StructuredDefinition &definition, unsigned index) {
  const auto &wire = definition.wires[index];
  if (!isa<ac::TableType>(wire.type))
    return std::nullopt;
  auto *op = wire.endpoint.value.getDefiningOp();
  if (wire.dependencies.size() == 1 &&
      isa_and_nonnull<ac::StructCreateOp, ac::StructGetOp>(op)) {
    unsigned input = wire.dependencies.front();
    if (definition.wires[input].type == wire.type)
      return input;
  }
  auto map = dyn_cast_or_null<ac::TableMapOp>(op);
  if (!map)
    return std::nullopt;
  gfir::detail::TableMapRow row(map,
      cast<OpResult>(wire.endpoint.value).getResultNumber());
  auto forwarded = row.tableOperand(row.getValue());
  if (auto select = row.getValue().getDefiningOp<ac::BitsSelectOp>()) {
    auto yes = row.tableOperand(select.getTrueValue());
    auto no = row.tableOperand(select.getFalseValue());
    if (yes && no && map.getTables()[*yes] == map.getTables()[*no])
      forwarded = yes;
  }
  if (!forwarded)
    return std::nullopt;
  Value input = map.getTables()[*forwarded];
  if (input.getType() != wire.type)
    return std::nullopt;
  auto wires = definition.valueWires.find(input);
  if (wires != definition.valueWires.end() && wires->second.size() == 1 &&
      llvm::is_contained(wire.dependencies, wires->second.front()))
    return wires->second.front();
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

struct WholeInput {
  BlockArgument argument;
  std::string type;
};

constexpr llvm::StringLiteral wholeInputName = "pyc_whole_input";

struct RecordFieldLayout {
  unsigned low;
  unsigned width;
  Type type;
  std::string wire;
  bool written = false;
  unsigned indexWidth = 0;
};

using RecordStateLayout = SmallVector<RecordFieldLayout>;

struct RecordCaptureLayout {
  SmallVector<unsigned> fields;
  std::string type;
};

struct KnownRecordEdit {
  const RecordFieldLayout *field;
  unsigned value;
  unsigned width;
  std::optional<unsigned> row;
  std::optional<unsigned> condition;
};

struct KnownRecordPlan {
  unsigned snapshotPort;
  unsigned snapshotCapture;
  SmallVector<KnownRecordEdit> edits;
};

std::string outputGroupName(unsigned index) {
  return "output_group_" + std::to_string(index);
}

std::string outputGroupType(unsigned index) {
  return "OutputGroup" + std::to_string(index);
}

std::string nativeGroupInput(unsigned index) {
  return outputGroupType(index) + "Inputs";
}

std::string nativeGroupResult(unsigned index) {
  return outputGroupType(index) + "Known";
}

StringRef nativeInteger(unsigned width) {
  if (width <= 8)
    return "std::uint8_t";
  if (width <= 16)
    return "std::uint16_t";
  if (width <= 32)
    return "std::uint32_t";
  return "std::uint64_t";
}

void groupSignature(raw_ostream &out, const gfir::StructuredDefinition &definition,
                    const TableWireTypes &tables, const OutputGroup &group,
                    unsigned index, StringRef owner = {}) {
  out << owner << outputGroupType(index) << ' ' << owner
      << outputGroupName(index) << '(';
  llvm::interleaveComma(group.inputs, out, [&](unsigned input) {
    out << "const " << graphType(definition.wires[input], tables) << " &" << wireName(input);
  });
  if (!group.checkedInputs.empty())
    out << ", bool pyc_inputs_known";
  out << ") const";
}

void preparationSignature(raw_ostream &out,
                          const gfir::StructuredDefinition &definition,
                          const TableWireTypes &tables,
                          const WholeInput *whole,
                          StringRef owner = {}) {
  out << "void " << owner << "prepare(";
  bool comma = !definition.explicitDomains;
  if (comma)
    out << "const pyc_behavior::Level &pyc_clock, "
           "const pyc_behavior::Level &pyc_reset";
  auto parameter = [&](StringRef type, StringRef name) {
    if (comma)
      out << ", ";
    out << "const " << type << " &" << name;
    comma = true;
  };
  if (whole)
    parameter(whole->type, wholeInputName);
  else
    for (unsigned input : definition.preparationInputs)
      parameter(graphType(definition.wires[input], tables), wireName(input));
  for (auto [ordinal, input] : llvm::enumerate(definition.inputAvailability))
    parameter("pyc_behavior::Level", inputAvailabilityName(input, ordinal));
  if (hasIncomingWrites(definition))
    parameter("IncomingState", "pyc_incoming");
  if (hasObservations(definition))
    out << ", const pyc_observation_sink *pyc_sink, std::size_t pyc_observation_base";
  out << ')';
}

SmallVector<unsigned> pureAvailabilityInputs(const gfir::StructuredDefinition &definition,
                                            unsigned result) {
  SmallVector<unsigned> ports;
  const auto &output = definition.outputs[result];
  for (auto [ordinal, input] : llvm::enumerate(definition.inputAvailability)) {
    if (llvm::is_contained(output.inputs, input.wire))
      ports.push_back(ordinal);
  }
  return ports;
}

void pureAvailabilitySignature(raw_ostream &out, const gfir::StructuredDefinition &definition,
                               const TableWireTypes &tables, unsigned result,
                               StringRef owner = {}) {
  out << "pyc_behavior::Level " << owner << pureAvailabilityName(result) << '(';
  SmallVector<std::string> parameters;
  for (unsigned input : definition.outputs[result].inputs)
    parameters.push_back("const " + graphType(definition.wires[input], tables) + " &" + wireName(input));
  for (unsigned ordinal : pureAvailabilityInputs(definition, result))
    parameters.push_back("const pyc_behavior::Level &" + inputAvailabilityName(definition.inputAvailability[ordinal], ordinal));
  out << join(parameters, ", ") << ") const";
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
                          bool groupSelects = false,
                          SmallVectorImpl<CaptureGraphStep> *schedule = nullptr,
                          bool emitAvailability = true);
  FailureOr<std::optional<unsigned>> internOutputGroup(
      unsigned definition, ArrayRef<unsigned> outputs, bool reuseKnownness);
  FailureOr<bool> emitOutputGroup(raw_ostream &out,
                                  const gfir::StructuredDefinition &definition,
                                  unsigned first, ArrayRef<unsigned> candidates,
                                  llvm::BitVector &emitted, StringRef instancePrefix,
                                  GroupKnownness *knownness,
                                  SmallVectorImpl<CaptureGraphStep> *schedule);
  FailureOr<bool> emitKnownOutputGroup(raw_ostream &out,
                                      const gfir::StructuredDefinition &definition,
                                      const OutputGroup &group, ArrayRef<unsigned> roots,
                                      SmallVectorImpl<unsigned> *checkedInputs = nullptr,
                                      SmallVectorImpl<unsigned> *nativeTables = nullptr,
                                      bool scalarResult = false);
  FailureOr<bool> emitNativeCaptures(raw_ostream &out,
                                     const gfir::StructuredDefinition &definition,
                                     const gfir::StructuredRule &rule,
                                     ArrayRef<CaptureGraphStep> schedule,
                                     ArrayRef<std::string> captures);
  std::optional<KnownRecordPlan> planKnownRecord(
      const gfir::StructuredDefinition &definition, const gfir::StructuredRule &rule) const;
  const KnownRecordPlan *knownRecord(const gfir::StructuredDefinition &definition) const;
  void declareKnownRecord(raw_ostream &out, const KnownRecordPlan &plan);
  LogicalResult planNativeOutputInlining();
  LogicalResult emitOutputGroupBody(unsigned index, unsigned ordinal);
  LogicalResult emitOutputCallables();
  LogicalResult planWholeOutputs();
  LogicalResult planWholeInputs();
  const WholeInput *wholeInput(const gfir::StructuredDefinition &definition) const;
  LogicalResult planRecordStates();
  const RecordStateLayout *recordLayout(
      const gfir::StructuredDefinition &definition, unsigned state) const;
  const RecordFieldLayout *recordFieldLayout(
      const gfir::StructuredDefinition &definition, const gfir::StructuredWire &wire) const;
  std::optional<std::string> recordField(
      const gfir::StructuredDefinition &definition, const gfir::StructuredWire &wire,
      StringRef instancePrefix) const;
  void declareRecordState(raw_ostream &out, const RecordStateLayout &layout,
                          StringRef payload, unsigned state, const KnownRecordPlan *known);
  void emitRecordWrite(raw_ostream &out, const RecordStateLayout &layout,
                       gfir::WriteOp write, const gfir::StructuredWritePath &path,
                       StringRef frame, const llvm::DenseMap<Value, std::string> &values);
  FailureOr<std::string> ruleInputType(BlockArgument argument, gfir::RuleOp rule);
  void planEvaluationMemoization();
  LogicalResult declareModule(raw_ostream &out, unsigned index);
  LogicalResult emitOutput(raw_ostream &out, unsigned index, unsigned result);
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
  bool isNativeScalar(Operation &op, bool allowShifts = false);
  FailureOr<std::string> nativeScalarExpression(Operation &op, unsigned width,
                                                ArrayRef<std::string> args);
  FailureOr<bool> emitScalarRegion(raw_ostream &out, ArrayRef<Operation *> operations,
                                   llvm::DenseMap<Value, std::string> &values);
  FailureOr<bool> emitKnownRuleSuffix(raw_ostream &out, Operation &first,
                                     const gfir::StructuredClocked &clocked,
                                     const llvm::DenseMap<Value, std::string> &prefix);
  FailureOr<bool> emitNativeTransaction(raw_ostream &out,
      const gfir::StructuredDefinition &definition,
      const gfir::StructuredRule &rule);
  LogicalResult emitInitial(raw_ostream &out, gfir::VarOp variable, unsigned index);
  LogicalResult emitBody(raw_ostream &out, Block &body,
                         const gfir::StructuredDefinition &definition,
                         const gfir::StructuredRule *rule = nullptr,
                         const llvm::DenseMap<Value, std::string> *outer = nullptr,
                         ArrayRef<std::string> stepTargets = {},
                         StringRef stepNatural = {}, StringRef stepOrdinal = {},
                         OrderedFifoBranchEmission *ordered = nullptr);
  LogicalResult emitLoop(raw_ostream &out, gfir::LoopOp loop,
                         const gfir::StructuredDefinition &definition,
                         const llvm::DenseMap<Value, std::string> &values,
                         ArrayRef<std::string> results);
  LogicalResult emitBranch(raw_ostream &out, gfir::IfOp branch,
                           const gfir::StructuredDefinition &definition,
                           const llvm::DenseMap<Value, std::string> &values,
                           ArrayRef<std::string> results,
                           OrderedFifoBranchEmission *ordered = nullptr);
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
  SmallVector<llvm::BitVector> usedOutputs;
  SmallVector<llvm::DenseMap<unsigned, WholeOutput>> wholeOutputs;
  llvm::DenseMap<Operation *, WholeInput> wholeInputs;
  llvm::BitVector memoizedRules;
  SmallVector<uint64_t> groupWorkRemaining;
  llvm::DenseMap<Operation *, RecordStateLayout> recordLayouts;
  llvm::DenseMap<Value, RecordCaptureLayout> recordCaptures;
  llvm::DenseMap<Operation *, KnownRecordPlan> knownRecords;
};

#include "BehavioralRecordState.inc"
#include "BehavioralKnownRecord.inc"

#include "BehavioralGraph.inc"

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
  memoizedRules.resize(program.definitions.size());
  for (auto [index, definition] : llvm::enumerate(program.definitions)) {
    if (!definition.clocked || definition.explicitDomains || definition.hasChecks() ||
        !definition.inputAvailability.empty() ||
        hasIncomingWrites(definition) || !definition.clockedCalls.empty() ||
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
      memoizedRules.set(index);
  }
}

LogicalResult BehavioralEmitter::planWholeInputs() {
  for (auto [index, definition] : llvm::enumerate(program.definitions)) {
    if (!memoizedRules.test(index) || definition.preparationInputs.size() < 2)
      continue;
    const auto &rule = definition.clocked->rules.front();
    // Only the root already receives the complete Struct at its public
    // boundary. Other split Rules still get the fields their callers schedule.
    if (rule.afterSnapshot && index != program.rootIndex)
      continue;
    auto operation = rule.operation;
    auto argument = dyn_cast<BlockArgument>(
        definition.wires[definition.preparationInputs.front()].endpoint.value);
    if (!argument || !isa<ac::StructType>(argument.getType()) ||
        (!rule.afterSnapshot &&
         !llvm::all_of(argument.getUsers(), [&](Operation *user) { return user == operation; })))
      continue;
    if (llvm::any_of(llvm::zip_equal(operation.getCaptures(), operation.getBody().front().getArguments()),
                     [&](const auto &binding) {
          return std::get<0>(binding) == argument && recordCaptures.contains(std::get<1>(binding));
        }))
      continue;
    auto leaves = definition.valueWires.find(argument);
    if (leaves == definition.valueWires.end() ||
        leaves->second.size() != definition.preparationInputs.size() ||
        !llvm::all_of(leaves->second, [&](unsigned leaf) {
          return llvm::is_contained(definition.preparationInputs, leaf);
        }))
      continue;
    // All preparation leaves belong to this exact nominal value. The
    // aggregate is a transport and memo key; the leaf schedule stays intact.
    auto wire = typedWire(argument.getType(), operation);
    if (failed(wire))
      return failure();
    wholeInputs.try_emplace(definition.module, WholeInput{argument, std::move(*wire)});
  }
  return success();
}

const WholeInput *BehavioralEmitter::wholeInput(
    const gfir::StructuredDefinition &definition) const {
  auto found = wholeInputs.find(definition.module);
  return found == wholeInputs.end() ? nullptr : &found->second;
}

LogicalResult BehavioralEmitter::declareModule(raw_ostream &out, unsigned index) {
  const auto &definition = program.definitions[index];
  out << "struct " << moduleName(index) << " {\n";
  for (unsigned result = 0; result < definition.outputs.size(); ++result) {
    if (!usedOutputs[index].test(result))
      continue;
    out << "  ";
    signature(out, definition, tableWireTypes, result);
    out << ";\n";
    if (!isClocked(definition) && !definition.wireAvailability.empty() &&
        !definition.wireAvailability[definition.outputs[result].wire].empty()) {
      out << "  ";
      pureAvailabilitySignature(out, definition, tableWireTypes, result);
      out << ";\n";
    }
  }
  for (auto [ordinal, group] : llvm::enumerate(outputGroups[index])) {
    out << "  struct " << outputGroupType(ordinal) << " {\n";
    for (unsigned output : group.outputs)
      out << "    " << graphType(definition.wires[definition.outputs[output].wire], tableWireTypes)
          << ' ' << resultName(output) << ";\n";
    if (!group.checkedInputs.empty())
      out << "    bool pyc_results_known = false;\n";
    out << "  };\n";
    out << "  ";
    groupSignature(out, definition, tableWireTypes, group, ordinal);
    out << ";\n";
    if (group.nativeEntry) {
      out << "  struct " << nativeGroupInput(ordinal) << " {\n";
      for (unsigned input : group.checkedInputs)
        out << "    " << nativeInteger(definition.wires[input].width) << ' '
            << wireName(input) << ";\n";
      for (unsigned input : group.nativeEntry->tables)
        out << "    const " << graphType(definition.wires[input], tableWireTypes) << " &"
            << wireName(input) << ";\n";
      out << "  };\n  struct " << nativeGroupResult(ordinal) << " {\n";
      for (unsigned output : group.outputs) {
        unsigned wire = definition.outputs[output].wire;
        out << "    " << nativeInteger(definition.wires[wire].width) << ' '
            << resultName(output) << ";\n";
      }
      out << "  };\n";
      if (group.nativeEntry->inlineBody)
        out << nativeInlineAttribute << "  inline ";
      else
        out << "  ";
      out << "bool " << outputGroupName(ordinal) << "_known(const "
          << nativeGroupInput(ordinal) << " &pyc_inputs, " << nativeGroupResult(ordinal)
          << " &pyc_results) const;\n";
    }
  }
  if (isClocked(definition)) for (const auto &output : definition.outputAvailability)
    out << "  " << (definition.fourStateAvailability ? "pyc_behavior::Level" : "bool") << ' '
        << availabilityName(output.port) << (definition.fourStateAvailability ? " = pyc_behavior::level(false);\n" : " = false;\n");
  for (auto [index, field] : llvm::enumerate(definition.fieldOutputs))
    if (!field.sources.empty())
      out << "  " << (definition.fourStateAvailability ? "pyc_behavior::Level" : "bool") << ' '
          << fieldAvailabilityName(index) << (definition.fourStateAvailability ? " = pyc_behavior::level(false);\n" : " = false;\n");
  if (hasIncomingWrites(definition)) {
    out << "  struct IncomingState {\n";
    for (auto [index, incoming] : llvm::enumerate(definition.incomingStates)) {
      if (!incoming.writable)
        continue;
      const auto &route = incoming.route;
      auto variable = program.definitions[route.definition].clocked->variables[route.state];
      auto type = typedWire(variable.getInitial().getType(), variable);
      if (failed(type))
        return failure();
      out << "    const std::optional<" << *type << "> *" << incomingCandidateName(index)
          << " = nullptr;\n";
    }
    out << "  };\n";
  }
  if (isClocked(definition)) {
    out << "  ";
    preparationSignature(out, definition, tableWireTypes, wholeInput(definition));
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
    if (memoizedRules.test(index)) {
      const auto &rule = definition.clocked->rules.front();
      if (const auto *whole = wholeInput(definition)) {
        out << "  std::optional<" << whole->type << "> pyc_memo_key;\n";
      } else {
        out << "  std::optional<std::tuple<";
        llvm::interleaveComma(definition.preparationInputs, out, [&](unsigned input) {
          out << graphType(definition.wires[input], tableWireTypes);
        });
        out << ">> pyc_memo_key;\n";
      }
      if (rule.afterSnapshot) {
        auto operation = rule.operation;
        SmallVector<std::string> types;
        for (BlockArgument value : operation.getBody().front().getArguments()) {
          auto type = ruleInputType(value, operation);
          if (failed(type))
            return failure();
          types.push_back(std::move(*type));
        }
        out << "  std::optional<std::tuple<" << join(types, ", ") << ">> pyc_memo_values;\n";
        if (const auto *known = knownRecord(definition))
          out << "  struct KnownMemo {\n"
              << "    " << wide.find(known->snapshotPort)->second.type << " snapshot;\n"
              << "    " << recordLayoutName(0) << "::KnownEdit edit;\n"
              << "    bool published = false;\n"
              << "  };\n"
              << "  std::optional<KnownMemo> pyc_memo_known;\n";
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

LogicalResult BehavioralEmitter::emitOutput(raw_ostream &out, unsigned index,
                                           unsigned result) {
  const auto &definition = program.definitions[index];
  const auto &wide = wholeOutputs[index];
  const auto &output = definition.outputs[result];
  signature(out, definition, tableWireTypes, result, moduleName(index) + "::");
  out << " {\n";
  if (wide.contains(output.port.port)) {
    auto value = graphLeaf(definition.wires[output.wire],
                           wholeOutputName(output.port.port) + "()");
    out << "    return " << value << ";\n}\n";
  } else if (output.requiresPreparation) {
    out << "    return " << preparedName(result) << ";\n}\n";
  } else {
    if (failed(emitWires(out, definition, {output.wire}, {}, false, nullptr,
                         isClocked(definition))))
      return failure();
    out << "    return " << wireName(output.wire) << ";\n}\n";
  }
  if (!isClocked(definition) && !definition.wireAvailability.empty() &&
      !definition.wireAvailability[output.wire].empty()) {
    pureAvailabilitySignature(out, definition, tableWireTypes, result, moduleName(index) + "::");
    out << " {\n";
    if (failed(emitWires(out, definition, {output.wire}))) return failure();
    auto available = availabilityExpression(definition, definition.wireAvailability[output.wire]);
    if (failed(available)) return failure();
    out << "    return " << *available << ";\n}\n";
  }
  return success();
}

LogicalResult BehavioralEmitter::emitModule(raw_ostream &out, unsigned index,
                                            raw_ostream *header) {
  const auto &definition = program.definitions[index];
  if (isClocked(definition)) {
    auto &prepare = header ? *header : out;
    if (header)
      prepare << "inline ";
    preparationSignature(prepare, definition, tableWireTypes, wholeInput(definition),
                         moduleName(index) + "::");
    prepare << " {\n";
    if (failed(emitTemporalWork(prepare, index)))
      return failure();
    prepare << "}\n";
    if (failed(emitLifecycle(out, definition, moduleName(index),
                             memoizedRules.test(index), header)))
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
  gfir::detail::TableMapRow row(map, result);
  auto yielded = row.getValue();
  auto forwarded = row.tableOperand(yielded);
  if (auto select = yielded.getDefiningOp<ac::BitsSelectOp>()) {
    auto yes = row.tableOperand(select.getTrueValue());
    auto no = row.tableOperand(select.getFalseValue());
    // A four-state mux of the same row preserves all three masks.
    if (yes && no && map.getTables()[*yes] == map.getTables()[*no])
      forwarded = yes;
    if (tableType.getShape().size() == 1 && isa<ac::BitsType>(tableType.getElementType())) {
      auto uniform = [&](const gfir::detail::TableMapRow::Uniform &input) {
        return args[input.operand] + (input.splat ? ".element(0)" : "");
      };
      auto condition = row.uniform(select.getCondition());
      if (!forwarded && yes && no && condition)
        return "pyc_behavior::selectRows(" + uniform(*condition) + ", " + args[*yes] + ", " +
               args[*no] + ")";

      auto replacement = row.uniform(select.getTrueValue());
      auto index = row.matchingIndex(select.getCondition());
      if (no && replacement && index) {
        auto width = analysis.getPackedWidth(body.getArgument(0).getType(), {}, map);
        if (failed(width))
          return failure();
        // A wrapped row ordinal can match more than one row.
        if (std::bit_width(*size - 1) <= *width)
          return "pyc_behavior::updatedTable(" + args[*no] + ", " + uniform(*index) +
                 ", " + uniform(*replacement) + ")";
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

#include "BehavioralTransactions.inc"

#include "BehavioralPreparation.inc"

#include "BehavioralRoot.inc"

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
  if (failed(planWholeOutputs()) || failed(planRecordStates()))
    return failure();
  planEvaluationMemoization();
  if (failed(planWholeInputs()))
    return failure();
  outputGroups.resize(program.definitions.size());
  for (const auto &definition : program.definitions)
    usedOutputs.emplace_back(definition.outputs.size());
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
  if (!recordLayouts.empty())
    parts.supportHeader += behavioralRecordSupport.str();
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
  if (failed(planNativeOutputInlining()) || failed(emitOutputCallables()))
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
