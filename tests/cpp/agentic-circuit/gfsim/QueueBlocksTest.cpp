#include "gfsim/queue_blocks.h"

#include "gtest/gtest.h"

#include <cstdint>
#include <limits>
#include <type_traits>

namespace gfsim {
namespace {

struct Increment {
  int operator()(const int &value) const { return value + 1; }
};

struct SelectParity {
  size_t operator()(const int &value) const {
    return static_cast<size_t>(value & 1);
  }
};

struct SelectIndex {
  size_t operator()(const int &value) const {
    return static_cast<size_t>(value);
  }
};

struct Positive {
  bool operator()(const int &value) const { return value > 0; }
};

struct Decrement {
  int operator()(const int &value) const { return value - 1; }
};

struct IncrementAndDouble {
  std::tuple<int, int> operator()(const int &left, const int &right) const {
    return {left + 1, right * 2};
  }
};

struct SumToWide {
  std::tuple<int64_t> operator()(const int &left, const int64_t &right) const {
    return {static_cast<int64_t>(left) + right};
  }
};

struct SequencedValue {
  uint64_t sequence = 0;
  int value = 0;

  bool operator==(const SequencedValue &) const = default;
};

struct SequenceKey {
  uint64_t operator()(const SequencedValue &value) const {
    return value.sequence;
  }
};

struct SignedSequencedValue {
  int64_t sequence = 0;
};

struct SignedSequenceKey {
  int64_t operator()(const SignedSequencedValue &value) const {
    return value.sequence;
  }
};

struct DependencyValue {
  uint64_t sequence = 0;
  uint64_t predecessor = 255;
  uint64_t resource = 0;
  uint64_t cycles = 1;

  bool operator==(const DependencyValue &) const = default;
};

struct DependencyKey {
  uint64_t operator()(const DependencyValue &value) const {
    return value.sequence;
  }
};

struct DependencyPredecessor {
  uint64_t operator()(const DependencyValue &value) const {
    return value.predecessor;
  }
};

struct DependencyCost {
  uint64_t operator()(const DependencyValue &value) const {
    return value.cycles;
  }
};

struct DependencyResource {
  uint64_t operator()(const DependencyValue &value) const {
    return value.resource;
  }
};

struct MemoryRequest {
  uint8_t address = 0;
  bool write = false;
  uint16_t data = 0;

