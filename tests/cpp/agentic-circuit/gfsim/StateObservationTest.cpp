// This test intentionally includes no replay, codec, or file I/O header.
#include "gfsim/queue_blocks.h"
#include "gtest/gtest.h"

namespace {
using namespace gfsim;
struct Entry {
  unsigned left = 0, right = 0;
  bool operator==(const Entry &) const = default;
};
struct MergeLeft {
  void operator()(Entry &to, const Entry &from) const { to.left = from.left; }
};
struct MergeRight {
  void operator()(Entry &to, const Entry &from) const { to.right = from.right; }
};
struct Collector : StateObserver {
  std::vector<std::pair<StateAction, Entry>> writes;
  ObjectId owner = kInvalidObjectId;
  ExecutionPhase phase = ExecutionPhase::External;
  void notify(const StateEvent &event) override {
    if (event.action == StateAction::BeforeWrite ||
        event.action == StateAction::AfterWrite)
      writes.emplace_back(event.action,
                          *static_cast<const Entry *>(event.value));
  }
  void context(ObjectId id, ExecutionPhase p) override {
    owner = id;
    phase = p;
  }
  void clearContext() override {
    owner = kInvalidObjectId;
    phase = ExecutionPhase::External;
  }
  void begin(Epoch) override {}
  void end() override {}
  void beforeReset() override {}
};
TEST(StateObservationTest, SameRowWritersExposeActualIntermediateMergeResults) {
  SimTable<Entry> table("state", 0, nullptr, 1);
  Collector collector;
  table.setStateObserver(&collector);
  const std::array<size_t, 1> left{0}, right{1};
  ASSERT_TRUE(table.proposeWrite(1, 0, Entry{4, 0}, left, MergeLeft{}));
  ASSERT_TRUE(table.proposeWrite(2, 0, Entry{0, 7}, right, MergeRight{}));
  table.doXfer({0, 0});
  ASSERT_EQ(collector.writes.size(), 4);
  EXPECT_EQ(collector.writes[0].second, (Entry{0, 0}));
  EXPECT_EQ(collector.writes[1].second, (Entry{4, 0}));
  EXPECT_EQ(collector.writes[2].second, (Entry{4, 0}));
  EXPECT_EQ(collector.writes[3].second, (Entry{4, 7}));
  EXPECT_EQ(table.at(0), (Entry{4, 7}));
}
TEST(StateObservationTest, ThrowingComponentClearsExecutionContext) {
  struct Throwing : SimObject {
    Throwing() : SimObject(ObjectKind::Compute, "throwing", 3, nullptr) {}
    void doWork(Epoch) override { throw std::runtime_error("failure"); }
  } component;
  Collector collector;
  component.setStateObserver(&collector);
  const auto row = makeDispatchRow(&component);
  EXPECT_THROW(row.work(row.object, {0, 0}), std::runtime_error);
  EXPECT_EQ(collector.owner, kInvalidObjectId);
  EXPECT_EQ(collector.phase, ExecutionPhase::External);
}
TEST(StateObservationTest, OpaquePayloadNeedsNoRecordingCodecToExecute) {
  struct Opaque {
    unsigned value;
  };
  SimQueue<Opaque> queue("opaque", 0, nullptr, 2);
  ASSERT_TRUE(queue.proposePush({7}));
  queue.doXfer({0, 0});
  ASSERT_TRUE(queue.peek());
  EXPECT_EQ(queue.peek()->value, 7);
  ASSERT_TRUE(queue.proposePop());
  queue.doXfer({1, 0});
  EXPECT_TRUE(queue.isEmpty());
}
} // namespace
