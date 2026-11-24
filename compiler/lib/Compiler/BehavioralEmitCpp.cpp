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

bool isClocked(const gfir::StructuredDefinition &definition) {
  return definition.clocked.has_value() || !definition.clockedCalls.empty();
}

constexpr StringLiteral levelName = "gfsim::behavior::level";

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
                                   Operation *site);
  FailureOr<std::string> valueExpression(Value result, ArrayRef<std::string> args);
  LogicalResult emitInitial(raw_ostream &out, gfir::VarOp variable, unsigned index);
  LogicalResult emitBody(raw_ostream &out, Block &body,
                         const gfir::StructuredClocked &clocked,
                         bool ruleResults = false);
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
  LogicalResult emitLifecycle(raw_ostream &out, const gfir::StructuredDefinition &definition,
                              StringRef name);

  const gfir::StructuredProgram &program;
  ac::HardwareAnalysis analysis;
  HardwareEmitContext common;
  llvm::StringMap<unsigned> groups;
  FinalCppSourceParts parts;
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
  if (isClocked(definition)) {
    out << "  ";
    preparationSignature(out, definition);
    out << ";\n  void reset() noexcept;\n  void discard() noexcept;\n"
           "  void commit() noexcept;\n";
  }
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
                             Operation *site) {
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
    fields.push_back(wireName(at));
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

LogicalResult BehavioralEmitter::emitBody(raw_ostream &out, Block &body,
                                          const gfir::StructuredClocked &clocked,
                                          bool ruleResults) {
  llvm::DenseMap<Value, std::string> values;
  for (Value arg : body.getArguments())
    values.try_emplace(arg, wireName(values.size()));
  for (Operation &op : body) {
    SmallVector<std::string> args, results;
    for (Value input : op.getOperands())
      args.push_back(values.lookup(input));
    for (Value result : op.getResults()) {
      results.push_back(wireName(values.size()));
      values.try_emplace(result, results.back());
    }
    std::string text;
    if (auto call = dyn_cast<gfir::CallOp>(op)) {
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
        args.insert(args.begin(), "txn");
        text = *callee + "(" + join(args, ", ") + ")";
      }
    } else if (auto read = dyn_cast<gfir::ReadOp>(op)) {
      auto state = stateName(clocked.stateAccesses.lookup(read));
      text = "txn." + state + " ? *txn." + state + " : " + state + ".value()";
    } else if (auto write = dyn_cast<gfir::WriteOp>(op)) {
      auto state = stateName(clocked.stateAccesses.lookup(write));
      std::string frame = "txn." + state;
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
        for (auto [index, arg] : llvm::enumerate(args))
          out << "    txn." << resultName(index) << " = " << arg << ";\n";
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
  }
  return success();
}

LogicalResult
BehavioralEmitter::declareClocked(raw_ostream &out,
                                 const gfir::StructuredClocked &clocked) {
  if (!clocked.variables.empty())
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
    out << "  template <class Frame> auto " << *name << "(Frame &txn";
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
    for (unsigned state : rule.states) {
      auto variable = clocked.variables[state];
      auto wire = typedWire(variable.getInitial().getType(), variable);
      if (failed(wire))
        return failure();
      out << "    std::optional<" << *wire << "> " << stateName(state) << ";\n";
    }
    for (auto [fifoId, fifo] : llvm::enumerate(clocked.fifos)) {
      if (fifo.producer != index)
        continue;
      auto type = typedWire(fifo.element, fifo.instance);
      if (failed(type))
        return failure();
      out << "    " << *type << " " << payloadName(fifoId) << ";\n";
    }
    auto operation = rule.operation;
    for (auto [result, type] : llvm::enumerate(operation.getFunctionType().getResults())) {
      auto wire = typedWire(type, operation);
      if (failed(wire))
        return failure();
      out << "    " << *wire << ' ' << resultName(result) << ";\n";
    }
    out << "  };\n  void " << ruleName(index) << "(" << name << " &txn";
    for (auto [input, type] : llvm::enumerate(operation.getFunctionType().getInputs())) {
      auto wire = typedWire(type, operation);
      if (failed(wire))
        return failure();
      out << ", const " << *wire << " &" << wireName(input);
    }
    out << ") {\n";
    if (failed(emitBody(out, operation.getBody().front(), clocked, /*ruleResults=*/true)))
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
  out << "    TxnFrame" << index << ' ' << frameName(index) << ";\n"
      << "    if (";
  if (!definition.clocked->variables.empty() && rule.resultUses.empty())
    out << "pyc_active && ";
  out << "gfsim::behavior::isOne(" << wireName(grant) << ")) "
      << instancePrefix << ruleName(index) << '(' << frameName(index);
  auto operation = rule.operation;
  for (Value capture : operation.getCaptures()) {
    auto value = graphWire(capture, definition, operation);
    if (failed(value))
      return failure();
    out << ", " << *value;
  }
  out << ");\n";
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
  for (auto [index, output] : llvm::enumerate(definition.outputs))
    if (output.requiresPreparation)
      needed.push_back(output.wire);
  if (definition.clocked) {
    const auto &plan = *definition.clocked;
    if (!plan.variables.empty()) {
      out << "    const auto pyc_sample = pyc_domain.evaluate(pyc_clock, pyc_reset);\n";
      if (llvm::any_of(plan.rules, [](const gfir::StructuredRule &rule) {
            return rule.resultUses.empty();
          }))
        out << "    const bool pyc_active = pyc_sample.rising && !pyc_sample.resetActive && "
               "pyc_sample.fault == gfsim::SampledClockFault::None;\n";
    }
    for (unsigned index = 0; index < plan.rules.size(); ++index)
      needed.push_back(definition.wires.size() + index);
    for (const auto &fifo : plan.fifos)
      for (auto call : {fifo.rawInput, fifo.rawOutput})
        if (call)
          for (Value input : call.getInputs())
            needed.append(definition.valueWires.find(input)->second);
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
  if (!hasState)
    return success();

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
      out << ".prepare(" << wireName(definition.wires.size() + writer->second)
          << ", " << frameName(writer->second) << '.' << stateName(index) << ");\n";
    } else {
      out << ".hold();\n";
    }
  }
  out << "      }\n    }\n";
  return success();
}

