// SPDX-License-Identifier: BSD-3-Clause
#include "pycircuit/gfsim/runtime.h"
#include "gtest/gtest.h"

#include <functional>
#include <string>
#include <vector>

namespace {
namespace tx = pycircuit::gfsim;

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
  ScriptModule producer, validator;
  tx::System system;
  system.Register(first);
  system.Register(second);
  system.Register(producer);
  system.Register(validator);
  std::vector<std::string> published;
  system.SetLogSink(
      [&](std::string_view message) { published.emplace_back(message); });
  producer.work = [&](tx::Context &context) {
    first.Propose(context, 12);
    second.Propose(context, 20);
    context.Log("pending");
  };
  validator.check = [&](tx::Context &context) {
    context.Assert(false, "transaction failed");
  };
  EXPECT_FALSE(system.Step());
  EXPECT_EQ(first.Q(), 11);
  EXPECT_EQ(second.Q(), 19);
  EXPECT_TRUE(published.empty());
  EXPECT_EQ(system.Cycle(), 0u);
  validator.check = {};
  ASSERT_TRUE(system.Step());
  EXPECT_EQ(first.Q(), 12);
  EXPECT_EQ(second.Q(), 20);
  EXPECT_EQ(published, std::vector<std::string>{"pending"});
}

TEST(TransactionalGfsimTest, UnknownControlFailsBeforeCommit) {
  tx::State<::gfsim::FourState<8>> state;
  ScriptModule module;
  tx::System system;
  system.Register(state);
  system.Register(module);
  module.work = [&](tx::Context &context) {
    state.Propose(context, ::gfsim::FourState<8>::known(::gfsim::Bits<8>{42}));
    tx::CheckedControl(::gfsim::FourState<1>::unknown());
  };
  EXPECT_FALSE(system.Step());
  EXPECT_TRUE(system.Exception());
  EXPECT_FALSE(state.Q().isFullyKnown());
}

TEST(TransactionalGfsimTest, SamplesHoldUntilTransferAndResetHasPriority) {
  tx::State<int> state(5);
  ScriptModule module;
  tx::System system;
  system.Register(state);
  system.Register(module);
  std::vector<std::string> published;
  system.SetLogSink(
      [&](std::string_view message) { published.emplace_back(message); });
  module.work = [&](tx::Context &context) {
    context.Log(std::to_string(state.Q()));
    state.Propose(context, state.Q() + 1);
  };
  ASSERT_TRUE(system.Sample(false));
  EXPECT_EQ(state.Q(), 5);
  ASSERT_TRUE(system.Sample(true));
  EXPECT_EQ(state.Q(), 6);
  ASSERT_TRUE(system.Sample(false, true));
  EXPECT_EQ(state.Q(), 6);
  ASSERT_TRUE(system.Sample(true, true));
  EXPECT_EQ(state.Q(), 5);
  ASSERT_TRUE(system.Sample(false));
  EXPECT_EQ(published, (std::vector<std::string>{"5", "5", "6", "6", "5"}));
}
} // namespace
