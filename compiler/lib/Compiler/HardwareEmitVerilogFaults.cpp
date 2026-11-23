#include "FinalCppNames.h"
#include "HardwareEmitCommon.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/SymbolTable.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/Support/JSON.h"
#include <algorithm>

using namespace mlir;
namespace acir::compiler {
namespace {
// Lifetime is one emission of a verified snapshot. Only static JSON is cached.
struct MetadataPool {
  llvm::DenseMap<Attribute, std::string> attributes;
  llvm::DenseMap<Operation *, std::string> rules;
  llvm::DenseMap<Attribute, std::string> locations;
  llvm::StringMap<unsigned> indices;
  SmallVector<std::string> fragments;

  std::string expression(StringRef json) {
    auto [entry, inserted] = indices.try_emplace(json, fragments.size());
    if (inserted)
      fragments.push_back(json.str());
    return "pyc_fault_meta_" + std::to_string(entry->second);
  }
  void emit(raw_ostream &out) {
    for (auto [index, json] : llvm::enumerate(fragments))
      out << "  localparam string pyc_fault_meta_" << index << " = "
          << jsonString(json) << ";\n";
  }
};
// Metadata is escaped before adding format slots: authored '%' is literal.
struct Record {
  MetadataPool &pool;
  std::string format, arguments;
  explicit Record(MetadataPool &pool) : pool(pool) {
    literal("{\"schema_version\":1,\"event\":\"physical_fault\"");
  }
  void literal(StringRef text) { format += displayLiteral(text); }
  void slot(StringRef spec, StringRef expression) {
    format += spec.str();
    arguments += ", " + expression.str();
  }
  void key(StringRef name) { literal("," + jsonString(name) + ":"); }
  void json(StringRef value) {
    // Short fragments cost less inline than an extra declaration/reference.
    if (value.size() < 64)
      literal(value);
    else
      slot("%s", pool.expression(value));
  }
  void raw(StringRef name, StringRef value) {
    key(name);
    json(value);
  }
  void text(StringRef name, StringRef value) { raw(name, jsonString(value)); }
  void bits(StringRef name, StringRef expression) {
    key(name);
    literal("{\"width\":1,\"bits\":\"");
    slot("%b", expression);
    literal("\"}");
  }
  void emit(raw_ostream &out) {
    literal("}");
    out << "      $display(\"PYCIRCUIT_FAULT_DETAIL " << format << "\""
        << arguments << ");\n      pyc_details = pyc_details + 1;\n";
  }
};
std::string sourceJson(MetadataPool &pool, Operation *op) {
  LocationAttr location = op->getLoc();
  auto cached = pool.locations.find(location);
  if (cached != pool.locations.end())
    return cached->second;
  if (auto file = dyn_cast<FileLineColLoc>(location)) {
    std::string text;
    llvm::raw_string_ostream stream(text);
    stream << llvm::json::Value(
        llvm::json::Object{{"path", file.getFilename().getValue()},
                           {"line", file.getLine()},
                           {"column", file.getColumn()}});
    pool.locations.try_emplace(location, text);
    return text;
  }
  pool.locations.try_emplace(location, "null");
  return "null";
}
FailureOr<std::string> metadata(MetadataPool &pool, Attribute value,
                                Operation *site) {
  if (!value)
    return std::string("null");
  auto found = pool.attributes.find(value);
  if (found != pool.attributes.end())
    return found->second;
  auto json = emissionMetadataJson(value, site);
  if (succeeded(json))
    pool.attributes.try_emplace(value, *json);
  return json;
}
FailureOr<std::string> ruleJson(MetadataPool &pool, ac::RuleOp rule) {
  if (!rule)
    return std::string("null");
  auto found = pool.rules.find(rule);
  if (found != pool.rules.end())
    return found->second;
  auto occurrence = metadata(pool, rule->getAttr("declaration"), rule);
  if (failed(occurrence))
    return failure();
  auto json = "{\"name\":" + jsonString(rule.getName()) +
              ",\"occurrence\":" + *occurrence + "}";
  pool.rules.try_emplace(rule, json);
  return json;
}
struct Address {
  std::string rtl = "dut", logical = "root";
  ac::HardwareBindings bindings;
  DictionaryAttr bindingKey;
};
LogicalResult descend(HardwareEmitContext &context, Address &address,
                      Operation *allocation, ArrayRef<uint64_t> coordinates) {
  auto raw = allocation->getAttrOfType<StringAttr>("instance_name");
  auto name = legalizeIdentifier(raw.getValue(),
                                 [&] { return allocation->emitOpError(); });
  if (failed(name))
    return failure();
  address.logical += "/" + raw.getValue().str();
  if (auto collection = dyn_cast<ac::CollectionOp>(allocation)) {
    auto shape = context.analysis.resolveShape(collection.getShape(),
                                               address.bindings, collection);
    if (failed(shape))
      return failure();
    uint64_t lane = 0;
    for (auto [extent, index] : llvm::zip_equal(*shape, coordinates)) {
      lane = lane * extent + index;
      address.logical += "[" + std::to_string(index) + "]";
    }
    address.rtl += ".pyc_instances_" + *name + "[" + std::to_string(lane) + "]";
  }
  auto child = isa<ac::InstanceOp>(allocation)
                   ? context.analysis.bindInstance(
                         cast<ac::InstanceOp>(allocation), address.bindings)
                   : context.analysis.bindInstance(
                         cast<ac::CollectionOp>(allocation), address.bindings);
  if (failed(child))
    return failure();
  address.bindings = std::move(*child);
  address.rtl += ".pyc_instance_" + *name;
  return success();
}
using OwnerCache = SmallVector<std::optional<Address>>;
struct ControlTrace {
  SmallVector<std::pair<Value, uint64_t>> controls;
  uint64_t work;
};
using TraceCache =
    llvm::DenseMap<std::pair<Value, DictionaryAttr>, ControlTrace>;
FailureOr<Address> ownerAddress(HardwareEmitContext &context, uint64_t owner,
                                OwnerCache &owners) {
  if (owners[owner])
    return *owners[owner];
  Address result;
  const auto &plan = context.sourceChecks();
  result.bindings = plan.owners.front().bindings;
  for (const auto &step : plan.owners[owner].path)
    if (failed(descend(context, result, step.allocation, step.coordinates)))
      return failure();
  Builder builder(context.package.getContext());
  NamedAttrList integers, types;
  for (const auto &entry : result.bindings.integers)
    integers.append(entry.first(), entry.second);
  for (const auto &entry : result.bindings.types)
    types.append(entry.first(), TypeAttr::get(entry.second));
  result.bindingKey = builder.getDictionaryAttr(
      {builder.getNamedAttr("owner",
                            SymbolTable::getSymbolName(result.bindings.owner)),
       builder.getNamedAttr("integers", builder.getDictionaryAttr(integers)),
       builder.getNamedAttr("types", builder.getDictionaryAttr(types))});
  owners[owner] = result;
  return result;
}
Value transparent(Value value) {
  if (auto argument = dyn_cast<BlockArgument>(value)) {
    if (auto rule = dyn_cast<ac::RuleOp>(argument.getOwner()->getParentOp()))
      return rule.getCaptures()[argument.getArgNumber()];
  }
  if (auto rule = value.getDefiningOp<ac::RuleOp>())
    return cast<ac::YieldOp>(rule.getBody().front().back())
        .getValues()[cast<OpResult>(value).getResultNumber()];
  return {};
}
std::string reference(HardwareEmitContext &context, StringRef owner,
                      Value value) {
  // The emitter records real names; only transparent Rule bindings need
  // peeling.
  while (value) {
    auto found = context.physicalLayout.nets.find(value);
    if (found != context.physicalLayout.nets.end())
      return owner.str() + "." + found->second;
    value = transparent(value);
  }
  return {};
}
void common(Record &record, StringRef kind, StringRef reason,
            const Address &address, StringRef source) {
  record.key("time_s");
  record.slot("%0.17e", "$realtime / 1s");
  record.key("instance");
  record.slot("%s", address.rtl.empty()
                        ? "pyc_fault_json_string(root_instance)"
                        : "pyc_fault_json_string({root_instance, \"." +
                              address.rtl + "\"})");
  record.text("logical_instance", address.logical);
  record.text("kind", kind);
  record.text("reason", reason);
  record.raw("source", source);
}
// Trace existing one-bit SSA, never derive new checks or evaluate source code.
LogicalResult relatedControls(HardwareEmitContext &context,
                              const Address &owner, Value pin,
                              uint64_t &remaining, Record &record,
                              TraceCache &traces) {
  auto key = std::make_pair(pin, owner.bindingKey);
  auto cached = traces.find(key);
  SmallVector<std::pair<Value, uint64_t>> controls;
  bool truncated = false;
  const uint64_t available = remaining;
  if (cached != traces.end()) {
    // Reuse analysis without increasing the original diagnostic expansion cap.
    // Visit ordinals retain the same prefix when this occurrence hits the cap.
    for (auto control : cached->second.controls) {
      if (control.second > available)
        break;
      controls.push_back(control);
    }
    truncated = cached->second.work > available;
    remaining -= std::min(available, cached->second.work);
  } else {
    SmallVector<Value> pending{pin};
    llvm::DenseSet<Value> seen;
    while (!pending.empty()) {
      Value value = pending.pop_back_val();
      if (!seen.insert(value).second)
        continue;
      if (!remaining) {
        truncated = true;
        break;
      }
      --remaining;
      if (auto peeled = transparent(value)) {
        pending.push_back(peeled);
        continue;
      }
      Operation *op = value.getDefiningOp();
      if (!op || isa<ac::InstanceOp, ac::CollectionOp>(op) ||
          !isa<ac::BitsType>(value.getType()))
        continue;
      auto width =
          context.analysis.getPackedWidth(value.getType(), owner.bindings, op);
      if (failed(width))
        return failure();
      if (*width != 1 || reference(context, owner.rtl, value).empty())
        continue;
      if (isa<ac::BitsSelectOp, ac::BitsCompareOp, ac::BitsUnaryOp,
              ac::BitsBinaryOp>(op))
        controls.emplace_back(value, available - remaining);
      for (Value operand : llvm::reverse(op->getOperands()))
        pending.push_back(operand);
    }
    if (!truncated)
      traces.try_emplace(key, ControlTrace{controls, available - remaining});
  }
  record.key("related_controls");
  record.literal("[");
  bool first = true;
  for (const auto &control : controls) {
    Value value = control.first;
    auto wire = reference(context, owner.rtl, value);
    if (wire.empty())
      continue;
    auto *op = value.getDefiningOp();
    auto rule = ruleJson(record.pool, op->getParentOfType<ac::RuleOp>());
    if (failed(rule))
      return failure();
    if (!first)
      record.literal(",");
    first = false;
    record.literal("{\"source\":");
    record.json(sourceJson(record.pool, op));
    record.literal(",\"operation\":" +
                   jsonString(op->getName().getStringRef()) + ",\"rule\":");
    record.json(*rule);
    record.bits("value", wire);
    record.literal("}");
  }
  record.literal("]");
  record.raw("related_controls_truncated", truncated ? "true" : "false");
  return success();
}
} // namespace

LogicalResult emitPhysicalFaultReporter(HardwareEmitContext &context,
                                        StringRef clock, StringRef reset,
                                        raw_ostream &destination) {
  if (context.batchesPhysicalEvents()) {
    destination << "`ifndef SYNTHESIS\n"
        "  task automatic pyc_report_physical_fault(input string root_instance);\n"
        "    begin\n"
        "      $display(\"PYCIRCUIT_FAULT_DETAIL {\\\"schema_version\\\":1,"
        "\\\"event\\\":\\\"physical_fault\\\","
        "\\\"kind\\\":\\\"event_batch\\\","
        "\\\"reason\\\":\\\"completed_batch_rejected\\\","
        "\\\"check_complete\\\":\\\"%b\\\","
        "\\\"sampling_error\\\":\\\"%b\\\","
        "\\\"local_error\\\":\\\"%b\\\","
        "\\\"setup_error\\\":\\\"%b\\\"}\", "
        "pyc_check_complete, pyc_sampling_error, pyc_local_error, "
        "pyc_physical_setup_error);\n";
    for (unsigned input : context.physicalProjection().clockInputs) {
      destination << "      if ($isunknown(pyc_frozen_input_" << input
          << ")) $display(\"PYCIRCUIT_FAULT_DETAIL {\\\"schema_version\\\":1,"
             "\\\"event\\\":\\\"physical_fault\\\","
             "\\\"kind\\\":\\\"root_clock\\\","
             "\\\"reason\\\":\\\"clock_unknown\\\","
             "\\\"input_ordinal\\\":" << input << "}\");\n";
      destination << "      if (pyc_initial_batch && (pyc_frozen_input_"
          << input << " === 1'b1)) $display(\"PYCIRCUIT_FAULT_DETAIL "
             "{\\\"schema_version\\\":1,"
             "\\\"event\\\":\\\"physical_fault\\\","
             "\\\"kind\\\":\\\"root_clock\\\","
             "\\\"reason\\\":\\\"initial_clock_not_zero\\\","
             "\\\"input_ordinal\\\":" << input << "}\");\n";
    }
    destination << "    end\n  endtask\n`endif\n";
    return success();
  }
  MetadataPool pool;
  OwnerCache owners(context.sourceChecks().owners.size());
  TraceCache traces;
  std::string body;
  llvm::raw_string_ostream out(body);
  out << R"sv(
  function automatic string pyc_fault_json_string(input string raw);
    string result;
    byte unsigned character;
    result = "\"";
    for (int i = 0; i < raw.len(); i++) begin
      character = raw[i];
      case (character)
        8'h22: result = {result, "\\\""};
        8'h5c: result = {result, "\\\\"};
        default: begin
          if (character < 32)
            result = {result, $sformatf("\\u%04x", character)};
          else result = {result, $sformatf("%c", character)};
        end
      endcase
    end
    return {result, "\""};
  endfunction
  task automatic pyc_report_physical_fault(input string root_instance);
    integer pyc_details;
    begin
      pyc_details = 0;
)sv";
  const auto &plan = context.sourceChecks();
  auto projection = context.physicalProjection();
  std::string effectiveReset = context.rtlReset(reset);
  Address root;
  root.rtl.clear();
  // Root instance is already a complete path; do not append an extra dot.
  auto rootRecord = [&](StringRef kind, StringRef reason, unsigned input) {
    Record record(pool);
    common(record, kind, reason, root, "null");
    record.raw(
        "endpoint",
        "{\"input_ordinal\":" + std::to_string(input) + ",\"input_name\":" +
            jsonString(cast<StringAttr>(projection.root.getInputNames()[input])
                           .getValue()) +
            ",\"role\":" +
            jsonString(kind == "root_clock" ? "clock" : "reset") + "}");
    return record;
  };
  out << "      if ($isunknown(" << clock << ")) begin\n";
  auto clockRecord =
      rootRecord("root_clock", "clock_unknown", projection.clockInput);
  clockRecord.key("values");
  clockRecord.literal("{");
  // bits() normally inserts a comma, so start each values object with a field.
  clockRecord.literal("\"clock\":{\"width\":1,\"bits\":\"");
  clockRecord.slot("%b", clock);
  clockRecord.literal("\"}}");
  clockRecord.emit(out);
  out << "      end else if (" << clock << " === 1'b1) begin\n"
      << "        if ($isunknown(" << effectiveReset << ")) begin\n";
  auto resetRecord =
      rootRecord("root_reset", "reset_unknown", projection.resetInput);
  resetRecord.key("values");
  resetRecord.literal("{\"raw_reset\":{\"width\":1,\"bits\":\"");
  resetRecord.slot("%b", reset);
  resetRecord.literal("\"}");
  resetRecord.bits("effective_reset", effectiveReset);
  resetRecord.literal("}");
  resetRecord.emit(out);
  out << "        end\n";
  for (const auto &check : plan.checks) {
    const auto &binding = plan.bindings[check.bindingIndex];
    auto owner = ownerAddress(context, check.ownerOccurrence, owners);
    auto id = metadata(pool, binding.checkID, binding.expect);
    auto source = metadata(pool, binding.location, binding.expect);
    auto rule = ruleJson(pool, binding.rule);
    if (failed(owner) || failed(id) || failed(source) || failed(rule))
      return failure();
    std::string path = reference(context, owner->rtl, binding.path);
    std::string condition = reference(context, owner->rtl, binding.condition);
    auto error = context.physicalLayout.checkErrors.find(binding.expect);
    if (path.empty() || condition.empty() ||
        error == context.physicalLayout.checkErrors.end())
      return binding.expect->emitOpError()
             << "physical check has no emitted layout";
    out << "        if (" << owner->rtl << "." << error->second << ") begin\n";
    for (bool reachability : {true, false}) {
      out << "          if ("
          << (reachability ? "$isunknown(" + path + ")"
                           : "!$isunknown(" + path + ") && (" + effectiveReset +
                                 " === 1'b0) && (" + path +
                                 " === 1'b1) && $isunknown(" + condition + ")")
          << ") begin\n";
      Record record(pool);
      common(record, "source_check",
             reachability ? "reachability_unknown" : "predicate_unknown",
             *owner, *source);
      record.raw("check_id", *id);
      record.text("check_kind", binding.kind.getValue());
      record.raw("rule", *rule);
      if (binding.message)
        record.text("message", binding.message.getValue());
      record.key("values");
      record.literal("{\"path\":{\"width\":1,\"bits\":\"");
      record.slot("%b", path);
      record.literal("\"}");
      record.bits("condition", condition);
      record.bits("effective_reset", effectiveReset);
      record.literal("}");
      record.emit(out);
      out << "          end\n";
    }
    out << "        end\n";
  }
  uint64_t remaining = ac::HardwareSourceCheckLimits{}.maxWorkUnits;
  for (const auto &endpoint : plan.commits) {
    auto kind = endpoint.primitiveKind.getValue();
    if (kind != "dff" && kind != "dffe")
      continue;
    auto owner = ownerAddress(context, endpoint.ownerOccurrence, owners);
    if (failed(owner))
      return failure();
    Address leaf = *owner;
    if (failed(
            descend(context, leaf, endpoint.allocation, endpoint.coordinates)))
      return failure();
    auto occurrence = metadata(pool, endpoint.allocation->getAttr("occurrence"),
                               endpoint.allocation);
    if (failed(occurrence))
      return failure();
    for (unsigned role = 0; role < (kind == "dffe" ? 2u : 1u); ++role) {
      // Bit selection of a function call is not portable across simulators.
      out << "        if ((" << leaf.rtl
          << ".pyc_physical_control_errors() & 2'd" << (1u << role)
          << ") != 0) begin\n";
      Record record(pool);
      common(record, "primitive_control",
             role ? "enable_unknown" : "reset_unknown", leaf,
             sourceJson(pool, endpoint.allocation));
      record.text("primitive", kind);
      std::string coordinates = "[";
      for (auto [index, coordinate] : llvm::enumerate(endpoint.coordinates)) {
        if (index)
          coordinates += ",";
        coordinates += std::to_string(coordinate);
      }
      coordinates += "]";
      record.key("endpoint");
      record.literal("{\"allocation\":");
      record.json(*occurrence);
      record.literal(",\"coordinates\":" + coordinates + ",\"role\":" +
                     jsonString(role ? "enable" : "reset") + "}");
      record.raw("rule", "null");
      record.key("values");
      record.literal("{\"reset\":{\"width\":1,\"bits\":\"");
      record.slot("%b", leaf.rtl + ".rst");
      record.literal("\"}");
      if (kind == "dffe")
        record.bits("enable", leaf.rtl + ".en");
      record.literal("}");
      if (role && failed(relatedControls(context, *owner,
                                         endpoint.allocation->getOperand(2),
                                         remaining, record, traces)))
        return failure();
      record.emit(out);
      out << "        end\n";
    }
  }
  out << "      end\n      if (pyc_details == 0) begin\n";
  Record gap(pool);
  common(gap, "diagnostic_gap", "unattributed_error", root, "null");
  gap.raw("endpoint", "null");
  gap.raw("attribution_complete", "false");
  gap.key("values");
  gap.literal("{\"clock\":{\"width\":1,\"bits\":\"");
  gap.slot("%b", clock);
  gap.literal("\"}");
  gap.bits("raw_reset", reset);
  gap.bits("effective_reset", effectiveReset);
  gap.bits("aggregate_error", "pyc_physical_error");
  gap.literal("}");
  gap.emit(out);
  out << "      end\n    end\n  endtask\n";
  out.flush();
  destination << "`ifndef SYNTHESIS\n";
  pool.emit(destination);
  destination << body << "`endif\n";
  return success();
}
} // namespace acir::compiler
