#include "gfsim/trace.h"

#include "acir/Bindings/Binding.h"

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/JSON.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <limits>
#include <set>
#include <sstream>

namespace gfsim {
namespace {

using llvm::json::Array;
using llvm::json::Object;
using llvm::json::Value;

std::string pointerToken(llvm::StringRef token) {
  std::string escaped;
  for (char character : token) {
    if (character == '~')
      escaped += "~0";
    else if (character == '/')
      escaped += "~1";
    else
      escaped += character;
  }
  return escaped;
}

bool contains(std::initializer_list<std::string_view> values,
              llvm::StringRef candidate) {
  return std::any_of(values.begin(), values.end(), [&](std::string_view value) {
    return candidate == llvm::StringRef(value.data(), value.size());
  });
}

class TraceParser {
public:
  explicit TraceParser(const TraceValidationLimits &limits) : limits_(limits) {}

  TraceLoadResult parse(std::string_view input) {
    acir::bindings::JsonParseLimits jsonLimits;
    jsonLimits.maxInputBytes = limits_.maxDocumentBytes;
    jsonLimits.maxDepth = limits_.maxNestingDepth;
    jsonLimits.maxStringBytes = limits_.maxStringBytes;
    jsonLimits.maxTotalStringBytes = limits_.maxAggregateDecodedBytes;
    jsonLimits.maxArrayElements =
        std::max({limits_.maxRecordCount, limits_.maxOperandsPerRecord,
                  limits_.maxDependenciesPerRecord});
    jsonLimits.maxObjectMembers = limits_.maxAttributeMembers;
    jsonLimits.maxStructuralWork =
        std::max<size_t>(limits_.maxAggregateDecodedBytes, 1);

    auto parsed = acir::bindings::parseIJson(
        llvm::StringRef(input.data(), input.size()), jsonLimits);
    if (!parsed) {
      std::string message = llvm::toString(parsed.takeError());
      std::string code =
          message.find("limit exceeded") != std::string::npos ||
                  message.find("maximum depth") != std::string::npos
              ? "ACTRACE-LIMIT"
              : "ACTRACE-JSON";
      fail(code, "", std::nullopt, std::move(message));
      return std::move(result_);
    }
    const Object *root = parsed->getAsObject();
    if (!root) {
      fail("ACTRACE-SCHEMA", "", std::nullopt,
           "trace document must be an object");
      return std::move(result_);
    }

    PtoTraceDocument document;
    if (!parseRoot(*root, document))
      return std::move(result_);
    result_.document = std::move(document);
    return std::move(result_);
  }

private:
  bool fail(std::string code, std::string pointer,
            std::optional<uint64_t> sequenceId, std::string message) {
    if (result_.diagnostics.size() < limits_.maxDiagnostics)
      result_.diagnostics.push_back({std::move(code), std::move(pointer),
                                     sequenceId, std::move(message)});
    return false;
  }

  bool spendDecoded(size_t bytes, std::string_view pointer,
                    std::optional<uint64_t> sequenceId = std::nullopt) {
    if (bytes > limits_.maxAggregateDecodedBytes -
                    std::min(decodedBytes_, limits_.maxAggregateDecodedBytes))
      return fail("ACTRACE-LIMIT", std::string(pointer), sequenceId,
                  "aggregate decoded byte limit exceeded");
    decodedBytes_ += bytes;
    return true;
  }

  bool checkObject(const Object &object, std::string_view pointer,
                   std::initializer_list<std::string_view> required,
                   std::initializer_list<std::string_view> allowed,
                   std::optional<uint64_t> sequenceId = std::nullopt) {
    for (const auto &entry : object) {
      llvm::StringRef key = entry.first;
      if (!contains(allowed, key))
        return fail("ACTRACE-SCHEMA",
                    std::string(pointer) + "/" + pointerToken(key), sequenceId,
                    "unknown object member");
    }
    for (std::string_view key : required)
      if (!object.get(key))
        return fail("ACTRACE-SCHEMA",
                    std::string(pointer) + "/" + std::string(key), sequenceId,
                    "required object member is missing");
    return true;
  }

