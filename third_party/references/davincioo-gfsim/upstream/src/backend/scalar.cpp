#include "backend/scalar.hpp"

namespace davincioo::backend {

Scalar::Scalar(EngineConfig config, ScalarCostConfig cost_config, std::string name)
    : Engine(PTOEngineKind::Scalar, config.count, std::move(name)),
      cost_config_(cost_config) {}

std::size_t Scalar::LookupLatency(const PTOInst& inst) const {
  switch (inst.opcode) {
    case PTOOpcode::TASSIGN:
      return cost_config_.tassign_latency;
    case PTOOpcode::Unknown:
    default:
      return cost_config_.unknown_latency != 0 ? cost_config_.unknown_latency : cost_config_.default_latency;
  }
}

}  // namespace davincioo::backend
