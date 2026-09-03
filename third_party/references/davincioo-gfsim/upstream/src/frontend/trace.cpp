#include "frontend/trace.hpp"

#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace davincioo::frontend {

namespace {

std::optional<std::string> ExtractJsonArraySegment(const std::string& line, const std::string& key) {
  const std::string pattern = "\"" + key + "\":[";
  std::size_t start = line.find(pattern);
  if (start == std::string::npos) {
    return std::nullopt;
  }
  start += pattern.size() - 1;
  std::size_t end = start;
  int depth = 0;
  for (; end < line.size(); ++end) {
    if (line[end] == '[') {
      ++depth;
    } else if (line[end] == ']') {
      --depth;
      if (depth == 0) {
        return line.substr(start, end - start + 1);
      }
    }
  }
  return std::nullopt;
}

std::vector<std::string> SplitJsonObjects(const std::string& array_segment) {
  std::vector<std::string> objects;
  std::size_t object_start = std::string::npos;
  int depth = 0;
  for (std::size_t index = 0; index < array_segment.size(); ++index) {
    if (array_segment[index] == '{') {
      if (depth == 0) {
        object_start = index;
      }
      ++depth;
    } else if (array_segment[index] == '}') {
      --depth;
      if (depth == 0 && object_start != std::string::npos) {
        objects.push_back(array_segment.substr(object_start, index - object_start + 1));
        object_start = std::string::npos;
      }
    }
  }
  return objects;
}

std::vector<std::int64_t> ParseJsonIntArray(const std::string& object, const std::string& key) {
  std::vector<std::int64_t> values;
  auto segment = ExtractJsonArraySegment(object, key);
  if (!segment.has_value()) {
    return values;
  }

  std::string content = segment->substr(1, segment->size() - 2);
  std::size_t start = 0;
  while (start < content.size()) {
    while (start < content.size() && (content[start] == ',' || content[start] == ' ')) {
      ++start;
    }
    if (start >= content.size()) {
      break;
    }
    std::size_t end = start;
    if (content[end] == '-') {
      ++end;
    }
    while (end < content.size() && content[end] >= '0' && content[end] <= '9') {
      ++end;
    }
    values.push_back(static_cast<std::int64_t>(std::stoll(content.substr(start, end - start))));
    start = end + 1;
  }
  return values;
}

std::uint64_t ParseHexUint64(const std::string& text) {
  if (text.empty()) {
    return 0;
  }
  return std::stoull(text, nullptr, 16);
}

PTOOpcodeDescriptor ParseOpcodeDescriptor(const std::string& raw_opcode) {
  if (const auto* descriptor = FindPTOOpcodeDescriptor(raw_opcode)) {
    return *descriptor;
  }
  return kUnknownPTOOpcodeDescriptor;
}

PTOOpcodeDescriptor RequireOpcodeDescriptor(const std::string& raw_opcode, std::size_t record_index) {
  const PTOOpcodeDescriptor descriptor = ParseOpcodeDescriptor(raw_opcode);
  if (descriptor.opcode == PTOOpcode::Unknown) {
    throw std::runtime_error(
        "unknown PTO opcode at trace record " + std::to_string(record_index) + ": " + raw_opcode);
  }
  return descriptor;
}

PTODType ParseDType(const std::string& value) {
  if (value == "bool") {
    return PTODType::kBool;
  }
  if (value == "int8") {
    return PTODType::kInt8;
  }
  if (value == "int32") {
    return PTODType::kInt32;
  }
  if (value == "int64") {
    return PTODType::kInt64;
  }
  if (value == "uint64") {
    return PTODType::kUInt64;
  }
  if (value == "float16" || value == "fp16") {
    return PTODType::kFloat16;
  }
  if (value == "bfloat16" || value == "bf16") {
    return PTODType::kBFloat16;
  }
  if (value == "float32") {
    return PTODType::kFloat32;
  }
  if (value == "float8" || value == "fp8") {
    return PTODType::kFloat8;
  }
  if (value == "float4" || value == "fp4") {
    return PTODType::kFloat4;
  }
  return PTODType::kUnknown;
}

PTOTileType ParseTileType(const std::string& value) {
  if (value == "Vec") {
    return PTOTileType::Vec;
  }
  if (value == "Mat") {
    return PTOTileType::Mat;
  }
  if (value == "Left") {
    return PTOTileType::Left;
  }
  if (value == "Right") {
    return PTOTileType::Right;
  }
  if (value == "Acc") {
    return PTOTileType::Acc;
  }
  if (value == "Bias") {
    return PTOTileType::Bias;
  }
  if (value == "Scaling") {
    return PTOTileType::Scaling;
  }
  if (value == "ScaleLeft") {
    return PTOTileType::ScaleLeft;
  }
  if (value == "ScaleRight") {
    return PTOTileType::ScaleRight;
  }
  if (value == "Ctrl") {
    return PTOTileType::Ctrl;
  }
  if (value == "Global") {
    return PTOTileType::Global;
  }
  return PTOTileType::Unknown;
}

PTOLayout ParseLayout(const std::string& value) {
  if (value == "ND") {
    return PTOLayout::kND;
  }
  if (value == "DN") {
    return PTOLayout::kDN;
  }
  if (value == "NZ") {
    return PTOLayout::kNZ;
  }
  if (value == "SCALE") {
    return PTOLayout::kScale;
  }
  if (value == "MX_A_ND") {
    return PTOLayout::kMxAND;
  }
  if (value == "MX_A_DN") {
    return PTOLayout::kMxADN;
  }
  if (value == "MX_A_ZZ") {
    return PTOLayout::kMxAZZ;
  }
  if (value == "MX_B_ND") {
    return PTOLayout::kMxBND;
  }
  if (value == "MX_B_DN") {
    return PTOLayout::kMxBDN;
  }
  if (value == "MX_B_NN") {
    return PTOLayout::kMxBNN;
  }
  if (value == "NC1HWC0") {
    return PTOLayout::kNC1HWC0;
  }
  if (value == "NCHW") {
    return PTOLayout::kNCHW;
  }
  if (value == "NHWC") {
    return PTOLayout::kNHWC;
  }
  if (value == "NDC1HWC0") {
    return PTOLayout::kNDC1HWC0;
  }
  if (value == "NCDHW") {
    return PTOLayout::kNCDHW;
  }
  if (value == "FRACTAL_Z") {
    return PTOLayout::kFractalZ;
  }
  if (value == "FRACTAL_Z_S16S8") {
    return PTOLayout::kFractalZS16S8;
  }
  if (value == "FRACTAL_Z_3D") {
    return PTOLayout::kFractalZ3D;
  }
  return PTOLayout::kUnknown;
}

PTOScalarValue ParseScalarValue(PTODType dtype, const std::string& value) {
  switch (dtype) {
    case PTODType::kBool:
      return value == "true";
    case PTODType::kInt32:
    case PTODType::kInt64:
      return static_cast<std::int64_t>(std::stoll(value));
    case PTODType::kUInt64:
      return static_cast<std::uint64_t>(std::stoull(value));
    case PTODType::kFloat32:
      return std::stod(value);
    case PTODType::kUnknown:
    default:
      return std::monostate{};
  }
}

PTOTileReg ParseTileReg(const std::string& object) {
  const std::string address = ExtractJsonStringField(object, "address").value_or("");
  const std::string tile_type = ExtractJsonStringField(object, "tile_type").value_or("");
  const std::string dtype = ExtractJsonStringField(object, "dtype").value_or("");
  const std::string layout = ExtractJsonStringField(object, "layout").value_or("");
  return PTOTileReg{
      .tile_type = ParseTileType(tile_type),
      .address = ParseHexUint64(address),
      .dtype = ParseDType(dtype),
      .layout = ParseLayout(layout),
      .shape = ParseJsonIntArray(object, "shape"),
  };
}

PTOScalarInput ParseScalarInput(const std::string& object) {
  const std::string dtype = ExtractJsonStringField(object, "dtype").value_or("");
  const PTODType parsed_dtype = ParseDType(dtype);
  const std::string value = ExtractJsonStringField(object, "value").value_or("");
  return PTOScalarInput{
      .dtype = parsed_dtype,
      .value = ParseScalarValue(parsed_dtype, value),
  };
}

std::vector<PTOTileReg> ParseTileRegArray(const std::string& line, const std::string& key) {
  std::vector<PTOTileReg> regs;
  auto segment = ExtractJsonArraySegment(line, key);
  if (!segment.has_value()) {
    return regs;
  }
  for (const std::string& object : SplitJsonObjects(*segment)) {
    regs.push_back(ParseTileReg(object));
  }
  return regs;
}

std::vector<PTOScalarInput> ParseScalarInputArray(const std::string& line, const std::string& key) {
  std::vector<PTOScalarInput> scalars;
  auto segment = ExtractJsonArraySegment(line, key);
  if (!segment.has_value()) {
    return scalars;
  }
  for (const std::string& object : SplitJsonObjects(*segment)) {
    scalars.push_back(ParseScalarInput(object));
  }
  return scalars;
}

}  // namespace

