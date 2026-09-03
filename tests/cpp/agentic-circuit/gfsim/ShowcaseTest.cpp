#include "gfsim/showcase.h"

#include "gtest/gtest.h"

#include <algorithm>
#include <array>
#include <string>
#include <variant>

namespace gfsim {
namespace {

const CommittedEvent *findEvent(const ShowcaseResult &result,
                                std::string_view category,
                                std::string_view name) {
  auto event = std::ranges::find_if(result.events, [&](const auto &candidate) {
    return candidate.category == category && candidate.name == name;
  });
  return event == result.events.end() ? nullptr : &*event;
}

TEST(ShowcaseTest, ProducerQueueConsumerUsesDenseDispatchAndCommittedFlow) {
  ProducerQueueConsumerPolicy policy;
  policy.values = {3, 5, 8};
  policy.queueCapacity = 2;

  ShowcaseResult result = runShowcase(policy, ShowcaseWorkOrder::Ascending);

  EXPECT_EQ(result.termination.classification, TerminationClass::Completed);
  EXPECT_EQ(result.architecturalValues.at("consumed_count"), 3u);
  EXPECT_EQ(result.architecturalValues.at("consumed_sum"), 16u);
  ASSERT_EQ(result.hierarchy.size(), 3u);
  for (ObjectId id = 0; id != result.hierarchy.size(); ++id)
    EXPECT_EQ(result.hierarchy[id].id, id);
  EXPECT_EQ(result.hierarchy[0].path, "/showcase/producer");
  EXPECT_EQ(result.hierarchy[1].path, "/showcase/queue");
  EXPECT_EQ(result.hierarchy[2].path, "/showcase/consumer");
  EXPECT_NE(findEvent(result, "queue", "occupancy"), nullptr);
  EXPECT_NE(findEvent(result, "transaction", "completed"), nullptr);
}

TEST(ShowcaseTest, BackpressuredPipelineRetainsOfferUntilExactTransfer) {
  BackpressuredPipelinePolicy policy;
  policy.values = {21, 34};
  policy.readyTicks = {2, 4};

  ShowcaseResult result = runShowcase(policy, ShowcaseWorkOrder::Descending);

  EXPECT_EQ(result.termination.classification, TerminationClass::Completed);
  EXPECT_EQ(result.architecturalValues.at("consumed_count"), 2u);
  EXPECT_EQ(result.architecturalValues.at("consumed_sum"), 55u);
  EXPECT_EQ(result.architecturalValues.at("transfer_count"), 2u);
  EXPECT_NE(findEvent(result, "stall", "backpressure"), nullptr);
}

TEST(ShowcaseTest, RequestResponseMemoryPreservesCorrelationAndState) {
  RequestResponseMemoryPolicy policy;
  policy.requests = {{.correlationId = 10, .address = 1, .value = 17},
                     {.correlationId = 11, .address = 3, .value = 29}};
  policy.memoryCapacity = 4;

  ShowcaseResult result = runShowcase(policy, ShowcaseWorkOrder::Seeded, 91);

  EXPECT_EQ(result.termination.classification, TerminationClass::Completed);
  EXPECT_EQ(result.architecturalValues.at("completed_responses"), 2u);
  EXPECT_EQ(result.architecturalValues.at("memory.1"), 17u);
  EXPECT_EQ(result.architecturalValues.at("memory.3"), 29u);
  const CommittedEvent *response = findEvent(result, "transaction", "response");
  ASSERT_NE(response, nullptr);
  EXPECT_TRUE(response->rootSequenceId == 10 || response->rootSequenceId == 11);
}

TEST(ShowcaseTest, NestedArraysExposeDenseIndependentLaneHierarchy) {
  NestedArraysPolicy policy;
  policy.laneValues = {{1, 2}, {10, 20}, {100, 200}};

  ShowcaseResult result = runShowcase(policy, ShowcaseWorkOrder::Ascending);

  EXPECT_EQ(result.termination.classification, TerminationClass::Completed);
  EXPECT_EQ(result.architecturalValues.at("lane.0.sum"), 3u);
  EXPECT_EQ(result.architecturalValues.at("lane.1.sum"), 30u);
  EXPECT_EQ(result.architecturalValues.at("lane.2.sum"), 300u);
  ASSERT_EQ(result.hierarchy.size(), 7u);
  for (ObjectId id = 0; id != result.hierarchy.size(); ++id)
    EXPECT_EQ(result.hierarchy[id].id, id);
  EXPECT_EQ(result.hierarchy.back().path, "/showcase/lanes/2/sink");
}

TEST(ShowcaseTest, TimeDomainBridgeUsesExactIntegerDomainAdvancement) {
  MultiTimeDomainBridgePolicy policy;
  policy.values = {4, 6, 9};
  policy.sourcePeriod = 2;
  policy.targetPeriod = 3;

  ShowcaseResult result = runShowcase(policy, ShowcaseWorkOrder::Ascending);

  EXPECT_EQ(result.termination.classification, TerminationClass::Completed);
  EXPECT_EQ(result.architecturalValues.at("bridged_sum"), 19u);
  EXPECT_EQ(result.architecturalValues.at("bridged_count"), 3u);
  EXPECT_GT(result.termination.domainCycles.at("source"), 0u);
  EXPECT_GT(result.termination.domainCycles.at("target"), 0u);
  EXPECT_EQ(result.architecturalValues.at("last_transfer_tick") % 3, 0u);
}

TEST(ShowcaseTest, SuspendedProcessWakesResumesAndPreservesLiveState) {
  SuspendedProcessPolicy policy;
  policy.initialValue = 40;
  policy.incrementAfterWake = 2;
  policy.wakeTick = 3;

  ShowcaseResult result = runShowcase(policy, ShowcaseWorkOrder::Ascending);

  EXPECT_EQ(result.termination.classification, TerminationClass::Completed);
  EXPECT_EQ(result.architecturalValues.at("process_result"), 42u);
  EXPECT_EQ(result.architecturalValues.at("resume_count"), 1u);
  EXPECT_EQ(result.architecturalValues.at("wake_tick"), 3u);
}

TEST(ShowcaseTest, LegalWorkOrdersHaveByteIdenticalCommittedResults) {
  const std::array<ShowcasePolicy, 6> policies = {
      ProducerQueueConsumerPolicy{}, BackpressuredPipelinePolicy{},
      RequestResponseMemoryPolicy{}, NestedArraysPolicy{},
      MultiTimeDomainBridgePolicy{}, SuspendedProcessPolicy{}};

  for (const ShowcasePolicy &policy : policies) {
    const std::string ascending = canonicalShowcaseResult(
        runShowcase(policy, ShowcaseWorkOrder::Ascending, 73));
    const std::string descending = canonicalShowcaseResult(
        runShowcase(policy, ShowcaseWorkOrder::Descending, 73));
    const std::string seeded = canonicalShowcaseResult(
        runShowcase(policy, ShowcaseWorkOrder::Seeded, 73));
    EXPECT_EQ(descending, ascending);
    EXPECT_EQ(seeded, ascending);
  }
}

} // namespace
} // namespace gfsim