  bool readString(const Value *value, std::string &output,
                  std::string_view pointer,
                  std::optional<uint64_t> sequenceId = std::nullopt,
                  bool allowEmpty = false) {
    auto string = value ? value->getAsString() : std::nullopt;
    if (!string || (!allowEmpty && string->empty()))
      return fail("ACTRACE-SCHEMA", std::string(pointer), sequenceId,
                  "expected a non-empty string");
    output = string->str();
    return spendDecoded(output.size(), pointer, sequenceId);
  }

  bool readUInt(const Value *value, uint64_t &output, std::string_view pointer,
                std::optional<uint64_t> sequenceId = std::nullopt) {
    auto integer = value ? value->getAsUINT64() : std::nullopt;
    if (!integer)
      return fail("ACTRACE-SCHEMA", std::string(pointer), sequenceId,
                  "expected an unsigned 64-bit integer");
    output = *integer;
    return spendDecoded(sizeof(output), pointer, sequenceId);
  }

  bool parseRoot(const Object &root, PtoTraceDocument &document) {
    if (!checkObject(
            root, "",
            {"schema", "version", "contract_epoch", "metadata", "records"},
            {"schema", "version", "contract_epoch", "metadata", "records"}))
      return false;
    auto schema = root.getString("schema");
    auto version = root.getString("version");
    auto epoch = root.getString("contract_epoch");
    if (!schema || *schema != "pto-trace")
      return fail("ACTRACE-SCHEMA", "/schema", std::nullopt,
                  "schema must equal pto-trace");
    if (!version || *version != "0.1")
      return fail("ACTRACE-SCHEMA", "/version", std::nullopt,
                  "version must equal 0.1");
    if (!epoch || *epoch != "0.5")
      return fail("ACTRACE-SCHEMA", "/contract_epoch", std::nullopt,
                  "contract_epoch must equal 0.5");

    const Object *metadata = root.getObject("metadata");
    if (!metadata)
      return fail("ACTRACE-SCHEMA", "/metadata", std::nullopt,
                  "metadata must be an object");
    if (!parseMetadata(*metadata, document.metadata))
      return false;

    const Array *records = root.getArray("records");
    if (!records)
      return fail("ACTRACE-SCHEMA", "/records", std::nullopt,
                  "records must be an array");
    if (records->size() > limits_.maxRecordCount)
      return fail("ACTRACE-LIMIT", "/records", std::nullopt,
                  "record count limit exceeded");

    document.records.reserve(records->size());
    for (size_t index = 0; index < records->size(); ++index) {
      const Object *record = (*records)[index].getAsObject();
      std::string pointer = "/records/" + std::to_string(index);
      if (!record)
        return fail("ACTRACE-SCHEMA", pointer, std::nullopt,
                    "record must be an object");
      PtoTraceRecord decoded;
      if (!parseRecord(*record, pointer, decoded))
        return false;
      document.records.push_back(std::move(decoded));
    }
    if (document.metadata.recordCount &&
        *document.metadata.recordCount != document.records.size())
      return fail("ACTRACE-METADATA", "/metadata/record_count", std::nullopt,
                  "record_count does not match records length");
    if (!validateContentHash(*records, document.metadata.contentHash))
      return false;
    return true;
  }

