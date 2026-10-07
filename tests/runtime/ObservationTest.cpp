#include "gfsim/ObservationSlot.h"
#include "gfsim/SimModule.h"
#include "gfsim/SimSystem.h"
#include "gtest/gtest.h"

#include <array>
#include <atomic>
#include <cstdlib>
#include <new>
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
                                        gfsim::ObservationKind kind) {
  return {.stableOrdinal = ordinal,
          .ownerKey = owner,
          .registrationKey = registration,
          .siteKey = site,
          .kind = kind};
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
        plans_(std::move(plans)), initial_(initial), current_(initial), next_(initial) {}

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
    if (resetPending_) current_ = next_;
    resetPending_ = false;
    workSlotsReady_ = false;
  }
  void Reset() noexcept override { next_ = initial_; resetPending_ = true; }
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

TEST(ObservationTest, StageCheckAndXferDoNotAllocate) {
  const std::array descriptors{
      descriptor(0, 1, 1, 1, gfsim::ObservationKind::Event)};
  gfsim::ObservationSlots slots;
  ASSERT_TRUE(slots.Configure(descriptors, 1));
  const size_t before = allocationCount.load(std::memory_order_relaxed);
  trackAllocations.store(true, std::memory_order_relaxed);
  bool staged = slots.Stage(0, gfsim::SlotValue::Unsigned(4), true, true, 0);
  bool checked = slots.Check();
  slots.Xfer(1);
  trackAllocations.store(false, std::memory_order_relaxed);
  EXPECT_TRUE(staged);
  EXPECT_TRUE(checked);
  EXPECT_EQ(allocationCount.load(std::memory_order_relaxed), before);
}

} // namespace
