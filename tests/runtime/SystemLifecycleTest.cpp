#include "gfsim/SimModule.h"
#include "gfsim/SimSystem.h"
#include "gtest/gtest.h"

#include <array>
#include <atomic>
#include <cstdlib>
#include <new>
#include <stdexcept>
#include <string>
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

template <typename T>
concept HasPublicCheck = requires(T &module) { module.Check(); };
template <typename T>
concept HasPublicDrive = requires(T &module) { module.Drive(true); };
static_assert(!HasPublicCheck<gfsim::SimModule>);
static_assert(!HasPublicDrive<gfsim::SimModule>);

struct WorkSlots {
  int d = 0;
  bool e = false;
  bool checkPasses = true;
  bool immutable = false;
};

struct Counts {
  unsigned build = 0, work = 0, precommit = 0, xfer = 0, cycleXfer = 0;
  unsigned primitiveXfer = 0, writes = 0;
  unsigned discard = 0, reset = 0, report = 0;
  size_t xferAllocations = 0;
};

class ProbeModule final : public gfsim::SimModule {
public:
  explicit ProbeModule(std::string name, int initial = 3)
      : gfsim::SimModule(std::move(name)), initial_(initial), current_(initial), next_(initial) {}

  WorkSlots workSlots;
  Counts counts;
  bool hasWork = true;
  bool throwOnBuild = false;
  bool checkPasses = true;
  bool writeProposal = true;
  int nextValue = 7;
  int observed = -1;
  unsigned qReadsInWork = 0;
  unsigned qReadsOutsideWork = 0;

  int value() const { return current_; }

private:
  void Build() override {
    ++counts.build;
    if (throwOnBuild)
      throw std::runtime_error("injected partial Build failure");
  }
  void Work() override {
    ++counts.work;
    observed = current_;
    ++qReadsInWork;
    next_ = writeProposal ? nextValue : current_;
    workSlots = {.d = nextValue,
                 .e = writeProposal,
                 .checkPasses = checkPasses,
                 .immutable = true};
  }
  void Xfer() noexcept override {
    const bool hasCycleSlots = workSlots.immutable;
    if (hasCycleSlots) {
      ++counts.xfer;
      ++counts.cycleXfer;
      const size_t before = allocationCount.load(std::memory_order_relaxed);
      const bool previous =
          trackAllocations.exchange(true, std::memory_order_relaxed);
      ++counts.writes;
      current_ = next_;
      trackAllocations.store(previous, std::memory_order_relaxed);
      counts.xferAllocations +=
          allocationCount.load(std::memory_order_relaxed) - before;
    } else {
      if (resetPending_)
        current_ = next_;
      resetPending_ = false;
    }
    ++counts.primitiveXfer;
    workSlots = {};
  }
  void DiscardNext() noexcept override {
    ++counts.discard;
    workSlots = {};
    next_ = current_;
    resetPending_ = false;
  }
  void Reset() noexcept override {
    ++counts.reset;
    workSlots = {};
    next_ = initial_;
    resetPending_ = true;
  }
  void ReportStat() override { ++counts.report; }
  bool HasWork() const noexcept override { return hasWork; }

  const int initial_;
  int current_;
  int next_;
  bool resetPending_ = false;
};

class ProbeSystem final : public gfsim::SimSystem {
public:
  bool rejectPrecommit = false;
  bool acceptFinalize = true;
  unsigned precommitCalls = 0;
  unsigned finalizeCalls = 0;
  gfsim::SimSystemState stateAtFinalize = gfsim::SimSystemState::Failed;
  std::vector<unsigned> buildCountsAtFinalize;
  std::vector<ProbeModule *> modules;

private:
  bool FinalizeBuild() noexcept override {
    ++finalizeCalls;
    stateAtFinalize = state();
    buildCountsAtFinalize.clear();
    for (ProbeModule *module : modules)
      buildCountsAtFinalize.push_back(module->counts.build);
    return acceptFinalize;
  }

  bool Precheck() noexcept override {
    ++precommitCalls;
    bool accepted = !rejectPrecommit;
    for (ProbeModule *module : modules) {
      ++module->counts.precommit;
      accepted &= module->workSlots.immutable &&
                  module->workSlots.checkPasses;
      // Precommit reads frozen D/E/check slots only. It never rereads Q,
      // mutates a slot, or reruns Work.
    }
    return accepted;
  }
};

void add(gfsim::SimSystem &system, ProbeModule &module) {
  ASSERT_TRUE(system.AddModule(module));
  auto *probeSystem = dynamic_cast<ProbeSystem *>(&system);
  ASSERT_NE(probeSystem, nullptr);
  probeSystem->modules.push_back(&module);
}

