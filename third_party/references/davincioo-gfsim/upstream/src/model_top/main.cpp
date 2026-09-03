#include <iostream>
#include <vector>

#include "frontend/trace.hpp"
#include "model_top/cli.hpp"

int main(int argc, char** argv) {
  auto parsed = davincioo::model_top::ParseArgs(argc, argv);
  if (!parsed.has_value()) {
    return 2;
  }

  const davincioo::model_top::Options& options = *parsed;
  if (options.help_only) {
    return 0;
  }

  std::vector<std::string> trace_lines;
  try {
    trace_lines = davincioo::frontend::ReadTraceLines(options.trace_path);
  } catch (const std::runtime_error& error) {
    std::cerr << error.what() << "\n";
    return 1;
  }

  try {
    return davincioo::model_top::RunGfsimCommand(options, trace_lines);
  } catch (const std::runtime_error& error) {
    std::cerr << error.what() << "\n";
    return 1;
  }
}
