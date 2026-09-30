#include "FinalRunnerEmission.h"
#include "FinalEmitCppSupport.h"
#include "FinalModelAbi.h"
#include "FinalRuntimeMetadata.h"
#include <utility>

namespace acir::compiler {
namespace {
std::string instancePath(const FinalProgram &program, size_t ordinal,
                         bool rtl) {
  const auto &instance = program.instances()[ordinal];
  if (!instance.parentOrdinal)
    return rtl ? "dut.root_" : "root";
  const auto &parent = program.instances()[*instance.parentOrdinal];
  auto position = llvm::find(parent.childOrdinals, ordinal);
  ac::InstanceOp placement = instance.placement;
  return instancePath(program, *instance.parentOrdinal, rtl) + "." +
         (rtl ? "child_" +
                    std::to_string(position - parent.childOrdinals.begin())
              : placement.getName().str());
}

size_t localOrdinal(ArrayRef<size_t> ordinals, size_t global) {
  return llvm::find(ordinals, global) - ordinals.begin();
}

FailureOr<std::string> slotExpression(const ObservationBinding &binding,
                                      StringRef value,
                                      ac::detail::EmitError emitError) {
  if (binding.values.size() > 1 ||
      binding.valueConstraints.size() != binding.values.size())
    return emitError()
           << "runner supports zero or one scalar observation value";
  if (binding.values.empty())
    return std::string("::gfsim::SlotValue::Unsigned(0)");
  auto constraint = dyn_cast<DictionaryAttr>(binding.valueConstraints[0]);
  auto logical =
      constraint ? constraint.getAs<DictionaryAttr>("type") : DictionaryAttr();
  auto kind = logical ? logical.getAs<StringAttr>("kind") : StringAttr();
  auto physical = dyn_cast<IntegerType>(binding.values[0].getType());
  if (!kind || !physical || physical.getWidth() > 64)
    return emitError() << "runner observation lacks a scalar logical type";
  if (kind.getValue() == "bool")
    return (Twine("::gfsim::SlotValue::Bool(") + value + " != 0)").str();
  auto interpretation = logical.getAs<StringAttr>("interpretation");
  if (kind.getValue() != "integer" || !interpretation)
    return emitError() << "runner observation has an unsupported logical type";
  if (interpretation.getValue() == "signed")
    return (Twine("::gfsim::SlotValue::Signed(SignExtend(") + value + ", " +
            Twine(physical.getWidth()) + "))")
        .str();
  return (Twine("::gfsim::SlotValue::Unsigned(") + value + ")").str();
}
} // namespace

FailureOr<FinalRunnerParts>
emitFinalRunnerPartsBody(const FinalProgram &program,
                         ac::detail::EmitError emitError) {
  for (const auto &instance : program.instances())
    if (!instance.staticArguments.empty() ||
        instance.module->hasAttr("ac.root_kind"))
      return emitError() << "migration runner requires a design root and empty "
                            "static arguments";
  auto descriptors = buildObservationDescriptors(program, emitError);
  if (failed(descriptors))
    return failure();
  FinalRunnerParts result;
  auto abi = buildFinalModelAbiParts();
  result.abiHeader = std::move(abi.header);
  result.abiSource = std::move(abi.source);
  llvm::raw_string_ostream metadata(result.metadataHeader);
  llvm::raw_string_ostream bridge(result.rtlBridge);
  llvm::raw_string_ostream adapter(result.rtlAdapter);
  metadata
      << "#pragma once\n#include <array>\n#include \"gfsim/SystemRunner.h\"\n"
      << "inline auto PycircuitRunnerMetadata() {\n"
      << "  ::std::array<::gfsim::RunnerObservation, " << descriptors->size()
      << "> entries{};\n";
  bridge << "module PycircuitRunnerBridge(input logic clk, input logic reset,\n"
            "  output wire permit";
  for (size_t i = 0; i < descriptors->size(); ++i)
    bridge << ",\n  output wire [63:0] value_" << i << ", output wire valid_"
           << i;
  for (size_t i = 0; i < program.checks().bindings.size(); ++i)
    bridge << ",\n  output wire check_failed_" << i;
  bridge << ");\n  FinalModel dut(.clk(clk), .reset(reset));\n"
            "  assign permit = dut.root_commit_ok;\n";
  adapter << "#pragma once\n#include \"VPycircuitRunnerBridge.h\"\n"
             "#include \"gfsim/SimSystem.h\"\n#include <array>\n#include "
             "<bit>\n#include <exception>\n"
             "class PycircuitRtlSystem final : public ::gfsim::SimSystem {\n"
             "  static ::std::int64_t SignExtend(::std::uint64_t bits, "
             "unsigned width) {\n"
             "    if (width < 64 && (bits & (UINT64_C(1) << (width - 1))))\n"
             "      bits |= (~UINT64_C(0)) << width;\n"
             "    return ::std::bit_cast<::std::int64_t>(bits);\n  }\n"
             "  class Device final : public ::gfsim::SimModule {\n"
             "  public:\n"
             "    explicit Device(PycircuitRtlSystem &owner) : "
             "SimModule(\"rtl\"), owner_(owner) {}\n"
             "  private:\n"
             "    void Build() override {}\n"
             "    void Work(::std::uint64_t epoch) override {\n"
             "      model_.clk = 0; model_.reset = 0; model_.eval();\n"
             "      permit_ = model_.permit; pending_ = true;\n"
             "      owner_.failure_ = {};\n";
  for (auto [global, check] : llvm::enumerate(program.checks().bindings)) {
    auto owner = llvm::find_if(program.instances(), [&](const auto &instance) {
      return instance.view == check.owner;
    });
    if (owner == program.instances().end())
      return emitError() << "runner check owner is missing";
    auto source = runtimeMetadataJson(check.location, emitError);
    auto id = runtimeMetadataJson(check.checkID, emitError);
    if (failed(source) || failed(id))
      return failure();
    bridge << "  assign check_failed_" << global << " = "
           << instancePath(program, owner->ordinal, true) << ".check_failed_"
           << localOrdinal(owner->checkOrdinals, global) << ";\n";
    adapter << "      if (model_.check_failed_" << global
            << " && owner_.failure_.code.empty()) owner_.failure_ = {"
            << "::gfsim::SimFailurePhase::Check, \"source_check_failed\", "
            << cppStringLiteral("source " + check.kind.getValue().str() +
                                " check failed")
            << ", " << cppStringLiteral(attrText(check.ownerRef)) << ", "
            << cppStringLiteral(*source) << ", " << cppStringLiteral(*id)
            << "};\n";
  }
  for (auto [slot, descriptor] : llvm::enumerate(*descriptors)) {
    const auto &binding =
        program.observations().bindings[descriptor.bindingIndex];
    const auto &owner = program.instances()[descriptor.ownerKey];
    auto spec = runtimeMetadataJson(binding.spec, emitError);
    auto site = binding.observationID.getAs<DictionaryAttr>("site");
    if (failed(spec) || !site)
      return failure();
    std::string index = std::to_string(slot);
    metadata << "  entries[" << slot
             << "].stableOrdinal = " << binding.stableOrdinal << ";\n";
    auto field = [&](StringRef name, StringRef text) {
      metadata << "  entries[" << slot << "]." << name << " = "
               << cppStringLiteral(text) << ";\n";
    };
    field("kind", binding.kind.getValue());
    field("instance", instancePath(program, owner.ordinal, false));
    field("registration", attrText(binding.registration));
    field("site", attrText(site));
    field("specJson", *spec);
    metadata << "  entries[" << slot
             << "].hasValue = " << (binding.values.empty() ? "false" : "true")
             << ";\n";
    if (descriptor.gauge) {
      auto name = binding.spec.getAs<StringAttr>("name");
      if (!name)
        return emitError() << "runner report lacks its validated name";
      field("reportName", name.getValue());
    }
    size_t local =
        localOrdinal(owner.observationOrdinals, descriptor.bindingIndex);
    bridge << "  assign value_" << slot << " = "
           << instancePath(program, owner.ordinal, true) << ".obs_value_"
           << local << ";\n  assign valid_" << slot << " = "
           << instancePath(program, owner.ordinal, true) << ".obs_path_"
           << local << ";\n";
    auto value = slotExpression(binding, "model_.value_" + index, emitError);
    if (failed(value))
      return failure();
    adapter << "      if (!owner_.observations_.Stage(" << slot << ", "
            << *value << ", true, model_.valid_" << slot
            << ", epoch)) permit_ = false;\n";
  }
  metadata << "  return entries;\n}\n";
  bridge << "endmodule\n";
  bool active = llvm::any_of(program.instances(), [](const auto &instance) {
    return !instance.ruleOrdinals.empty();
  });
  adapter
      << "    }\n"
         "    void Xfer() noexcept override {\n"
         "      if (!pending_ && !reset_) return;\n"
         "      model_.reset = reset_; model_.clk = 0; model_.eval();\n"
         "      model_.clk = 1; model_.eval(); model_.clk = 0; model_.eval();\n"
         "      model_.reset = 0; model_.eval(); reset_ = false; pending_ = "
         "false;\n"
         "    }\n"
         "    void Reset() noexcept override { reset_ = true; pending_ = "
         "false; }\n"
         "    void DiscardNext() noexcept override { pending_ = false; }\n"
         "    void ReportStat() override {}\n"
         "    bool HasWork() const noexcept override { return "
      << (active ? "true" : "false")
      << "; }\n"
         "    friend class PycircuitRtlSystem;\n"
         "    PycircuitRtlSystem &owner_;\n"
         "    ::VerilatedContext context_;\n"
         "    VPycircuitRunnerBridge model_{&context_};\n"
         "    bool permit_ = false, pending_ = false, reset_ = false;\n"
         "  };\npublic:\n"
         "  PycircuitRtlSystem() : device_(*this) {\n"
         "    const ::std::array<::gfsim::ObservationDescriptor, "
      << descriptors->size() << "> descriptors{{\n";
  for (const auto &descriptor : *descriptors) {
    const auto &binding =
        program.observations().bindings[descriptor.bindingIndex];
    adapter << "      {" << binding.stableOrdinal << ", " << descriptor.ownerKey
            << ", " << descriptor.registrationKey << ", " << descriptor.siteKey
            << ", ::gfsim::ObservationKind::"
            << (descriptor.gauge ? "Gauge" : "Event") << "},\n";
  }
  adapter
      << "    }};\n"
         "    if (!observations_.Configure(descriptors, descriptors.size()) "
         "||\n"
         "        !AttachObservations(observations_) || !AddModule(device_)) "
         "::std::terminate();\n"
         "  }\n"
         "  ::gfsim::ObservationSlots &Observations() noexcept { return "
         "observations_; }\n"
         "protected:\n"
         "  bool Precommit(::std::uint64_t) noexcept override { return "
         "device_.permit_; }\n"
         "  ::gfsim::SimFailureInfo DescribeFailure(::gfsim::SimFailurePhase "
         "phase) const noexcept override {\n"
         "    if (!failure_.code.empty()) return failure_;\n"
         "    return ::gfsim::SimSystem::DescribeFailure(phase);\n"
         "  }\nprivate:\n"
         "  ::gfsim::ObservationSlots observations_;\n"
         "  ::gfsim::SimFailureInfo failure_;\n"
         "  Device device_;\n};\n";
  result.mainSource = R"cpp(#include "gfsim/SystemRunner.h"
#include "runner_metadata.hpp"
#include <exception>
#include <iostream>
#ifdef PYCIRCUIT_RTL_RUNNER
#include "rtl_system.hpp"
using RunnerSystem = PycircuitRtlSystem;
#else
#include "pycircuit_system.hpp"
using RunnerSystem = FinalSystem;
#endif
int main(int argc, char **argv) {
  try {
    ::gfsim::SystemRunner runner(argc, argv);
    if (!runner.ready()) return 2;
    auto metadata = PycircuitRunnerMetadata();
    RunnerSystem model;
    return runner.Run(model, model.Observations(), metadata);
  } catch (const ::std::exception &error) {
    ::std::cerr << "pycircuit_system: " << error.what() << '\n';
    return 1;
  } catch (...) {
    ::std::cerr << "pycircuit_system: unexpected startup failure\n";
    return 1;
  }
}
)cpp";
  metadata.flush();
  bridge.flush();
  adapter.flush();
  return result;
}
} // namespace acir::compiler
