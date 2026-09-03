#include "backend/vector.hpp"

namespace davincioo::backend {

namespace {

bool IsSlowVectorOp(PTOOpcode opcode) {
  switch (opcode) {
    case PTOOpcode::Unknown:
      return true;
    default:
      return false;
  }
}

bool IsReductionLike(const PTOInst& inst) {
  const std::string opcode = OpcodeName(inst);
  return opcode == "TROWMAX" || opcode == "TROWSUM" || opcode == "TROWMIN" ||
         opcode == "TCOLMAX" || opcode == "TCOLSUM" || opcode == "TCOLMIN";
}

bool IsMoveLike(const PTOInst& inst) {
  const std::string opcode = OpcodeName(inst);
  return opcode == "TMOV" || opcode == "TMOV_FP" || opcode == "TTRANS" ||
         opcode == "TEXTRACT" || opcode == "TRESHAPE";
}

}  // namespace

Vector::Vector(EngineConfig config, VectorCostConfig cost_config, std::string name)
    : Engine(PTOEngineKind::Vec, config.count, std::move(name)),
      cost_config_(cost_config) {}

std::size_t Vector::LookupLatency(const PTOInst& inst) const {
  switch (inst.opcode) {
    case PTOOpcode::TADD: {
      const auto& output = inst.tile_reg_outputs.empty() ? PTOTileReg{} : inst.tile_reg_outputs.front();
      const std::size_t bytes = TileByteSizeCeil(output);
      return CeilDiv(bytes, cost_config_.fast_bandwidth_bytes_per_cycle) + cost_config_.elementwise_compute_cycles;
    }
    case PTOOpcode::TADDC: {
      const auto& output = inst.tile_reg_outputs.empty() ? PTOTileReg{} : inst.tile_reg_outputs.front();
      const std::size_t bytes = TileByteSizeCeil(output);
      return CeilDiv(bytes, cost_config_.fast_bandwidth_bytes_per_cycle) + cost_config_.elementwise_compute_cycles;
    }
    case PTOOpcode::TMOV: {
      const auto& output = inst.tile_reg_outputs.empty() ? PTOTileReg{} : inst.tile_reg_outputs.front();
      const std::size_t bytes = TileByteSizeCeil(output);
      return CeilDiv(bytes, cost_config_.fast_bandwidth_bytes_per_cycle) + cost_config_.move_compute_cycles;
    }
    case PTOOpcode::TMOV_FP: {
      const auto& output = inst.tile_reg_outputs.empty() ? PTOTileReg{} : inst.tile_reg_outputs.front();
      const std::size_t bytes = TileByteSizeCeil(output);
      return CeilDiv(bytes, cost_config_.fast_bandwidth_bytes_per_cycle) + cost_config_.move_compute_cycles;
    }
    case PTOOpcode::TROWMAX: {
      const auto& input = inst.tile_reg_inputs.empty() ? PTOTileReg{} : inst.tile_reg_inputs.front();
      const std::size_t bytes = TileByteSizeCeil(input);
      return CeilDiv(bytes, cost_config_.fast_bandwidth_bytes_per_cycle) + cost_config_.reduction_compute_cycles +
             cost_config_.reduction_merge_cycles;
    }
    case PTOOpcode::Unknown:
    default: {
      if (cost_config_.unknown_latency != 0) {
        if (!IsReductionLike(inst) && !IsMoveLike(inst)) {
          return cost_config_.unknown_latency;
        }
      }
      const auto& output = inst.tile_reg_outputs.empty() ? PTOTileReg{} : inst.tile_reg_outputs.front();
      const std::size_t bytes = TileByteSizeCeil(output);
      const bool move_like = IsMoveLike(inst);
      const bool reduction_like = IsReductionLike(inst);
      const std::size_t bandwidth = IsSlowVectorOp(inst.opcode) ? cost_config_.slow_bandwidth_bytes_per_cycle
                                                                 : cost_config_.fast_bandwidth_bytes_per_cycle;
      const std::size_t compute = reduction_like ? cost_config_.reduction_compute_cycles + cost_config_.reduction_merge_cycles
                                  : move_like  ? cost_config_.move_compute_cycles
                                               : IsSlowVectorOp(inst.opcode) ? cost_config_.slow_compute_cycles
                                                                             : cost_config_.elementwise_compute_cycles;
      return CeilDiv(bytes, bandwidth) + compute;
    }
  }
}

}  // namespace davincioo::backend