  bool parseMetadata(const Object &metadata, PtoTraceMetadata &decoded) {
    if (!checkObject(metadata, "/metadata", {},
                     {"producer", "pto_identity", "source_program",
                      "address_spaces", "data_layout", "record_count",
                      "content_hash"}))
      return false;
    if (const Value *value = metadata.get("producer");
        value && !readString(value, decoded.producer, "/metadata/producer"))
      return false;
    if (const Value *value = metadata.get("pto_identity");
        value &&
        !readString(value, decoded.ptoIdentity, "/metadata/pto_identity"))
      return false;
    if (const Value *value = metadata.get("source_program");
        value &&
        !readString(value, decoded.sourceProgram, "/metadata/source_program"))
      return false;
    if (const Value *value = metadata.get("data_layout");
        value &&
        !readString(value, decoded.dataLayout, "/metadata/data_layout"))
      return false;
    if (const Value *value = metadata.get("record_count")) {
      uint64_t count = 0;
      if (!readUInt(value, count, "/metadata/record_count"))
        return false;
      decoded.recordCount = count;
    }
    if (const Value *value = metadata.get("content_hash")) {
      if (!readString(value, decoded.contentHash, "/metadata/content_hash"))
        return false;
      if (decoded.contentHash.size() != 71 ||
          !decoded.contentHash.starts_with("sha256:") ||
          !std::all_of(decoded.contentHash.begin() + 7,
                       decoded.contentHash.end(), [](unsigned char character) {
                         return std::isdigit(character) ||
                                (character >= 'a' && character <= 'f');
                       }))
        return fail("ACTRACE-SCHEMA", "/metadata/content_hash", std::nullopt,
                    "content_hash must be a lowercase sha256 fingerprint");
    }
    if (const Value *value = metadata.get("address_spaces")) {
      const Array *spaces = value->getAsArray();
      if (!spaces)
        return fail("ACTRACE-SCHEMA", "/metadata/address_spaces", std::nullopt,
                    "address_spaces must be an array");
      std::set<std::string> unique;
      for (size_t index = 0; index < spaces->size(); ++index) {
        std::string space;
        std::string pointer =
            "/metadata/address_spaces/" + std::to_string(index);
        if (!readString(&(*spaces)[index], space, pointer))
          return false;
        if (!unique.insert(space).second)
          return fail("ACTRACE-SCHEMA", pointer, std::nullopt,
                      "address_spaces entries must be unique");
        declaredAddressSpaces_.insert(space);
        decoded.addressSpaces.push_back(std::move(space));
      }
    }
    return true;
  }