TEST(SystemLifecycleTest, SystemOwnsFlatModuleLifecycle) {
  ProbeSystem system;
  ProbeModule root("root"), left("left"), right("right");
  add(system, root);
  add(system, left);
  add(system, right);
  system.Build();
  EXPECT_EQ(system.state(), gfsim::SimSystemState::Built);
  system.Reset();
  for (ProbeModule *module : {&root, &left, &right})
    EXPECT_EQ(module->counts.build, 1u);

  EXPECT_EQ(system.Step(), gfsim::SimStepResult::Running);
  for (ProbeModule *module : {&root, &left, &right}) {
    EXPECT_EQ(module->counts.work, 1u);
    EXPECT_EQ(module->counts.precommit, 1u);
    EXPECT_EQ(module->counts.xfer, 1u);
    EXPECT_EQ(module->counts.primitiveXfer, 2u); // reset transfer plus cycle
  }
  system.ReportStat();
  for (ProbeModule *module : {&root, &left, &right})
    EXPECT_EQ(module->counts.report, 1u);
}

TEST(SystemLifecycleTest, FinalizeBuildRunsOnceAfterEveryModuleBuild) {
  ProbeSystem system;
  ProbeModule root("root"), left("left"), right("right");
  add(system, root);
  add(system, left);
  add(system, right);

  system.Build();
  EXPECT_EQ(system.finalizeCalls, 1u);
  EXPECT_EQ(system.stateAtFinalize, gfsim::SimSystemState::PreBuild);
  EXPECT_EQ(system.buildCountsAtFinalize, (std::vector<unsigned>{1u, 1u, 1u}));
  EXPECT_EQ(system.state(), gfsim::SimSystemState::Built);

  system.Build();
  EXPECT_EQ(system.finalizeCalls, 1u);
  EXPECT_EQ(root.counts.build, 1u);
  EXPECT_EQ(left.counts.build, 1u);
  EXPECT_EQ(right.counts.build, 1u);
}

TEST(SystemLifecycleTest, FailedFinalizeCannotBeRecoveredByReset) {
  ProbeSystem system;
  ProbeModule first("first"), second("second");
  add(system, first);
  add(system, second);
  system.acceptFinalize = false;

  system.Build();
  EXPECT_EQ(system.finalizeCalls, 1u);
  EXPECT_EQ(system.stateAtFinalize, gfsim::SimSystemState::PreBuild);
  EXPECT_EQ(system.buildCountsAtFinalize, (std::vector<unsigned>{1u, 1u}));
  EXPECT_EQ(system.state(), gfsim::SimSystemState::Failed);

  system.Reset();
  EXPECT_EQ(system.state(), gfsim::SimSystemState::Failed);
  EXPECT_EQ(system.finalizeCalls, 1u);
  for (ProbeModule *module : {&first, &second}) {
    EXPECT_EQ(module->counts.reset, 0u);
    EXPECT_EQ(module->counts.xfer, 0u);
    EXPECT_EQ(module->counts.primitiveXfer, 0u);
  }
}

TEST(SystemLifecycleTest, WorkReadsOldQAndPrecommitConsumesFrozenSlots) {
  ProbeSystem system;
  ProbeModule producer("producer"), observer("observer");
  observer.writeProposal = false;
  add(system, producer);
  add(system, observer);
  system.Build();
  system.Reset();
  EXPECT_EQ(system.Step(), gfsim::SimStepResult::Running);
  EXPECT_EQ(observer.observed, 3);
  EXPECT_EQ(producer.value(), 7);
  EXPECT_EQ(system.cycle(), 1u);
  EXPECT_EQ(system.precommitCalls, 1u);
  EXPECT_EQ(producer.qReadsInWork, 1u);
  EXPECT_EQ(producer.qReadsOutsideWork, 0u);
  EXPECT_EQ(producer.counts.writes, 1u);
  EXPECT_FALSE(producer.workSlots.immutable);
}

TEST(SystemLifecycleTest, RejectedPrecommitDiscardsAllSlotsWithoutTransfer) {
  ProbeSystem system;
  ProbeModule early("early"), failing("failing"), sibling("sibling");
  failing.checkPasses = false;
  add(system, early);
  add(system, failing);
  add(system, sibling);
  system.Build();
  system.Reset();
  const unsigned resetDiscards = early.counts.discard;
  EXPECT_EQ(system.Step(), gfsim::SimStepResult::Failed);
  EXPECT_EQ(system.state(), gfsim::SimSystemState::Failed);
  EXPECT_EQ(system.cycle(), 0u);
  for (ProbeModule *module : {&early, &failing, &sibling}) {
    EXPECT_EQ(module->counts.work, 1u);
    EXPECT_EQ(module->counts.xfer, 0u);
    EXPECT_EQ(module->value(), 3);
    EXPECT_EQ(module->counts.discard, resetDiscards + 1u);
    EXPECT_FALSE(module->workSlots.immutable);
  }
  EXPECT_EQ(system.Step(), gfsim::SimStepResult::Failed);
  EXPECT_EQ(early.counts.work, 1u);
}