std::optional<std::string> ExtractJsonStringField(const std::string& line, const std::string& key) {
  const std::string pattern = "\"" + key + "\":\"";
  std::size_t start = line.find(pattern);
  if (start == std::string::npos) {
    return std::nullopt;
  }
  start += pattern.size();
  std::size_t end = start;
  while (end < line.size()) {
    if (line[end] == '"' && line[end - 1] != '\\') {
      break;
    }
    ++end;
  }
  if (end >= line.size()) {
    return std::nullopt;
  }
  return line.substr(start, end - start);
}

std::optional<std::uint64_t> ExtractJsonUintField(const std::string& line, const std::string& key) {
  const std::string pattern = "\"" + key + "\":";
  std::size_t start = line.find(pattern);
  if (start == std::string::npos) {
    return std::nullopt;
  }
  start += pattern.size();
  std::size_t end = start;
  while (end < line.size() && line[end] >= '0' && line[end] <= '9') {
    ++end;
  }
  if (end == start) {
    return std::nullopt;
  }
  return static_cast<std::uint64_t>(std::stoull(line.substr(start, end - start)));
}

std::string JsonEscape(const std::string& input) {
  std::ostringstream escaped;
  for (char ch : input) {
    switch (ch) {
      case '\\':
        escaped << "\\\\";
        break;
      case '"':
        escaped << "\\\"";
        break;
      case '\n':
        escaped << "\\n";
        break;
      case '\r':
        escaped << "\\r";
        break;
      case '\t':
        escaped << "\\t";
        break;
      default:
        escaped << ch;
        break;
    }
  }
  return escaped.str();
}