  bool parseRecord(const Object &record, std::string_view pointer,
                   PtoTraceRecord &decoded) {
    if (!checkObject(
            record, pointer,
            {"sequence_id", "opcode", "operands", "dependencies", "attributes"},
            {"sequence_id", "opcode", "operands", "dependencies", "attributes",
             "issue_time", "source"}))
      return false;
    if (!readUInt(record.get("sequence_id"), decoded.sequenceId,
                  std::string(pointer) + "/sequence_id"))
      return false;
    if (lastSequenceId_ && decoded.sequenceId <= *lastSequenceId_)
      return fail("ACTRACE-IDENTITY", std::string(pointer) + "/sequence_id",
                  decoded.sequenceId,
                  "sequence_id values must be strictly increasing");
    lastSequenceId_ = decoded.sequenceId;

    if (!readString(record.get("opcode"), decoded.opcode,
                    std::string(pointer) + "/opcode", decoded.sequenceId))
      return false;
    if (!isOpcode(decoded.opcode))
      return fail("ACTRACE-SCHEMA", std::string(pointer) + "/opcode",
                  decoded.sequenceId, "opcode has an invalid spelling");

    const Array *operands = record.getArray("operands");
    if (!operands)
      return fail("ACTRACE-SCHEMA", std::string(pointer) + "/operands",
                  decoded.sequenceId, "operands must be an array");
    if (operands->size() > limits_.maxOperandsPerRecord)
      return fail("ACTRACE-LIMIT", std::string(pointer) + "/operands",
                  decoded.sequenceId, "operand count limit exceeded");
    decoded.operands.reserve(operands->size());
    for (size_t index = 0; index < operands->size(); ++index) {
      const Object *operand = (*operands)[index].getAsObject();
      std::string operandPointer =
          std::string(pointer) + "/operands/" + std::to_string(index);
      if (!operand)
        return fail("ACTRACE-SCHEMA", operandPointer, decoded.sequenceId,
                    "operand must be an object");
      PtoTraceOperand decodedOperand;
      if (!parseOperand(*operand, operandPointer, decoded.sequenceId,
                        decodedOperand))
        return false;
      decoded.operands.push_back(std::move(decodedOperand));
    }

    const Array *dependencies = record.getArray("dependencies");
    if (!dependencies)
      return fail("ACTRACE-SCHEMA", std::string(pointer) + "/dependencies",
                  decoded.sequenceId, "dependencies must be an array");
    if (dependencies->size() > limits_.maxDependenciesPerRecord)
      return fail("ACTRACE-LIMIT", std::string(pointer) + "/dependencies",
                  decoded.sequenceId, "dependency count limit exceeded");
    std::set<uint64_t> uniqueDependencies;
    for (size_t index = 0; index < dependencies->size(); ++index) {
      uint64_t dependency = 0;
      std::string dependencyPointer =
          std::string(pointer) + "/dependencies/" + std::to_string(index);
      if (!readUInt(&(*dependencies)[index], dependency, dependencyPointer,
                    decoded.sequenceId))
        return false;
      if (!uniqueDependencies.insert(dependency).second)
        return fail("ACTRACE-DEPENDENCY", dependencyPointer, decoded.sequenceId,
                    "duplicate dependency");
      if (!seenSequenceIds_.contains(dependency))
        return fail("ACTRACE-DEPENDENCY", dependencyPointer, decoded.sequenceId,
                    "dependency must reference an earlier record");
      decoded.dependencies.push_back(dependency);
    }

    const Object *attributes = record.getObject("attributes");
    if (!attributes)
      return fail("ACTRACE-SCHEMA", std::string(pointer) + "/attributes",
                  decoded.sequenceId, "attributes must be an object");
    if (attributes->size() > limits_.maxAttributeMembers)
      return fail("ACTRACE-LIMIT", std::string(pointer) + "/attributes",
                  decoded.sequenceId, "attribute member limit exceeded");
    for (const auto &entry : *attributes) {
      llvm::StringRef key = entry.first;
      PtoValue value;
      std::string attributePointer =
          std::string(pointer) + "/attributes/" + pointerToken(key);
      if (!parseValue(entry.second, attributePointer, decoded.sequenceId,
                      value))
        return false;
      decoded.attributes.emplace(key.str(), std::move(value));
    }

    if (const Value *issue = record.get("issue_time")) {
      uint64_t issueTime = 0;
      if (!readUInt(issue, issueTime, std::string(pointer) + "/issue_time",
                    decoded.sequenceId))
        return false;
      decoded.issueTime = issueTime;
    }
    if (const Object *source = record.getObject("source")) {
      PtoSourceLocation location;
      if (!parseSource(*source, std::string(pointer) + "/source",
                       decoded.sequenceId, location))
        return false;
      decoded.source = std::move(location);
    } else if (record.get("source")) {
      return fail("ACTRACE-SCHEMA", std::string(pointer) + "/source",
                  decoded.sequenceId, "source must be an object");
    }
    seenSequenceIds_.insert(decoded.sequenceId);
    return true;
  }

