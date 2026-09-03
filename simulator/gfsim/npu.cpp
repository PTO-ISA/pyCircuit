#include "gfsim/npu.h"

#include "gfsim/components.h"

#include <array>
#include <charconv>
#include <cstddef>
#include <initializer_list>
#include <limits>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <variant>

namespace gfsim {
namespace {

constexpr uint64_t kMaxPortableJsonInteger = 9007199254740991ULL;

// Frozen from davincioo@e73633301cabed0d871ea5ff66e76a91df870aeb
// model/include/davincioo/model/pto_inst.hpp.
constexpr auto kTmaOpcodes = std::to_array<std::string_view>({
    "TLOAD",
    "TSTORE",
    "TSTORE_FP",
    "MGATHER",
    "MSCATTER",
    "TPUT",
    "TGET",
    "TPUT_ASYNC",
    "TGET_ASYNC",
    "TBROADCAST",
    "TREDUCE",
    "TPREFETCH",
    "TIMG2COL",
});

constexpr auto kScalarOpcodes = std::to_array<std::string_view>({
    "RECORD_EVENT",    "WAIT_EVENT",
    "BARRIER",         "TSYNC",
    "TALLOC",          "TFREE",
    "TPUSH",           "TPOP",
    "TNOTIFY",         "TWAIT",
    "TTEST",           "TRESHAPE",
    "TASSIGN",         "TCI",
    "TPRINT",          "SETFMATRIX",
    "SET_IMG2COL_RPT", "SET_IMG2COL_PADDING",
    "TGET_SCALE_ADDR",
});

constexpr auto kCubeOpcodes = std::to_array<std::string_view>({
    "TMATMUL",
    "TMATMUL_MX",
    "TMATMUL_ACC",
    "TMATMUL_BIAS",
    "TMATMUL_MX_ACC",
    "TMATMUL_MX_BIAS",
    "TGEMV",
    "TGEMV_ACC",
    "TGEMV_BIAS",
    "TGEMV_MX",
});

constexpr auto kVectorOpcodes = std::to_array<std::string_view>({
    "TADD",
    "TSUB",
    "TMUL",
    "TDIV",
    "TREM",
    "TAND",
    "TOR",
    "TXOR",
    "TSHL",
    "TSHR",
    "TMAX",
    "TMIN",
    "TPRELU",
    "TCMP",
    "TABS",
    "TEXP",
    "TLOG",
    "TSQRT",
    "TRSQRT",
    "TRECIP",
    "TNEG",
    "TNOT",
    "TRELU",
    "TCVT",
    "TADDC",
    "TSUBC",
    "TSEL",
    "TADDS",
    "TSUBS",
    "TMULS",
    "TDIVS",
    "TREMS",
    "TANDS",
    "TORS",
    "TXORS",
    "TSHLS",
    "TSHRS",
    "TMAXS",
    "TMINS",
    "TLRELU",
    "TAXPY",
    "TCMPS",
    "TADDSC",
    "TSUBSC",
    "TSELS",
    "TEXPANDS",
    "TROWSUM",
    "TROWPROD",
    "TROWMAX",
    "TROWMIN",
    "TROWARGMAX",
    "TROWARGMIN",
    "TROWEXPAND",
    "TCOLSUM",
    "TCOLPROD",
    "TCOLMAX",
    "TCOLMIN",
    "TCOLARGMAX",
    "TCOLARGMIN",
    "TCOLEXPAND",
    "TROWEXPANDDIV",
    "TROWEXPANDMUL",
    "TROWEXPANDSUB",
    "TROWEXPANDADD",
    "TROWEXPANDMAX",
    "TROWEXPANDMIN",
    "TROWEXPANDEXPDIF",
    "TCOLEXPANDDIV",
    "TCOLEXPANDMUL",
    "TCOLEXPANDSUB",
    "TCOLEXPANDADD",
    "TCOLEXPANDMAX",
    "TCOLEXPANDMIN",
    "TCOLEXPANDEXPDIF",
    "TFILLPAD",
    "TFILLPAD_INPLACE",
    "TFILLPAD_EXPAND",
    "TMOV",
    "TMOV_FP",
    "TTRANS",
    "TEXTRACT",
    "TEXTRACT_FP",
    "TTRI",
    "TGATHER",
    "TGATHERB",
    "TSCATTER",
    "TSORT32",
    "TMRGSORT",
    "TPARTADD",
    "TPARTMUL",
    "TPARTMAX",
    "TPARTMIN",
    "TPARTARGMAX",
    "TPARTARGMIN",
    "TRANDOM",
    "TCONCAT",
    "TDEQUANT",
    "TINSERT",
    "TINSERT_FP",
    "TFMOD",
    "TFMODS",
    "THISTOGRAM",
    "TQUANT",
    "TSUBVIEW",
});

static_assert(kTmaOpcodes.size() + kScalarOpcodes.size() + kCubeOpcodes.size() +
                  kVectorOpcodes.size() ==
              146);

template <size_t Size>
constexpr bool contains(const std::array<std::string_view, Size> &values,
                        std::string_view value) {
  for (std::string_view candidate : values)
    if (candidate == value)
      return true;
  return false;
}

std::optional<NpuEngineClass> classify(std::string_view opcode) {
  if (contains(kScalarOpcodes, opcode))
    return NpuEngineClass::Scalar;
  if (contains(kVectorOpcodes, opcode))
    return NpuEngineClass::Vector;
  if (contains(kCubeOpcodes, opcode))
    return NpuEngineClass::Cube;
  if (contains(kTmaOpcodes, opcode))
    return NpuEngineClass::Tma;
  return std::nullopt;
}

NpuDecodeResult reject(std::string code, std::string message) {
  NpuDecodeResult result;
  result.diagnostics.push_back(
      {.code = std::move(code), .message = std::move(message)});
  return result;
}

template <typename T> const T *get(const PtoValue &value) {
  return std::get_if<T>(&value.value);
}

std::optional<uint64_t> unsignedInteger(const PtoValue &value) {
  if (const auto *integer = get<uint64_t>(value))
    return *integer;
  if (const auto *integer = get<int64_t>(value); integer && *integer >= 0)
    return static_cast<uint64_t>(*integer);
  return std::nullopt;
}

const PtoValue *find(const PtoValue::Object &object, std::string_view key) {
  auto iterator = object.find(std::string(key));
  return iterator == object.end() ? nullptr : &iterator->second;
}

bool hasExactKeys(const PtoValue::Object &object,
                  std::initializer_list<std::string_view> keys) {
  if (object.size() != keys.size())
    return false;
  for (std::string_view key : keys)
    if (!object.contains(std::string(key)))
      return false;
  return true;
}

bool isScalarValue(const PtoValue &value) {
  return !std::holds_alternative<std::monostate>(value.value) &&
         !std::holds_alternative<PtoValue::Array>(value.value) &&
         !std::holds_alternative<PtoValue::Object>(value.value);
}

struct TileAttribute {
  std::string_view address;
  std::string_view type;
  std::string_view layout;
  const PtoValue::Array *shape = nullptr;
};

std::optional<TileAttribute> parseTile(const PtoValue &value) {
  const auto *object = get<PtoValue::Object>(value);
  if (!object ||
      !hasExactKeys(*object, {"address", "dtype", "layout", "shape"}))
    return std::nullopt;
  const auto *address = find(*object, "address");
  const auto *dtype = find(*object, "dtype");
  const auto *layout = find(*object, "layout");
  const auto *shape = find(*object, "shape");
  if (!address || !dtype || !layout || !shape)
    return std::nullopt;
  const auto *addressText = get<std::string>(*address);
  const auto *dtypeText = get<std::string>(*dtype);
  const auto *layoutText = get<std::string>(*layout);
  const auto *dimensions = get<PtoValue::Array>(*shape);
  if (!addressText || addressText->empty() || !dtypeText ||
      dtypeText->empty() || !layoutText || layoutText->empty() || !dimensions)
    return std::nullopt;
  for (const PtoValue &dimension : *dimensions)
    if (!get<int64_t>(dimension) && !get<uint64_t>(dimension))
      return std::nullopt;
  return TileAttribute{.address = *addressText,
                       .type = *dtypeText,
                       .layout = *layoutText,
                       .shape = dimensions};
}

struct ScalarAttribute {
  std::string_view type;
  const PtoValue *value = nullptr;
};

std::optional<ScalarAttribute> parseScalar(const PtoValue &value) {
  const auto *object = get<PtoValue::Object>(value);
  if (!object || !hasExactKeys(*object, {"dtype", "value"}))
    return std::nullopt;
  const PtoValue *dtype = find(*object, "dtype");
  const PtoValue *scalarValue = find(*object, "value");
  if (!dtype || !scalarValue)
    return std::nullopt;
  const auto *type = get<std::string>(*dtype);
  if (!type || type->empty() || !isScalarValue(*scalarValue))
    return std::nullopt;
  return ScalarAttribute{.type = *type, .value = scalarValue};
}

std::string tileIdentity(uint64_t blockId, std::string_view address) {
  return "block/" + std::to_string(blockId) + "/tile/" + std::string(address);
}

EventProposal observation(std::string name, const NpuInstruction &instruction) {
  return {.ownerId = kInvalidObjectId,
          .category = "instruction",
          .name = std::move(name),
          .phase = TraceEventPhase::Instant,
          .rootSequenceId = instruction.sequenceId,
          .arguments = {{.name = "block_id", .value = instruction.blockId},
                        {.name = "engine",
                         .value = std::string(toString(instruction.engine))},
                        {.name = "opcode", .value = instruction.opcode}}};
}

uint64_t dependencyFlowId(uint64_t blockId, uint64_t producerSequenceId,
                          uint64_t consumerSequenceId,
                          std::string_view tileIdentity) {
  constexpr uint64_t kOffset = 14695981039346656037ULL;
  constexpr uint64_t kPrime = 1099511628211ULL;
  uint64_t hash = kOffset;
  auto appendByte = [&](uint8_t byte) {
    hash ^= byte;
    hash *= kPrime;
  };
  auto appendInteger = [&](uint64_t value) {
    for (unsigned shift = 0; shift != 64; shift += 8)
      appendByte(static_cast<uint8_t>(value >> shift));
  };
  appendInteger(blockId);
  appendInteger(producerSequenceId);
  appendInteger(consumerSequenceId);
  for (char character : tileIdentity)
    appendByte(static_cast<uint8_t>(character));
  return hash % kMaxPortableJsonInteger + 1;
}

bool issueEntryLess(const NpuIssueEntry &left, const NpuIssueEntry &right) {
  return std::tie(left.instruction.sequenceId, left.stableObjectId) <
         std::tie(right.instruction.sequenceId, right.stableObjectId);
}

} // namespace

std::string_view toString(NpuEngineClass engine) {
  switch (engine) {
  case NpuEngineClass::Scalar:
    return "scalar";
  case NpuEngineClass::Vector:
    return "vector";
  case NpuEngineClass::Cube:
    return "cube";
  case NpuEngineClass::Tma:
    return "tma";
  }
  return "unknown";
}

NpuDecodeResult NpuDecoder::decode(const PtoTraceRecord &record) const {
  std::optional<NpuEngineClass> engine = classify(record.opcode);
  if (!engine)
    return reject("npu_unsupported_opcode",
                  "opcode is not in the pinned DavinciOO opcode table");

  auto attributesIterator = record.attributes.find("davincioo");
  if (attributesIterator == record.attributes.end())
    return reject("npu_missing_davincioo_attributes",
                  "record does not contain attributes.davincioo");
  const auto *attributes = get<PtoValue::Object>(attributesIterator->second);
  if (!attributes ||
      !hasExactKeys(*attributes, {"block_idx", "input_tiles", "operand_roles",
                                  "output_tiles", "scalar_inputs"}))
    return reject("npu_invalid_davincioo_attributes",
                  "attributes.davincioo is not the closed imported shape");

  const PtoValue *blockValue = find(*attributes, "block_idx");
  const PtoValue *inputValue = find(*attributes, "input_tiles");
  const PtoValue *roleValue = find(*attributes, "operand_roles");
  const PtoValue *outputValue = find(*attributes, "output_tiles");
  const PtoValue *scalarValue = find(*attributes, "scalar_inputs");
  if (!blockValue || !inputValue || !roleValue || !outputValue || !scalarValue)
    return reject("npu_invalid_davincioo_attributes",
                  "attributes.davincioo is incomplete");

  std::optional<uint64_t> blockId = unsignedInteger(*blockValue);
  const auto *inputs = get<PtoValue::Array>(*inputValue);
  const auto *roles = get<PtoValue::Array>(*roleValue);
  const auto *outputs = get<PtoValue::Array>(*outputValue);
  const auto *scalars = get<PtoValue::Array>(*scalarValue);
  if (!blockId || !inputs || !roles || !outputs || !scalars)
    return reject("npu_invalid_davincioo_attributes",
                  "attributes.davincioo contains a value of the wrong type");
  if (roles->size() != record.operands.size())
    return reject("npu_operand_role_mismatch",
                  "operand_roles must have one entry per ordered operand");

  NpuInstruction instruction;
  instruction.sequenceId = record.sequenceId;
  instruction.blockId = *blockId;
  instruction.opcode = record.opcode;
  instruction.dependencies = record.dependencies;
  instruction.operands = record.operands;
  instruction.engine = *engine;

  size_t inputIndex = 0;
  size_t outputIndex = 0;
  size_t scalarIndex = 0;
  for (size_t operandIndex = 0; operandIndex < roles->size(); ++operandIndex) {
    const auto *role = get<std::string>((*roles)[operandIndex]);
    if (!role)
      return reject("npu_operand_role_mismatch",
                    "every operand role must be a string");
    const PtoTraceOperand &operand = record.operands[operandIndex];
    if (*role == "input_tile" || *role == "output_tile") {
      const bool input = *role == "input_tile";
      const PtoValue::Array &tiles = input ? *inputs : *outputs;
      size_t &tileIndex = input ? inputIndex : outputIndex;
      if (tileIndex >= tiles.size())
        return reject("npu_operand_role_mismatch",
                      "tile role count does not match imported tile arrays");
      std::optional<TileAttribute> tile = parseTile(tiles[tileIndex++]);
      if (!tile)
        return reject("npu_invalid_davincioo_attributes",
                      "tile attributes are malformed");
      std::string identity = tileIdentity(*blockId, tile->address);
      if (operand.kind != PtoOperandKind::Tile || operand.id != identity)
        return reject(
            "npu_tile_identity_mismatch",
            "ordered tile operand does not match its imported identity");
      (input ? instruction.inputTiles : instruction.outputTiles)
          .push_back(std::move(identity));
      (input ? instruction.inputTileDescriptors
             : instruction.outputTileDescriptors)
          .push_back({.identity = tileIdentity(*blockId, tile->address),
                      .address = std::string(tile->address),
                      .type = std::string(tile->type),
                      .layout = std::string(tile->layout),
                      .shape = *tile->shape});
      continue;
    }
    if (*role == "scalar_input") {
      if (scalarIndex >= scalars->size())
        return reject("npu_operand_role_mismatch",
                      "scalar role count does not match scalar_inputs");
      std::optional<ScalarAttribute> scalar =
          parseScalar((*scalars)[scalarIndex++]);
      if (!scalar)
        return reject("npu_invalid_davincioo_attributes",
                      "scalar input attributes are malformed");
      if (operand.kind != PtoOperandKind::Immediate ||
          operand.type != scalar->type || !operand.immediate ||
          *operand.immediate != *scalar->value)
        return reject("npu_scalar_mismatch",
                      "ordered immediate does not match its imported scalar");
      instruction.scalarInputs.push_back(
          {.type = std::string(scalar->type), .value = *scalar->value});
      continue;
    }
    return reject("npu_operand_role_mismatch",
                  "operand role is not supported by the DavinciOO adapter");
  }

  if (inputIndex != inputs->size() || outputIndex != outputs->size() ||
      scalarIndex != scalars->size())
    return reject("npu_operand_role_mismatch",
                  "imported operand arrays contain unreferenced entries");

  NpuDecodeResult result;
  result.observations.push_back(observation("decode", instruction));
  result.observations.push_back(observation("dispatch", instruction));
  result.instruction = std::move(instruction);
  return result;
}

NpuDependencyTracker::NpuDependencyTracker(std::string name, ObjectId id,
                                           SimObject *parent,
                                           NpuIssueQueueCapacities capacities,
                                           ObservationSink *observations)
    : SimObject(ObjectKind::Scheduler, std::move(name), id, parent,
                observations),
      capacities_(capacities) {}

size_t NpuDependencyTracker::engineIndex(NpuEngineClass engine) {
  switch (engine) {
  case NpuEngineClass::Scalar:
    return 0;
  case NpuEngineClass::Vector:
    return 1;
  case NpuEngineClass::Cube:
    return 2;
  case NpuEngineClass::Tma:
    return 3;
  }
  return 0;
}

size_t NpuDependencyTracker::capacity(NpuEngineClass engine) const {
  switch (engine) {
  case NpuEngineClass::Scalar:
    return capacities_.scalar;
  case NpuEngineClass::Vector:
    return capacities_.vector;
  case NpuEngineClass::Cube:
    return capacities_.cube;
  case NpuEngineClass::Tma:
    return capacities_.tma;
  }
  return 0;
}

bool NpuDependencyTracker::knownSequence(uint64_t sequenceId) const {
  if (completedSequences_.contains(sequenceId) ||
      outstandingSequences_.contains(sequenceId))
    return true;
  if (std::ranges::any_of(dispatchProposals_,
                          [&](const auto &proposal) {
                            return proposal.instruction.sequenceId ==
                                   sequenceId;
                          }) ||
      std::ranges::any_of(acceptedDispatches_, [&](const auto &entry) {
        return entry.instruction.sequenceId == sequenceId;
      }))
    return true;
  for (const auto &queue : queues_)
    if (std::ranges::any_of(queue, [&](const auto &entry) {
          return entry.instruction.sequenceId == sequenceId;
        }))
      return true;
  return false;
}

bool NpuDependencyTracker::ready(const NpuIssueEntry &entry) const {
  return std::ranges::all_of(
      entry.derivedDependencies, [&](const NpuDependency &dependency) {
        return completedSequences_.contains(dependency.producerSequenceId);
      });
}

bool NpuDependencyTracker::proposeDispatch(const NpuInstruction &instruction,
                                           ObjectId stableObjectId) {
  if (stableObjectId == kInvalidObjectId ||
      (lastDispatchedSequence_ &&
       instruction.sequenceId <= *lastDispatchedSequence_) ||
      knownSequence(instruction.sequenceId))
    return false;
  dispatchProposals_.push_back({instruction, stableObjectId});
  return true;
}

bool NpuDependencyTracker::proposeIssue(NpuEngineClass engine) {
  const size_t index = engineIndex(engine);
  if (issueProposals_[index])
    return false;
  const auto &queue = queues_[index];
  const NpuIssueEntry *candidate = nullptr;
  for (const NpuIssueEntry &entry : queue)
    if (ready(entry) && (!candidate || issueEntryLess(entry, *candidate)))
      candidate = &entry;
  if (!candidate)
    return false;
  issueProposals_[index] = *candidate;
  return true;
}

bool NpuDependencyTracker::proposeComplete(uint64_t sequenceId) {
  if (!outstandingSequences_.contains(sequenceId) ||
      std::ranges::find(completionProposals_, sequenceId) !=
          completionProposals_.end())
    return false;
  completionProposals_.push_back(sequenceId);
  return true;
}

void NpuDependencyTracker::doArbitrate(Epoch) {
  std::sort(
      dispatchProposals_.begin(), dispatchProposals_.end(),
      [](const DispatchProposal &left, const DispatchProposal &right) {
        return std::tie(left.instruction.sequenceId, left.stableObjectId) <
               std::tie(right.instruction.sequenceId, right.stableObjectId);
      });

  std::array<size_t, 4> remaining{};
  for (size_t index = 0; index < queues_.size(); ++index) {
    const size_t released = issueProposals_[index] ? 1 : 0;
    const size_t occupied = queues_[index].size() - released;
    NpuEngineClass engine = static_cast<NpuEngineClass>(index);
    remaining[index] =
        capacity(engine) > occupied ? capacity(engine) - occupied : 0;
  }

  std::set<uint64_t> shadowFlowIds = usedFlowIds_;
  bool dispatchBlocked = false;
  for (const DispatchProposal &proposal : dispatchProposals_) {
    const size_t index = engineIndex(proposal.instruction.engine);
    if (dispatchBlocked || remaining[index] == 0) {
      proposedRejectedDispatches_.push_back(proposal.instruction.sequenceId);
      dispatchBlocked = true;
      continue;
    }

    NpuIssueEntry entry{.instruction = proposal.instruction,
                        .stableObjectId = proposal.stableObjectId};
    for (const std::string &tile : entry.instruction.inputTiles) {
      auto producer = producers_.find({entry.instruction.blockId, tile});
      if (producer == producers_.end())
        continue;
      uint64_t flow =
          dependencyFlowId(entry.instruction.blockId, producer->second,
                           entry.instruction.sequenceId, tile);
      for (size_t attempts = 0; shadowFlowIds.contains(flow); ++attempts) {
        if (attempts >= shadowFlowIds.size()) {
          setRuntimeFailureCode("npu_dependency_flow_id_exhausted");
          break;
        }
        flow = flow == kMaxPortableJsonInteger ? 1 : flow + 1;
      }
      if (!runtimeFailureCode().empty())
        break;
      shadowFlowIds.insert(flow);
      entry.derivedDependencies.push_back(
          {.producerSequenceId = producer->second,
           .tileIdentity = tile,
           .flowId = flow});
    }
    if (!runtimeFailureCode().empty()) {
      proposedRejectedDispatches_.push_back(proposal.instruction.sequenceId);
      dispatchBlocked = true;
      continue;
    }
    --remaining[index];
    acceptedDispatches_.push_back(std::move(entry));
  }

  for (const NpuIssueEntry &entry : acceptedDispatches_) {
    emitObservation(
        {.category = "instruction",
         .name = "dispatch",
         .phase = TraceEventPhase::Instant,
         .rootSequenceId = entry.instruction.sequenceId,
         .arguments = {
             {"engine", std::string(toString(entry.instruction.engine))},
             {"stable_object_id",
              static_cast<uint64_t>(entry.stableObjectId)}}});
    for (const NpuDependency &dependency : entry.derivedDependencies)
      emitObservation(
          {.category = "dependency",
           .name = "tile",
           .phase = TraceEventPhase::FlowStart,
           .rootSequenceId = entry.instruction.sequenceId,
           .flowId = dependency.flowId,
           .arguments = {
               {"producer_sequence_id", dependency.producerSequenceId},
               {"tile_identity", dependency.tileIdentity}}});
  }
  for (uint64_t sequenceId : proposedRejectedDispatches_)
    emitObservation({.category = "stall",
                     .name = "issue_queue_capacity",
                     .phase = TraceEventPhase::Instant,
                     .rootSequenceId = sequenceId});

  std::vector<const NpuIssueEntry *> issues;
  for (const auto &proposal : issueProposals_)
    if (proposal)
      issues.push_back(&*proposal);
  std::sort(issues.begin(), issues.end(),
            [](const auto *left, const auto *right) {
              return issueEntryLess(*left, *right);
            });
  for (const NpuIssueEntry *entry : issues) {
    emitObservation(
        {.category = "instruction",
         .name = "issue",
         .phase = TraceEventPhase::Instant,
         .rootSequenceId = entry->instruction.sequenceId,
         .arguments = {
             {"engine", std::string(toString(entry->instruction.engine))},
             {"stable_object_id",
              static_cast<uint64_t>(entry->stableObjectId)}}});
    for (const NpuDependency &dependency : entry->derivedDependencies)
      emitObservation(
          {.category = "dependency",
           .name = "tile",
           .phase = TraceEventPhase::FlowEnd,
           .rootSequenceId = entry->instruction.sequenceId,
           .flowId = dependency.flowId,
           .arguments = {
               {"producer_sequence_id", dependency.producerSequenceId},
               {"tile_identity", dependency.tileIdentity}}});
  }

  std::sort(completionProposals_.begin(), completionProposals_.end());
  for (uint64_t producer : completionProposals_)
    for (const auto &queue : queues_)
      for (const NpuIssueEntry &entry : queue)
        if (std::ranges::any_of(entry.derivedDependencies,
                                [&](const NpuDependency &dependency) {
                                  return dependency.producerSequenceId ==
                                         producer;
                                }))
          emitObservation({.category = "dependency",
                           .name = "ready",
                           .phase = TraceEventPhase::Instant,
                           .rootSequenceId = entry.instruction.sequenceId,
                           .arguments = {{"producer_sequence_id", producer}}});
}

void NpuDependencyTracker::doXfer(Epoch epoch) {
  const bool changed = hasPendingCommit();
  issued_.clear();
  acceptedDispatchSequences_.clear();
  rejectedDispatches_ = proposedRejectedDispatches_;

  for (NpuIssueEntry &entry : acceptedDispatches_) {
    entry.instruction.timestamps.dispatched = epoch.time;
    for (const std::string &tile : entry.instruction.outputTiles)
      producers_[{entry.instruction.blockId, tile}] =
          entry.instruction.sequenceId;
    for (const NpuDependency &dependency : entry.derivedDependencies)
      usedFlowIds_.insert(dependency.flowId);
    acceptedDispatchSequences_.insert(entry.instruction.sequenceId);
    lastDispatchedSequence_ = entry.instruction.sequenceId;
    queues_[engineIndex(entry.instruction.engine)].push_back(std::move(entry));
    ++totalDispatches_;
  }
  totalDispatchStalls_ += proposedRejectedDispatches_.size();

  for (size_t index = 0; index < issueProposals_.size(); ++index) {
    if (!issueProposals_[index])
      continue;
    auto &queue = queues_[index];
    auto position =
        std::ranges::find_if(queue, [&](const NpuIssueEntry &entry) {
          return entry.instruction.sequenceId ==
                     issueProposals_[index]->instruction.sequenceId &&
                 entry.stableObjectId == issueProposals_[index]->stableObjectId;
        });
    if (position == queue.end()) {
      setRuntimeFailureCode("npu_issue_entry_missing");
      continue;
    }
    position->instruction.timestamps.issued = epoch.time;
    outstandingSequences_.insert(position->instruction.sequenceId);
    issued_.push_back(std::move(*position));
    queue.erase(position);
    ++totalIssues_;
  }
  std::sort(issued_.begin(), issued_.end(), issueEntryLess);

  for (uint64_t sequenceId : completionProposals_) {
    for (const auto &queue : queues_)
      for (const NpuIssueEntry &entry : queue)
        totalDependencyWakeups_ += static_cast<uint64_t>(std::ranges::count_if(
            entry.derivedDependencies, [&](const NpuDependency &dependency) {
              return dependency.producerSequenceId == sequenceId;
            }));
    completedSequences_.insert(sequenceId);
    outstandingSequences_.erase(sequenceId);
  }

  for (size_t index = 0; index < queues_.size(); ++index) {
    std::sort(queues_[index].begin(), queues_[index].end(), issueEntryLess);
    highWatermarks_[index] =
        std::max(highWatermarks_[index], queues_[index].size());
  }

  dispatchProposals_.clear();
  acceptedDispatches_.clear();
  proposedRejectedDispatches_.clear();
  for (auto &proposal : issueProposals_)
    proposal.reset();
  completionProposals_.clear();
  if (changed)
    lastUpdate_ = epoch;
}

bool NpuDependencyTracker::hasPendingCommit() const {
  return !dispatchProposals_.empty() || !acceptedDispatches_.empty() ||
         !proposedRejectedDispatches_.empty() ||
         std::ranges::any_of(
             issueProposals_,
             [](const auto &entry) { return entry.has_value(); }) ||
         !completionProposals_.empty();
}

bool NpuDependencyTracker::isRunnable(Epoch) const {
  return !dispatchProposals_.empty() || !completionProposals_.empty();
}

RuntimeObjectState NpuDependencyTracker::runtimeState(Epoch epoch) const {
  RuntimeObjectState state = SimObject::runtimeState(epoch);
  for (const auto &queue : queues_)
    state.queueOccupancy += queue.size();
  state.pendingOffers = dispatchProposals_.size() + completionProposals_.size();
  state.activeReservations = outstandingSequences_.size();
  state.quiescent = state.queueOccupancy == 0 && state.pendingOffers == 0 &&
                    state.activeReservations == 0 && !hasPendingCommit();
  if (!state.quiescent)
    state.reason =
        state.pendingOffers != 0
            ? "npu_pending_proposal"
            : (state.queueOccupancy != 0 ? "npu_issue_queue_not_empty"
                                         : "npu_execution_outstanding");
  return state;
}

void NpuDependencyTracker::collectStatistics(
    std::vector<StatSnapshot> &out) const {
  auto append = [&](std::string name, StatisticKind kind, uint64_t value) {
    out.push_back({.name = std::move(name),
                   .objectPath = std::string(path()),
                   .kind = kind,
                   .value = value,
                   .lastUpdate = lastUpdate_});
  };
  constexpr std::array names = {"scalar", "vector", "cube", "tma"};
  for (size_t index = 0; index < queues_.size(); ++index) {
    append("issue_queue_occupancy_" + std::string(names[index]),
           StatisticKind::Gauge, queues_[index].size());
    append("issue_queue_peak_" + std::string(names[index]),
           StatisticKind::Gauge, highWatermarks_[index]);
  }
  append("dispatch_stalls", StatisticKind::Counter, totalDispatchStalls_);
  append("dispatched_instructions", StatisticKind::Counter, totalDispatches_);
  append("issued_instructions", StatisticKind::Counter, totalIssues_);
  append("dependency_wakeups", StatisticKind::Counter, totalDependencyWakeups_);
}

const NpuIssueEntry *
NpuDependencyTracker::proposedIssue(NpuEngineClass engine) const {
  const auto &proposal = issueProposals_[engineIndex(engine)];
  return proposal ? &*proposal : nullptr;
}

std::vector<NpuIssueEntry>
NpuDependencyTracker::queued(NpuEngineClass engine) const {
  return queues_[engineIndex(engine)];
}

std::vector<NpuDependency>
NpuDependencyTracker::dependencies(uint64_t sequenceId) const {
  for (const auto &queue : queues_)
    for (const NpuIssueEntry &entry : queue)
      if (entry.instruction.sequenceId == sequenceId)
        return entry.derivedDependencies;
  return {};
}

bool NpuDependencyTracker::isReady(uint64_t sequenceId) const {
  for (const auto &queue : queues_)
    for (const NpuIssueEntry &entry : queue)
      if (entry.instruction.sequenceId == sequenceId)
        return ready(entry);
  return false;
}

bool NpuDependencyTracker::dispatchAccepted(uint64_t sequenceId) const {
  return acceptedDispatchSequences_.contains(sequenceId);
}

size_t NpuDependencyTracker::queueSize(NpuEngineClass engine) const {
  return queues_[engineIndex(engine)].size();
}

void NpuDependencyTracker::reset() {
  for (auto &queue : queues_)
    queue.clear();
  dispatchProposals_.clear();
  acceptedDispatches_.clear();
  proposedRejectedDispatches_.clear();
  rejectedDispatches_.clear();
  acceptedDispatchSequences_.clear();
  for (auto &proposal : issueProposals_)
    proposal.reset();
  issued_.clear();
  completionProposals_.clear();
  outstandingSequences_.clear();
  completedSequences_.clear();
  producers_.clear();
  usedFlowIds_.clear();
  lastDispatchedSequence_.reset();
  highWatermarks_.fill(0);
  totalDispatches_ = 0;
  totalDispatchStalls_ = 0;
  totalIssues_ = 0;
  totalDependencyWakeups_ = 0;
  lastUpdate_ = {};
  clearRuntimeFailureCode();
}

struct NpuExecutionPipeline::Impl {
  struct RobEntry {
    NpuInstruction instruction;
    bool completed = false;
  };

