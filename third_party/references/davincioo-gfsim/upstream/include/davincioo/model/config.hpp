#pragma once

#include <cstddef>
#include <filesystem>

namespace davincioo {

struct ROBConfig {
  std::size_t entries = 64;
};

struct RenameConfig {
  std::size_t tile_tags = 4096;
};

struct IssueQueueConfig {
  std::size_t entries = 8;
};

struct ScalarCostConfig {
  std::size_t default_latency = 1;
  std::size_t tassign_latency = 1;
  std::size_t unknown_latency = 1;
};

struct VectorCostConfig {
  std::size_t fast_bandwidth_bytes_per_cycle = 512;
  std::size_t slow_bandwidth_bytes_per_cycle = 256;
  std::size_t elementwise_compute_cycles = 4;
  std::size_t reduction_compute_cycles = 8;
  std::size_t reduction_merge_cycles = 3;
  std::size_t slow_compute_cycles = 12;
  std::size_t move_compute_cycles = 4;
  std::size_t unknown_latency = 12;
};

struct CubeCostConfig {
  std::size_t input_bandwidth_bytes_per_cycle = 512;
  std::size_t macs_per_cycle_fp32 = 4096;
  std::size_t macs_per_cycle_fp16 = 4096;
  std::size_t macs_per_cycle_bf16 = 4096;
  std::size_t macs_per_cycle_fp8 = 8192;
  std::size_t macs_per_cycle_int8 = 8192;
  std::size_t macs_per_cycle_fp4 = 32768;
  std::size_t accumulate_extra_cycles = 0;
  bool overlap_mode = false;
  std::size_t unknown_latency = 72;
};

struct TmaCostConfig {
  std::size_t bandwidth_bytes_per_cycle = 512;
  std::size_t load_overhead_cycles = 0;
  std::size_t store_overhead_cycles = 0;
  std::size_t move_overhead_cycles = 0;
  std::size_t unknown_latency = 2;
};

struct EngineConfig {
  std::size_t count = 1;
};

struct FrontendWidthConfig {
  std::size_t fetch_width     = 1;  // TraceSourceModule -> rob_input_q
  std::size_t rob_alloc_width = 1;  // ROB::Allocate per cycle
  std::size_t rob_issue_width = 1;  // ROB -> rename per cycle
  std::size_t rename_width    = 1;  // Rename body per cycle
  std::size_t dispatch_width  = 1;  // Dispatch routing per cycle
};

struct CoreConfig {
  ROBConfig rob;
  RenameConfig rename;
  IssueQueueConfig issue_queue;
  FrontendWidthConfig frontend_width;
  EngineConfig scalar;
  EngineConfig vec;
  EngineConfig cube;
  EngineConfig tma;
  ScalarCostConfig scalar_cost;
  VectorCostConfig vec_cost;
  CubeCostConfig cube_cost;
  TmaCostConfig tma_cost;
};

CoreConfig ParseCoreTomlConfig(const std::filesystem::path& path);

}  // namespace davincioo
