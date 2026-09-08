#include "gfsim/bits.h"
#include "gfsim/queue_blocks.h"
#include "gfsim/replay_session.h"

#include "gtest/gtest.h"

#include <cstdlib>
#include <filesystem>

namespace {
using namespace gfsim;

struct DoubleWrite {
  using Plan =
      StateTransitionPlan<std::tuple<UInt<3>, UInt<64>>, std::tuple<UInt<64>>>;
  std::optional<Plan> operator()(
      Epoch,
      std::tuple<const SimTable<UInt<3>> *, const SimTable<UInt<64>> *> tables,
      const UInt<64> &value) const {
    const auto count = std::get<0>(tables)->at(0);
    if (value == 0)
      return std::nullopt;
    return Plan{{OwnerWriteBatch<UInt<3>>{{0, count + 1}},
                 OwnerWriteBatch<UInt<64>>{{0, value}, {1, value}}},
                {std::optional<UInt<64>>{value}}};
  }
};

TEST(ReplayTest, FullMultiOwnerJournalPreservesBackpressureAndRetry) {
  const auto path = std::filesystem::temp_directory_path() /
                    "pyc-replay-transaction.pyctrace";
  SimQueue<UInt<64>> input("input", 0, nullptr, 1);
  SimQueue<UInt<64>> output("output", 1, nullptr, 1);
  SimTable<UInt<3>> count("count", 2, nullptr, 1);
  SimTable<UInt<64>> entries("entries", 3, nullptr, 2);
  QueueStateTransition<
      DoubleWrite, std::tuple<UInt<3>, UInt<64>>, std::tuple<UInt<64>>,
      std::tuple<UInt<64>>,
      std::tuple<TableFullEntryMerge<UInt<3>>, TableFullEntryMerge<UInt<64>>>>
      transition("update", 4, nullptr, {&count, &entries}, {&input}, {&output},
                 {TableWriteMode::Replace, TableWriteMode::Replace});
  const std::array rows{makeDispatchRow(&input), makeDispatchRow(&output),
                        makeDispatchRow(&count), makeDispatchRow(&entries),
                        makeDispatchRow(&transition)};
  ReplaySession replay(path.string(), rows);
  replay.add(input);
  replay.add(output);
  replay.add(count);
  replay.add(entries);
  replay.connect(transition, {&input}, {&output}, {&count, &entries});
  replay.start();
  std::ofstream native;
  if (const char *directory = std::getenv("PYC_REPLAY_TEST_OUT")) {
    std::filesystem::create_directories(directory);
    native.open(std::filesystem::path(directory) / "transaction.native.tsv");
  }
  auto cycle = [&](Tick tick, bool work) {
    replay.begin({tick, 0});
    if (work)
      for (const auto &row : rows)
        row.work(row.object, {tick, 0});
    for (const auto phase :
         {XferPhase::Arbitrate, XferPhase::Probe, XferPhase::Commit})
      for (const auto &row : rows)
        row.xfer(row.object, {tick, 0}, phase);
    replay.end();
    if (native) {
      native << tick << ' ' << count.at(0).value() << ' '
             << entries.at(0).value() << ' ' << entries.at(1).value() << ' '
             << input.committedSize() << ' ' << output.committedSize();
      for (const auto &value : input.committedValues())
        native << ' ' << value.value();
      for (const auto &value : output.committedValues())
        native << ' ' << value.value();
      native << '\n';
    }
  };
  ASSERT_TRUE(output.proposePush(7));
  ASSERT_TRUE(input.proposePush(UINT64_MAX));
  cycle(0, false);
  cycle(1, true);
  EXPECT_EQ(count.at(0), 0);
  EXPECT_EQ(input.committedSize(), 1);
  ASSERT_TRUE(output.proposePop());
  cycle(2, false);
  cycle(3, true);
  EXPECT_EQ(count.at(0), 1);
  EXPECT_EQ(entries.at(0).value(), UINT64_MAX);
  EXPECT_EQ(entries.at(1).value(), UINT64_MAX);
  EXPECT_TRUE(input.isEmpty());
  EXPECT_EQ(output.peek()->value(), UINT64_MAX);
  replay.finish();
  if (const char *directory = std::getenv("PYC_REPLAY_TEST_OUT")) {
    std::filesystem::create_directories(directory);
    std::filesystem::copy_file(
        path, std::filesystem::path(directory) / "transaction.pyctrace",
        std::filesystem::copy_options::overwrite_existing);
  }
  std::filesystem::remove(path);
}

TEST(ReplayTest, EqualTokensAndDelayedReadinessHaveDistinctIdentity) {
  const auto path =
      std::filesystem::temp_directory_path() / "pyc-replay-latency.pyctrace";
  SimQueue<UInt<64>> queue("delayed", 0, nullptr, 4, SIZE_MAX, nullptr, 3, 2);
  const std::array rows{makeDispatchRow(&queue)};
  ReplaySession replay(path.string(), rows);
  replay.add(queue);
  replay.start();
  ASSERT_TRUE(queue.proposePush(9));
  ASSERT_TRUE(queue.proposePush(9));
  replay.begin({0, 0});
  queue.doXfer({0, 0});
  replay.end();
  EXPECT_EQ(queue.committedSize(), 0);
  replay.begin({1, 0});
  queue.doXfer({1, 0});
  replay.end();
  EXPECT_EQ(queue.committedSize(), 0);
  replay.begin({2, 0});
  queue.doXfer({2, 0});
  replay.end();
  EXPECT_EQ(queue.committedSize(), 2);
  ASSERT_TRUE(queue.proposePop());
  ASSERT_TRUE(queue.proposePush(9));
  replay.begin({3, 0});
  queue.doXfer({3, 0});
  replay.end();
  EXPECT_EQ(queue.committedSize(), 1);
  replay.begin({5, 0});
  queue.doXfer({5, 0});
  replay.end();
  EXPECT_EQ(queue.committedSize(), 2);
  replay.finish();
  if (const char *directory = std::getenv("PYC_REPLAY_TEST_OUT")) {
    std::filesystem::create_directories(directory);
    std::filesystem::copy_file(
        path, std::filesystem::path(directory) / "latency.pyctrace",
        std::filesystem::copy_options::overwrite_existing);
  }
  std::filesystem::remove(path);
}

TEST(ReplayTest, SessionDetachesAndCanBeReattached) {
  const auto path =
      std::filesystem::temp_directory_path() / "pyc-replay-detach.pyctrace";
  SimQueue<UInt<3>> queue("queue", 0, nullptr, 1);
  const std::array rows{makeDispatchRow(&queue)};
  for (unsigned i = 0; i < 2; ++i) {
    {
      ReplaySession replay(path.string(), rows);
      replay.add(queue);
      replay.start();
      EXPECT_THROW(queue.reset(), std::runtime_error);
      replay.finish();
      EXPECT_NO_THROW(queue.reset());
    }
    EXPECT_EQ(queue.stateObserver(), nullptr);
  }
  std::filesystem::remove(path);
}

TEST(ReplayTest, RejectsMissingSerializerInsteadOfClaimingCompleteCoverage) {
  struct Opaque {};
  SimTable<Opaque> state("opaque", 0, nullptr, 1);
  const auto path =
      std::filesystem::temp_directory_path() / "pyc-replay-opaque.pyctrace";
  const std::array rows{makeDispatchRow(&state)};
  ReplaySession replay(path.string(), rows);
  EXPECT_THROW(replay.add(state), std::runtime_error);
  EXPECT_EQ(state.stateObserver(), nullptr);
  std::filesystem::remove(path);
}

TEST(ReplayTest, RejectsPartialBarrierAndUntrackedMutation) {
  const auto path =
      std::filesystem::temp_directory_path() / "pyc-replay-partial.pyctrace";
  ReplayRecorder recorder(path.string());
  unsigned state = 0;
  recorder.add(0, {{{"name", "state"}}, [&] { return replayValue(state); }});
  recorder.start();
  ++state;
  EXPECT_THROW(recorder.finish("completed"), std::runtime_error);
  recorder.begin({1, 0});
  EXPECT_THROW(recorder.finish("completed"), std::runtime_error);
  recorder.end();
  recorder.finish("completed");
  std::filesystem::remove(path);
}

TEST(ReplayTest, FailedAttachmentDoesNotDetachAnotherSession) {
  const auto a =
      std::filesystem::temp_directory_path() / "pyc-observe-a.pyctrace";
  const auto b =
      std::filesystem::temp_directory_path() / "pyc-observe-b.pyctrace";
  SimQueue<unsigned> first("first", 0, nullptr, 1),
      second("second", 1, nullptr, 1);
  const std::array firstRows{makeDispatchRow(&second)};
  ReplaySession active(a.string(), firstRows);
  active.add(second);
  active.start();
  const std::array rows{makeDispatchRow(&first), makeDispatchRow(&second)};
  {
    ReplaySession rejected(b.string(), rows);
    rejected.add(first);
    rejected.add(second);
    EXPECT_THROW(rejected.start(), std::runtime_error);
    EXPECT_EQ(first.stateObserver(), nullptr);
    EXPECT_EQ(second.stateObserver(), &active);
  }
  EXPECT_EQ(second.stateObserver(), &active);
  active.finish();
  std::filesystem::remove(a);
  std::filesystem::remove(b);
}
TEST(ReplayTest, FinishedSessionAllowsContinuedExecutionAndSystemReset) {
  const auto path =
      std::filesystem::temp_directory_path() / "pyc-observe-system.pyctrace";
  SimSystem system("system");
  SimQueue<unsigned> queue("queue", 0, nullptr, 1);
  const std::array rows{makeDispatchRow(&queue)};
  ReplaySession replay(path.string(), rows);
  replay.add(queue);
  replay.start();
  replay.attach(system);
  EXPECT_THROW(system.reset(), std::runtime_error);
  replay.finish();
  EXPECT_EQ(system.stateObserver(), nullptr);
  EXPECT_EQ(queue.stateObserver(), nullptr);
  EXPECT_NO_THROW(system.reset());
  EXPECT_NO_THROW(queue.reset());
  ASSERT_TRUE(queue.proposePush(9));
  queue.doXfer({1, 0});
  EXPECT_EQ(*queue.peek(), 9);
  std::filesystem::remove(path);
}
} // namespace
