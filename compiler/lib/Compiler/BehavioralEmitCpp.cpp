#include "BehavioralEmitCpp.h"
#include "FinalCppNames.h"
#include "HardwareEmitCommon.h"
#include "pycircuit/Dialect/GFIR/StructuredProgram.h"
#include "llvm/ADT/BitVector.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/Support/JSON.h"
#include "llvm/Support/raw_ostream.h"
#include <limits>

using namespace mlir;

namespace acir::compiler {
namespace {

std::string wireName(unsigned index) { return "value_" + std::to_string(index); }
std::string moduleName(unsigned index) { return "pyc_gfir_module_" + std::to_string(index); }
std::string functionName(unsigned index) { return "output_" + std::to_string(index); }
std::string carrier(unsigned width) { return "gfsim::FourState<" + std::to_string(width) + ">"; }
std::string stateName(unsigned index) { return "pyc_state_" + std::to_string(index); }
std::string initialName(unsigned index) { return "pyc_initial_" + std::to_string(index); }
std::string frameName(unsigned index) { return "pyc_frame_" + std::to_string(index); }
std::string ruleName(unsigned index) { return "pyc_rule_" + std::to_string(index); }
std::string payloadName(unsigned index) { return "pyc_payload_" + std::to_string(index); }
std::string resultName(unsigned index) { return "pyc_result_" + std::to_string(index); }
std::string preparedName(unsigned index) { return "pyc_prepared_" + std::to_string(index); }
std::string availabilityName(unsigned port) { return "pyc_available_" + std::to_string(port); }
std::string memoryPinName(unsigned memory, unsigned port, unsigned argument) {
  return "pyc_memory_" + std::to_string(memory) + "_" + std::to_string(port) + "_" + std::to_string(argument);
}
std::string intentName(unsigned index) { return "pyc_intent_" + std::to_string(index); }

bool isClocked(const gfir::StructuredDefinition &definition) {
  return definition.clocked.has_value() || !definition.clockedCalls.empty();
}

bool needsDomain(const gfir::StructuredClocked &clocked) {
  return !clocked.variables.empty() || llvm::any_of(clocked.rules,
      [](const gfir::StructuredRule &rule) { return rule.sampled; });
}

constexpr StringLiteral levelName = "gfsim::behavior::level";
constexpr StringLiteral noCheck = "std::numeric_limits<unsigned>::max()";

std::string cppString(StringRef value) {
  std::string text;
  llvm::raw_string_ostream out(text);
  out << '"';
  out.write_escaped(value, false);
  out << '"';
  return text;
}

std::string concatenate(ArrayRef<std::string> fields) {
  std::string result = fields.front();
  for (const auto &field : fields.drop_front())
    result = "gfsim::concat(" + result + ", " + field + ")";
  return result;
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

void signature(raw_ostream &out, const gfir::StructuredDefinition &definition,
               unsigned result, StringRef owner = {}) {
  const auto &output = definition.outputs[result];
  out << carrier(definition.wires[output.wire].width) << ' ' << owner
      << functionName(result) << '(';
  ArrayRef<unsigned> inputs = output.requiresPreparation ? ArrayRef<unsigned>{}
                                                         : ArrayRef<unsigned>(output.inputs);
  llvm::interleaveComma(inputs, out, [&](unsigned input) {
    out << "const " << carrier(definition.wires[input].width) << " &"
        << wireName(input);
  });
  out << ") const";
}

void preparationSignature(raw_ostream &out,
                          const gfir::StructuredDefinition &definition,
                          StringRef owner = {}) {
  out << "void " << owner << "prepare(const gfsim::behavior::Level &pyc_clock, "
         "const gfsim::behavior::Level &pyc_reset";
  for (unsigned input : definition.preparationInputs)
    out << ", const " << carrier(definition.wires[input].width) << " &" << wireName(input);
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
                                    StringRef instancePrefix);
  LogicalResult emitWires(raw_ostream &out,
                          const gfir::StructuredDefinition &definition,
                          ArrayRef<unsigned> outputs, StringRef instancePrefix = {});
  LogicalResult declareModule(raw_ostream &out, unsigned index);
  LogicalResult emitModule(raw_ostream &out, unsigned index);
  LogicalResult emitRoot(FinalCppSourceGroup &group);
  FailureOr<FinalCppSourceGroup *> getGroup(gfir::ModuleOp module);
  FailureOr<std::string> typedWire(Type type, Operation *site);
  FailureOr<std::string> graphWire(Value value,
                                   const gfir::StructuredDefinition &definition,
                                   Operation *site,
                                   std::optional<ArrayRef<unsigned>> available = {});
  FailureOr<std::string> valueExpression(Value result, ArrayRef<std::string> args);
  FailureOr<std::string> tableMapExpression(ac::TableMapOp map, unsigned result,
                                           ArrayRef<std::string> args);
  LogicalResult emitInitial(raw_ostream &out, gfir::VarOp variable, unsigned index);
  LogicalResult emitBody(raw_ostream &out, Block &body,
                         const gfir::StructuredClocked &clocked,
                         const gfir::StructuredRule *rule = nullptr,
                         const llvm::DenseMap<Value, std::string> *outer = nullptr,
                         ArrayRef<std::string> stepTargets = {});
  LogicalResult emitLoop(raw_ostream &out, gfir::LoopOp loop,
                         const gfir::StructuredClocked &clocked,
                         const llvm::DenseMap<Value, std::string> &values,
                         ArrayRef<std::string> results);
  LogicalResult emitBranch(raw_ostream &out, gfir::IfOp branch,
                           const gfir::StructuredClocked &clocked,
                           const llvm::DenseMap<Value, std::string> &values,
                           ArrayRef<std::string> results);
  LogicalResult emitBlockedReturn(raw_ostream &out, Block &body, bool ruleResults);
  LogicalResult declareClocked(raw_ostream &out, const gfir::StructuredClocked &clocked);
  FailureOr<std::string> recipient(const gfir::StructuredDefinition &definition,
                                    const gfir::StructuredFifo &fifo,
                                    std::optional<unsigned> rule = {}, bool ownPop = false);
  LogicalResult emitGrant(raw_ostream &out, const gfir::StructuredDefinition &definition,
                          unsigned grant, StringRef instancePrefix);
  LogicalResult emitClockedCall(raw_ostream &out,
                                const gfir::StructuredDefinition &definition,
                                unsigned index, StringRef instancePrefix);
  LogicalResult emitTemporalWork(raw_ostream &out,
                                 const gfir::StructuredDefinition &definition);
  LogicalResult emitPreparation(raw_ostream &out, const gfir::StructuredDefinition &definition,
                                StringRef clock);
  LogicalResult prepareMemory(raw_ostream &out, const gfir::StructuredDefinition &definition,
                              const gfir::StructuredMemory &memory, StringRef clock);
  LogicalResult emitLifecycle(raw_ostream &out, const gfir::StructuredDefinition &definition,
                              StringRef name);

  const gfir::StructuredProgram &program;
  ac::HardwareAnalysis analysis;
  HardwareEmitContext common;
  llvm::StringMap<unsigned> groups;
  FinalCppSourceParts parts;
  unsigned nextBranch = 0;
  unsigned nextLoop = 0;
};

FailureOr<std::string>
BehavioralEmitter::expression(const gfir::StructuredWire &wire,
                             const gfir::StructuredDefinition &definition,
                             StringRef instancePrefix) {
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
      return "gfsim::extract<" + std::to_string(wire.width) + ">(" +
             read + ".data().packed(), " + std::to_string(wire.low) + ")";
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
    return "gfsim::extract<" + std::to_string(wire.width) + ">(" + read +
           ".packed(), " + std::to_string(wire.low) + ")";
  }
  if (wire.state)
    return "gfsim::extract<" + std::to_string(wire.width) + ">(" +
           instancePrefix.str() + stateName(*wire.state) + ".value().packed(), " +
           std::to_string(wire.low) + ")";
  if (wire.ruleResult) {
    auto [rule, result] = *wire.ruleResult;
    return "gfsim::extract<" + std::to_string(wire.width) + ">(" +
           frameName(rule) + "." + resultName(result) + ".packed(), " +
           std::to_string(wire.low) + ")";
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
    return "gfsim::extract<" + std::to_string(wire.width) + ">(" + *value +
           ".packed(), " + std::to_string(wire.low) + ")";
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
  if (isa<ac::BitsSelectOp>(op))
    return call("select");
  if (isa<ac::BitsConcatOp>(op))
    return concatenate(args);
  std::string width = std::to_string(bitWidth);
  if (auto extract = dyn_cast<ac::BitsExtractOp>(op)) {
    auto low = integer(extract.getLow(), analysis, op);
    if (failed(low))
      return failure();
    return "gfsim::extract<" + width + ">(" + args.front() + ", " +
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

LogicalResult BehavioralEmitter::emitWires(
    raw_ostream &out, const gfir::StructuredDefinition &definition,
    ArrayRef<unsigned> outputs, StringRef instancePrefix) {
  llvm::BitVector needed(definition.order.size());
  for (unsigned output : outputs)
    needed.set(output);
  for (unsigned index : llvm::reverse(definition.order))
    if (needed.test(index)) {
      for (unsigned input : definition.dependencies(index))
        needed.set(input);
    }
  for (unsigned index : definition.order) {
    if (!needed.test(index))
      continue;
    if (index >= definition.firstClockedCallNode()) {
      if (failed(emitClockedCall(out, definition,
                                 index - definition.firstClockedCallNode(), instancePrefix)))
        return failure();
      continue;
    }
    if (index >= definition.wires.size()) {
      if (failed(emitGrant(out, definition, index, instancePrefix)))
        return failure();
      continue;
    }
    const auto &wire = definition.wires[index];
    if (isa<BlockArgument>(wire.endpoint.value))
      continue;
    auto value = expression(wire, definition, instancePrefix);
    if (failed(value))
      return failure();
    out << "    const auto " << wireName(index) << " = " << *value << ";\n";
  }
  return success();
}

LogicalResult BehavioralEmitter::declareModule(raw_ostream &out, unsigned index) {
  const auto &definition = program.definitions[index];
  out << "struct " << moduleName(index) << " {\n";
  for (unsigned result = 0; result < definition.outputs.size(); ++result) {
    out << "  ";
    signature(out, definition, result);
    out << ";\n";
  }
  for (const auto &output : definition.outputAvailability)
    out << "  bool " << availabilityName(output.port) << " = false;\n";
  if (isClocked(definition)) {
    out << "  ";
    preparationSignature(out, definition);
    out << ";\n  void reset() noexcept;\n  void discard() noexcept;\n"
           "  void commit() noexcept;\n";
  }
  if (definition.hasChecks())
    out << "  void configure_checks(std::string instance);\n"
           "  bool validate_sampling(gfsim::SimFailureInfo &failure) const noexcept;\n"
           "  bool validate_checks(gfsim::SimFailureInfo &failure) const noexcept;\n";
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
  if (definition.clocked && failed(declareClocked(out, *definition.clocked)))
    return failure();
  if (isClocked(definition)) {
    out << "private:\n";
    if (definition.hasChecks())
      out << "  bool pyc_checks_valid = false;\n"
             "  gfsim::SampledClockFault pyc_sampling_fault = gfsim::SampledClockFault::None;\n"
             "  std::string pyc_check_instance;\n"
          << "  unsigned pyc_failed_rule = " << noCheck << ";\n"
          << "  unsigned pyc_failed_check = " << noCheck << ";\n";
    for (auto [result, output] : llvm::enumerate(definition.outputs)) {
      if (!output.requiresPreparation)
        continue;
      auto type = carrier(definition.wires[output.wire].width);
      out << "  " << type << ' ' << preparedName(result) << " = " << type
          << "::unknown();\n";
    }
  }
  out << "};\n";
  return success();
}

LogicalResult BehavioralEmitter::emitModule(raw_ostream &out, unsigned index) {
  const auto &definition = program.definitions[index];
  for (auto [result, output] : llvm::enumerate(definition.outputs)) {
    signature(out, definition, result, moduleName(index) + "::");
    out << " {\n";
    if (output.requiresPreparation) {
      out << "    return " << preparedName(result) << ";\n}\n";
    } else {
      if (failed(emitWires(out, definition, {output.wire})))
        return failure();
      out << "    return " << wireName(output.wire) << ";\n}\n";
    }
  }
  if (isClocked(definition)) {
    preparationSignature(out, definition, moduleName(index) + "::");
    out << " {\n";
    if (failed(emitTemporalWork(out, definition)))
      return failure();
    out << "}\n";
    if (failed(emitLifecycle(out, definition, moduleName(index))))
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
        for (auto [ordinal, check] : llvm::enumerate(rule.checks)) {
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

  return success();
}

FailureOr<std::string> BehavioralEmitter::typedWire(Type type, Operation *site) {
  auto payload = common.cppType(type, site);
  if (failed(payload))
    return failure();
  return "gfsim::wire<" + *payload + ">";
}

FailureOr<std::string>
BehavioralEmitter::graphWire(Value value,
                             const gfir::StructuredDefinition &definition,
                             Operation *site,
                             std::optional<ArrayRef<unsigned>> available) {
  auto type = typedWire(value.getType(), site);
  auto values = definition.valueWires.find(value);
  if (failed(type) || values == definition.valueWires.end())
    return failure();
  SmallVector<unsigned> ordered(values->second);
  llvm::sort(ordered, [&](unsigned a, unsigned b) {
    return definition.wires[a].low > definition.wires[b].low;
  });
  SmallVector<std::string> fields;
  for (unsigned at : ordered)
    fields.push_back(!available || llvm::is_contained(*available, at)
                        ? wireName(at)
                        : carrier(definition.wires[at].width) + "::unknown()");
  return *type + "::fromPacked(" + concatenate(fields) + ")";
}

FailureOr<std::string>
BehavioralEmitter::valueExpression(Value result, ArrayRef<std::string> args) {
  auto *op = result.getDefiningOp();
  auto type = result.getType();
  if (isa<ac::TableSplatOp>(op)) {
    auto size = analysis.getTableSize(cast<ac::TableType>(type), {}, op);
    if (failed(size))
      return failure();
    return "gfsim::behavior::splat<" + std::to_string(*size) + ">(" + args.front() + ")";
  }
  if (auto map = dyn_cast<ac::TableMapOp>(op))
    return tableMapExpression(map, cast<OpResult>(result).getResultNumber(), args);
  if (isa<ac::TableGetOp>(op)) {
    unsigned ordinal = cast<OpResult>(result).getResultNumber();
    if (ordinal == 0)
      return "std::get<0>(gfsim::behavior::lookup(" + join(args, ", ") + "))";
    auto size = analysis.getTableSize(cast<ac::TableType>(op->getOperand(0).getType()), {}, op);
    if (failed(size))
      return failure();
    return "gfsim::behavior::Level::fromPacked(gfsim::behavior::inRange<" +
           std::to_string(*size) + ">(" + args[1] + "))";
  }
  auto wire = typedWire(type, op);
  auto width = analysis.getPackedWidth(type, {}, op);
  if (failed(wire) || failed(width) || *width > std::numeric_limits<unsigned>::max())
    return failure();
  SmallVector<std::string> packed;
  for (const auto &arg : args)
    packed.push_back(arg + ".packed()");
  std::string prefix = *wire + "::fromPacked(";
  if (isa<ac::StructCreateOp>(op))
    return prefix + concatenate(packed) + ")";
  if (auto get = dyn_cast<ac::StructGetOp>(op)) {
    auto leaves = analysis.getLeaves(get.getValue().getType(), {}, op);
    if (failed(leaves))
      return failure();
    auto field = llvm::find_if(llvm::reverse(*leaves), [&](const ac::PackedLeaf &leaf) {
      return !leaf.path.empty() && leaf.path.front().getValue() == get.getField();
    });
    if (field == leaves->rend())
      return get.emitOpError("behavioral field has no packed layout");
    return prefix + "gfsim::extract<" + std::to_string(*width) + ">(" +
           packed.front() + ", " + std::to_string(field->low) + "))";
  }
  auto computed = bitExpression(op, *width, packed);
  if (failed(computed))
    return failure();
  return prefix + *computed + ")";
}

FailureOr<std::string> BehavioralEmitter::tableMapExpression(
    ac::TableMapOp map, unsigned result, ArrayRef<std::string> args) {
  auto type = typedWire(map.getResult(result).getType(), map);
  auto size = analysis.getTableSize(cast<ac::TableType>(map.getResult(result).getType()), {}, map);
  if (failed(type) || failed(size))
    return failure();
  auto &body = map.getBody().front();
  auto yielded = cast<ac::YieldOp>(body.getTerminator()).getValues()[result];
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
      out << "    const auto pyc_map_input_" << index << " = " << args[index] << ";\n";
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
                                          const gfir::StructuredClocked &clocked,
                                          const gfir::StructuredRule *rule,
                                          const llvm::DenseMap<Value, std::string> *outer,
                                          ArrayRef<std::string> stepTargets) {
  bool ruleResults = rule != nullptr;
  llvm::DenseMap<Value, std::string> values = outer ? *outer : llvm::DenseMap<Value, std::string>{};
  for (Value arg : body.getArguments())
    values.try_emplace(arg, wireName(values.size()));
  auto snapshot = [&] {
    auto yield = cast<gfir::YieldOp>(body.getTerminator());
    for (auto [index, value] : llvm::enumerate(yield.getValues()))
      out << "    txn." << resultName(index) << " = " << values.lookup(value) << ";\n";
  };
  for (Operation &op : body) {
    if (rule && rule->afterSnapshot == &op) {
      snapshot();
      out << "    if (!pyc_active) return;\n";
    }
    SmallVector<std::string> args, results;
    for (Value input : op.getOperands())
      args.push_back(values.lookup(input));
    for (Value result : op.getResults()) {
      results.push_back(wireName(values.size()));
      values.try_emplace(result, results.back());
    }
    std::string text;
    if (auto loop = dyn_cast<gfir::LoopOp>(op)) {
      if (failed(emitLoop(out, loop, clocked, values, results)))
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
      auto predicate = values.lookup(guard.getPredicate());
      out << "    if (!gfsim::behavior::isOne(" << predicate << ")) {\n"
          << "      if (txn.pyc_check_active && !" << predicate << ".isFullyKnown()) "
          << "txn.pyc_failed_check = pyc_check_base + " << clocked.checkOffsets.lookup(guard) << ";\n"
             "      txn.pyc_running = false;\n      ";
      if (failed(emitBlockedReturn(out, body, ruleResults)))
        return failure();
      out << "    }\n";
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
      if (failed(emitBranch(out, branch, clocked, values, results)))
        return failure();
      if (clocked.checkedBranches.contains(branch)) {
        out << "    if (!txn.pyc_running) { ";
        if (failed(emitBlockedReturn(out, body, ruleResults)))
          return failure();
        out << "    }\n";
      }
      continue;
    } else if (auto call = dyn_cast<gfir::CallOp>(op)) {
      auto memoryUse = clocked.transactionMemoryCalls.find(call);
      if (memoryUse != clocked.transactionMemoryCalls.end()) {
        const auto &use = memoryUse->second;
        const auto &memory = clocked.memories[use.memory];
        auto name = instanceName(memory.instance);
        if (failed(name))
          return failure();
        if (use.access == gfir::StructuredMemoryAccess::Result) {
          out << "    if (!gfsim::behavior::isOne(" << *name << ".live(" << use.port
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
          const auto &fifo = clocked.fifos[use->second.fifo];
          auto name = instanceName(fifo.instance);
          if (failed(name))
            return failure();
          // The caller has already admitted this Rule using all requirements.
          // The call reads old Q or records the payload in the caller's frame.
          if (use->second.access == gfir::StructuredFifoAccess::In)
            text = "txn." + payloadName(use->second.fifo) + " = " + args.front();
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
    } else if (auto write = dyn_cast<gfir::WriteOp>(op)) {
      unsigned index = clocked.stateAccesses.lookup(write);
      auto state = stateName(index);
      std::string frame = "txn." + state;
      if (clocked.conditionalWrites.contains(index))
        out << "    txn." << intentName(index) << " = " << levelName << "(true);\n";
      if (auto path = clocked.writePaths.find(write); path != clocked.writePaths.end()) {
        out << "    {\n      if (!" << frame << ") " << frame << ".emplace(" << state
            << ".value());\n";
        std::string replacement = values.lookup(write.getValue()) + ".packed()";
        if (write.getIndex()) {
          auto type = typedWire(path->second.types.back(), write);
          if (failed(type))
            return failure();
          out << "      auto pyc_field = " << *type << "::fromPacked(gfsim::extract<"
              << path->second.width << ">(" << frame << "->packed(), "
              << path->second.low << "));\n"
              << "      gfsim::behavior::replace(pyc_field, "
              << values.lookup(write.getIndex()) << ", "
              << values.lookup(write.getValue()) << ");\n";
          replacement = "pyc_field.packed()";
        }
        out << "      " << frame << "->assignSlice(" << path->second.low << ", "
            << replacement << ");\n    }\n";
        continue;
      }
      if (write.getIndex()) {
        out << "    if (!" << frame << ") " << frame << ".emplace(" << state
            << ".value());\n";
        text = "gfsim::behavior::replace(*" + frame + ", " +
               values.lookup(write.getIndex()) + ", " + values.lookup(write.getValue()) + ")";
      } else {
        text = frame + " = " + values.lookup(write.getValue());
      }
    } else if (isa<gfir::YieldOp>(op)) {
      if (ruleResults) {
        if (!rule->afterSnapshot)
          snapshot();
      } else if (args.size() == 1)
        out << "    return " << args.front() << ";\n";
      else if (!args.empty())
        out << "    return std::tuple{" << join(args, ", ") << "};\n";
      continue;
    } else if (isa<ac::TableGetOp>(op)) {
      text = "gfsim::behavior::lookup(" + join(args, ", ") + ")";
    } else {
      auto value = valueExpression(op.getResult(0), args);
      if (failed(value))
        return failure();
      text = *value;
    }
    out << "    ";
    if (results.size() == 1)
      out << "const auto " << results.front() << " = ";
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
    raw_ostream &out, gfir::LoopOp loop, const gfir::StructuredClocked &clocked,
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
    if (failed(emitBody(out, body, clocked, nullptr, &iteration,
                        results.drop_back())))
      return failure();
    out << "    }\n";
  }
  out << "    const auto " << results.back() << " = " << levelName << "(true);\n";
  return success();
}

LogicalResult BehavioralEmitter::emitBranch(
    raw_ostream &out, gfir::IfOp branch, const gfir::StructuredClocked &clocked,
    const llvm::DenseMap<Value, std::string> &values,
    ArrayRef<std::string> results) {
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
    if (failed(emitBody(out, arm->front(), clocked, nullptr, &values)))
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
      return "(" + slot + " ? *" + slot + " : " + stateName(state) + ".value()).packed()";
    };
    out << "      txn." << stateName(state) << " = " << *type
        << "::fromPacked(gfsim::select(" << predicate << ".packed(), "
        << candidate(thenFrame) << ", " << candidate(elseFrame) << "));\n"
        << "      txn." << intentName(state)
        << " = gfsim::behavior::Level::fromPacked(gfsim::select(" << predicate
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
    out << "      " << results[index] << " = " << *type
        << "::fromPacked(gfsim::select(" << predicate << ".packed(), "
        << returned(thenValue) << ".packed(), " << returned(elseValue) << ".packed()));\n";
  }
  out << "    }\n";
  return success();
}

LogicalResult
BehavioralEmitter::declareClocked(raw_ostream &out,
                                 const gfir::StructuredClocked &clocked) {
  if (needsDomain(clocked))
    out << "  gfsim::behavior::SampledDomain pyc_domain;\n";
  for (const auto &fifo : clocked.fifos) {
    auto name = instanceName(fifo.instance);
    auto type = common.cppType(fifo.element, fifo.instance);
    if (failed(name) || failed(type))
      return failure();
    out << "  gfsim::behavior::Fifo<" << *type << ", " << fifo.depth
        << ", gfsim::QueueReadyPolicy::"
        << (fifo.downstreamPop ? "DownstreamPop" : "LocalOccupancy") << ", "
        << fifo.latency << "> " << *name << ";\n";
  }
  for (const auto &memory : clocked.memories) {
    auto name = instanceName(memory.instance);
    auto type = common.cppType(memory.element, memory.instance);
    if (failed(name) || failed(type))
      return failure();
    out << "  gfsim::behavior::" << (memory.asynchronous() ? "ByteMemory" : "SyncMemory")
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
    out << "  gfsim::behavior::State<" << *type << "> " << stateName(index) << ";\n";
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
    if (failed(emitBody(out, method.getBody().front(), clocked)))
      return failure();
    out << "  }\n";
  }
  for (auto [index, rule] : llvm::enumerate(clocked.rules)) {
    auto name = "TxnFrame" + std::to_string(index);
    out << "  struct " << name << " {\n";
    if (rule.sampled)
      out << "    bool pyc_running = true;\n"
          << "    unsigned pyc_failed_check = " << noCheck << ";\n"
          << "    bool pyc_check_active = false;\n";
    for (unsigned state : rule.states) {
      auto variable = clocked.variables[state];
      auto wire = typedWire(variable.getInitial().getType(), variable);
      if (failed(wire))
        return failure();
      out << "    std::optional<" << *wire << "> " << stateName(state) << ";\n";
      if (clocked.conditionalWrites.contains(state))
        out << "    gfsim::behavior::Level " << intentName(state)
            << " = " << levelName << "(false);\n";
    }
    for (auto [fifoId, fifo] : llvm::enumerate(clocked.fifos)) {
      if (fifo.producer != index)
        continue;
      auto type = typedWire(fifo.element, fifo.instance);
      if (failed(type))
        return failure();
      out << "    " << *type << " " << payloadName(fifoId) << ";\n";
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
      if (failed(declare(memory.writer, 2)))
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
    if (failed(emitBody(out, operation.getBody().front(), clocked, &rule)))
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
    return "gfsim::behavior::Level::fromPacked(" + wireName(wires->second.front()) + ")";
  }
  if (fifo.consumer && fifo.consumer != rule)
    return wireName(definition.wires.size() + *fifo.consumer);
  return levelName.str() + (ownPop ? "(true)" : "(false)");
}

LogicalResult BehavioralEmitter::emitGrant(raw_ostream &out,
                                            const gfir::StructuredDefinition &definition,
                                            unsigned grant, StringRef instancePrefix) {
  unsigned index = grant - definition.wires.size();
  const auto &rule = definition.clocked->rules[index];
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
    grantExpr = "gfsim::behavior::Level::fromPacked(" + combined + ")";
  }
  out << "    const auto " << wireName(grant) << " = " << grantExpr << ";\n";
  out << "    TxnFrame" << index << ' ' << frameName(index) << ";\n";
  if (rule.sampled)
    out << "    " << frameName(index) << ".pyc_check_active = pyc_active;\n";
  out << "    if (";
  if (needsDomain(*definition.clocked) && rule.resultUses.empty())
    out << "pyc_active && ";
  out << "gfsim::behavior::isOne(" << wireName(grant) << ")) "
      << instancePrefix << ruleName(index) << '(' << frameName(index);
  if (rule.afterSnapshot)
    out << ", pyc_active";
  auto operation = rule.operation;
  for (Value capture : operation.getCaptures()) {
    auto value = graphWire(capture, definition, operation);
    if (failed(value))
      return failure();
    out << ", " << *value;
  }
  out << ");\n";
  if (rule.sampled)
    out << "    if (" << frameName(index) << ".pyc_failed_check != " << noCheck
        << " && " << index << " < pyc_failed_rule) {\n"
        << "      pyc_failed_rule = " << index << ";\n"
        << "      pyc_failed_check = " << frameName(index) << ".pyc_failed_check;\n"
        << "    }\n";
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
  out << "    " << instancePrefix << *member << ".prepare(pyc_clock, pyc_reset";
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
  out << ");\n";
  return success();
}

LogicalResult BehavioralEmitter::emitTemporalWork(
    raw_ostream &out, const gfir::StructuredDefinition &definition) {
  SmallVector<unsigned> needed;
  if (definition.hasChecks())
    out << "    pyc_checks_valid = false;\n"
        << "    pyc_failed_rule = " << noCheck << ";\n"
        << "    pyc_failed_check = " << noCheck << ";\n"
        << "    pyc_sampling_fault = gfsim::SampledClockFault::None;\n";
  for (auto [index, output] : llvm::enumerate(definition.outputs))
    if (output.requiresPreparation)
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
  if (failed(emitWires(out, definition, needed)))
    return failure();
  if (definition.clocked && failed(emitPreparation(out, definition, "pyc_clock")))
    return failure();
  for (auto [index, output] : llvm::enumerate(definition.outputs))
    if (output.requiresPreparation)
      out << "    " << preparedName(index) << " = " << wireName(output.wire) << ";\n";
  for (const auto &output : definition.outputAvailability) {
    out << "    " << availabilityName(output.port) << " = ";
    llvm::interleave(output.rules, out, [&](unsigned rule) {
      out << frameName(rule) << ".pyc_running";
    }, " && ");
    out << ";\n";
  }
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
        enabled = "gfsim::behavior::Level::fromPacked(gfsim::bit_and(" + enabled +
                  ".packed(), " + levelName.str() + "(" + frameName(writer->second) + ".pyc_running).packed()))";
      if (plan.conditionalWrites.contains(index))
        enabled = "gfsim::behavior::Level::fromPacked(gfsim::bit_and(" + enabled +
                  ".packed(), " + frameName(writer->second) + '.' + intentName(index) + ".packed()))";
      out << ".prepare(" << enabled
          << ", " << frameName(writer->second) << '.' << stateName(index) << ");\n";
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
    raw_ostream &out, const gfir::StructuredDefinition &definition,
    StringRef name) {
  for (StringRef action : {"reset", "discard", "commit"}) {
    out << "void " << name << "::" << action << "() noexcept {\n";
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

LogicalResult BehavioralEmitter::emitRoot(FinalCppSourceGroup &group) {
  const auto &definition = program.definitions[program.rootIndex];
  auto root = definition.module;
  auto entry = program.root;
  auto policy = entry.getExecutionPolicyAttr();
  const bool resetIfOne = policy && policy.getAs<StringAttr>("reset").getValue() == "if_one";
  const bool system = entry.getKind() == "system";
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
         << "(std::string name, gfsim::WorkExecutor* = nullptr)\n"
         << "      : gfsim::SimModule(std::move(name)) {\n";
  if (definition.hasChecks())
    header << "    pyc_body.configure_checks(std::string(this->name()));\n";
  header << "  }\n"
         << "  bool HasWork() const noexcept override { return true; }\n"
         << "  void Work() override;\n";
  if (definition.hasChecks())
    header << "  bool pyc_validate_checks(gfsim::SimFailureInfo &failure) const noexcept;\n";
  else
    header << "  bool pyc_validate_checks(gfsim::SimFailureInfo &) const noexcept "
              "{ return true; }\n";
  if (isClocked(definition))
    header << "  void Reset() noexcept override;\n"
           << "  void DiscardNext() noexcept override;\n"
           << "  void Xfer() noexcept override;\n";
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
    header << "  gfsim::behavior::Level " << *wire << ";\n";
    (*outputRows[output.port].getAsObject())["availability"] = *wire;
    controls.emplace_back(llvm::json::Object{{"direction", "output"},
                                             {"role", "availability"},
                                             {"wire", *wire}, {"width", 1}});
    outputs.push_back(*wire);
    companions.emplace_back(output.port, std::move(*wire));
  }
  llvm::json::Object domains{{"clock", nullptr}, {"reset", nullptr}};
  std::string clock, reset;
  if (isClocked(definition)) {
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
      header << "  gfsim::behavior::Level " << *pin << ";\n";
      (StringRef(role) == "clock" ? clock : reset) = *pin;
    }
  }

  header << "private:\n  " << moduleName(program.rootIndex) << " pyc_body;\n";
  if (system)
    header << "  gfsim::SystemSampledClock pyc_system_clock;\n";
  header << "};\n";
  for (size_t i = 1; i < components.size(); ++i)
    header << "}\n";

  llvm::raw_string_ostream source(group.source);
  StringRef scopedName = StringRef(*name).drop_front(2);
  source << "void " << scopedName << "::Work() {\n";
  if (isClocked(definition)) {
    source << "  try {\n"
           << "    const auto pyc_reset = gfsim::interpretReset(" << reset
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
    source << "    const auto " << wireName(index) << " = gfsim::extract<"
           << wire.width << ">(" << inputs[arg] << ".packed(), " << wire.low << ");\n";
  }
  if (isClocked(definition)) {
    source << "    pyc_body.prepare(" << clock << ", pyc_reset";
    for (unsigned input : definition.preparationInputs)
      source << ", " << wireName(input);
    source << ");\n";
  }
  if (system)
    source << "    pyc_system_clock.prepare(" << clock << ", " << reset
           << ", gfsim::ResetSemantics::" << (resetIfOne ? "IfOne" : "Strict") << ");\n";

  for (auto [index, output] : llvm::enumerate(definition.outputs)) {
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
    if (isa<ac::TableType>(type.getResult(output.port.port)))
      source << "    " << outputs[output.port.port] << " = decltype("
             << outputs[output.port.port] << ")::fromPacked(pyc_body." << functionName(index)
             << '(' << join(actuals, ", ") << "));\n";
    else
      source << "    " << outputs[output.port.port] << ".assignSlice(" << leaf->low
             << ", pyc_body." << functionName(index) << '(' << join(actuals, ", ") << "));\n";
  }
  for (const auto &[port, wire] : companions)
    source << "    " << wire << " = " << levelName << "(pyc_body."
           << availabilityName(port) << ");\n";
  if (isClocked(definition))
    source << "  } catch (...) {\n    DiscardNext();\n    throw;\n  }\n";
  source << "}\n";
  if (isClocked(definition)) {
    for (auto [hook, action] : {std::pair{"Reset", "reset"},
                              {"DiscardNext", "discard"}, {"Xfer", "commit"}}) {
      source << "void " << scopedName << "::" << hook << "() noexcept {\n"
             << "  pyc_body." << action << "();\n";
      if (system)
        source << "  pyc_system_clock." << action << "();\n";
      source << "}\n";
    }
  }

  if (definition.hasChecks())
    source << "bool " << scopedName
           << "::pyc_validate_checks(gfsim::SimFailureInfo &failure) const noexcept {\n"
              "  failure = {};\n"
              "  return pyc_body.validate_sampling(failure) && pyc_body.validate_checks(failure);\n}\n";

  parts.systemHeader = "#pragma once\n#include \"" + parts.rootHeaderPath +
                       "\"\nusing pyc_root = " + *name + ";\n";
  if (system) {
    parts.systemHeader += "#include <gfsim/SystemRunner.h>\n"
                          "inline std::span<const gfsim::RunnerObservation> "
                          "pyc_observation_metadata() { return {}; }\n";
    addCppSystemDriver(parts, clock, reset, program.samples, resetIfOne);
  }
  appendCppDutHeader(parts, inputs, outputs, "observations_.Configure({}, 0)",
                     /*hasChecks=*/definition.hasChecks(), /*observes=*/false,
                     /*alwaysActiveRoot=*/true);
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
  }
  parts.supportHeader = "#pragma once\n#include <gfsim/wire.h>\n"
                        "#include <gfsim/SimModule.h>\n#include <gfsim/WorkExecutor.h>\n";
  if (llvm::any_of(program.definitions, [](const gfir::StructuredDefinition &definition) {
        return definition.hasChecks();
      }))
    parts.supportHeader += "#include <gfsim/SimSystem.h>\n#include <limits>\n#include <string>\n";
  else
    parts.supportHeader += "namespace gfsim { struct SimFailureInfo; }\n";
  if (llvm::any_of(program.definitions, [](const gfir::StructuredDefinition &definition) {
        return isClocked(definition) || llvm::any_of(definition.wires,
            [](const gfir::StructuredWire &wire) {
              return isa_and_nonnull<ac::TableSplatOp, ac::TableGetOp, ac::TableMapOp>(
                  wire.endpoint.value.getDefiningOp());
            });
      }))
    parts.supportHeader += "#include <gfsim/behavior.h>\n#include <optional>\n";
  llvm::raw_string_ostream support(parts.supportHeader);
  if (failed(emitCppRecords(common, support)))
    return failure();
  for (auto [index, definition] : llvm::enumerate(program.definitions)) {
    if (failed(declareModule(support, index)))
      return failure();
    auto group = getGroup(definition.module);
    if (failed(group))
      return failure();
    llvm::raw_string_ostream source((*group)->source);
    if (failed(emitModule(source, index)))
      return failure();
  }
  auto rootGroup = getGroup(program.definitions[program.rootIndex].module);
  if (failed(rootGroup) || failed(emitRoot(**rootGroup)))
    return failure();
  return std::move(parts);
}

} // namespace

FailureOr<FinalCppSourceParts>
emitBehavioralCpp(ModuleOp package, const gfir::StructuredProgram &program) {
  return BehavioralEmitter(package, program).emit();
}

} // namespace acir::compiler
