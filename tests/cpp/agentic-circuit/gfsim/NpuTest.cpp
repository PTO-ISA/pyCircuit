#include "gfsim/npu.h"
#include "gfsim/tooling/trace_io.h"

#include "gtest/gtest.h"

#include <algorithm>
#include <array>
#include <fstream>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>

namespace gfsim {
namespace {

static_assert(Component<NpuScheduleV2>);

PtoValue value(std::string text) { return {.value = std::move(text)}; }

PtoValue value(uint64_t integer) { return {.value = integer}; }

PtoValue array(PtoValue::Array values) { return {.value = std::move(values)}; }

PtoValue object(PtoValue::Object members) {
  return {.value = std::move(members)};
}

PtoValue tile(std::string address) {
  return object({{"address", value(std::move(address))},
                 {"dtype", value("float32")},
                 {"layout", value("ND")},
                 {"shape", array({value(2), value(4)})}});
}

PtoValue scalar(std::string type, std::string scalarValue) {
  return object({{"dtype", value(std::move(type))},
                 {"value", value(std::move(scalarValue))}});
}

PtoTraceOperand tileOperand(std::string id) {
  return {.kind = PtoOperandKind::Tile, .id = std::move(id)};
}

PtoTraceOperand scalarOperand(std::string type, std::string scalarValue) {
  return {.kind = PtoOperandKind::Immediate,
          .type = std::move(type),
          .immediate = value(std::move(scalarValue))};
}

PtoTraceRecord record(std::string opcode, uint64_t sequenceId, uint64_t blockId,
                      PtoValue::Array inputTiles, PtoValue::Array scalarInputs,
                      PtoValue::Array outputTiles,
                      std::vector<std::string> roles,
                      std::vector<PtoTraceOperand> operands) {
  PtoValue::Array roleValues;
  for (std::string &role : roles)
    roleValues.push_back(value(std::move(role)));
  PtoTraceRecord result;
  result.sequenceId = sequenceId;
  result.opcode = std::move(opcode);
  result.operands = std::move(operands);
  result.attributes.emplace(
      "davincioo", object({{"block_idx", value(blockId)},
                           {"input_tiles", array(std::move(inputTiles))},
                           {"operand_roles", array(std::move(roleValues))},
                           {"output_tiles", array(std::move(outputTiles))},
                           {"scalar_inputs", array(std::move(scalarInputs))}}));
  return result;
}

PtoTraceRecord representative(std::string opcode) {
  if (opcode == "TASSIGN")
    return record(
        std::move(opcode), 7, 3, {}, {scalar("uint64", "64")}, {tile("0x40")},
        {"scalar_input", "output_tile"},
        {scalarOperand("uint64", "64"), tileOperand("block/3/tile/0x40")});
  if (opcode == "TLOAD")
    return record(
        std::move(opcode), 7, 3, {tile("0x20")}, {}, {tile("0x40")},
        {"input_tile", "output_tile"},
        {tileOperand("block/3/tile/0x20"), tileOperand("block/3/tile/0x40")});
  if (opcode == "TMATMUL")
    return record(std::move(opcode), 7, 3, {tile("0x10"), tile("0x20")}, {},
                  {tile("0x40")}, {"input_tile", "input_tile", "output_tile"},
                  {tileOperand("block/3/tile/0x10"),
                   tileOperand("block/3/tile/0x20"),
                   tileOperand("block/3/tile/0x40")});
  return record(
      std::move(opcode), 7, 3, {tile("0x20")}, {scalar("float32", "1.25")},
      {tile("0x40")}, {"input_tile", "scalar_input", "output_tile"},
      {tileOperand("block/3/tile/0x20"), scalarOperand("float32", "1.25"),
       tileOperand("block/3/tile/0x40")});
}

NpuInstruction tileInstruction(std::string opcode, uint64_t sequenceId,
                               uint64_t blockId,
                               std::vector<std::string> inputAddresses,
                               std::vector<std::string> outputAddresses) {
  PtoValue::Array inputs;
  PtoValue::Array outputs;
  std::vector<std::string> roles;
  std::vector<PtoTraceOperand> operands;
  for (std::string &address : inputAddresses) {
    inputs.push_back(tile(address));
    roles.push_back("input_tile");
    operands.push_back(
        tileOperand("block/" + std::to_string(blockId) + "/tile/" + address));
  }
  for (std::string &address : outputAddresses) {
    outputs.push_back(tile(address));
    roles.push_back("output_tile");
    operands.push_back(
        tileOperand("block/" + std::to_string(blockId) + "/tile/" + address));
  }
  NpuDecodeResult decoded = NpuDecoder{}.decode(
      record(std::move(opcode), sequenceId, blockId, std::move(inputs), {},
             std::move(outputs), std::move(roles), std::move(operands)));
  EXPECT_TRUE(decoded.succeeded());
  return std::move(*decoded.instruction);
}

void commit(NpuDependencyTracker &tracker, Epoch epoch) {
  tracker.doArbitrate(epoch);
  tracker.doXfer(epoch);
}

void commit(NpuExecutionPipeline &pipeline, Epoch epoch) {
  pipeline.doArbitrate(epoch);
  pipeline.doXfer(epoch);
}

NpuIssueEntry issueEntry(NpuInstruction instruction, ObjectId objectId) {
  return {.instruction = std::move(instruction), .stableObjectId = objectId};
}

NpuCompletion completionFor(const NpuIssueEntry &entry) {
  NpuCompletion completion{.sequenceId = entry.instruction.sequenceId};
  for (const NpuOutputRename &rename : entry.outputRenames)
    completion.outputTags.push_back(rename.output);
  return completion;
}

struct RecorderSink final : ObservationSink {
  ObservationRecorder recorder;
  bool proposeObservation(EventProposal proposal) override {
    return recorder.propose(std::move(proposal));
  }
};

struct RejectingSink final : ObservationSink {
  bool proposeObservation(EventProposal) override { return false; }
};

TEST(NpuDecoderTest, ClassifiesRepresentativePinnedDavinciOOOpcodes) {
  NpuDecoder decoder;
  const std::array cases = {
      std::pair{"TASSIGN", NpuEngineClass::Scalar},
      std::pair{"TADDS", NpuEngineClass::Vector},
      std::pair{"TMATMUL", NpuEngineClass::Cube},
      std::pair{"TLOAD", NpuEngineClass::Tma},
  };

  for (const auto &[opcode, engine] : cases) {
    SCOPED_TRACE(opcode);
    NpuDecodeResult decoded = decoder.decode(representative(opcode));
    ASSERT_TRUE(decoded.succeeded());
    ASSERT_TRUE(decoded.instruction);
    EXPECT_EQ(decoded.instruction->opcode, opcode);
    EXPECT_EQ(decoded.instruction->engine, engine);
  }
}

TEST(NpuDecoderTest, PreservesOrderedOperandsTilesScalarsAndDependencies) {
  NpuDecoder decoder;
  PtoTraceRecord source = representative("TADDS");
  source.dependencies = {1, 5};

  NpuDecodeResult decoded = decoder.decode(source);
  ASSERT_TRUE(decoded.succeeded());
  const NpuInstruction &instruction = *decoded.instruction;
  EXPECT_EQ(instruction.sequenceId, 7u);
  EXPECT_EQ(instruction.blockId, 3u);
  EXPECT_EQ(instruction.dependencies, (std::vector<uint64_t>{1, 5}));
  EXPECT_EQ(instruction.operands, source.operands);
  EXPECT_EQ(instruction.inputTiles,
            (std::vector<std::string>{"block/3/tile/0x20"}));
  EXPECT_EQ(instruction.outputTiles,
            (std::vector<std::string>{"block/3/tile/0x40"}));
  ASSERT_EQ(instruction.scalarInputs.size(), 1u);
  EXPECT_EQ(instruction.scalarInputs[0].type, "float32");
  EXPECT_EQ(instruction.scalarInputs[0].value, value("1.25"));
  EXPECT_EQ(instruction.timestamps, NpuTimestamps{});
}

TEST(NpuDecoderTest, ProducesDecodeAndDispatchProposalsWithRootIdentity) {
  NpuDecodeResult decoded = NpuDecoder{}.decode(representative("TMATMUL"));
  ASSERT_TRUE(decoded.succeeded());
  ASSERT_EQ(decoded.observations.size(), 2u);
  EXPECT_EQ(decoded.observations[0].category, "instruction");
  EXPECT_EQ(decoded.observations[0].name, "decode");
  EXPECT_EQ(decoded.observations[1].category, "instruction");
  EXPECT_EQ(decoded.observations[1].name, "dispatch");
  for (const EventProposal &proposal : decoded.observations) {
    EXPECT_EQ(proposal.rootSequenceId, 7u);
    EXPECT_EQ(proposal.arguments,
              (std::vector<ObservationArgument>{
                  {.name = "block_id", .value = uint64_t{3}},
                  {.name = "engine", .value = std::string("cube")},
                  {.name = "opcode", .value = std::string("TMATMUL")}}));
  }
}

TEST(NpuDecoderTest, RejectsUnsupportedAndMalformedImportedRecords) {
  NpuDecoder decoder;

  PtoTraceRecord unsupported = representative("TADDS");
  unsupported.opcode = "TUNSUPPORTED";
  EXPECT_EQ(decoder.decode(unsupported).primaryDiagnostic(),
            "npu_unsupported_opcode");

  PtoTraceRecord missing = representative("TADDS");
  missing.attributes.clear();
  EXPECT_EQ(decoder.decode(missing).primaryDiagnostic(),
            "npu_missing_davincioo_attributes");

  PtoTraceRecord unknownField = representative("TADDS");
  auto &attributes =
      std::get<PtoValue::Object>(unknownField.attributes.at("davincioo").value);
  attributes.emplace("unexpected", value(1));
  EXPECT_EQ(decoder.decode(unknownField).primaryDiagnostic(),
            "npu_invalid_davincioo_attributes");

  PtoTraceRecord wrongRoleCount = representative("TADDS");
  auto &roleArray = std::get<PtoValue::Array>(
      std::get<PtoValue::Object>(
          wrongRoleCount.attributes.at("davincioo").value)
          .at("operand_roles")
          .value);
  roleArray.pop_back();
  EXPECT_EQ(decoder.decode(wrongRoleCount).primaryDiagnostic(),
            "npu_operand_role_mismatch");

  PtoTraceRecord wrongTile = representative("TADDS");
  wrongTile.operands.front().id = "block/3/tile/0x21";
  EXPECT_EQ(decoder.decode(wrongTile).primaryDiagnostic(),
            "npu_tile_identity_mismatch");

  PtoTraceRecord wrongScalar = representative("TADDS");
  wrongScalar.operands[1].type = "float16";
  EXPECT_EQ(decoder.decode(wrongScalar).primaryDiagnostic(),
            "npu_scalar_mismatch");
}

TEST(NpuDecoderTest, RepeatedDecodeIsByteIndependentAndValueIdentical) {
  NpuDecoder decoder;
  const PtoTraceRecord source = representative("TADDS");
  const NpuDecodeResult first = decoder.decode(source);
  const NpuDecodeResult second = decoder.decode(source);
  ASSERT_TRUE(first.succeeded());
  ASSERT_TRUE(second.succeeded());
  EXPECT_EQ(first.instruction, second.instruction);
  EXPECT_EQ(first.observations, second.observations);
  EXPECT_EQ(source, representative("TADDS"));
}

TEST(NpuDecoderTest, UnsupportedOpcodeNeverCommitsATraceOffer) {
  PtoTraceDocument document;
  document.records.push_back(representative("TADDS"));
  document.records.front().opcode = "TUNSUPPORTED";
  TraceSource<NpuInstruction, NpuDecoder> source("trace", 1, nullptr,
                                                 std::move(document));

  source.doWork({0, 0});
  source.doXfer({0, 0});

  EXPECT_FALSE(source.hasOffer());
  EXPECT_EQ(source.position().nextRecordIndex, 0u);
  EXPECT_EQ(source.runtimeFailureCode(), "trace_decode_failed");
}

TEST(NpuDependencyTrackerTest, RejectsInvalidTagCapacityBeforeAllocation) {
  EXPECT_THROW((NpuDependencyTracker("zero", 40, nullptr, {1, 1, 1, 1},
                                     size_t{0})),
               std::invalid_argument);
  if constexpr (std::numeric_limits<size_t>::max() >
                std::numeric_limits<uint32_t>::max()) {
    const size_t aboveTagRange =
        static_cast<size_t>(std::numeric_limits<uint32_t>::max()) + 1;
    EXPECT_THROW((NpuDependencyTracker("above-u32", 40, nullptr,
                                       {1, 1, 1, 1}, aboveTagRange)),
                 std::invalid_argument);
    EXPECT_THROW((NpuDependencyTracker(
                     "size-max", 40, nullptr, {1, 1, 1, 1},
                     std::numeric_limits<size_t>::max())),
                 std::invalid_argument);
  }
}

TEST(NpuDependencyTrackerTest, RawDependencyWakesOnlyAtCompletionXfer) {
  RecorderSink sink;
  NpuDependencyTracker tracker("dependencies", 40, nullptr, {2, 2, 2, 2},
                               &sink);
  EXPECT_EQ(tracker.physicalTagCapacity(), 256u);
  const NpuInstruction producer = tileInstruction("TADD", 10, 0, {}, {"0x10"});
  NpuInstruction consumer = tileInstruction("TADD", 11, 0, {"0x10"}, {"0x20"});
  consumer.dependencies = {4, 9};

  ASSERT_TRUE(tracker.proposeDispatch(producer, 7));
  commit(tracker, {0, 0});
  ASSERT_TRUE(sink.recorder.commitOwner(40, {0, 0}));
  ASSERT_TRUE(tracker.proposeDispatch(consumer, 8));
  commit(tracker, {1, 0});
  ASSERT_TRUE(sink.recorder.commitOwner(40, {1, 0}));

  ASSERT_EQ(tracker.dependencies(11).size(), 1u);
  EXPECT_EQ(tracker.dependencies(11).front().producerSequenceId, 10u);
  ASSERT_EQ(tracker.queued(NpuEngineClass::Vector).size(), 2u);
  EXPECT_EQ(
      tracker.queued(NpuEngineClass::Vector).back().instruction.dependencies,
      (std::vector<uint64_t>{4, 9}));
  EXPECT_FALSE(tracker.isReady(11));
  ASSERT_TRUE(tracker.proposeIssue(NpuEngineClass::Vector));
  EXPECT_EQ(
      tracker.proposedIssue(NpuEngineClass::Vector)->instruction.sequenceId,
      10u);
  commit(tracker, {2, 0});
  ASSERT_TRUE(sink.recorder.commitOwner(40, {2, 0}));

  ASSERT_TRUE(tracker.proposeComplete(10));
  EXPECT_FALSE(tracker.isReady(11));
  commit(tracker, {3, 0});
  ASSERT_TRUE(sink.recorder.commitOwner(40, {3, 0}));
  EXPECT_TRUE(tracker.isReady(11));
  ASSERT_TRUE(tracker.proposeIssue(NpuEngineClass::Vector));
  EXPECT_EQ(
      tracker.proposedIssue(NpuEngineClass::Vector)->instruction.sequenceId,
      11u);
  commit(tracker, {4, 0});
  ASSERT_TRUE(sink.recorder.commitOwner(40, {4, 0}));

  std::vector<TraceEventPhase> dependencyPhases;
  for (const CommittedEvent &event : sink.recorder.events())
    if (event.category == "dependency" && event.name == "tile") {
      dependencyPhases.push_back(event.phase);
      ASSERT_TRUE(event.flowId);
      EXPECT_LE(*event.flowId, 9007199254740991ULL);
    }
  EXPECT_EQ(dependencyPhases,
            (std::vector<TraceEventPhase>{TraceEventPhase::FlowStart,
                                          TraceEventPhase::FlowEnd}));

  std::vector<StatSnapshot> statistics;
  tracker.collectStatistics(statistics);
  auto valueOf = [&](std::string_view name) {
    auto position = std::ranges::find(statistics, name, &StatSnapshot::name);
    EXPECT_NE(position, statistics.end());
    return position == statistics.end() ? uint64_t{0} : position->value;
  };
  EXPECT_EQ(valueOf("issued_instructions"), 2u);
  EXPECT_EQ(valueOf("dependency_wakeups"), 1u);
}

TEST(NpuDependencyTrackerTest, FourFiniteEngineQueuesIssueIndependently) {
  NpuDependencyTracker tracker("dependencies", 40, nullptr, {1, 1, 1, 1});
  const std::array instructions = {
      tileInstruction("TASSIGN", 1, 0, {}, {"0x10"}),
      tileInstruction("TADD", 2, 0, {}, {"0x20"}),
      tileInstruction("TMATMUL", 3, 0, {}, {"0x30"}),
      tileInstruction("TLOAD", 4, 0, {}, {"0x40"}),
  };
  for (size_t index = 0; index < instructions.size(); ++index)
    ASSERT_TRUE(tracker.proposeDispatch(instructions[index],
                                        static_cast<ObjectId>(10 + index)));
  commit(tracker, {0, 0});

  EXPECT_EQ(tracker.queueSize(NpuEngineClass::Scalar), 1u);
  EXPECT_EQ(tracker.queueSize(NpuEngineClass::Vector), 1u);
  EXPECT_EQ(tracker.queueSize(NpuEngineClass::Cube), 1u);
  EXPECT_EQ(tracker.queueSize(NpuEngineClass::Tma), 1u);
  EXPECT_TRUE(tracker.proposeIssue(NpuEngineClass::Tma));
  EXPECT_TRUE(tracker.proposeIssue(NpuEngineClass::Cube));
  EXPECT_TRUE(tracker.proposeIssue(NpuEngineClass::Vector));
  EXPECT_TRUE(tracker.proposeIssue(NpuEngineClass::Scalar));
  commit(tracker, {1, 0});

  ASSERT_EQ(tracker.issued().size(), 4u);
  for (size_t index = 0; index < tracker.issued().size(); ++index) {
    EXPECT_EQ(tracker.issued()[index].instruction.sequenceId, index + 1);
    EXPECT_EQ(tracker.issued()[index].instruction.timestamps.issued, 1u);
  }
}

TEST(NpuDependencyTrackerTest, OldestReadySelectionIgnoresInsertionOrder) {
  auto run = [](std::array<size_t, 3> order) {
    NpuDependencyTracker tracker("dependencies", 40, nullptr, {3, 3, 3, 3});
    const std::array instructions = {
        tileInstruction("TADD", 30, 0, {}, {"0x30"}),
        tileInstruction("TADD", 10, 0, {}, {"0x10"}),
        tileInstruction("TADD", 20, 0, {}, {"0x20"}),
    };
    for (size_t index : order)
      EXPECT_TRUE(tracker.proposeDispatch(instructions[index],
                                          static_cast<ObjectId>(9 - index)));
    commit(tracker, {0, 0});
    EXPECT_TRUE(tracker.proposeIssue(NpuEngineClass::Vector));
    return tracker.proposedIssue(NpuEngineClass::Vector)
        ->instruction.sequenceId;
  };

  EXPECT_EQ(run({0, 1, 2}), 10u);
  EXPECT_EQ(run({2, 0, 1}), 10u);
}

TEST(NpuDependencyTrackerTest, RenameUsesLatestProducerAndIsBlockLocal) {
  NpuDependencyTracker tracker("dependencies", 40, nullptr, {8, 8, 8, 8});
  const NpuInstruction first = tileInstruction("TADD", 1, 0, {}, {"0x10"});
  const NpuInstruction overwrite = tileInstruction("TADD", 2, 0, {}, {"0x10"});
  const NpuInstruction sameBlock =
      tileInstruction("TADD", 3, 0, {"0x10"}, {"0x20"});
  const NpuInstruction otherBlock =
      tileInstruction("TADD", 4, 1, {"0x10"}, {"0x20"});

  ASSERT_TRUE(tracker.proposeDispatch(first, 1));
  ASSERT_TRUE(tracker.proposeDispatch(overwrite, 2));
  commit(tracker, {0, 0});
  ASSERT_TRUE(tracker.proposeDispatch(sameBlock, 3));
  ASSERT_TRUE(tracker.proposeDispatch(otherBlock, 4));
  commit(tracker, {1, 0});

  ASSERT_EQ(tracker.dependencies(3).size(), 1u);
  EXPECT_EQ(tracker.dependencies(3).front().producerSequenceId, 2u);
  EXPECT_TRUE(tracker.dependencies(4).empty());
  EXPECT_TRUE(tracker.isReady(4));
}

TEST(NpuDependencyTrackerTest, SameArbitrateRenameUsesLatestProducer) {
  NpuDependencyTracker tracker("dependencies", 40, nullptr, {8, 8, 8, 8});
  const NpuInstruction first = tileInstruction("TADD", 1, 0, {}, {"0x10"});
  const NpuInstruction overwrite = tileInstruction("TADD", 2, 0, {}, {"0x10"});
  const NpuInstruction consumer =
      tileInstruction("TADD", 3, 0, {"0x10"}, {"0x20"});
  ASSERT_TRUE(tracker.proposeDispatch(first, 1));
  ASSERT_TRUE(tracker.proposeDispatch(overwrite, 2));
  ASSERT_TRUE(tracker.proposeDispatch(consumer, 3));
  commit(tracker, {0, 0});
  ASSERT_EQ(tracker.dependencies(3).size(), 1u);
  EXPECT_EQ(tracker.dependencies(3).front().producerSequenceId, 2u);
}

TEST(NpuDependencyTrackerTest, FiniteCapacityRejectsWithoutMutatingOffer) {
  RecorderSink sink;
  NpuDependencyTracker tracker("dependencies", 40, nullptr, {1, 1, 1, 1},
                               &sink);
  tracker.setPath("/npu/dependencies");
  const NpuInstruction first = tileInstruction("TADD", 1, 0, {}, {"0x10"});
  const NpuInstruction blocked = tileInstruction("TADD", 2, 0, {}, {"0x20"});
  // NOLINTNEXTLINE(performance-unnecessary-copy-initialization)
  const NpuInstruction original = blocked;

  ASSERT_TRUE(tracker.proposeDispatch(first, 1));
  commit(tracker, {0, 0});
  ASSERT_TRUE(sink.recorder.commitOwner(40, {0, 0}));
  ASSERT_TRUE(tracker.proposeDispatch(blocked, 2));
  commit(tracker, {1, 0});
  ASSERT_TRUE(sink.recorder.commitOwner(40, {1, 0}));

  EXPECT_EQ(blocked, original);
  EXPECT_EQ(tracker.queueSize(NpuEngineClass::Vector), 1u);
  EXPECT_EQ(tracker.rejectedDispatches(), (std::vector<uint64_t>{2}));
  EXPECT_FALSE(tracker.dispatchAccepted(2));
  ASSERT_FALSE(sink.recorder.events().empty());
  EXPECT_EQ(sink.recorder.events().back().category, "stall");
  EXPECT_EQ(sink.recorder.events().back().name, "issue_window_capacity");

  ASSERT_TRUE(tracker.proposeIssue(NpuEngineClass::Vector));
  ASSERT_TRUE(tracker.proposeDispatch(blocked, 2));
  commit(tracker, {2, 0});
  ASSERT_TRUE(sink.recorder.commitOwner(40, {2, 0}));
  EXPECT_FALSE(tracker.dispatchAccepted(2));
  ASSERT_EQ(tracker.issued().size(), 1u);
  const NpuIssueEntry issued = tracker.issued().front();
  ASSERT_TRUE(tracker.proposeComplete(completionFor(issued)));
  commit(tracker, {3, 0});
  ASSERT_TRUE(sink.recorder.commitOwner(40, {3, 0}));
  ASSERT_TRUE(tracker.proposeDispatch(blocked, 2));
  commit(tracker, {4, 0});
  ASSERT_TRUE(sink.recorder.commitOwner(40, {4, 0}));
  EXPECT_TRUE(tracker.dispatchAccepted(2));
  ASSERT_EQ(tracker.queued(NpuEngineClass::Vector).size(), 1u);
  EXPECT_EQ(
      tracker.queued(NpuEngineClass::Vector).front().instruction.sequenceId,
      2u);

  std::vector<StatSnapshot> statistics;
  tracker.collectStatistics(statistics);
  auto find = [&](std::string_view name) -> const StatSnapshot * {
    auto position = std::ranges::find(statistics, name, &StatSnapshot::name);
    return position == statistics.end() ? nullptr : &*position;
  };
  ASSERT_NE(find("dispatch_stalls"), nullptr);
  EXPECT_EQ(find("dispatch_stalls")->value, 2u);
  ASSERT_NE(find("issue_queue_occupancy_vector"), nullptr);
  EXPECT_EQ(find("issue_queue_occupancy_vector")->value, 1u);
}

TEST(NpuDependencyTrackerTest,
     WorkPermutationPreservesQueuesStatisticsAndObservations) {
  struct Snapshot {
    std::vector<NpuIssueEntry> queue;
    std::vector<std::pair<std::string, uint64_t>> statistics;
    std::vector<CommittedEvent> observations;
    bool operator==(const Snapshot &) const = default;
  };
  auto run = [](std::array<size_t, 3> order) {
    RecorderSink sink;
    NpuDependencyTracker tracker("dependencies", 40, nullptr, {1, 2, 1, 1},
                                 &sink);
    tracker.setPath("/npu/dependencies");
    const std::array instructions = {
        tileInstruction("TADD", 3, 0, {}, {"0x30"}),
        tileInstruction("TADD", 1, 0, {}, {"0x10"}),
        tileInstruction("TADD", 2, 0, {"0x10"}, {"0x20"}),
    };
    for (size_t index : order)
      EXPECT_TRUE(tracker.proposeDispatch(instructions[index],
                                          static_cast<ObjectId>(10 + index)));
    commit(tracker, {0, 0});
    EXPECT_TRUE(sink.recorder.commitOwner(40, {0, 0}));
    std::vector<StatSnapshot> statistics;
    tracker.collectStatistics(statistics);
    std::vector<std::pair<std::string, uint64_t>> statisticValues;
    for (const StatSnapshot &statistic : statistics)
      statisticValues.emplace_back(statistic.name, statistic.value);
    return Snapshot{tracker.queued(NpuEngineClass::Vector),
                    std::move(statisticValues),
                    std::vector<CommittedEvent>(sink.recorder.events().begin(),
                                                sink.recorder.events().end())};
  };

  EXPECT_EQ(run({0, 1, 2}), run({2, 0, 1}));
}

TEST(NpuDependencyTrackerTest,
     SameEpochRenameAllocatesTagsWithReadBeforeWriteOrdering) {
  NpuDependencyTracker tracker("dependencies", 40, nullptr, {8, 8, 8, 8}, 4);
  ASSERT_TRUE(
      tracker.proposeDispatch(tileInstruction("TADD", 1, 0, {}, {"0x10"}), 1));
  ASSERT_TRUE(tracker.proposeDispatch(
      tileInstruction("TADD", 2, 0, {"0x10"}, {"0x10"}), 2));
  ASSERT_TRUE(tracker.proposeDispatch(
      tileInstruction("TADD", 3, 0, {"0x10"}, {"0x20"}), 3));
  commit(tracker, {0, 0});

  const auto first = tracker.queued(NpuEngineClass::Vector);
  ASSERT_EQ(first.size(), 3u);
  ASSERT_EQ(first[0].outputRenames.size(), 1u);
  ASSERT_EQ(first[1].derivedDependencies.size(), 1u);
  ASSERT_EQ(first[1].outputRenames.size(), 1u);
  ASSERT_EQ(first[2].derivedDependencies.size(), 1u);
  EXPECT_EQ(first[0].outputRenames[0].output, (NpuPhysicalTag{0, 1}));
  EXPECT_EQ(first[1].derivedDependencies[0].tag, 0u);
  EXPECT_EQ(first[1].derivedDependencies[0].generation, 1u);
  EXPECT_EQ(first[1].outputRenames[0].output, (NpuPhysicalTag{1, 1}));
  EXPECT_EQ(first[1].outputRenames[0].replaced,
            (std::optional<NpuPhysicalTag>{{0, 1}}));
  EXPECT_EQ(first[2].derivedDependencies[0].tag, 1u);
  EXPECT_EQ(first[2].derivedDependencies[0].generation, 1u);
}

TEST(NpuDependencyTrackerTest,
     PhysicalTagExhaustionStallsInProgramOrderWithoutPartialCommit) {
  RecorderSink sink;
  NpuDependencyTracker tracker("dependencies", 40, nullptr, {8, 8, 8, 8}, 1,
                               &sink);
  ASSERT_TRUE(
      tracker.proposeDispatch(tileInstruction("TADD", 1, 0, {}, {"0x10"}), 1));
  ASSERT_TRUE(
      tracker.proposeDispatch(tileInstruction("TADD", 2, 0, {}, {"0x20"}), 2));
  ASSERT_TRUE(
      tracker.proposeDispatch(tileInstruction("TADD", 3, 0, {}, {}), 3));
  commit(tracker, {0, 0});

  ASSERT_EQ(tracker.queued(NpuEngineClass::Vector).size(), 1u);
  EXPECT_EQ(tracker.queued(NpuEngineClass::Vector)[0].instruction.sequenceId,
            1u);
  EXPECT_EQ(tracker.rejectedDispatches(), (std::vector<uint64_t>{2, 3}));
  EXPECT_EQ(tracker.freeTagCount(), 0u);
  EXPECT_FALSE(tracker.producerTag(0, "block/0/tile/0x20"));
  ASSERT_TRUE(sink.recorder.commitOwner(40, {0, 0}));
  EXPECT_TRUE(
      std::ranges::any_of(sink.recorder.events(), [](const auto &event) {
        return event.category == "stall" && event.name == "physical_tag_pool";
      }));
  EXPECT_TRUE(
      std::ranges::any_of(sink.recorder.events(), [](const auto &event) {
        return event.category == "rename" && event.name == "allocate";
      }));
  std::vector<StatSnapshot> statistics;
  tracker.collectStatistics(statistics);
  auto statistic = [&](std::string_view name) {
    return std::ranges::find(statistics, name, &StatSnapshot::name);
  };
  ASSERT_NE(statistic("physical_tags_free"), statistics.end());
  EXPECT_EQ(statistic("physical_tags_free")->value, 0u);
  ASSERT_NE(statistic("physical_tag_pool_stalls"), statistics.end());
  EXPECT_EQ(statistic("physical_tag_pool_stalls")->value, 2u);
}

TEST(NpuDependencyTrackerTest,
     MultipleDependenciesWakeIndependentlyAndRejectStaleCompletion) {
  NpuDependencyTracker tracker("dependencies", 40, nullptr, {8, 8, 8, 8}, 4);
  ASSERT_TRUE(
      tracker.proposeDispatch(tileInstruction("TADD", 1, 0, {}, {"0x10"}), 1));
  ASSERT_TRUE(
      tracker.proposeDispatch(tileInstruction("TADD", 2, 0, {}, {"0x20"}), 2));
  commit(tracker, {0, 0});
  ASSERT_TRUE(tracker.proposeDispatch(
      tileInstruction("TADD", 3, 0, {"0x10", "0x20"}, {"0x30"}), 3));
  commit(tracker, {1, 0});
  ASSERT_EQ(tracker.dependencies(3).size(), 2u);
  EXPECT_EQ(tracker.liveFlowCount(), 2u);
  EXPECT_FALSE(tracker.isReady(3));

  ASSERT_TRUE(tracker.proposeIssue(NpuEngineClass::Vector));
  commit(tracker, {2, 0});
  const NpuIssueEntry first = tracker.issued().front();
  ASSERT_TRUE(tracker.proposeIssue(NpuEngineClass::Vector));
  commit(tracker, {3, 0});
  const NpuIssueEntry second = tracker.issued().front();
  NpuCompletion stale = completionFor(first);
  ASSERT_FALSE(stale.outputTags.empty());
  ++stale.outputTags[0].generation;
  EXPECT_FALSE(tracker.proposeComplete(std::move(stale)));
  ASSERT_TRUE(tracker.proposeComplete(completionFor(first)));
  EXPECT_FALSE(tracker.proposeComplete(completionFor(first)));
  commit(tracker, {4, 0});
  EXPECT_FALSE(tracker.isReady(3));
  ASSERT_TRUE(tracker.proposeComplete(completionFor(second)));
  commit(tracker, {5, 0});
  EXPECT_TRUE(tracker.isReady(3));
  ASSERT_TRUE(tracker.proposeIssue(NpuEngineClass::Vector));
  commit(tracker, {6, 0});
  EXPECT_EQ(tracker.liveFlowCount(), 0u);
}

TEST(NpuDependencyTrackerTest, CompletionBeforeConsumerArrivalIsObservedReady) {
  NpuDependencyTracker tracker("dependencies", 40, nullptr, {4, 4, 4, 4}, 2);
  ASSERT_TRUE(
      tracker.proposeDispatch(tileInstruction("TADD", 1, 0, {}, {"0x10"}), 1));
  commit(tracker, {0, 0});
  ASSERT_TRUE(tracker.proposeIssue(NpuEngineClass::Vector));
  commit(tracker, {1, 0});
  const NpuIssueEntry producer = tracker.issued().front();
  ASSERT_TRUE(tracker.proposeComplete(completionFor(producer)));
  commit(tracker, {2, 0});
  ASSERT_TRUE(
      tracker.proposeDispatch(tileInstruction("TADD", 2, 0, {"0x10"}, {}), 2));
  commit(tracker, {3, 0});
  ASSERT_EQ(tracker.dependencies(2).size(), 1u);
  EXPECT_TRUE(tracker.isReady(2));
}

TEST(NpuDependencyTrackerTest,
     IssueWindowIncludesOutstandingUntilExactCompletion) {
  for (bool producesOutput : {false, true}) {
    SCOPED_TRACE(producesOutput);
    RecorderSink sink;
    NpuDependencyTracker tracker("dependencies", 40, nullptr, {2, 2, 2, 2}, 8,
                                 &sink);
    const std::vector<std::string> firstOutput =
        producesOutput ? std::vector<std::string>{"0x10"}
                       : std::vector<std::string>{};
    const std::vector<std::string> secondOutput =
        producesOutput ? std::vector<std::string>{"0x20"}
                       : std::vector<std::string>{};
    ASSERT_TRUE(tracker.proposeDispatch(
        tileInstruction("TADD", 1, 0, {}, firstOutput), 1));
    ASSERT_TRUE(tracker.proposeDispatch(
        tileInstruction("TADD", 2, 0, {}, secondOutput), 2));
    commit(tracker, {0, 0});
    ASSERT_TRUE(sink.recorder.commitOwner(40, {0, 0}));
    ASSERT_TRUE(tracker.proposeIssue(NpuEngineClass::Vector));
    commit(tracker, {1, 0});
    ASSERT_TRUE(sink.recorder.commitOwner(40, {1, 0}));
    const NpuIssueEntry first = tracker.issued().front();
    ASSERT_TRUE(tracker.proposeIssue(NpuEngineClass::Vector));
    commit(tracker, {2, 0});
    ASSERT_TRUE(sink.recorder.commitOwner(40, {2, 0}));
    EXPECT_EQ(tracker.queueSize(NpuEngineClass::Vector), 0u);
    EXPECT_EQ(tracker.outstandingProducerCount(), 2u);
    EXPECT_EQ(tracker.issueWindowOccupancy(NpuEngineClass::Vector), 2u);

    ASSERT_TRUE(
        tracker.proposeDispatch(tileInstruction("TADD", 3, 0, {}, {}), 3));
    commit(tracker, {3, 0});
    ASSERT_TRUE(sink.recorder.commitOwner(40, {3, 0}));
    EXPECT_FALSE(tracker.dispatchAccepted(3));
    EXPECT_EQ(tracker.issueWindowOccupancy(NpuEngineClass::Vector), 2u);
    EXPECT_TRUE(std::ranges::any_of(
        sink.recorder.events(), [](const CommittedEvent &event) {
          return event.category == "stall" &&
                 event.name == "issue_window_capacity";
        }));

    ASSERT_TRUE(tracker.proposeComplete(completionFor(first)));
    commit(tracker, {4, 0});
    ASSERT_TRUE(sink.recorder.commitOwner(40, {4, 0}));
    EXPECT_EQ(tracker.issueWindowOccupancy(NpuEngineClass::Vector), 1u);
    ASSERT_TRUE(
        tracker.proposeDispatch(tileInstruction("TADD", 3, 0, {}, {}), 3));
    commit(tracker, {5, 0});
    ASSERT_TRUE(sink.recorder.commitOwner(40, {5, 0}));
    EXPECT_TRUE(tracker.dispatchAccepted(3));
    EXPECT_EQ(tracker.issueWindowOccupancy(NpuEngineClass::Vector), 2u);

    std::vector<StatSnapshot> statistics;
    tracker.collectStatistics(statistics);
    auto statistic = [&](std::string_view name) {
      return std::ranges::find(statistics, name, &StatSnapshot::name);
    };
    ASSERT_NE(statistic("issue_window_occupancy_vector"), statistics.end());
    EXPECT_EQ(statistic("issue_window_occupancy_vector")->value, 2u);
    ASSERT_NE(statistic("issue_window_stalls_vector"), statistics.end());
    EXPECT_EQ(statistic("issue_window_stalls_vector")->value, 1u);
  }
}

TEST(NpuDependencyTrackerTest,
     ExactGenerationRecycleReusesLowestTagAndBoundsLongRunningState) {
  NpuDependencyTracker tracker("dependencies", 40, nullptr, {4, 4, 4, 4}, 2);
  std::optional<NpuPhysicalTag> previous;
  std::array<uint64_t, 2> generations{};
  for (uint64_t sequence = 1; sequence <= 64; ++sequence) {
    ASSERT_TRUE(tracker.proposeDispatch(
        tileInstruction("TADD", sequence, 0, {}, {"0x10"}), sequence));
    commit(tracker, {sequence * 4, 0});
    auto current = tracker.producerTag(0, "block/0/tile/0x10");
    ASSERT_TRUE(current);
    if (generations[current->tag] != 0)
      EXPECT_FALSE(
          tracker.proposeRecycle({current->tag, generations[current->tag]}));
    EXPECT_GT(current->generation, generations[current->tag]);
    generations[current->tag] = current->generation;
    if (previous) {
      ASSERT_TRUE(tracker.proposeRecycle(*previous));
      commit(tracker, {sequence * 4 + 1, 0});
    }
    ASSERT_TRUE(tracker.proposeIssue(NpuEngineClass::Vector));
    commit(tracker, {sequence * 4 + 2, 0});
    const NpuIssueEntry issued = tracker.issued().front();
    ASSERT_TRUE(tracker.proposeComplete(completionFor(issued)));
    commit(tracker, {sequence * 4 + 3, 0});
    if (previous) {
      EXPECT_FALSE(tracker.proposeRecycle(*previous));
      if (current->tag == previous->tag)
        EXPECT_GT(current->generation, previous->generation);
    }
    previous = current;
    EXPECT_EQ(tracker.outstandingProducerCount(), 0u);
    EXPECT_EQ(tracker.liveFlowCount(), 0u);
  }
  EXPECT_EQ(tracker.freeTagCount(), 1u);
}

TEST(NpuScheduleV2Test,
     QueueBackedPathPreservesEntriesAcrossOutputBackpressure) {
  RecorderSink sink;
  Module parent("parent", 1, nullptr);
  SimQueue<NpuDispatch> dispatch("dispatch", 2, &parent, 4);
  SimQueue<NpuCompletion> completion("completion", 3, &parent, 4);
  SimQueue<NpuPhysicalTag> recycle("recycle", 4, &parent, 4);
  SimQueue<NpuIssueEntry> scalar("scalar", 5, &parent, 1);
  SimQueue<NpuIssueEntry> vector("vector", 6, &parent, 1);
  SimQueue<NpuIssueEntry> cube("cube", 7, &parent, 1);
  SimQueue<NpuIssueEntry> tma("tma", 8, &parent, 1);
  for (SimObject *queue : std::array<SimObject *, 7>{
           &dispatch, &completion, &recycle, &scalar, &vector, &cube, &tma})
    ASSERT_TRUE(parent.attachChild(*queue));
  NpuScheduleV2 schedule("schedule", 9, &parent, {4, 4, 4, 4}, 4, dispatch,
                         completion, {&scalar, &vector, &cube, &tma}, &recycle,
                         &sink);
  EXPECT_EQ(NpuScheduleV2::contractName, "ac.schedule.v2");
  ASSERT_TRUE(parent.attachChild(schedule));
  ASSERT_TRUE(schedule.validate());
  EXPECT_TRUE(schedule.children().empty());
  Module other("other", 10, nullptr);
  SimQueue<NpuCompletion> foreignCompletion("completion", 11, &other, 1);
  EXPECT_THROW(
      (NpuScheduleV2("invalid", 12, &parent, {4, 4, 4, 4}, 4, dispatch,
                     foreignCompletion, {&scalar, &vector, &cube, &tma})),
      std::invalid_argument);
  SimQueue<NpuCompletion> unattachedCompletion("unattached", 13, &parent, 1);
  EXPECT_THROW(
      (NpuScheduleV2("invalid", 14, &parent, {4, 4, 4, 4}, 4, dispatch,
                     unattachedCompletion, {&scalar, &vector, &cube, &tma})),
      std::invalid_argument);
  EXPECT_THROW((NpuScheduleV2("invalid", 15, &parent, {4, 4, 4, 4}, 4, dispatch,
                              completion, {&scalar, &scalar, &cube, &tma})),
               std::invalid_argument);

  auto commitQueue = [](auto &queue, Epoch epoch) {
    queue.doArbitrate(epoch);
    queue.doXfer(epoch);
  };
  auto run = [&](Epoch epoch) {
    schedule.doWork(epoch);
    schedule.doArbitrate(epoch);
    dispatch.doArbitrate(epoch);
    completion.doArbitrate(epoch);
    recycle.doArbitrate(epoch);
    scalar.doArbitrate(epoch);
    vector.doArbitrate(epoch);
    cube.doArbitrate(epoch);
    tma.doArbitrate(epoch);
    schedule.doXfer(epoch);
    dispatch.doXfer(epoch);
    completion.doXfer(epoch);
    recycle.doXfer(epoch);
    scalar.doXfer(epoch);
    vector.doXfer(epoch);
    cube.doXfer(epoch);
    tma.doXfer(epoch);
    EXPECT_TRUE(sink.recorder.commitOwner(9, epoch));
  };

  ASSERT_TRUE(
      dispatch.proposePush({tileInstruction("TADD", 1, 0, {}, {"0x10"}), 11}));
  commitQueue(dispatch, {0, 0});
  run({1, 0});
  EXPECT_TRUE(dispatch.isEmpty());
  run({2, 0});
  ASSERT_EQ(vector.committedSize(), 1u);
  const NpuIssueEntry first = vector.committedValues().front();

  ASSERT_TRUE(dispatch.proposePush(
      {tileInstruction("TADD", 2, 0, {"0x10"}, {"0x20"}), 12}));
  commitQueue(dispatch, {3, 0});
  run({4, 0});
  ASSERT_TRUE(completion.proposePush(completionFor(first)));
  commitQueue(completion, {5, 0});
  run({6, 0});
  EXPECT_TRUE(completion.isEmpty());
  const size_t freeTagsBeforeReject = schedule.tracker().freeTagCount();
  const size_t outstandingBeforeReject =
      schedule.tracker().outstandingProducerCount();
  ASSERT_TRUE(completion.proposePush(completionFor(first)));
  ASSERT_TRUE(recycle.proposePush({3, 99}));
  commitQueue(completion, {7, 0});
  commitQueue(recycle, {7, 0});
  run({8, 0});
  EXPECT_TRUE(completion.isEmpty());
  EXPECT_TRUE(recycle.isEmpty());
  EXPECT_EQ(schedule.tracker().freeTagCount(), freeTagsBeforeReject);
  EXPECT_EQ(schedule.tracker().outstandingProducerCount(),
            outstandingBeforeReject);
  EXPECT_TRUE(
      std::ranges::any_of(sink.recorder.events(), [](const auto &event) {
        return event.category == "stall" && event.name == "completion_rejected";
      }));
  EXPECT_TRUE(
      std::ranges::any_of(sink.recorder.events(), [](const auto &event) {
        return event.category == "stall" && event.name == "recycle_rejected";
      }));
  std::vector<StatSnapshot> rejectedStatistics;
  schedule.collectStatistics(rejectedStatistics);
  auto rejectedStatistic = [&](std::string_view name) {
    return std::ranges::find(rejectedStatistics, name, &StatSnapshot::name);
  };
  ASSERT_NE(rejectedStatistic("completion_update_rejects"),
            rejectedStatistics.end());
  EXPECT_EQ(rejectedStatistic("completion_update_rejects")->value, 1u);
  EXPECT_EQ(rejectedStatistic("completion_update_rejects")->lastUpdate,
            (Epoch{8, 0}));
  ASSERT_NE(rejectedStatistic("recycle_update_rejects"),
            rejectedStatistics.end());
  EXPECT_EQ(rejectedStatistic("recycle_update_rejects")->value, 1u);
  EXPECT_EQ(rejectedStatistic("recycle_update_rejects")->lastUpdate,
            (Epoch{8, 0}));

  run({10, 0});
  EXPECT_EQ(vector.committedSize(), 1u);
  EXPECT_EQ(schedule.tracker().queueSize(NpuEngineClass::Vector), 1u);
  EXPECT_TRUE(
      std::ranges::any_of(sink.recorder.events(), [](const auto &event) {
        return event.category == "stall" && event.name == "output_backpressure";
      }));

  ASSERT_TRUE(vector.proposePop());
  commitQueue(vector, {11, 0});
  run({12, 0});
  ASSERT_EQ(vector.committedSize(), 1u);
  EXPECT_EQ(vector.committedValues().front().instruction.sequenceId, 2u);
  EXPECT_EQ(schedule.tracker().queueSize(NpuEngineClass::Vector), 0u);
  const NpuIssueEntry second = vector.committedValues().front();
  ASSERT_TRUE(dispatch.proposePush(
      {tileInstruction("TADD", 3, 0, {}, {"0x30"}), kInvalidObjectId}));
  ASSERT_TRUE(
      dispatch.proposePush({tileInstruction("TADD", 2, 0, {}, {"0x30"}), 16}));
  ASSERT_TRUE(
      dispatch.proposePush({tileInstruction("TADD", 1, 0, {}, {"0x30"}), 17}));
  ASSERT_TRUE(
      dispatch.proposePush({tileInstruction("TADD", 3, 0, {}, {"0x30"}), 18}));
  commitQueue(dispatch, {13, 0});
  const size_t freeTagsBeforeDispatchRejects =
      schedule.tracker().freeTagCount();
  const size_t outstandingBeforeDispatchRejects =
      schedule.tracker().outstandingProducerCount();
  run({14, 0});
  run({15, 0});
  run({16, 0});
  EXPECT_EQ(dispatch.committedSize(), 1u);
  EXPECT_EQ(schedule.tracker().queueSize(NpuEngineClass::Vector), 0u);
  EXPECT_EQ(schedule.tracker().freeTagCount(), freeTagsBeforeDispatchRejects);
  EXPECT_EQ(schedule.tracker().outstandingProducerCount(),
            outstandingBeforeDispatchRejects);
  EXPECT_FALSE(schedule.tracker().producerTag(0, "block/0/tile/0x30"));
  run({17, 0});
  EXPECT_TRUE(dispatch.isEmpty());
  EXPECT_EQ(schedule.tracker().queueSize(NpuEngineClass::Vector), 1u);
  std::vector<StatSnapshot> dispatchStatistics;
  schedule.collectStatistics(dispatchStatistics);
  auto dispatchRejects = std::ranges::find(
      dispatchStatistics, "dispatch_input_rejects", &StatSnapshot::name);
  ASSERT_NE(dispatchRejects, dispatchStatistics.end());
  EXPECT_EQ(dispatchRejects->value, 3u);
  EXPECT_EQ(dispatchRejects->lastUpdate, (Epoch{16, 0}));
  EXPECT_EQ(std::ranges::count_if(sink.recorder.events(),
                                  [](const CommittedEvent &event) {
                                    return event.category == "stall" &&
                                           event.name == "dispatch_rejected";
                                  }),
            3);

  ASSERT_TRUE(completion.proposePush(completionFor(second)));
  commitQueue(completion, {18, 0});
  run({19, 0});
  EXPECT_TRUE(completion.isEmpty());
  run({20, 0});
  EXPECT_EQ(vector.committedSize(), 1u);
}

TEST(NpuScheduleV2Test, InvalidEngineDispatchIsConsumedWithoutStateMutation) {
  RecorderSink sink;
  Module parent("parent", 1, nullptr);
  SimQueue<NpuDispatch> dispatch("dispatch", 2, &parent, 1);
  SimQueue<NpuCompletion> completion("completion", 3, &parent, 1);
  SimQueue<NpuIssueEntry> scalar("scalar", 4, &parent, 1);
  SimQueue<NpuIssueEntry> vector("vector", 5, &parent, 1);
  SimQueue<NpuIssueEntry> cube("cube", 6, &parent, 1);
  SimQueue<NpuIssueEntry> tma("tma", 7, &parent, 1);
  for (SimObject *queue : std::array<SimObject *, 6>{
           &dispatch, &completion, &scalar, &vector, &cube, &tma})
    ASSERT_TRUE(parent.attachChild(*queue));
  NpuScheduleV2 schedule("schedule", 8, &parent, {1, 1, 1, 1}, 2, dispatch,
                         completion, {&scalar, &vector, &cube, &tma}, nullptr,
                         &sink);
  ASSERT_TRUE(parent.attachChild(schedule));

  NpuInstruction invalid =
      tileInstruction("TADD", 1, 0, {}, {"block/0/tile/0x10"});
  invalid.engine = static_cast<NpuEngineClass>(255);
  EXPECT_EQ(schedule.tracker().validateDispatch(invalid, 9),
            NpuDispatchValidation::InvalidEngineClass);
  EXPECT_THROW(schedule.tracker().queueSize(invalid.engine),
               std::invalid_argument);

  const size_t freeTags = schedule.tracker().freeTagCount();
  ASSERT_TRUE(dispatch.proposePush({invalid, 9}));
  dispatch.doArbitrate({0, 0});
  dispatch.doXfer({0, 0});

  schedule.doWork({1, 0});
  schedule.doArbitrate({1, 0});
  for (SimObject *queue : std::array<SimObject *, 6>{
           &dispatch, &completion, &scalar, &vector, &cube, &tma})
    queue->doArbitrate({1, 0});
  schedule.doXfer({1, 0});
  for (SimObject *queue : std::array<SimObject *, 6>{
           &dispatch, &completion, &scalar, &vector, &cube, &tma})
    queue->doXfer({1, 0});
  ASSERT_TRUE(sink.recorder.commitOwner(8, {1, 0}));

  EXPECT_TRUE(dispatch.isEmpty());
  EXPECT_EQ(schedule.tracker().freeTagCount(), freeTags);
  EXPECT_EQ(schedule.tracker().outstandingProducerCount(), 0u);
  EXPECT_EQ(schedule.tracker().liveFlowCount(), 0u);
  EXPECT_EQ(schedule.tracker().queueSize(NpuEngineClass::Scalar), 0u);
  EXPECT_EQ(schedule.tracker().queueSize(NpuEngineClass::Vector), 0u);
  EXPECT_EQ(schedule.tracker().queueSize(NpuEngineClass::Cube), 0u);
  EXPECT_EQ(schedule.tracker().queueSize(NpuEngineClass::Tma), 0u);

  auto rejected = std::ranges::find_if(
      sink.recorder.events(), [](const CommittedEvent &event) {
        return event.name == "dispatch_rejected";
      });
  ASSERT_NE(rejected, sink.recorder.events().end());
  auto reason = std::ranges::find(rejected->arguments, "reason",
                                  &ObservationArgument::name);
  ASSERT_NE(reason, rejected->arguments.end());
  EXPECT_EQ(reason->value,
            ObservationValue{std::string("invalid_engine_class")});

  std::vector<StatSnapshot> statistics;
  schedule.collectStatistics(statistics);
  auto rejects = std::ranges::find(statistics, "dispatch_input_rejects",
                                   &StatSnapshot::name);
  ASSERT_NE(rejects, statistics.end());
  EXPECT_EQ(rejects->value, 1u);
  EXPECT_EQ(rejects->lastUpdate, (Epoch{1, 0}));

  NpuInstruction valid =
      tileInstruction("TADD", 1, 0, {}, {"block/0/tile/0x10"});
  ASSERT_TRUE(dispatch.proposePush({valid, 10}));
  dispatch.doArbitrate({2, 0});
  dispatch.doXfer({2, 0});
  schedule.doWork({3, 0});
  schedule.doArbitrate({3, 0});
  for (SimObject *queue : std::array<SimObject *, 6>{
           &dispatch, &completion, &scalar, &vector, &cube, &tma})
    queue->doArbitrate({3, 0});
  schedule.doXfer({3, 0});
  for (SimObject *queue : std::array<SimObject *, 6>{
           &dispatch, &completion, &scalar, &vector, &cube, &tma})
    queue->doXfer({3, 0});
  ASSERT_TRUE(sink.recorder.commitOwner(8, {3, 0}));
  EXPECT_TRUE(dispatch.isEmpty());
  EXPECT_EQ(schedule.tracker().queueSize(NpuEngineClass::Vector), 1u);
  EXPECT_EQ(std::ranges::count_if(sink.recorder.events(),
                                  [](const CommittedEvent &event) {
                                    return event.name == "dispatch_rejected";
                                  }),
            1);
}

TEST(NpuScheduleV2Test, ObservationFailureCancelsAllFunctionalProposals) {
  RejectingSink sink;
  Module parent("parent", 1, nullptr);
  SimQueue<NpuDispatch> dispatch("dispatch", 2, &parent, 1);
  SimQueue<NpuCompletion> completion("completion", 3, &parent, 1);
  SimQueue<NpuIssueEntry> scalar("scalar", 4, &parent, 1);
  SimQueue<NpuIssueEntry> vector("vector", 5, &parent, 1);
  SimQueue<NpuIssueEntry> cube("cube", 6, &parent, 1);
  SimQueue<NpuIssueEntry> tma("tma", 7, &parent, 1);
  for (SimObject *queue : std::array<SimObject *, 6>{
           &dispatch, &completion, &scalar, &vector, &cube, &tma})
    ASSERT_TRUE(parent.attachChild(*queue));
  NpuScheduleV2 schedule("schedule", 8, &parent, {2, 2, 2, 2}, 2, dispatch,
                         completion, {&scalar, &vector, &cube, &tma}, nullptr,
                         &sink);
  ASSERT_TRUE(parent.attachChild(schedule));
  ASSERT_TRUE(
      dispatch.proposePush({tileInstruction("TADD", 1, 0, {}, {"0x10"}), 9}));
  dispatch.doArbitrate({0, 0});
  dispatch.doXfer({0, 0});

  schedule.doWork({1, 0});
  schedule.doArbitrate({1, 0});
  EXPECT_EQ(schedule.runtimeFailureCode(), "observation_proposal_failed");
  EXPECT_FALSE(schedule.hasPendingCommit());
  EXPECT_EQ(dispatch.committedSize(), 1u);
  EXPECT_EQ(vector.committedSize(), 0u);
  EXPECT_EQ(schedule.tracker().queueSize(NpuEngineClass::Vector), 0u);
  EXPECT_EQ(schedule.tracker().freeTagCount(), 2u);
  EXPECT_FALSE(schedule.tracker().producerTag(0, "block/0/tile/0x10"));
  dispatch.doArbitrate({1, 0});
  schedule.doXfer({1, 0});
  dispatch.doXfer({1, 0});
  EXPECT_EQ(dispatch.committedSize(), 1u);
}

TEST(NpuExecutionPipelineTest, FrozenLatenciesCompleteFourEnginesPrecisely) {
  NpuExecutionPipeline pipeline("execution", 50, nullptr,
                                {.scalarUnits = 1,
                                 .vectorUnits = 1,
                                 .cubeUnits = 1,
                                 .tmaUnits = 1,
                                 .memoryRequests = 2,
                                 .scratchpadTiles = 8});
  const std::array entries = {
      issueEntry(tileInstruction("TASSIGN", 1, 0, {}, {"0x10"}), 10),
      issueEntry(tileInstruction("TADD", 2, 0, {}, {"0x20"}), 11),
      issueEntry(tileInstruction("TMATMUL", 3, 0, {}, {"0x30"}), 12),
      issueEntry(tileInstruction("TLOAD", 4, 0, {"0x100"}, {"0x40"}), 13),
  };
  for (const NpuIssueEntry &entry : entries)
    ASSERT_TRUE(pipeline.proposeAdmit(entry.instruction));
  commit(pipeline, {0, 0});
  for (const NpuIssueEntry &entry : entries)
    ASSERT_TRUE(pipeline.proposeExecute(entry, {1, 0}));
  commit(pipeline, {1, 0});

  EXPECT_EQ(pipeline.completionEpoch(1), (Epoch{2, 0}));
  EXPECT_EQ(pipeline.completionEpoch(2), (Epoch{3, 0}));
  EXPECT_EQ(pipeline.completionEpoch(3), (Epoch{5, 0}));
  EXPECT_EQ(pipeline.completionEpoch(4), (Epoch{4, 0}));

  pipeline.doWork({2, 0});
  commit(pipeline, {2, 0});
  ASSERT_EQ(pipeline.completed().size(), 1u);
  EXPECT_EQ(pipeline.completed().front().instruction.sequenceId, 1u);
  pipeline.doWork({3, 0});
  commit(pipeline, {3, 0});
  ASSERT_EQ(pipeline.completed().size(), 1u);
  EXPECT_EQ(pipeline.completed().front().instruction.sequenceId, 2u);
  pipeline.doWork({4, 0});
  commit(pipeline, {4, 0});
  ASSERT_EQ(pipeline.completed().size(), 1u);
  EXPECT_EQ(pipeline.completed().front().instruction.sequenceId, 4u);
  pipeline.doWork({5, 0});
  commit(pipeline, {5, 0});
  ASSERT_EQ(pipeline.completed().size(), 1u);
  EXPECT_EQ(pipeline.completed().front().instruction.sequenceId, 3u);
}

TEST(NpuExecutionPipelineTest, UnitPressureRejectsWithoutMutatingIssue) {
  NpuExecutionPipeline pipeline("execution", 50, nullptr,
                                {.scalarUnits = 1,
                                 .vectorUnits = 1,
                                 .cubeUnits = 1,
                                 .tmaUnits = 1,
                                 .memoryRequests = 1,
                                 .scratchpadTiles = 2});
  const NpuIssueEntry first =
      issueEntry(tileInstruction("TADD", 1, 0, {}, {"0x10"}), 10);
  const NpuIssueEntry blocked =
      issueEntry(tileInstruction("TADD", 2, 0, {}, {"0x20"}), 11);
  // NOLINTNEXTLINE(performance-unnecessary-copy-initialization)
  const NpuIssueEntry original = blocked;
  ASSERT_TRUE(pipeline.proposeAdmit(first.instruction));
  ASSERT_TRUE(pipeline.proposeAdmit(blocked.instruction));
  commit(pipeline, {0, 0});

  EXPECT_TRUE(pipeline.proposeExecute(first, {1, 0}));
  EXPECT_FALSE(pipeline.proposeExecute(blocked, {1, 0}));
  EXPECT_EQ(blocked, original);
  commit(pipeline, {1, 0});
  EXPECT_EQ(pipeline.activeExecutions(NpuEngineClass::Vector), 1u);
}

TEST(NpuExecutionPipelineTest, TmaUsesExactAddressAndCorrelation) {
  NpuExecutionPipeline pipeline("execution", 50, nullptr,
                                {.scalarUnits = 1,
                                 .vectorUnits = 1,
                                 .cubeUnits = 1,
                                 .tmaUnits = 1,
                                 .memoryRequests = 1,
                                 .scratchpadTiles = 2});
  const NpuIssueEntry load =
      issueEntry(tileInstruction("TLOAD", 7, 0, {"0x100"}, {"0x0"}), 10);
  ASSERT_TRUE(pipeline.proposeAdmit(load.instruction));
  commit(pipeline, {0, 0});
  ASSERT_TRUE(pipeline.proposeTraceExhausted());
  ASSERT_TRUE(pipeline.proposeExecute(load, {1, 0}));
  commit(pipeline, {1, 0});

  ASSERT_EQ(pipeline.memoryRequests().size(), 1u);
  EXPECT_EQ(pipeline.memoryRequests().front().sequenceId, 7u);
  EXPECT_EQ(pipeline.memoryRequests().front().correlationId, 7u);
  EXPECT_EQ(pipeline.memoryRequests().front().address, 0x100u);
  EXPECT_FALSE(pipeline.memoryRequests().front().write);

  pipeline.doWork({2, 0});
  commit(pipeline, {2, 0});
  pipeline.doWork({4, 0});
  commit(pipeline, {4, 0});
  ASSERT_EQ(pipeline.memoryResponses().size(), 1u);
  EXPECT_EQ(pipeline.memoryResponses().front().correlationId, 7u);
  EXPECT_TRUE(pipeline.scratchpadContains("block/0/tile/0x0"));
  EXPECT_FALSE(pipeline.complete());
  pipeline.doWork({5, 0});
  commit(pipeline, {5, 0});
  EXPECT_TRUE(pipeline.complete());
}

TEST(NpuExecutionPipelineTest, StoreUsesScratchpadValueAndGlobalAddress) {
  NpuExecutionPipeline pipeline("execution", 50, nullptr,
                                {.scalarUnits = 1,
                                 .vectorUnits = 1,
                                 .cubeUnits = 1,
                                 .tmaUnits = 1,
                                 .memoryRequests = 1,
                                 .scratchpadTiles = 2});
  const NpuIssueEntry produce =
      issueEntry(tileInstruction("TASSIGN", 1, 0, {}, {"0x0"}), 10);
  const NpuIssueEntry store =
      issueEntry(tileInstruction("TSTORE", 2, 0, {"0x0"}, {"0x200"}), 11);
  ASSERT_TRUE(pipeline.proposeAdmit(produce.instruction));
  ASSERT_TRUE(pipeline.proposeAdmit(store.instruction));
  commit(pipeline, {0, 0});
  ASSERT_TRUE(pipeline.proposeExecute(produce, {1, 0}));
  commit(pipeline, {1, 0});
  pipeline.doWork({2, 0});
  commit(pipeline, {2, 0});
  ASSERT_TRUE(pipeline.proposeExecute(store, {3, 0}));
  commit(pipeline, {3, 0});

  ASSERT_EQ(pipeline.memoryRequests().size(), 1u);
  EXPECT_TRUE(pipeline.memoryRequests().front().write);
  EXPECT_EQ(pipeline.memoryRequests().front().address, 0x200u);
  EXPECT_EQ(pipeline.memoryRequests().front().tileIdentity, "block/0/tile/0x0");
  pipeline.doWork({4, 0});
  commit(pipeline, {4, 0});
  EXPECT_EQ(pipeline.completionEpoch(2), (Epoch{7, 0}));
  pipeline.doWork({7, 0});
  commit(pipeline, {7, 0});
  EXPECT_EQ(pipeline.globalMemoryValue(0x200), 1u);
  EXPECT_FALSE(pipeline.scratchpadContains("block/0/tile/0x200"));
}

TEST(NpuExecutionPipelineTest, MemoryOrderingIgnoresExecuteProposalOrder) {
  NpuExecutionPipeline pipeline("execution", 50, nullptr,
                                {.scalarUnits = 1,
                                 .vectorUnits = 1,
                                 .cubeUnits = 1,
                                 .tmaUnits = 2,
                                 .memoryRequests = 2,
                                 .scratchpadTiles = 4});
  const NpuIssueEntry first =
      issueEntry(tileInstruction("TLOAD", 1, 0, {"0x100"}, {"0x10"}), 11);
  const NpuIssueEntry second =
      issueEntry(tileInstruction("TLOAD", 2, 0, {"0x200"}, {"0x20"}), 10);
  ASSERT_TRUE(pipeline.proposeAdmit(first.instruction));
  ASSERT_TRUE(pipeline.proposeAdmit(second.instruction));
  commit(pipeline, {0, 0});
  ASSERT_TRUE(pipeline.proposeExecute(second, {1, 0}));
  ASSERT_TRUE(pipeline.proposeExecute(first, {1, 0}));
  commit(pipeline, {1, 0});

  ASSERT_EQ(pipeline.memoryRequests().size(), 2u);
  EXPECT_EQ(pipeline.memoryRequests()[0].correlationId, 1u);
  EXPECT_EQ(pipeline.memoryRequests()[1].correlationId, 2u);
  pipeline.doWork({2, 0});
  commit(pipeline, {2, 0});
  pipeline.doWork({4, 0});
  commit(pipeline, {4, 0});
  ASSERT_EQ(pipeline.memoryResponses().size(), 2u);
  EXPECT_EQ(pipeline.memoryResponses()[0].correlationId, 1u);
  EXPECT_EQ(pipeline.memoryResponses()[1].correlationId, 2u);
}

TEST(NpuExecutionPipelineTest, CompletionIsDrivenByScheduledRuntimeEvent) {
  SimSystem system;
  NpuExecutionPipeline pipeline("execution", 50, nullptr,
                                {.scalarUnits = 1,
                                 .vectorUnits = 1,
                                 .cubeUnits = 1,
                                 .tmaUnits = 1,
                                 .memoryRequests = 1,
                                 .scratchpadTiles = 2},
                                &system);
  system.registerObject(&pipeline);
  const NpuIssueEntry entry =
      issueEntry(tileInstruction("TADD", 1, 0, {}, {"0x10"}), 10);
  ASSERT_TRUE(pipeline.proposeAdmit(entry.instruction));
  commit(pipeline, {0, 0});
  ASSERT_TRUE(pipeline.proposeExecute(entry, {0, 0}));
  commit(pipeline, {0, 0});

  EXPECT_EQ(system.nextEvent(), std::nullopt);
  ASSERT_TRUE(system.step());
  ASSERT_TRUE(system.nextEvent());
  EXPECT_EQ(system.nextEvent()->readyTime, (Epoch{2, 0}));
  EXPECT_EQ(system.nextEvent()->targetId, 50u);
  EXPECT_FALSE(system.step());
  ASSERT_EQ(pipeline.completed().size(), 1u);
  EXPECT_EQ(pipeline.completed().front().instruction.sequenceId, 1u);
}

TEST(NpuExecutionPipelineTest,
     OutOfOrderCompletionStillRetiresInBlockSequenceOrder) {
  NpuExecutionPipeline pipeline("execution", 50, nullptr,
                                {.scalarUnits = 1,
                                 .vectorUnits = 1,
                                 .cubeUnits = 1,
                                 .tmaUnits = 1,
                                 .memoryRequests = 1,
                                 .scratchpadTiles = 4});
  const NpuIssueEntry slow =
      issueEntry(tileInstruction("TMATMUL", 1, 0, {}, {"0x10"}), 10);
  const NpuIssueEntry fast =
      issueEntry(tileInstruction("TASSIGN", 2, 0, {}, {"0x20"}), 11);
  ASSERT_TRUE(pipeline.proposeAdmit(slow.instruction));
  ASSERT_TRUE(pipeline.proposeAdmit(fast.instruction));
  commit(pipeline, {0, 0});
  ASSERT_TRUE(pipeline.proposeExecute(slow, {1, 0}));
  ASSERT_TRUE(pipeline.proposeExecute(fast, {1, 0}));
  commit(pipeline, {1, 0});

  pipeline.doWork({2, 0});
  commit(pipeline, {2, 0});
  ASSERT_EQ(pipeline.completed().size(), 1u);
  EXPECT_EQ(pipeline.completed().front().instruction.sequenceId, 2u);
  EXPECT_TRUE(pipeline.retired().empty());
  EXPECT_EQ(pipeline.architecturalResult(), NpuArchitecturalResult{});
  pipeline.doWork({5, 0});
  commit(pipeline, {5, 0});
  ASSERT_EQ(pipeline.retired().size(), 2u);
  EXPECT_EQ(pipeline.retired()[0].sequenceId, 1u);
  EXPECT_EQ(pipeline.retired()[1].sequenceId, 2u);
  EXPECT_EQ(pipeline.retired()[0].timestamps.retired, 5u);
  EXPECT_EQ(pipeline.retired()[1].timestamps.retired, 5u);
  EXPECT_EQ(pipeline.architecturalResult().retiredInstructions, 2u);
  EXPECT_EQ(pipeline.architecturalResult().retiredSequenceIds,
            (std::vector<uint64_t>{1, 2}));
}

TEST(NpuExecutionPipelineTest, CompletionNeedsTraceExhaustionAndQuiescence) {
  NpuExecutionPipeline pipeline("execution", 50, nullptr,
                                {.scalarUnits = 1,
                                 .vectorUnits = 1,
                                 .cubeUnits = 1,
                                 .tmaUnits = 1,
                                 .memoryRequests = 1,
                                 .scratchpadTiles = 2});
  const NpuIssueEntry entry =
      issueEntry(tileInstruction("TASSIGN", 1, 0, {}, {"0x10"}), 10);
  ASSERT_TRUE(pipeline.proposeAdmit(entry.instruction));
  commit(pipeline, {0, 0});
  ASSERT_TRUE(pipeline.proposeTraceExhausted());
  ASSERT_TRUE(pipeline.proposeExecute(entry, {1, 0}));
  commit(pipeline, {1, 0});
  EXPECT_FALSE(pipeline.complete());
  pipeline.doWork({2, 0});
  commit(pipeline, {2, 0});
  EXPECT_TRUE(pipeline.complete());
}

TEST(NpuTraceSourceTest,
     ExecutesFourEnginesWithDependenciesAndCommitsArchitecturalState) {
  PtoTraceDocument document;
  document.records.push_back(record(
      "TLOAD", 0, 0, {tile("0x100")}, {}, {tile("0x0")},
      {"input_tile", "output_tile"},
      {tileOperand("block/0/tile/0x100"), tileOperand("block/0/tile/0x0")}));
  document.records.push_back(
      record("TASSIGN", 1, 0, {}, {scalar("uint64", "7")}, {tile("0x10")},
             {"scalar_input", "output_tile"},
             {scalarOperand("uint64", "7"), tileOperand("block/0/tile/0x10")}));
  document.records.push_back(
      record("TADD", 2, 0, {tile("0x0"), tile("0x10")}, {}, {tile("0x20")},
             {"input_tile", "input_tile", "output_tile"},
             {tileOperand("block/0/tile/0x0"), tileOperand("block/0/tile/0x10"),
              tileOperand("block/0/tile/0x20")}));
  document.records.push_back(record(
      "TMATMUL", 3, 0, {tile("0x20")}, {}, {tile("0x40")},
      {"input_tile", "output_tile"},
      {tileOperand("block/0/tile/0x20"), tileOperand("block/0/tile/0x40")}));
  document.records.push_back(record("TADD", 4, 0, {}, {}, {tile("0x50")},
                                    {"output_tile"},
                                    {tileOperand("block/0/tile/0x50")}));
  document.records.push_back(record(
      "TSTORE", 5, 0, {tile("0x40")}, {}, {tile("0x200")},
      {"input_tile", "output_tile"},
      {tileOperand("block/0/tile/0x40"), tileOperand("block/0/tile/0x200")}));

  RecorderSink sink;
  NpuTraceSource provider("trace_source", 7, nullptr, &sink);
  ASSERT_TRUE(provider.loadDocument(std::move(document)));
  provider.doWork({0, 0});
  ASSERT_TRUE(provider.runtimeFailureCode().empty())
      << provider.runtimeFailureCode();
  ASSERT_TRUE(provider.hasPendingCommit());
  provider.doXfer({0, 0});
  ASSERT_TRUE(sink.recorder.commitOwner(provider.id(), {0, 0}));

  RuntimeObjectState state = provider.runtimeState({0, 0});
  EXPECT_TRUE(state.quiescent);
  EXPECT_TRUE(state.traceEof);
  EXPECT_EQ(state.tracePosition, 6u);
  std::vector<StatSnapshot> statistics;
  provider.collectStatistics(statistics);
  auto statistic = [&](std::string_view name) {
    return std::ranges::find(statistics, name, &StatSnapshot::name);
  };
  ASSERT_NE(statistic("architectural_retired_instructions"), statistics.end());
  EXPECT_EQ(statistic("architectural_retired_instructions")->value, 6u);
  ASSERT_NE(statistic("architectural_digest"), statistics.end());
  EXPECT_NE(statistic("architectural_digest")->value,
            NpuArchitecturalResult{}.digest);
  EXPECT_GT(sink.recorder.events().size(), 24u);
}

TEST(NpuTraceSourceTest, RejectsUnsupportedOpcodeWithoutCommit) {
  PtoTraceDocument document;
  document.records.push_back(representative("TUNSUPPORTED"));
  NpuTraceSource provider("trace_source", 7, nullptr);
  ASSERT_TRUE(provider.loadDocument(std::move(document)));
  provider.doWork({0, 0});
  EXPECT_EQ(provider.runtimeFailureCode(), "npu_decode_failed");
  EXPECT_FALSE(provider.hasPendingCommit());
  EXPECT_FALSE(provider.runtimeState({0, 0}).traceEof);
}

TEST(NpuTraceSourceTest, RunsAsTheSingleSystemTraceOwner) {
  PtoTraceDocument document;
  document.records.push_back(representative("TASSIGN"));
  SimSystem system;
  NpuTraceSource provider("trace_source", 7, &system.root());
  provider.setObservationSink(&system);
  system.registerObject(&provider);
  ASSERT_TRUE(provider.loadDocument(std::move(document)));

  TerminationResult result = system.run();
  EXPECT_EQ(result.classification, TerminationClass::Completed)
      << result.diagnosticCode << ": " << result.message.value_or("");
  EXPECT_EQ(result.tracePosition, 1u);
  EXPECT_FALSE(system.observations().empty());
}

TEST(NpuTraceSourceTest, DecodesTheCheckedInDavinciOOFixture) {
  std::ifstream input(
      std::string(ACIR_TEST_SOURCE_DIR) +
          "/examples/agentic-circuit/workspaces/npu/traces/pto-trace.json",
      std::ios::binary);
  ASSERT_TRUE(input);
  std::string bytes((std::istreambuf_iterator<char>(input)),
                    std::istreambuf_iterator<char>());
  TraceLoadResult loaded = parsePtoTrace(bytes);
  ASSERT_TRUE(loaded.succeeded()) << loaded.primaryDiagnostic();
  ASSERT_TRUE(loaded.document);
  EXPECT_EQ(loaded.document->records.size(), 6u);
  for (const PtoTraceRecord &source : loaded.document->records) {
    SCOPED_TRACE(source.sequenceId);
    NpuDecodeResult decoded = NpuDecoder{}.decode(source);
    EXPECT_TRUE(decoded.succeeded()) << decoded.primaryDiagnostic();
  }
}

} // namespace
} // namespace gfsim
