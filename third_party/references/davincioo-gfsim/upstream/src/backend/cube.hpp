#pragma once

#include "backend/engine.hpp"
#include "davincioo/model/config.hpp"

namespace davincioo::backend {

class Cube : public Engine {
public:
  Cube(EngineConfig config, CubeCostConfig cost_config, std::string name);

private:
  std::size_t LookupLatency(const PTOInst& inst) const override;
  std::size_t MatmulMacs(const PTOInst& inst) const;
  std::size_t MatmulInputBytes(const PTOInst& inst, std::size_t input_index) const;
  std::size_t MatmulMacsPerCycle(const PTOInst& inst) const;

  CubeCostConfig cost_config_{};
};

}  // namespace davincioo::backend
