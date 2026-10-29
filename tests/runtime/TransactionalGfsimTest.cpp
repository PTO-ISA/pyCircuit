// SPDX-License-Identifier: BSD-3-Clause
#include "pycircuit/transactional_gfsim/runtime.h"
#include "gtest/gtest.h"

#include <functional>
#include <string>
#include <vector>

namespace {
namespace tx = pycircuit::transactional_gfsim;

struct ScriptModule final : tx::Module {
  std::function<void(tx::Context &)> comb, work, check;
  void CombWork(tx::Context &context) override {
    if (comb)
      comb(context);
  }
  void Work(tx::Context &context) override {
    if (work)
      work(context);
  }
  void Check(tx::Context &context) override {
    if (check)
      check(context);
  }
};

TEST(TransactionalGfsimTest, AllModulesReadOldStateAndLogsFollowCommit) {
  tx::State<int> left(3), right(7);
  ScriptModule producer, consumer;
  tx::System system;
  system.Register(left);
  system.Register(right);
  system.Register(producer);
  system.Register(consumer);
  std::vector<std::string> published;
  system.SetLogSink([&](std::string_view message) {
    EXPECT_EQ(left.Q(), 4);
    EXPECT_EQ(right.Q(), 3);
    published.emplace_back(message);
  });
  producer.work = [&](tx::Context &context) {
    left.Propose(context, left.Q() + 1);
    context.Log("committed");
  };
  consumer.comb = [&](tx::Context &) { EXPECT_EQ(left.Q(), 3); };
  consumer.work = [&](tx::Context &context) {
    EXPECT_TRUE(published.empty());
    right.Propose(context, left.Q());
  };
  ASSERT_TRUE(system.Step());
  EXPECT_EQ(published, std::vector<std::string>{"committed"});
  system.Reset();
  EXPECT_EQ(left.Q(), 3);
  EXPECT_EQ(right.Q(), 7);
  producer.work = {};
  consumer.comb = {};
  consumer.work = {};
  ASSERT_TRUE(system.Step());
  EXPECT_EQ(left.Q(), 3);
  EXPECT_EQ(right.Q(), 7);
}

TEST(TransactionalGfsimTest, LateFailureDiscardsEveryProposalAndLog) {
  tx::State<int> first(11), second(19);
  tx::Queue<int, 2> queue;
  ScriptModule producer, validator;
  tx::System system;
  system.Register(first);
  system.Register(second);
  system.Register(queue);
  system.Register(producer);
  system.Register(validator);
  std::vector<std::string> published;
  system.SetLogSink(
      [&](std::string_view message) { published.emplace_back(message); });
  producer.work = [&](tx::Context &context) { queue.ProposePush(context, 7); };
  ASSERT_TRUE(system.Step());
  ASSERT_TRUE(queue.CanPop());
  ASSERT_TRUE(queue.CanPush());
  EXPECT_EQ(queue.Head(), 7);
  EXPECT_EQ(queue.Size(), 1u);
  producer.work = [&](tx::Context &context) {
    first.Propose(context, 12);
    second.Propose(context, 20);
    queue.ProposePop(context);
    queue.ProposePush(context, 9);
    context.Log("pending");
  };
  validator.check = [&](tx::Context &context) {
    context.Assert(false, "transaction failed");
  };
  EXPECT_FALSE(system.Step());
  EXPECT_EQ(first.Q(), 11);
  EXPECT_EQ(second.Q(), 19);
  EXPECT_TRUE(queue.CanPop());
  EXPECT_TRUE(queue.CanPush());
  EXPECT_EQ(queue.Head(), 7);
  EXPECT_EQ(queue.Size(), 1u);
  EXPECT_TRUE(published.empty());
  EXPECT_EQ(system.Cycle(), 1u);
  // Reset is the supported recovery boundary; do not resume a failed run.
  system.Reset();
  EXPECT_EQ(first.Q(), 11);
  EXPECT_EQ(second.Q(), 19);
  EXPECT_FALSE(queue.CanPop());
  EXPECT_TRUE(queue.CanPush());
}

TEST(TransactionalGfsimTest, SamplesHoldUntilTransferAndResetHasPriority) {
  tx::State<int> state(5);
  tx::Queue<int, 2> queue;
  ScriptModule module;
  tx::System system;
  system.Register(state);
  system.Register(queue);
  system.Register(module);
  std::vector<std::string> published;
  system.SetLogSink(
      [&](std::string_view message) { published.emplace_back(message); });
  module.work = [&](tx::Context &context) {
    context.Log(std::to_string(state.Q()));
    state.Propose(context, state.Q() + 1);
    queue.ProposePush(context, state.Q());
  };
  ASSERT_TRUE(system.Sample(false));
  EXPECT_EQ(state.Q(), 5);
  EXPECT_FALSE(queue.CanPop());
  ASSERT_TRUE(system.Sample(true));
  EXPECT_EQ(state.Q(), 6);
  EXPECT_EQ(queue.Head(), 5);
  ASSERT_TRUE(system.Sample(false, true));
  EXPECT_EQ(state.Q(), 6);
  EXPECT_EQ(queue.Head(), 5);
  EXPECT_TRUE(queue.CanPush());
  ASSERT_TRUE(system.Sample(true, true));
  EXPECT_EQ(state.Q(), 5);
  EXPECT_FALSE(queue.CanPop());
  ASSERT_TRUE(system.Sample(false));
  EXPECT_EQ(published, (std::vector<std::string>{"5", "5", "6", "6", "5"}));
}
} // namespace