LogicalResult BehavioralEmitter::emitLifecycle(
    raw_ostream &out, const gfir::StructuredDefinition &definition,
    StringRef name) {
  for (StringRef action : {"reset", "discard", "commit"}) {
    out << "void " << name << "::" << action << "() noexcept {\n";
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
      if (!plan.variables.empty()) {
        if (action == "commit") {
          out << "  if (pyc_domain.commit()) {\n";
          for (unsigned index = 0; index < plan.variables.size(); ++index)
            out << "    " << stateName(index) << ".commit();\n";
          out << "  }\n";
        } else {
          out << "  pyc_domain." << action << "();\n";
          if (action == "reset")
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
  auto name = qualifiedName(root.getSymName(), root);
  if (failed(name))
    return failure();
  parts.rootCppName = *name;
  parts.rootHeaderPath = group.headerPath;
  SmallVector<StringRef> components;
  StringRef(*name).drop_front(2).split(components, "::");
  llvm::raw_string_ostream header(group.header);
  for (auto component : ArrayRef<StringRef>(components).drop_back())
    header << "namespace " << component << " {\n";
  auto className = components.back();
  header << "class " << className << " final : public gfsim::SimModule {\n"
         << "public:\n  explicit " << className
         << "(std::string name, gfsim::WorkExecutor* = nullptr)\n"
         << "      : gfsim::SimModule(std::move(name)) {}\n"
         << "  bool HasWork() const noexcept override { return true; }\n"
         << "  void Work() override;\n";
  if (isClocked(definition))
    header << "  void Reset() noexcept override;\n"
           << "  void DiscardNext() noexcept override;\n"
           << "  void Xfer() noexcept override;\n";
  SmallVector<std::string> inputs, outputs;
  llvm::json::Array inputRows, outputRows;
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
  header << "private:\n  " << moduleName(program.rootIndex) << " pyc_body;\n};\n";
  for (size_t i = 1; i < components.size(); ++i)
    header << "}\n";

  llvm::raw_string_ostream source(group.source);
  StringRef scopedName = StringRef(*name).drop_front(2);
  source << "void " << scopedName << "::Work() {\n";
  if (isClocked(definition)) {
    auto policy = program.root->getAttrOfType<DictionaryAttr>("execution_policy");
    bool ifOne = policy && policy.getAs<StringAttr>("reset").getValue() == "if_one";
    source << "  try {\n"
           << "    const auto pyc_reset = gfsim::interpretReset(" << reset
           << ", gfsim::ResetSemantics::" << (ifOne ? "IfOne" : "Strict") << ");\n";
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
    source << "    " << outputs[output.port.port] << ".assignSlice(" << leaf->low
           << ", pyc_body." << functionName(index) << '(' << join(actuals, ", ") << "));\n";
  }
  if (isClocked(definition))
    source << "  } catch (...) {\n    DiscardNext();\n    throw;\n  }\n";
  source << "}\n";
  if (isClocked(definition))
    for (auto [hook, action] : {std::pair{"Reset", "reset"},
                              {"DiscardNext", "discard"}, {"Xfer", "commit"}})
      source << "void " << scopedName << "::" << hook << "() noexcept {\n"
             << "  pyc_body." << action << "();\n}\n";

  parts.systemHeader = "#pragma once\n#include \"" + parts.rootHeaderPath +
                       "\"\nusing pyc_root = " + *name + ";\n";
  appendCppDutHeader(parts, inputs, outputs, "observations_.Configure({}, 0)",
                     /*hasChecks=*/false, /*observes=*/false,
                     /*alwaysActiveRoot=*/true);
  llvm::raw_string_ostream map(parts.portMap);
  map << llvm::json::Value(llvm::json::Object{
      {"schema_version", 1}, {"target", "cpp"}, {"inputs", std::move(inputRows)},
      {"outputs", std::move(outputRows)}, {"controls", llvm::json::Array{}},
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
  for (const auto &definition : program.definitions) {
    auto module = definition.module;
    auto name = qualifiedName(module.getSymName(), module);
    if (failed(name) || failed(names.insert(*name, module)))
      return failure();
  }
  parts.supportHeader = "#pragma once\n#include <gfsim/wire.h>\n"
                        "#include <gfsim/SimModule.h>\n#include <gfsim/WorkExecutor.h>\n";
  if (llvm::any_of(program.definitions, isClocked))
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
