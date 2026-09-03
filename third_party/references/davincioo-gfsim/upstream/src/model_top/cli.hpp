#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace davincioo::model_top {

struct Options {
  std::string command;
  std::filesystem::path trace_path{};
  std::filesystem::path config_path{};
  std::filesystem::path summary_out{};
  std::filesystem::path pipeview_out{};
  std::filesystem::path perfetto_out{};
  std::size_t limit = 0;
  bool dump_cycles = false;
  bool help_only = false;
};

void PrintUsage();
std::optional<Options> ParseArgs(int argc, char** argv);
int RunGfsimCommand(const Options& options, const std::vector<std::string>& trace_lines);

}  // namespace davincioo::model_top
