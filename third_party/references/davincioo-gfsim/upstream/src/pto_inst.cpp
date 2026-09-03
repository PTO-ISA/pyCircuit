#include "davincioo/model/pto_inst.hpp"

#include <sstream>
#include <type_traits>

namespace davincioo {

namespace {

std::string DumpScalarValue(const PTOScalarValue& value) {
  return std::visit(
      [](const auto& inner) -> std::string {
        using T = std::decay_t<decltype(inner)>;
        if constexpr (std::is_same_v<T, std::monostate>) {
          return "null";
        } else if constexpr (std::is_same_v<T, bool>) {
          return inner ? "true" : "false";
        } else {
          return std::to_string(inner);
        }
      },
      value);
}

void AppendShape(std::ostringstream& stream, const std::vector<std::int64_t>& shape) {
  stream << "[";
  for (std::size_t index = 0; index < shape.size(); ++index) {
    if (index != 0) {
      stream << ",";
    }
    stream << shape[index];
  }
  stream << "]";
}

void AppendTileReg(std::ostringstream& stream, const PTOTileReg& reg) {
  stream << "0x" << std::hex << reg.address << std::dec
         << ":tag=";
  if (reg.tile_tag == kInvalidTileTag) {
    stream << "-";
  } else {
    stream << reg.tile_tag;
  }
  stream << ":rm=" << (reg.rename_managed ? 1 : 0)
         << ":" << ToString(reg.tile_type)
         << ":" << ToString(reg.dtype)
         << ":" << ToString(reg.layout)
         << ":";
  AppendShape(stream, reg.shape);
}

void AppendTileInputState(
    std::ostringstream& stream,
    const PTOInst& inst,
    std::size_t index) {
  if (index >= inst.tile_input_valid.size() || index >= inst.tile_input_ready.size()) {
    return;
  }
  stream << ":v=" << (inst.tile_input_valid[index] ? 1 : 0)
         << ":r=" << (inst.tile_input_ready[index] ? 1 : 0);
}

void AppendTileRegVector(
    std::ostringstream& stream,
    std::string_view label,
    const PTOInst& inst,
    const std::vector<PTOTileReg>& regs,
    bool annotate_inputs) {
  stream << " " << label << "=[";
  for (std::size_t index = 0; index < regs.size(); ++index) {
    if (index != 0) {
      stream << ", ";
    }
    AppendTileReg(stream, regs[index]);
    if (annotate_inputs) {
      AppendTileInputState(stream, inst, index);
    }
  }
  stream << "]";
}

void AppendScalarVector(
    std::ostringstream& stream,
    const std::vector<PTOScalarInput>& scalars) {
  stream << " scalars=[";
  for (std::size_t index = 0; index < scalars.size(); ++index) {
    if (index != 0) {
      stream << ", ";
    }
    stream << ToString(scalars[index].dtype) << ":" << DumpScalarValue(scalars[index].value);
  }
  stream << "]";
}

}  // namespace

std::string OpcodeName(const PTOInst& inst) {
  if (!inst.raw_opcode.empty()) {
    return inst.raw_opcode;
  }
  return std::string(ToString(inst.opcode));
}

std::string DumpPTOInst(const PTOInst& inst) {
  std::ostringstream stream;
  stream << "opcode=" << OpcodeName(inst)
         << " engine=" << ToString(inst.engine_kind)
         << " block=" << inst.block_idx
         << " seq=" << inst.sequence_id
         << " rob=" << inst.rob_id
         << " eng_id=" << inst.engine_id;
  AppendTileRegVector(stream, "in", inst, inst.tile_reg_inputs, true);
  AppendScalarVector(stream, inst.scalar_inputs);
  AppendTileRegVector(stream, "out", inst, inst.tile_reg_outputs, false);
  return stream.str();
}

}  // namespace davincioo