TEST(SystemLifecycleTest, ResetClearsRejectedEpochAndRerunsSystem) {
  ProbeSystem system;
  ProbeModule first("first"), second("second");
  second.checkPasses = false;
  add(system, first);
  add(system, second);
  system.Build();
  system.Reset();
  EXPECT_EQ(system.Step(), gfsim::SimStepResult::Failed);
  EXPECT_EQ(first.counts.cycleXfer, 0u);
  system.Reset();
  EXPECT_EQ(system.state(), gfsim::SimSystemState::Ready);
  EXPECT_EQ(system.cycle(), 0u);
  EXPECT_EQ(first.value(), 3);
  EXPECT_EQ(second.value(), 3);
  EXPECT_EQ(first.counts.reset, 2u);
  second.checkPasses = true;
  EXPECT_EQ(system.Step(), gfsim::SimStepResult::Running);
  EXPECT_EQ(first.value(), 7);
  EXPECT_EQ(second.value(), 7);
  EXPECT_EQ(system.cycle(), 1u);
}

TEST(SystemLifecycleTest, SuccessfulResetRerunReproducesCommittedTrace) {
  ProbeSystem system;
  ProbeModule first("first"), second("second");
  first.nextValue = 7;
  second.nextValue = 11;
  add(system, first);
  add(system, second);
  system.Build();

  auto run = [&]() {
    std::vector<std::array<int, 2>> trace;
    system.Reset();
    trace.push_back({first.value(), second.value()});
    EXPECT_EQ(system.state(), gfsim::SimSystemState::Ready);
    EXPECT_EQ(system.cycle(), 0u);
    EXPECT_EQ(system.Step(), gfsim::SimStepResult::Running);
    trace.push_back({first.value(), second.value()});
    EXPECT_EQ(system.cycle(), 1u);
    return trace;
  };

  const auto firstRun = run();
  const auto rerun = run();
  EXPECT_EQ(firstRun, (std::vector<std::array<int, 2>>{{3, 3}, {7, 11}}));
  EXPECT_EQ(rerun, firstRun);
  EXPECT_EQ(first.counts.reset, 2u);
  EXPECT_EQ(second.counts.reset, 2u);
  EXPECT_EQ(first.counts.cycleXfer, 2u);
  EXPECT_EQ(second.counts.cycleXfer, 2u);
}

TEST(SystemLifecycleTest, ZeroRuleSystemIsQuiescentAtEpochZeroAndStaysReady) {
  ProbeSystem system;
  system.Build();
  system.Reset();
  EXPECT_EQ(system.Step(), gfsim::SimStepResult::Quiescent);
  EXPECT_EQ(system.state(), gfsim::SimSystemState::Ready);
  EXPECT_EQ(system.cycle(), 0u);
  EXPECT_EQ(system.precommitCalls, 0u);
}

TEST(SystemLifecycleTest, PartialBuildFailureCannotBeRecoveredByReset) {
  ProbeSystem system;
  ProbeModule built("built"), failing("failing"), untouched("untouched");
  failing.throwOnBuild = true;
  add(system, built);
  add(system, failing);
  add(system, untouched);

  system.Build();
  EXPECT_EQ(system.state(), gfsim::SimSystemState::Failed);
  EXPECT_EQ(system.finalizeCalls, 0u);
  EXPECT_EQ(built.counts.build, 1u);
  EXPECT_EQ(failing.counts.build, 1u);
  EXPECT_EQ(untouched.counts.build, 0u);

  system.Reset();
  EXPECT_EQ(system.state(), gfsim::SimSystemState::Failed);
  for (ProbeModule *module : {&built, &failing, &untouched}) {
    EXPECT_EQ(module->counts.reset, 0u);
    EXPECT_EQ(module->counts.xfer, 0u);
    EXPECT_EQ(module->counts.primitiveXfer, 0u);
  }
}

TEST(SystemLifecycleTest, AcceptedNoWriteRunsXferWithoutAllocation) {
  ProbeSystem system;
  ProbeModule module("no_write");
  module.writeProposal = false;
  add(system, module);
  system.Build();
  system.Reset();
  const size_t before = allocationCount.load(std::memory_order_relaxed);
  trackAllocations.store(true, std::memory_order_relaxed);
  EXPECT_EQ(system.Step(), gfsim::SimStepResult::Running);
  trackAllocations.store(false, std::memory_order_relaxed);
  EXPECT_EQ(system.cycle(), 1u);
  EXPECT_EQ(module.counts.cycleXfer, 1u);
  EXPECT_EQ(module.counts.primitiveXfer, 2u); // reset transfer plus cycle
  EXPECT_EQ(module.counts.writes, 1u);
  EXPECT_EQ(module.value(), 3);
  EXPECT_EQ(module.counts.xferAllocations, 0u);
  EXPECT_EQ(allocationCount.load(std::memory_order_relaxed), before);
}

} // namespace
