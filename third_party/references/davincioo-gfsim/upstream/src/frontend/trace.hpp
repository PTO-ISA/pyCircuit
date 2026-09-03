#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "davincioo/model/pto_inst.hpp"

namespace davincioo::frontend {

std::optional<std::string> ExtractJsonStringField(const std::string& line, const std::string& key);
std::optional<std::uint64_t> ExtractJsonUintField(const std::string& line, const std::string& key);
std::string JsonEscape(const std::string& input);
std::vector<std::string> ReadTraceLines(const std::filesystem::path& trace_path);
std::vector<PTOInst> ParsePTOInsts(const std::vector<std::string>& trace_lines);

}  // namespace davincioo::frontend