std::vector<std::string> ReadTraceLines(const std::filesystem::path& trace_path) {
  std::ifstream trace_stream(trace_path);
  if (!trace_stream) {
    throw std::runtime_error("failed to open trace file: " + trace_path.string());
  }

  std::vector<std::string> lines;
  std::string line;
  while (std::getline(trace_stream, line)) {
    if (!line.empty()) {
      lines.push_back(line);
    }
  }
  return lines;
}

std::vector<PTOInst> ParsePTOInsts(const std::vector<std::string>& trace_lines) {
  std::vector<PTOInst> insts;
  insts.reserve(trace_lines.size());
  for (std::size_t index = 0; index < trace_lines.size(); ++index) {
    const std::string& line = trace_lines[index];
    const std::string raw_opcode = ExtractJsonStringField(line, "opcode").value_or("UNKNOWN");
    const PTOOpcodeDescriptor descriptor = RequireOpcodeDescriptor(raw_opcode, index);
    insts.push_back(PTOInst{
        .opcode = descriptor.opcode,
        .raw_opcode = raw_opcode,
        .engine_kind = descriptor.engine_kind,
        .block_idx = ExtractJsonUintField(line, "block_idx").value_or(0),
        .sequence_id = ExtractJsonUintField(line, "sequence_id").value_or(0),
        .tile_reg_inputs = ParseTileRegArray(line, "input_tiles"),
        .scalar_inputs = ParseScalarInputArray(line, "scalar_inputs"),
        .tile_reg_outputs = ParseTileRegArray(line, "output_tiles"),
    });
  }
  return insts;
}

}  // namespace davincioo::frontend
