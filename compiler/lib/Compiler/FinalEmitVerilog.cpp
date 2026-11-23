#include "FinalCppNames.h"
#include "FinalEmitVerilog.h"
#include "HardwareEmitCommon.h"
#include "pycircuit/Dialect/ACIR/SourceUnitValidation.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/StringSet.h"
#include "llvm/Support/JSON.h"
#include "llvm/Support/SaveAndRestore.h"
#include "llvm/Support/raw_ostream.h"
#include <algorithm>
using namespace mlir;
namespace acir::compiler {
namespace {
LogicalResult emitEnum(HardwareEmitContext &context, ac::EnumOp declaration,
                       raw_ostream &out) {
  auto type =
      ac::EnumType::get(declaration.getContext(), declaration.getSymNameAttr());
  auto definition = context.analysis.resolveEnum(type, declaration);
  auto name = context.rtlType(type, declaration);
  if (failed(definition) || failed(name))
    return failure();
  out << "typedef logic [" << definition->width << "-1:0] "
      << StringRef(*name).rsplit("::").second << ";\n";
  return success();
}
LogicalResult emitRecord(HardwareEmitContext &context, ac::StructOp record,
                         raw_ostream &out) {
  auto name = context.rtlType(
      ac::StructType::get(record.getContext(), record.getSymNameAttr()),
      record);
  if (failed(name))
    return failure();
  out << "typedef struct packed {\n";
  for (auto raw : record.getFields()) {
    auto field = cast<DictionaryAttr>(raw);
    auto fieldType = field.getAs<TypeAttr>("type").getValue();
    auto type = context.rtlType(fieldType, record);
    auto member = legalizeIdentifier(field.getAs<StringAttr>("name").getValue(),
                                     [&] { return record.emitOpError(); });
    if (failed(type) || failed(member))
      return failure();
    // All record typedefs share this package; nested members use local names.
    // Keep qualification on uses outside the package, such as module ports.
    StringRef spelling = *type;
    if (isa<ac::StructType, ac::EnumType>(fieldType))
      spelling = spelling.rsplit("::").second;
    out << "  " << spelling << " " << *member << ";\n";
  }
  out << "} " << StringRef(*name).rsplit("::").second << ";\n";
  return success();
}
LogicalResult emitTypedef(HardwareEmitContext &context, Operation *declaration,
                          llvm::DenseSet<Operation *> &emitted,
                          RtlPackageDependencies &package, raw_ostream &out) {
  if (!emitted.insert(declaration).second)
    return success();
  RtlPackageDependencies dependencies;
  std::string text;
  {
    // Collect dependencies of the rendered fields before writing this typedef.
    llvm::SaveAndRestore recording(context.rtlPackage, &dependencies);
    llvm::raw_string_ostream body(text);
    auto enumeration = dyn_cast<ac::EnumOp>(declaration);
    if (failed(enumeration
                   ? emitEnum(context, enumeration, body)
                   : emitRecord(context, cast<ac::StructOp>(declaration), body)))
      return failure();
  }
  package.floorDiv |= dependencies.floorDiv;
  package.mod |= dependencies.mod;
  for (auto *type : dependencies.types)
    if (failed(emitTypedef(context, type, emitted, package, out)))
      return failure();
  out << text;
  return success();
}
LogicalResult emitTypePackage(HardwareEmitContext &context,
                              RtlPackageDependencies &dependencies,
                              raw_ostream &out) {
  if (dependencies.types.empty() && !dependencies.floorDiv && !dependencies.mod)
    return success();
  std::string typedefs;
  llvm::raw_string_ostream types(typedefs);
  llvm::DenseSet<Operation *> emitted;
  for (auto *type : dependencies.types)
    if (failed(emitTypedef(context, type, emitted, dependencies, types)))
      return failure();
  out << "package pycircuit_types;\n";
  if (dependencies.floorDiv)
    out << "  function automatic logic signed [63:0] pyc_floordiv(input logic "
           "signed [63:0] a, b);\n"
           "    pyc_floordiv = a/b - ((a%b != 0 && (a<0)!=(b<0)) ? 64'sd1 : "
           "64'sd0);\n"
           "  endfunction\n";
  if (dependencies.mod)
    out << "  function automatic logic signed [63:0] pyc_mod(input logic "
           "signed [63:0] a, b);\n"
           "    logic signed [63:0] r;\n"
           "    r = a%b;\n"
           "    pyc_mod = r != 0 && (r<0)!=(b<0) ? r+b : r;\n"
           "  endfunction\n";
  out << typedefs << "endpackage\n";
  return success();
}
FailureOr<ac::ModuleOp> rootModule(HardwareEmitContext &context) {
  auto systems = context.package.getOps<ac::SystemOp>();
  if (!llvm::hasSingleElement(systems))
    return context.package.emitOpError() << "one system required";
  auto system = *systems.begin();
  auto symbol = system.getEntry().getAs<FlatSymbolRefAttr>("callee");
  auto root = symbol ? dyn_cast_or_null<ac::ModuleOp>(
                           context.analysis.lookupDefinition(symbol.getValue()))
                     : ac::ModuleOp();
  if (!root)
    return system.emitOpError() << "root module is unresolved";
  return root;
}
struct SystemControls {
  std::string clock, reset;
  SmallVector<std::string> sampled;
};
FailureOr<std::string> managedReset(HardwareEmitContext &context,
                                    ac::ModuleOp root) {
  auto domain = root->getAttrOfType<DictionaryAttr>("ac.domain_inputs");
  if (!domain || domain.empty())
    return std::string("1'b0");
  auto reset = domain.getAs<IntegerAttr>("reset");
  auto layout = ac::getModulePortLayout(root);
  if (failed(layout))
    return failure();
  auto names =
      root.getInputNames().getValue().take_front(layout->sourceInputCount);
  if (!reset || reset.getValue().getActiveBits() > 64 ||
      reset.getValue().getZExtValue() >= names.size())
    return root.emitOpError()
           << "managed module requires verified reset context";
  auto name = legalizeIdentifier(
      cast<StringAttr>(names[reset.getValue().getZExtValue()]).getValue(),
      [&] { return root.emitOpError(); });
  if (failed(name))
    return failure();
  return context.rtlReset(*name);
}
FailureOr<SystemControls> systemControls(ac::ModuleOp root) {
  auto sampled = ac::getSystemSampleInputs(root);
  if (failed(sampled))
    return failure();
  if (!sampled->empty()) {
    SystemControls result;
    for (unsigned index : *sampled) {
      auto name = legalizeIdentifier(
          cast<StringAttr>(root.getInputNames()[index]).getValue(),
          [&] { return root.emitOpError(); });
      if (failed(name))
        return failure();
      result.sampled.push_back(*name);
    }
    return result;
  }
  auto domain = root->getAttrOfType<DictionaryAttr>("ac.domain_inputs");
  auto clock = domain ? domain.getAs<IntegerAttr>("clock") : IntegerAttr();
  auto reset = domain ? domain.getAs<IntegerAttr>("reset") : IntegerAttr();
  auto layout = ac::getModulePortLayout(root);
  if (failed(layout))
    return failure();
  auto inputs =
      root.getInputNames().getValue().take_front(layout->sourceInputCount);
  if (!domain || domain.size() != 2 || !clock || !reset ||
      clock.getValue().getActiveBits() > 64 ||
      reset.getValue().getActiveBits() > 64)
    return root.emitOpError()
           << "system root requires verified clock/reset domains";
  uint64_t clockIndex = clock.getValue().getZExtValue();
  uint64_t resetIndex = reset.getValue().getZExtValue();
  if (clockIndex == resetIndex || inputs.size() != 2 ||
      clockIndex >= inputs.size() || resetIndex >= inputs.size() ||
      layout->sourceOutputCount != 0)
    return root.emitOpError()
           << "system root must be closed except for clock/reset";
  auto legal = [&](uint64_t index) {
    return legalizeIdentifier(cast<StringAttr>(inputs[index]).getValue(),
                              [&] { return root.emitOpError(); });
  };
  auto clockName = legal(clockIndex), resetName = legal(resetIndex);
  if (failed(clockName) || failed(resetName))
    return failure();
  return SystemControls{std::move(*clockName), std::move(*resetName), {}};
}
struct ObservationAddress {
  std::string object;
  size_t localIndex = 0;
};
FailureOr<ObservationAddress>
observationHierarchy(const ObservationEmissionPlan &plan,
                     const ObservationEmissionOccurrence &occurrence) {
  std::string result = "dut.dut";
  for (auto [operation, lane] : occurrence.path) {
    auto raw = operation->getAttrOfType<StringAttr>("instance_name");
    if (!raw)
      return operation->emitOpError() << "instance has no name";
    auto name = legalizeIdentifier(raw.getValue(),
                                   [&] { return operation->emitOpError(); });
    if (failed(name))
      return failure();
    if (isa<ac::CollectionOp>(operation))
      result += ".pyc_instances_" + *name + "[" + std::to_string(lane) + "]";
    result += ".pyc_instance_" + *name;
  }
  auto owner = occurrence.site.op->getParentOfType<ac::ModuleOp>();
  auto sites = plan.definitions.find(owner);
  if (sites == plan.definitions.end())
    return occurrence.site.op->emitOpError()
           << "observation has no definition layout";
  auto found =
      llvm::find_if(sites->second, [&](const ObservationEmissionSite &site) {
        return site.op == occurrence.site.op &&
               site.valueIndex == occurrence.site.valueIndex;
      });
  if (found == sites->second.end())
    return occurrence.site.op->emitOpError()
           << "observation has no local layout slot";
  return ObservationAddress{std::move(result),
                            static_cast<size_t>(found - sites->second.begin())};
}
FailureOr<std::string> systemSimulationTop(HardwareEmitContext &context,
                                           ac::ModuleOp root,
                                           StringRef wrapper) {
  auto controls = systemControls(root);
  auto plan = observationEmissionPlan(context);
  if (failed(controls) || failed(plan))
    return failure();
  auto samples = ac::getSystemSamples(root);
  if (failed(samples))
    return failure();
  std::string scheduleDeclaration, scheduleSteps, cycleGuard;
  auto controlNames =
      controls->sampled.empty()
          ? SmallVector<std::string>{controls->clock, controls->reset}
          : controls->sampled;
  auto controlWidth = std::to_string(controlNames.size());
  if (!samples->empty()) {
    std::string pattern;
    for (Attribute row : *samples)
      pattern += cast<StringAttr>(row).getValue().str();
    scheduleDeclaration =
        "  localparam logic [" + std::to_string(pattern.size() - 1) +
        ":0] pyc_samples = " + std::to_string(pattern.size()) + "'b" + pattern +
        ";\n";
    cycleGuard =
        "    if ($test$plusargs(\"cycles\")) $fatal(1, \"pycircuit_sim: "
        "explicit cycles conflicts with samples\");\n";
    scheduleSteps =
        "    for (pyc_cycle=0; pyc_cycle<" + std::to_string(samples->size()) +
        "; pyc_cycle=pyc_cycle+1) begin\n      pyc_step(pyc_samples[(" +
        std::to_string(samples->size()) + "-1-pyc_cycle)*" + controlWidth +
        " +: " + controlWidth + "]);\n    end\n";
  } else {
    scheduleSteps =
        "    for (pyc_cycle=0; pyc_cycle<pyc_cycles; pyc_cycle=pyc_cycle+1) "
        "begin\n"
        "      pyc_step(2'b00);\n      pyc_step(2'b10);\n    end\n";
  }
  struct ReportOutput {
    uint64_t ordinal;
    std::string instance, name;
  };
  SmallVector<ReportOutput> reports;
  for (const auto &occurrence : plan->occurrences) {
    if (!occurrence.site.gauge)
      continue;
    auto observe = cast<ac::SourceObserveOp>(occurrence.site.op);
    auto name = observe.getSpec().getAs<StringAttr>("name");
    if (!name)
      return observe.emitOpError() << "report observation has no name";
    std::string instance = "root";
    for (auto [operation, lane] : occurrence.logicalOwnerPath) {
      auto allocation = operation->getAttrOfType<StringAttr>("instance_name");
      if (!allocation)
        return operation->emitOpError() << "instance has no name";
      instance += "/" + allocation.getValue().str();
      if (isa<ac::CollectionOp>(operation))
        instance += "[" + std::to_string(lane) + "]";
    }
    reports.push_back(
        {occurrence.ordinal, std::move(instance), name.getValue().str()});
  }
  llvm::sort(reports, [](const ReportOutput &left, const ReportOutput &right) {
    return std::tie(left.instance, left.name) <
           std::tie(right.instance, right.name);
  });
  std::string text;
  llvm::raw_string_ostream out(text);
  out << "module pycircuit_sim;\n"
         "  logic ["
      << controlNames.size() - 1
      << ":0] pyc_controls = '0;\n"
         "  logic [2:0] pyc_phase = 3'd0;\n"
         "  logic pyc_root_commit_ok = 1'b0;\n"
         "  wire pyc_check_complete;\n"
         "  wire pyc_sampling_error;\n"
         "  wire pyc_local_error;\n"
         "  localparam longint unsigned pyc_max_cycles = "
         "64'd4611686018427387903;\n"
         "  longint unsigned pyc_cycles = 64'd10;\n"
         "  longint unsigned pyc_epoch = 64'd0;\n"
         "  longint unsigned pyc_cycle;\n"
         "  string pyc_cycles_text;\n"
         "  bit pyc_cycles_valid;\n  "
      << wrapper << " dut (\n";
  for (auto [index, name] : llvm::enumerate(controlNames))
    out << "    ." << name << "(pyc_controls["
        << controlNames.size() - 1 - index << "]),\n";
  out << "    .pyc_phase(pyc_phase),\n"
         "    .pyc_root_commit_ok(pyc_root_commit_ok),\n"
         "    .pyc_check_complete(pyc_check_complete),\n"
         "    .pyc_sampling_error(pyc_sampling_error),\n"
         "    .pyc_local_error(pyc_local_error)\n  );\n";
  for (const auto &report : reports)
    out << "  longint unsigned pyc_report_value_" << report.ordinal
        << " = 0;\n  longint unsigned pyc_report_update_" << report.ordinal
        << " = 0;\n";
  out << "  task automatic pyc_parse_cycles(\n"
         "      input string text, output bit accepted,\n"
         "      output longint unsigned value);\n"
         "    integer index;\n"
         "    integer character;\n"
         "    longint unsigned digit;\n"
         "    begin\n"
         "      value = 0;\n"
         "      accepted = text.len() != 0;\n"
         "      for (index = 0; accepted && index < text.len();\n"
         "           index = index + 1) begin\n"
         "        character = text[index];\n"
         "        if (character < 48 || character > 57)\n"
         "          accepted = 1'b0;\n"
         "        else begin\n"
         "          digit = character - 48;\n"
         "          if (value > (pyc_max_cycles - digit) / 10)\n"
         "            accepted = 1'b0;\n"
         "          else\n"
         "            value = value * 10 + digit;\n"
         "        end\n"
         "      end\n"
         "      accepted = accepted && (value != 0);\n"
         "    end\n"
         "  endtask\n";
  struct ObservationGroup {
    size_t begin, size;
    SmallVector<ObservationAddress> addresses;
    SmallVector<uint64_t> widths;
  };
  SmallVector<ObservationGroup> groups;
  for (size_t begin = 0; begin < plan->occurrences.size();) {
    const auto &first = plan->occurrences[begin];
    auto observe = cast<ac::SourceObserveOp>(first.site.op);
    const size_t size = std::max<size_t>(1, observe.getValues().size());
    if (first.site.valueIndex != 0 || size > plan->occurrences.size() - begin ||
        (first.site.gauge && size != 1))
      return observe.emitOpError() << "observation group layout is incomplete";
    ObservationGroup group{begin, size, {}, {}};
    for (size_t index = 0; index < size; ++index) {
      const auto &lane = plan->occurrences[begin + index];
      if (lane.site.op != first.site.op || lane.path != first.path ||
          lane.site.valueIndex != index ||
          lane.site.carriesValue != !observe.getValues().empty() ||
          lane.site.gauge != first.site.gauge)
        return observe.emitOpError()
               << "observation group layout is inconsistent";
      auto hierarchy = observationHierarchy(*plan, lane);
      auto width = observationPayloadWidth(context, lane);
      if (failed(hierarchy) || failed(width))
        return failure();
      group.addresses.push_back(std::move(*hierarchy));
      group.widths.push_back(*width);
    }
    groups.push_back(std::move(group));
    begin += size;
  }
  auto signal = [](const ObservationAddress &address, StringRef field) {
    return address.object + ".pyc_observation_" + field.str() + "_" +
           std::to_string(address.localIndex);
  };
  out << "  bit pyc_observation_groups_ok;\n";
  for (const auto &group : groups)
    out << "  bit pyc_observation_active_" << group.begin << " = 1'b0;\n";
  out << "  task automatic pyc_validate_observations(output bit accepted);\n"
         "    begin\n"
         "      accepted = 1'b1;\n";
  // Validate every frozen group before granting commit permission. A later
  // partial group cannot commit hardware or publish an earlier group's prefix.
  for (const auto &group : groups) {
    const auto &first = group.addresses[0];
    out << "      pyc_observation_active_" << group.begin << " = 1'b0;\n";
    for (const auto &address : group.addresses)
      out << "      if ((" << signal(address, "pending") << " !== 1'b0) && ("
          << signal(address, "pending") << " !== 1'b1)) accepted = 1'b0;\n";
    for (size_t index = 1; index < group.size; ++index)
      out << "      if (" << signal(group.addresses[index], "pending")
          << " !== " << signal(first, "pending") << ") accepted = 1'b0;\n";
    out << "      if (" << signal(first, "pending") << " === 1'b1) begin\n";
    for (const auto &address : group.addresses) {
      for (StringRef field : {"path", "valid"})
        out << "        if ((" << signal(address, field) << " !== 1'b0) && ("
            << signal(address, field) << " !== 1'b1)) accepted = 1'b0;\n";
    }
    for (size_t index = 1; index < group.size; ++index)
      out << "        if (" << signal(group.addresses[index], "path")
          << " !== " << signal(first, "path") << ") accepted = 1'b0;\n";
    // Known false paths suppress the whole group even when payload validity
    // differs. On a true path only all-invalid or all-valid groups are legal.
    out << "        if (" << signal(first, "path") << " === 1'b1) begin\n";
    for (size_t index = 1; index < group.size; ++index)
      out << "          if (" << signal(group.addresses[index], "valid")
          << " !== " << signal(first, "valid") << ") accepted = 1'b0;\n";
    if (plan->occurrences[group.begin].site.gauge)
      out << "          if ($isunknown(" << signal(first, "value")
          << ")) accepted = 1'b0;\n";
    out << "          if (" << signal(first, "valid") << " === 1'b1)\n"
        << "            pyc_observation_active_" << group.begin << " = 1'b1;\n"
        << "        end\n"
        << "      end\n";
  }
  out << "    end\n  endtask\n";
  llvm::DenseSet<uint64_t> formattedWidths;
  for (const auto &group : groups) {
    for (uint64_t width : group.widths) {
      if (!formattedWidths.insert(width).second)
        continue;
      uint64_t padded = (width / 4 + (width % 4 != 0)) * 4;
      out << "  function automatic string pyc_observation_json_" << width
          << "(input logic [" << width - 1 << ":0] raw, input bit as_bool);\n"
          << "    bit [" << padded - 1
          << ":0] value_plane, known_plane, z_plane;\n"
          << "    longint unsigned index;\n    begin\n";
      if (width <= 64)
        out << "      if (!$isunknown(raw)) begin\n"
            << "        if (as_bool) pyc_observation_json_" << width
            << " = raw ? \"{\\\"kind\\\":\\\"bool\\\",\\\"value\\\":true}\" : "
               "\"{\\\"kind\\\":\\\"bool\\\",\\\"value\\\":false}\";\n"
            << "        else pyc_observation_json_" << width
            << " = "
               "$sformatf(\"{\\\"kind\\\":\\\"integer\\\",\\\"value\\\":\\\"%"
               "0d\\\"}\", $unsigned(raw));\n"
            << "      end else begin\n";
      out << "      value_plane = '0; known_plane = '0; z_plane = '0;\n"
          << "      for (index = 0; index < " << width
          << "; index = index + 1) begin\n"
          << "        case (raw[index])\n"
          << "          1'b0: known_plane[index] = 1'b1;\n"
          << "          1'b1: begin value_plane[index] = 1'b1; "
             "known_plane[index] = 1'b1; end\n"
          << "          1'bz: z_plane[index] = 1'b1;\n"
          << "          default: begin end\n"
          << "        endcase\n      end\n"
          << "      pyc_observation_json_" << width
          << " = $sformatf(\"{\\\"kind\\\":\\\"bits4\\\",\\\"width\\\":"
          << width
          << ",\\\"value\\\":\\\"%h\\\",\\\"known\\\":\\\"%h\\\",\\\"z\\\":"
             "\\\"%h\\\"}\", value_plane, known_plane, z_plane);\n";
      if (width <= 64)
        out << "      end\n";
      out << "    end\n  endfunction\n";
    }
  }
  out << "  task automatic pyc_publish_observations;\n    begin\n";
  for (const auto &group : groups) {
    const auto &occurrence = plan->occurrences[group.begin];
    auto observe = cast<ac::SourceObserveOp>(occurrence.site.op);
    auto identity = observe->getAttrOfType<DictionaryAttr>("ac.observation_id");
    if (!identity)
      return observe.emitOpError() << "observation identity is missing";
    auto registration =
        emissionMetadataJson(identity.get("registration"), observe);
    auto site = observationSiteMetadataJson(observe);
    auto spec = emissionMetadataJson(observe.getSpec(), observe);
    if (failed(registration) || failed(site) || failed(spec))
      return failure();
    std::string instance = "root";
    for (auto [operation, lane] : occurrence.logicalOwnerPath) {
      auto name = operation->getAttrOfType<StringAttr>("instance_name");
      instance += "/" + name.getValue().str();
      if (isa<ac::CollectionOp>(operation))
        instance += "[" + std::to_string(lane) + "]";
    }
    // Escape authored metadata before inserting format slots. No source text
    // is reserved as a placeholder: event names and literals may contain it.
    std::string record =
        displayLiteral("{\"kind\":" + jsonString(observe.getKind()) +
                       ",\"instance\":" + jsonString(instance) +
                       ",\"registration\":" + jsonString(*registration) +
                       ",\"site\":" + jsonString(*site) +
                       ",\"evaluation_epoch\":\"") +
        "%0d" + displayLiteral("\",\"commit_epoch\":\"") + "%0d" +
        displayLiteral("\",\"spec\":" + *spec + ",\"values\":[");
    std::string arguments;
    for (size_t index = 0; index < group.size; ++index) {
      if (!occurrence.site.carriesValue)
        continue;
      if (index)
        record += ',';
      const std::string value = signal(group.addresses[index], "value");
      record += "%s";
      arguments +=
          ", pyc_observation_json_" + std::to_string(group.widths[index]) +
          "(" + value + ", " +
          (group.widths[index] == 1 && !occurrence.site.gauge ? "1'b1"
                                                              : "1'b0") +
          ")";
    }
    record += "]}";
    out << "      if (pyc_observation_active_" << group.begin << ") begin\n";
    if (occurrence.site.gauge) {
      const std::string value = signal(group.addresses[0], "value");
      out << "        pyc_report_value_" << occurrence.ordinal
          << " = $unsigned(" << value << ");\n"
          << "        pyc_report_update_" << occurrence.ordinal
          << " = pyc_epoch + 1;\n";
    }
    out << "        $display(\"" << record << "\", pyc_epoch, pyc_epoch + 1"
        << arguments << ");\n"
        << "      end\n";
  }
  out << "    end\n  endtask\n"
      << scheduleDeclaration << "  task automatic pyc_step(input logic ["
      << controlNames.size() - 1
      << ":0] levels);\n"
         "    begin\n"
         "      pyc_controls = levels;\n"
         "      pyc_root_commit_ok = 1'b0;\n"
         "      #1; // Settle combinational pins before the shared PREPARE "
         "edge.\n"
         "      pyc_phase = 3'd1; #1;\n"
         "      if (pyc_check_complete !== 1'b1) begin\n"
         "        pyc_phase = 3'd3; #1;\n"
         "        $fatal(1, \"pycircuit_sim: incomplete check snapshot "
         "at epoch %0d\", pyc_epoch);\n"
         "      end\n"
         "      if (pyc_sampling_error !== 1'b0) begin\n"
         "        pyc_phase = 3'd3; #1;\n"
         "        $fatal(1, \"pycircuit_sim: physical sampling check failed "
         "at epoch %0d\", pyc_epoch);\n"
         "      end\n"
         "      if (pyc_local_error !== 1'b0) begin\n"
         "        pyc_phase = 3'd3; #1;\n"
         "        $fatal(1, \"pycircuit_sim: source or primitive check failed "
         "at epoch %0d\", pyc_epoch);\n"
         "      end\n"
         "      pyc_validate_observations(pyc_observation_groups_ok);\n"
         "      if (!pyc_observation_groups_ok) begin\n"
         "        pyc_root_commit_ok = 1'b0;\n"
         "        pyc_phase = 3'd3; #1;\n"
         "        $fatal(1, \"pycircuit_sim: inconsistent observation group "
         "at epoch %0d\", pyc_epoch);\n"
         "      end\n"
         "      pyc_root_commit_ok = 1'b1;\n"
         "      pyc_phase = 3'd2; #1;\n"
         "      pyc_publish_observations();\n"
         "      pyc_phase = 3'd0;\n"
         "      pyc_root_commit_ok = 1'b0;\n"
         "      pyc_epoch = pyc_epoch + 1;\n"
         "    end\n  endtask\n"
         "  initial begin\n"
      << cycleGuard
      << "    if ($value$plusargs(\"cycles=%s\", pyc_cycles_text)) begin\n"
         "      pyc_parse_cycles(pyc_cycles_text, pyc_cycles_valid, "
         "pyc_cycles);\n"
         "      if (!pyc_cycles_valid)\n"
         "        $fatal(1, \"pycircuit_sim: +cycles requires a positive "
         "decimal value no greater than 4611686018427387903\");\n"
         "    end else if ($test$plusargs(\"cycles\"))\n"
         "      $fatal(1, \"pycircuit_sim: +cycles requires a positive decimal "
         "value no greater than 4611686018427387903\");\n"
         "    pyc_controls = '0;\n"
         "    #1; // Reset initializers settle before RESET_PREPARE.\n"
         "    pyc_root_commit_ok = 1'b1; pyc_phase = 3'd4; #1;\n"
         "    pyc_phase = 3'd2; #1;\n"
         "    pyc_phase = 3'd0; pyc_root_commit_ok = 1'b0;\n"
      << scheduleSteps
      << "    "
         "$write(\"{\\\"reset_semantics\\\":\\\""
      << (context.resetIfOne() ? "if_one" : "strict")
      << "\\\",\\\"kind\\\":\\\"result\\\",\\\"status\\\":"
         "\\\"TERMINATED\\\",\\\"epoch_time\\\":\\\"%0d\\\",\\\"statistics\\\":"
         "[\", pyc_epoch);\n"
         "    "
         "$write(\"{\\\"buckets\\\":[],\\\"count\\\":0,\\\"kind\\\":"
         "\\\"counter\\\",\\\"last_update\\\":{\\\"delta\\\":0,\\\"time\\\":%"
         "0d},\\\"maximum\\\":0,\\\"minimum\\\":0,\\\"name\\\":\\\"cycles\\\","
         "\\\"object_path\\\":\\\"@runtime\\\",\\\"sum\\\":0,\\\"value\\\":%0d}"
         "\", pyc_epoch, pyc_epoch);\n"
         "    "
         "$write(\",{\\\"buckets\\\":[],\\\"count\\\":0,\\\"kind\\\":"
         "\\\"gauge\\\",\\\"last_update\\\":{\\\"delta\\\":0,\\\"time\\\":0},"
         "\\\"maximum\\\":0,\\\"minimum\\\":0,\\\"name\\\":\\\"stop_reason\\\","
         "\\\"object_path\\\":\\\"@runtime\\\",\\\"sum\\\":0,\\\"value\\\":0}"
         "\");\n";
  for (const auto &report : reports) {
    std::string row =
        displayLiteral(",{\"buckets\":[],\"count\":0,\"kind\":\"gauge\",\"last_"
                       "update\":{\"delta\":0,\"time\":") +
        "%0d" +
        displayLiteral("},\"maximum\":0,\"minimum\":0,\"name\":" +
                       jsonString(report.name) + ",\"object_path\":" +
                       jsonString(report.instance) + ",\"sum\":0,\"value\":") +
        "%0d}";
    out << "    $write(\"" << row << "\", pyc_report_update_" << report.ordinal
        << ", pyc_report_value_" << report.ordinal << ");\n";
  }
  out << "    $display(\"],\\\"error\\\":null}\");\n"
         "    $finish;\n  end\nendmodule\n";
  out.flush();
  return text;
}
// Simulation-only finite event batches. Each detector owns its NBA slot;
// the coordinator never performs a shared detector read/modify/write.
LogicalResult emitPhysicalBatchCoordinator(HardwareEmitContext &context,
                                           ac::ModuleOp root,
                                           raw_ostream &out) {
  auto bindings = context.rootBindings();
  if (failed(bindings))
    return failure();
  auto layout = ac::getModulePortLayout(root);
  if (failed(layout))
    return failure();
  const auto &projection = context.physicalProjection();
  SmallVector<unsigned> events(projection.clockInputs);
  llvm::append_range(events, projection.resetInputs);
  llvm::sort(events);
  events.erase(std::unique(events.begin(), events.end()), events.end());
  out << "  // Finite batches: payload settles first; each root clock/reset\n"
         "  // changes at most once in one update region, then stays stable\n"
         "  // through delta drain. Observe in the read-only region.\n"
         "  // Initial root clocks must all be known zero.\n"
         "  wire pyc_check_complete, pyc_sampling_error, pyc_local_error;\n"
         "`ifndef SYNTHESIS\n"
         "  logic [2:0] pyc_phase = 3'd0;\n"
         "  bit pyc_root_commit_ok = 1'b0;\n"
         "  bit pyc_physical_fault = 1'b0;\n"
         "  bit pyc_initial_batch = 1'b1;\n"
         "  bit pyc_event_seal = 1'b0;\n"
         "  bit pyc_batch_pending = 1'b0;\n"
         "  initial begin\n"
         "    pyc_batch_pending = 1'b1;\n"
         "    pyc_event_seal <= 1'b1; // establish initial clock histories\n"
         "  end\n";
  for (unsigned index : events) {
    auto name = legalizeIdentifier(
        cast<StringAttr>(root.getInputNames()[index]).getValue(),
        [&] { return root.emitOpError(); });
    if (failed(name))
      return failure();
    out << "  bit pyc_event_slot_" << index << " = 1'b0;\n"
        << "  always @(" << *name << ") pyc_event_slot_" << index
        << " <= ~pyc_event_slot_" << index << ";\n";
  }
  for (unsigned index = 0; index < layout->sourceInputCount; ++index) {
    auto type = context.analysis.resolveType(
        root.getFunctionType().getInput(index), *bindings, root);
    if (failed(type))
      return failure();
    auto rendered = context.rtlType(*type, root);
    if (failed(rendered))
      return failure();
    out << "  " << *rendered << " pyc_frozen_input_" << index << ";\n";
  }
  out << "  wire pyc_physical_setup_error = pyc_initial_batch && (1'b0";
  for (unsigned clock : projection.clockInputs)
    out << " || (pyc_frozen_input_" << clock << " !== 1'b0)";
  out << ");\n"
         "  wire pyc_physical_error = pyc_physical_setup_error |\n"
         "      (pyc_check_complete !== 1'b1) |\n"
         "      (pyc_sampling_error !== 1'b0) | (pyc_local_error !== 1'b0)";
  for (unsigned clock : projection.clockInputs)
    out << " | $isunknown(pyc_frozen_input_" << clock << ")";
  out << ";\n";
  if (!events.empty()) {
    out << "  always @(";
    for (auto [index, event] : llvm::enumerate(events))
      out << (index ? " or " : "") << "pyc_event_slot_" << event;
    out << ") begin\n"
           "    if (!pyc_physical_fault && !pyc_batch_pending) begin\n"
           "      pyc_batch_pending = 1'b1;\n"
           "      pyc_event_seal <= ~pyc_event_seal;\n"
           "    end\n"
           "  end\n";
  }
  out << "  always @(pyc_event_seal) begin\n"
         "    if (pyc_batch_pending && !pyc_physical_fault) begin\n";
  for (unsigned index = 0; index < layout->sourceInputCount; ++index) {
    auto name = legalizeIdentifier(
        cast<StringAttr>(root.getInputNames()[index]).getValue(),
        [&] { return root.emitOpError(); });
    if (failed(name))
      return failure();
    out << "      pyc_frozen_input_" << index << " = " << *name << ";\n";
  }
  out << "      pyc_root_commit_ok <= 1'b0;\n"
         "      pyc_phase <= 3'd1;\n"
         "    end\n"
         "  end\n"
         "  always @(pyc_phase) begin\n"
         "    case (pyc_phase)\n"
         "      3'd1: pyc_phase <= 3'd5; // preparation NBA barrier\n"
         "      3'd5: pyc_phase <= 3'd6; // completion/error reduction "
         "barrier\n"
         "      3'd6: begin\n"
         "        if (pyc_physical_error) begin\n"
         "          pyc_physical_fault <= 1'b1;\n"
         "          pyc_root_commit_ok <= 1'b0;\n"
         "          pyc_phase <= 3'd3;\n"
         "          $display(\"PYCIRCUIT_PHYSICAL_FAULT\");\n"
         "          pyc_report_physical_fault($sformatf(\"%m\"));\n"
         "        end else begin\n"
         "          pyc_root_commit_ok <= 1'b1;\n"
         "          pyc_phase <= 3'd2;\n"
         "        end\n"
         "      end\n"
         "      3'd2: begin\n"
         "        pyc_initial_batch <= 1'b0;\n"
         "        pyc_phase <= 3'd7; // successful storage NBA drain\n"
         "      end\n"
         "      3'd3: pyc_phase <= 3'd7; // discard drain\n"
         "      3'd7: begin\n"
         "        pyc_root_commit_ok <= 1'b0;\n"
         "        pyc_batch_pending <= 1'b0;\n"
         "        pyc_phase <= 3'd0;\n"
         "      end\n"
         "      default: begin end\n"
         "    endcase\n"
         "  end\n"
         "`else\n"
         "  wire [2:0] pyc_phase = 3'd0;\n"
         "  wire pyc_root_commit_ok = 1'b1;\n"
         "`endif\n";
  return success();
}
LogicalResult emitWrapper(HardwareEmitContext &context, ac::ModuleOp root,
                          StringRef wrapper, StringRef callee,
                          raw_ostream &out) {
  out << "module " << wrapper << " (";
  auto bindings = context.rootBindings();
  if (failed(bindings))
    return failure();
  auto signature = root.getFunctionType();
  auto layout = ac::getModulePortLayout(root);
  if (failed(layout))
    return failure();
  bool first = true;
  auto port = [&](StringRef direction, Type type,
                  StringRef name) -> LogicalResult {
    auto resolved = context.analysis.resolveType(type, *bindings, root);
    if (failed(resolved))
      return failure();
    auto rendered = context.rtlType(*resolved, root);
    auto id = legalizeIdentifier(name, [&] { return root.emitOpError(); });
    if (failed(rendered) || failed(id))
      return failure();
    out << (first ? "\n" : ",\n") << "  " << direction << " wire " << *rendered
        << " " << *id;
    first = false;
    return success();
  };
  for (auto [i, type] : llvm::enumerate(
           signature.getInputs().take_front(layout->sourceInputCount)))
    if (failed(port("input", type,
                    cast<StringAttr>(root.getInputNames()[i]).getValue())))
      return failure();
  for (auto [i, type] : llvm::enumerate(
           signature.getResults().take_front(layout->sourceOutputCount)))
    if (failed(port("output", type,
                    cast<StringAttr>(root.getOutputNames()[i]).getValue())))
      return failure();
  for (int64_t companion : layout->outputValidity)
    if (companion >= 0 &&
        failed(port(
            "output", signature.getResult(companion),
            cast<StringAttr>(root.getOutputNames()[companion]).getValue())))
      return failure();
  if (context.managesChecks() && !context.batchesPhysicalEvents()) {
    out << (first ? "\n" : ",\n")
        << "  input wire [2:0] pyc_phase,\n"
           "  input wire pyc_root_commit_ok,\n"
           "  output wire pyc_check_complete,\n"
           "  output wire pyc_sampling_error,\n"
           "  output wire pyc_local_error";
    first = false;
  }
  out << (first ? ");\n" : "\n);\n");
  auto parameters = callee == context.names(root).rtl
                        ? context.rootRtlParameters()
                        : FailureOr<std::string>(std::string());
  if (failed(parameters))
    return failure();
  if (context.batchesPhysicalEvents()) {
    if (failed(emitPhysicalBatchCoordinator(context, root, out)) ||
        failed(emitPhysicalFaultReporter(context, "", "", out)))
      return failure();
  } else if (context.isPhysical()) {
    const auto &projection = context.physicalProjection();
    auto clock = legalizeIdentifier(
        cast<StringAttr>(root.getInputNames()[projection.clockInput])
            .getValue(),
        [&] { return root.emitOpError(); });
    auto reset = legalizeIdentifier(
        cast<StringAttr>(root.getInputNames()[projection.resetInput])
            .getValue(),
        [&] { return root.emitOpError(); });
    if (failed(clock) || failed(reset))
      return failure();
    out << "  wire pyc_local_error;\n"
           "  wire pyc_root_commit_ok;\n"
           "`ifndef SYNTHESIS\n"
           "  bit pyc_physical_fault = 1'b0;\n"
           "  wire pyc_physical_error = pyc_local_error | $isunknown("
        << context.rtlReset(*reset)
        << ");\n"
           "  assign pyc_root_commit_ok = !pyc_physical_fault && "
           "!pyc_physical_error;\n"
           // A level change to one is a physical rising edge; transitions
           // into X/Z are faults even when Verilog calls them falling edges.
           "  always @("
        << *clock
        << ") begin\n"
           "    if ($isunknown("
        << *clock << ") || ((" << *clock
        << " === 1'b1) && pyc_physical_error)) begin\n"
           "      if (!pyc_physical_fault) begin\n"
           "        $display(\"PYCIRCUIT_PHYSICAL_FAULT\");\n"
           "        pyc_report_physical_fault($sformatf(\"%m\"));\n"
           "      end\n"
           "      pyc_physical_fault = 1'b1;\n"
           "    end\n"
           "  end\n"
           "`else\n"
           "  assign pyc_root_commit_ok = 1'b1;\n"
           "`endif\n";
    if (failed(emitPhysicalFaultReporter(context, *clock, *reset, out)))
      return failure();
  }
  out << "  " << callee << *parameters << " dut (";
  first = true;
  auto connection = [&](StringRef name, StringRef expression) -> LogicalResult {
    auto id = legalizeIdentifier(name, [&] { return root.emitOpError(); });
    if (failed(id))
      return failure();
    out << (first ? "\n" : ",\n") << "    ." << *id << "(" << expression << ")";
    first = false;
    return success();
  };
  for (auto [index, raw] : llvm::enumerate(root.getInputNames())) {
    StringRef name = cast<StringAttr>(raw).getValue();
    std::string expression;
    if (index < layout->sourceInputCount) {
      auto id = legalizeIdentifier(name, [&] { return root.emitOpError(); });
      if (failed(id))
        return failure();
      expression = std::move(*id);
      if (context.batchesPhysicalEvents()) {
        out << "\n`ifndef SYNTHESIS\n";
        // Freeze only the active batch. Idle combinational outputs continue
        // following ordinary data inputs, including roots with no event ports.
        if (failed(connection(name, "pyc_batch_pending ? pyc_frozen_input_" +
                                        std::to_string(index) + " : " +
                                        expression)))
          return failure();
        out << "\n`else\n";
        // Both preprocessor branches occupy the same connection slot.
        first = index == 0;
        if (failed(connection(name, expression)))
          return failure();
        out << "\n`endif\n";
        continue;
      }
    } else {
      auto width = context.analysis.getPackedWidth(signature.getInput(index),
                                                   *bindings, root);
      if (failed(width))
        return failure();
      auto initial = cast<IntegerAttr>(
          layout->rootInputDefaults[index - layout->sourceInputCount]);
      expression =
          std::to_string(*width) + "'d" + std::to_string(initial.getInt());
    }
    if (failed(connection(name, expression)))
      return failure();
  }
  for (auto [index, raw] : llvm::enumerate(root.getOutputNames())) {
    StringRef name = cast<StringAttr>(raw).getValue();
    std::string expression;
    if (index < layout->sourceOutputCount ||
        llvm::is_contained(layout->outputValidity,
                           static_cast<int64_t>(index))) {
      auto id = legalizeIdentifier(name, [&] { return root.emitOpError(); });
      if (failed(id))
        return failure();
      expression = std::move(*id);
    }
    if (failed(connection(name, expression)))
      return failure();
  }
  if (context.managesChecks()) {
    auto reset = context.batchesPhysicalEvents()
                     ? FailureOr<std::string>(std::string("1'b0"))
                     : managedReset(context, root);
    if (failed(reset))
      return failure();
    out << (first ? "\n" : ",\n")
        << "    .pyc_phase(pyc_phase),\n"
           "    .pyc_root_commit_ok(pyc_root_commit_ok),\n"
           "    .pyc_reset_active("
        << *reset << "),\n"
        << "    .pyc_check_complete(pyc_check_complete),\n"
           "    .pyc_sampling_error(pyc_sampling_error),\n"
        << "    .pyc_local_error(pyc_local_error)";
    first = false;
  }
  if (context.isPhysical() && !context.batchesPhysicalEvents()) {
    auto reset = managedReset(context, root);
    if (failed(reset))
      return failure();
    out << (first ? "\n" : ",\n")
        << "    .pyc_root_commit_ok(pyc_root_commit_ok),\n"
           "    .pyc_reset_active("
        << *reset
        << "),\n"
           "    .pyc_local_error(pyc_local_error)";
    first = false;
  }
  out << (first ? ");\n" : "\n  );\n") << "endmodule\n";
  return success();
}
} // namespace
FailureOr<FinalVerilogSourceParts>
emitVerilogSourceParts(ModuleOp package, ac::HardwareAnalysis &analysis,
                       bool physical) {
  HardwareEmitContext context(package, analysis, physical);
  const bool system = context.isSystem();
  if (failed(context.prepareNativeChecks()))
    return failure();
  if (failed(analysis.verifySourceCheckPlan(context.sourceChecks())))
    return failure();
  auto root = rootModule(context);
  if (failed(root))
    return failure();
  llvm::DenseSet<Operation *> reachable;
  SmallVector<ac::ModuleOp> pending{*root};
  while (!pending.empty()) {
    auto definition = pending.pop_back_val();
    if (!reachable.insert(definition).second)
      continue;
    for (Operation &operation : definition.getBody().front()) {
      Operation *callee = nullptr;
      if (auto instance = dyn_cast<ac::InstanceOp>(operation))
        callee = analysis.resolveCallee(instance);
      else if (auto collection = dyn_cast<ac::CollectionOp>(operation))
        callee = analysis.resolveCallee(collection);
      if (auto child = dyn_cast_or_null<ac::ModuleOp>(callee))
        pending.push_back(child);
    }
  }
  auto unsupported = package.walk([&](Operation *op) {
    // Reachable assertions already belong to the verified source-check plan;
    // an assertion in an unselected definition does not add a root lifecycle.
    if (!isa<ac::SourceObserveOp>(op) || system ||
        !reachable.contains(op->getParentOfType<ac::ModuleOp>()))
      return WalkResult::advance();
    op->emitOpError() << "hardware instrumentation emission is not implemented";
    return WalkResult::interrupt();
  });
  if (unsupported.wasInterrupted())
    return failure();
  FinalVerilogSourceParts parts;
  auto portMap = context.rootPortMap();
  if (failed(portMap))
    return failure();
  parts.portMap = std::move(*portMap);
  parts.sourceCheckCount = context.sourceChecks().checks.size();
  parts.rootRtlName = "pyc_root";
  llvm::StringSet<> standard;
  package.walk([&](ac::InstanceOp instance) {
    if (!ac::isQueueInstance(instance))
      return;
    const std::string path = "include/verilog/fifo.v";
    if (standard.insert(path).second)
      parts.standardSources.push_back(path);
  });
  for (auto imported : package.getOps<ac::ModuleImportOp>()) {
    auto kind = analysis.getPrimitiveKind(imported);
    if (kind.empty())
      return imported.emitOpError()
             << "unresolved module import in RTL emitter";
    if (kind == "fifo_local" || kind == "fifo_downstream_pop")
      continue;
    std::string path = "include/verilog/" + kind.str() + ".v";
    if (standard.insert(path).second)
      parts.standardSources.push_back(path);
  }
  RtlPackageDependencies dependencies;
  context.rtlPackage = &dependencies;
  auto emitError = [&] { return package.emitOpError(); };
  auto units = ac::collectFinalSourceUnits(package, emitError);
  if (failed(units))
    return failure();
  llvm::StringMap<DictionaryAttr> paths;
  for (const ac::FinalSourceUnitView &unit : *units) {
    FinalVerilogSourceGroup group;
    group.sourceOwner = unit.owner;
    llvm::raw_string_ostream out(group.text);
    for (Operation *declaration : unit.declarations)
      if (auto module = dyn_cast<ac::ModuleOp>(declaration)) {
        group.path = context.names(module).verilog;
        group.definitions.push_back(
            FlatSymbolRefAttr::get(module.getContext(), module.getSymName()));
        if (failed(emitHardwareVerilogDefinition(context, module, out)))
          return failure();
      }
    out.flush();
    // Shared nominal typedefs remain in the core. An owner without a module
    // has provenance membership but no source file or placeholder RTL group.
    if (group.definitions.empty())
      continue;
    if (!paths.try_emplace(group.path, unit.owner).second)
      return emitError() << "source owner Verilog path collision";
    parts.sourceGroups.push_back(std::move(group));
  }
  {
    llvm::raw_string_ostream out(parts.core);
    if (failed(emitWrapper(context, *root, parts.rootRtlName,
                           context.names(*root).rtl, out)))
      return failure();
    out.flush();
  }
  if (system) {
    auto simulation = systemSimulationTop(context, *root, parts.rootRtlName);
    if (failed(simulation))
      return failure();
    parts.simulationTop = std::move(*simulation);
  }
  std::string typePackage;
  llvm::raw_string_ostream out(typePackage);
  if (failed(emitTypePackage(context, dependencies, out)))
    return failure();
  parts.core.insert(0, typePackage);
  return parts;
}
LogicalResult emitVerilog(ModuleOp package, ac::HardwareAnalysis &analysis,
                          raw_ostream &out, bool physical) {
  auto parts = emitVerilogSourceParts(package, analysis, physical);
  if (failed(parts))
    return failure();
  out << parts->core;
  for (auto &group : parts->sourceGroups)
    out << group.text;
  out << parts->runtimeGlue;
  out << parts->simulationTop;
  return success();
}
} // namespace acir::compiler
