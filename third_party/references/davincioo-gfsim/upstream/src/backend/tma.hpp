#pragma once

#include "backend/engine.hpp"
#include "davincioo/model/config.hpp"

namespace davincioo::backend {

class Tma : public Engine {
public:
  Tma(EngineConfig config, TmaCostConfig cost_config, std::string name);

private:
  std::size_t LookupLatency(const PTOInst& inst) const override;

  TmaCostConfig cost_config_{};
};

}  // namespace davincioo::backend
