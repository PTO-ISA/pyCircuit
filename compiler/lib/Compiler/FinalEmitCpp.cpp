#include "FinalEmitCpp.h"
#include "FinalCppNames.h"
#include "HardwareEmitCommon.h"
#include "HardwareEmitCppChecks.h"
#include "pycircuit/Dialect/ACIR/SourceUnitValidation.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringSet.h"
#include "llvm/Support/raw_ostream.h"
using namespace mlir;
namespace acir::compiler {
namespace {
LogicalResult emitEnum(HardwareEmitContext &context, ac::EnumOp declaration,
                       raw_ostream &out) {
  auto type =
      ac::EnumType::get(declaration.getContext(), declaration.getSymNameAttr());
  auto definition = context.analysis.resolveEnum(type, declaration);
  auto name = context.cppType(type, declaration);
  if (failed(definition) || failed(name))
    return failure();
  SmallVector<StringRef> parts;
  StringRef(*name).drop_front(2).split(parts, "::");
  for (size_t i = 0; i + 1 < parts.size(); ++i)
    out << "namespace " << parts[i] << " { ";
  out << "struct " << parts.back() << " { gfsim::Bits<" << definition->width
      << "> code{}; };\n";
  for (size_t i = 0; i + 1 < parts.size(); ++i)
    out << "}\n";
  out << "template <> struct gfsim::hardware_traits<" << *name << "> : "
      << "gfsim::hardware_struct_traits<" << *name << ", &" << *name
      << "::code> {};\n";
  return success();
}
LogicalResult emitRecord(HardwareEmitContext &context, ac::StructOp record,
                         raw_ostream &out, llvm::DenseSet<Operation *> &done) {
  if (done.contains(record))
    return success();
  for (auto raw : record.getFields()) {
    auto field = cast<DictionaryAttr>(raw);
    if (auto nested = dyn_cast<ac::StructType>(
            field.getAs<TypeAttr>("type").getValue())) {
      auto declaration = context.analysis.lookupStruct(nested);
      if (!declaration || failed(emitRecord(context, declaration, out, done)))
        return failure();
    }
  }
  done.insert(record);
  auto name = context.cppType(
      ac::StructType::get(record.getContext(), record.getSymNameAttr()),
      record);
  if (failed(name))
    return failure();
  StringRef qualified = *name;
  SmallVector<StringRef> parts;
  qualified.drop_front(2).split(parts, "::");
  for (size_t i = 0; i + 1 < parts.size(); ++i)
    out << "namespace " << parts[i] << " { ";
  out << "struct " << parts.back() << " {\n";
  SmallVector<std::string> pointers;
  for (auto raw : record.getFields()) {
    auto field = cast<DictionaryAttr>(raw);
    auto type =
        context.cppType(field.getAs<TypeAttr>("type").getValue(), record);
    auto source = field.getAs<StringAttr>("name");
    auto member = legalizeIdentifier(source.getValue(),
                                     [&] { return record.emitOpError(); });
    if (failed(type) || failed(member))
      return failure();
    out << "  " << *type << " " << *member << "{};\n";
    pointers.push_back("&" + *name + "::" + *member);
  }
  out << "};\n";
  for (size_t i = 0; i + 1 < parts.size(); ++i)
    out << "}\n";
  out << "template <> struct gfsim::hardware_traits<" << *name << "> : "
      << "gfsim::hardware_struct_traits<" << *name;
  for (auto &pointer : pointers)
    out << ", " << pointer;
  out << "> {};\n";
  return success();
}
FailureOr<ac::ModuleOp> rootModule(HardwareEmitContext &context) {
  auto systems = context.package.getOps<ac::SystemOp>();
  if (!llvm::hasSingleElement(systems))
    return context.package.emitOpError() << "one system required";
  auto system = *systems.begin();
  auto callee = system.getEntry().getAs<FlatSymbolRefAttr>("callee");
  auto root = callee ? dyn_cast_or_null<ac::ModuleOp>(
                           context.analysis.lookupDefinition(callee.getValue()))
                     : ac::ModuleOp();
  if (!root)
    return system.emitOpError() << "root module is unresolved";
  return root;
}
bool isSystemRoot(ac::ModuleOp root) {
  auto kind = root->getAttrOfType<StringAttr>("ac.root_kind");
  return kind && kind.getValue() == "system";
}
struct SystemControls {
  std::string clock, reset;
};
FailureOr<SystemControls> systemControls(ac::ModuleOp root) {
  auto fail = [&] { return root.emitOpError(); };
  auto kind = root->getAttrOfType<StringAttr>("ac.root_kind");
  if (!kind || kind.getValue() != "system")
    return fail() << "generated simulation requires a verified system root";
  auto domain = root->getAttrOfType<DictionaryAttr>("ac.domain_inputs");
  auto clock = domain ? domain.getAs<IntegerAttr>("clock") : IntegerAttr();
  auto reset = domain ? domain.getAs<IntegerAttr>("reset") : IntegerAttr();
  if (!domain || domain.size() != 2 || !clock || !reset ||
      clock.getValue().getActiveBits() > 64 ||
      reset.getValue().getActiveBits() > 64)
    return fail() << "system root requires verified clock/reset domains";
  uint64_t clockOrdinal = clock.getValue().getZExtValue();
  uint64_t resetOrdinal = reset.getValue().getZExtValue();
  auto inputs = root.getInputNames();
  if (clockOrdinal == resetOrdinal || inputs.size() != 2 ||
      clockOrdinal >= inputs.size() || resetOrdinal >= inputs.size() ||
      !root.getOutputNames().empty())
    return fail() << "system root must be closed except for clock/reset";
  auto legal = [&](uint64_t ordinal) {
    return legalizeIdentifier(cast<StringAttr>(inputs[ordinal]).getValue(),
                              fail);
  };
  auto clockName = legal(clockOrdinal), resetName = legal(resetOrdinal);
  if (failed(clockName) || failed(resetName))
    return failure();
  return SystemControls{std::move(*clockName), std::move(*resetName)};
}
std::string systemSimulationMain(const SystemControls &controls,
                                 StringRef config) {
  std::string text;
  llvm::raw_string_ostream out(text);
  out << "#include <gfsim/SystemRunner.h>\n"
         "#include \"pycircuit_system.hpp\"\n"
         "#include <charconv>\n#include <cstdint>\n#include <iostream>\n"
         "#include <limits>\n#include <string>\n#include <string_view>\n"
         "#include <vector>\n\n"
         "namespace {\n"
         "constexpr std::string_view pyc_simulation_config = R\"pyc("
      << config
      << ")pyc\";\n"
         "constexpr std::uint64_t pyc_max_cycles = "
         "4611686018427387903ULL;\n"
         "auto pyc_bit(bool value) {\n"
         "  return gfsim::wire<gfsim::Bits<1>>::known("
         "gfsim::Bits<1>{value ? 1u : 0u});\n}\n"
         "struct pyc_run_context {\n"
         "  pyc_dut &dut;\n  std::uint64_t cycles;\n"
         "  pyc_dut::Inputs inputs{};\n"
         "  static void initialize(void *opaque) {\n"
         "    auto &self = *static_cast<pyc_run_context *>(opaque);\n"
         "    self.inputs."
      << controls.clock
      << " = pyc_bit(false);\n"
         "    self.inputs."
      << controls.reset
      << " = pyc_bit(false);\n"
         "    self.dut.drive(self.inputs);\n  }\n"
         "  static bool drive(void *opaque, std::uint64_t epoch) {\n"
         "    auto &self = *static_cast<pyc_run_context *>(opaque);\n"
         "    if (epoch >= self.cycles * 2) return false;\n"
         "    self.inputs."
      << controls.clock
      << " = pyc_bit((epoch & 1u) != 0);\n"
         "    self.inputs."
      << controls.reset
      << " = pyc_bit(false);\n"
         "    self.dut.drive(self.inputs);\n    return true;\n  }\n"
         "};\n"
         "bool pyc_cycles(std::string_view text, std::uint64_t &value) {\n"
         "  if (text.empty()) return false;\n"
         "  auto result = std::from_chars(text.data(), text.data() + "
         "text.size(), value);\n"
         "  return result.ec == std::errc{} && result.ptr == text.data() + "
         "text.size() && value > 0 && value <= pyc_max_cycles;\n}\n"
         "} // namespace\n\n"
         "int main(int argc, char **argv) {\n"
         "  std::uint64_t cycles = 10;\n  bool has_cycles = false;\n"
         "  bool has_events = false;\n"
         "  std::vector<std::string> forwarded;\n"
         "  forwarded.emplace_back(argv && argc > 0 ? argv[0] : "
         "\"pycircuit_system\");\n"
         "  for (int index = 1; index < argc; ++index) {\n"
         "    std::string_view option = argv[index];\n"
         "    if (option == \"--cycles\") {\n"
         "      if (has_cycles || ++index >= argc || "
         "!pyc_cycles(argv[index], cycles)) {\n"
         "        std::cerr << \"pycircuit_system: --cycles requires one "
         "positive decimal value\\n\";\n        return 2;\n      }\n"
         "      has_cycles = true;\n      continue;\n    }\n"
         "    has_events = has_events || option == \"--events\";\n"
         "    forwarded.emplace_back(argv[index]);\n  }\n"
         "  if (!has_events) { forwarded.emplace_back(\"--events\"); "
         "forwarded.emplace_back(\"-\"); }\n"
         "  std::vector<char *> runner_args;\n"
         "  for (auto &argument : forwarded) "
         "runner_args.push_back(argument.data());\n"
         "  gfsim::SystemRunner runner(static_cast<int>(runner_args.size()), "
         "runner_args.data(), pyc_simulation_config);\n"
         "  if (!runner.ready()) return 2;\n"
         "  pyc_dut dut(runner.workers());\n"
         "  pyc_run_context context{dut, cycles};\n"
         "  const gfsim::RunnerCallbacks callbacks{&context, "
         "&pyc_run_context::initialize, &pyc_run_context::drive, nullptr};\n"
         "  return runner.Run(dut.system(), dut.observations(), "
         "pyc_observation_metadata(), callbacks);\n}\n";
  out.flush();
  return text;
}
LogicalResult emitDefinitionOnce(HardwareEmitContext &context,
                                 ac::ModuleOp module, raw_ostream &out,
                                 llvm::DenseSet<Operation *> &done) {
  if (done.contains(module))
    return success();
  for (auto &op : module.getBody().front())
    if (isa<ac::InstanceOp, ac::CollectionOp>(op)) {
      auto *callee =
          isa<ac::InstanceOp>(op)
              ? context.analysis.resolveCallee(cast<ac::InstanceOp>(op))
              : context.analysis.resolveCallee(cast<ac::CollectionOp>(op));
      if (auto nested = dyn_cast_or_null<ac::ModuleOp>(callee))
        if (nested.getSourceOwner() == module.getSourceOwner() &&
            failed(emitDefinitionOnce(context, nested, out, done)))
          return failure();
    }
  done.insert(module);
  return emitHardwareCppDefinition(context, module, out);
}
} // namespace
FailureOr<FinalCppSourceParts>
emitCppSourceParts(ModuleOp package, ac::HardwareAnalysis &analysis) {
  HardwareEmitContext context(package, analysis);
  // Native preparation builds the shared observation and source-check plans.
  // Closed system roots consume both through their generated runner; module
  // roots retain the existing public assertion rejection until they own an
  // equivalent execution boundary.
  if (failed(context.prepareNativeChecks()))
    return failure();
  auto root = rootModule(context);
  if (failed(root))
    return failure();
  const bool managedRoot = context.managesChecks();
  auto unsupported = package.walk([&](Operation *op) {
    if (!isa<ac::SourceExpectOp>(op) || managedRoot)
      return WalkResult::advance();
    op->emitOpError() << "hardware instrumentation emission is not implemented";
    return WalkResult::interrupt();
  });
  if (unsupported.wasInterrupted())
    return failure();
  return emitPreparedCppSourceParts(context);
}
FailureOr<FinalCppSourceParts>
emitPreparedCppSourceParts(HardwareEmitContext &context) {
  auto package = context.package;
  auto &analysis = context.analysis;
  // The retained view never grants authority after mutation. Both public and
  // internal native gates consume this exact verified emission implementation.
  if (failed(analysis.verifySourceCheckPlan(context.sourceChecks())))
    return failure();
  auto root = rootModule(context);
  if (failed(root))
    return failure();
  FinalCppSourceParts parts;
  parts.sourceCheckCount = context.sourceChecks().checks.size();
  auto rootType = context.rootCppType();
  if (failed(rootType))
    return failure();
  parts.rootCppName = *rootType;
  parts.rootHeaderPath = context.names(*root).header;
  // The descriptor table is the compiled form of every source observation
  // occurrence, so it is built before the glue that carries it. An unobserved
  // design keeps the previous empty configuration and byte-identical glue.
  auto descriptorTable = cppObservationDescriptorTable(context);
  if (failed(descriptorTable))
    return failure();
  std::string observationMetadata;
  if (isSystemRoot(*root)) {
    auto generated = cppObservationRunnerMetadata(context);
    if (failed(generated))
      return failure();
    observationMetadata = std::move(*generated);
  }
  auto configure = cppObservationConfigureCall(context);
  if (failed(configure))
    return failure();
  bool observes = !descriptorTable->empty();
  parts.supportHeader =
      "#pragma once\n#include <gfsim/wire.h>\n#include <gfsim/dff.h>\n"
      "#include <gfsim/sync_mem.h>\n#include <gfsim/byte_mem.h>\n#include "
      "<gfsim/fifo.h>\n"
      "#include <gfsim/SimModule.h>\n#include <gfsim/SimSystem.h>\n"
      "#include <gfsim/collection.h>\n"
      "#include <gfsim/WorkExecutor.h>\n"
      "#include <memory>\n#include <string>\n#include <string_view>\n"
      "#include <array>\n#include "
      "<algorithm>\n"
      "#include <bit>\n#include <type_traits>\n#include <utility>\n"
      "#include <cstddef>\n#include <cstdint>\n";
  if (observes)
    parts.supportHeader +=
        "#include <gfsim/ObservationSlot.h>\n"
        "// Native observation binding. A generated module reaches the "
        "system-owned\n"
        "// observation slots through this handle, which the DUT binds before "
        "Build().\n"
        "struct pyc_observation_sink {\n"
        "  gfsim::ObservationSlots *slots = nullptr;\n"
        "  const gfsim::SimSystem *system = nullptr;\n"
        "};\n";
  parts.supportHeader +=
      "constexpr std::int64_t pyc_floordiv(std::int64_t a, std::int64_t b) { "
      "return a/b - ((a%b != 0 && (a<0)!=(b<0)) ? 1 : 0); }\n"
      "constexpr std::int64_t pyc_mod(std::int64_t a, std::int64_t b) { auto "
      "r=a%b; return r != 0 && (r<0)!=(b<0) ? r+b : r; }\n"
      "template <std::size_t N> consteval std::uint64_t pyc_mod_words(\n"
      "  std::array<std::uint64_t,N> words, bool negative, std::uint64_t "
      "modulus) {\n"
      "  std::uint64_t remainder=0;\n"
      "  for (std::size_t i=N; i-- > 0;) for (unsigned bit=64; bit-- > 0;)\n"
      "    remainder=(remainder*2+((words[i]>>bit)&1))%modulus;\n"
      "  return negative && remainder ? modulus-remainder : remainder;\n}\n"
      "template <unsigned W> gfsim::Bits<W> pyc_bits(std::int64_t value) "
      "{ return gfsim::Bits<W>{static_cast<std::uint64_t>(value)}; }\n"
      "\n";
  {
    llvm::raw_string_ostream out(parts.supportHeader);
    for (auto declaration : package.getOps<ac::EnumOp>())
      if (failed(emitEnum(context, declaration, out)))
        return failure();
    llvm::DenseSet<Operation *> done;
    for (auto record : analysis.getStructs())
      if (failed(emitRecord(context, record, out, done)))
        return failure();
    out.flush();
  }
  auto emitError = [&] { return package.emitOpError(); };
  auto units = ac::collectFinalSourceUnits(package, emitError);
  if (failed(units))
    return failure();
  llvm::StringMap<DictionaryAttr> paths;
  for (const ac::FinalSourceUnitView &unit : *units) {
    FinalCppSourceGroup group;
    group.sourceOwner = unit.owner;
    auto owner = sourceOwnerComponents(unit.owner, emitError);
    if (failed(owner))
      return failure();
    auto headerPath = sourcePathName(owner->filePath, ".hpp", emitError);
    if (failed(headerPath))
      return failure();
    group.headerPath = std::move(*headerPath);
    if (!paths.try_emplace(group.headerPath, unit.owner).second)
      return emitError() << "source owner header path collision";
    for (Operation *declaration : unit.declarations)
      if (auto module = dyn_cast<ac::ModuleOp>(declaration)) {
        group.sourcePath = context.names(module).source;
        break;
      }
    group.header = "#pragma once\n#include \"pycircuit_support.hpp\"\n";
    llvm::StringSet<> includes;
    for (Operation *declaration : unit.declarations)
      if (auto module = dyn_cast<ac::ModuleOp>(declaration))
        for (auto &op : module.getBody().front())
          if (isa<ac::InstanceOp, ac::CollectionOp>(op)) {
            auto *callee =
                isa<ac::InstanceOp>(op)
                    ? analysis.resolveCallee(cast<ac::InstanceOp>(op))
                    : analysis.resolveCallee(cast<ac::CollectionOp>(op));
            if (auto nested = dyn_cast_or_null<ac::ModuleOp>(callee)) {
              auto childHeader = context.names(nested).header;
              if (childHeader != group.headerPath)
                includes.insert(childHeader);
            }
          }
    for (auto &item : includes)
      group.header += "#include \"" + item.getKey().str() + "\"\n";
    {
      llvm::raw_string_ostream out(group.header);
      llvm::DenseSet<Operation *> done;
      for (Operation *declaration : unit.declarations)
        if (auto module = dyn_cast<ac::ModuleOp>(declaration))
          if (failed(emitDefinitionOnce(context, module, out, done)))
            return failure();
      out.flush();
    }
    if (!group.sourcePath.empty())
      group.source = "#include \"" + group.headerPath + "\"\n";
    parts.sourceGroups.push_back(std::move(group));
  }
  parts.systemHeader = "#pragma once\n#include \"" + parts.rootHeaderPath +
                       "\"\n"
                       "using pyc_root = " +
                       parts.rootCppName + ";\n";
  if (isSystemRoot(*root))
    parts.systemHeader += "#include <gfsim/SystemRunner.h>\n";
  if (observes)
    parts.systemHeader += *descriptorTable;
  parts.systemHeader += observationMetadata;
  {
    llvm::raw_string_ostream out(parts.systemHeader);
    out << "#include <gfsim/SimSystem.h>\n#include <stdexcept>\n"
           "class pyc_dut final {\npublic:\n";
    SmallVector<std::string> inputs, outputs;
    auto ports = [&](StringRef typeName, ArrayAttr names,
                     SmallVectorImpl<std::string> &ports) -> LogicalResult {
      out << "  struct " << typeName << " {\n";
      for (Attribute raw : names) {
        auto name = legalizeIdentifier(cast<StringAttr>(raw).getValue(),
                                       [&] { return root->emitOpError(); });
        if (failed(name))
          return failure();
        ports.push_back(*name);
        out << "    decltype(std::declval<pyc_root>()." << *name << ") "
            << *name << ";\n";
      }
      out << "  };\n";
      return success();
    };
    if (failed(ports("Inputs", root->getInputNames(), inputs)) ||
        failed(ports("Outputs", root->getOutputNames(), outputs)))
      return failure();
    out << "  explicit pyc_dut(std::size_t workers = 1) : executor_(workers), ";
    if (observes)
      out << "root_(std::make_shared<pyc_root>(\"root\", &executor_, "
             "&observation_sink_, 0))";
    else
      out << "root_(std::make_shared<pyc_root>(\"root\", &executor_))";
    if (context.sourceChecks().hasChecks())
      out << ", system_(*root_)";
    out << " {\n";
    if (observes)
      out << "    observation_sink_.slots = &observations_;\n"
             "    observation_sink_.system = &system_;\n";
    out << "    if (!" << *configure
        << " || "
           "!system_.AttachObservations(observations_) || "
           "!system_.AddModule(*root_)) throw std::logic_error(\"DUT setup "
           "failed\");\n"
           "  }\n  pyc_dut(const pyc_dut &) = delete;\n"
           "  pyc_dut &operator=(const pyc_dut &) = delete;\n"
           "  // Control-thread access only, between sampling epochs.\n"
           "  void drive(const Inputs &inputs) {\n";
    for (auto &name : inputs)
      out << "    root_->" << name << " = inputs." << name << ";\n";
    out << "  }\n  Outputs sample() const {\n"
           "    if (system_.state() != gfsim::SimSystemState::Ready || "
           "system_.cycle() == 0) throw std::logic_error(\"no successful DUT "
           "sample\");\n"
           "    return {";
    llvm::interleaveComma(outputs, out, [&](const std::string &name) {
      out << "root_->" << name;
    });
    out << "};\n  }\n  gfsim::SimSystem &system() { return system_; }\n"
           "  gfsim::ObservationSlots &observations() { return observations_; "
           "}\n"
           "private:\n  class System final : public gfsim::SimSystem {\n";
    if (context.sourceChecks().hasChecks())
      out << "  public:\n    explicit System(pyc_root &root) noexcept : "
             "root_(root) {}\n"
             "  private:\n    bool Precheck() noexcept override {\n"
             "      source_failure_ = {};\n"
             "      return root_.__pyc_validate_checks(source_failure_);\n    "
             "}\n"
             "    gfsim::SimFailureInfo DescribeFailure(gfsim::SimFailurePhase "
             "phase) "
             "const noexcept override {\n"
             "      if (phase == gfsim::SimFailurePhase::Check && "
             "!source_failure_.code.empty()) return source_failure_;\n"
             "      return gfsim::SimSystem::DescribeFailure(phase);\n    }\n"
             "    pyc_root &root_;\n    gfsim::SimFailureInfo "
             "source_failure_{};\n";
    else
      out << "    bool Precheck() noexcept override { return true; }\n";
    out << "  };\n"
           "  gfsim::WorkExecutor executor_;\n";
    if (observes)
      out << "  pyc_observation_sink observation_sink_;\n";
    out << "  std::shared_ptr<pyc_root> root_;\n"
           "  gfsim::ObservationSlots observations_;\n  System system_;\n};\n";
  }
  if (isSystemRoot(*root)) {
    auto controls = systemControls(*root);
    if (failed(controls))
      return failure();
    parts.simulationConfig =
        "{\"deadlock_window\":null,\"max_domain_cycles\":{},"
        "\"max_ticks\":9223372036854775807,"
        "\"schema\":\"pycircuit-model-config\",\"version\":\"1\"}\n";
    parts.simulationMain = systemSimulationMain(
        *controls, StringRef(parts.simulationConfig).trim());
  }
  return parts;
}
LogicalResult emitCpp(ModuleOp package, ac::HardwareAnalysis &analysis,
                      llvm::raw_ostream &out) {
  auto parts = emitCppSourceParts(package, analysis);
  if (failed(parts))
    return failure();
  out << parts->supportHeader;
  for (auto &group : parts->sourceGroups)
    out << "\n" << group.header;
  out << "\n" << parts->systemHeader;
  return success();
}
} // namespace acir::compiler
