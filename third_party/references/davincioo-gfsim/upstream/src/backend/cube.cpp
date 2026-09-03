#include "backend/cube.hpp"

namespace davincioo::backend {

namespace {

bool IsAccumulateLike(const PTOInst& inst) {
  const std::string opcode = OpcodeName(inst);
  return opcode == "TMATMUL_ACC" || opcode == "TMATMUL_MX_ACC" ||
         opcode == "TGEMV_ACC";
}

bool IsMxLike(const PTOInst& inst) {
  const std::string opcode = OpcodeName(inst);
  return opcode == "TMATMUL_MX" || opcode == "TMATMUL_MX_ACC" || opcode == "TMATMUL_MX_BIAS" ||
         opcode == "TGEMV_MX";
}

std::size_t AInputIndex(const PTOInst& inst) {
  if (IsAccumulateLike(inst)) {
    return IsMxLike(inst) ? 1 : 1;
  }
  return 0;
}

std::size_t BInputIndex(const PTOInst& inst) {
  if (IsMxLike(inst)) {
    return IsAccumulateLike(inst) ? 3 : 2;
  }
  return IsAccumulateLike(inst) ? 2 : 1;
}

bool IsCubeFamily(const PTOInst& inst) {
  return inst.engine_kind == PTOEngineKind::Cube;
}

}  // namespace

Cube::Cube(EngineConfig config, CubeCostConfig cost_config, std::string name)
    : Engine(PTOEngineKind::Cube, config.count, std::move(name)),
      cost_config_(cost_config) {}

std::size_t Cube::MatmulInputBytes(const PTOInst& inst, std::size_t input_index) const {
  if (input_index >= inst.tile_reg_inputs.size()) {
    return 0;
  }
  return TileByteSizeCeil(inst.tile_reg_inputs[input_index]);
}

std::size_t Cube::MatmulMacs(const PTOInst& inst) const {
  const std::size_t a_index = AInputIndex(inst);
  const std::size_t b_index = BInputIndex(inst);
  if (a_index >= inst.tile_reg_inputs.size() || b_index >= inst.tile_reg_inputs.size()) {
    return 0;
  }
  const auto& a = inst.tile_reg_inputs[a_index];
  const auto& b = inst.tile_reg_inputs[b_index];
  if (a.shape.size() < 2 || b.shape.size() < 2) {
    return 0;
  }
  const std::size_t m = static_cast<std::size_t>(a.shape[0]);
  const std::size_t k = static_cast<std::size_t>(a.shape[1]);
  const std::size_t n = static_cast<std::size_t>(b.shape[1]);
  return m * n * k;
}

std::size_t Cube::MatmulMacsPerCycle(const PTOInst& inst) const {
  const std::size_t a_index = AInputIndex(inst);
  if (a_index >= inst.tile_reg_inputs.size()) {
    return cost_config_.macs_per_cycle_fp32;
  }
  switch (inst.tile_reg_inputs[a_index].dtype) {
    case PTODType::kFloat16:
      return cost_config_.macs_per_cycle_fp16;
    case PTODType::kBFloat16:
      return cost_config_.macs_per_cycle_bf16;
    case PTODType::kFloat8:
      return cost_config_.macs_per_cycle_fp8;
    case PTODType::kInt8:
      return cost_config_.macs_per_cycle_int8;
    case PTODType::kFloat4:
      return cost_config_.macs_per_cycle_fp4;
    case PTODType::kFloat32:
    case PTODType::kUnknown:
    default:
      return cost_config_.macs_per_cycle_fp32;
  }
}

std::size_t Cube::LookupLatency(const PTOInst& inst) const {
  if (IsCubeFamily(inst)) {
    const std::size_t a_index = AInputIndex(inst);
    const std::size_t b_index = BInputIndex(inst);
    const std::size_t a_bytes = MatmulInputBytes(inst, a_index);
    const std::size_t b_bytes = MatmulInputBytes(inst, b_index);
    const std::size_t load_cycles =
        CeilDiv(a_bytes + b_bytes, cost_config_.input_bandwidth_bytes_per_cycle);
    const std::size_t compute_cycles =
        CeilDiv(MatmulMacs(inst), MatmulMacsPerCycle(inst));
    const std::size_t base_cycles =
        cost_config_.overlap_mode
            ? CeilDiv(a_bytes, cost_config_.input_bandwidth_bytes_per_cycle) +
                  std::max(CeilDiv(b_bytes, cost_config_.input_bandwidth_bytes_per_cycle), compute_cycles)
            : load_cycles + compute_cycles;
    return base_cycles + (IsAccumulateLike(inst) ? cost_config_.accumulate_extra_cycles : 0);
  }
  switch (inst.opcode) {
    case PTOOpcode::Unknown:
    default:
      return cost_config_.unknown_latency;
  }
}

}  // namespace davincioo::backend