  bool parseOperand(const Object &operand, std::string_view pointer,
                    uint64_t rootSequenceId, PtoTraceOperand &decoded) {
    std::string kind;
    if (!readString(operand.get("kind"), kind, std::string(pointer) + "/kind",
                    rootSequenceId))
      return false;
    if (kind == "immediate") {
      if (!checkObject(operand, pointer, {"kind", "type", "value"},
                       {"kind", "type", "value"}, rootSequenceId))
        return false;
      decoded.kind = PtoOperandKind::Immediate;
      if (!readString(operand.get("type"), decoded.type,
                      std::string(pointer) + "/type", rootSequenceId))
        return false;
      PtoValue value;
      if (!parseValue(*operand.get("value"), std::string(pointer) + "/value",
                      rootSequenceId, value, true))
        return false;
      decoded.immediate = std::move(value);
      return true;
    }
    if (kind == "buffer" || kind == "tile" || kind == "symbol") {
      if (!checkObject(operand, pointer, {"kind", "id"}, {"kind", "id"},
                       rootSequenceId))
        return false;
      decoded.kind = kind == "buffer" ? PtoOperandKind::Buffer
                     : kind == "tile" ? PtoOperandKind::Tile
                                      : PtoOperandKind::Symbol;
      return readString(operand.get("id"), decoded.id,
                        std::string(pointer) + "/id", rootSequenceId);
    }
    if (kind == "address") {
      if (!checkObject(operand, pointer, {"kind", "space", "value"},
                       {"kind", "space", "value"}, rootSequenceId))
        return false;
      decoded.kind = PtoOperandKind::Address;
      if (!readString(operand.get("space"), decoded.addressSpace,
                      std::string(pointer) + "/space", rootSequenceId) ||
          !readUInt(operand.get("value"), decoded.address,
                    std::string(pointer) + "/value", rootSequenceId))
        return false;
      if (!declaredAddressSpaces_.contains(decoded.addressSpace))
        return fail("ACTRACE-ADDRESS", std::string(pointer) + "/space",
                    rootSequenceId,
                    "address space is absent from trace metadata");
      return true;
    }
    if (kind == "record_result") {
      if (!checkObject(operand, pointer,
                       {"kind", "sequence_id", "result_index"},
                       {"kind", "sequence_id", "result_index"}, rootSequenceId))
        return false;
      decoded.kind = PtoOperandKind::RecordResult;
      if (!readUInt(operand.get("sequence_id"), decoded.sequenceId,
                    std::string(pointer) + "/sequence_id", rootSequenceId) ||
          !readUInt(operand.get("result_index"), decoded.resultIndex,
                    std::string(pointer) + "/result_index", rootSequenceId))
        return false;
      if (!seenSequenceIds_.contains(decoded.sequenceId))
        return fail("ACTRACE-DEPENDENCY", std::string(pointer) + "/sequence_id",
                    rootSequenceId,
                    "record_result must reference an earlier record");
      return true;
    }
    return fail("ACTRACE-SCHEMA", std::string(pointer) + "/kind",
                rootSequenceId, "unknown operand kind");
  }

  bool parseSource(const Object &source, std::string_view pointer,
                   uint64_t sequenceId, PtoSourceLocation &decoded) {
    if (!checkObject(source, pointer, {"file", "line", "column"},
                     {"file", "line", "column"}, sequenceId) ||
        !readString(source.get("file"), decoded.file,
                    std::string(pointer) + "/file", sequenceId) ||
        !readUInt(source.get("line"), decoded.line,
                  std::string(pointer) + "/line", sequenceId) ||
        !readUInt(source.get("column"), decoded.column,
                  std::string(pointer) + "/column", sequenceId))
      return false;
    if (decoded.line == 0 || decoded.column == 0)
      return fail("ACTRACE-SCHEMA", std::string(pointer), sequenceId,
                  "source line and column must be positive");
    return true;
  }

  bool parseValue(const Value &value, std::string_view pointer,
                  uint64_t sequenceId, PtoValue &decoded,
                  bool scalarOnly = false) {
    if (!spendDecoded(1, pointer, sequenceId))
      return false;
    if (value.kind() == Value::Null) {
      decoded.value = std::monostate{};
      return true;
    }
    if (auto boolean = value.getAsBoolean()) {
      decoded.value = *boolean;
      return true;
    }
    if (auto integer = value.getAsInteger()) {
      decoded.value = *integer;
      return true;
    }
    if (auto integer = value.getAsUINT64()) {
      decoded.value = *integer;
      return true;
    }
    if (auto number = value.getAsNumber()) {
      decoded.value = *number;
      return true;
    }
    if (auto string = value.getAsString()) {
      if (!spendDecoded(string->size(), pointer, sequenceId))
        return false;
      decoded.value = string->str();
      return true;
    }
    if (scalarOnly)
      return fail("ACTRACE-SCHEMA", std::string(pointer), sequenceId,
                  "immediate value must be a JSON scalar");
    if (const Array *array = value.getAsArray()) {
      PtoValue::Array output;
      output.reserve(array->size());
      for (size_t index = 0; index < array->size(); ++index) {
        PtoValue element;
        if (!parseValue((*array)[index],
                        std::string(pointer) + "/" + std::to_string(index),
                        sequenceId, element))
          return false;
        output.push_back(std::move(element));
      }
      decoded.value = std::move(output);
      return true;
    }
    if (const Object *object = value.getAsObject()) {
      PtoValue::Object output;
      for (const auto &entry : *object) {
        PtoValue member;
        llvm::StringRef key = entry.first;
        if (!parseValue(entry.second,
                        std::string(pointer) + "/" + pointerToken(key),
                        sequenceId, member) ||
            !spendDecoded(key.size(), pointer, sequenceId))
          return false;
        output.emplace(key.str(), std::move(member));
      }
      decoded.value = std::move(output);
      return true;
    }
    return fail("ACTRACE-SCHEMA", std::string(pointer), sequenceId,
                "unsupported JSON value");
  }

