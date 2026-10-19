#include "gfsim/ObservationSlot.h"
#include "gfsim/SimModule.h"
#include "gfsim/SimSystem.h"
#include "gtest/gtest.h"

#include <array>
#include <atomic>
#include <cstdlib>
#include <functional>
#include <new>
#include <utility>
#include <vector>

namespace {
std::atomic<bool> trackAllocations{false};
std::atomic<size_t> allocationCount{0};
} // namespace

void *operator new(std::size_t size) {
  if (trackAllocations.load(std::memory_order_relaxed))
    allocationCount.fetch_add(1, std::memory_order_relaxed);
  if (void *storage = std::malloc(size))
    return storage;
  throw std::bad_alloc();
}
void operator delete(void *storage) noexcept { std::free(storage); }
void operator delete(void *storage, std::size_t) noexcept {
  std::free(storage);
}

namespace {

gfsim::ObservationDescriptor descriptor(uint32_t ordinal, uint64_t owner,
                                        uint64_t registration, uint64_t site,
                                        gfsim::ObservationKind kind,
                                        uint32_t groupIndex = 0,
                                        uint32_t groupSize = 1) {
  return {.stableOrdinal = ordinal,
          .ownerKey = owner,
          .registrationKey = registration,
          .siteKey = site,
          .kind = kind,
          .groupIndex = groupIndex,
          .groupSize = groupSize};
}

struct StagePlan {
  size_t slot;
  gfsim::SlotValue value;
};

template <typename T>
concept HasPublicCheck = requires(T &module) { module.Check(); };
template <typename T>
concept HasPublicDrive = requires(T &module) { module.Drive(true); };
static_assert(!HasPublicCheck<gfsim::SimModule>);
static_assert(!HasPublicDrive<gfsim::SimModule>);

class SlotModule final : public gfsim::SimModule {
public:
  SlotModule(std::string name, gfsim::ObservationSlots &slots,
             std::vector<StagePlan> plans, int initial = 3)
      : gfsim::SimModule(std::move(name)), slots_(slots),
        plans_(std::move(plans)), initial_(initial), current_(initial),
        next_(initial) {}

  void Build() override {}
  void Work() override {
    ++workCalls;
    workSlotsReady_ = true;
    observed = current_;
    next_ = nextValue;
    evaluationEpoch_ = system->cycle();
    for (const StagePlan &plan : plans_)
      slots_.Stage(plan.slot,
                   stageCurrentValue ? gfsim::SlotValue::Unsigned(
                                           static_cast<uint64_t>(observed))
                                     : plan.value,
                   true, true, evaluationEpoch_);
  }
  void Xfer() noexcept override {
    if (workSlotsReady_) {
      ++xferCalls;
      current_ = next_;
    }
    if (resetPending_)
      current_ = next_;
    resetPending_ = false;
    workSlotsReady_ = false;
  }
  void Reset() noexcept override {
    next_ = initial_;
    resetPending_ = true;
  }
  void ReportStat() override {}
  bool HasWork() const noexcept override { return true; }

  int value() const { return current_; }

