#include "FinalEmitVerilog.h"
#include "FinalCppNames.h"
#include "HardwareEmitCommon.h"
#include "pycircuit/Dialect/ACIR/SourceUnitValidation.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/StringSet.h"
#include "llvm/Support/JSON.h"
#include "llvm/Support/raw_ostream.h"
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
                         raw_ostream &out, llvm::DenseSet<Operation *> &done) {
  if (done.contains(record))
    return success();
  for (auto raw : record.getFields()) {
    auto field = cast<DictionaryAttr>(raw);
    auto fieldType = field.getAs<TypeAttr>("type").getValue();
    // Follow nominal dependencies through packed Table elements as well.
    while (auto table = dyn_cast<ac::TableType>(fieldType))
      fieldType = table.getElementType();
    if (auto nested = dyn_cast<ac::StructType>(fieldType)) {
      auto declaration = context.analysis.lookupStruct(nested);
      if (!declaration || failed(emitRecord(context, declaration, out, done)))
        return failure();
    }
  }
  done.insert(record);
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
};
FailureOr<std::string> managedReset(ac::ModuleOp root) {
  auto domain = root->getAttrOfType<DictionaryAttr>("ac.domain_inputs");
  if (!domain || domain.empty())
    return std::string("1'b0");
  auto reset = domain.getAs<IntegerAttr>("reset");
  auto names = root.getInputNames();
  if (!reset || reset.getValue().getActiveBits() > 64 ||
      reset.getValue().getZExtValue() >= names.size())
    return root.emitOpError()
           << "managed module requires verified reset context";
  return legalizeIdentifier(
      cast<StringAttr>(names[reset.getValue().getZExtValue()]).getValue(),
      [&] { return root.emitOpError(); });
}
FailureOr<SystemControls> systemControls(ac::ModuleOp root) {
  auto domain = root->getAttrOfType<DictionaryAttr>("ac.domain_inputs");
  auto clock = domain ? domain.getAs<IntegerAttr>("clock") : IntegerAttr();
  auto reset = domain ? domain.getAs<IntegerAttr>("reset") : IntegerAttr();
  auto inputs = root.getInputNames();
  if (!domain || domain.size() != 2 || !clock || !reset ||
      clock.getValue().getActiveBits() > 64 ||
      reset.getValue().getActiveBits() > 64)
    return root.emitOpError()
           << "system root requires verified clock/reset domains";
  uint64_t clockIndex = clock.getValue().getZExtValue();
  uint64_t resetIndex = reset.getValue().getZExtValue();
  if (clockIndex == resetIndex || inputs.size() != 2 ||
      clockIndex >= inputs.size() || resetIndex >= inputs.size() ||
      !root.getOutputNames().empty())
    return root.emitOpError()
           << "system root must be closed except for clock/reset";
  auto legal = [&](uint64_t index) {
    return legalizeIdentifier(cast<StringAttr>(inputs[index]).getValue(),
                              [&] { return root.emitOpError(); });
  };
  auto clockName = legal(clockIndex), resetName = legal(resetIndex);
  if (failed(clockName) || failed(resetName))
    return failure();
  return SystemControls{std::move(*clockName), std::move(*resetName)};
}
std::string jsonString(StringRef value) {
  std::string text;
  llvm::raw_string_ostream out(text);
  out << llvm::json::Value(value);
  return text;
}
std::string displayLiteral(StringRef value) {
  std::string text;
  text.reserve(value.size());
  for (char character : value) {
    switch (character) {
    case '\\':
      text += "\\\\";
      break;
    case '"':
      text += "\\\"";
      break;
    case '\n':
      text += "\\n";
      break;
    case '\r':
      text += "\\r";
      break;
    case '\t':
      text += "\\t";
      break;
    case '%':
      text += "%%";
      break;
    default:
      text += character;
      break;
    }
  }
  return text;
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
FailureOr<uint64_t>
observationWidth(HardwareEmitContext &context,
                 const ObservationEmissionOccurrence &occurrence) {
  auto rootBindings = context.rootBindings();
  if (failed(rootBindings))
    return failure();
  ac::HardwareBindings bindings = std::move(*rootBindings);
  for (auto [operation, lane] : occurrence.path) {
    (void)lane;
    auto child = isa<ac::InstanceOp>(operation)
                     ? context.analysis.bindInstance(
                           cast<ac::InstanceOp>(operation), bindings)
                     : context.analysis.bindInstance(
                           cast<ac::CollectionOp>(operation), bindings);
    if (failed(child))
      return failure();
    bindings = std::move(*child);
  }
  auto observe = cast<ac::SourceObserveOp>(occurrence.site.op);
  Value observed = occurrence.site.carriesValue
                       ? observe.getValues()[occurrence.site.valueIndex]
                       : observe.getPath();
  auto width =
      context.analysis.getPackedWidth(observed.getType(), bindings, observe);
  if (failed(width))
    return failure();
  if (*width == 0 || *width > 64)
    return observe.emitOpError()
           << "RTL observation requires a scalar payload of at most 64 bits";
  return *width;
}
FailureOr<std::string> systemSimulationTop(HardwareEmitContext &context,
                                           ac::ModuleOp root,
                                           StringRef wrapper) {
  auto controls = systemControls(root);
  auto plan = observationEmissionPlan(context);
  if (failed(controls) || failed(plan))
    return failure();
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
    for (auto [operation, lane] : occurrence.path) {
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
         "  logic pyc_clk = 1'b0;\n"
         "  logic pyc_rst = 1'b0;\n"
         "  logic [2:0] pyc_phase = 3'd0;\n"
         "  logic pyc_root_commit_ok = 1'b0;\n"
         "  wire pyc_local_error;\n"
         "  localparam longint unsigned pyc_max_cycles = "
         "64'd4611686018427387903;\n"
         "  longint unsigned pyc_cycles = 64'd10;\n"
         "  longint unsigned pyc_epoch = 64'd0;\n"
         "  longint unsigned pyc_cycle;\n"
         "  string pyc_cycles_text;\n  "
      << wrapper << " dut (\n    ." << controls->clock << "(pyc_clk),\n    ."
      << controls->reset
      << "(pyc_rst),\n"
         "    .pyc_phase(pyc_phase),\n"
         "    .pyc_root_commit_ok(pyc_root_commit_ok),\n"
         "    .pyc_local_error(pyc_local_error)\n  );\n";
  for (const auto &report : reports)
    out << "  longint unsigned pyc_report_value_" << report.ordinal
        << " = 0;\n  longint unsigned pyc_report_update_" << report.ordinal
        << " = 0;\n";
  out << "  function automatic bit pyc_parse_cycles(\n"
         "      input string text, output longint unsigned value);\n"
         "    integer index;\n"
         "    integer character;\n"
         "    longint unsigned digit;\n"
         "    begin\n"
         "      value = 0;\n"
         "      if (text.len() == 0) return 1'b0;\n"
         "      for (index = 0; index < text.len(); index = index + 1) begin\n"
         "        character = text.getc(index);\n"
         "        if (character < 48 || character > 57) return 1'b0;\n"
         "        digit = character - 48;\n"
         "        if (value > (pyc_max_cycles - digit) / 10) return 1'b0;\n"
         "        value = value * 10 + digit;\n"
         "      end\n"
         "      return value != 0;\n"
         "    end\n"
         "  endfunction\n"
         "  task automatic pyc_publish_observations;\n    begin\n";
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
      auto width = observationWidth(context, lane);
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
  // Check every group before emitting any record from this epoch. A partially
  // active later group must not publish an earlier complete group's prefix.
  for (const auto &group : groups)
    for (size_t index = 1; index < group.size; ++index)
      out << "      if ((" << signal(group.addresses[index], "valid")
          << " !== " << signal(group.addresses[0], "valid") << ") || ("
          << signal(group.addresses[index], "path")
          << " !== " << signal(group.addresses[0], "path") << "))\n"
          << "        $fatal(1, \"pycircuit_sim: inconsistent observation "
             "group\");\n";
  for (const auto &group : groups) {
    const auto &occurrence = plan->occurrences[group.begin];
    auto observe = cast<ac::SourceObserveOp>(occurrence.site.op);
    auto identity = observe->getAttrOfType<DictionaryAttr>("ac.observation_id");
    if (!identity)
      return observe.emitOpError() << "observation identity is missing";
    auto registration =
        emissionMetadataJson(identity.get("registration"), observe);
    auto site = emissionMetadataJson(identity.get("site"), observe);
    auto spec = emissionMetadataJson(observe.getSpec(), observe);
    if (failed(registration) || failed(site) || failed(spec))
      return failure();
    std::string instance = "root";
    for (auto [operation, lane] : occurrence.path) {
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
      if (group.widths[index] == 1 && !occurrence.site.gauge) {
        record += displayLiteral("{\"kind\":\"bool\",\"value\":") + "%s}";
        arguments += ", " + value + " ? \"true\" : \"false\"";
      } else {
        record += displayLiteral("{\"kind\":\"integer\",\"value\":\"") + "%0d" +
                  displayLiteral("\"}");
        arguments += ", $unsigned(" + value + ")";
      }
    }
    record += "]}";
    out << "      if (" << signal(group.addresses[0], "valid") << " && "
        << signal(group.addresses[0], "path") << ") begin\n";
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
         "  task automatic pyc_step(input logic level);\n"
         "    begin\n"
         "      pyc_clk = level;\n"
         "      pyc_root_commit_ok = 1'b0;\n"
         "      pyc_phase = 3'd1; #1;\n"
         "      if (pyc_local_error !== 1'b0) begin\n"
         "        pyc_phase = 3'd3; #1;\n"
         "        $fatal(1, \"pycircuit_sim: source or primitive check failed "
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
         "    if ($value$plusargs(\"cycles=%s\", pyc_cycles_text)) begin\n"
         "      if (!pyc_parse_cycles(pyc_cycles_text, pyc_cycles))\n"
         "        $fatal(1, \"pycircuit_sim: +cycles requires a positive "
         "decimal value no greater than 4611686018427387903\");\n"
         "    end else if ($test$plusargs(\"cycles\"))\n"
         "      $fatal(1, \"pycircuit_sim: +cycles requires a positive decimal "
         "value no greater than 4611686018427387903\");\n"
         "    pyc_clk = 1'b0; pyc_rst = 1'b0;\n"
         "    pyc_root_commit_ok = 1'b1; pyc_phase = 3'd4; #1;\n"
         "    pyc_phase = 3'd2; #1;\n"
         "    pyc_phase = 3'd0; pyc_root_commit_ok = 1'b0;\n"
         "    for (pyc_cycle = 0; pyc_cycle < pyc_cycles; pyc_cycle = "
         "pyc_cycle + 1) begin\n"
         "      pyc_step(1'b0);\n      pyc_step(1'b1);\n    end\n"
         "    "
         "$write(\"{\\\"kind\\\":\\\"result\\\",\\\"status\\\":"
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
LogicalResult emitWrapper(HardwareEmitContext &context, ac::ModuleOp root,
                          StringRef wrapper, StringRef callee,
                          raw_ostream &out) {
  out << "module " << wrapper << " (";
  auto bindings = context.rootBindings();
  if (failed(bindings))
    return failure();
  auto signature = root.getFunctionType();
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
  for (auto [i, type] : llvm::enumerate(signature.getInputs()))
    if (failed(port("input", type,
                    cast<StringAttr>(root.getInputNames()[i]).getValue())))
      return failure();
  for (auto [i, type] : llvm::enumerate(signature.getResults()))
    if (failed(port("output", type,
                    cast<StringAttr>(root.getOutputNames()[i]).getValue())))
      return failure();
  if (context.managesChecks()) {
    out << (first ? "\n" : ",\n")
        << "  input wire [2:0] pyc_phase,\n"
           "  input wire pyc_root_commit_ok,\n"
           "  output wire pyc_local_error";
    first = false;
  }
  out << (first ? ");\n" : "\n);\n");
  auto parameters = callee == context.names(root).rtl
                        ? context.rootRtlParameters()
                        : FailureOr<std::string>(std::string());
  if (failed(parameters))
    return failure();
  out << "  " << callee << *parameters << " dut (";
  first = true;
  auto connection = [&](StringRef name) -> LogicalResult {
    auto id = legalizeIdentifier(name, [&] { return root.emitOpError(); });
    if (failed(id))
      return failure();
    out << (first ? "\n" : ",\n") << "    ." << *id << "(" << *id << ")";
    first = false;
    return success();
  };
  for (auto raw : root.getInputNames())
    if (failed(connection(cast<StringAttr>(raw).getValue())))
      return failure();
  for (auto raw : root.getOutputNames())
    if (failed(connection(cast<StringAttr>(raw).getValue())))
      return failure();
  if (context.managesChecks()) {
    auto reset = managedReset(root);
    if (failed(reset))
      return failure();
    out << (first ? "\n" : ",\n")
        << "    .pyc_phase(pyc_phase),\n"
           "    .pyc_root_commit_ok(pyc_root_commit_ok),\n"
           "    .pyc_reset_active("
        << *reset << "),\n"
        << "    .pyc_local_error(pyc_local_error)";
    first = false;
  }
  out << (first ? ");\n" : "\n  );\n") << "endmodule\n";
  return success();
}
} // namespace
FailureOr<FinalVerilogSourceParts>
emitVerilogSourceParts(ModuleOp package, ac::HardwareAnalysis &analysis) {
  HardwareEmitContext context(package, analysis);
  const bool system = context.isSystem();
  if (failed(context.prepareNativeChecks()))
    return failure();
  if (failed(analysis.verifySourceCheckPlan(context.sourceChecks())))
    return failure();
  auto unsupported = package.walk([&](Operation *op) {
    if ((!isa<ac::SourceObserveOp>(op) || system) &&
        (!isa<ac::SourceExpectOp>(op) || context.managesChecks()))
      return WalkResult::advance();
    op->emitOpError() << "hardware instrumentation emission is not implemented";
    return WalkResult::interrupt();
  });
  if (unsupported.wasInterrupted())
    return failure();
  auto root = rootModule(context);
  if (failed(root))
    return failure();
  FinalVerilogSourceParts parts;
  parts.sourceCheckCount = context.sourceChecks().checks.size();
  parts.rootRtlName = "pyc_root";
  llvm::StringSet<> standard;
  package.walk([&](ac::QueueOp) {
    const std::string path = "include/verilog/fifo.v";
    if (standard.insert(path).second)
      parts.standardSources.push_back(path);
  });
  for (auto imported : package.getOps<ac::ModuleImportOp>()) {
    auto kind = analysis.getPrimitiveKind(imported);
    if (kind.empty())
      return imported.emitOpError()
             << "unresolved module import in RTL emitter";
    std::string path = "include/verilog/" + kind.str() + ".v";
    if (standard.insert(path).second)
      parts.standardSources.push_back(path);
  }
  std::string typedefs;
  {
    llvm::raw_string_ostream out(typedefs);
    out << "package pycircuit_types;\n"
           "  function automatic logic signed [63:0] pyc_floordiv(input logic "
           "signed [63:0] a, b);\n"
           "    return a/b - ((a%b != 0 && (a<0)!=(b<0)) ? 64'sd1 : 64'sd0);\n"
           "  endfunction\n"
           "  function automatic logic signed [63:0] pyc_mod(input logic "
           "signed [63:0] a, b);\n"
           "    logic signed [63:0] r; r=a%b; return r != 0 && (r<0)!=(b<0) ? "
           "r+b : r;\n"
           "  endfunction\n";
    for (auto declaration : package.getOps<ac::EnumOp>())
      if (failed(emitEnum(context, declaration, out)))
        return failure();
    llvm::DenseSet<Operation *> done;
    for (auto record : analysis.getStructs())
      if (failed(emitRecord(context, record, out, done)))
        return failure();
    out << "endpackage\n";
    out.flush();
  }
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
  parts.core = typedefs;
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
  return parts;
}
LogicalResult emitVerilog(ModuleOp package, ac::HardwareAnalysis &analysis,
                          raw_ostream &out) {
  auto parts = emitVerilogSourceParts(package, analysis);
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
