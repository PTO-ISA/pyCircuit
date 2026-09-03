#include "model_top/cli.hpp"

#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <stdexcept>

#include "frontend/trace.hpp"
#include "model_top/core_system.hpp"
#include "model_top/kanata_pipeview.hpp"
#include "model_top/perfetto_trace.hpp"

namespace davincioo::model_top {

void PrintUsage() {
  std::cerr
      << "usage:\n"
      << "  gfsim simulate --trace <trace.jsonl> [--config <core.toml>] [--summary-out <summary.json>] [--pipeview-out <pipeview.log>] [--perfetto-out <trace.json>] [--dump-cycles]\n"
      << "  gfsim summary --trace <trace.jsonl> [--config <core.toml>] --summary-out <summary.json> [--pipeview-out <pipeview.log>] [--perfetto-out <trace.json>]\n"
      << "  gfsim dump-trace --trace <trace.jsonl> [--limit N]\n"
      << "  gfsim stats --trace <trace.jsonl>\n";
}

std::optional<Options> ParseArgs(int argc, char** argv) {
  if (argc < 2) {
    PrintUsage();
    return std::nullopt;
  }

  Options options;
  options.command = argv[1];
  if (options.command == "--help" || options.command == "-h" || options.command == "help") {
    options.help_only = true;
    PrintUsage();
    return options;
  }

  for (int index = 2; index < argc; ++index) {
    std::string arg = argv[index];
    if (arg == "--trace" && index + 1 < argc) {
      options.trace_path = argv[++index];
      continue;
    }
    if (arg == "--summary-out" && index + 1 < argc) {
      options.summary_out = argv[++index];
      continue;
    }
    if (arg == "--pipeview-out" && index + 1 < argc) {
      options.pipeview_out = argv[++index];
      continue;
    }
    if (arg == "--perfetto-out" && index + 1 < argc) {
      options.perfetto_out = argv[++index];
      continue;
    }
    if (arg == "--config" && index + 1 < argc) {
      options.config_path = argv[++index];
      continue;
    }
    if (arg == "--limit" && index + 1 < argc) {
      options.limit = static_cast<std::size_t>(std::stoull(argv[++index]));
      continue;
    }
    if (arg == "--dump-cycles") {
      options.dump_cycles = true;
      continue;
    }
    std::cerr << "unknown or incomplete argument: " << arg << "\n";
    PrintUsage();
    return std::nullopt;
  }

  if (options.command != "simulate" && options.command != "summary" &&
      options.command != "dump-trace" && options.command != "stats") {
    std::cerr << "unknown command: " << options.command << "\n";
    PrintUsage();
    return std::nullopt;
  }
  if (options.trace_path.empty()) {
    std::cerr << "--trace is required\n";
    PrintUsage();
    return std::nullopt;
  }
  if ((options.command == "simulate" || options.command == "summary") && options.summary_out.empty() &&
      options.command == "summary") {
    std::cerr << "--summary-out is required for summary\n";
    PrintUsage();
    return std::nullopt;
  }
  return options;
}

namespace {

std::string ScalarValueToString(const PTOScalarValue& value) {
  return std::visit(
      [](const auto& inner) -> std::string {
        using T = std::decay_t<decltype(inner)>;
        if constexpr (std::is_same_v<T, std::monostate>) {
          return "null";
        } else if constexpr (std::is_same_v<T, bool>) {
          return inner ? "true" : "false";
        } else if constexpr (std::is_same_v<T, std::int64_t> || std::is_same_v<T, std::uint64_t>) {
          return std::to_string(inner);
        } else {
          return std::to_string(inner);
        }
      },
      value);
}

void WriteSummaryJson(const Options& options, const SimulationResult& result) {
  if (options.summary_out.empty()) {
    return;
  }
  if (!options.summary_out.parent_path().empty()) {
    std::filesystem::create_directories(options.summary_out.parent_path());
  }

  std::ofstream summary_stream(options.summary_out, std::ios::out | std::ios::trunc);
  if (!summary_stream) {
    throw std::runtime_error("failed to open summary output: " + options.summary_out.string());
  }

  std::string first_opcode = "UNKNOWN";
  if (!result.processed.empty()) {
    first_opcode = OpcodeName(result.processed.front());
  }

  summary_stream << "{\n"
                 << "  \"tool\": \"gfsim\",\n"
                 << "  \"model\": \"davinci_ooo_model\",\n"
                 << "  \"status\": \"basic_model_top\",\n"
                 << "  \"message\": \"Basic core model executed trace through ROB, dispatch, and engines.\",\n"
                 << "  \"trace_path\": \"" << frontend::JsonEscape(options.trace_path.string()) << "\",\n"
                 << "  \"record_count\": " << result.record_count << ",\n"
                 << "  \"simulated_cycles\": " << result.simulated_cycles << ",\n"
                 << "  \"rob_capacity\": " << result.rob_capacity << ",\n"
                 << "  \"rob_count\": " << result.rob_count << ",\n"
                 << "  \"mode\": \"basic_core_model\",\n"
                 << "  \"first_opcode\": \"" << frontend::JsonEscape(first_opcode) << "\",\n"
                 << "  \"opcode_counts\": {\n";

  bool first = true;
  for (const auto& [opcode, count] : result.opcode_counts) {
    if (!first) {
      summary_stream << ",\n";
    }
    first = false;
    summary_stream << "    \"" << frontend::JsonEscape(opcode) << "\": " << count;
  }
  summary_stream << "\n  }\n}\n";
}

void WritePipeView(const Options& options, const SimulationResult& result) {
  if (options.pipeview_out.empty()) {
    return;
  }
  WriteKanataPipeView(options.pipeview_out, result);
}

void WritePerfetto(const Options& options, const SimulationResult& result) {
  if (options.perfetto_out.empty()) {
    return;
  }
  WritePerfettoTrace(options.perfetto_out, result);
}

void DumpCycleReplay(const SimulationResult& result) {
  for (std::size_t index = 0; index < result.processed.size(); ++index) {
    const PTOInst& inst = result.processed[index];
    std::cout << "retire_index=" << index
              << " opcode=" << OpcodeName(inst)
              << " engine=" << ToString(inst.engine_kind)
              << " block_idx=" << inst.block_idx
              << " sequence_id=" << inst.sequence_id
              << " rob_id=" << inst.rob_id
              << " engine_id=" << inst.engine_id
              << " src_ready=" << (inst.src_ready ? 1 : 0)
              << " runtime_latency=" << inst.runtime_latency
              << " alloc_cycle=" << inst.timestamps.rob_alloc_cycle
              << " rename_cycle=" << inst.timestamps.rename_cycle
              << " dispatch_cycle=" << inst.timestamps.dispatch_cycle
              << " issue_cycle=" << inst.timestamps.issue_cycle
              << " engine_pop_cycle=" << inst.timestamps.engine_pop_cycle
              << " engine_complete_cycle=" << inst.timestamps.engine_complete_cycle
              << " retire_cycle=" << inst.timestamps.rob_retire_cycle
              << " input_regs=" << inst.tile_reg_inputs.size()
              << " scalar_inputs=" << inst.scalar_inputs.size()
              << " output_regs=" << inst.tile_reg_outputs.size();
    if (!inst.tile_reg_inputs.empty()) {
      std::cout << " in0=0x" << std::hex << inst.tile_reg_inputs.front().address << std::dec;
      std::cout << " in0_tag=" << inst.tile_reg_inputs.front().tile_tag;
      std::cout << " in0_rm=" << (inst.tile_reg_inputs.front().rename_managed ? 1 : 0);
      if (!inst.tile_input_valid.empty()) {
        std::cout << " in0_valid=" << (inst.tile_input_valid.front() ? 1 : 0);
      }
      if (!inst.tile_input_ready.empty()) {
        std::cout << " in0_ready=" << (inst.tile_input_ready.front() ? 1 : 0);
      }
    }
    if (!inst.tile_reg_outputs.empty()) {
      std::cout << " out0=0x" << std::hex << inst.tile_reg_outputs.front().address << std::dec;
      std::cout << " out0_tag=" << inst.tile_reg_outputs.front().tile_tag;
      std::cout << " out0_rm=" << (inst.tile_reg_outputs.front().rename_managed ? 1 : 0);
      if (!inst.output_replaced_tile_tags.empty()) {
        std::cout << " out0_prev_tag=" << inst.output_replaced_tile_tags.front();
      }
    }
    if (!inst.scalar_inputs.empty()) {
      std::cout << " scalar0=" << ScalarValueToString(inst.scalar_inputs.front().value);
    }
    std::cout << "\n";
  }
}

CoreConfig ResolveConfig(const Options& options) {
  if (!options.config_path.empty()) {
    return ParseCoreTomlConfig(options.config_path);
  }
  return CoreConfig{};
}

int RunSimulationLikeCommand(const Options& options, const std::vector<std::string>& trace_lines) {
  CoreSystem system(ResolveConfig(options));
  system.LoadTrace(frontend::ParsePTOInsts(trace_lines));
  const SimulationResult result = system.RunToCompletion();

  if (options.dump_cycles) {
    DumpCycleReplay(result);
  }
  WriteSummaryJson(options, result);
  WritePipeView(options, result);
  WritePerfetto(options, result);

  std::cout << "trace: " << options.trace_path << "\n";
  std::cout << "records: " << result.record_count << "\n";
  std::cout << "simulated_cycles: " << result.simulated_cycles << "\n";
  std::cout << "rob_capacity: " << result.rob_capacity << "\n";
  std::cout << "rob_count: " << result.rob_count << "\n";
  if (!options.pipeview_out.empty()) {
    std::cout << "pipeview: " << options.pipeview_out << "\n";
  }
  if (!options.perfetto_out.empty()) {
    std::cout << "perfetto: " << options.perfetto_out << "\n";
  }
  std::cout << "mode: basic_core_model\n";
  return 0;
}

int RunDumpTrace(const Options& options, const std::vector<std::string>& trace_lines) {
  std::size_t emitted = 0;
  for (const std::string& line : trace_lines) {
    if (options.limit != 0 && emitted >= options.limit) {
      break;
    }
    std::cout << line << "\n";
    ++emitted;
  }
  return 0;
}

int RunStats(const Options& options, const std::vector<std::string>& trace_lines) {
  CoreSystem system(ResolveConfig(options));
  system.LoadTrace(frontend::ParsePTOInsts(trace_lines));
  const SimulationResult result = system.RunToCompletion();

  std::cout << "trace: " << options.trace_path << "\n";
  std::cout << "records: " << result.record_count << "\n";
  std::cout << "simulated_cycles: " << result.simulated_cycles << "\n";
  std::cout << "rob_capacity: " << result.rob_capacity << "\n";
  std::cout << "rob_count: " << result.rob_count << "\n";
  std::cout << "opcodes:\n";
  for (const auto& [opcode, count] : result.opcode_counts) {
    std::cout << "  " << std::setw(12) << std::left << opcode << " " << count << "\n";
  }
  return 0;
}

}  // namespace

int RunGfsimCommand(const Options& options, const std::vector<std::string>& trace_lines) {
  if (options.command == "simulate" || options.command == "summary") {
    return RunSimulationLikeCommand(options, trace_lines);
  }
  if (options.command == "dump-trace") {
    return RunDumpTrace(options, trace_lines);
  }
  return RunStats(options, trace_lines);
}

}  // namespace davincioo::model_top
