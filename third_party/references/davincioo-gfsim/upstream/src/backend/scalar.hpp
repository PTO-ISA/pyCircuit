#pragma once

#include "backend/engine.hpp"
#include "davincioo/model/config.hpp"

namespace davincioo::backend {

class Scalar : public Engine {
public:
  Scalar(EngineConfig config, ScalarCostConfig cost_config, std::string name);

private:
  std::size_t LookupLatency(const PTOInst& inst) const override;

  ScalarCostConfig cost_config_{};
};

}  // namespace davincioo::backend
