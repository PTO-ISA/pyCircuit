// Emit source-owned modules from verified final ACIR. Publication is
// transactional in the Python driver; this transport writes nothing before
// complete success.
#include "Compiler/FinalSourceMap.h"
#include "Compiler/StructuralCppBackend.h"
#include "Compiler/VerilogBackend.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Parser/Parser.h"
#include "pycircuit/Dialect/ACIR/ACIRDialect.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "pycircuit/Dialect/ACIR/SourceUnitValidation.h"
#include "pycircuit/Dialect/HWIR/HWIRDialect.h"
#include "pycircuit/Dialect/HWIR/HWIROps.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/Support/FormatVariadic.h"
#include "llvm/Support/JSON.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/raw_ostream.h"

using namespace mlir;

namespace {
llvm::json::Object sourceOwner(DictionaryAttr owner) {
  return llvm::json::Object{
      {"package", owner.getAs<StringAttr>("package").getValue()},
      {"path", owner.getAs<StringAttr>("path").getValue()}};
}
std::string attributeText(Attribute attr) {
  std::string text;
  llvm::raw_string_ostream stream(text);
  attr.print(stream);
  return text;
}
std::string cppStringLiteral(StringRef value) {
  std::string text;
  llvm::raw_string_ostream out(text);
  out << '"';
  out.write_escaped(value, false);
  out << '"';
  return text;
}

llvm::json::Object portAvailability(
    const acir::compiler::VerilogDesignPlan &plan) {
  const auto &root = plan.design.modules[plan.design.rootModule];
  llvm::json::Array inputs, outputs, controls, observations;
  auto describe = [](const acir::compiler::ConcretePortPlan &port,
                     unsigned ordinal) {
    llvm::json::Array fields;
    for (const auto &field : port.type.fields) {
      llvm::json::Array path;
      for (const std::string &component : field.path)
        path.emplace_back(component);
      fields.emplace_back(llvm::json::Object{{"path", std::move(path)},
                                             {"low", field.low},
                                             {"width", field.width}});
    }
    return llvm::json::Object{{"ordinal", ordinal},
                              {"name", port.name.getValue()},
                              {"wire", port.cppName},
                              {"width", port.type.packedWidth},
                              {"fields", std::move(fields)}};
  };
  auto exposedInput = [&](unsigned index) {
    return index < root.publicInputCount ||
           (plan.mode == acir::compiler::VerilogExportMode::Physical &&
            ((root.clockInput && index == *root.clockInput) ||
             (root.resetInput && index == *root.resetInput)));
  };
  for (auto [index, port] : llvm::enumerate(root.inputs))
    if (exposedInput(index))
      inputs.emplace_back(describe(port, index));
  for (unsigned index = 0; index < root.publicOutputCount; ++index) {
    auto output = describe(root.outputs[index], index);
    output["availability"] = nullptr;
    outputs.emplace_back(std::move(output));
  }
  llvm::json::Object domains{{"clock", nullptr}, {"reset", nullptr}};
  if (root.clockInput)
    domains["clock"] = *root.clockInput;
  if (root.resetInput)
    domains["reset"] = *root.resetInput;
  if (plan.mode == acir::compiler::VerilogExportMode::Managed &&
      plan.design.system) {
    auto control = [&](StringRef direction, StringRef role, StringRef wire,
                       unsigned width) {
      controls.emplace_back(llvm::json::Object{{"direction", direction},
                                               {"role", role},
                                               {"wire", wire},
                                               {"width", width}});
    };
    control("input", "phase", "pyc_phase", 3);
    control("input", "commit_permission", "pyc_root_commit_ok", 1);
    control("output", "check_complete", "pyc_check_complete", 1);
    control("output", "sampling_error", "pyc_sampling_error", 1);
    control("output", "local_error", "pyc_local_error", 1);
  }
  llvm::json::Object result{{"schema_version", 1},
                            {"target", "verilog"},
                            {"inputs", std::move(inputs)},
                            {"outputs", std::move(outputs)},
                            {"domains", std::move(domains)},
                            {"controls", std::move(controls)}};
  if (plan.mode == acir::compiler::VerilogExportMode::Physical &&
      !plan.design.observations.empty()) {
    uint64_t maximumWidth = 1;
    for (const auto &observation : plan.design.observations)
      maximumWidth = std::max(maximumWidth, observation.payloadWidth);
    for (const auto &observation : plan.design.observations)
      observations.emplace_back(llvm::json::Object{
          {"ordinal", observation.stableOrdinal},
          {"group_index", observation.groupIndex},
          {"group_size", observation.groupSize},
          {"kind", observation.kindName},
          {"instance", observation.instancePath},
          {"payload_width", observation.payloadWidth},
          {"carries_value", observation.carriesValue},
          {"registration", observation.registrationJson},
          {"site", observation.siteJson},
          {"spec", observation.specJson},
          {"report_name", observation.reportName}});
    result["observation_channel"] = llvm::json::Object{
        {"protocol", "pycircuit-physical-observation-v1"},
        {"timing", "registered_on_committed_rising_edge"},
        {"count", plan.design.observations.size()},
        {"slot_width", maximumWidth},
        {"valid", "pyc_observation_valid"},
        {"path", "pyc_observation_path"},
        {"value", "pyc_observation_value"},
        {"known", "pyc_observation_known"},
        {"z", "pyc_observation_z"}};
    result["observations"] = std::move(observations);
  }
  return result;
}

FailureOr<acir::ac::ModuleOp>
resolveSystemRoot(ModuleOp package, acir::ac::HardwareAnalysis &analysis) {
  auto systems = package.getOps<acir::ac::SystemOp>();
  if (!llvm::hasSingleElement(systems)) {
    package.emitOpError() << "emit requires exactly one ac.system";
    return failure();
  }
  auto system = *systems.begin();
  auto callee = system.getEntry().getAs<FlatSymbolRefAttr>("callee");
  if (!callee) {
    system.emitOpError() << "system entry has no callee";
    return failure();
  }
  auto root = dyn_cast_or_null<acir::ac::ModuleOp>(
      analysis.lookupDefinition(callee.getValue()));
  if (!root) {
    system.emitOpError() << "system entry does not resolve to a module";
    return failure();
  }
  return root;
}
} // namespace

