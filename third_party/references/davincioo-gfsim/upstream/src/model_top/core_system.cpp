#include "model_top/core_system.hpp"

#include <algorithm>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <tuple>

namespace davincioo::model_top {

CoreSystem::CoreSystem(CoreConfig config)
    : config_(config) {
}

void CoreSystem::LoadTrace(std::vector<PTOInst> insts) {
  EnsureCoresForBlocks(insts);

  std::map<std::uint64_t, std::vector<PTOInst>> insts_by_block;
  for (PTOInst& inst : insts) {
    insts_by_block[inst.block_idx].push_back(std::move(inst));
  }
  for (std::size_t index = 0; index < cores_.size(); ++index) {
    cores_[index]->LoadTrace(std::move(insts_by_block[core_block_indices_[index]]));
  }
}

void CoreSystem::RunReference(std::optional<std::uint64_t> stop_pc) {
  (void)stop_pc;
}

void CoreSystem::step() {
  Step();
}

std::uint64_t CoreSystem::getCycles() const {
  return Cycle();
}

bool CoreSystem::needTerminate() const {
  return std::all_of(cores_.begin(), cores_.end(), [](const Core* core) { return core->Done(); });
}

void CoreSystem::enableTrace(std::optional<std::uint64_t> start_pc) {
  (void)start_pc;
  trace_enabled_ = true;
}

void CoreSystem::PrintPipeView(std::ostream& os) const {
  os << "cycle=" << Cycle();
  for (std::size_t index = 0; index < cores_.size(); ++index) {
    os << " block=" << core_block_indices_[index] << " ";
    cores_[index]->PrintPipeView(os);
  }
}

SimulationResult CoreSystem::RunToCompletion() {
  return RunToCompletion(RunArgs{});
}

SimulationResult CoreSystem::RunToCompletion(const RunArgs& args) {
  Build();
  Reset();
  bool terminate = false;
  std::uint64_t idle_cycles = 0;
  while (!terminate) {
    RunReference(args.stop_pc);
    step();
    std::size_t total_progress = 0;
    for (Core* core : cores_) {
      total_progress += core->ConsumeProgress();
    }
    if (total_progress == 0) {
      ++idle_cycles;
    } else {
      idle_cycles = 0;
    }
    if (args.print_pipeview) {
      PrintPipeView(std::cout);
    }
    enableTrace(std::nullopt);
    terminate = needTerminate();
    if (!terminate && idle_cycles >= args.deadlock_cycles) {
      throw std::runtime_error(
          "deadlock detected: no simulator progress for " + std::to_string(args.deadlock_cycles) +
          " cycles\n" + DumpRobStates());
    }
    if (getCycles() >= args.stop_cycles) {
      break;
    }
  }

  SimulationResult result;
  result.record_count = 0;
  result.simulated_cycles = static_cast<std::size_t>(Cycle());
  result.rob_capacity = 0;
  result.rob_count = 0;
  result.scalar_engine_count = cores_.empty() ? 0 : cores_.front()->ScalarEngineCount();
  result.vec_engine_count = cores_.empty() ? 0 : cores_.front()->VecEngineCount();
  result.cube_engine_count = cores_.empty() ? 0 : cores_.front()->CubeEngineCount();
  result.tma_engine_count = cores_.empty() ? 0 : cores_.front()->TmaEngineCount();

  for (Core* core : cores_) {
    result.record_count += core->SourceCount();
    result.rob_capacity += core->RobCapacity();
    result.rob_count += core->RobCount();
    const auto& processed = core->Processed();
    result.processed.insert(result.processed.end(), processed.begin(), processed.end());
  }
  std::sort(result.processed.begin(), result.processed.end(), [](const PTOInst& lhs, const PTOInst& rhs) {
    return std::tie(lhs.timestamps.rob_alloc_cycle, lhs.block_idx, lhs.sequence_id) <
           std::tie(rhs.timestamps.rob_alloc_cycle, rhs.block_idx, rhs.sequence_id);
  });
  for (const PTOInst& inst : result.processed) {
    ++result.opcode_counts[OpcodeName(inst)];
  }
  return result;
}

void CoreSystem::EnsureCoresForBlocks(const std::vector<PTOInst>& insts) {
  std::set<std::uint64_t> blocks;
  for (const PTOInst& inst : insts) {
    blocks.insert(inst.block_idx);
  }
  const std::vector<std::uint64_t> desired_blocks(blocks.begin(), blocks.end());
  if (!cores_.empty()) {
    GFSIM_ASSERT(core_block_indices_ == desired_blocks);
    return;
  }
  core_block_indices_ = desired_blocks;
  cores_.reserve(core_block_indices_.size());
  for (std::size_t index = 0; index < core_block_indices_.size(); ++index) {
    cores_.push_back(&EmplaceOwnedModule<Core>(config_));
  }
}

std::string CoreSystem::DumpRobStates() const {
  std::ostringstream stream;
  for (std::size_t index = 0; index < cores_.size(); ++index) {
    if (index != 0) {
      stream << "\n";
    }
    stream << "block_idx=" << core_block_indices_[index] << "\n" << cores_[index]->DumpRobState();
    stream << "\n" << cores_[index]->DumpSchedulerState();
  }
  return stream.str();
}

}  // namespace davincioo::model_top
