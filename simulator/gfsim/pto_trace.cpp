#include "gfsim/pto_trace.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <fstream>
#include <limits>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <utility>

namespace gfsim {
namespace {

[[noreturn]] void traceError(std::string code, std::string detail) {
  throw std::runtime_error(std::move(code) + ": " + std::move(detail));
}

size_t skipSpace(std::string_view text, size_t at) {
  while (at < text.size() &&
         std::isspace(static_cast<unsigned char>(text[at])))
    ++at;
  return at;
}

size_t fieldValue(std::string_view text, std::string_view field) {
  std::string needle = "\"" + std::string(field) + "\"";
  size_t at = text.find(needle);
  if (at == std::string_view::npos)
    traceError("ACTRACE-MISSING-FIELD", std::string(field));
  at = text.find(':', at + needle.size());
  if (at == std::string_view::npos)
    traceError("ACTRACE-MALFORMED", "missing ':' after " + std::string(field));
  return skipSpace(text, at + 1);
}

std::string stringField(std::string_view text, std::string_view field) {
  size_t at = fieldValue(text, field);
  if (at >= text.size() || text[at] != '"')
    traceError("ACTRACE-FIELD-TYPE", std::string(field) + " must be a string");
  std::string result;
  for (++at; at < text.size(); ++at) {
    char value = text[at];
    if (value == '"')
      return result;
    if (value == '\\') {
      if (++at >= text.size())
        break;
      value = text[at];
    }
    result.push_back(value);
  }
  traceError("ACTRACE-MALFORMED", "unterminated string " + std::string(field));
}

uint64_t uintField(std::string_view text, std::string_view field) {
  size_t at = fieldValue(text, field);
  size_t end = at;
  while (end < text.size() &&
         std::isdigit(static_cast<unsigned char>(text[end])))
    ++end;
  if (end == at)
    traceError("ACTRACE-FIELD-TYPE",
               std::string(field) + " must be an unsigned integer");
  uint64_t result = 0;
  for (; at < end; ++at) {
    unsigned digit = static_cast<unsigned>(text[at] - '0');
    if (result > (std::numeric_limits<uint64_t>::max() - digit) / 10)
      traceError("ACTRACE-FIELD-RANGE", std::string(field));
    result = result * 10 + digit;
  }
  return result;
}

std::string_view arrayField(std::string_view text, std::string_view field) {
  size_t at = fieldValue(text, field);
  if (at >= text.size() || text[at] != '[')
    traceError("ACTRACE-FIELD-TYPE", std::string(field) + " must be an array");
  bool quoted = false;
  bool escaped = false;
  unsigned depth = 0;
  for (size_t end = at; end < text.size(); ++end) {
    char value = text[end];
    if (quoted) {
      if (escaped)
        escaped = false;
      else if (value == '\\')
        escaped = true;
      else if (value == '"')
        quoted = false;
      continue;
    }
    if (value == '"') {
      quoted = true;
      continue;
    }
    if (value == '[')
      ++depth;
    else if (value == ']' && --depth == 0)
      return text.substr(at + 1, end - at - 1);
  }
  traceError("ACTRACE-MALFORMED", "unterminated array " + std::string(field));
}

std::vector<std::string_view> arrayObjects(std::string_view array) {
  std::vector<std::string_view> result;
  bool quoted = false;
  bool escaped = false;
  unsigned depth = 0;
  size_t begin = std::string_view::npos;
  for (size_t at = 0; at < array.size(); ++at) {
    char value = array[at];
    if (quoted) {
      if (escaped)
        escaped = false;
      else if (value == '\\')
        escaped = true;
      else if (value == '"')
        quoted = false;
      continue;
    }
    if (value == '"') {
      quoted = true;
      continue;
    }
    if (value == '{') {
      if (depth++ == 0)
        begin = at;
    } else if (value == '}') {
      if (depth == 0)
        traceError("ACTRACE-MALFORMED", "unbalanced object array");
      if (--depth == 0)
        result.push_back(array.substr(begin, at - begin + 1));
    }
  }
  if (depth != 0)
    traceError("ACTRACE-MALFORMED", "unterminated object");
  return result;
}

std::vector<uint64_t> shapeField(std::string_view tile) {
  std::string_view array = arrayField(tile, "shape");
  std::vector<uint64_t> result;
  size_t at = 0;
  while (at < array.size()) {
    at = skipSpace(array, at);
    if (at == array.size())
      break;
    if (array[at] == ',') {
      ++at;
      continue;
    }
    size_t end = at;
    while (end < array.size() &&
           std::isdigit(static_cast<unsigned char>(array[end])))
      ++end;
    if (end == at)
      traceError("ACTRACE-FIELD-TYPE", "shape extent must be an integer");
    uint64_t extent = 0;
    for (; at < end; ++at)
      extent = extent * 10 + static_cast<unsigned>(array[at] - '0');
    result.push_back(extent);
  }
  return result;
}

uint64_t parseAddress(std::string_view value) {
  int base = 10;
  size_t at = 0;
  if (value.size() > 2 && value[0] == '0' &&
      (value[1] == 'x' || value[1] == 'X')) {
    base = 16;
    at = 2;
  }
  uint64_t result = 0;
  for (; at < value.size(); ++at) {
    char ch = value[at];
    unsigned digit = 0;
    if (ch >= '0' && ch <= '9')
      digit = static_cast<unsigned>(ch - '0');
    else if (base == 16 && ch >= 'a' && ch <= 'f')
      digit = static_cast<unsigned>(ch - 'a' + 10);
    else if (base == 16 && ch >= 'A' && ch <= 'F')
      digit = static_cast<unsigned>(ch - 'A' + 10);
    else
      traceError("ACTRACE-FIELD-TYPE", "invalid tile address");
    if (result > (std::numeric_limits<uint64_t>::max() - digit) /
                     static_cast<unsigned>(base))
      traceError("ACTRACE-FIELD-RANGE", "tile address");
    result = result * static_cast<unsigned>(base) + digit;
  }
  return result;
}

uint64_t dtypeBits(std::string_view dtype) {
  if (dtype == "float64" || dtype == "int64" || dtype == "uint64")
    return 64;
  if (dtype == "float32" || dtype == "int32" || dtype == "uint32")
    return 32;
  if (dtype == "float16" || dtype == "bfloat16" || dtype == "int16" ||
      dtype == "uint16")
    return 16;
  if (dtype == "float8" || dtype == "int8" || dtype == "uint8")
    return 8;
  if (dtype == "int4" || dtype == "uint4")
    return 4;
  traceError("ACTRACE-UNSUPPORTED-DTYPE", std::string(dtype));
}

struct Tile {
  uint64_t address = 0;
  std::string dtype;
  std::vector<uint64_t> shape;
};

std::vector<Tile> tileArray(std::string_view line, std::string_view field) {
  std::vector<Tile> result;
  for (std::string_view object : arrayObjects(arrayField(line, field))) {
    Tile tile;
    tile.address = parseAddress(stringField(object, "address"));
    tile.dtype = stringField(object, "dtype");
    tile.shape = shapeField(object);
    result.push_back(std::move(tile));
  }
  return result;
}

uint64_t elementCount(const Tile &tile) {
  uint64_t count = 1;
  for (uint64_t extent : tile.shape) {
    if (extent != 0 && count > std::numeric_limits<uint64_t>::max() / extent)
      traceError("ACTRACE-FIELD-RANGE", "tile shape");
    count *= extent;
  }
  return count;
}

uint64_t byteCount(const Tile &tile) {
  uint64_t elements = elementCount(tile);
  uint64_t bitsPerElement = dtypeBits(tile.dtype);
  if (elements > std::numeric_limits<uint64_t>::max() / bitsPerElement)
    traceError("ACTRACE-FIELD-RANGE", "tile byte count");
  uint64_t bits = elements * bitsPerElement;
  return bits / 8 + (bits % 8 != 0);
}

enum class Opcode : uint64_t {
  Tassign = 0,
  Tload = 1,
  Textract = 2,
  Tmatmul = 3,
  TmatmulAcc = 4,
  Tstore = 5,
  Tcvt = 6,
  Tmuls = 7,
  Tcolmax = 8,
  Tmax = 9,
  Tsub = 10,
  Texp = 11,
  Tmul = 12,
  Tcolexpandsub = 13,
  Tcolsum = 14,
  Tadd = 15,
  Tmov = 16,
  Tcolexpandmul = 17,
  Trecip = 18,
  Texpands = 19
};

Opcode opcode(std::string_view name) {
  if (name == "TASSIGN")
    return Opcode::Tassign;
  if (name == "TLOAD")
    return Opcode::Tload;
  if (name == "TEXTRACT")
    return Opcode::Textract;
  if (name == "TMATMUL")
    return Opcode::Tmatmul;
  if (name == "TMATMUL_ACC")
    return Opcode::TmatmulAcc;
  if (name == "TSTORE")
    return Opcode::Tstore;
  if (name == "TCVT")
    return Opcode::Tcvt;
  if (name == "TMULS")
    return Opcode::Tmuls;
  if (name == "TCOLMAX")
    return Opcode::Tcolmax;
  if (name == "TMAX")
    return Opcode::Tmax;
  if (name == "TSUB")
    return Opcode::Tsub;
  if (name == "TEXP")
    return Opcode::Texp;
  if (name == "TMUL")
    return Opcode::Tmul;
  if (name == "TCOLEXPANDSUB")
    return Opcode::Tcolexpandsub;
  if (name == "TCOLSUM")
    return Opcode::Tcolsum;
  if (name == "TADD")
    return Opcode::Tadd;
  if (name == "TMOV")
    return Opcode::Tmov;
  if (name == "TCOLEXPANDMUL")
    return Opcode::Tcolexpandmul;
  if (name == "TRECIP")
    return Opcode::Trecip;
  if (name == "TEXPANDS")
    return Opcode::Texpands;
  traceError("ACTRACE-UNSUPPORTED-OPCODE", std::string(name));
}

uint64_t checkedProduct(uint64_t lhs, uint64_t rhs,
                        std::string_view description) {
  if (rhs != 0 && lhs > std::numeric_limits<uint64_t>::max() / rhs)
    traceError("ACTRACE-FIELD-RANGE", std::string(description));
  return lhs * rhs;
}

/// Return trace-semantic work, never architecture cycles. Transfer-like
/// operations use bytes and cube operations use scalar MAC operations.
uint64_t workload(Opcode opcode, const std::vector<Tile> &inputs,
                  const std::vector<Tile> &outputs) {
  uint64_t work = 0;
  auto primaryBytes = [&](const std::vector<Tile> &tiles) {
    return tiles.empty() ? uint64_t{0} : byteCount(tiles.front());
  };
  if (opcode == Opcode::Tload)
    work = primaryBytes(outputs);
  else if (opcode == Opcode::Tstore)
    work = primaryBytes(inputs);
  else if (opcode == Opcode::Tmatmul || opcode == Opcode::TmatmulAcc) {
    size_t aIndex = opcode == Opcode::TmatmulAcc ? 1 : 0;
    size_t bIndex = opcode == Opcode::TmatmulAcc ? 2 : 1;
    if (inputs.size() > bIndex && inputs[aIndex].shape.size() >= 2 &&
        inputs[bIndex].shape.size() >= 2) {
      const auto &a = inputs[aIndex].shape;
      const auto &b = inputs[bIndex].shape;
      work = checkedProduct(a[a.size() - 2], a[a.size() - 1],
                            "matmul workload");
      work = checkedProduct(work, b.back(), "matmul workload");
    } else {
      traceError("ACTRACE-MATMUL-SHAPE",
                 "matmul inputs must provide rank-2 matrix dimensions");
    }
  } else if (opcode == Opcode::Tcolmax || opcode == Opcode::Tcolsum)
    work = primaryBytes(inputs);
  else if (opcode == Opcode::Tassign)
    work = 0;
  else
    work = outputs.empty() ? primaryBytes(inputs) : primaryBytes(outputs);
  if (work > PtoScheduleDescriptor::kMaxWorkload)
    traceError("ACTRACE-WORKLOAD-CAP",
               "raw workload does not fit the 24-bit descriptor field");
  return work;
}

struct StorageKey {
  std::string dtype;
  uint64_t address = 0;
  auto operator<=>(const StorageKey &) const = default;
};

struct ParsedRecord {
  uint64_t sequence = 0;
  Opcode opcode{};
  std::vector<Tile> inputs;
  std::vector<Tile> outputs;
};

uint64_t buildDescriptor(const ParsedRecord &record,
                         const std::array<uint8_t, 3> &dependencies,
                         uint8_t dependencyValid) {
  using D = PtoScheduleDescriptor;
  uint64_t descriptor = record.sequence << D::kSequenceShift;
  descriptor |= static_cast<uint64_t>(record.opcode) << D::kOpcodeShift;
  descriptor |= static_cast<uint64_t>(dependencies[0])
                << D::kDependency0Shift;
  descriptor |= static_cast<uint64_t>(dependencies[1])
                << D::kDependency1Shift;
  descriptor |= static_cast<uint64_t>(dependencies[2])
                << D::kDependency2Shift;
  descriptor |= static_cast<uint64_t>(dependencyValid)
                << D::kDependencyValidShift;
  descriptor |= workload(record.opcode, record.inputs, record.outputs)
                << D::kWorkloadShift;
  return descriptor;
}

} // namespace

void PtoTraceProvider::load(std::string source, const std::string &path) {
  std::ifstream input(path);
  if (!input)
    traceError("ACTRACE-OPEN", path);

  std::vector<ParsedRecord> records;
  std::string line;
  while (std::getline(input, line)) {
    if (std::all_of(line.begin(), line.end(), [](unsigned char ch) {
          return std::isspace(ch);
        }))
      continue;
    if (records.size() == 256)
      traceError("ACTRACE-CAPACITY", "at most 256 records are supported");
    ParsedRecord record;
    record.sequence = uintField(line, "sequence_id");
    if (record.sequence != records.size())
      traceError("ACTRACE-SEQUENCE",
                 "sequence_id must be contiguous and start at zero");
    record.opcode = opcode(stringField(line, "opcode"));
    record.inputs = tileArray(line, "input_tiles");
    record.outputs = tileArray(line, "output_tiles");
    records.push_back(std::move(record));
  }
  if (!input.eof())
    traceError("ACTRACE-READ", path);
  if (records.empty())
    traceError("ACTRACE-EMPTY", path);

  Source parsed;
  parsed.descriptors.reserve(records.size());
  std::map<StorageKey, uint8_t> lastWriter;
  for (const ParsedRecord &record : records) {
    std::array<uint8_t, 3> dependencies{};
    uint8_t dependencyValid = 0;
    size_t dependencyCount = 0;
    std::set<uint8_t> uniqueDependencies;
    bool managedInputs =
        record.opcode != Opcode::Tassign && record.opcode != Opcode::Tload &&
        record.opcode != Opcode::Texpands;
    if (managedInputs) {
      for (const Tile &tile : record.inputs) {
        auto found = lastWriter.find({tile.dtype, tile.address});
        if (found == lastWriter.end() ||
            !uniqueDependencies.insert(found->second).second)
          continue;
        if (dependencyCount == dependencies.size())
          traceError("ACTRACE-DEPENDENCY-CAP",
                     "at most three producer dependencies are supported");
        dependencies[dependencyCount] = found->second;
        dependencyValid |= static_cast<uint8_t>(1U << dependencyCount);
        ++dependencyCount;
      }
    }
    bool managedOutputs =
        record.opcode != Opcode::Tassign && record.opcode != Opcode::Tstore;
    if (managedOutputs)
      for (const Tile &tile : record.outputs)
        lastWriter[{tile.dtype, tile.address}] =
            static_cast<uint8_t>(record.sequence);
    parsed.descriptors.push_back(
        buildDescriptor(record, dependencies, dependencyValid));
  }

  activeSource_ = source;
  sources_[std::move(source)] = std::move(parsed);
}

const PtoTraceProvider::Source &
PtoTraceProvider::requireSource(std::string_view source) const {
  auto found = sources_.find(source);
  if (found == sources_.end())
    traceError("ACTRACE-NOT-LOADED", std::string(source));
  return found->second;
}

size_t PtoTraceProvider::open(std::string_view source) const {
  (void)requireSource(source);
  return 0;
}

TraceNextResult PtoTraceProvider::next(std::string_view source,
                                       size_t cursor) const {
  const Source &loaded = requireSource(source);
  if (cursor >= loaded.descriptors.size())
    return {cursor, 0, false};
  return {cursor + 1, cursor, true};
}

bool PtoTraceProvider::eof(std::string_view source, size_t cursor) const {
  return cursor >= requireSource(source).descriptors.size();
}

size_t PtoTraceProvider::position(std::string_view source,
                                  size_t cursor) const {
  return std::min(cursor, requireSource(source).descriptors.size());
}

uint64_t PtoTraceProvider::decode(uint64_t handle) const {
  if (activeSource_.empty())
    traceError("ACTRACE-NOT-LOADED", "no active trace source");
  const Source &loaded = requireSource(activeSource_);
  if (handle >= loaded.descriptors.size())
    traceError("ACTRACE-HANDLE-RANGE", std::to_string(handle));
  return loaded.descriptors[handle];
}

size_t PtoTraceProvider::recordCount(std::string_view source) const {
  return requireSource(source).descriptors.size();
}

} // namespace gfsim