  struct ActiveExecution {
    NpuIssueEntry entry;
    Epoch issueEpoch;
    Epoch readyEpoch;
    size_t unitIndex = 0;
    bool memoryDelivered = false;
    std::optional<NpuMemoryRequest> memoryRequest;
  };

  explicit Impl(NpuExecutionConfig configuration)
      : config(configuration),
        memoryProtocol("memory_protocol", kInvalidObjectId, nullptr,
                       configuration.memoryRequests),
        memoryController("memory_controller", kInvalidObjectId, nullptr,
                         static_cast<uint32_t>(std::min<size_t>(
                             configuration.memoryRequests,
                             std::numeric_limits<uint32_t>::max()))) {}

  NpuExecutionConfig config;
  SimSystem *system = nullptr;
  RequestResponse<NpuMemoryRequest, NpuMemoryResponse> memoryProtocol;
  Resource memoryController;
  std::map<uint64_t, std::vector<RobEntry>> rob;
  std::map<uint64_t, uint64_t> lastAdmittedSequence;
  std::vector<NpuInstruction> admissionProposals;
  std::vector<ActiveExecution> executionProposals;
  std::map<uint64_t, ActiveExecution> active;
  std::vector<uint64_t> deliveryProposals;
  std::vector<uint64_t> completionProposals;
  bool traceExhaustedProposal = false;
  bool traceExhausted = false;
  std::set<uint64_t> executedSequences;
  std::vector<NpuIssueEntry> completed;
  std::vector<NpuInstruction> retired;
  std::vector<NpuMemoryRequest> memoryRequests;
  std::vector<NpuMemoryResponse> memoryResponses;
  std::map<std::string, uint64_t> scratchpad;
  std::map<uint64_t, uint64_t> globalMemory;
  NpuArchitecturalResult result;
  std::array<size_t, 4> unitHighWatermarks{};
  uint64_t totalExecutions = 0;
  uint64_t totalCompletions = 0;
  uint64_t totalUnitStalls = 0;
  uint64_t unitStallProposals = 0;
  uint64_t totalMemoryRequests = 0;
  Epoch lastUpdate;
};

namespace {

constexpr uint32_t kNpuCompletionEvent = 0x4e505501;
constexpr uint32_t kNpuMemoryServiceEvent = 0x4e505502;
constexpr uint32_t kNpuMemoryCleanupEvent = 0x4e505503;

size_t configuredUnits(const NpuExecutionConfig &config,
                       NpuEngineClass engine) {
  switch (engine) {
  case NpuEngineClass::Scalar:
    return config.scalarUnits;
  case NpuEngineClass::Vector:
    return config.vectorUnits;
  case NpuEngineClass::Cube:
    return config.cubeUnits;
  case NpuEngineClass::Tma:
    return config.tmaUnits;
  }
  return 0;
}

std::optional<uint64_t> parseAddress(std::string_view text) {
  if (text.size() < 3 || !text.starts_with("0x"))
    return std::nullopt;
  uint64_t result = 0;
  auto [position, error] =
      std::from_chars(text.data() + 2, text.data() + text.size(), result, 16);
  if (error != std::errc{} || position != text.data() + text.size())
    return std::nullopt;
  return result;
}

void updateDigest(uint64_t &digest, const NpuInstruction &instruction) {
  constexpr uint64_t kPrime = 1099511628211ULL;
  auto append = [&](uint8_t byte) {
    digest ^= byte;
    digest *= kPrime;
  };
  auto appendInteger = [&](uint64_t value) {
    for (unsigned shift = 0; shift != 64; shift += 8)
      append(static_cast<uint8_t>(value >> shift));
  };
  appendInteger(instruction.sequenceId);
  appendInteger(instruction.blockId);
  for (char character : instruction.opcode)
    append(static_cast<uint8_t>(character));
  append(0);
  for (const std::string &tile : instruction.outputTiles) {
    for (char character : tile)
      append(static_cast<uint8_t>(character));
    append(0);
  }
}

} // namespace

NpuExecutionPipeline::NpuExecutionPipeline(std::string name, ObjectId id,
                                           SimObject *parent,
                                           NpuExecutionConfig config,
                                           SimSystem *system,
                                           ObservationSink *observations)
    : SimObject(ObjectKind::Compute, std::move(name), id, parent, observations),
      impl_(std::make_unique<Impl>(config)) {
  impl_->system = system;
  if (config.memoryRequests > std::numeric_limits<uint32_t>::max())
    setRuntimeFailureCode("npu_memory_capacity_invalid");
}

NpuExecutionPipeline::~NpuExecutionPipeline() = default;

bool NpuExecutionPipeline::proposeAdmit(const NpuInstruction &instruction) {
  auto matches = [&](const NpuInstruction &candidate) {
    return candidate.sequenceId == instruction.sequenceId;
  };
  if (std::ranges::any_of(impl_->admissionProposals, matches))
    return false;
  if (auto previous = impl_->lastAdmittedSequence.find(instruction.blockId);
      previous != impl_->lastAdmittedSequence.end() &&
      instruction.sequenceId <= previous->second)
    return false;
  for (const auto &[blockId, entries] : impl_->rob)
    if (std::ranges::any_of(entries, [&](const Impl::RobEntry &entry) {
          return entry.instruction.sequenceId == instruction.sequenceId;
        }))
      return false;
  impl_->admissionProposals.push_back(instruction);
  return true;
}

uint64_t
NpuExecutionPipeline::executionLatency(const NpuInstruction &instruction) {
  switch (instruction.engine) {
  case NpuEngineClass::Scalar:
    return 1;
  case NpuEngineClass::Vector:
    if (instruction.opcode == "TEXP" || instruction.opcode == "TLOG" ||
        instruction.opcode == "TSQRT" || instruction.opcode == "TRSQRT")
      return 3;
    return 2;
  case NpuEngineClass::Cube:
    if (instruction.opcode.starts_with("TMATMUL_MX"))
      return 5;
    return 4;
  case NpuEngineClass::Tma:
    if (instruction.opcode == "TSTORE" || instruction.opcode == "TSTORE_FP")
      return 4;
    return 3;
  }
  return 1;
}

bool NpuExecutionPipeline::proposeExecute(const NpuIssueEntry &entry,
                                          Epoch issueEpoch) {
  const uint64_t sequenceId = entry.instruction.sequenceId;
  const NpuInstruction *admitted = nullptr;
  for (const auto &[blockId, entries] : impl_->rob)
    for (const Impl::RobEntry &rob : entries)
      if (rob.instruction.sequenceId == sequenceId)
        admitted = &rob.instruction;
  if (!admitted || impl_->executedSequences.contains(sequenceId) ||
      impl_->active.contains(sequenceId) ||
      std::ranges::any_of(impl_->executionProposals,
                          [&](const Impl::ActiveExecution &proposal) {
                            return proposal.entry.instruction.sequenceId ==
                                   sequenceId;
                          }))
    return false;
  NpuInstruction admittedIdentity = *admitted;
  NpuInstruction issuedIdentity = entry.instruction;
  admittedIdentity.timestamps = {};
  issuedIdentity.timestamps = {};
  if (admittedIdentity != issuedIdentity) {
    setRuntimeFailureCode("npu_execution_identity_mismatch");
    return false;
  }

  size_t occupied = activeExecutions(entry.instruction.engine);
  occupied += std::ranges::count_if(
      impl_->executionProposals, [&](const Impl::ActiveExecution &proposal) {
        return proposal.entry.instruction.engine == entry.instruction.engine;
      });
  if (occupied >= configuredUnits(impl_->config, entry.instruction.engine)) {
    ++impl_->unitStallProposals;
    return false;
  }

  const uint64_t latency = executionLatency(entry.instruction);
  if (issueEpoch.time > std::numeric_limits<uint64_t>::max() - latency) {
    setRuntimeFailureCode("npu_execution_time_overflow");
    return false;
  }
  Epoch readyEpoch{issueEpoch.time + latency, 0};
  size_t unit = 0;
  auto occupiedUnit = [&](size_t candidate) {
    return std::ranges::any_of(
               impl_->active,
               [&](const auto &active) {
                 return active.second.entry.instruction.engine ==
                            entry.instruction.engine &&
                        active.second.unitIndex == candidate;
               }) ||
           std::ranges::any_of(impl_->executionProposals,
                               [&](const Impl::ActiveExecution &proposal) {
                                 return proposal.entry.instruction.engine ==
                                            entry.instruction.engine &&
                                        proposal.unitIndex == candidate;
                               });
  };
  while (occupiedUnit(unit))
    ++unit;

  Impl::ActiveExecution proposal{.entry = entry,
                                 .issueEpoch = issueEpoch,
                                 .readyEpoch = readyEpoch,
                                 .unitIndex = unit};
  const bool store = entry.instruction.opcode.starts_with("TSTORE");
  std::set<std::string> pendingTiles;
  for (const auto &[activeSequence, active] : impl_->active)
    if (!active.entry.instruction.opcode.starts_with("TSTORE"))
      for (const NpuTileDescriptor &tile :
           active.entry.instruction.outputTileDescriptors)
        if (!impl_->scratchpad.contains(tile.identity))
          pendingTiles.insert(tile.identity);
  for (const Impl::ActiveExecution &pending : impl_->executionProposals)
    if (!pending.entry.instruction.opcode.starts_with("TSTORE"))
      for (const NpuTileDescriptor &tile :
           pending.entry.instruction.outputTileDescriptors)
        if (!impl_->scratchpad.contains(tile.identity))
          pendingTiles.insert(tile.identity);
  if (!store)
    for (const NpuTileDescriptor &tile :
         entry.instruction.outputTileDescriptors)
      if (!impl_->scratchpad.contains(tile.identity))
        pendingTiles.insert(tile.identity);
  if (impl_->scratchpad.size() + pendingTiles.size() >
      impl_->config.scratchpadTiles) {
    ++impl_->unitStallProposals;
    return false;
  }
  if (store && (entry.instruction.inputTileDescriptors.empty() ||
                !impl_->scratchpad.contains(
                    entry.instruction.inputTileDescriptors.front().identity))) {
    setRuntimeFailureCode("npu_scratchpad_input_missing");
    return false;
  }

  if (entry.instruction.engine == NpuEngineClass::Tma) {
    size_t memoryOccupied =
        std::ranges::count_if(impl_->active, [](const auto &active) {
          return active.second.memoryRequest.has_value();
        });
    memoryOccupied += std::ranges::count_if(
        impl_->executionProposals, [](const Impl::ActiveExecution &pending) {
          return pending.memoryRequest.has_value();
        });
    if (memoryOccupied >= impl_->config.memoryRequests) {
      ++impl_->unitStallProposals;
      return false;
    }
    const bool write = store;
    const auto &descriptors = write ? entry.instruction.outputTileDescriptors
                                    : entry.instruction.inputTileDescriptors;
    if (descriptors.empty() ||
        (!write && entry.instruction.outputTileDescriptors.empty())) {
      setRuntimeFailureCode("npu_memory_descriptor_missing");
      return false;
    }
    std::optional<uint64_t> address = parseAddress(descriptors.front().address);
    if (!address) {
      setRuntimeFailureCode("npu_memory_address_invalid");
      return false;
    }
    NpuMemoryRequest request{
        .sequenceId = sequenceId,
        .correlationId = sequenceId,
        .address = *address,
        .write = write,
        .tileIdentity =
            write ? entry.instruction.inputTileDescriptors.front().identity
                  : entry.instruction.outputTileDescriptors.front().identity};
    proposal.memoryRequest = request;
  }

  if (impl_->system) {
    Epoch wake = entry.instruction.engine == NpuEngineClass::Tma
                     ? Epoch{issueEpoch.time + 1, 0}
                     : readyEpoch;
    uint32_t kind = entry.instruction.engine == NpuEngineClass::Tma
                        ? kNpuMemoryServiceEvent
                        : kNpuCompletionEvent;
    if (!impl_->system->scheduleEvent({wake, id(), kind, sequenceId})) {
      setRuntimeFailureCode("npu_completion_schedule_failed");
      return false;
    }
  }
  impl_->executionProposals.push_back(std::move(proposal));
  return true;
}

bool NpuExecutionPipeline::proposeTraceExhausted() {
  if (impl_->traceExhausted || impl_->traceExhaustedProposal)
    return false;
  impl_->traceExhaustedProposal = true;
  return true;
}

void NpuExecutionPipeline::doWork(Epoch epoch) {
  while (auto response = impl_->memoryProtocol.proposePopResponse())
    (void)response;

  while (const auto *request = impl_->memoryProtocol.peekRequest()) {
    auto active = impl_->active.find(request->correlationId);
    if (active == impl_->active.end()) {
      setRuntimeFailureCode("npu_memory_correlation_missing");
      break;
    }
    if (active->second.issueEpoch.time ==
            std::numeric_limits<uint64_t>::max() ||
        epoch.time < active->second.issueEpoch.time + 1)
      break;
    if (active->second.memoryDelivered) {
      setRuntimeFailureCode("npu_memory_request_delivered_twice");
      break;
    }
    auto popped = impl_->memoryProtocol.proposePopRequest();
    if (!popped)
      break;
    impl_->deliveryProposals.push_back(popped->correlationId);
  }

  for (const auto &[sequenceId, active] : impl_->active) {
    if (active.readyEpoch != epoch ||
        std::ranges::find(impl_->completionProposals, sequenceId) !=
            impl_->completionProposals.end())
      continue;
    if (active.memoryRequest) {
      if (!active.memoryDelivered)
        continue;
      uint64_t value =
          active.memoryRequest->write
              ? impl_->scratchpad.at(active.memoryRequest->tileIdentity)
              : active.memoryRequest->address ^ sequenceId;
      if (!impl_->memoryProtocol.proposeResponse(
              {.correlationId = sequenceId, .value = value}, sequenceId) ||
          !impl_->memoryController.proposeCancel(id(), sequenceId)) {
        setRuntimeFailureCode("npu_memory_response_failed");
        continue;
      }
    }
    impl_->completionProposals.push_back(sequenceId);
  }
}

void NpuExecutionPipeline::doArbitrate(Epoch) {
  std::sort(impl_->admissionProposals.begin(), impl_->admissionProposals.end(),
            [](const NpuInstruction &left, const NpuInstruction &right) {
              return std::tie(left.sequenceId, left.blockId) <
                     std::tie(right.sequenceId, right.blockId);
            });
  std::sort(impl_->executionProposals.begin(), impl_->executionProposals.end(),
            [](const Impl::ActiveExecution &left,
               const Impl::ActiveExecution &right) {
              return issueEntryLess(left.entry, right.entry);
            });
  for (const Impl::ActiveExecution &execution : impl_->executionProposals)
    if (execution.memoryRequest &&
        (!impl_->memoryProtocol.proposeRequest(
             *execution.memoryRequest,
             execution.entry.instruction.sequenceId) ||
         !impl_->memoryController.proposeReserve(
             id(), 1, execution.issueEpoch,
             execution.entry.instruction.sequenceId, execution.readyEpoch,
             execution.entry.instruction.sequenceId)))
      setRuntimeFailureCode("npu_memory_request_failed");
  impl_->memoryProtocol.doArbitrate({});
  impl_->memoryController.doArbitrate({});
  for (const Impl::ActiveExecution &execution : impl_->executionProposals)
    emitObservation(
        {.category = "instruction",
         .name = "execute",
         .phase = TraceEventPhase::Complete,
         .rootSequenceId = execution.entry.instruction.sequenceId,
         .duration = executionLatency(execution.entry.instruction),
         .arguments = {
             {"engine",
              std::string(toString(execution.entry.instruction.engine))},
             {"unit_index", static_cast<uint64_t>(execution.unitIndex)}}});
  for (uint64_t sequenceId : impl_->completionProposals)
    emitObservation({.category = "instruction",
                     .name = "complete",
                     .phase = TraceEventPhase::Instant,
                     .rootSequenceId = sequenceId});
}

void NpuExecutionPipeline::doXfer(Epoch epoch) {
  const bool changed = hasPendingCommit();
  impl_->completed.clear();
  impl_->retired.clear();
  impl_->memoryProtocol.doXfer(epoch);
  impl_->memoryController.doXfer(epoch);

  for (NpuInstruction &instruction : impl_->admissionProposals) {
    auto &entries = impl_->rob[instruction.blockId];
    entries.push_back({.instruction = std::move(instruction)});
    impl_->lastAdmittedSequence[entries.back().instruction.blockId] =
        entries.back().instruction.sequenceId;
    std::sort(entries.begin(), entries.end(),
              [](const auto &left, const auto &right) {
                return left.instruction.sequenceId <
                       right.instruction.sequenceId;
              });
  }

  for (Impl::ActiveExecution &execution : impl_->executionProposals) {
    const uint64_t sequenceId = execution.entry.instruction.sequenceId;
    impl_->executedSequences.insert(sequenceId);
    if (execution.memoryRequest) {
      impl_->memoryRequests.push_back(*execution.memoryRequest);
      ++impl_->totalMemoryRequests;
    }
    impl_->active.emplace(sequenceId, std::move(execution));
    ++impl_->totalExecutions;
  }

  for (uint64_t sequenceId : impl_->deliveryProposals) {
    auto active = impl_->active.find(sequenceId);
    if (active == impl_->active.end()) {
      setRuntimeFailureCode("npu_memory_correlation_missing");
      continue;
    }
    active->second.memoryDelivered = true;
    if (impl_->system &&
        !impl_->system->scheduleEvent(
            {active->second.readyEpoch, id(), kNpuCompletionEvent, sequenceId}))
      setRuntimeFailureCode("npu_completion_schedule_failed");
  }

  std::sort(impl_->completionProposals.begin(),
            impl_->completionProposals.end());
  for (uint64_t sequenceId : impl_->completionProposals) {
    auto active = impl_->active.find(sequenceId);
    if (active == impl_->active.end()) {
      setRuntimeFailureCode("npu_completion_correlation_missing");
      continue;
    }
    active->second.entry.instruction.timestamps.completed = epoch.time;
    NpuIssueEntry completed = active->second.entry;
    for (auto &[blockId, entries] : impl_->rob)
      for (Impl::RobEntry &entry : entries)
        if (entry.instruction.sequenceId == sequenceId) {
          entry.instruction = completed.instruction;
          entry.completed = true;
        }

    uint64_t value = sequenceId;
    if (active->second.memoryRequest) {
      const NpuMemoryRequest &request = *active->second.memoryRequest;
      value = request.write ? impl_->scratchpad.at(request.tileIdentity)
                            : request.address ^ sequenceId;
      if (request.write)
        impl_->globalMemory[request.address] = value;
      impl_->memoryResponses.push_back(
          {.correlationId = request.correlationId, .value = value});
      if (impl_->system) {
        if (epoch.time == std::numeric_limits<uint64_t>::max() ||
            !impl_->system->scheduleEvent({{epoch.time + 1, 0},
                                           id(),
                                           kNpuMemoryCleanupEvent,
                                           sequenceId}))
          setRuntimeFailureCode("npu_completion_schedule_failed");
      }
    }
    if (!completed.instruction.opcode.starts_with("TSTORE"))
      for (const NpuTileDescriptor &tile :
           completed.instruction.outputTileDescriptors)
        impl_->scratchpad[tile.identity] = value;
    impl_->completed.push_back(std::move(completed));
    impl_->active.erase(active);
    ++impl_->totalCompletions;
  }
  std::sort(impl_->completed.begin(), impl_->completed.end(), issueEntryLess);

  for (auto &[blockId, entries] : impl_->rob) {
    while (!entries.empty() && entries.front().completed) {
      NpuInstruction instruction = std::move(entries.front().instruction);
      entries.erase(entries.begin());
      instruction.timestamps.retired = epoch.time;
      updateDigest(impl_->result.digest, instruction);
      ++impl_->result.retiredInstructions;
      impl_->result.retiredSequenceIds.push_back(instruction.sequenceId);
      emitObservation({.category = "instruction",
                       .name = "retire",
                       .phase = TraceEventPhase::Instant,
                       .rootSequenceId = instruction.sequenceId});
      impl_->retired.push_back(std::move(instruction));
    }
  }
  std::sort(impl_->retired.begin(), impl_->retired.end(),
            [](const NpuInstruction &left, const NpuInstruction &right) {
              return std::tie(left.blockId, left.sequenceId) <
                     std::tie(right.blockId, right.sequenceId);
            });

  impl_->traceExhausted =
      impl_->traceExhausted || impl_->traceExhaustedProposal;
  impl_->totalUnitStalls += impl_->unitStallProposals;
  for (size_t index = 0; index < impl_->unitHighWatermarks.size(); ++index) {
    NpuEngineClass engine = static_cast<NpuEngineClass>(index);
    impl_->unitHighWatermarks[index] =
        std::max(impl_->unitHighWatermarks[index], activeExecutions(engine));
  }
  impl_->admissionProposals.clear();
  impl_->executionProposals.clear();
  impl_->deliveryProposals.clear();
  impl_->completionProposals.clear();
  impl_->traceExhaustedProposal = false;
  impl_->unitStallProposals = 0;
  if (changed)
    impl_->lastUpdate = epoch;
}

bool NpuExecutionPipeline::hasPendingCommit() const {
  return !impl_->admissionProposals.empty() ||
         !impl_->executionProposals.empty() ||
         !impl_->deliveryProposals.empty() ||
         !impl_->completionProposals.empty() || impl_->traceExhaustedProposal ||
         impl_->unitStallProposals != 0 ||
         impl_->memoryProtocol.hasPendingCommit() ||
         impl_->memoryController.hasPendingCommit();
}

bool NpuExecutionPipeline::isRunnable(Epoch epoch) const {
  if (hasPendingCommit())
    return true;
  return std::ranges::any_of(impl_->active, [&](const auto &active) {
    return active.second.readyEpoch == epoch;
  });
}

RuntimeObjectState NpuExecutionPipeline::runtimeState(Epoch epoch) const {
  RuntimeObjectState state = SimObject::runtimeState(epoch);
  for (const auto &[blockId, entries] : impl_->rob)
    state.queueOccupancy += entries.size();
  state.activeReservations = impl_->active.size();
  state.pendingOffers = impl_->admissionProposals.size() +
                        impl_->executionProposals.size() +
                        impl_->completionProposals.size();
  state.quiescent =
      complete() ||
      (state.queueOccupancy == 0 && state.activeReservations == 0 &&
       state.pendingOffers == 0 && !hasPendingCommit());
  if (!state.quiescent)
    state.reason = state.activeReservations ? "npu_execution_active"
                                            : "npu_retirement_pending";
  return state;
}

void NpuExecutionPipeline::collectStatistics(
    std::vector<StatSnapshot> &out) const {
  auto append = [&](std::string name, StatisticKind kind, uint64_t value) {
    out.push_back({.name = std::move(name),
                   .objectPath = std::string(path()),
                   .kind = kind,
                   .value = value,
                   .lastUpdate = impl_->lastUpdate});
  };
  append("active_executions", StatisticKind::Gauge, impl_->active.size());
  append("executed_instructions", StatisticKind::Counter,
         impl_->totalExecutions);
  append("completed_instructions", StatisticKind::Counter,
         impl_->totalCompletions);
  append("retired_instructions", StatisticKind::Counter,
         impl_->result.retiredInstructions);
  append("execution_unit_stalls", StatisticKind::Counter,
         impl_->totalUnitStalls);
  append("memory_requests", StatisticKind::Counter, impl_->totalMemoryRequests);
  append("scratchpad_tiles", StatisticKind::Gauge, impl_->scratchpad.size());
}

void NpuExecutionPipeline::bindSystem(SimSystem *system) {
  impl_->system = system;
}

Epoch NpuExecutionPipeline::completionEpoch(uint64_t sequenceId) const {
  auto active = impl_->active.find(sequenceId);
  return active == impl_->active.end() ? Epoch{} : active->second.readyEpoch;
}

bool NpuExecutionPipeline::canAccept(NpuEngineClass engine) const {
  size_t occupied = activeExecutions(engine);
  occupied += std::ranges::count_if(
      impl_->executionProposals, [&](const Impl::ActiveExecution &proposal) {
        return proposal.entry.instruction.engine == engine;
      });
  if (occupied >= configuredUnits(impl_->config, engine))
    return false;
  if (engine != NpuEngineClass::Tma)
    return true;
  size_t memoryOccupied =
      std::ranges::count_if(impl_->active, [](const auto &active) {
        return active.second.memoryRequest.has_value();
      });
  memoryOccupied += std::ranges::count_if(
      impl_->executionProposals, [](const Impl::ActiveExecution &proposal) {
        return proposal.memoryRequest.has_value();
      });
  return memoryOccupied < impl_->config.memoryRequests;
}

size_t NpuExecutionPipeline::activeExecutions(NpuEngineClass engine) const {
  return std::ranges::count_if(impl_->active, [&](const auto &active) {
    return active.second.entry.instruction.engine == engine;
  });
}

const std::vector<NpuIssueEntry> &NpuExecutionPipeline::completed() const {
  return impl_->completed;
}

const std::vector<NpuInstruction> &NpuExecutionPipeline::retired() const {
  return impl_->retired;
}

const std::vector<NpuMemoryRequest> &
NpuExecutionPipeline::memoryRequests() const {
  return impl_->memoryRequests;
}

const std::vector<NpuMemoryResponse> &
NpuExecutionPipeline::memoryResponses() const {
  return impl_->memoryResponses;
}

bool NpuExecutionPipeline::scratchpadContains(
    std::string_view tileIdentity) const {
  return impl_->scratchpad.contains(std::string(tileIdentity));
}

std::optional<uint64_t>
NpuExecutionPipeline::globalMemoryValue(uint64_t address) const {
  auto value = impl_->globalMemory.find(address);
  return value == impl_->globalMemory.end()
             ? std::nullopt
             : std::optional<uint64_t>(value->second);
}

const NpuArchitecturalResult &
NpuExecutionPipeline::architecturalResult() const {
  return impl_->result;
}

bool NpuExecutionPipeline::complete() const {
  if (!impl_->traceExhausted || !impl_->active.empty() ||
      !impl_->admissionProposals.empty() ||
      !impl_->executionProposals.empty() || hasPendingCommit())
    return false;
  for (const auto &[blockId, entries] : impl_->rob)
    if (!entries.empty())
      return false;
  return impl_->memoryProtocol.runtimeState({}).quiescent &&
         impl_->memoryController.runtimeState({}).quiescent;
}

void NpuExecutionPipeline::reset() {
  NpuExecutionConfig config = impl_->config;
  SimSystem *system = impl_->system;
  impl_ = std::make_unique<Impl>(config);
  impl_->system = system;
  clearRuntimeFailureCode();
}

NpuTraceSource::NpuTraceSource(std::string name, ObjectId id, SimObject *parent,
                               ObservationSink *observations)
    : SimObject(ObjectKind::TraceSource, std::move(name), id, parent,
                observations) {}

bool NpuTraceSource::loadDocument(PtoTraceDocument document) {
  if (loaded_ || pending_ || committed_)
    return false;
  document_ = std::move(document);
  loaded_ = true;
  return true;
}

void NpuTraceSource::doWork(Epoch) {
  if (pending_ || committed_)
    return;
  if (!loaded_) {
    setRuntimeFailureCode("npu_trace_not_loaded");
    return;
  }

  struct LocalSink final : ObservationSink {
    ObservationRecorder recorder;
    bool proposeObservation(EventProposal proposal) override {
      return recorder.propose(std::move(proposal));
    }
  } sink;

  const size_t queueCapacity = std::max<size_t>(document_.records.size(), 1);
  NpuDependencyTracker tracker(
      "dependencies", 10, nullptr,
      {queueCapacity, queueCapacity, queueCapacity, queueCapacity}, &sink);
  NpuExecutionPipeline execution(
      "execution", 11, nullptr,
      {.scalarUnits = 1,
       .vectorUnits = 2,
       .cubeUnits = 1,
       .tmaUnits = 1,
       .memoryRequests = 2,
       .scratchpadTiles = std::max<size_t>(document_.records.size() * 2, 8)},
      nullptr, &sink);

  auto commitTracker = [&](Epoch epoch) {
    tracker.doArbitrate(epoch);
    tracker.doXfer(epoch);
    return sink.recorder.commitOwner(tracker.id(), epoch);
  };
  auto commitExecution = [&](Epoch epoch) {
    execution.doArbitrate(epoch);
    execution.doXfer(epoch);
    return sink.recorder.commitOwner(execution.id(), epoch);
  };

  NpuDecoder decoder;
  uint64_t tick = 0;
  for (const PtoTraceRecord &record : document_.records) {
    NpuDecodeResult decoded = decoder.decode(record);
    if (!decoded.succeeded()) {
      setRuntimeFailureCode("npu_decode_failed");
      return;
    }
    NpuInstruction instruction = std::move(*decoded.instruction);
    ObjectId stableObjectId =
        static_cast<ObjectId>(20 + static_cast<uint8_t>(instruction.engine));
    if (!tracker.proposeDispatch(instruction, stableObjectId) ||
        !execution.proposeAdmit(instruction) || !commitTracker({tick, 0}) ||
        !tracker.dispatchAccepted(record.sequenceId) ||
        !commitExecution({tick, 0})) {
      setRuntimeFailureCode("npu_dispatch_failed");
      return;
    }
    if (!tracker.runtimeFailureCode().empty() ||
        !execution.runtimeFailureCode().empty()) {
      setRuntimeFailureCode("npu_dispatch_runtime_failed");
      return;
    }
    ++tick;
  }

  if (!execution.proposeTraceExhausted()) {
    setRuntimeFailureCode("npu_trace_exhaustion_failed");
    return;
  }

  std::vector<NpuIssueEntry> pendingIssues;
  constexpr std::array engines = {NpuEngineClass::Scalar,
                                  NpuEngineClass::Vector, NpuEngineClass::Cube,
                                  NpuEngineClass::Tma};
  const uint64_t maximumTicks =
      std::max<uint64_t>(128, document_.records.size() * 32);
  bool finished = false;
  for (uint64_t iteration = 0; iteration < maximumTicks; ++iteration, ++tick) {
    Epoch epoch{tick, 0};
    for (const NpuIssueEntry &issue : pendingIssues)
      if (!execution.proposeExecute(issue, epoch)) {
        setRuntimeFailureCode("npu_execution_admission_failed");
        return;
      }
    pendingIssues.clear();

    execution.doWork(epoch);
    if (!commitExecution(epoch)) {
      setRuntimeFailureCode("npu_execution_observation_failed");
      return;
    }
    if (!execution.runtimeFailureCode().empty()) {
      setRuntimeFailureCode("npu_execution_runtime_failed");
      return;
    }
    for (const NpuIssueEntry &completed : execution.completed())
      if (!tracker.proposeComplete(completed.instruction.sequenceId)) {
        setRuntimeFailureCode("npu_completion_broadcast_failed");
        return;
      }
    for (NpuEngineClass engine : engines)
      if (execution.canAccept(engine))
        tracker.proposeIssue(engine);
    if (!commitTracker(epoch)) {
      setRuntimeFailureCode("npu_dependency_observation_failed");
      return;
    }
    if (!tracker.runtimeFailureCode().empty()) {
      setRuntimeFailureCode("npu_dependency_runtime_failed");
      return;
    }
    pendingIssues.assign(tracker.issued().begin(), tracker.issued().end());

    if (execution.complete() && pendingIssues.empty() &&
        tracker.runtimeState(epoch).quiescent) {
      finished = true;
      break;
    }
  }
  if (!finished) {
    setRuntimeFailureCode("npu_execution_limit_reached");
    return;
  }

  result_ = execution.architecturalResult();
  eventCount_ = sink.recorder.events().size();
  for (const CommittedEvent &event : sink.recorder.events()) {
    std::vector<ObservationArgument> arguments = event.arguments;
    arguments.push_back(
        {.name = "npu_epoch_delta", .value = uint64_t{event.epoch.delta}});
    arguments.push_back({.name = "npu_epoch_time", .value = event.epoch.time});
    std::sort(
        arguments.begin(), arguments.end(),
        [](const ObservationArgument &left, const ObservationArgument &right) {
          return left.name < right.name;
        });
    if (!emitObservation({.category = event.category,
                          .name = event.name,
                          .phase = event.phase,
                          .rootSequenceId = event.rootSequenceId,
                          .duration = event.duration,
                          .flowId = event.flowId,
                          .arguments = std::move(arguments)}))
      return;
  }
  pending_ = true;
}

void NpuTraceSource::doXfer(Epoch epoch) {
  if (!pending_)
    return;
  pending_ = false;
  committed_ = true;
  lastUpdate_ = epoch;
}

bool NpuTraceSource::hasPendingCommit() const { return pending_; }

RuntimeObjectState NpuTraceSource::runtimeState(Epoch) const {
  return {.quiescent = committed_ && !pending_,
          .runnable = loaded_ && !committed_ && !pending_,
          .pendingCommit = pending_,
          .reason = committed_ ? "" : "npu_trace_pending",
          .traceOwner = true,
          .tracePosition = committed_ ? document_.records.size() : 0,
          .traceLastCommittedSequenceId =
              committed_ && !document_.records.empty()
                  ? std::optional<uint64_t>(document_.records.back().sequenceId)
                  : std::nullopt,
          .traceEof = committed_};
}

void NpuTraceSource::collectStatistics(std::vector<StatSnapshot> &out) const {
  if (!committed_)
    return;
  auto append = [&](std::string name, uint64_t value) {
    out.push_back({.name = std::move(name),
                   .objectPath = std::string(path()),
                   .kind = StatisticKind::Counter,
                   .value = value,
                   .lastUpdate = lastUpdate_});
  };
  append("trace_records", document_.records.size());
  append("npu_events", eventCount_);
  append("architectural_retired_instructions", result_.retiredInstructions);
  append("architectural_digest", result_.digest);
}

void NpuTraceSource::reset() {
  document_ = {};
  result_ = {};
  eventCount_ = 0;
  loaded_ = false;
  pending_ = false;
  committed_ = false;
  lastUpdate_ = {};
  clearRuntimeFailureCode();
}

bool NpuTraceSource::validate() const { return true; }

} // namespace gfsim
