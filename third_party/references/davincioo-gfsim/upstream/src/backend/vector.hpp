#pragma once

#include "backend/engine.hpp"
#include "davincioo/model/config.hpp"

namespace davincioo::backend {

class Vector : public Engine {
public:
  Vector(EngineConfig config, VectorCostConfig cost_config, std::string name);

private:
  std::size_t LookupLatency(const PTOInst& inst) const override;

  VectorCostConfig cost_config_{};
};

}  // namespace davincioo::backend