  gfsim::SimSystem *system = nullptr;
  int nextValue = 7;
  int observed = -1;
  bool stageCurrentValue = false;
  unsigned workCalls = 0, xferCalls = 0, discardCalls = 0;

private:
  void DiscardNext() noexcept override {
    ++discardCalls;
    next_ = current_;
    resetPending_ = false;
    workSlotsReady_ = false;
  }
  gfsim::ObservationSlots &slots_;
  std::vector<StagePlan> plans_;
  const int initial_;
  int current_, next_;
  bool resetPending_ = false;
  uint64_t evaluationEpoch_ = 0;
  bool workSlotsReady_ = false;
};

class ObservationSystem final : public gfsim::SimSystem {
public:
  explicit ObservationSystem(gfsim::ObservationSlots &slots) : slots_(slots) {}
  bool allowPrecommit = true;

private:
  bool Precheck() noexcept override { return allowPrecommit; }
  gfsim::ObservationSlots &slots_;
};

TEST(ObservationTest, ConfigureRequiresStableOrderAndUniqueDescriptors) {
  const std::array sorted{
      descriptor(3, 1, 1, 1, gfsim::ObservationKind::Event),
      descriptor(7, 1, 1, 2, gfsim::ObservationKind::Gauge),
      descriptor(9, 2, 1, 1, gfsim::ObservationKind::Event),
  };
  gfsim::ObservationSlots accepted;
  EXPECT_TRUE(accepted.Configure(sorted, 2));
  EXPECT_FALSE(accepted.Configure(sorted, 2));
  EXPECT_EQ(accepted.Descriptors().size(), 3u);

  gfsim::ObservationSlots reversed;
  const std::array unsorted{sorted[1], sorted[0]};
  EXPECT_FALSE(reversed.Configure(unsorted, 1));

  gfsim::ObservationSlots duplicateOrdinal;
  const std::array duplicate{
      sorted[0], descriptor(3, 1, 1, 3, gfsim::ObservationKind::Event)};
  EXPECT_FALSE(duplicateOrdinal.Configure(duplicate, 2));
}

TEST(ObservationTest,
     ConfigureRejectsMalformedGroupsKindsAndMissingAppendedFieldsAtomically) {
  const std::vector<gfsim::ObservationDescriptor> valid{
      descriptor(10, 1, 2, 3, gfsim::ObservationKind::Event, 0, 2),
      descriptor(11, 1, 2, 3, gfsim::ObservationKind::Event, 1, 2),
      descriptor(20, 2, 4, 5, gfsim::ObservationKind::Gauge, 0, 1),
  };
  const std::vector<
      std::function<void(std::vector<gfsim::ObservationDescriptor> &)>>
      mutations{
          [](auto &rows) { rows[0].groupSize = 0; },
          [](auto &rows) { rows[0].groupIndex = 1; },
          [](auto &rows) { rows[1].groupIndex = 0; },
          [](auto &rows) { rows[1].groupIndex = 2; },
          [](auto &rows) { rows[1].groupSize = 3; },
          [](auto &rows) { rows[1].ownerKey = 9; },
          [](auto &rows) { rows[1].registrationKey = 9; },
          [](auto &rows) { rows[1].siteKey = 9; },
          [](auto &rows) { rows[1].kind = gfsim::ObservationKind::Gauge; },
          [](auto &rows) {
            rows.pop_back();
            rows.pop_back();
          },
          [](auto &rows) {
            rows[0].kind = static_cast<gfsim::ObservationKind>(99);
            rows[0].groupSize = 1;
            rows.erase(rows.begin() + 1);
          },
          [](auto &rows) {
            rows[0].kind = gfsim::ObservationKind::Gauge;
            rows[1].kind = gfsim::ObservationKind::Gauge;
          },
      };
  for (std::size_t index = 0; index < mutations.size(); ++index) {
    SCOPED_TRACE(index);
    auto malformed = valid;
    mutations[index](malformed);
    gfsim::ObservationSlots slots;
    EXPECT_FALSE(slots.Configure(malformed, 4));
    EXPECT_TRUE(slots.Configure(valid, 4))
        << "failed Configure must not partially mutate the slot table";
  }

  const std::array omittedAppendedFields{
      gfsim::ObservationDescriptor{1, 1, 1, 1, gfsim::ObservationKind::Event}};
  gfsim::ObservationSlots omitted;
  EXPECT_FALSE(omitted.Configure(omittedAppendedFields, 1));
}

TEST(ObservationTest, GroupCheckAndPrecommitImplementTheCompleteTruthTable) {
  const std::array descriptors{
      descriptor(0, 1, 1, 1, gfsim::ObservationKind::Event, 0, 2),
      descriptor(1, 1, 1, 1, gfsim::ObservationKind::Event, 1, 2),
  };
  auto configured = [&] {
    gfsim::ObservationSlots slots;
    EXPECT_TRUE(slots.Configure(descriptors, 2));
    return slots;
  };

  {
    auto slots = configured();
    EXPECT_TRUE(slots.Check());
    EXPECT_TRUE(slots.Precommit(0));
  }
  {
    auto slots = configured();
    ASSERT_TRUE(slots.Stage(0, gfsim::SlotValue::Unsigned(1), true, false, 0));
    EXPECT_FALSE(slots.Check());
    EXPECT_FALSE(slots.Precommit(0));
  }
  {
    auto slots = configured();
    ASSERT_TRUE(slots.Stage(0, gfsim::SlotValue::Unsigned(1), true, true, 1));
    ASSERT_TRUE(slots.Stage(1, gfsim::SlotValue::Unsigned(2), true, true, 1));
    EXPECT_TRUE(slots.Check());
    EXPECT_FALSE(slots.Precommit(0));
  }
  {
    auto slots = configured();
    ASSERT_TRUE(slots.Stage(0, gfsim::SlotValue::Unsigned(1), true, false, 0));
    ASSERT_TRUE(slots.Stage(1, gfsim::SlotValue::Unsigned(2), true, true, 0));
    EXPECT_FALSE(slots.Check());
    EXPECT_FALSE(slots.Precommit(0));
  }
  {
    auto slots = configured();
    ASSERT_TRUE(slots.Stage(0, gfsim::SlotValue::Unsigned(1), true, false, 0));
    ASSERT_TRUE(slots.Stage(1, gfsim::SlotValue::Signed(-2), false, false, 0));
    EXPECT_TRUE(slots.Check());
    EXPECT_TRUE(slots.Precommit(0));
    slots.Xfer(1);
    EXPECT_TRUE(slots.Events().empty());
  }
  {
    auto slots = configured();
    ASSERT_TRUE(slots.Stage(0, gfsim::SlotValue::Unsigned(1), false, true, 0));
    ASSERT_TRUE(slots.Stage(1, gfsim::SlotValue::Signed(-2), false, true, 0));
    EXPECT_TRUE(slots.Check());
    EXPECT_TRUE(slots.Precommit(0));
    slots.Xfer(1);
    EXPECT_TRUE(slots.Events().empty());
  }
  {
    auto slots = configured();
    ASSERT_TRUE(slots.Stage(0, gfsim::SlotValue::Unsigned(1), true, true, 0));
    ASSERT_TRUE(slots.Stage(1, gfsim::SlotValue::Signed(-2), false, true, 0));
    EXPECT_FALSE(slots.Check());
    EXPECT_FALSE(slots.Precommit(0));
  }
}

TEST(ObservationTest, ActiveGroupKindsGaugeContractAndCapacityFailClosed) {
  const std::array eventDescriptors{
      descriptor(0, 1, 1, 1, gfsim::ObservationKind::Event, 0, 3),
      descriptor(1, 1, 1, 1, gfsim::ObservationKind::Event, 1, 3),
      descriptor(2, 1, 1, 1, gfsim::ObservationKind::Event, 2, 3),
  };
  gfsim::ObservationSlots accepted;
  ASSERT_TRUE(accepted.Configure(eventDescriptors, 3));
  ASSERT_TRUE(accepted.Stage(0, gfsim::SlotValue::Bool(true), true, true, 0));
  ASSERT_TRUE(accepted.Stage(1, gfsim::SlotValue::Signed(-7), true, true, 0));
  ASSERT_TRUE(accepted.Stage(2, gfsim::SlotValue::Unsigned(9), true, true, 0));
  EXPECT_TRUE(accepted.Check());
  EXPECT_TRUE(accepted.Precommit(0));

  gfsim::ObservationSlots invalidEventKind;
  ASSERT_TRUE(invalidEventKind.Configure(eventDescriptors, 3));
  auto invalidValue = gfsim::SlotValue::Unsigned(1);
  invalidValue.kind = static_cast<gfsim::SlotValueKind>(99);
  ASSERT_TRUE(invalidEventKind.Stage(0, invalidValue, true, true, 0));
  ASSERT_TRUE(
      invalidEventKind.Stage(1, gfsim::SlotValue::Signed(-7), true, true, 0));
  ASSERT_TRUE(
      invalidEventKind.Stage(2, gfsim::SlotValue::Unsigned(9), true, true, 0));
  EXPECT_FALSE(invalidEventKind.Check());
  EXPECT_FALSE(invalidEventKind.Precommit(0));

  gfsim::ObservationSlots capacity;
  ASSERT_TRUE(capacity.Configure(eventDescriptors, 2));
  ASSERT_TRUE(capacity.Stage(0, gfsim::SlotValue::Bool(true), true, true, 0));
  ASSERT_TRUE(capacity.Stage(1, gfsim::SlotValue::Signed(-7), true, true, 0));
  ASSERT_TRUE(capacity.Stage(2, gfsim::SlotValue::Unsigned(9), true, true, 0));
  EXPECT_FALSE(capacity.Check());
  EXPECT_FALSE(capacity.Precommit(0));

  const std::array gaugeDescriptor{
      descriptor(3, 2, 2, 2, gfsim::ObservationKind::Gauge, 0, 1)};
  gfsim::ObservationSlots gauge;
  ASSERT_TRUE(gauge.Configure(gaugeDescriptor, 0));
  ASSERT_TRUE(gauge.Stage(0, gfsim::SlotValue::Unsigned(17), true, true, 0));
  EXPECT_TRUE(gauge.Check());
  EXPECT_TRUE(gauge.Precommit(0));
  gauge.DiscardNext();
  ASSERT_TRUE(gauge.Stage(0, gfsim::SlotValue::Bool(true), true, true, 0));
  EXPECT_FALSE(gauge.Check());
  EXPECT_FALSE(gauge.Precommit(0));
  gauge.DiscardNext();
  ASSERT_TRUE(gauge.Stage(0, gfsim::SlotValue::Signed(-1), true, true, 0));
  EXPECT_FALSE(gauge.Check());
  EXPECT_FALSE(gauge.Precommit(0));
}

TEST(ObservationTest, SystemPublishesOldQInStableDescriptorOrder) {
  const std::array descriptors{
      descriptor(0, 1, 4, 2, gfsim::ObservationKind::Event),
      descriptor(1, 2, 3, 1, gfsim::ObservationKind::Event),
  };
  gfsim::ObservationSlots slots;
  ASSERT_TRUE(slots.Configure(descriptors, 2));
  SlotModule later("later", slots, {{1, gfsim::SlotValue::Unsigned(99)}});
  SlotModule earlier("earlier", slots, {{0, gfsim::SlotValue::Unsigned(88)}});
  later.stageCurrentValue = true;
  earlier.stageCurrentValue = true;
  ObservationSystem system(slots);
  ASSERT_TRUE(system.AttachObservations(slots));
  later.system = &system;
  ASSERT_TRUE(system.AddModule(later));
  earlier.system = &system;
  ASSERT_TRUE(system.AddModule(earlier));
  system.Build();
  system.Reset();
  EXPECT_EQ(system.Step(), gfsim::SimStepResult::Running);

  ASSERT_EQ(slots.Events().size(), 2u);
  EXPECT_EQ(slots.Events()[0].descriptor, descriptors[0]);
  EXPECT_EQ(slots.Events()[1].descriptor, descriptors[1]);
  EXPECT_EQ(slots.Events()[0].value, gfsim::SlotValue::Unsigned(3));
  EXPECT_EQ(slots.Events()[1].value, gfsim::SlotValue::Unsigned(3));
  EXPECT_EQ(slots.Events()[0].epoch, 1u);
  EXPECT_EQ(slots.Events()[1].epoch, 1u);
  EXPECT_EQ(earlier.value(), 7);
  EXPECT_EQ(later.value(), 7);
}

TEST(ObservationTest, EventBatchIsPerEpochAndFailureKeepsLastCommittedBatch) {
  const std::array descriptors{
      descriptor(0, 1, 1, 7, gfsim::ObservationKind::Event)};
  gfsim::ObservationSlots slots;
  ASSERT_TRUE(slots.Configure(descriptors, 1));
  SlotModule module("repeated_site", slots,
                    {{0, gfsim::SlotValue::Unsigned(3)}});
  ObservationSystem system(slots);
  ASSERT_TRUE(system.AttachObservations(slots));
  module.system = &system;
  ASSERT_TRUE(system.AddModule(module));
  system.Build();
  system.Reset();

  EXPECT_EQ(system.Step(), gfsim::SimStepResult::Running);
  ASSERT_EQ(slots.Events().size(), 1u);
  EXPECT_EQ(slots.Events()[0].descriptor, descriptors[0]);
  EXPECT_EQ(slots.Events()[0].epoch, 1u);

  EXPECT_EQ(system.Step(), gfsim::SimStepResult::Running);
  ASSERT_EQ(slots.Events().size(), 1u);
  EXPECT_EQ(slots.Events()[0].descriptor, descriptors[0]);
  EXPECT_EQ(slots.Events()[0].epoch, 2u);

  const auto lastCommittedBatch = slots.Events()[0];
  system.allowPrecommit = false;
  EXPECT_EQ(system.Step(), gfsim::SimStepResult::Failed);
  ASSERT_EQ(slots.Events().size(), 1u);
  EXPECT_EQ(slots.Events()[0], lastCommittedBatch);
}

TEST(ObservationTest, PrecommitFailureDiscardsEventsAndGauges) {
  const std::array descriptors{
      descriptor(0, 1, 1, 1, gfsim::ObservationKind::Event),
      descriptor(1, 1, 1, 2, gfsim::ObservationKind::Gauge),
  };
  gfsim::ObservationSlots slots;
  ASSERT_TRUE(slots.Configure(descriptors, 1));
  SlotModule module(
      "failure", slots,
      {{0, gfsim::SlotValue::Unsigned(5)}, {1, gfsim::SlotValue::Signed(-6)}});
  ObservationSystem system(slots);
  ASSERT_TRUE(system.AttachObservations(slots));
  module.system = &system;
  ASSERT_TRUE(system.AddModule(module));
  system.Build();
  system.Reset();
  EXPECT_EQ(system.Step(), gfsim::SimStepResult::Failed);
  EXPECT_EQ(system.state(), gfsim::SimSystemState::Failed);
  EXPECT_TRUE(slots.Events().empty());
  ASSERT_EQ(slots.Gauges().size(), 1u);
  EXPECT_EQ(slots.Gauges()[0].value, gfsim::SlotValue::Unsigned(0));
  EXPECT_EQ(module.xferCalls, 0u);
  EXPECT_EQ(module.discardCalls, 2u); // Reset cleanup and rejected precommit.
}

TEST(ObservationTest, CapacityOverflowFailsBeforeXfer) {
  const std::array descriptors{
      descriptor(0, 1, 1, 1, gfsim::ObservationKind::Event),
      descriptor(1, 1, 1, 2, gfsim::ObservationKind::Event),
  };
  gfsim::ObservationSlots slots;
  ASSERT_TRUE(slots.Configure(descriptors, 1));
  SlotModule module(
      "overflow", slots,
      {{0, gfsim::SlotValue::Unsigned(1)}, {1, gfsim::SlotValue::Unsigned(2)}});
  ObservationSystem system(slots);
  ASSERT_TRUE(system.AttachObservations(slots));
  module.system = &system;
  ASSERT_TRUE(system.AddModule(module));
  system.Build();
  system.Reset();
  const unsigned resetXfers = module.xferCalls;
  const unsigned resetDiscards = module.discardCalls;
  EXPECT_EQ(system.Step(), gfsim::SimStepResult::Failed);
  EXPECT_EQ(system.state(), gfsim::SimSystemState::Failed);
  EXPECT_EQ(module.xferCalls, resetXfers);
  EXPECT_EQ(module.discardCalls, resetDiscards + 1);
  EXPECT_TRUE(slots.Events().empty());
}

TEST(ObservationTest, GaugeStageDuplicateAndResetContracts) {
  const std::array descriptors{
      descriptor(0, 1, 1, 1, gfsim::ObservationKind::Gauge)};
  gfsim::ObservationSlots slots;
  ASSERT_TRUE(slots.Configure(descriptors, 0));
  EXPECT_TRUE(slots.Stage(0, gfsim::SlotValue::Unsigned(17), true, true, 0));
  EXPECT_FALSE(slots.Stage(0, gfsim::SlotValue::Unsigned(18), true, true, 0));
  EXPECT_TRUE(slots.Check());
  EXPECT_EQ(slots.Gauges()[0].value, gfsim::SlotValue::Unsigned(0));
  slots.Xfer(1);
  EXPECT_EQ(slots.Gauges()[0].value, gfsim::SlotValue::Unsigned(17));
  EXPECT_EQ(slots.Gauges()[0].lastUpdate, 1u);

  EXPECT_TRUE(slots.Stage(0, gfsim::SlotValue::Signed(-1), true, true, 1));
  EXPECT_FALSE(slots.Check());
  slots.DiscardNext();
  slots.Reset();
  EXPECT_EQ(slots.Gauges()[0].value, gfsim::SlotValue::Unsigned(0));
  EXPECT_EQ(slots.Gauges()[0].lastUpdate, 0u);
  EXPECT_TRUE(slots.Events().empty());
}

gfsim::FourState<130> bits4Pattern(bool alternate) {
  gfsim::Bits<130> value{}, known{}, z{};
  for (unsigned bit = 0; bit < 130; ++bit) {
    const auto mask = std::uint64_t{1} << (bit % 64);
    const unsigned word = bit / 64;
    if (bit % 7 == (alternate ? 3u : 1u)) {
      z.setWord(word, z.word(word) | mask);
    } else if (bit % 5 != (alternate ? 2u : 4u)) {
      known.setWord(word, known.word(word) | mask);
      if ((bit + alternate) % 2)
        value.setWord(word, value.word(word) | mask);
    }
  }
  return gfsim::FourState<130>::fromMasks(value, known, z);
}

void expectBits4(const gfsim::SlotValue &actual,
                 const gfsim::FourState<130> &expected) {
  ASSERT_EQ(actual.kind, gfsim::SlotValueKind::Bits4);
  ASSERT_EQ(actual.width, 130u);
  ASSERT_EQ(actual.valueWords.size(), 3u);
  for (unsigned word = 0; word < 3; ++word) {
    EXPECT_EQ(actual.valueWords[word],
              expected.value().word(word) & expected.knownMask().word(word));
    EXPECT_EQ(actual.knownWords[word], expected.knownMask().word(word));
    EXPECT_EQ(actual.zWords[word], expected.zMask().word(word));
  }
}

TEST(ObservationTest,
     Bits4PendingAndCommittedSnapshotsStaySeparateAcrossWorkAndDiscard) {
  const std::array descriptors{
      descriptor(0, 1, 1, 1, gfsim::ObservationKind::Event)};
  auto row = descriptors;
  row[0].payloadWidth = 130;
  gfsim::ObservationSlots slots;
  ASSERT_TRUE(slots.Configure(row, 1));
  const auto first = bits4Pattern(false), second = bits4Pattern(true);

  ASSERT_TRUE(slots.StagePacked(0, first, false, true, true, 0));
  ASSERT_TRUE(slots.Precommit(0));
  slots.Xfer(1);
  ASSERT_EQ(slots.Events().size(), 1u);
  expectBits4(slots.Events()[0].value, first);

  ASSERT_TRUE(slots.StagePacked(0, second, false, true, true, 1));
  // Staging the next Work writes only the pending snapshot.
  expectBits4(slots.Events()[0].value, first);
  slots.DiscardNext();
  expectBits4(slots.Events()[0].value, first);

  ASSERT_TRUE(slots.StagePacked(0, second, false, true, true, 1));
  ASSERT_TRUE(slots.Precommit(1));
  slots.Xfer(2);
  ASSERT_EQ(slots.Events().size(), 1u);
  expectBits4(slots.Events()[0].value, second);
}

TEST(ObservationTest, Bits4CopyAndMoveOwnersRebindBorrowedPlaneViews) {
  const std::array descriptors{
      descriptor(0, 1, 1, 1, gfsim::ObservationKind::Event)};
  auto row = descriptors;
  row[0].payloadWidth = 130;
  const auto pattern = bits4Pattern(false);

  auto pendingCopy = [&] {
    gfsim::ObservationSlots source;
    EXPECT_TRUE(source.Configure(row, 1));
    EXPECT_TRUE(source.StagePacked(0, pattern, false, true, true, 0));
    return gfsim::ObservationSlots(source);
  }();
  ASSERT_TRUE(pendingCopy.Precommit(0));
  pendingCopy.Xfer(1);
  ASSERT_EQ(pendingCopy.Events().size(), 1u);
  expectBits4(pendingCopy.Events()[0].value, pattern);

  gfsim::ObservationSlots committedCopy(pendingCopy);
  const auto replacement = bits4Pattern(true);
  ASSERT_TRUE(
      pendingCopy.StagePacked(0, replacement, false, true, true, 1));
  ASSERT_TRUE(pendingCopy.Precommit(1));
  pendingCopy.Xfer(2);
  expectBits4(pendingCopy.Events()[0].value, replacement);
  // A shallow copied event would now observe replacement through the source's
  // overwritten committed buffer. Free that owner as a second lifetime check.
  expectBits4(committedCopy.Events()[0].value, pattern);
  pendingCopy = gfsim::ObservationSlots{};
  expectBits4(committedCopy.Events()[0].value, pattern);
  gfsim::ObservationSlots moved(std::move(committedCopy));
  ASSERT_EQ(moved.Events().size(), 1u);
  expectBits4(moved.Events()[0].value, pattern);
}

TEST(ObservationTest, MalformedBits4NeverBecomesPendingOrCommitted) {
  const std::array descriptors{
      descriptor(0, 1, 1, 1, gfsim::ObservationKind::Event)};
  auto row = descriptors;
  row[0].payloadWidth = 65;
  gfsim::ObservationSlots slots;
  ASSERT_TRUE(slots.Configure(row, 1));
  std::array<std::uint64_t, 2> value{0, 0}, known{0, 0}, z{0, 0};

  value[0] = 1; // Value under an unknown bit is noncanonical.
  EXPECT_FALSE(slots.Stage(
      0, gfsim::SlotValue::Bits4(65, value, known, z), true, true, 0));
  value[0] = 0;
  known[0] = z[0] = 1; // Known and Z planes overlap.
  EXPECT_FALSE(slots.Stage(
      0, gfsim::SlotValue::Bits4(65, value, known, z), true, true, 0));
  known[0] = z[0] = 0;
  known[1] = 2; // Bit 65 is outside a width-65 payload.
  EXPECT_FALSE(slots.Stage(
      0, gfsim::SlotValue::Bits4(65, value, known, z), true, true, 0));

  EXPECT_TRUE(slots.Check());
  EXPECT_TRUE(slots.Precommit(0));
  slots.Xfer(1);
  EXPECT_TRUE(slots.Events().empty());
}

TEST(ObservationTest, KnownSmallStagePackedKeepsLegacyScalarKinds) {
  const std::array descriptors{
      descriptor(0, 1, 1, 1, gfsim::ObservationKind::Event),
      descriptor(1, 2, 1, 1, gfsim::ObservationKind::Event)};
  auto rows = descriptors;
  rows[0].payloadWidth = 8;
  rows[1].payloadWidth = 1;
  gfsim::ObservationSlots slots;
  ASSERT_TRUE(slots.Configure(rows, 2));
  ASSERT_TRUE(slots.StagePacked(
      0, gfsim::FourState<8>::known(gfsim::Bits<8>{37}), false, true, true, 0));
  ASSERT_TRUE(slots.StagePacked(
      1, gfsim::FourState<1>::known(gfsim::Bits<1>{1}), true, true, true, 0));
  ASSERT_TRUE(slots.Precommit(0));
  slots.Xfer(1);
  ASSERT_EQ(slots.Events().size(), 2u);
  EXPECT_EQ(slots.Events()[0].value, gfsim::SlotValue::Unsigned(37));
  EXPECT_EQ(slots.Events()[1].value, gfsim::SlotValue::Bool(true));
}

TEST(ObservationTest, StageCheckAndXferDoNotAllocate) {
  const std::array descriptors{
      descriptor(0, 1, 1, 1, gfsim::ObservationKind::Event)};
  gfsim::ObservationSlots slots;
  ASSERT_TRUE(slots.Configure(descriptors, 1));
  const size_t before = allocationCount.load(std::memory_order_relaxed);
  trackAllocations.store(true, std::memory_order_relaxed);
  bool staged = slots.Stage(0, gfsim::SlotValue::Unsigned(4), true, true, 0);
  bool checked = slots.Check();
  bool precommitted = slots.Precommit(0);
  slots.Xfer(1);
  trackAllocations.store(false, std::memory_order_relaxed);
  EXPECT_TRUE(staged);
  EXPECT_TRUE(checked);
  EXPECT_TRUE(precommitted);
  EXPECT_EQ(allocationCount.load(std::memory_order_relaxed), before);
}

} // namespace
