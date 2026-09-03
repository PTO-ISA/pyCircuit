#include "backend/tma.hpp"

namespace davincioo::backend {

Tma::Tma(EngineConfig config, TmaCostConfig cost_config, std::string name)
    : Engine(PTOEngineKind::Tma, config.count, std::move(name)),
      cost_config_(cost_config) {}

std::size_t Tma::LookupLatency(const PTOInst& inst) const {
  switch (inst.opcode) {
    case PTOOpcode::TLOAD: {
      const auto& source = inst.tile_reg_inputs.empty() ? PTOTileReg{} : inst.tile_reg_inputs.front();
      return CeilDiv(TileByteSizeCeil(source), cost_config_.bandwidth_bytes_per_cycle) + cost_config_.load_overhead_cycles;
    }
    case PTOOpcode::TSTORE: {
      const auto& source = inst.tile_reg_inputs.empty() ? PTOTileReg{} : inst.tile_reg_inputs.front();
      return CeilDiv(TileByteSizeCeil(source), cost_config_.bandwidth_bytes_per_cycle) + cost_config_.store_overhead_cycles;
    }
    case PTOOpcode::TSTORE_FP: {
      const auto& source = inst.tile_reg_inputs.empty() ? PTOTileReg{} : inst.tile_reg_inputs.front();
      return CeilDiv(TileByteSizeCeil(source), cost_config_.bandwidth_bytes_per_cycle) + cost_config_.store_overhead_cycles;
    }
    case PTOOpcode::Unknown:
    default:
      return cost_config_.unknown_latency;
  }
}

}  // namespace davincioo::backend