  bool operator==(const MemoryRequest &) const = default;
};

struct MemoryAddress {
  uint8_t operator()(const MemoryRequest &request) const {
    return request.address;
  }
};

struct MemoryWrite {
  bool operator()(const MemoryRequest &request) const { return request.write; }
};

struct MemoryWriteData {
  uint16_t operator()(const MemoryRequest &request) const {
    return request.data;
  }
};

struct MemoryResponse {
  MemoryRequest operator()(const MemoryRequest &request,
                           const uint16_t &oldData) const {
    MemoryRequest response = request;
    response.data = oldData;
    return response;
  }
};

struct TableAddress {
  uint8_t operator()(const MemoryRequest &request) const {
    return request.address;
  }
};

struct TableEnable {
  bool operator()(const MemoryRequest &request) const { return request.write; }
};

struct TableValue {
  uint16_t operator()(const MemoryRequest &request) const {
    return request.data;
  }
};

struct TableAlways {
  bool operator()(const MemoryRequest &) const { return true; }
};

struct TableNever {
  bool operator()(const MemoryRequest &) const { return false; }
};

struct TableZeroAddress {
  uint8_t operator()() const { return 0; }
};

struct TableNonzero {
  SimTable<uint16_t> *table = nullptr;
  bool operator()() const { return table->at(0) != 0; }
};

struct TableEntryDependentWhen {
  SimTable<uint16_t> *table = nullptr;
  bool operator()(const MemoryRequest &request) const {
    return table->checkedAt(request.address) != 0;
  }
};

struct TableStateAddress {
  uint8_t address = 0;
  uint8_t operator()() const { return address; }
};

struct TableStateEntryDependentWhen {
  SimTable<uint16_t> *table = nullptr;
  uint8_t address = 0;
  bool operator()() const { return table->checkedAt(address) != 0; }
};

struct StateWriteAddress {
  unsigned *calls = nullptr;
  uint8_t operator()() const {
    ++*calls;
    return 0;
  }
};

struct StateWriteEnable {
  bool *enabled = nullptr;
  bool operator()() const { return *enabled; }
};

struct StateWriteIncrement {
  SimTable<uint16_t> *table = nullptr;
  uint16_t operator()() const {
    return static_cast<uint16_t>(table->at(0) + 1);
  }
};

struct MaskedWriteMask {
  uint64_t *mask = nullptr;
  unsigned *calls = nullptr;
  uint64_t operator()() const {
    ++*calls;
    return *mask;
  }
};

struct MaskedWriteEnable {
  bool *enabled = nullptr;
  bool operator()() const { return *enabled; }
};

struct MaskedWriteIncrement {
  unsigned *calls = nullptr;
  uint16_t operator()(const uint16_t &oldValue) const {
    ++*calls;
    return static_cast<uint16_t>(oldValue + 10);
  }
};

struct FieldEntry {
  bool valid = false;
  bool ready = false;
};

struct MergeValid {
  static constexpr std::array<size_t, 1> fields{0};
  void operator()(FieldEntry &target, const FieldEntry &value) const {
    target.valid = value.valid;
  }
};

struct MergeReady {
  static constexpr std::array<size_t, 1> fields{1};
  void operator()(FieldEntry &target, const FieldEntry &value) const {
    target.ready = value.ready;
  }
};

struct CountingMatch {
  unsigned *calls = nullptr;
  bool operator()(const FieldEntry &entry) const {
    ++*calls;
    return entry.valid;
  }
};

struct CachedMask {
  TableMatchCache<FieldEntry, CountingMatch> *match = nullptr;
  unsigned *calls = nullptr;
  uint64_t operator()(Epoch epoch) const {
    ++*calls;
    return match->get(epoch);
  }
};

struct CountingReadyKey {
  unsigned *calls = nullptr;
  uint64_t operator()(const FieldEntry &entry) const {
    ++*calls;
    return entry.ready ? 0 : 1;
  }
};

struct SlotReleaseFlag {
  bool *release = nullptr;
  bool operator()() const { return *release; }
};

struct SlotReleaseAtEpoch {
  Epoch releaseEpoch{};
  bool operator()(Epoch epoch) const { return epoch == releaseEpoch; }
};

TEST(QueueBlocksTest, HighLevelProvidersFreezeStructuralTemplateParameters) {
  using Schedule4 =
      Schedule<DependencyValue, 16, 4, 255, DependencyKey,
               DependencyPredecessor, DependencyResource, DependencyCost>;
  using Schedule2 =
      Schedule<DependencyValue, 8, 2, 255, DependencyKey, DependencyPredecessor,
               DependencyResource, DependencyCost>;
  using Engine4 = Engine<DependencyValue, 4, DependencyCost>;
  using ComputeInt = Compute<int, int, 1, Increment>;
  using Pipeline2 = Pipeline<int, 2, 1>;
  using Ordered64 = Reorder<SequencedValue, 64, 0, SequenceKey>;
  using Table32 = SimTable<uint16_t>;

  static_assert(!std::is_same_v<Schedule4, Schedule2>);
  EXPECT_EQ(Schedule4::contractName, "ac.schedule");
  EXPECT_EQ(Engine4::contractName, "ac.engine");
  EXPECT_EQ(ComputeInt::contractName, "ac.compute");
  EXPECT_EQ(Pipeline2::contractName, "ac.pipeline");
  EXPECT_EQ(Ordered64::contractName, "ac.reorder");
  EXPECT_EQ(Table32::contractName, "ac.table");

  SimQueue<DependencyValue> dependencyInput("dependency_input", 1, nullptr, 2);
  SimQueue<DependencyValue> dependencyOutput("dependency_output", 2, nullptr,
                                             2);
  Schedule4 schedule("schedule", 3, nullptr, dependencyInput, dependencyOutput);
  Engine4 engine("engine", 4, nullptr, dependencyInput, dependencyOutput);
  SimQueue<SequencedValue> orderedInput("ordered_input", 5, nullptr, 2);
  SimQueue<SequencedValue> orderedOutput("ordered_output", 6, nullptr, 2);
  Ordered64 reorder("reorder", 7, nullptr, orderedInput, orderedOutput);
  Table32 table("table", 10, nullptr, 32);
  SimQueue<int> computeInput("compute_input", 11, nullptr, 2);
  SimQueue<int> computeOutput("compute_output", 12, nullptr, 2);
  ComputeInt compute("compute", 13, nullptr, computeInput, computeOutput);
  SimQueue<int> pipelineOutput("pipeline_output", 14, nullptr, 2,
                               std::numeric_limits<size_t>::max(), nullptr, 2);
  Pipeline2 pipeline("pipeline", 15, nullptr, computeOutput, pipelineOutput);

  EXPECT_EQ(schedule.active(), 0u);
  EXPECT_EQ(engine.active(), 0u);
  EXPECT_EQ(reorder.active(), 0u);
  EXPECT_EQ(table.at(0), 0u);
  EXPECT_FALSE(compute.hasPendingCommit());
  EXPECT_FALSE(pipeline.hasPendingCommit());
}

TEST(QueueBlocksTest, StatefulTableReadsOldDataAndCommitsWriteAtTickEnd) {
  SimTable<uint16_t> table("table", 1, nullptr, 8);
  SimQueue<MemoryRequest> readInput("read_input", 2, nullptr, 1);
  SimQueue<uint16_t> readOutput("read_output", 3, nullptr, 1);
  SimQueue<MemoryRequest> writeInput("write_input", 4, nullptr, 1);
  QueueTableRead<MemoryRequest, uint16_t, TableAddress, TableAlways> read(
      "read", 5, nullptr, table, readInput, readOutput);
  QueueTableWrite<MemoryRequest, uint16_t, TableAddress, TableEnable,
                  TableValue>
      write("write", 6, nullptr, table, writeInput);

  ASSERT_TRUE(readInput.proposePush({2, false, 0}));
  ASSERT_TRUE(writeInput.proposePush({2, true, 42}));
  readInput.doXfer({0, 0});
  writeInput.doXfer({0, 0});

  read.doWork({1, 0});
  write.doWork({1, 0});
  EXPECT_EQ(table.at(2), 0u);
  readInput.doXfer({1, 0});
  readOutput.doXfer({1, 0});
  writeInput.doXfer({1, 0});
  read.doXfer({1, 0});
  write.doXfer({1, 0});

  ASSERT_NE(readOutput.peek(), nullptr);
  EXPECT_EQ(*readOutput.peek(), 0u);
  EXPECT_EQ(table.at(2), 42u);
  table.reset();
  EXPECT_EQ(table.at(2), 0u);
}

TEST(QueueBlocksTest, DisabledTableWriteConsumesWithoutChangingState) {
  SimTable<uint16_t> table("table", 1, nullptr, 4);
  SimQueue<MemoryRequest> input("input", 2, nullptr, 1);
  QueueTableWrite<MemoryRequest, uint16_t, TableAddress, TableEnable,
                  TableValue>
      write("write", 3, nullptr, table, input);
  ASSERT_TRUE(input.proposePush({1, false, 99}));
  input.doXfer({0, 0});
  write.doWork({1, 0});
  input.doXfer({1, 0});
  write.doXfer({1, 0});
  EXPECT_TRUE(input.isEmpty());
  EXPECT_EQ(table.at(1), 0u);
}

TEST(QueueBlocksTest, TableReadPreservesDisabledAndOutOfRangeRequests) {
  SimTable<uint16_t> table("table", 1, nullptr, 4);
  SimQueue<MemoryRequest> disabledInput("disabled_input", 2, nullptr, 1);
  SimQueue<uint16_t> disabledOutput("disabled_output", 3, nullptr, 1);
  QueueTableRead<MemoryRequest, uint16_t, TableAddress, TableNever> disabled(
      "disabled", 4, nullptr, table, disabledInput, disabledOutput);
  ASSERT_TRUE(disabledInput.proposePush({1, false, 0}));
  disabledInput.doXfer({0, 0});
  disabled.doWork({1, 0});
  EXPECT_FALSE(disabledInput.isEmpty());
  EXPECT_TRUE(disabledOutput.isEmpty());

  SimQueue<MemoryRequest> invalidInput("invalid_input", 5, nullptr, 1);
  SimQueue<uint16_t> invalidOutput("invalid_output", 6, nullptr, 1);
  QueueTableRead<MemoryRequest, uint16_t, TableAddress, TableAlways> invalid(
      "invalid", 7, nullptr, table, invalidInput, invalidOutput);
  ASSERT_TRUE(invalidInput.proposePush({4, false, 0}));
  invalidInput.doXfer({0, 0});
  invalid.doWork({1, 0});
  EXPECT_EQ(invalid.runtimeFailureCode(), "table_index_out_of_range");
  EXPECT_FALSE(invalidInput.isEmpty());
  EXPECT_TRUE(invalidOutput.isEmpty());
  EXPECT_TRUE(table.runtimeFailureCode().empty());
}

TEST(QueueBlocksTest, QueueTableEntryDependentWhenReportsDynamicOutOfRange) {
  SimTable<uint16_t> table("table", 1, nullptr, 4);
  SimQueue<MemoryRequest> input("input", 2, nullptr, 1);
  SimQueue<uint16_t> output("output", 3, nullptr, 1);
  QueueTableRead<MemoryRequest, uint16_t, TableAddress, TableEntryDependentWhen>
      read("read", 4, nullptr, table, input, output, {}, {&table});

  ASSERT_TRUE(input.proposePush({4, false, 0}));
  input.doXfer({0, 0});
  EXPECT_NO_THROW(read.doWork({1, 0}));
  EXPECT_EQ(table.runtimeFailureCode(), "table_index_out_of_range");
  EXPECT_TRUE(read.runtimeFailureCode().empty());
  EXPECT_FALSE(input.isEmpty());
  EXPECT_TRUE(output.isEmpty());
}

TEST(QueueBlocksTest, StateTableEntryDependentWhenReportsDynamicOutOfRange) {
  SimTable<uint16_t> table("table", 1, nullptr, 4);
  SimQueue<uint16_t> output("output", 2, nullptr, 1);
  TableReadSource<uint16_t, TableStateAddress, TableStateEntryDependentWhen>
      read("read", 3, nullptr, table, output, {4}, {&table, 4});

  EXPECT_NO_THROW(read.doWork({1, 0}));
  EXPECT_EQ(table.runtimeFailureCode(), "table_index_out_of_range");
  EXPECT_TRUE(read.runtimeFailureCode().empty());
  EXPECT_TRUE(output.isEmpty());
}

TEST(QueueBlocksTest, FalseTableWhenDoesNotEvaluateAddressOrReportFailure) {
  SimTable<uint16_t> table("table", 1, nullptr, 4);
  SimQueue<MemoryRequest> input("input", 2, nullptr, 1);
  SimQueue<uint16_t> output("output", 3, nullptr, 1);
  QueueTableRead<MemoryRequest, uint16_t, TableAddress, TableNever> read(
      "read", 4, nullptr, table, input, output);

  ASSERT_TRUE(input.proposePush({4, false, 0}));
  input.doXfer({0, 0});
  read.doWork({1, 0});
  EXPECT_TRUE(table.runtimeFailureCode().empty());
  EXPECT_TRUE(read.runtimeFailureCode().empty());
  EXPECT_FALSE(input.isEmpty());
  EXPECT_TRUE(output.isEmpty());
}

TEST(QueueBlocksTest, StateTableReadRepeatsAndBackpressuredValueStaysStable) {
  SimTable<uint16_t> table("table", 1, nullptr, 1);
  SimQueue<uint16_t> output("output", 2, nullptr, 1);
  TableReadSource<uint16_t, TableZeroAddress, TableNonzero> read(
      "read", 3, nullptr, table, output, {}, TableNonzero{&table});
  ASSERT_TRUE(table.initializeEntry(0, 7));
  read.doWork({1, 0});
  output.doXfer({1, 0});
  read.doXfer({1, 0});
  ASSERT_NE(output.peek(), nullptr);
  EXPECT_EQ(*output.peek(), 7u);

  ASSERT_TRUE(table.initializeEntry(0, 9));
  read.doWork({2, 0});
  ASSERT_NE(output.peek(), nullptr);
  EXPECT_EQ(*output.peek(), 7u);

  ASSERT_TRUE(output.proposePop());
  output.doXfer({2, 0});
  read.doWork({3, 0});
  output.doXfer({3, 0});
  read.doXfer({3, 0});
  ASSERT_NE(output.peek(), nullptr);
  EXPECT_EQ(*output.peek(), 9u);
}

TEST(QueueBlocksTest,
     StateTableWriteSkipsDisabledExpressionsAndCommitsOldState) {
  SimTable<uint16_t> table("table", 1, nullptr, 1);
  bool enabled = false;
  unsigned addressCalls = 0;
  TableWriteSource<uint16_t, StateWriteAddress, StateWriteEnable,
                   StateWriteIncrement>
      write("write", 2, nullptr, table, {&addressCalls}, {&enabled}, {&table});
  TableWriteSource<uint16_t, StateWriteAddress, StateWriteEnable,
                   StateWriteIncrement>
      allocation("allocation", 3, nullptr, table, {&addressCalls}, {&enabled},
                 {&table}, {}, TableWriteMode::Replace);

  write.doWork({0, 0});
  allocation.doWork({0, 0});
  EXPECT_EQ(addressCalls, 0u);
  EXPECT_EQ(table.at(0), 0);

  enabled = true;
  write.doWork({1, 0});
  EXPECT_EQ(addressCalls, 1u);
  EXPECT_EQ(table.at(0), 0);
  write.doXfer({1, 0});
  EXPECT_EQ(table.at(0), 1);

  write.doWork({2, 0});
  EXPECT_EQ(table.at(0), 1);
  write.doXfer({2, 0});
  EXPECT_EQ(table.at(0), 2);
}

TEST(QueueBlocksTest, TableMergesDisjointWriterFieldsFromOldState) {
  SimTable<FieldEntry> table("table", 1, nullptr, 2);
  ASSERT_TRUE(table.initializeEntry(0, FieldEntry{true, false}));
  ASSERT_TRUE(table.proposeWrite(10, 0, FieldEntry{false, false},
                                 MergeValid::fields, MergeValid{}));
  ASSERT_TRUE(table.proposeWrite(11, 0, FieldEntry{false, true},
                                 MergeReady::fields, MergeReady{}));
  EXPECT_TRUE(table.at(0).valid);
  EXPECT_FALSE(table.at(0).ready);
  table.commitWrite();
  EXPECT_FALSE(table.at(0).valid);
  EXPECT_TRUE(table.at(0).ready);

  ASSERT_TRUE(table.proposeWrite(10, 1, FieldEntry{true, false},
                                 MergeValid::fields, MergeValid{}));
  ASSERT_TRUE(table.proposeWrite(11, 0, FieldEntry{false, false},
                                 MergeReady::fields, MergeReady{}));
  table.commitWrite();
  EXPECT_TRUE(table.at(1).valid);
  EXPECT_FALSE(table.at(0).ready);
}

TEST(QueueBlocksTest, TableReplaceWinsAfterFieldMergesIndependentOfEntry) {
  SimTable<FieldEntry> table("table", 1, nullptr, 2);
  ASSERT_TRUE(table.initializeEntry(0, FieldEntry{true, false}));
  ASSERT_TRUE(table.initializeEntry(1, FieldEntry{false, false}));

  ASSERT_TRUE(table.proposeWrite(30, 0, FieldEntry{true, true},
                                 MergeReady::fields, MergeReady{}));
  EXPECT_FALSE(table.initializeEntry(1, FieldEntry{true, true}));
  ASSERT_TRUE(table.proposeWrite(
      20, 0, FieldEntry{false, false}, TableFullEntryMerge<FieldEntry>::fields,
      TableFullEntryMerge<FieldEntry>{}, TableWriteMode::Replace));
  table.commitWrite();
  EXPECT_FALSE(table.at(0).valid);
  EXPECT_FALSE(table.at(0).ready);
  EXPECT_FALSE(table.at(1).valid);
  EXPECT_FALSE(table.at(1).ready);

  ASSERT_TRUE(table.proposeWrite(30, 1, FieldEntry{false, true},
                                 MergeReady::fields, MergeReady{}));
  ASSERT_TRUE(table.proposeWrite(
      20, 0, FieldEntry{true, false}, TableFullEntryMerge<FieldEntry>::fields,
      TableFullEntryMerge<FieldEntry>{}, TableWriteMode::Replace));
  table.commitWrite();
  EXPECT_TRUE(table.at(0).valid);
  EXPECT_FALSE(table.at(0).ready);
  EXPECT_TRUE(table.at(1).ready);

  ASSERT_TRUE(table.proposeWrite(40, 0, FieldEntry{true, false},
                                 MergeValid::fields, MergeValid{}));
  ASSERT_TRUE(table.proposeWrite(
      41, 0, FieldEntry{true, true}, TableFullEntryMerge<FieldEntry>::fields,
      TableFullEntryMerge<FieldEntry>{}, TableWriteMode::Replace));
  EXPECT_FALSE(table.proposeWrite(
      42, 1, FieldEntry{false, false}, TableFullEntryMerge<FieldEntry>::fields,
      TableFullEntryMerge<FieldEntry>{}, TableWriteMode::Replace));
  table.cancelWrite(41);
  table.commitWrite();
  EXPECT_TRUE(table.at(0).valid);
  EXPECT_FALSE(table.at(0).ready);
}

TEST(QueueBlocksTest, TableMatchAndChooseAreEvaluatedOncePerEpoch) {
  SimTable<FieldEntry> table("table", 1, nullptr, 3);
  ASSERT_TRUE(table.initializeEntry(0, FieldEntry{true, false}));
  ASSERT_TRUE(table.initializeEntry(1, FieldEntry{true, true}));
  unsigned predicateCalls = 0;
  unsigned maskCalls = 0;
  unsigned keyCalls = 0;
  TableMatchCache<FieldEntry, CountingMatch> match(
      table, CountingMatch{&predicateCalls});
  TableSelectionCache<FieldEntry, CachedMask, CountingReadyKey> selection(
      table, CachedMask{&match, &maskCalls}, CountingReadyKey{&keyCalls},
      TableChoosePolicy::Min);

  EXPECT_EQ(selection.get({4, 0}).index, 1u);
  EXPECT_TRUE(selection.get({4, 0}).valid);
  EXPECT_EQ(match.get({4, 0}), 0b011u);
  EXPECT_EQ(predicateCalls, 3u);
  EXPECT_EQ(maskCalls, 1u);
  EXPECT_EQ(keyCalls, 2u);

  EXPECT_EQ(selection.get({5, 0}).index, 1u);
  EXPECT_EQ(predicateCalls, 6u);
  EXPECT_EQ(maskCalls, 2u);
  EXPECT_EQ(keyCalls, 4u);
  selection.reset();
  match.reset();
  EXPECT_TRUE(selection.get({5, 0}).valid);
  EXPECT_EQ(predicateCalls, 9u);
  EXPECT_EQ(maskCalls, 3u);
}

TEST(QueueBlocksTest, TableCancellationIsWriterLocal) {
  SimTable<FieldEntry> table("table", 1, nullptr, 1);
  ASSERT_TRUE(table.proposeWrite(10, 0, FieldEntry{true, false},
                                 MergeValid::fields, MergeValid{}));
  ASSERT_TRUE(table.proposeWrite(11, 0, FieldEntry{false, true},
                                 MergeReady::fields, MergeReady{}));
  table.cancelWrite(10);
  table.commitWrite();
  EXPECT_FALSE(table.at(0).valid);
  EXPECT_TRUE(table.at(0).ready);
}

TEST(QueueBlocksTest, MaskedTableWriteCommitsSelectedOldStateAtomically) {
  SimTable<uint16_t> table("table", 1, nullptr, 4);
  for (size_t index = 0; index < table.size(); ++index) {
    ASSERT_TRUE(table.initializeEntry(index, static_cast<uint16_t>(index + 1)));
  }
  bool enabled = false;
  uint64_t mask = 0b1011;
  unsigned maskCalls = 0;
  unsigned valueCalls = 0;
  TableMaskedWriteSource<uint16_t, MaskedWriteMask, MaskedWriteEnable,
                         MaskedWriteIncrement>
      write("write", 2, nullptr, table, {&mask, &maskCalls}, {&enabled},
            {&valueCalls});

  write.doWork({0, 0});
  EXPECT_EQ(maskCalls, 0u);
  EXPECT_EQ(valueCalls, 0u);

  enabled = true;
  write.doWork({1, 0});
  EXPECT_EQ(maskCalls, 1u);
  EXPECT_EQ(valueCalls, 3u);
  EXPECT_EQ(table.at(0), 1u);
  EXPECT_EQ(table.at(1), 2u);
  EXPECT_EQ(table.at(2), 3u);
  EXPECT_EQ(table.at(3), 4u);
  write.doXfer({1, 0});
  EXPECT_EQ(table.at(0), 11u);
  EXPECT_EQ(table.at(1), 12u);
  EXPECT_EQ(table.at(2), 3u);
  EXPECT_EQ(table.at(3), 14u);

  mask = 0;
  write.doWork({2, 0});
  EXPECT_EQ(maskCalls, 2u);
  EXPECT_EQ(valueCalls, 3u);
  write.doXfer({2, 0});
  EXPECT_EQ(table.at(0), 11u);

  table.reset();
  for (size_t index = 0; index < table.size(); ++index)
    EXPECT_EQ(table.at(index), 0u);
}

TEST(QueueBlocksTest, SlotCapturesBackpressuresReleasesAndDoesNotRefill) {
  SimQueue<uint16_t> input("input", 1, nullptr, 2);
  SlotState<uint16_t> state;
  bool release = false;
  QueueSlot<uint16_t, SlotReleaseFlag> slot("slot", 2, nullptr, input, state,
                                            {&release});

  ASSERT_TRUE(input.proposePush(7));
  input.doXfer({0, 0});
  slot.doWork({1, 0});
  EXPECT_FALSE(slot.valid());
  slot.doXfer({1, 0});
  input.doXfer({1, 0});
  EXPECT_TRUE(slot.valid());
  EXPECT_EQ(slot.value(), 7);

  ASSERT_TRUE(input.proposePush(8));
  input.doXfer({2, 0});
  slot.doWork({2, 0});
  slot.doXfer({2, 0});
  EXPECT_TRUE(slot.valid());
  EXPECT_FALSE(input.isEmpty());

  release = true;
  slot.doWork({3, 0});
  slot.doXfer({3, 0});
  EXPECT_FALSE(slot.valid());
  EXPECT_EQ(slot.value(), 7);
  EXPECT_FALSE(input.isEmpty());

  release = false;
  slot.doWork({4, 0});
  slot.doXfer({4, 0});
  input.doXfer({4, 0});
  EXPECT_TRUE(slot.valid());
  EXPECT_EQ(slot.value(), 8);
  EXPECT_TRUE(input.isEmpty());
}

TEST(QueueBlocksTest, SlotReleasePolicyMayObserveEpoch) {
  SimQueue<uint16_t> input("input", 1, nullptr, 1);
  SlotState<uint16_t> state{true, 7};
  QueueSlot<uint16_t, SlotReleaseAtEpoch> slot("slot", 2, nullptr, input, state,
                                               {{3, 1}});

  EXPECT_FALSE(slot.isRunnable({3, 0}));
  EXPECT_TRUE(slot.isRunnable({3, 1}));
  slot.doWork({3, 1});
  slot.doXfer({3, 1});
  EXPECT_FALSE(slot.valid());
}

struct SharedMemoryAddress {
  uint8_t operator()(size_t, const MemoryRequest &request) const {
    return request.address;
  }
};
struct SharedMemoryWrite {
  bool operator()(size_t, const MemoryRequest &request) const {
    return request.write;
  }
};
struct SharedMemoryWriteData {
  uint16_t operator()(size_t, const MemoryRequest &request) const {
    return request.data;
  }
};
struct SharedMemoryResponse {
  MemoryRequest operator()(size_t, const MemoryRequest &request,
                           const uint16_t &oldData) const {
    MemoryRequest response = request;
    response.data = oldData;
    return response;
  }
};

TEST(QueueBlocksTest, TransformCommitsOnlyAcrossTheQueueBarrier) {
  SimQueue<int> input("input", 1, nullptr, 2);
  SimQueue<int> output("output", 2, nullptr, 2);
  QueueTransform<int, int, Increment> transform("transform", 3, nullptr, input,
                                                output);

  ASSERT_TRUE(input.proposePush(41));
  input.doXfer({0, 0});
  transform.doWork({1, 0});

  ASSERT_NE(input.peek(), nullptr);
  EXPECT_EQ(*input.peek(), 41);
  EXPECT_TRUE(output.isEmpty());

  input.doXfer({1, 0});
  output.doXfer({1, 0});
  transform.doXfer({1, 0});
  EXPECT_TRUE(input.isEmpty());
  ASSERT_NE(output.peek(), nullptr);
  EXPECT_EQ(*output.peek(), 42);
}

TEST(QueueBlocksTest, SimQueueRateBoundsOneEpochProposals) {
  SimQueue<int> queue("rate_two", 1, nullptr, 4,
                      std::numeric_limits<size_t>::max(), nullptr, 1, 2);
  EXPECT_EQ(queue.rate(), 2u);
  EXPECT_TRUE(queue.proposePush(10));
  EXPECT_TRUE(queue.proposePush(20));
  EXPECT_FALSE(queue.proposePush(30));
  queue.doXfer({0, 0});

  ASSERT_NE(queue.peekProposable(), nullptr);
  EXPECT_EQ(*queue.peekProposable(), 10);
  EXPECT_EQ(queue.proposePop(), 10);
  ASSERT_NE(queue.peekProposable(), nullptr);
  EXPECT_EQ(*queue.peekProposable(), 20);
  EXPECT_EQ(queue.proposePop(), 20);
  EXPECT_EQ(queue.peekProposable(), nullptr);
  EXPECT_EQ(queue.proposePop(), std::nullopt);
  queue.doXfer({1, 0});
  EXPECT_TRUE(queue.isEmpty());

  EXPECT_THROW(
      (SimQueue<int>("invalid", 2, nullptr, 1,
                     std::numeric_limits<size_t>::max(), nullptr, 1, 0)),
      std::invalid_argument);
  EXPECT_THROW(
      (SimQueue<int>("too_wide", 3, nullptr, 1,
                     std::numeric_limits<size_t>::max(), nullptr, 1, 2)),
      std::invalid_argument);
}

TEST(QueueBlocksTest, ComputeConsumesAndProducesItsStaticRate) {
  SimQueue<int> input("input", 1, nullptr, 4,
                      std::numeric_limits<size_t>::max(), nullptr, 1, 2);
  SimQueue<int> output("output", 2, nullptr, 4,
                       std::numeric_limits<size_t>::max(), nullptr, 1, 2);
  Compute<int, int, 2, Increment> compute("compute", 3, nullptr, input, output);
  ASSERT_TRUE(input.proposePush(10));
  ASSERT_TRUE(input.proposePush(20));
  input.doXfer({0, 0});

  compute.doWork({1, 0});
  input.doXfer({1, 0});
  output.doXfer({1, 0});
  compute.doXfer({1, 0});

  EXPECT_TRUE(input.isEmpty());
  ASSERT_EQ(output.committedSize(), 2u);
  EXPECT_EQ(output.proposePop(), 11);
  EXPECT_EQ(output.proposePop(), 21);
}

TEST(QueueBlocksTest, TransformDoesNotConsumeWhenOutputIsBackpressured) {
  SimQueue<int> input("input", 1, nullptr, 1);
  SimQueue<int> output("output", 2, nullptr, 1);
  QueueTransform<int, int, Increment> transform("transform", 3, nullptr, input,
                                                output);
  ASSERT_TRUE(input.proposePush(7));
  ASSERT_TRUE(output.proposePush(99));
  input.doXfer({0, 0});
  output.doXfer({0, 0});

  transform.doWork({1, 0});
  input.doXfer({1, 0});
  output.doXfer({1, 0});
  ASSERT_NE(input.peek(), nullptr);
  EXPECT_EQ(*input.peek(), 7);
  ASSERT_NE(output.peek(), nullptr);
  EXPECT_EQ(*output.peek(), 99);
}

TEST(QueueBlocksTest, AtomicTransformCommitsAllQueuesTogether) {
  SimQueue<int> left("left", 1, nullptr, 1);
  SimQueue<int> right("right", 2, nullptr, 1);
  SimQueue<int> leftOutput("left_output", 3, nullptr, 1);
  SimQueue<int> rightOutput("right_output", 4, nullptr, 1);
  QueueAtomicTransform<IncrementAndDouble, std::tuple<int, int>,
                       std::tuple<int, int>>
      atomic("atomic", 5, nullptr, {&left, &right},
             {&leftOutput, &rightOutput});
  ASSERT_TRUE(left.proposePush(4));
  ASSERT_TRUE(right.proposePush(7));
  left.doXfer({0, 0});
  right.doXfer({0, 0});
  atomic.doWork({1, 0});
  EXPECT_EQ(left.committedSize(), 1u);
  EXPECT_EQ(right.committedSize(), 1u);
  EXPECT_TRUE(leftOutput.isEmpty());
  EXPECT_TRUE(rightOutput.isEmpty());
  left.doXfer({1, 0});
  right.doXfer({1, 0});
  leftOutput.doXfer({1, 0});
  rightOutput.doXfer({1, 0});
  atomic.doXfer({1, 0});
  EXPECT_TRUE(left.isEmpty());
  EXPECT_TRUE(right.isEmpty());
  ASSERT_NE(leftOutput.peek(), nullptr);
  ASSERT_NE(rightOutput.peek(), nullptr);
  EXPECT_EQ(*leftOutput.peek(), 5);
  EXPECT_EQ(*rightOutput.peek(), 14);
}

TEST(QueueBlocksTest, AtomicTransformSupportsIndependentInputOutputArity) {
  SimQueue<int> left("left", 1, nullptr, 1);
  SimQueue<int64_t> right("right", 2, nullptr, 1);
  SimQueue<int64_t> output("output", 3, nullptr, 1);
  QueueAtomicTransform<SumToWide, std::tuple<int, int64_t>, std::tuple<int64_t>>
      atomic("atomic", 4, nullptr, {&left, &right}, {&output});

  ASSERT_TRUE(left.proposePush(4));
  ASSERT_TRUE(right.proposePush(8));
  left.doXfer({0, 0});
  right.doXfer({0, 0});
  atomic.doWork({1, 0});

  EXPECT_EQ(left.committedSize(), 1u);
  EXPECT_EQ(right.committedSize(), 1u);
  EXPECT_TRUE(output.isEmpty());
  left.doXfer({1, 0});
  right.doXfer({1, 0});
  output.doXfer({1, 0});
  atomic.doXfer({1, 0});
  EXPECT_TRUE(left.isEmpty());
  EXPECT_TRUE(right.isEmpty());
  ASSERT_NE(output.peek(), nullptr);
  EXPECT_EQ(*output.peek(), 12);
}

TEST(QueueBlocksTest, BarrierTransfersHeterogeneousQueuesAtomically) {
  SimQueue<int> left("left", 1, nullptr, 1);
  SimQueue<int64_t> right("right", 2, nullptr, 1);
  SimQueue<int> leftOutput("left_output", 3, nullptr, 1);
  SimQueue<int64_t> rightOutput("right_output", 4, nullptr, 1);
  QueueBarrier<std::tuple<int, int64_t>> barrier(
      "barrier", 5, nullptr, {&left, &right}, {&leftOutput, &rightOutput});
  ASSERT_TRUE(left.proposePush(4));
  ASSERT_TRUE(right.proposePush(8));
  ASSERT_TRUE(rightOutput.proposePush(99));
  left.doXfer({0, 0});
  right.doXfer({0, 0});
  rightOutput.doXfer({0, 0});

  barrier.doWork({1, 0});
  EXPECT_FALSE(barrier.hasPendingCommit());
  EXPECT_EQ(left.committedSize(), 1u);
  EXPECT_EQ(right.committedSize(), 1u);
  EXPECT_TRUE(leftOutput.isEmpty());

  rightOutput.proposePop();
  rightOutput.doXfer({1, 0});
  barrier.doWork({2, 0});
  ASSERT_TRUE(barrier.hasPendingCommit());
  left.doXfer({2, 0});
  right.doXfer({2, 0});
  leftOutput.doXfer({2, 0});
  rightOutput.doXfer({2, 0});
  barrier.doXfer({2, 0});
  EXPECT_TRUE(left.isEmpty());
  EXPECT_TRUE(right.isEmpty());
  ASSERT_NE(leftOutput.peek(), nullptr);
  ASSERT_NE(rightOutput.peek(), nullptr);
  EXPECT_EQ(*leftOutput.peek(), 4);
  EXPECT_EQ(*rightOutput.peek(), 8);
}

TEST(QueueBlocksTest, ReorderRetiresOutOfOrderArrivalsBySequenceKey) {
  SimQueue<SequencedValue> input("input", 1, nullptr, 4);
  SimQueue<SequencedValue> output("output", 2, nullptr, 4);
  QueueReorder<SequencedValue, SequenceKey> reorder("reorder", 3, nullptr,
                                                    input, output, 4, 0);
  QueueSink<SequencedValue> sink("sink", 4, nullptr, output);
  ASSERT_TRUE(input.proposePush({2, 20}));
  ASSERT_TRUE(input.proposePush({0, 0}));
  ASSERT_TRUE(input.proposePush({1, 10}));
  input.doXfer({0, 0});

  for (uint64_t tick = 1; tick < 12; ++tick) {
    const Epoch epoch{tick, 0};
    reorder.doWork(epoch);
    sink.doWork(epoch);
    input.doXfer(epoch);
    output.doXfer(epoch);
    reorder.doXfer(epoch);
    sink.doXfer(epoch);
  }

  ASSERT_EQ(sink.received().size(), 3u);
  EXPECT_EQ(sink.received()[0], (SequencedValue{0, 0}));
  EXPECT_EQ(sink.received()[1], (SequencedValue{1, 10}));
  EXPECT_EQ(sink.received()[2], (SequencedValue{2, 20}));
}

TEST(QueueBlocksTest, ReorderRejectsDuplicateSequenceKey) {
  SimQueue<SequencedValue> input("input", 1, nullptr, 2);
  SimQueue<SequencedValue> output("output", 2, nullptr, 2);
  QueueReorder<SequencedValue, SequenceKey> reorder("reorder", 3, nullptr,
                                                    input, output, 2, 0);
  ASSERT_TRUE(input.proposePush({0, 1}));
  ASSERT_TRUE(input.proposePush({0, 2}));
  input.doXfer({0, 0});
  reorder.doWork({1, 0});
  input.doXfer({1, 0});
  output.doXfer({1, 0});
  reorder.doXfer({1, 0});
  reorder.doWork({2, 0});
  EXPECT_EQ(reorder.runtimeFailureCode(), "reorder_duplicate_key");
  EXPECT_FALSE(reorder.hasPendingCommit());
  EXPECT_TRUE(output.isEmpty());
}

TEST(QueueBlocksTest, ReorderRejectsNegativeSequenceKey) {
  SimQueue<SignedSequencedValue> input("input", 1, nullptr, 1);
  SimQueue<SignedSequencedValue> output("output", 2, nullptr, 1);
  QueueReorder<SignedSequencedValue, SignedSequenceKey> reorder(
      "reorder", 3, nullptr, input, output, 1, 0);
  ASSERT_TRUE(input.proposePush({-1}));
  input.doXfer({0, 0});
  reorder.doWork({1, 0});
  EXPECT_EQ(reorder.runtimeFailureCode(), "reorder_negative_key");
  EXPECT_FALSE(reorder.hasPendingCommit());
  EXPECT_TRUE(output.isEmpty());
}

TEST(QueueBlocksTest, DependencyCompletesReadyTokensOutOfOrder) {
  SimQueue<DependencyValue> input("input", 1, nullptr, 4);
  SimQueue<DependencyValue> output("output", 2, nullptr, 4);
  QueueDependency<DependencyValue, DependencyKey, DependencyPredecessor,
                  DependencyResource, DependencyCost>
      dependency("dependency", 3, nullptr, input, output, 4, 2, 255);
  QueueSink<DependencyValue> sink("sink", 4, nullptr, output);
  ASSERT_TRUE(input.proposePush({0, 255, 0, 4}));
  ASSERT_TRUE(input.proposePush({1, 255, 0, 1}));
  ASSERT_TRUE(input.proposePush({2, 255, 1, 1}));
  ASSERT_TRUE(input.proposePush({3, 0, 1, 1}));
  input.doXfer({0, 0});

  for (uint64_t tick = 1; tick < 16; ++tick) {
    const Epoch epoch{tick, 0};
    dependency.doWork(epoch);
    sink.doWork(epoch);
    input.doXfer(epoch);
    output.doXfer(epoch);
    dependency.doXfer(epoch);
    sink.doXfer(epoch);
  }

  ASSERT_EQ(sink.received().size(), 4u);
  EXPECT_EQ(sink.received()[0].sequence, 2u);
  EXPECT_EQ(sink.received()[1].sequence, 0u);
  EXPECT_EQ(sink.received()[2].sequence, 1u);
  EXPECT_EQ(sink.received()[3].sequence, 3u);
}

TEST(QueueBlocksTest, DependencyRejectsZeroExecutionCost) {
  SimQueue<DependencyValue> input("input", 1, nullptr, 1);
  SimQueue<DependencyValue> output("output", 2, nullptr, 1);
  QueueDependency<DependencyValue, DependencyKey, DependencyPredecessor,
                  DependencyResource, DependencyCost>
      dependency("dependency", 3, nullptr, input, output, 1, 1, 255);
  ASSERT_TRUE(input.proposePush({0, 255, 0, 0}));
  input.doXfer({0, 0});
  dependency.doWork({1, 0});
  EXPECT_EQ(dependency.runtimeFailureCode(), "dependency_nonpositive_cost");
  EXPECT_FALSE(dependency.hasPendingCommit());
  EXPECT_TRUE(output.isEmpty());
}

TEST(QueueBlocksTest, DependencyRejectsOutOfRangeResource) {
  SimQueue<DependencyValue> input("input", 1, nullptr, 1);
  SimQueue<DependencyValue> output("output", 2, nullptr, 1);
  QueueDependency<DependencyValue, DependencyKey, DependencyPredecessor,
                  DependencyResource, DependencyCost>
      dependency("dependency", 3, nullptr, input, output, 1, 1, 255);
  ASSERT_TRUE(input.proposePush({0, 255, 1, 1}));
  input.doXfer({0, 0});
  dependency.doWork({1, 0});
  EXPECT_EQ(dependency.runtimeFailureCode(),
            "dependency_resource_out_of_range");
  EXPECT_FALSE(dependency.hasPendingCommit());
  EXPECT_TRUE(output.isEmpty());
}

TEST(QueueBlocksTest, CreditWindowCompletesParallelTokensAndReturnsSlots) {
  SimQueue<DependencyValue> input("input", 1, nullptr, 4);
  SimQueue<DependencyValue> output("output", 2, nullptr, 4);
  QueueCredit<DependencyValue, DependencyCost> credit("credit", 3, nullptr,
                                                      input, output, 2);
  QueueSink<DependencyValue> sink("sink", 4, nullptr, output);
  ASSERT_TRUE(input.proposePush({0, 255, 0, 4}));
  ASSERT_TRUE(input.proposePush({1, 255, 0, 1}));
  ASSERT_TRUE(input.proposePush({2, 255, 0, 1}));
  input.doXfer({0, 0});

  for (uint64_t tick = 1; tick < 12; ++tick) {
    const Epoch epoch{tick, 0};
    credit.doWork(epoch);
    sink.doWork(epoch);
    input.doXfer(epoch);
    output.doXfer(epoch);
    credit.doXfer(epoch);
    sink.doXfer(epoch);
  }

  ASSERT_EQ(sink.received().size(), 3u);
  EXPECT_EQ(sink.received()[0].sequence, 1u);
  EXPECT_EQ(sink.received()[1].sequence, 0u);
  EXPECT_EQ(sink.received()[2].sequence, 2u);
  EXPECT_EQ(credit.active(), 0u);
}

TEST(QueueBlocksTest, CreditRejectsZeroCostWithoutConsumingInput) {
  SimQueue<DependencyValue> input("input", 1, nullptr, 1);
  SimQueue<DependencyValue> output("output", 2, nullptr, 1);
  QueueCredit<DependencyValue, DependencyCost> credit("credit", 3, nullptr,
                                                      input, output, 1);
  ASSERT_TRUE(input.proposePush({0, 255, 0, 0}));
  input.doXfer({0, 0});
  credit.doWork({1, 0});
  EXPECT_EQ(credit.runtimeFailureCode(), "credit_nonpositive_cost");
  EXPECT_FALSE(credit.hasPendingCommit());
  EXPECT_EQ(input.committedSize(), 1u);
  EXPECT_TRUE(output.isEmpty());
}

TEST(QueueBlocksTest, MemoryReturnsOldDataAndCommitsWriteAtXfer) {
  SimQueue<MemoryRequest> input("input", 1, nullptr, 2);
  SimQueue<MemoryRequest> output("output", 2, nullptr, 2);
  QueueMemory<MemoryRequest, uint16_t, MemoryAddress, MemoryWrite,
              MemoryWriteData, MemoryResponse>
      memory("memory", 3, nullptr, input, output, 16);
  QueueSink<MemoryRequest> sink("sink", 4, nullptr, output);
  ASSERT_TRUE(input.proposePush({3, true, 42}));
  ASSERT_TRUE(input.proposePush({3, false, 0}));
  input.doXfer({0, 0});

  for (uint64_t tick = 1; tick < 8; ++tick) {
    const Epoch epoch{tick, 0};
    memory.doWork(epoch);
    sink.doWork(epoch);
    input.doXfer(epoch);
    output.doXfer(epoch);
    memory.doXfer(epoch);
    sink.doXfer(epoch);
  }

  ASSERT_EQ(sink.received().size(), 2u);
  EXPECT_EQ(sink.received()[0].data, 0u);
  EXPECT_EQ(sink.received()[1].data, 42u);
  EXPECT_EQ(memory.at(3), 42u);
}

TEST(QueueBlocksTest, MemoryRejectsOutOfRangeAddress) {
  SimQueue<MemoryRequest> input("input", 1, nullptr, 1);
  SimQueue<MemoryRequest> output("output", 2, nullptr, 1);
  QueueMemory<MemoryRequest, uint16_t, MemoryAddress, MemoryWrite,
              MemoryWriteData, MemoryResponse>
      memory("memory", 3, nullptr, input, output, 4);
  ASSERT_TRUE(input.proposePush({4, false, 0}));
  input.doXfer({0, 0});
  memory.doWork({1, 0});
  EXPECT_EQ(memory.runtimeFailureCode(), "memory_address_out_of_range");
  EXPECT_FALSE(memory.hasPendingCommit());
  EXPECT_TRUE(output.isEmpty());
}

TEST(QueueBlocksTest, SharedMemoryUsesPriorityAndBlocksUntilResponseAccepted) {
  SimQueue<MemoryRequest> input0("input0", 1, nullptr, 2);
  SimQueue<MemoryRequest> input1("input1", 2, nullptr, 2);
  SimQueue<MemoryRequest> output0("output0", 3, nullptr, 1);
  SimQueue<MemoryRequest> output1("output1", 4, nullptr, 1);
  QueueMemoryArbiter<MemoryRequest, uint16_t, 2, SharedMemoryAddress,
                     SharedMemoryWrite, SharedMemoryWriteData,
                     SharedMemoryResponse>
      memory("memory", 5, nullptr, {&input0, &input1}, {&output0, &output1},
             16);
  ASSERT_TRUE(input0.proposePush({3, true, 42}));
  ASSERT_TRUE(input1.proposePush({3, false, 0}));
  ASSERT_TRUE(output0.proposePush({0, false, 99}));
  input0.doXfer({0, 0});
  input1.doXfer({0, 0});
  output0.doXfer({0, 0});

  memory.doWork({1, 0});
  input0.doXfer({1, 0});
  input1.doXfer({1, 0});
  memory.doXfer({1, 0});
  EXPECT_TRUE(memory.busy());
  EXPECT_EQ(memory.selectedEndpoint(), 0u);
  EXPECT_EQ(input0.committedSize(), 0u);
  EXPECT_EQ(input1.committedSize(), 1u);
  EXPECT_EQ(memory.at(3), 42u);

  memory.doWork({2, 0});
  EXPECT_FALSE(memory.hasPendingCommit());
  ASSERT_TRUE(output0.proposePop());
  output0.doXfer({2, 0});
  memory.doWork({2, 1});
  output0.doXfer({2, 1});
  memory.doXfer({2, 1});
  EXPECT_FALSE(memory.busy());
  memory.doWork({2, 1});
  EXPECT_EQ(input1.committedSize(), 1u);

  memory.doWork({3, 0});
  input1.doXfer({3, 0});
  memory.doXfer({3, 0});
  EXPECT_EQ(input1.committedSize(), 0u);
}

TEST(QueueBlocksTest,
     SharedMemoryLatencyDelaysResponseAndBackpressuresRequests) {
  SimQueue<MemoryRequest> input("input", 1, nullptr, 2);
  SimQueue<MemoryRequest> output("output", 2, nullptr, 1);
  QueueMemoryArbiter<MemoryRequest, uint16_t, 1, SharedMemoryAddress,
                     SharedMemoryWrite, SharedMemoryWriteData,
                     SharedMemoryResponse>
      memory("memory", 3, nullptr, {&input}, {&output}, 16, 0, 3);
  EXPECT_EQ(memory.latency(), 3u);
  ASSERT_TRUE(input.proposePush({3, false, 0}));
  input.doXfer({0, 0});

  memory.doWork({1, 0});
  input.doXfer({1, 0});
  memory.doXfer({1, 0});
  ASSERT_TRUE(memory.busy());
  ASSERT_TRUE(input.proposePush({7, false, 0}));
  input.doXfer({2, 0});

  for (uint64_t tick : {2, 3}) {
    memory.doWork({tick, 0});
    EXPECT_TRUE(memory.hasPendingCommit());
    EXPECT_EQ(input.committedSize(), 1u);
    EXPECT_TRUE(output.isEmpty());
    memory.doXfer({tick, 0});
  }

  memory.doWork({4, 0});
  EXPECT_TRUE(memory.hasPendingCommit());
  output.doXfer({4, 0});
  memory.doXfer({4, 0});
  EXPECT_FALSE(memory.busy());
  EXPECT_EQ(input.committedSize(), 1u);
  EXPECT_EQ(output.committedSize(), 1u);

  memory.doWork({4, 0});
  EXPECT_FALSE(memory.hasPendingCommit());
  memory.doWork({5, 0});
  input.doXfer({5, 0});
  memory.doXfer({5, 0});
  EXPECT_TRUE(memory.busy());
  EXPECT_EQ(input.committedSize(), 0u);
}

TEST(QueueBlocksTest, SharedMemoryRejectsZeroLatency) {
  SimQueue<MemoryRequest> input("input", 1, nullptr, 1);
  SimQueue<MemoryRequest> output("output", 2, nullptr, 1);
  EXPECT_THROW((QueueMemoryArbiter<MemoryRequest, uint16_t, 1,
                                   SharedMemoryAddress, SharedMemoryWrite,
                                   SharedMemoryWriteData, SharedMemoryResponse>(
                   "memory", 3, nullptr, {&input}, {&output}, 16, 0, 0)),
               std::invalid_argument);
}

TEST(QueueBlocksTest, QueueLatencyDelaysVisibilityButReservesCapacity) {
  SimQueue<int> queue("queue", 1, nullptr, 1,
                      std::numeric_limits<size_t>::max(), nullptr, 3);
  EXPECT_EQ(queue.latency(), 3u);
  ASSERT_TRUE(queue.proposePush(5));
  queue.doXfer({0, 0});
  EXPECT_TRUE(queue.isEmpty());
  EXPECT_TRUE(queue.isFull());
  queue.doXfer({1, 0});
  EXPECT_TRUE(queue.isEmpty());
  queue.doXfer({2, 0});
  ASSERT_NE(queue.peek(), nullptr);
  EXPECT_EQ(*queue.peek(), 5);
}

TEST(QueueBlocksTest, SinkConsumesAtWorkAndPublishesAtXfer) {
  SimQueue<int> input("input", 1, nullptr, 1);
  QueueSink<int> sink("sink", 2, nullptr, input);
  ASSERT_TRUE(input.proposePush(13));
  input.doXfer({0, 0});

  sink.doWork({1, 0});
  EXPECT_TRUE(sink.received().empty());
  input.doXfer({1, 0});
  sink.doXfer({1, 0});
  ASSERT_EQ(sink.received().size(), 1u);
  EXPECT_EQ(sink.received().front(), 13);
}

TEST(QueueBlocksTest, ObserveCommitsWithoutConsumingOrBackpressure) {
  SimQueue<int> input("input", 1, nullptr, 2);
  QueueObserve<int> observe("observe", 2, nullptr, input);
  ASSERT_TRUE(input.proposePush(13));
  input.doXfer({0, 0});
  observe.doWork({1, 0});
  EXPECT_EQ(input.committedSize(), 1u);
  EXPECT_TRUE(observe.observed().empty());
  observe.doXfer({1, 0});
  ASSERT_EQ(observe.observed().size(), 1u);
  EXPECT_EQ(observe.observed().front(), 13);
  observe.doWork({2, 0});
  observe.doXfer({2, 0});
  EXPECT_EQ(observe.observed().size(), 1u);
  EXPECT_EQ(input.committedSize(), 1u);
}

TEST(QueueBlocksTest, ExpectChecksHeadWithoutConsumingIt) {
  SimQueue<int> input("input", 1, nullptr, 2);
  QueueExpect<int, Positive> expect("expect", 2, nullptr, input,
                                    "must be positive");
  ASSERT_TRUE(input.proposePush(7));
  input.doXfer({0, 0});
  expect.doWork({1, 0});
  EXPECT_TRUE(expect.hasPendingCommit());
  expect.doXfer({1, 0});
  EXPECT_EQ(input.committedSize(), 1u);
  EXPECT_TRUE(expect.runtimeFailureCode().empty());
  EXPECT_EQ(expect.message(), "must be positive");
}

TEST(QueueBlocksTest, ExpectReportsPredicateFailure) {
  SimQueue<int> input("input", 1, nullptr, 1);
  QueueExpect<int, Positive> expect("expect", 2, nullptr, input,
                                    "must be positive");
  ASSERT_TRUE(input.proposePush(-1));
  input.doXfer({0, 0});
  expect.doWork({1, 0});
  EXPECT_EQ(expect.runtimeFailureCode(), "expectation_failed");
  EXPECT_FALSE(expect.hasPendingCommit());
  EXPECT_EQ(input.committedSize(), 1u);
}

TEST(QueueBlocksTest, BroadcastWaitsForEveryOutput) {
  SimQueue<int> input("input", 1, nullptr, 1);
  SimQueue<int> left("left", 2, nullptr, 1);
  SimQueue<int> right("right", 3, nullptr, 1);
  QueueBroadcast<int, 2> broadcast("broadcast", 4, nullptr, input,
                                   {&left, &right});
  ASSERT_TRUE(input.proposePush(9));
  ASSERT_TRUE(right.proposePush(4));
  input.doXfer({0, 0});
  right.doXfer({0, 0});
  broadcast.doWork({1, 0});
  input.doXfer({1, 0});
  left.doXfer({1, 0});
  ASSERT_NE(input.peek(), nullptr);
  EXPECT_TRUE(left.isEmpty());
}

TEST(QueueBlocksTest, ForkDeliversOutputsIndependentlyBeforeInputPop) {
  SimQueue<int> input("input", 1, nullptr, 1);
  SimQueue<int> left("left", 2, nullptr, 1);
  SimQueue<int> right("right", 3, nullptr, 1);
  QueueFork<int, 2> fork("fork", 4, nullptr, input, {&left, &right});
  ASSERT_TRUE(input.proposePush(9));
  ASSERT_TRUE(right.proposePush(4));
  input.doXfer({0, 0});
  right.doXfer({0, 0});

  fork.doWork({1, 0});
  input.doXfer({1, 0});
  left.doXfer({1, 0});
  right.doXfer({1, 0});
  fork.doXfer({1, 0});
  ASSERT_NE(input.peek(), nullptr);
  ASSERT_NE(left.peek(), nullptr);
  EXPECT_EQ(*left.peek(), 9);
  EXPECT_EQ(*right.peek(), 4);

  right.proposePop();
  right.doXfer({2, 0});
  fork.doWork({2, 0});
  input.doXfer({2, 0});
  left.doXfer({2, 0});
  right.doXfer({2, 0});
  fork.doXfer({2, 0});
  EXPECT_TRUE(input.isEmpty());
  EXPECT_EQ(left.committedSize(), 1u);
  ASSERT_NE(right.peek(), nullptr);
  EXPECT_EQ(*right.peek(), 9);
}

TEST(QueueBlocksTest, RouteSelectsExactlyOneOutput) {
  SimQueue<int> input("input", 1, nullptr, 1);
  SimQueue<int> even("even", 2, nullptr, 1);
  SimQueue<int> odd("odd", 3, nullptr, 1);
  QueueRoute<int, 2, SelectParity> route("route", 4, nullptr, input,
                                         {&even, &odd});
  ASSERT_TRUE(input.proposePush(7));
  input.doXfer({0, 0});
  route.doWork({1, 0});
  input.doXfer({1, 0});
  even.doXfer({1, 0});
  odd.doXfer({1, 0});
  EXPECT_TRUE(even.isEmpty());
  ASSERT_NE(odd.peek(), nullptr);
  EXPECT_EQ(*odd.peek(), 7);
}

TEST(QueueBlocksTest, SelectConsumesControlAndChosenInputOnly) {
  SimQueue<int> control("control", 1, nullptr, 1);
  SimQueue<int> left("left", 2, nullptr, 1);
  SimQueue<int> right("right", 3, nullptr, 1);
  SimQueue<int> output("output", 4, nullptr, 1);
  QueueSelect<int, int, 2, SelectIndex> select("select", 5, nullptr, control,
                                               {&left, &right}, output);
  ASSERT_TRUE(control.proposePush(1));
  ASSERT_TRUE(left.proposePush(10));
  ASSERT_TRUE(right.proposePush(20));
  control.doXfer({0, 0});
  left.doXfer({0, 0});
  right.doXfer({0, 0});
  select.doWork({1, 0});
  control.doXfer({1, 0});
  left.doXfer({1, 0});
  right.doXfer({1, 0});
  output.doXfer({1, 0});
  select.doXfer({1, 0});
  EXPECT_TRUE(control.isEmpty());
  ASSERT_NE(left.peek(), nullptr);
  EXPECT_EQ(*left.peek(), 10);
  EXPECT_TRUE(right.isEmpty());
  ASSERT_NE(output.peek(), nullptr);
  EXPECT_EQ(*output.peek(), 20);
}

TEST(QueueBlocksTest, SelectRejectsOutOfRangeSelector) {
  SimQueue<int> control("control", 1, nullptr, 1);
  SimQueue<int> left("left", 2, nullptr, 1);
  SimQueue<int> right("right", 3, nullptr, 1);
  SimQueue<int> output("output", 4, nullptr, 1);
  QueueSelect<int, int, 2, SelectIndex> select("select", 5, nullptr, control,
                                               {&left, &right}, output);
  ASSERT_TRUE(control.proposePush(2));
  control.doXfer({0, 0});
  select.doWork({1, 0});
  EXPECT_EQ(select.runtimeFailureCode(), "select_selector_out_of_range");
  EXPECT_FALSE(select.hasPendingCommit());
}

TEST(QueueBlocksTest, MergeRoundRobinIgnoresWorkInsertionOrder) {
  SimQueue<int> left("left", 1, nullptr, 2);
  SimQueue<int> right("right", 2, nullptr, 2);
  SimQueue<int> output("output", 3, nullptr, 2);
  QueueMerge<int, 2> merge("merge", 4, nullptr, {&left, &right}, output);
  ASSERT_TRUE(left.proposePush(10));
  ASSERT_TRUE(right.proposePush(20));
  left.doXfer({0, 0});
  right.doXfer({0, 0});
  merge.doWork({1, 0});
  left.doXfer({1, 0});
  right.doXfer({1, 0});
  output.doXfer({1, 0});
  merge.doXfer({1, 0});
  ASSERT_NE(output.peek(), nullptr);
  EXPECT_EQ(*output.peek(), 10);
  output.proposePop();
  output.doXfer({2, 0});
  merge.doWork({2, 0});
  right.doXfer({2, 0});
  output.doXfer({2, 0});
  ASSERT_NE(output.peek(), nullptr);
  EXPECT_EQ(*output.peek(), 20);
}

TEST(QueueBlocksTest, FeedbackUsesParentOwnedStateQueue) {
  using State = FeedbackToken<int>;
  SimQueue<int> input("input", 1, nullptr, 1);
  SimQueue<State> feedback("feedback", 2, nullptr, 1);
  SimQueue<int> output("output", 3, nullptr, 1);
  QueueFeedback<int, Decrement, Positive> loop("feedback_block", 4, nullptr,
                                               input, feedback, output, 8);
  ASSERT_TRUE(input.proposePush(3));
  input.doXfer({0, 0});
  for (uint64_t tick = 1; tick <= 4; ++tick) {
    loop.doWork({tick, 0});
    input.doXfer({tick, 0});
    feedback.doXfer({tick, 0});
    output.doXfer({tick, 0});
    loop.doXfer({tick, 0});
  }
  ASSERT_NE(output.peek(), nullptr);
  EXPECT_EQ(*output.peek(), 0);
}

} // namespace
} // namespace gfsim
