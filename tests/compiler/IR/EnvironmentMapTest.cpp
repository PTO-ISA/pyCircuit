// Exact invariants of the shared-frame environment map: container semantics
// around erases and re-inserts, fork deltas measured from the shared frames,
// and single-settlement billing of real container work.
#include "Compiler/PythonImportEnvironment.h"

#include "gtest/gtest.h"

#include <string>

namespace {
namespace detail = acir::compiler::detail;
using Map = detail::EnvironmentMap<int>;

bool hasName(const llvm::SmallVectorImpl<llvm::StringRef> &names,
             llvm::StringRef name) {
  return llvm::is_contained(names, name);
}

TEST(EnvironmentMapTest, EraseHidesFrameValuesAndStaysHidden) {
  Map parent;
  parent["x"] = 1;
  parent.commit();
  Map child = parent.fork();
  ASSERT_TRUE(child.erase("x"));
  EXPECT_FALSE(child.contains("x"));
  EXPECT_EQ(child.size(), 0u);
  EXPECT_TRUE(child.materialize().empty());
  // Erasing an overlay write that shadows a frame value must hide both.
  child["y"] = 5;
  ASSERT_TRUE(child.contains("y"));
  ASSERT_TRUE(child.erase("y"));
  EXPECT_FALSE(child.contains("y"));
}

TEST(EnvironmentMapTest, ReinsertAfterEraseDoesNotResurrectFrameValue) {
  Map parent;
  parent["x"] = 1;
  parent.commit();
  Map child = parent.fork();
  ASSERT_TRUE(child.erase("x"));
  // StringMap semantics: a locally erased name is absent, so operator[]
  // starts from the default value and try_emplace inserts the given one.
  EXPECT_EQ(child["x"], 0);
  child.erase("x");
  EXPECT_EQ(child.try_emplace("x", 2), 2);
  EXPECT_EQ(child.lookup("x"), 2);
  EXPECT_EQ(child.size(), 1u);
  // All read interfaces agree after the re-insert.
  EXPECT_TRUE(child.materialize().contains("x"));
  EXPECT_EQ(child.materialize().lookup("x"), 2);
}

TEST(EnvironmentMapTest, PendingNamesNeverListANameTwice) {
  Map map;
  map["a"] = 1;
  map.commit();
  map["b"] = 2;
  map.erase("b");
  map.erase("a");
  // "b" only ever existed locally, so erasing it removes it outright; only
  // the frame-backed "a" needs a tombstone, and each name is listed once.
  auto names = map.pendingNames();
  EXPECT_EQ(names.size(), 1u);
  EXPECT_TRUE(hasName(names, "a"));
  EXPECT_FALSE(hasName(names, "b"));
  EXPECT_FALSE(map.contains("a"));
  EXPECT_FALSE(map.contains("b"));
}

TEST(EnvironmentMapTest, JoinDeltaIncludesWritesCommittedInsideAnArm) {
  // A nested fork inside an arm commits arm-local writes to frames the
  // sibling never sees; the join delta must still report them.
  Map base;
  base["a"] = 1;
  base.commit();
  Map yes = base.fork();
  yes["b"] = 2;
  yes.commit(); // nested guard fork: b leaves the overlay for a yes-only frame
  Map no = base.fork();
  llvm::SmallVector<llvm::StringRef, 4> delta;
  yes.appendDeltaSince(no, delta);
  no.appendDeltaSince(yes, delta);
  EXPECT_EQ(delta.size(), 1u);
  EXPECT_TRUE(hasName(delta, "b"));
  EXPECT_FALSE(hasName(delta, "a"));
}

TEST(EnvironmentMapTest, FlattenBillsItsWalkExactlyOnce) {
  Map map;
  map.takeUnbilledWork(); // drain whatever earlier state existed
  // 65 distinct commits: the 65th sees depth 64 and flattens the chain into
  // one frame holding 64 entries. The flatten walk is the only unbilled work:
  // exactly entries + 1, billed once, not per fork, not per arm.
  for (int i = 0; i < 65; ++i) {
    map[std::string("n") + std::to_string(i)] = i;
    map.commit();
  }
  Map yes = map.fork();
  Map no = map.fork();
  EXPECT_EQ(yes.takeUnbilledWork(), 0u); // forks inherit no pending billing
  EXPECT_EQ(no.takeUnbilledWork(), 0u);
  EXPECT_EQ(map.takeUnbilledWork(), 65u); // 64 merged entries + the frame
  EXPECT_EQ(map.takeUnbilledWork(), 0u);  // draining settles it exactly once
}

TEST(EnvironmentMapTest, FlattenKeepsNewestValueForRepeatedNames) {
  Map map;
  map["x"] = 1;
  map.commit();
  for (int i = 0; i < 65; ++i) {
    map["pad" + std::to_string(i)] = i;
    map.commit();
  }
  map["x"] = 2;
  map.commit();
  // Depth reached 64 again and flattened: the newest x must win.
  EXPECT_EQ(map.lookup("x"), 2);
  map.commit();
  EXPECT_EQ(map.lookup("x"), 2);
  EXPECT_EQ(map.materialize().lookup("x"), 2);
}

TEST(EnvironmentMapTest, MoveTransfersPendingBillingOnce) {
  Map map;
  for (int i = 0; i < 65; ++i) {
    map["pad" + std::to_string(i)] = i;
    map.commit();
  }
  // The flatten inside the loop left exactly one pending charge; moving the
  // map transfers the counter instead of duplicating or dropping it.
  Map moved = std::move(map);
  EXPECT_EQ(moved.takeUnbilledWork(), 65u);
  EXPECT_EQ(moved.takeUnbilledWork(), 0u); // settled exactly once
}
} // namespace
