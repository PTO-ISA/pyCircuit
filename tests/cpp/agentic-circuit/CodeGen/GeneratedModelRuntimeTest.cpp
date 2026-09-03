#include "gfsim/object.h"

#include "gtest/gtest.h"

#include <array>
#include <cstddef>
#include <memory>
#include <vector>

namespace gfsim {
namespace {

class OneShotObject final : public SimObject {
public:
  OneShotObject(ObjectId id, bool active)
      : SimObject(ObjectKind::Compute, "object", id), active_(active) {}

  void doWork(Epoch) override {
    ++workInvocations;
    if (active_ && !committed_)
      pending_ = true;
  }
  void doXfer(Epoch) override {
    if (pending_) {
      committed_ = true;
      pending_ = false;
    }
  }
  bool hasPendingCommit() const override { return pending_; }

  size_t workInvocations = 0;
  bool committed() const { return committed_; }

private:
  bool active_ = false;
  bool pending_ = false;
  bool committed_ = false;
};

class DomainTickObject final : public SimObject {
public:
  DomainTickObject(ObjectId id, SimSystem &system)
      : SimObject(ObjectKind::Process, "domain_tick", id), system_(system) {}

  void doWork(Epoch epoch) override {
    ++workInvocations;
    EXPECT_TRUE(system_.scheduleEvent({{epoch.time + 1, 0}, id(), 0, 0}));
  }

  size_t workInvocations = 0;

private:
  SimSystem &system_;
};

struct RuntimeObservation {
  size_t activeWorkInvocations = 0;
  size_t idleWorkInvocations = 0;
  size_t activationEdges = 0;
  bool committedResult = false;
};

RuntimeObservation runActiveFrontier(size_t idleCount) {
  SimSystem system("sparse");
  std::vector<std::unique_ptr<OneShotObject>> objects;
  std::vector<DispatchRow> rows;
  objects.reserve(idleCount + 1);
  rows.reserve(idleCount + 1);
  for (size_t index = 0; index <= idleCount; ++index) {
    objects.push_back(std::make_unique<OneShotObject>(index, index == 0));
    rows.push_back(makeDispatchRow(objects.back().get()));
  }
  std::vector<uint32_t> offsets(idleCount + 2, 1);
  offsets.front() = 0;
  const std::array<ObjectId, 1> targets = {0};
  EXPECT_TRUE(system.setDispatchTable(rows));
  EXPECT_TRUE(system.setActivationPlan(offsets, targets));
  EXPECT_TRUE(system.scheduleWork(0, {0, 0}));
  const TerminationResult result = system.run();
  EXPECT_EQ(result.classification, TerminationClass::Completed);

  size_t idleInvocations = 0;
  for (size_t index = 1; index < objects.size(); ++index)
    idleInvocations += objects[index]->workInvocations;
  return {objects.front()->workInvocations, idleInvocations, targets.size(),
          objects.front()->committed()};
}

TEST(GeneratedModelRuntimeTest, PermanentlyIdleObjectsDoNotIncreaseHotWork) {
  const RuntimeObservation baseline = runActiveFrontier(0);
  const RuntimeObservation sparse = runActiveFrontier(4096);
  EXPECT_EQ(sparse.activeWorkInvocations, baseline.activeWorkInvocations);
  EXPECT_EQ(sparse.idleWorkInvocations, 0u);
  EXPECT_EQ(sparse.activationEdges, baseline.activationEdges);
  EXPECT_EQ(sparse.committedResult, baseline.committedResult);
}

TEST(GeneratedModelRuntimeTest, GeneratedDomainLimitStopsBeforeExcessWork) {
  SimSystem system("generated");
  DomainTickObject object(0, system);
  const std::array rows = {makeDispatchRow(&object)};
  ASSERT_TRUE(system.setDispatchTable(rows));
  const std::array domains = {TimeDomainRuntime{"core", 2, 1, 1}};
  ASSERT_TRUE(system.setTimeDomains(domains));
  RuntimeLimits limits;
  limits.maxDomainCycles = {{"core", 2}};
  ASSERT_TRUE(system.setRuntimeLimits(limits));

  const TerminationResult result = system.run();
  EXPECT_EQ(result.classification, TerminationClass::Incomplete);
  EXPECT_EQ(result.diagnosticCode, "max_domain_cycles_reached");
  EXPECT_EQ(result.finalEpoch, (Epoch{5, 0}));
  EXPECT_EQ(result.domainCycles.at("core"), 2u);
  EXPECT_EQ(object.workInvocations, 5u);
}

} // namespace
} // namespace gfsim
