#include "davincioo/model/config.hpp"

#include <fstream>
#include <stdexcept>
#include <string>

namespace davincioo {

namespace {

std::string Trim(const std::string& input) {
  std::size_t start = input.find_first_not_of(" \t\r\n");
  if (start == std::string::npos) {
    return "";
  }
  std::size_t end = input.find_last_not_of(" \t\r\n");
  return input.substr(start, end - start + 1);
}

bool IsPowerOfTwo(std::size_t value) {
  return value != 0 && (value & (value - 1)) == 0;
}

}  // namespace

CoreConfig ParseCoreTomlConfig(const std::filesystem::path& path) {
  std::ifstream stream(path);
  if (!stream) {
    throw std::runtime_error("failed to open config: " + path.string());
  }

  CoreConfig config;
  std::string section;
  std::string line;
  while (std::getline(stream, line)) {
    const std::size_t comment = line.find('#');
    if (comment != std::string::npos) {
      line = line.substr(0, comment);
    }
    line = Trim(line);
    if (line.empty()) {
      continue;
    }
    if (line.front() == '[' && line.back() == ']') {
      section = Trim(line.substr(1, line.size() - 2));
      continue;
    }

    const std::size_t eq = line.find('=');
    if (eq == std::string::npos) {
      continue;
    }
    const std::string key = Trim(line.substr(0, eq));
    const std::string value = Trim(line.substr(eq + 1));

    if (section == "rob") {
      if (key == "entries") {
        config.rob.entries = static_cast<std::size_t>(std::stoull(value));
      }
      continue;
    }
    if (section == "rename") {
      if (key == "tile_tags") {
        config.rename.tile_tags = static_cast<std::size_t>(std::stoull(value));
      }
      continue;
    }
    if (section == "issue_queue") {
      if (key == "entries") {
        config.issue_queue.entries = static_cast<std::size_t>(std::stoull(value));
      }
      continue;
    }
    if (section == "frontend_width") {
      const std::size_t v = static_cast<std::size_t>(std::stoull(value));
      if (key == "width") {
        config.frontend_width.fetch_width = v;
        config.frontend_width.rob_alloc_width = v;
        config.frontend_width.rob_issue_width = v;
        config.frontend_width.rename_width = v;
        config.frontend_width.dispatch_width = v;
      } else if (key == "fetch_width") {
        config.frontend_width.fetch_width = v;
      } else if (key == "rob_alloc_width") {
        config.frontend_width.rob_alloc_width = v;
      } else if (key == "rob_issue_width") {
        config.frontend_width.rob_issue_width = v;
      } else if (key == "rename_width") {
        config.frontend_width.rename_width = v;
      } else if (key == "dispatch_width") {
        config.frontend_width.dispatch_width = v;
      }
      continue;
    }
    if (section == "scalar") {
      if (key == "count") {
        config.scalar.count = static_cast<std::size_t>(std::stoull(value));
      }
      continue;
    }
    if (section == "vec") {
      if (key == "count") {
        config.vec.count = static_cast<std::size_t>(std::stoull(value));
      }
      continue;
    }
    if (section == "cube") {
      if (key == "count") {
        config.cube.count = static_cast<std::size_t>(std::stoull(value));
      }
      continue;
    }
    if (section == "tma") {
      if (key == "count") {
        config.tma.count = static_cast<std::size_t>(std::stoull(value));
      }
      continue;
    }
    if (section == "scalar_cost") {
      if (key == "default_latency") {
        config.scalar_cost.default_latency = static_cast<std::size_t>(std::stoull(value));
      } else if (key == "tassign_latency") {
        config.scalar_cost.tassign_latency = static_cast<std::size_t>(std::stoull(value));
      } else if (key == "unknown_latency") {
        config.scalar_cost.unknown_latency = static_cast<std::size_t>(std::stoull(value));
      }
      continue;
    }
    if (section == "vec_cost") {
      if (key == "fast_bandwidth_bytes_per_cycle") {
        config.vec_cost.fast_bandwidth_bytes_per_cycle = static_cast<std::size_t>(std::stoull(value));
      } else if (key == "slow_bandwidth_bytes_per_cycle") {
        config.vec_cost.slow_bandwidth_bytes_per_cycle = static_cast<std::size_t>(std::stoull(value));
      } else if (key == "elementwise_compute_cycles") {
        config.vec_cost.elementwise_compute_cycles = static_cast<std::size_t>(std::stoull(value));
      } else if (key == "reduction_compute_cycles") {
        config.vec_cost.reduction_compute_cycles = static_cast<std::size_t>(std::stoull(value));
      } else if (key == "reduction_merge_cycles") {
        config.vec_cost.reduction_merge_cycles = static_cast<std::size_t>(std::stoull(value));
      } else if (key == "slow_compute_cycles") {
        config.vec_cost.slow_compute_cycles = static_cast<std::size_t>(std::stoull(value));
      } else if (key == "move_compute_cycles") {
        config.vec_cost.move_compute_cycles = static_cast<std::size_t>(std::stoull(value));
      } else if (key == "unknown_latency") {
        config.vec_cost.unknown_latency = static_cast<std::size_t>(std::stoull(value));
      }
      continue;
    }
    if (section == "cube_cost") {
      if (key == "input_bandwidth_bytes_per_cycle") {
        config.cube_cost.input_bandwidth_bytes_per_cycle = static_cast<std::size_t>(std::stoull(value));
      } else if (key == "macs_per_cycle_fp32") {
        config.cube_cost.macs_per_cycle_fp32 = static_cast<std::size_t>(std::stoull(value));
      } else if (key == "macs_per_cycle_fp16") {
        config.cube_cost.macs_per_cycle_fp16 = static_cast<std::size_t>(std::stoull(value));
      } else if (key == "macs_per_cycle_bf16") {
        config.cube_cost.macs_per_cycle_bf16 = static_cast<std::size_t>(std::stoull(value));
      } else if (key == "macs_per_cycle_fp8") {
        config.cube_cost.macs_per_cycle_fp8 = static_cast<std::size_t>(std::stoull(value));
      } else if (key == "macs_per_cycle_int8") {
        config.cube_cost.macs_per_cycle_int8 = static_cast<std::size_t>(std::stoull(value));
      } else if (key == "macs_per_cycle_fp4") {
        config.cube_cost.macs_per_cycle_fp4 = static_cast<std::size_t>(std::stoull(value));
      } else if (key == "accumulate_extra_cycles") {
        config.cube_cost.accumulate_extra_cycles = static_cast<std::size_t>(std::stoull(value));
      } else if (key == "overlap_mode") {
        config.cube_cost.overlap_mode = (value == "true" || value == "1");
      } else if (key == "unknown_latency") {
        config.cube_cost.unknown_latency = static_cast<std::size_t>(std::stoull(value));
      }
      continue;
    }
    if (section == "tma_cost") {
      if (key == "bandwidth_bytes_per_cycle") {
        config.tma_cost.bandwidth_bytes_per_cycle = static_cast<std::size_t>(std::stoull(value));
      } else if (key == "load_overhead_cycles") {
        config.tma_cost.load_overhead_cycles = static_cast<std::size_t>(std::stoull(value));
      } else if (key == "store_overhead_cycles") {
        config.tma_cost.store_overhead_cycles = static_cast<std::size_t>(std::stoull(value));
      } else if (key == "move_overhead_cycles") {
        config.tma_cost.move_overhead_cycles = static_cast<std::size_t>(std::stoull(value));
      } else if (key == "unknown_latency") {
        config.tma_cost.unknown_latency = static_cast<std::size_t>(std::stoull(value));
      }
      continue;
    }
  }

  if (!IsPowerOfTwo(config.rob.entries)) {
    throw std::runtime_error("rob.entries must be a power of two and > 0");
  }
  if (config.rename.tile_tags == 0) {
    throw std::runtime_error("rename.tile_tags must be > 0");
  }
  if (config.issue_queue.entries == 0) {
    throw std::runtime_error("issue_queue.entries must be > 0");
  }
  if (config.scalar.count == 0 || config.vec.count == 0 || config.cube.count == 0 || config.tma.count == 0) {
    throw std::runtime_error("engine counts must be > 0");
  }
  if (config.frontend_width.fetch_width == 0 ||
      config.frontend_width.rob_alloc_width == 0 ||
      config.frontend_width.rob_issue_width == 0 ||
      config.frontend_width.rename_width == 0 ||
      config.frontend_width.dispatch_width == 0) {
    throw std::runtime_error("frontend_width.* must be > 0");
  }
  return config;
}

}  // namespace davincioo