int main(int argc, char **argv) {
  DialectRegistry dialects;
  dialects.insert<acir::ac::ACIRDialect, acir::hwir::HWIRDialect>();
  MLIRContext context(dialects);
  context.loadAllAvailableDialects();
  auto emitError = [&] { return mlir::emitError(UnknownLoc::get(&context)); };
  if (argc == 3 && llvm::StringRef(argv[1]) == "--verify-source-map") {
    auto buffer = llvm::MemoryBuffer::getFile(argv[2]);
    if (!buffer) {
      emitError() << "cannot read source map";
      return 1;
    }
    std::string path;
    if (failed(acir::compiler::validateFinalSourceMapText(
            (*buffer)->getBuffer(), context, emitError, &path)))
      return 1;
    llvm::outs() << llvm::formatv(
        "{0}\n", llvm::json::Value(llvm::json::Object{{"path", path}}));
    return 0;
  }
  StringRef target, format = "json", rtlMode = "managed";
  for (int index = 2; index + 1 < argc; index += 2) {
    StringRef option = argv[index];
    if (option == "--target")
      target = argv[index + 1];
    else if (option == "--format")
      format = argv[index + 1];
    else if (option == "--rtl-mode")
      rtlMode = argv[index + 1];
    else
      target = "";
  }
  if ((target != "verilog" && target != "structural-cpp") ||
      (format != "json" && format != "source") ||
      (rtlMode != "managed" && rtlMode != "physical") ||
      (target != "verilog" && rtlMode != "managed") || argc < 4 ||
      ((argc - 2) % 2) != 0) {
    llvm::errs() << "usage: pycircuit-emit FINAL.ac --target "
                    "verilog|structural-cpp "
                    "[--format json|source] "
                    "[--rtl-mode managed|physical]\n";
    return 2;
  }
  auto package = parseSourceFile<ModuleOp>(argv[1], &context);
  if (!package || failed(mlir::verify(*package)))
    return 1;
  const auto verilogMode = rtlMode == "physical"
                               ? acir::compiler::VerilogExportMode::Physical
                               : acir::compiler::VerilogExportMode::Managed;
  if (format == "source") {
    std::string text;
    llvm::raw_string_ostream stream(text);
    LogicalResult status = failure();
    if (target == "structural-cpp") {
      if ((*package)->hasAttr("hwir.contract")) {
        auto plan = acir::compiler::prepareStructuralCpp(*package);
        if (succeeded(plan))
          status = acir::compiler::emitStructuralCpp(*plan, stream);
      }
    }
    else {
      auto plan = acir::compiler::prepareVerilog(*package, verilogMode);
      if (succeeded(plan))
        status = acir::compiler::emitVerilog(*plan, stream);
    }
    if (failed(status))
      return 1;
    llvm::outs() << text;
    return 0;
  }

  if (target == "structural-cpp") {
    if (!(*package)->hasAttr("hwir.contract")) {
      package->emitOpError()
          << "structural C++ requires independent HWIR; run "
             "ac-final-to-hwir first";
      return 1;
    }
    auto plan = acir::compiler::prepareStructuralCpp(*package);
    if (failed(plan))
      return 1;
    auto systems = package->getOps<acir::hwir::SystemOp>();
    if (!llvm::hasSingleElement(systems))
      return 1;
    auto system = *systems.begin();
    auto callee = system.getEntry().getAs<FlatSymbolRefAttr>("callee");
    auto root = dyn_cast_or_null<acir::hwir::ModuleOp>(
        SymbolTable::lookupSymbolIn(*package, callee));
    if (!root)
      return 1;
    std::string source;
    llvm::raw_string_ostream sourceStream(source);
    if (failed(acir::compiler::emitStructuralCppHeader(*plan, sourceStream)))
      return 1;
    if (failed(acir::compiler::emitStructuralCppHostApi(*plan, sourceStream)))
      return 1;
    llvm::json::Array manifest;
    for (const auto &entry : plan->manifest) {
      llvm::json::Object row{{"cpp_name", entry.cppName},
                             {"definition", entry.definition}};
      llvm::json::Object integers, types;
      for (const auto &argument : entry.integerArguments)
        integers[argument.name] = argument.value;
      for (const auto &argument : entry.typeArguments)
        types[argument.name] = argument.value;
      row["integer_arguments"] = std::move(integers);
      row["type_arguments"] = std::move(types);
      manifest.emplace_back(std::move(row));
    }
    const auto &rootPlan = plan->modules[plan->rootModule];
    std::string observationDeclarations;
    llvm::raw_string_ostream observationStream(observationDeclarations);
    observationStream << "static constexpr std::array<gfsim::ObservationDescriptor, "
                      << plan->observations.size()
                      << "> pyc_observation_descriptors{{\n";
    for (const auto &observation : plan->observations)
      observationStream
          << "    gfsim::ObservationDescriptor{" << observation.stableOrdinal
          << ", " << observation.ownerKey << ", "
          << observation.registrationKey << ", " << observation.siteKey
          << ", gfsim::ObservationKind::"
          << (observation.kind ==
                      acir::compiler::ConcreteObservationKind::Gauge
                  ? "Gauge"
                  : "Event")
          << ", " << observation.groupIndex << ", "
          << observation.groupSize << ", " << observation.payloadWidth
          << "},\n";
    observationStream << "}};\n"
                      << "static const std::array<gfsim::RunnerObservation, "
                      << plan->observations.size()
                      << "> pyc_observation_metadata{{\n";
    for (const auto &observation : plan->observations)
      observationStream
          << "    gfsim::RunnerObservation{" << observation.stableOrdinal
          << ", " << cppStringLiteral(observation.kindName) << ", "
          << cppStringLiteral(observation.instancePath) << ", "
          << cppStringLiteral(observation.registrationJson) << ", "
          << cppStringLiteral(observation.siteJson) << ", "
          << cppStringLiteral(observation.specJson) << ", "
          << (observation.carriesValue ? "true" : "false") << ", "
          << cppStringLiteral(observation.reportName) << ", "
          << observation.groupIndex << ", " << observation.groupSize
          << "},\n";
    observationStream << "}};\n\n";
    observationStream.flush();
    std::string simulationMain;
    llvm::raw_string_ostream mainStream(simulationMain);
    if (plan->system) {
      const auto &clock = rootPlan.inputs[plan->system->clockInput];
      const auto &reset = rootPlan.inputs[plan->system->resetInput];
      mainStream << "#include \"design.hpp\"\n"
                  "#include <gfsim/ObservationSlot.h>\n"
                  "#include <gfsim/SimSystem.h>\n"
                  "#include <gfsim/SystemRunner.h>\n"
                  "#include <array>\n#include <charconv>\n#include <cstdint>\n"
                  "#include <iostream>\n#include <limits>\n#include <stdexcept>\n"
                  "#include <string>\n#include <string_view>\n#include <vector>\n\n"
               << observationDeclarations
               << "namespace {\n"
           "constexpr std::string_view pyc_simulation_config = "
           "R\"pyc({\"deadlock_window\":null,\"max_domain_cycles\":{},"
           "\"max_ticks\":9223372036854775807,"
           "\"schema\":\"pycircuit-model-config\",\"version\":\"1\"})pyc\";\n"
           "constexpr std::uint64_t pyc_max_cycles = "
           "4611686018427387903ULL;\n"
           "auto pyc_bit(bool value) { return "
           "gfsim::wire<gfsim::Bits<1>>::known(gfsim::Bits<1>{value ? 1u : 0u}); }\n"
           "struct pyc_run_context {\n"
           "  pyc_dut &dut;\n  std::uint64_t cycles;\n  pyc_dut::Inputs inputs{};\n"
           "  static void initialize(void *opaque) {\n"
           "    auto &self = *static_cast<pyc_run_context *>(opaque);\n"
           "    self.inputs."
        << clock.cppName << " = pyc_bit(false);\n    self.inputs."
        << reset.cppName
        << " = pyc_bit(false);\n    self.dut.drive(self.inputs);\n  }\n"
           "  static bool drive(void *opaque, std::uint64_t epoch) {\n"
           "    auto &self = *static_cast<pyc_run_context *>(opaque);\n"
           "    if (epoch >= self.cycles * 2) return false;\n"
           "    self.inputs."
        << clock.cppName << " = pyc_bit((epoch & 1u) != 0);\n"
           "    self.inputs."
        << reset.cppName
        << " = pyc_bit(false);\n    self.dut.drive(self.inputs);\n"
           "    return true;\n  }\n};\n"
           "bool pyc_cycles(std::string_view text, std::uint64_t &value) {\n"
           "  if (text.empty()) return false;\n"
           "  auto result = std::from_chars(text.data(), text.data() + text.size(), value);\n"
           "  return result.ec == std::errc{} && result.ptr == text.data() + "
           "text.size() && value > 0 && value <= pyc_max_cycles;\n}\n"
           "} // namespace\n\n"
           "int main(int argc, char **argv) {\n"
           "  std::uint64_t cycles = 10;\n  bool has_cycles = false;\n"
           "  std::vector<std::string> forwarded;\n"
           "  forwarded.emplace_back(argv && argc > 0 ? argv[0] : \"pycircuit_system\");\n"
           "  for (int index = 1; index < argc; ++index) {\n"
           "    std::string_view option = argv[index];\n"
           "    if (option == \"--cycles\") {\n"
           "      if (has_cycles || ++index >= argc || !pyc_cycles(argv[index], cycles)) {\n"
           "        std::cerr << \"pycircuit_system: --cycles requires one positive decimal value\\n\";\n"
           "        return 2;\n      }\n      has_cycles = true;\n      continue;\n    }\n"
           "    forwarded.emplace_back(argv[index]);\n  }\n"
           "  std::vector<char *> runner_args;\n"
           "  for (auto &argument : forwarded) runner_args.push_back(argument.data());\n"
           "  gfsim::SystemRunner runner(static_cast<int>(runner_args.size()), "
           "runner_args.data(), pyc_simulation_config);\n"
           "  if (!runner.ready()) return 2;\n"
           "  pyc_dut dut(runner.workers());\n  pyc_run_context context{dut, cycles};\n"
           "  const gfsim::RunnerCallbacks callbacks{&context, "
           "&pyc_run_context::initialize, &pyc_run_context::drive, nullptr};\n"
           "  return runner.Run(dut.system(), dut.observations(), "
           "pyc_observation_metadata, callbacks);\n}\n";
    }
    mainStream.flush();
    llvm::json::Array arguments;
    for (StringRef kind :
         {StringRef("parameters"), StringRef("type_arguments")})
      if (auto values = system.getEntry().getAs<ArrayAttr>(kind))
        for (Attribute value : values)
          arguments.emplace_back(llvm::json::Object{
              {"kind", kind}, {"value", attributeText(value)}});
    llvm::SmallVector<acir::compiler::FinalSourceMembership> memberships;
    llvm::json::Array sourceGroups;
    auto workShards = acir::compiler::emitStructuralCppWorkShards(*plan);
    if (failed(workShards))
      return 1;
    llvm::SmallVector<llvm::SmallVector<unsigned, 0>, 0> shardsBySourceUnit(
        plan->sourceUnits.size());
    for (auto [index, shard] : llvm::enumerate(*workShards)) {
      if (shard.sourceUnit >= shardsBySourceUnit.size())
        return 1;
      shardsBySourceUnit[shard.sourceUnit].push_back(index);
    }
    for (auto [index, unit] : llvm::enumerate(plan->sourceUnits)) {
      memberships.push_back({unit.sourceOwner, {}});
      std::string path = ("source_unit_" + llvm::Twine(index) + ".cpp").str();
      memberships.back().generatedFiles.push_back(path);
      std::string unitText;
      llvm::raw_string_ostream unitStream(unitText);
      if (failed(acir::compiler::emitStructuralCppSourceUnit(
              *plan, index, unitStream)))
        return 1;
      sourceGroups.emplace_back(llvm::json::Object{
          {"source", sourceOwner(unit.sourceOwner)},
          {"path", path},
          {"text", std::move(unitText)}});
      for (unsigned shardIndex : shardsBySourceUnit[index]) {
        auto &shard = (*workShards)[shardIndex];
        memberships.back().generatedFiles.push_back(shard.path);
        sourceGroups.emplace_back(llvm::json::Object{
            {"source", sourceOwner(unit.sourceOwner)},
            {"path", shard.path},
            {"text", std::move(shard.text)}});
      }
    }
    auto maps =
        acir::compiler::emitFinalSourceMaps(*package, memberships, emitError);
    if (failed(maps))
      return 1;
    llvm::json::Array sourceMaps;
    for (const auto &map : *maps)
      sourceMaps.emplace_back(
          llvm::json::Object{{"source", sourceOwner(map.sourceOwner)},
                             {"path", map.path},
                             {"text", map.text}});
    llvm::outs() << llvm::formatv(
        "{0:2}\n",
        llvm::json::Value(llvm::json::Object{
            {"entry",
             llvm::json::Object{
                 {"definition", "@\"" + callee.getValue().str() + "\""},
                 {"arguments", std::move(arguments)}}},
            {"entry_source", sourceOwner(root.getSourceOwnerAttr())},
            {"structural_cpp_header", source},
            {"structural_cpp_groups", std::move(sourceGroups)},
            {"simulation_main", simulationMain},
            {"simulation_config",
             "{\"deadlock_window\":null,\"max_domain_cycles\":{},"
             "\"max_ticks\":9223372036854775807,"
             "\"schema\":\"pycircuit-model-config\",\"version\":\"1\"}\n"},
            {"source_check_count", plan->sourceCheckCount},
            {"manifest", std::move(manifest)},
            {"source_maps", std::move(sourceMaps)}}));
    return 0;
  }

  if (!(*package)->hasAttr("hwir.contract")) {
    package->emitOpError()
        << "Verilog requires independent HWIR; run ac-final-to-hwir first";
    return 1;
  }

  auto systems = package->getOps<acir::hwir::SystemOp>();
  if (!llvm::hasSingleElement(systems))
    return 1;
  auto system = *systems.begin();
  auto callee = system.getEntry().getAs<FlatSymbolRefAttr>("callee");
  auto root = dyn_cast_or_null<acir::hwir::ModuleOp>(
      SymbolTable::lookupSymbolIn(*package, callee));
  if (!root)
    return 1;
  llvm::json::Array arguments;
  for (StringRef kind : {StringRef("parameters"), StringRef("type_arguments")})
    if (auto values = system.getEntry().getAs<ArrayAttr>(kind))
      for (Attribute value : values)
        arguments.emplace_back(llvm::json::Object{
            {"kind", kind}, {"value", attributeText(value)}});
  llvm::json::Object result{
      {"entry", llvm::json::Object{{"definition",
                                    "@\"" + callee.getValue().str() + "\""},
                                   {"arguments", std::move(arguments)}}},
      {"entry_source", sourceOwner(root.getSourceOwnerAttr())},
      {"rtl_mode", rtlMode.str()},
      {"ir_contract", "hwir-v2"}};
  auto plan = acir::compiler::prepareVerilog(*package, verilogMode);
  if (failed(plan))
    return 1;
  result["port_availability"] = portAvailability(*plan);
  llvm::SmallVector<acir::compiler::FinalSourceMembership> memberships;
  llvm::DenseMap<Attribute, unsigned> membershipIndices;
  for (const auto &unit : plan->design.sourceUnits) {
    membershipIndices.try_emplace(unit.sourceOwner, memberships.size());
    memberships.push_back({unit.sourceOwner, {}});
  }
  {
    auto parts = acir::compiler::emitVerilog(*plan);
    if (failed(parts))
      return 1;
    llvm::json::Array groups, standard;
    for (const auto &group : parts->sourceGroups) {
      auto found = membershipIndices.find(group.sourceOwner);
      if (found == membershipIndices.end()) {
        emitError() << "RTL source group has no final SourceOwner";
        return 1;
      }
      memberships[found->second].generatedFiles.push_back(group.path);
      groups.emplace_back(
          llvm::json::Object{{"source", sourceOwner(group.sourceOwner)},
                             {"path", group.path},
                             {"text", group.text}});
    }
    for (const auto &source : parts->standardSources)
      standard.emplace_back(source);
    result["rtl_source_groups"] = std::move(groups);
    result["rtl_standard_sources"] = std::move(standard);
    result["rtl_core"] = parts->core;
    result["simulation_top"] = parts->simulationTop;
    result["source_check_count"] = parts->sourceCheckCount;
    result["root_rtl_name"] = parts->rootRtlName;
  }
  auto maps =
      acir::compiler::emitFinalSourceMaps(*package, memberships, emitError);
  if (failed(maps))
    return 1;
  llvm::json::Array rows;
  for (const auto &map : *maps)
    rows.emplace_back(
        llvm::json::Object{{"source", sourceOwner(map.sourceOwner)},
                           {"path", map.path},
                           {"text", map.text}});
  result["source_maps"] = std::move(rows);
  llvm::outs() << llvm::formatv("{0:2}\n",
                                llvm::json::Value(std::move(result)));
  return 0;
}