  bool validateContentHash(const Array &records, const std::string &declared) {
    if (declared.empty())
      return true;
    Array copy;
    copy.reserve(records.size());
    for (const Value &record : records)
      copy.push_back(record);
    acir::bindings::JsonParseLimits limits;
    limits.maxInputBytes = limits_.maxDocumentBytes;
    limits.maxDepth = limits_.maxNestingDepth;
    limits.maxStringBytes = limits_.maxStringBytes;
    limits.maxTotalStringBytes = limits_.maxAggregateDecodedBytes;
    limits.maxArrayElements =
        std::max({limits_.maxRecordCount, limits_.maxOperandsPerRecord,
                  limits_.maxDependenciesPerRecord});
    limits.maxObjectMembers = limits_.maxAttributeMembers;
    auto canonical =
        acir::bindings::canonicalizeJson(Value(std::move(copy)), limits);
    if (!canonical)
      return fail("ACTRACE-LIMIT", "/metadata/content_hash", std::nullopt,
                  llvm::toString(canonical.takeError()));
    if (acir::bindings::sha256Fingerprint(*canonical) != declared)
      return fail("ACTRACE-METADATA", "/metadata/content_hash", std::nullopt,
                  "content_hash does not match canonical records bytes");
    return true;
  }

  static bool isOpcode(std::string_view opcode) {
    if (opcode.empty() ||
        !(std::isalpha(static_cast<unsigned char>(opcode.front())) ||
          opcode.front() == '_'))
      return false;
    return std::all_of(opcode.begin() + 1, opcode.end(), [](unsigned char c) {
      return std::isalnum(c) || c == '_' || c == '.';
    });
  }

  const TraceValidationLimits &limits_;
  TraceLoadResult result_;
  size_t decodedBytes_ = 0;
  std::optional<uint64_t> lastSequenceId_;
  std::set<uint64_t> seenSequenceIds_;
  std::set<std::string> declaredAddressSpaces_;
};

} // namespace

std::string TraceLoadResult::primaryDiagnostic() const {
  if (diagnostics.empty())
    return {};
  const TraceDiagnostic &diagnostic = diagnostics.front();
  std::ostringstream output;
  output << diagnostic.code;
  if (!diagnostic.jsonPointer.empty())
    output << " at " << diagnostic.jsonPointer;
  if (diagnostic.sequenceId)
    output << " for sequence_id " << *diagnostic.sequenceId;
  output << ": " << diagnostic.message;
  return output.str();
}

TraceLoadResult parsePtoTrace(std::string_view input,
                              const TraceValidationLimits &limits) {
  return TraceParser(limits).parse(input);
}

bool PtoTraceStream::append(std::string_view bytes) {
  if (finished_ || exceededByteLimit_)
    return false;
  if (buffer_.size() > limits_.maxDocumentBytes ||
      bytes.size() > limits_.maxDocumentBytes - buffer_.size()) {
    exceededByteLimit_ = true;
    return false;
  }
  buffer_.append(bytes);
  return true;
}

TraceLoadResult PtoTraceStream::finish() {
  if (finished_)
    return {.diagnostics = {{"ACTRACE-STREAM", "", std::nullopt,
                             "trace stream was already finished"}}};
  finished_ = true;
  if (exceededByteLimit_)
    return {.diagnostics = {{"ACTRACE-LIMIT", "", std::nullopt,
                             "document byte limit exceeded"}}};
  return parsePtoTrace(buffer_, limits_);
}

} // namespace gfsim
