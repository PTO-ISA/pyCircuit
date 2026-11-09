#include "VerilogBackend.h"

#include "FinalEmissionNames.h"
#include "pycircuit/Dialect/HWIR/HWIRVerification.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringSet.h"
#include "llvm/Support/Format.h"
#include "llvm/ADT/ScopeExit.h"
#include "llvm/Support/JSON.h"

#include <functional>

using namespace mlir;

namespace acir::compiler {
namespace {

LogicalResult fail(Operation *site, const Twine &message) {
  return site->emitOpError() << message;
}

std::string rtlModuleName(const ConcreteModulePlan &module) {
  return "pyc_rtl_" + module.cppName;
}

std::string valueName(const ConcreteModulePlan &module, ConcreteValueId id) {
  return "pyc_net_" + std::to_string(id);
}

std::string laneExpression(const ConcreteModulePlan &module,
                           ConcreteValueId id) {
  const ConcreteValuePlan &value = module.values[id];
  if (!value.availabilityParent || !value.tableLane)
    return valueName(module, id);
  const auto &parent = module.values[*value.availabilityParent];
  const uint64_t lanes = parent.type.packedWidth / value.type.packedWidth;
  return valueName(module, *value.availabilityParent) + "[" +
         std::to_string((lanes - 1 - *value.tableLane) *
                        value.type.packedWidth) +
         " +: " +
         std::to_string(value.type.packedWidth) + "]";
}

uint64_t physicalObservationWidth(const ConcreteDesignPlan &plan) {
  uint64_t result = 1;
  for (const auto &observation : plan.observations)
    result = std::max(result, observation.payloadWidth);
  return result;
}

std::string svString(StringRef value, bool trustedFormat = false) {
  std::string result = "\"";
  for (char byte : value) {
    // SystemVerilog treats percent escapes in every display-like system task,
    // including $write with no following arguments. Preserve user-authored
    // observation text by escaping a literal percent as %%.
    if (byte == '%' && !trustedFormat) {
      result += "%%";
      continue;
    }
    if (byte == '\\' || byte == '"')
      result.push_back('\\');
    if (byte == '\n')
      result += "\\n";
    else if (byte == '\r')
      result += "\\r";
    else
      result.push_back(byte);
  }
  result.push_back('"');
  return result;
}

std::string jsonString(StringRef value) {
  std::string result;
  llvm::raw_string_ostream out(result);
  out << llvm::json::Value(value);
  out.flush();
  return result;
}

std::string svFormatPayload(StringRef value) {
  std::string result;
  result.reserve(value.size());
  for (char byte : value) {
    if (byte == '%')
      result.push_back('%');
    result.push_back(byte);
  }
  return result;
}

void emitType(raw_ostream &out, uint64_t width) {
  out << "logic [" << (width - 1) << ":0]";
}

void emitConstant(raw_ostream &out, const ConcreteValuePlan &output,
                  ArrayRef<uint64_t> words) {
  const uint64_t width = output.type.packedWidth;
  const uint64_t digits = (width + 3) / 4;
  out << width << "'h";
  for (uint64_t digit = digits; digit-- > 0;) {
    const uint64_t bit = digit * 4;
    const uint64_t word = bit / 64 < words.size() ? words[bit / 64] : 0;
    out << "0123456789abcdef"[(word >> (bit % 64)) & 0xf];
  }
}

void emitConstant4(raw_ostream &out, const ConcreteValuePlan &output,
                   ArrayRef<uint64_t> valueWords,
                   ArrayRef<uint64_t> knownWords,
                   ArrayRef<uint64_t> zWords) {
  const uint64_t width = output.type.packedWidth;
  out << width << "'b";
  for (uint64_t bit = width; bit-- > 0;) {
    auto planeBit = [bit](ArrayRef<uint64_t> words) {
      return bit / 64 < words.size() &&
             ((words[bit / 64] >> (bit % 64)) & 1);
    };
    out << (planeBit(knownWords) ? (planeBit(valueWords) ? '1' : '0')
                                 : (planeBit(zWords) ? 'z' : 'x'));
  }
}

FailureOr<uint64_t> primitivePayloadWidth(const ConcreteChildPlan &child) {
  if (!child.primitive)
    return failure();
  for (const ConcreteTypeBinding &type : child.primitive->types)
    if (type.name.getValue() == "T" && type.packedWidth != 0)
      return type.packedWidth;
  return failure();
}

const VerilogDesignPlan::PhysicalInitializer *
physicalInitializer(const VerilogDesignPlan &plan, unsigned moduleIndex,
                    unsigned childIndex) {
  for (const auto &initializer : plan.physicalInitializers)
    if (initializer.moduleIndex == moduleIndex &&
        initializer.childIndex == childIndex)
      return &initializer;
  return nullptr;
}

struct FourStateConstant {
  llvm::APInt value;
  llvm::APInt known;
  llvm::APInt z;
};

FailureOr<FourStateConstant>
evaluatePhysicalInitializer(const ConcreteModulePlan &module,
                            ConcreteValueId root) {
  llvm::DenseMap<ConcreteValueId, const ConcreteScheduleStep *> producers;
  llvm::DenseSet<ConcreteValueId> ambiguous;
  for (const ConcreteScheduleStep &step : module.concreteSchedule.steps) {
    if (step.kind != ConcreteScheduleStepKind::ComputeValue || !step.compute)
      continue;
    for (ConcreteValueId output : step.outputs)
      if (!producers.try_emplace(output, &step).second)
        ambiguous.insert(output);
  }
  llvm::DenseMap<ConcreteValueId, FourStateConstant> memo;
  llvm::DenseSet<ConcreteValueId> active;
  std::function<FailureOr<FourStateConstant>(ConcreteValueId)> evaluate =
      [&](ConcreteValueId value) -> FailureOr<FourStateConstant> {
    if (value >= module.values.size() || ambiguous.contains(value) ||
        !active.insert(value).second)
      return failure();
    llvm::scope_exit leave([&] { active.erase(value); });
    if (auto known = memo.find(value); known != memo.end())
      return known->second;
    const ConcreteValuePlan &valuePlan = module.values[value];
    if (valuePlan.availabilityParent && valuePlan.tableLane) {
      auto parent = evaluate(*valuePlan.availabilityParent);
      if (failed(parent))
        return failure();
      const auto &parentPlan =
          module.values[*valuePlan.availabilityParent];
      const uint64_t lanes =
          parentPlan.type.packedWidth / valuePlan.type.packedWidth;
      const unsigned low =
          (lanes - 1 - *valuePlan.tableLane) * valuePlan.type.packedWidth;
      FourStateConstant result{
          parent->value.extractBits(valuePlan.type.packedWidth, low),
          parent->known.extractBits(valuePlan.type.packedWidth, low),
          parent->z.extractBits(valuePlan.type.packedWidth, low)};
      memo.try_emplace(value, result);
      return result;
    }
    auto found = producers.find(value);
    if (found == producers.end())
      return failure();
    const ConcreteScheduleStep &step = *found->second;
    const ConcreteComputePlan &compute = *step.compute;
    auto operand = [&](unsigned index) -> FailureOr<FourStateConstant> {
      if (index >= step.inputs.size())
        return failure();
      return evaluate(step.inputs[index]);
    };
    const unsigned width = valuePlan.type.packedWidth;
    FourStateConstant result{llvm::APInt(width, 0), llvm::APInt::getAllOnes(width),
                             llvm::APInt(width, 0)};
    switch (compute.kind) {
    case ConcreteComputeKind::BitsConstant:
      result.value = llvm::APInt(width, compute.constantWords).zextOrTrunc(width);
      break;
    case ConcreteComputeKind::BitsConstant4:
      result.value = llvm::APInt(width, compute.constantWords).zextOrTrunc(width);
      result.known = llvm::APInt(width, compute.knownWords).zextOrTrunc(width);
      result.z = llvm::APInt(width, compute.zWords).zextOrTrunc(width);
      break;
    case ConcreteComputeKind::PackedIdentity: {
      auto input = operand(0);
      if (failed(input))
        return failure();
      result.value = input->value.zextOrTrunc(width);
      result.known = input->known.zextOrTrunc(width);
      result.z = input->z.zextOrTrunc(width);
      break;
    }
    case ConcreteComputeKind::BitsExtract: {
      auto input = operand(0);
      if (failed(input) || !compute.immediate ||
          *compute.immediate + width > input->value.getBitWidth())
        return failure();
      result.value = input->value.extractBits(width, *compute.immediate);
      result.known = input->known.extractBits(width, *compute.immediate);
      result.z = input->z.extractBits(width, *compute.immediate);
      break;
    }
    case ConcreteComputeKind::BitsResize: {
      auto input = operand(0);
      if (failed(input))
        return failure();
      if (compute.opcode == "sext") {
        result.value = input->value.sextOrTrunc(width);
        result.known = input->known.sextOrTrunc(width);
        result.z = input->z.sextOrTrunc(width);
      } else {
        result.value = input->value.zextOrTrunc(width);
        result.known = input->known.zextOrTrunc(width);
        if (width > input->known.getBitWidth())
          result.known.setBits(input->known.getBitWidth(), width);
        result.z = input->z.zextOrTrunc(width);
      }
      break;
    }
    case ConcreteComputeKind::BitsConcat: {
      unsigned low = 0;
      for (ConcreteValueId inputId : llvm::reverse(step.inputs)) {
        auto input = evaluate(inputId);
        if (failed(input) || low + input->value.getBitWidth() > width)
          return failure();
        result.value.insertBits(input->value, low);
        result.known.insertBits(input->known, low);
        result.z.insertBits(input->z, low);
        low += input->value.getBitWidth();
      }
      if (low != width)
        return failure();
      break;
    }
    case ConcreteComputeKind::TableCreate: {
      unsigned low = 0;
      for (ConcreteValueId inputId : step.inputs) {
        auto input = evaluate(inputId);
        if (failed(input) || low + input->value.getBitWidth() > width)
          return failure();
        result.value.insertBits(input->value, low);
        result.known.insertBits(input->known, low);
        result.z.insertBits(input->z, low);
        low += input->value.getBitWidth();
      }
      if (low != width)
        return failure();
      break;
    }
    case ConcreteComputeKind::TableSplat: {
      auto input = operand(0);
      if (failed(input) || width % input->value.getBitWidth() != 0)
        return failure();
      for (unsigned low = 0; low < width; low += input->value.getBitWidth()) {
        result.value.insertBits(input->value, low);
        result.known.insertBits(input->known, low);
        result.z.insertBits(input->z, low);
      }
      break;
    }
    case ConcreteComputeKind::BitsUnary:
    case ConcreteComputeKind::BitsBinary:
    case ConcreteComputeKind::BitsCompare:
    case ConcreteComputeKind::BitsSelect:
    case ConcreteComputeKind::TableGet:
    case ConcreteComputeKind::TableView:
    case ConcreteComputeKind::TableIndex:
    case ConcreteComputeKind::TableMatch:
    case ConcreteComputeKind::TableChoose:
    case ConcreteComputeKind::TableFold:
    case ConcreteComputeKind::ValueMerge:
    case ConcreteComputeKind::EnumFromBits:
      return failure();
    }
    memo.try_emplace(value, result);
    return result;
  };
  return evaluate(root);
}

FailureOr<std::string> computeExpression(const ConcreteModulePlan &module,
                                         const ConcreteScheduleStep &step) {
  const ConcreteComputePlan &compute = *step.compute;
  auto input = [&](unsigned index) { return laneExpression(module, step.inputs[index]); };
  std::string text;
  llvm::raw_string_ostream out(text);
  switch (compute.kind) {
  case ConcreteComputeKind::BitsConstant:
    emitConstant(out, module.values[step.outputs[0]], compute.constantWords);
    break;
  case ConcreteComputeKind::BitsConstant4:
    emitConstant4(out, module.values[step.outputs[0]], compute.constantWords,
                  compute.knownWords, compute.zWords);
    break;
  case ConcreteComputeKind::BitsUnary:
    out << "~(" << input(0) << ")";
    break;
  case ConcreteComputeKind::BitsBinary: {
    StringRef opcode = llvm::StringSwitch<StringRef>(compute.opcode)
                           .Case("bit_and", "&")
                           .Case("bit_or", "|")
                           .Case("bit_xor", "^")
                           .Case("add", "+")
                           .Case("sub", "-")
                           .Case("mul", "*")
                           .Case("udiv", "/")
                           .Case("urem", "%")
                           .Case("shl", "<<")
                           .Case("lshr", ">>")
                           .Case("ashr", ">>>")
                           .Default("");
    if (opcode.empty())
      return failure();
    if (compute.opcode == "ashr")
      out << "$signed(" << input(0) << ") " << opcode << " " << input(1);
    else
      out << "(" << input(0) << ") " << opcode << " (" << input(1) << ")";
    break;
  }
  case ConcreteComputeKind::BitsCompare: {
    StringRef opcode = llvm::StringSwitch<StringRef>(compute.opcode)
                           .Case("EQ", "==")
                           .Case("NE", "!=")
                           .Case("CASE_EQ", "===")
                           .Cases({"ULT", "SLT"}, "<")
                           .Cases({"ULE", "SLE"}, "<=")
                           .Cases({"UGT", "SGT"}, ">")
                           .Cases({"UGE", "SGE"}, ">=")
                           .Default("");
    if (opcode.empty())
      return failure();
    const bool isSigned = compute.opcode.starts_with("S");
    out << (isSigned ? "$signed(" : "(") << input(0) << ") " << opcode
        << " " << (isSigned ? "$signed(" : "(") << input(1) << ")";
    break;
  }
  case ConcreteComputeKind::BitsSelect:
    out << "(" << input(0) << ") ? (" << input(1) << ") : (" << input(2)
        << ")";
    break;
  case ConcreteComputeKind::BitsConcat:
    if (step.compute->opcode == "splat") {
      if (step.inputs.size() != 1 || !step.compute->immediate)
        return failure();
      out << "{" << *step.compute->immediate << "{" << input(0) << "}}";
    } else {
      out << "{";
      for (auto [index, id] : llvm::enumerate(step.inputs)) {
        if (index)
          out << ", ";
        out << laneExpression(module, id);
      }
      out << "}";
    }
    break;
  case ConcreteComputeKind::BitsExtract:
    if (const ConcreteValuePlan &source = module.values[step.inputs[0]];
        source.availabilityParent && source.tableLane) {
      const auto &parent = module.values[*source.availabilityParent];
      const uint64_t lanes =
          parent.type.packedWidth / source.type.packedWidth;
      out << valueName(module, *source.availabilityParent) << "["
          << ((lanes - 1 - *source.tableLane) * source.type.packedWidth +
              *compute.immediate)
          << " +: " << module.values[step.outputs[0]].type.packedWidth << "]";
    } else {
      out << input(0) << "[" << *compute.immediate << " +: "
          << module.values[step.outputs[0]].type.packedWidth << "]";
    }
    break;
  case ConcreteComputeKind::BitsResize:
    if (compute.opcode == "trunc")
      out << input(0) << "[" << module.values[step.outputs[0]].type.packedWidth - 1
          << ":0]";
    else if (compute.opcode == "sext")
      out << "$signed(" << input(0) << ")";
    else
      out << input(0);
    break;
  case ConcreteComputeKind::PackedIdentity:
    out << input(0);
    break;
  case ConcreteComputeKind::TableCreate:
    out << "{";
    for (auto [index, id] : llvm::enumerate(step.inputs)) {
      if (index)
        out << ", ";
      out << laneExpression(module, id);
    }
    out << "}";
    break;
  case ConcreteComputeKind::TableSplat: {
    if (!step.compute->immediate)
      return failure();
    const uint64_t lanes = *step.compute->immediate;
    out << "{" << lanes << "{" << input(0) << "}}";
    break;
  }
  case ConcreteComputeKind::TableGet:
  case ConcreteComputeKind::TableView:
  case ConcreteComputeKind::TableIndex:
  case ConcreteComputeKind::TableMatch:
  case ConcreteComputeKind::TableChoose:
  case ConcreteComputeKind::TableFold:
  case ConcreteComputeKind::ValueMerge:
  case ConcreteComputeKind::EnumFromBits:
    return failure();
  }
  out.flush();
  return text;
}

LogicalResult emitModule(const VerilogDesignPlan &verilog, unsigned moduleIndex,
                         raw_ostream &out) {
  const ConcreteDesignPlan &plan = verilog.design;
  const ConcreteModulePlan &module = plan.modules[moduleIndex];
  const bool managed = verilog.mode == VerilogExportMode::Managed &&
                       (plan.system.has_value() || plan.sourceCheckCount != 0);
  out << "module " << rtlModuleName(module) << " (\n";
  bool first = true;
  auto port = [&](StringRef direction, StringRef name, uint64_t width) {
    if (!first)
      out << ",\n";
    out << "  " << direction << " wire ";
    emitType(out, width);
    out << " " << name;
    first = false;
  };
  for (const ConcretePortPlan &input : module.inputs)
    port("input", input.cppName, input.type.packedWidth);
  for (const ConcretePortPlan &output : module.outputs)
    port("output", output.cppName, output.type.packedWidth);
  if (managed) {
    port("input", "pyc_phase", 3);
    port("input", "pyc_root_commit_ok", 1);
    port("input", "pyc_reset_active", 1);
    port("output", "pyc_check_complete", 1);
    port("output", "pyc_sampling_error", 1);
    port("output", "pyc_local_error", 1);
  } else if (verilog.mode == VerilogExportMode::Physical) {
    port("input", "pyc_root_commit_ok", 1);
    port("input", "pyc_reset_active", 1);
    port("output", "pyc_local_error", 1);
    if (module.observationSubtreeWidth != 0) {
      port("output", "pyc_observation_path_raw",
           module.observationSubtreeWidth);
      port("output", "pyc_observation_value_raw",
           module.observationSubtreeWidth * physicalObservationWidth(plan));
    }
  }
  out << "\n);\n";

  for (const ConcreteValuePlan &value : module.values) {
    if (value.kind == ConcreteValueKind::TableElement &&
        value.availabilityParent && value.tableLane)
      continue;
    out << "  wire ";
    emitType(out, value.type.packedWidth);
    out << " " << valueName(module, value.id) << ";\n";
  }
  for (const ConcretePortPlan &input : module.inputs)
    out << "  assign " << valueName(module, input.value) << " = "
        << input.cppName << ";\n";
  if (verilog.mode == VerilogExportMode::Physical) {
    const uint64_t observationWidth = physicalObservationWidth(plan);
    for (auto [siteIndex, site] : llvm::enumerate(module.observationSites)) {
      out << "  assign pyc_observation_path_raw[" << siteIndex << "] = "
          << laneExpression(module, site.path) << ";\n  assign "
             "pyc_observation_value_raw[" << (siteIndex * observationWidth)
          << " +: " << observationWidth << "] = ";
      if (site.payloadWidth < observationWidth)
        out << "{{" << (observationWidth - site.payloadWidth)
            << "{1'b0}}, ";
      out << laneExpression(module, site.value);
      if (site.payloadWidth < observationWidth)
        out << "}";
      out << ";\n";
    }
  }

  if (managed)
    for (auto [siteIndex, site] : llvm::enumerate(module.observationSites))
      out << "  logic pyc_observation_pending_" << siteIndex
          << " = 1'b0;\n  logic pyc_observation_valid_" << siteIndex
          << " = 1'b0;\n  logic pyc_observation_path_" << siteIndex
          << " = 1'b0;\n  logic [" << (site.payloadWidth - 1)
          << ":0] pyc_observation_value_" << siteIndex << " = '0;\n"
          << "  always @(pyc_phase) begin\n    case (pyc_phase)\n"
          << "      3'd1: begin\n        pyc_observation_value_" << siteIndex
          << " = " << laneExpression(module, site.value)
          << ";\n        pyc_observation_valid_" << siteIndex
          << " = !$isunknown(" << laneExpression(module, site.value)
          << ") && !$isunknown(" << laneExpression(module, site.path)
          << ");\n        pyc_observation_path_" << siteIndex << " = ("
          << laneExpression(module, site.path)
          << " === 1'b1);\n        pyc_observation_pending_" << siteIndex
          << " = 1'b1;\n      end\n"
          << "      3'd0, 3'd3, 3'd4: begin\n"
          << "        pyc_observation_pending_" << siteIndex
          << " = 1'b0;\n        pyc_observation_valid_" << siteIndex
          << " = 1'b0;\n        pyc_observation_path_" << siteIndex
          << " = 1'b0;\n        pyc_observation_value_" << siteIndex
          << " = '0;\n      end\n    endcase\n  end\n";

  llvm::SmallVector<std::string> childErrors, childCompletions,
      childSamplingErrors;
  for (auto [childIndex, child] : llvm::enumerate(module.children)) {
    if (!child.primitive || child.kind == ConcreteChildKind::Module) {
      if (child.kind != ConcreteChildKind::Module || !child.moduleIndex)
        return fail(child.occurrence, "Verilog child plan is incomplete");
    } else if (child.primitive->inputMembers.size() != child.inputs.size() ||
               child.primitive->outputMembers.size() != child.outputs.size()) {
      return fail(child.occurrence, "Verilog primitive pin plan is incomplete");
    }
    const bool lifecycle = managed || verilog.mode == VerilogExportMode::Physical;
    const std::string error = "pyc_child_error_" + std::to_string(childIndex);
    if (lifecycle) {
      out << "  wire " << error << ";\n";
      childErrors.push_back(error);
    }
    uint64_t childObservationBase = module.observationSites.size();
    for (unsigned prior = 0; prior < childIndex; ++prior) {
      const auto &sibling = module.children[prior];
      if (sibling.kind == ConcreteChildKind::Module && sibling.moduleIndex)
        childObservationBase +=
            plan.modules[*sibling.moduleIndex].observationSubtreeWidth;
    }
    std::string complete, sampling;
    if (child.kind == ConcreteChildKind::Module &&
        managed) {
      complete = "pyc_child_complete_" + std::to_string(childIndex);
      sampling = "pyc_child_sampling_" + std::to_string(childIndex);
      out << "  wire " << complete << ";\n  wire " << sampling << ";\n";
      childCompletions.push_back(complete);
      childSamplingErrors.push_back(sampling);
    }
    std::string moduleKind =
        child.kind == ConcreteChildKind::Module
            ? rtlModuleName(plan.modules[*child.moduleIndex])
            : child.primitiveKind.getValue().str();
    out << "  " << moduleKind;
    if (child.kind != ConcreteChildKind::Module) {
      const ConcretePrimitiveConfig &primitive = *child.primitive;
      auto payloadWidth = primitivePayloadWidth(child);
      if (failed(payloadWidth))
        return fail(child.occurrence,
                    "Verilog primitive plan lacks concrete payload type T");
      out << " #( .T(";
      emitType(out, *payloadWidth);
      out << ")";
      for (const auto &integer : primitive.integers)
        if (!(child.kind == ConcreteChildKind::Queue &&
              (integer.name.getValue() == "D" ||
               integer.name.getValue() == "L")))
          out << ", ." << integer.name.getValue() << "("
              << integer.canonicalValue << ")";
      if (child.kind == ConcreteChildKind::Queue && child.queue)
        out << ", .DEPTH(" << child.queue->depth << "), .READY_POLICY("
            << (child.queue->readyPolicy == ac::QueueReadyPolicy::LocalOccupancy ? 0 : 1)
            << "), .AVAILABILITY_LATENCY(" << child.queue->availabilityLatency
            << ")";
      if (managed)
        out << ", .pyc_managed(1'b1)";
      else if (verilog.mode == VerilogExportMode::Physical) {
        out << ", .pyc_physical(1'b1)";
        StringRef kind = child.primitiveKind.getValue();
        if (kind == "dff" || kind == "dffe") {
          const auto *initializer =
              physicalInitializer(verilog, moduleIndex, childIndex);
          if (!initializer)
            return fail(child.occurrence,
                        "physical RTL primitive lacks a static initializer");
          out << ", .pyc_initial(";
          emitConstant4(out, module.values[child.inputs.back().value],
                        initializer->words, initializer->knownWords,
                        initializer->zWords);
          out << ")";
        }
      }
      out << " )";
    }
    out << " pyc_instance_" << childIndex << " (\n";
    bool connectionFirst = true;
    auto connect = [&](StringRef name, StringRef expression) {
      if (!connectionFirst)
        out << ",\n";
      out << "    ." << name << "(" << expression << ")";
      connectionFirst = false;
    };
    for (auto [index, binding] : llvm::enumerate(child.inputs)) {
      StringRef pin = child.kind == ConcreteChildKind::Module
                          ? plan.modules[*child.moduleIndex].inputs[index].cppName
                          : StringRef(child.primitive->inputMembers[index]);
      connect(pin, laneExpression(module, binding.value));
    }
    for (auto [index, binding] : llvm::enumerate(child.outputs)) {
      StringRef pin = child.kind == ConcreteChildKind::Module
                          ? plan.modules[*child.moduleIndex].outputs[index].cppName
                          : StringRef(child.primitive->outputMembers[index]);
      connect(pin, laneExpression(module, binding.value));
    }
    if (managed) {
      connect("pyc_phase", "pyc_phase");
      connect("pyc_root_commit_ok", "pyc_root_commit_ok");
      if (child.kind == ConcreteChildKind::Module) {
        connect("pyc_reset_active", "pyc_reset_active");
        connect("pyc_check_complete", complete);
        connect("pyc_sampling_error", sampling);
      }
      connect("pyc_local_error", error);
    } else if (verilog.mode == VerilogExportMode::Physical) {
      connect("pyc_root_commit_ok", "pyc_root_commit_ok");
      if (child.kind == ConcreteChildKind::Module)
        connect("pyc_reset_active", "pyc_reset_active");
      connect("pyc_local_error", error);
      if (child.kind == ConcreteChildKind::Module) {
        const uint64_t slots =
            plan.modules[*child.moduleIndex].observationSubtreeWidth;
        if (slots != 0) {
          const uint64_t width = physicalObservationWidth(plan);
          connect("pyc_observation_path_raw",
                  (Twine("pyc_observation_path_raw[") +
                   Twine(childObservationBase) + " +: " + Twine(slots) + "]")
                      .str());
          connect("pyc_observation_value_raw",
                  (Twine("pyc_observation_value_raw[") +
                   Twine(childObservationBase * width) + " +: " +
                   Twine(slots * width) + "]")
                      .str());
        }
      }
    }
    out << "\n  );\n";
  }

  for (const ConcreteScheduleStep &step : module.concreteSchedule.steps) {
    if (step.kind == ConcreteScheduleStepKind::ComputeValue) {
      if (!step.compute)
        return fail(step.operation, "Verilog compute plan is absent");
      if (step.compute->kind == ConcreteComputeKind::TableGet) {
        const auto &table = module.values[step.inputs[0]];
        const auto &value = module.values[step.outputs[0]];
        const uint64_t lanes = table.type.packedWidth / value.type.packedWidth;
        out << "  assign " << laneExpression(module, step.outputs[0]) << " = ("
            << laneExpression(module, step.inputs[1]) << " < " << lanes << ") ? "
            << valueName(module, step.inputs[0]) << "[((" << lanes
            << " - 1 - " << laneExpression(module, step.inputs[1]) << ") * "
            << value.type.packedWidth << ") +: " << value.type.packedWidth
            << "] : 'x;\n  assign " << laneExpression(module, step.outputs[1])
            << " = (" << laneExpression(module, step.inputs[1]) << " < "
            << lanes << ");\n";
      } else if (step.compute->kind == ConcreteComputeKind::EnumFromBits) {
        out << "  assign " << laneExpression(module, step.outputs[0]) << " = "
            << laneExpression(module, step.inputs[0]) << ";\n  assign "
            << laneExpression(module, step.outputs[1]) << " = ";
        for (auto [index, words] : llvm::enumerate(step.compute->enumCodes)) {
          if (index)
            out << " | ";
          out << "(" << laneExpression(module, step.inputs[0]) << " == ";
          emitConstant(out, module.values[step.inputs[0]], words);
          out << ")";
        }
        out << ";\n";
      } else if (step.compute->kind == ConcreteComputeKind::TableView) {
        const auto &input = module.values[step.inputs[0]];
        const auto &output = module.values[step.outputs[0]];
        const uint64_t width = output.type.packedWidth / step.compute->laneMap.size();
        for (auto [lane, source] : llvm::enumerate(step.compute->laneMap))
          out << "  assign " << valueName(module, output.id) << "["
              << (step.compute->laneMap.size() - 1 - lane) * width
              << " +: " << width << "] = "
              << valueName(module, input.id) << "["
              << (input.type.packedWidth / width - 1 - source) * width
              << " +: " << width << "];\n";
      } else if (step.compute->kind == ConcreteComputeKind::TableIndex) {
        const auto &output = module.values[step.outputs[0]];
        uint64_t total = 1;
        llvm::SmallVector<uint64_t> strides(step.compute->laneMap.size());
        for (size_t i = step.compute->laneMap.size(); i-- > 0;) {
          strides[i] = total;
          total *= step.compute->laneMap[i];
        }
        out << "  assign " << valueName(module, output.id) << " = (";
        for (auto [i, inputId] : llvm::enumerate(step.inputs)) {
          if (i) out << " && ";
          out << "($unsigned(" << laneExpression(module, inputId) << ") < "
              << step.compute->laneMap[i] << ")";
        }
        out << ") ? ";
        for (auto [i, inputId] : llvm::enumerate(step.inputs)) {
          if (i) out << " + ";
          out << "$unsigned(" << laneExpression(module, inputId) << ") * "
              << strides[i];
        }
        // A definitely out-of-range coordinate dominates unknown coordinates
        // on other axes. Only when no axis is known OOB do X/Z inputs make
        // the flattened ordinal unknown.
        out << " : (";
        for (auto [i, inputId] : llvm::enumerate(step.inputs)) {
          if (i) out << " || ";
          out << "(!$isunknown(" << laneExpression(module, inputId)
              << ") && ($unsigned(" << laneExpression(module, inputId)
              << ") >= " << step.compute->laneMap[i] << "))";
        }
        out << ") ? " << total << " : 'x;\n";
      } else if (step.compute->kind == ConcreteComputeKind::TableMatch) {
        out << "  assign " << laneExpression(module, step.outputs[0]) << " = {";
        for (auto [i, id] : llvm::enumerate(llvm::reverse(step.inputs))) {
          if (i) out << ", ";
          out << laneExpression(module, id);
        }
        out << "};\n";
      } else if (step.compute->kind == ConcreteComputeKind::TableChoose) {
        const uint64_t count = *step.compute->immediate;
        const uint64_t maskWidth = module.values[step.inputs[1]].type.packedWidth;
        const uint64_t indexWidth = module.values[step.outputs[0]].type.packedWidth;
        std::string prefix = "pyc_choice_" + std::to_string(step.outputs[0]);
        out << "  wire [" << (maskWidth - 1) << ":0] " << prefix
            << "_mask [0:" << count << "];\n  wire [" << (indexWidth - 1)
            << ":0] " << prefix << "_index [0:" << (count - 1)
            << "][0:" << maskWidth << "];\n  wire " << prefix
            << "_valid [0:" << (count - 1) << "];\n  assign " << prefix
            << "_mask[0] = " << laneExpression(module, step.inputs[1]) << ";\n";
        for (uint64_t rank = 0; rank < count; ++rank) {
          out << "  assign " << prefix << "_index[" << rank << "][0] = '0;\n"
              << "  assign " << prefix << "_valid[" << rank << "] = |"
              << prefix << "_mask[" << rank << "];\n";
          for (uint64_t scan = 0; scan < maskWidth; ++scan) {
            uint64_t bit = step.compute->opcode == "low" ? maskWidth - 1 - scan : scan;
            out << "  assign " << prefix << "_index[" << rank << "][" << (scan + 1)
                << "] = " << prefix << "_mask[" << rank << "][" << bit << "] ? "
                << indexWidth << "'d" << bit << " : " << prefix << "_index["
                << rank << "][" << scan << "];\n";
          }
          for (uint64_t bit = 0; bit < maskWidth; ++bit)
            out << "  assign " << prefix << "_mask[" << (rank + 1) << "][" << bit
                << "] = " << prefix << "_mask[" << rank << "][" << bit
                << "] & ~(" << prefix << "_valid[" << rank << "] & ("
                << prefix << "_index[" << rank << "][" << maskWidth << "] == "
                << indexWidth << "'d" << bit << "));\n";
          out << "  assign " << laneExpression(module, step.outputs[rank]) << " = "
              << prefix << "_index[" << rank << "][" << maskWidth << "];\n"
              << "  assign " << laneExpression(module, step.outputs[count + rank])
              << " = " << prefix << "_valid[" << rank << "];\n";
        }
      } else if (step.compute->kind == ConcreteComputeKind::TableFold) {
        const auto &input = module.values[step.inputs[0]];
        const auto &output = module.values[step.outputs[0]];
        const uint64_t lanes = input.type.packedWidth / output.type.packedWidth;
        std::string prefix = "pyc_fold_" + std::to_string(output.id);
        out << "  wire [" << (output.type.packedWidth - 1) << ":0] " << prefix
            << " [0:" << (lanes - 1) << "];\n";
        for (uint64_t lane = 0; lane < lanes; ++lane)
          out << "  assign " << prefix << "[" << lane << "] = "
              << valueName(module, input.id) << "["
              << (lanes - 1 - lane) * output.type.packedWidth
              << " +: " << output.type.packedWidth << "];\n";
        uint64_t nodes = lanes, base = 0, next = lanes;
        while (nodes > 1) {
          for (uint64_t pair = 0; pair < nodes / 2; ++pair) {
            out << "  wire [" << (output.type.packedWidth - 1) << ":0] " << prefix
                << "_n" << next << " = ";
            std::string a = prefix + (base ? "_n" + std::to_string(base + 2 * pair) : "[" + std::to_string(2 * pair) + "]");
            std::string b = prefix + (base ? "_n" + std::to_string(base + 2 * pair + 1) : "[" + std::to_string(2 * pair + 1) + "]");
            if (step.compute->opcode == "min" || step.compute->opcode == "max")
              out << "(" << a << (step.compute->opcode == "min" ? " <= " : " >= ") << b << ") ? " << a << " : " << b;
            else
              out << a << " " << llvm::StringSwitch<StringRef>(step.compute->opcode).Case("add", "+").Case("mul", "*").Case("and", "&").Case("or", "|").Case("xor", "^").Default("") << " " << b;
            out << ";\n"; ++next;
          }
          if (nodes % 2) {
            std::string a = prefix + (base ? "_n" + std::to_string(base + nodes - 1) : "[" + std::to_string(nodes - 1) + "]");
            out << "  wire [" << (output.type.packedWidth - 1) << ":0] " << prefix
                << "_n" << next++ << " = " << a << ";\n";
          }
          base = next - ((nodes + 1) / 2); nodes = (nodes + 1) / 2;
        }
        std::string root = lanes == 1 ? prefix + "[0]" : prefix + "_n" + std::to_string(base);
        out << "  assign " << valueName(module, output.id) << " = " << root << ";\n";
      } else if (step.compute->kind == ConcreteComputeKind::ValueMerge) {
        const auto &base = module.values[step.inputs[0]];
        const auto &output = module.values[step.outputs[0]];
        const uint64_t writes = (step.inputs.size() - 1) / 2;
        struct Segment { uint64_t low, width; int write; };
        llvm::SmallVector<Segment> segments;
        llvm::SmallVector<Segment> writeSegments;
        writeSegments.reserve(writes);
        for (uint64_t i = 0; i < writes; ++i) {
          const uint64_t low = step.compute->laneMap[2 * i];
          const uint64_t width = step.compute->laneMap[2 * i + 1];
          if (width == 0 || low > output.type.packedWidth ||
              width > output.type.packedWidth - low)
            return fail(step.operation,
                        "Verilog value merge has an invalid packed lane");
          writeSegments.push_back({low, width, static_cast<int>(i)});
        }
        // Source field order follows declaration order, while packed offsets
        // need not be ascending (the first declared field is commonly the
        // most-significant lane).  Build the partition in packed-bit order so
        // gaps are emitted exactly once and the concatenation keeps its width.
        llvm::sort(writeSegments, [](const Segment &a, const Segment &b) {
          return a.low < b.low;
        });
        uint64_t cursor = 0;
        for (const Segment &write : writeSegments) {
          if (write.low < cursor)
            return fail(step.operation,
                        "Verilog value merge has overlapping packed lanes");
          if (cursor < write.low)
            segments.push_back({cursor, write.low - cursor, -1});
          segments.push_back(write);
          cursor = write.low + write.width;
        }
        if (cursor < output.type.packedWidth) segments.push_back({cursor, output.type.packedWidth - cursor, -1});
        llvm::sort(segments, [](const Segment &a, const Segment &b) { return a.low > b.low; });
        out << "  assign " << laneExpression(module, output.id) << " = {";
        for (auto [i, segment] : llvm::enumerate(segments)) {
          if (i) out << ", ";
          if (segment.write < 0) {
            if (base.availabilityParent && base.tableLane)
              out << valueName(module, *base.availabilityParent) << "["
                  << ((module.values[*base.availabilityParent].type.packedWidth /
                           base.type.packedWidth -
                       1 - *base.tableLane) *
                          base.type.packedWidth +
                      segment.low)
                  << " +: " << segment.width << "]";
            else
              out << laneExpression(module, base.id) << "[" << segment.low
                  << " +: " << segment.width << "]";
          }
          else {
            const uint64_t write = segment.write;
            out << laneExpression(module, step.inputs[1 + write]) << " ? "
                << laneExpression(module, step.inputs[1 + writes + write])
                << " : ";
            if (base.availabilityParent && base.tableLane)
              out << valueName(module, *base.availabilityParent) << "["
                  << ((module.values[*base.availabilityParent].type.packedWidth /
                           base.type.packedWidth -
                       1 - *base.tableLane) *
                          base.type.packedWidth +
                      segment.low)
                  << " +: " << segment.width << "]";
            else
              out << laneExpression(module, base.id) << "[" << segment.low
                  << " +: " << segment.width << "]";
          }
        }
        out << "};\n  assign " << laneExpression(module, step.outputs[1]) << " = ";
        out << "1'b0";
        for (uint64_t i = 0; i < writes; ++i) {
          out << " | ";
          const std::string enable = laneExpression(module, step.inputs[1 + i]);
          out << enable;
        }
        out << ";\n";
      } else {
        auto expression = computeExpression(module, step);
        if (failed(expression))
          return fail(step.operation, "Verilog renderer lacks a planned compute mapping");
        out << "  assign " << laneExpression(module, step.outputs[0]) << " = "
            << *expression << ";\n";
      }
    } else if (step.kind == ConcreteScheduleStepKind::ReadChildOutputField) {
      if (!step.childIndex || !step.portIndex || !step.fieldLow ||
          step.outputs.size() != 1)
        return fail(step.operation,
                    "Verilog child output field read is malformed");
      const ConcreteChildPlan &child = module.children[*step.childIndex];
      if (*step.portIndex >= child.outputs.size())
        return fail(step.operation,
                    "Verilog child output field port is out of range");
      const ConcreteValuePlan &aggregate =
          module.values[child.outputs[*step.portIndex].value];
      const ConcreteValuePlan &field = module.values[step.outputs.front()];
      out << "  assign " << laneExpression(module, field.id) << " = "
          << laneExpression(module, aggregate.id) << "[" << *step.fieldLow
          << " +: " << field.type.packedWidth << "];\n";
    } else if (step.kind == ConcreteScheduleStepKind::MaterializeOutput) {
      // Field materialization steps partition aggregate refresh work for the
      // imperative Structural C++ backend. Verilog is a continuous dataflow
      // network: the complete yielded aggregate already has one canonical
      // driver, and publishing its fields again would multiply-drive the
      // entire output port.
      if (step.fieldLow)
        continue;
      out << "  assign " << module.outputs[*step.portIndex].cppName << " = "
          << laneExpression(module, step.inputs[0]) << ";\n";
    } else if (step.kind == ConcreteScheduleStepKind::AvailabilityBarrier) {
      if (step.outputs.size() != 1 || step.inputs.empty())
        return fail(step.operation, "Verilog availability barrier is malformed");
      const auto &output = module.values[step.outputs.front()];
      const auto &first = module.values[step.inputs.front()];
      // Collection/table-map lane IDs already denote slices of the parent
      // packed wire. The barrier carries dependency availability only.
      if (first.availabilityParent != output.id)
        return fail(step.operation,
                    "Verilog availability lanes disagree with parent");
    }
  }

  llvm::SmallVector<std::string> localErrors = childErrors;
  llvm::SmallVector<std::string> localSampling = childSamplingErrors;
  llvm::SmallVector<std::string> localComplete = childCompletions;
  for (auto [checkIndex, check] : llvm::enumerate(plan.checks)) {
    if (check.moduleIndex != moduleIndex)
      continue;
    std::string complete = "pyc_check_complete_" + std::to_string(checkIndex);
    std::string error = "pyc_check_error_" + std::to_string(checkIndex);
    const std::string condition = laneExpression(module, check.condition);
    const std::string path = laneExpression(module, check.path);
    const std::string reset =
        module.resetInput ? laneExpression(module, module.inputs[*module.resetInput].value)
                          : "pyc_reset_active";
    if (verilog.mode == VerilogExportMode::Physical) {
      out << "`ifndef SYNTHESIS\n  wire " << error << " = $isunknown("
          << path << ") || ((pyc_reset_active === 1'b0) && (" << path
          << " === 1'b1) && $isunknown(" << condition
          << "));\n`else\n  wire " << error << " = 1'b0;\n`endif\n";
      localErrors.push_back(error);
      continue;
    }
    if (!managed)
      continue;
    if (check.kind == "guard_known") {
      if (check.sampling.size() != 2)
        return fail(module.key.definition,
                    "managed guard_known plan requires clock/reset sampling");
      const std::string prefix = "pyc_sampling_" + std::to_string(checkIndex);
      out << "`ifndef SYNTHESIS\n"
          << "  logic " << prefix << "_clock = 1'b0;\n"
          << "  logic " << prefix << "_pending_clock = 1'b0;\n"
          << "  logic " << prefix << "_pending_valid = 1'b0;\n"
          << "  logic " << prefix << "_rising = 1'b0;\n"
          << "  logic " << prefix << "_reset_active = 1'b0;\n"
          << "  logic [1:0] " << prefix << "_fault = 2'd0;\n"
          << "  logic " << error << "_condition = 1'b0;\n"
          << "  logic " << error << "_path = 1'b0;\n"
          << "  logic " << complete << " = 1'b0;\n"
          << "  always @(pyc_phase) begin\n    case (pyc_phase)\n"
          << "      3'd1: begin\n        " << complete << " = 1'b0;\n"
          << "        " << prefix << "_pending_valid = 1'b0;\n"
          << "        " << prefix << "_fault = 2'd0;\n"
          << "        " << prefix << "_rising = 1'b0;\n"
          << "        " << prefix << "_reset_active = 1'b0;\n"
          << "        " << error << "_condition = " << condition << ";\n"
          << "        " << error << "_path = " << path << ";\n"
          << "        if ($isunknown("
          << laneExpression(module, check.sampling[0]) << "))\n"
          << "          " << prefix << "_fault = 2'd1;\n"
          << "        else begin\n          " << prefix << "_rising = ("
          << prefix << "_clock === 1'b0) && ("
          << laneExpression(module, check.sampling[0]) << " === 1'b1);\n"
          << "          if (" << prefix << "_rising && $isunknown("
          << laneExpression(module, check.sampling[1]) << "))\n"
          << "            " << prefix << "_fault = 2'd2;\n"
          << "          else begin\n            " << prefix
          << "_reset_active = " << prefix << "_rising && ("
          << laneExpression(module, check.sampling[1]) << " === 1'b1);\n"
          << "            " << prefix << "_pending_clock = "
          << laneExpression(module, check.sampling[0]) << ";\n"
          << "            " << prefix << "_pending_valid = 1'b1;\n"
          << "          end\n        end\n        " << complete
          << " = 1'b1;\n      end\n"
          << "      3'd2: begin\n        if ((pyc_root_commit_ok === 1'b1) && ("
          << prefix << "_pending_valid === 1'b1)) begin\n          " << prefix
          << "_clock = " << prefix << "_pending_clock;\n          " << prefix
          << "_pending_valid = 1'b0;\n        end\n"
          << "        if (pyc_root_commit_ok === 1'b1) " << complete
          << " = 1'b0;\n      end\n"
          << "      3'd3, 3'd4: begin\n        " << complete
          << " = 1'b0;\n        " << error << "_condition = 1'b0;\n        "
          << error << "_path = 1'b0;\n        " << prefix
          << "_fault = 2'd0;\n        " << prefix
          << "_pending_clock = 1'b0;\n        " << prefix
          << "_pending_valid = (pyc_phase == 3'd4);\n      end\n"
          << "      default: begin end\n    endcase\n  end\n"
          << "  wire " << error << " = $isunknown(" << error
          << "_path) || (" << prefix << "_rising && !" << prefix
          << "_reset_active && (" << error
          << "_path === 1'b1) && $isunknown(" << error
          << "_condition));\n`else\n  wire " << complete
          << " = 1'b1;\n  wire [1:0] " << prefix
          << "_fault = 2'd0;\n  wire " << error << " = 1'b0;\n`endif\n";
      localComplete.push_back(complete);
      localSampling.push_back("(" + prefix + "_fault != 2'd0)");
      localErrors.push_back(error);
      continue;
    }
    out << "`ifndef SYNTHESIS\n  logic " << error
        << "_condition = 1'b0;\n  logic " << error
        << "_path = 1'b0;\n  logic " << error
        << "_reset = 1'b0;\n  logic " << complete
        << " = 1'b0;\n  always @(pyc_phase) begin\n    case (pyc_phase)\n"
        << "      3'd1: begin\n        " << complete << " = 1'b0;\n        "
        << error << "_condition = " << condition << ";\n        " << error
        << "_path = " << path << ";\n        " << error << "_reset = "
        << reset << ";\n        " << complete << " = 1'b1;\n      end\n"
        << "      3'd2: if (pyc_root_commit_ok === 1'b1) " << complete
        << " = 1'b0;\n      3'd3, 3'd4: begin\n        " << complete
        << " = 1'b0;\n        " << error << "_condition = 1'b0;\n        "
        << error << "_path = 1'b0;\n        " << error
        << "_reset = 1'b0;\n      end\n      default: begin end\n"
        << "    endcase\n  end\n  wire " << error << " = ((" << error
        << "_path & ~" << error << "_reset & ~" << error
        << "_condition) !== 1'b0);\n`else\n  wire " << complete
        << " = 1'b1;\n  wire " << error << " = 1'b0;\n`endif\n";
    localComplete.push_back(complete);
    localErrors.push_back(error);
  }
  auto reduction = [&](ArrayRef<std::string> values, StringRef op,
                       StringRef empty) {
    if (values.empty()) {
      out << empty;
      return;
    }
    for (auto [index, value] : llvm::enumerate(values)) {
      if (index)
        out << " " << op << " ";
      out << value;
    }
  };
  if (managed) {
    out << "  assign pyc_check_complete = ";
    reduction(localComplete, "&", "1'b1");
    out << ";\n  assign pyc_sampling_error = ";
    reduction(localSampling, "|", "1'b0");
    out << ";\n  assign pyc_local_error = (pyc_check_complete !== 1'b1) | pyc_sampling_error";
    for (const std::string &error : localErrors)
      out << " | " << error;
    out << ";\n";
  } else if (verilog.mode == VerilogExportMode::Physical) {
    out << "  assign pyc_local_error = ";
    llvm::SmallVector<std::string> all = localErrors;
    all.append(localSampling.begin(), localSampling.end());
    reduction(all, "|", "1'b0");
    out << ";\n";
  }
  out << "endmodule\n";
  return success();
}

LogicalResult emitWrapper(const VerilogDesignPlan &verilog, raw_ostream &out) {
  const ConcreteDesignPlan &plan = verilog.design;
  const ConcreteModulePlan &root = plan.modules[plan.rootModule];
  const bool managed = verilog.mode == VerilogExportMode::Managed &&
                       (plan.system.has_value() || plan.sourceCheckCount != 0);
  out << "module pyc_root (\n";
  bool first = true;
  auto port = [&](StringRef direction, const ConcretePortPlan &value) {
    if (!first)
      out << ",\n";
    out << "  " << direction << " wire ";
    emitType(out, value.type.packedWidth);
    out << " " << value.cppName;
    first = false;
  };
  auto exposedInput = [&](unsigned index) {
    return index < root.publicInputCount ||
           (verilog.mode == VerilogExportMode::Physical &&
            ((root.clockInput && index == *root.clockInput) ||
             (root.resetInput && index == *root.resetInput)));
  };
  for (unsigned index = 0; index < root.inputs.size(); ++index)
    if (exposedInput(index))
      port("input", root.inputs[index]);
  for (unsigned index = 0; index < root.publicOutputCount; ++index)
    port("output", root.outputs[index]);
  auto scalarPort = [&](StringRef direction, StringRef name, unsigned width = 1) {
    if (!first)
      out << ",\n";
    out << "  " << direction << " wire ";
    emitType(out, width);
    out << " " << name;
    first = false;
  };
  if (managed) {
    scalarPort("input", "pyc_phase", 3);
    scalarPort("input", "pyc_root_commit_ok");
    scalarPort("output", "pyc_check_complete");
    scalarPort("output", "pyc_sampling_error");
    scalarPort("output", "pyc_local_error");
  } else if (verilog.mode == VerilogExportMode::Physical &&
             !plan.observations.empty()) {
    uint64_t maximumWidth = 1;
    for (const auto &observation : plan.observations)
      maximumWidth = std::max(maximumWidth, observation.payloadWidth);
    scalarPort("output", "pyc_observation_valid", plan.observations.size());
    scalarPort("output", "pyc_observation_path", plan.observations.size());
    scalarPort("output", "pyc_observation_value",
               plan.observations.size() * maximumWidth);
    scalarPort("output", "pyc_observation_known",
               plan.observations.size() * maximumWidth);
    scalarPort("output", "pyc_observation_z",
               plan.observations.size() * maximumWidth);
  }
  out << "\n);\n";
  if (verilog.mode == VerilogExportMode::Physical) {
    const uint64_t maximumWidth = physicalObservationWidth(plan);
    out << "  wire pyc_local_error;\n  wire pyc_root_commit_ok;\n";
    if (!plan.observations.empty())
      out << "  wire [" << (plan.observations.size() - 1)
          << ":0] pyc_observation_path_raw;\n  wire ["
          << (plan.observations.size() * maximumWidth - 1)
          << ":0] pyc_observation_value_raw;\n";
    out <<
           "`ifndef SYNTHESIS\n  bit pyc_physical_fault = 1'b0;\n"
           "  wire pyc_observation_error = ";
    if (plan.observations.empty()) {
      out << "1'b0";
    } else {
      for (auto [index, observation] : llvm::enumerate(plan.observations)) {
        if (index)
          out << " | ";
        const std::string path =
            "pyc_observation_path_raw[" + std::to_string(index) + "]";
        const std::string value =
            "pyc_observation_value_raw[" +
            std::to_string(index * maximumWidth) + " +: " +
            std::to_string(observation.payloadWidth) + "]";
        out << "(($isunknown(" << path << "))";
        if (observation.kind == ConcreteObservationKind::Gauge)
          out << " | ((" << path << " === 1'b1) && $isunknown(" << value << "))";
        if (observation.groupIndex != 0) {
          const auto &first = plan.observations[index - observation.groupIndex];
          out << " | (!$isunknown(" << path << ") && (" << path << " !== "
              << "pyc_observation_path_raw["
              << (index - observation.groupIndex) << "]))";
        }
        out << ")";
      }
    }
    out << ";\n  wire pyc_physical_error = pyc_local_error | "
           "pyc_observation_error | $isunknown("
        << root.inputs[*root.resetInput].cppName
        << ");\n  assign pyc_root_commit_ok = !pyc_physical_fault && !pyc_physical_error;\n"
           "  always @("
        << root.inputs[*root.clockInput].cppName << ") begin\n    if ($isunknown("
        << root.inputs[*root.clockInput].cppName << ") || (("
        << root.inputs[*root.clockInput].cppName
        << " === 1'b1) && pyc_physical_error)) begin\n"
           "      if (!pyc_physical_fault) $display(\"PYCIRCUIT_PHYSICAL_FAULT\");\n"
           "      pyc_physical_fault = 1'b1;\n    end\n  end\n"
           "`else\n  assign pyc_root_commit_ok = 1'b1;\n`endif\n";
    if (!plan.observations.empty()) {
      const uint64_t count = plan.observations.size();
      const std::string &clock = root.inputs[*root.clockInput].cppName;
      const std::string &reset = root.inputs[*root.resetInput].cppName;
      out << "  logic [" << (count - 1)
          << ":0] pyc_observation_valid_reg = '0;\n"
             "  logic [" << (count - 1)
          << ":0] pyc_observation_path_reg = '0;\n"
             "  logic [" << (count * maximumWidth - 1)
          << ":0] pyc_observation_value_reg = '0;\n"
             "  logic [" << (count * maximumWidth - 1)
          << ":0] pyc_observation_known_reg = '0;\n"
             "  logic [" << (count * maximumWidth - 1)
          << ":0] pyc_observation_z_reg = '0;\n"
             "  assign pyc_observation_valid = pyc_observation_valid_reg;\n"
             "  assign pyc_observation_path = pyc_observation_path_reg;\n"
             "  assign pyc_observation_value = pyc_observation_value_reg;\n"
             "  assign pyc_observation_known = pyc_observation_known_reg;\n"
             "  assign pyc_observation_z = pyc_observation_z_reg;\n"
             "  always @(posedge " << clock << ") begin\n"
             "    pyc_observation_valid_reg <= '0;\n"
             "    pyc_observation_path_reg <= '0;\n"
             "    pyc_observation_value_reg <= '0;\n"
             "    pyc_observation_known_reg <= '0;\n"
             "    pyc_observation_z_reg <= '0;\n"
             "    if ((pyc_root_commit_ok === 1'b1) && (" << reset
          << " === 1'b0)) begin\n";
      for (auto [index, observation] : llvm::enumerate(plan.observations)) {
        const std::string path =
            "pyc_observation_path_raw[" + std::to_string(index) + "]";
        const std::string value = "pyc_observation_value_raw";
        out << "      pyc_observation_path_reg[" << index << "] <= ("
            << path << " === 1'b1);\n"
               "      if (" << path << " === 1'b1) begin\n"
               "        pyc_observation_valid_reg[" << index << "] <= 1'b1;\n";
        for (uint64_t bit = 0; bit < observation.payloadWidth; ++bit) {
          uint64_t outputBit = index * maximumWidth + bit;
          out << "        case (" << value << "["
              << (index * maximumWidth + bit) << "])\n"
                 "          1'b0: pyc_observation_known_reg[" << outputBit
              << "] <= 1'b1;\n"
                 "          1'b1: begin pyc_observation_value_reg[" << outputBit
              << "] <= 1'b1; pyc_observation_known_reg[" << outputBit
              << "] <= 1'b1; end\n"
                 "          1'bz: pyc_observation_z_reg[" << outputBit
              << "] <= 1'b1;\n"
                 "          default: begin end\n"
                 "        endcase\n";
        }
        out << "      end\n";
      }
      out << "    end\n  end\n";
    }
  }
  out << "  " << rtlModuleName(root) << " dut (\n";
  bool connectionFirst = true;
  auto connect = [&](StringRef name, StringRef value) {
    if (!connectionFirst)
      out << ",\n";
    out << "    ." << name << "(" << value << ")";
    connectionFirst = false;
  };
  for (auto [index, input] : llvm::enumerate(root.inputs)) {
    if (exposedInput(index))
      connect(input.cppName, input.cppName);
    else
      connect(input.cppName,
              (Twine(input.type.packedWidth) + "'d" +
               Twine(root.rootInputDefaults[index - root.publicInputCount]))
                  .str());
  }
  for (unsigned index = 0; index < root.publicOutputCount; ++index)
    connect(root.outputs[index].cppName, root.outputs[index].cppName);
  if (managed) {
    connect("pyc_phase", "pyc_phase");
    connect("pyc_root_commit_ok", "pyc_root_commit_ok");
    connect("pyc_reset_active",
            root.resetInput ? root.inputs[*root.resetInput].cppName : "1'b0");
    connect("pyc_check_complete", "pyc_check_complete");
    connect("pyc_sampling_error", "pyc_sampling_error");
    connect("pyc_local_error", "pyc_local_error");
  } else if (verilog.mode == VerilogExportMode::Physical) {
    connect("pyc_root_commit_ok", "pyc_root_commit_ok");
    connect("pyc_reset_active", root.inputs[*root.resetInput].cppName);
    connect("pyc_local_error", "pyc_local_error");
    if (!plan.observations.empty()) {
      out << ",\n    .pyc_observation_path_raw(pyc_observation_path_raw)"
             ",\n    .pyc_observation_value_raw(pyc_observation_value_raw)";
    }
  }
  out << "\n  );\nendmodule\n";
  return success();
}

std::string systemSimulationTop(const VerilogDesignPlan &verilog) {
  const auto &plan = verilog.design;
  const auto &root = plan.modules[plan.rootModule];
  if (!plan.system || verilog.mode != VerilogExportMode::Managed)
    return {};
  const auto &clock = root.inputs[plan.system->clockInput];
  const auto &reset = root.inputs[plan.system->resetInput];
  auto hierarchy = [&](const ConcreteObservationPlan &observation) {
    std::string result = "dut.dut";
    unsigned moduleIndex = plan.rootModule;
    StringRef rest(observation.physicalInstancePath);
    rest.consume_front("root");
    while (rest.consume_front("/")) {
      auto [component, tail] = rest.split('/');
      bool found = false;
      for (auto [childIndex, child] :
           llvm::enumerate(plan.modules[moduleIndex].children))
        if (child.kind == ConcreteChildKind::Module && child.moduleIndex &&
            child.instanceName.getValue() == component) {
          result += ".pyc_instance_" + std::to_string(childIndex);
          moduleIndex = *child.moduleIndex;
          found = true;
          break;
        }
      if (!found)
        break;
      rest = tail;
    }
    return result;
  };
  struct ObservationGroup {
    std::size_t begin = 0;
    std::size_t size = 0;
  };
  llvm::SmallVector<ObservationGroup, 0> groups;
  for (std::size_t begin = 0; begin < plan.observations.size();) {
    const auto &first = plan.observations[begin];
    groups.push_back({begin, first.groupSize});
    begin += first.groupSize;
  }
  std::string text;
  llvm::raw_string_ostream out(text);
  out << "module pycircuit_sim;\n  logic " << clock.cppName
      << " = 1'b0;\n  logic " << reset.cppName
      << " = 1'b0;\n  logic [2:0] pyc_phase = 3'd0;\n"
         "  logic pyc_root_commit_ok = 1'b0;\n  wire pyc_check_complete;\n"
         "  wire pyc_sampling_error;\n  wire pyc_local_error;\n"
         "  localparam longint unsigned pyc_max_cycles = 64'd4611686018427387903;\n"
         "  longint unsigned pyc_cycles = 64'd10;\n  longint unsigned pyc_epoch = 0;\n"
         "  longint unsigned pyc_cycle;\n  string pyc_cycles_text;\n"
         "  bit pyc_cycles_valid;\n  pyc_root dut (."
      << clock.cppName << "(" << clock.cppName << "), ." << reset.cppName
      << "(" << reset.cppName
      << "), .pyc_phase(pyc_phase), .pyc_root_commit_ok(pyc_root_commit_ok), "
         ".pyc_check_complete(pyc_check_complete), .pyc_sampling_error(pyc_sampling_error), "
         ".pyc_local_error(pyc_local_error));\n";
  out << "  bit pyc_observation_groups_ok;\n";
  for (const auto &group : groups) {
    const auto &observation = plan.observations[group.begin];
    out << "  bit pyc_observation_active_" << observation.stableOrdinal
        << " = 1'b0;\n";
    if (observation.kind == ConcreteObservationKind::Gauge)
      out << "  longint unsigned pyc_report_value_"
          << observation.stableOrdinal
          << " = 0;\n  longint unsigned pyc_report_update_"
          << observation.stableOrdinal << " = 0;\n";
  }
  out << "  task automatic pyc_validate_observations(output bit accepted); begin\n"
         "    accepted = 1'b1;\n";
  auto signal = [&](const ConcreteObservationPlan &observation,
                    StringRef field) {
    return hierarchy(observation) + ".pyc_observation_" + field.str() + "_" +
           std::to_string(observation.localSiteIndex);
  };
  for (const auto &group : groups) {
    const auto &first = plan.observations[group.begin];
    out << "    pyc_observation_active_" << first.stableOrdinal
        << " = 1'b0;\n";
    for (std::size_t index = 0; index < group.size; ++index) {
      const auto &observation = plan.observations[group.begin + index];
      out << "    if ((" << signal(observation, "pending")
          << " !== 1'b0) && (" << signal(observation, "pending")
          << " !== 1'b1)) accepted = 1'b0;\n";
      if (index != 0)
        out << "    if (" << signal(observation, "pending") << " !== "
            << signal(first, "pending") << ") accepted = 1'b0;\n";
    }
    out << "    if (" << signal(first, "pending") << " === 1'b1) begin\n";
    for (std::size_t index = 0; index < group.size; ++index) {
      const auto &observation = plan.observations[group.begin + index];
      for (StringRef field : {"path", "valid"})
        out << "      if ((" << signal(observation, field)
            << " !== 1'b0) && (" << signal(observation, field)
            << " !== 1'b1)) accepted = 1'b0;\n";
      if (index != 0)
        out << "      if (" << signal(observation, "path") << " !== "
            << signal(first, "path") << ") accepted = 1'b0;\n";
    }
    out << "      if (" << signal(first, "path") << " === 1'b1) begin\n";
    for (std::size_t index = 1; index < group.size; ++index)
      out << "        if ("
          << signal(plan.observations[group.begin + index], "valid")
          << " !== " << signal(first, "valid")
          << ") accepted = 1'b0;\n";
    if (first.kind == ConcreteObservationKind::Gauge)
      out << "        if ($isunknown(" << signal(first, "value")
          << ")) accepted = 1'b0;\n";
    out << "        if (" << signal(first, "valid")
        << " === 1'b1) pyc_observation_active_" << first.stableOrdinal
        << " = 1'b1;\n      end\n    end\n";
  }
  out << "  end endtask\n";
  llvm::DenseSet<uint64_t> formattedWidths;
  for (const auto &observation : plan.observations) {
    if (!observation.carriesValue ||
        !formattedWidths.insert(observation.payloadWidth).second)
      continue;
    const uint64_t width = observation.payloadWidth;
    const uint64_t padded = ((width + 3) / 4) * 4;
    out << "  function automatic string pyc_observation_json_" << width
        << "(input logic [" << width
        << "-1:0] raw, input bit as_bool);\n"
           "    bit ["
        << padded
        << "-1:0] value_plane, known_plane, z_plane;\n"
           "    longint unsigned index;\n    begin\n";
    if (width <= 64)
      out << "      if (!$isunknown(raw)) begin\n"
             "        if (as_bool) pyc_observation_json_"
          << width
          << " = raw ? \"{\\\"kind\\\":\\\"bool\\\",\\\"value\\\":true}\" : "
             "\"{\\\"kind\\\":\\\"bool\\\",\\\"value\\\":false}\";\n"
             "        else pyc_observation_json_"
          << width
          << " = $sformatf(\"{\\\"kind\\\":\\\"integer\\\",\\\"value\\\":\\\"%0d\\\"}\", $unsigned(raw));\n"
             "      end else begin\n";
    out << "      value_plane = '0; known_plane = '0; z_plane = '0;\n"
           "      for (index = 0; index < "
        << width
        << "; index = index + 1) begin\n"
           "        case (raw[index])\n"
           "          1'b0: known_plane[index] = 1'b1;\n"
           "          1'b1: begin value_plane[index] = 1'b1; known_plane[index] = 1'b1; end\n"
           "          1'bz: z_plane[index] = 1'b1;\n"
           "          default: begin end\n"
           "        endcase\n      end\n"
           "      pyc_observation_json_"
        << width
        << " = $sformatf(\"{\\\"kind\\\":\\\"bits4\\\",\\\"width\\\":"
        << width
        << ",\\\"value\\\":\\\"%h\\\",\\\"known\\\":\\\"%h\\\",\\\"z\\\":\\\"%h\\\"}\", value_plane, known_plane, z_plane);\n";
    if (width <= 64)
      out << "      end\n";
    out << "    end\n  endfunction\n";
  }
  out << "  task automatic pyc_parse_cycles(input string text, output bit accepted, output longint unsigned value);\n"
         "    integer index; integer character; longint unsigned digit;\n"
         "    begin value = 0; accepted = text.len() != 0;\n"
         "      for (index = 0; accepted && index < text.len(); index = index + 1) begin\n"
         "        character = text[index];\n"
         "        if (character < 48 || character > 57) accepted = 1'b0;\n"
         "        else begin digit = character - 48;\n"
         "          if (value > (pyc_max_cycles - digit) / 10) accepted = 1'b0;\n"
         "          else value = value * 10 + digit;\n"
         "        end\n      end\n"
         "      accepted = accepted && value != 0;\n"
         "    end\n  endtask\n"
         "  task automatic pyc_publish_observations; begin\n";
  for (const auto &group : groups) {
    const auto &observation = plan.observations[group.begin];
    out << "    if (pyc_observation_active_" << observation.stableOrdinal
        << ") begin\n      $write(\"{\\\"kind\\\":\\\""
        << observation.kindName << "\\\",\\\"instance\\\":\\\""
        << observation.instancePath
        << "\\\",\\\"registration\\\":\");\n      $write("
        << svString(svString(observation.registrationJson))
        << ");\n      $write(\",\\\"site\\\":\"); $write("
        << svString(svString(observation.siteJson))
        << ");\n      $write(\",\\\"evaluation_epoch\\\":\\\"%0d\\\",\\\"commit_epoch\\\":\\\"%0d\\\",\\\"spec\\\":\", pyc_epoch, pyc_epoch + 1);\n"
        << "      $write(" << svString(observation.specJson)
        << ");\n      $write(\",\\\"values\\\":[\");\n";
    for (std::size_t index = 0; index < group.size; ++index) {
      const auto &member = plan.observations[group.begin + index];
      if (!member.carriesValue)
        continue;
      if (index != 0)
        out << "      $write(\",\");\n";
      out << "      $write(\"%s\", pyc_observation_json_"
          << member.payloadWidth << "(" << signal(member, "value") << ", "
          << (member.kind == ConcreteObservationKind::Event &&
                      member.payloadWidth == 1
                  ? "1'b1"
                  : "1'b0")
          << "));\n";
    }
    out << "      $display(\"]}\");\n";
    if (observation.kind == ConcreteObservationKind::Gauge)
      out << "      pyc_report_value_" << observation.stableOrdinal
          << " = $unsigned(" << signal(observation, "value")
          << ");\n      pyc_report_update_" << observation.stableOrdinal
          << " = pyc_epoch + 1;\n";
    out << "    end\n";
  }
  out << "  end endtask\n"
         "  task automatic pyc_step(input logic level); begin\n    "
      << clock.cppName
      << " = level; pyc_root_commit_ok = 1'b0; #1; pyc_phase = 3'd1; #1;\n"
         "    if (pyc_check_complete !== 1'b1 || pyc_sampling_error !== 1'b0 || "
         "pyc_local_error !== 1'b0) begin pyc_phase = 3'd3; #1; "
         "$fatal(1, \"pycircuit_sim: hardware check failed at epoch %0d\", pyc_epoch); end\n"
         "    pyc_validate_observations(pyc_observation_groups_ok);\n"
         "    if (!pyc_observation_groups_ok) begin pyc_phase = 3'd3; #1; "
         "$fatal(1, \"pycircuit_sim: inconsistent observation group\"); end\n"
         "    pyc_root_commit_ok = 1'b1; pyc_phase = 3'd2; #1;\n"
         "    pyc_publish_observations();\n"
         "    pyc_phase = 3'd0; pyc_root_commit_ok = 1'b0; pyc_epoch++;\n  end endtask\n"
         "  initial begin\n"
         "    if ($value$plusargs(\"cycles=%s\", pyc_cycles_text)) begin\n"
         "      pyc_parse_cycles(pyc_cycles_text, pyc_cycles_valid, pyc_cycles);\n"
         "      if (!pyc_cycles_valid) $fatal(1, \"pycircuit_sim: +cycles requires a positive decimal value no greater than 4611686018427387903\");\n"
         "    end else if ($test$plusargs(\"cycles\"))\n"
         "      $fatal(1, \"pycircuit_sim: +cycles requires a positive decimal value no greater than 4611686018427387903\");\n    "
      << reset.cppName
      << " = 1'b0; #1; pyc_root_commit_ok = 1'b1; pyc_phase = 3'd4; #1; "
         "pyc_phase = 3'd2; #1; pyc_phase = 3'd0; pyc_root_commit_ok = 1'b0;\n"
         "    for (pyc_cycle = 0; pyc_cycle < pyc_cycles; pyc_cycle++) begin "
         "pyc_step(1'b0); pyc_step(1'b1); end\n"
         "    $write(\"{\\\"kind\\\":\\\"result\\\",\\\"status\\\":\\\"TERMINATED\\\",\\\"epoch_time\\\":\\\"%0d\\\",\\\"statistics\\\":[\", pyc_epoch);\n"
         "    $write(\"{\\\"buckets\\\":[],\\\"count\\\":0,\\\"kind\\\":\\\"counter\\\",\\\"last_update\\\":{\\\"delta\\\":0,\\\"time\\\":%0d},\\\"maximum\\\":0,\\\"minimum\\\":0,\\\"name\\\":\\\"cycles\\\",\\\"object_path\\\":\\\"@runtime\\\",\\\"sum\\\":0,\\\"value\\\":%0d}\", pyc_epoch, pyc_epoch);\n"
         "    $write(\",{\\\"buckets\\\":[],\\\"count\\\":0,\\\"kind\\\":\\\"gauge\\\",\\\"last_update\\\":{\\\"delta\\\":0,\\\"time\\\":0},\\\"maximum\\\":0,\\\"minimum\\\":0,\\\"name\\\":\\\"stop_reason\\\",\\\"object_path\\\":\\\"@runtime\\\",\\\"sum\\\":0,\\\"value\\\":0}\");\n";
  for (const auto &group : groups) {
    const auto &observation = plan.observations[group.begin];
    if (observation.kind != ConcreteObservationKind::Gauge)
      continue;
    const std::string row =
        ",{\"buckets\":[],\"count\":0,\"kind\":\"gauge\","
        "\"last_update\":{\"delta\":0,\"time\":%0d},\"maximum\":0,"
        "\"minimum\":0,\"name\":" +
        jsonString(svFormatPayload(observation.reportName)) +
        ",\"object_path\":" +
        jsonString(svFormatPayload(observation.instancePath)) +
        ",\"sum\":0,\"value\":%0d}";
    out << "    $write(" << svString(row, /*trustedFormat=*/true)
        << ", pyc_report_update_"
        << observation.stableOrdinal << ", pyc_report_value_"
        << observation.stableOrdinal << ");\n";
  }
  out << "    $display(\"],\\\"error\\\":null}\");\n"
         "    $finish;\n  end\nendmodule\n";
  out.flush();
  return text;
}

} // namespace

FailureOr<VerilogDesignPlan> prepareVerilog(ModuleOp package,
                                            VerilogExportMode mode) {
  if (!package || !package->hasAttr("hwir.contract")) {
    if (package)
      package.emitOpError() << "Verilog requires independent HWIR; run ac-final-to-hwir first";
    return failure();
  }
  if (failed(hwir::verifyHWIRPackage(package)))
    return failure();
  ac::HardwareAnalysis sharedPayloadAnalysis(package);
  auto concrete = buildConcreteDesignPlan(package, sharedPayloadAnalysis);
  if (failed(concrete))
    return failure();
  VerilogDesignPlan result{std::move(*concrete), mode};
  if (mode == VerilogExportMode::Physical) {
    const auto &root = result.design.modules[result.design.rootModule];
    if (root.closedSystem || result.design.system) {
      root.key.definition->emitOpError()
          << "physical RTL requires an externally driven ordinary module root";
      return failure();
    }
    if (!root.clockInput || !root.resetInput) {
      root.key.definition->emitOpError()
          << "physical RTL requires one concrete clock/reset pair";
      return failure();
    }
    for (const ConcreteCheckPlan &check : result.design.checks)
      if (check.kind != "guard_known") {
        result.design.modules[check.moduleIndex].key.definition->emitOpError()
            << "physical RTL permits only residual guard_known checks";
        return failure();
      }
    for (auto [moduleIndex, module] : llvm::enumerate(result.design.modules)) {
      if (!module.clockInput || !module.resetInput) {
        module.key.definition->emitOpError()
            << "physical RTL requires every reachable module to carry one "
               "clock/reset domain";
        return failure();
      }
      const ConcreteValueId clock = module.inputs[*module.clockInput].value;
      const ConcreteValueId reset = module.inputs[*module.resetInput].value;
      for (const ConcreteScheduleStep &step : module.concreteSchedule.steps) {
        if (llvm::is_contained(step.inputs, clock) &&
            step.kind != ConcreteScheduleStepKind::DriveChildInput &&
            step.kind != ConcreteScheduleStepKind::DrivePrimitiveInput) {
          step.operation->emitOpError()
              << "physical RTL forbids clock use as data or combinational control";
          return failure();
        }
        if ((step.kind != ConcreteScheduleStepKind::DriveChildInput &&
             step.kind != ConcreteScheduleStepKind::DrivePrimitiveInput) ||
            !step.childIndex || !step.portIndex)
          continue;
        const ConcreteChildPlan &child = module.children[*step.childIndex];
        if (step.inputs.front() == clock &&
            ((child.kind == ConcreteChildKind::Module &&
              *step.portIndex != *result.design.modules[*child.moduleIndex].clockInput) ||
             (child.kind != ConcreteChildKind::Module && *step.portIndex != 0))) {
          step.operation->emitOpError()
              << "physical RTL forbids clock use as data or combinational control";
          return failure();
        }
        if (child.kind == ConcreteChildKind::Module) {
          const auto &callee = result.design.modules[*child.moduleIndex];
          if ((*step.portIndex == *callee.clockInput &&
               step.inputs.front() != clock) ||
              (*step.portIndex == *callee.resetInput &&
               step.inputs.front() != reset)) {
            step.operation->emitOpError()
                << "physical RTL requires all sampling and storage to use "
                   "the actual root clock/reset pair";
            return failure();
          }
        } else if (*step.portIndex < 2 &&
                   step.inputs.front() !=
                       (*step.portIndex == 0 ? clock : reset)) {
          step.operation->emitOpError()
              << "physical RTL requires all sampling and storage to use "
                 "the actual root clock/reset pair";
          return failure();
        }
      }
      for (const ConcreteChildPlan &child : module.children)
        if (child.kind != ConcreteChildKind::Module &&
            !llvm::is_contained(
                {StringRef("dff"), StringRef("dffe"), StringRef("fifo"),
                 StringRef("byte_mem"), StringRef("sync_mem"),
                 StringRef("sync_mem_dp")},
                child.primitiveKind.getValue())) {
          child.occurrence->emitOpError()
              << "physical RTL does not support primitive '"
              << child.primitiveKind.getValue() << "'";
          return failure();
        }
      for (auto [childIndex, child] : llvm::enumerate(module.children)) {
        if (child.kind == ConcreteChildKind::Module)
          continue;
        StringRef kind = child.primitiveKind.getValue();
        if (kind != "dff" && kind != "dffe")
          continue;
        if (child.inputs.empty()) {
          child.occurrence->emitOpError()
              << "state initializer requires a closed static literal/constructor DAG";
          return failure();
        }
        auto initializer =
            evaluatePhysicalInitializer(module, child.inputs.back().value);
        if (failed(initializer)) {
          child.occurrence->emitOpError()
              << "state initializer requires a closed static literal/constructor DAG";
          return failure();
        }
        VerilogDesignPlan::PhysicalInitializer materialized;
        materialized.moduleIndex = moduleIndex;
        materialized.childIndex = childIndex;
        for (unsigned word = 0; word < initializer->value.getNumWords(); ++word) {
          materialized.words.push_back(initializer->value.getRawData()[word]);
          materialized.knownWords.push_back(initializer->known.getRawData()[word]);
          materialized.zWords.push_back(initializer->z.getRawData()[word]);
        }
        result.physicalInitializers.push_back(std::move(materialized));
      }
    }
  }
  return result;
}

FailureOr<VerilogSourceParts>
emitVerilog(const VerilogDesignPlan &verilog,
            uint64_t sourceShardTargetBytes) {
  if (failed(verifyConcreteDesignPlan(verilog.design)))
    return failure();
  if (sourceShardTargetBytes == 0) {
    const_cast<ModuleOp &>(verilog.design.package).emitError()
        << "Verilog source shard target must be positive";
    return failure();
  }
  VerilogSourceParts parts;
  parts.rootRtlName = "pyc_root";
  parts.sourceCheckCount = verilog.design.sourceCheckCount;
  llvm::StringSet<> standards;
  for (const ConcreteModulePlan &module : verilog.design.modules)
    for (const ConcreteChildPlan &child : module.children)
      if (child.kind != ConcreteChildKind::Module) {
        std::string path = "include/verilog/" + child.primitiveKind.getValue().str() + ".v";
        if (standards.insert(path).second)
          parts.standardSources.push_back(std::move(path));
  }
  for (auto [unitIndex, unit] : llvm::enumerate(verilog.design.sourceUnits)) {
    std::optional<VerilogSourceGroup> shard;
    unsigned shardIndex = 0;
    auto startShard = [&] {
      shard.emplace();
      shard->sourceOwner = unit.sourceOwner;
      shard->path = "sources/source_unit_" + std::to_string(unitIndex) + "_" +
                    std::to_string(shardIndex++) + ".sv";
    };
    auto finishShard = [&] {
      if (shard && !shard->definitions.empty())
        parts.sourceGroups.push_back(std::move(*shard));
      shard.reset();
    };
    for (unsigned moduleIndex : unit.moduleIndices) {
      const auto &module = verilog.design.modules[moduleIndex];
      std::string moduleText;
      llvm::raw_string_ostream moduleOut(moduleText);
      if (failed(emitModule(verilog, moduleIndex, moduleOut)))
        return failure();
      moduleOut.flush();
      // A shard is cut only before a complete module. A single module larger
      // than the target remains atomic so generated SystemVerilog is never
      // split across files in the middle of a grammar production.
      if (shard && !shard->definitions.empty() &&
          shard->text.size() + moduleText.size() > sourceShardTargetBytes)
        finishShard();
      if (!shard)
        startShard();
      shard->definitions.push_back(FlatSymbolRefAttr::get(
          module.key.definition->getContext(), module.key.definition.getSymName()));
      shard->text += moduleText;
    }
    finishShard();
  }
  llvm::raw_string_ostream core(parts.core);
  if (failed(emitWrapper(verilog, core)))
    return failure();
  core.flush();
  parts.simulationTop = systemSimulationTop(verilog);
  return parts;
}

LogicalResult emitVerilog(const VerilogDesignPlan &plan, raw_ostream &out) {
  auto parts = emitVerilog(plan);
  if (failed(parts))
    return failure();
  out << parts->core;
  for (const auto &group : parts->sourceGroups)
    out << group.text;
  out << parts->simulationTop;
  return success();
}

} // namespace acir::compiler
