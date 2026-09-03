#pragma once

#include <cstdint>
#include <map>
#include <iosfwd>
#include <limits>
#include <optional>
#include <vector>

#include "davincioo/model/config.hpp"
#include "davincioo/model/framework.hpp"
#include "davincioo/model/pto_inst.hpp"
#include "model_top/core.hpp"

namespace davincioo::model_top {

class CoreSystem : public SimSystem {
public:
  explicit CoreSystem(CoreConfig config);

  struct RunArgs {
    std::optional<std::uint64_t> stop_pc;
    std::uint64_t stop_cycles = std::numeric_limits<std::uint64_t>::max();
    std::uint64_t deadlock_cycles = 2000;
    bool print_pipeview = false;
  };

  void LoadTrace(std::vector<PTOInst> insts);
  SimulationResult RunToCompletion();
  SimulationResult RunToCompletion(const RunArgs& args);
  void RunReference(std::optional<std::uint64_t> stop_pc);
  void step();
  std::uint64_t getCycles() const;
  bool needTerminate() const;
  void enableTrace(std::optional<std::uint64_t> start_pc);
  void PrintPipeView(std::ostream& os) const;

private:
  void EnsureCoresForBlocks(const std::vector<PTOInst>& insts);
  std::string DumpRobStates() const;

  CoreConfig config_{};
  std::vector<Core*> cores_;
  std::vector<std::uint64_t> core_block_indices_;
  bool trace_enabled_ = false;
};

}  // namespace davincioo::model_top
