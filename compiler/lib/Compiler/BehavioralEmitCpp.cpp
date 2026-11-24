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

using namespace mlir;

namespace acir::compiler {
namespace {

std::string wireName(unsigned index) { return "value_" + std::to_string(index); }
std::string moduleName(unsigned index) { return "pyc_gfir_module_" + std::to_string(index); }
std::string functionName(unsigned index) { return "output_" + std::to_string(index); }
std::string carrier(unsigned width) { return "gfsim::FourState<" + std::to_string(width) + ">"; }

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
  llvm::interleaveComma(output.inputs, out, [&](unsigned input) {
    out << "const " << carrier(definition.wires[input].width) << " &"
        << wireName(input);
  });
  out << ") const";
}

class BehavioralEmitter {
public:
  BehavioralEmitter(ModuleOp package, const gfir::StructuredProgram &program)
      : program(program), analysis(package), common(package, analysis) {}
  FailureOr<FinalCppSourceParts> emit();

private:
  FailureOr<std::string> expression(const gfir::StructuredWire &wire,
                                    StringRef instancePrefix);
  LogicalResult emitWires(raw_ostream &out,
                          const gfir::StructuredDefinition &definition,
                          ArrayRef<unsigned> outputs, StringRef instancePrefix = {});
  LogicalResult declareModule(raw_ostream &out, unsigned index);
  LogicalResult emitModule(raw_ostream &out, unsigned index);
  LogicalResult emitRoot(FinalCppSourceGroup &group);
  FailureOr<FinalCppSourceGroup *> getGroup(gfir::ModuleOp module);

  const gfir::StructuredProgram &program;
  ac::HardwareAnalysis analysis;
  HardwareEmitContext common;
  llvm::StringMap<unsigned> groups;
  FinalCppSourceParts parts;
};

FailureOr<std::string>
BehavioralEmitter::expression(const gfir::StructuredWire &wire,
                             StringRef instancePrefix) {
  auto *op = wire.endpoint.value.getDefiningOp();
  SmallVector<std::string> args;
  for (unsigned dep : wire.dependencies)
    args.push_back(wireName(dep));
  if (wire.call) {
    auto member = instanceName(wire.call->receiver);
    if (failed(member))
      return failure();
    return instancePrefix.str() + *member + "." + functionName(wire.call->output) +
           "(" + join(args, ", ") + ")";
  }
  if (isa<ac::StructCreateOp, ac::StructGetOp>(op))
    return args.front();
  if (auto constant = dyn_cast<ac::BitsConstant4Op>(op))
    return constant4(constant.getPattern());
  if (auto constant = dyn_cast<ac::BitsConstantOp>(op)) {
    auto number = integer(constant.getValue(), analysis, op);
    if (failed(number))
      return failure();
    return carrier(wire.width) + "::known(" +
           bitWords(number->zextOrTrunc(wire.width)) + ")";
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
  if (isa<ac::BitsConcatOp>(op)) {
    std::string result = args.front();
    for (const auto &arg : ArrayRef<std::string>(args).drop_front())
      result = "gfsim::concat(" + result + ", " + arg + ")";
    return result;
  }
  std::string width = std::to_string(wire.width);
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
  llvm::BitVector needed(definition.wires.size());
  for (unsigned output : outputs)
    needed.set(output);
  for (unsigned index : llvm::reverse(definition.order))
    if (needed.test(index))
      for (unsigned input : definition.wires[index].dependencies)
        needed.set(input);
  for (unsigned index : definition.order) {
    const auto &wire = definition.wires[index];
    if (!needed.test(index) || isa<BlockArgument>(wire.endpoint.value))
      continue;
    auto value = expression(wire, instancePrefix);
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
  llvm::SmallDenseSet<Operation *> seen;
  for (const auto &wire : definition.wires) {
    if (!wire.call || !seen.insert(wire.call->receiver).second)
      continue;
    auto member = instanceName(wire.call->receiver);
    if (failed(member))
      return failure();
    out << "  " << moduleName(wire.call->definition) << " " << *member << ";\n";
  }
  out << "};\n";
  return success();
}

LogicalResult BehavioralEmitter::emitModule(raw_ostream &out, unsigned index) {
  const auto &definition = program.definitions[index];
  for (auto [result, output] : llvm::enumerate(definition.outputs)) {
    signature(out, definition, result, moduleName(index) + "::");
    out << " {\n";
    if (failed(emitWires(out, definition, {output.wire})))
      return failure();
    out << "    return " << wireName(output.wire) << ";\n}\n";
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
  header << "private:\n  " << moduleName(program.rootIndex) << " pyc_body;\n};\n";
  for (size_t i = 1; i < components.size(); ++i)
    header << "}\n";

  llvm::raw_string_ostream source(group.source);
  source << "void " << StringRef(*name).drop_front(2) << "::Work() {\n";
  llvm::SmallDenseSet<unsigned> declaredInputs;
  SmallVector<unsigned> outputWires;
  for (const auto &output : definition.outputs) {
    outputWires.push_back(output.wire);
    for (unsigned input : output.inputs)
      declaredInputs.insert(input);
  }
  for (auto [index, wire] : llvm::enumerate(definition.wires)) {
    if (!declaredInputs.contains(index))
      continue;
    auto arg = cast<BlockArgument>(wire.endpoint.value).getArgNumber();
    source << "    const auto " << wireName(index) << " = gfsim::extract<"
           << wire.width << ">(" << inputs[arg] << ".packed(), " << wire.low << ");\n";
  }
  if (failed(emitWires(source, definition, outputWires, "pyc_body.")))
    return failure();
  for (const auto &output : definition.outputs) {
    const auto &wire = definition.wires[output.wire];
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
    source << "    " << outputs[output.port.port] << ".assignSlice(" << leaf->low
           << ", " << wireName(output.wire) << ");\n";
  }
  source << "}\n";
  parts.systemHeader = "#pragma once\n#include \"" + parts.rootHeaderPath +
                       "\"\nusing pyc_root = " + *name + ";\n";
  appendCppDutHeader(parts, inputs, outputs, "observations_.Configure({}, 0)",
                     /*hasChecks=*/false, /*observes=*/false);
  llvm::raw_string_ostream map(parts.portMap);
  map << llvm::json::Value(llvm::json::Object{
      {"schema_version", 1}, {"target", "cpp"}, {"inputs", std::move(inputRows)},
      {"outputs", std::move(outputRows)}, {"controls", llvm::json::Array{}},
      {"domains", llvm::json::Object{{"clock", nullptr}, {"reset", nullptr}}}});
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
