#include "gfsim/SimModule.h"
#include "gfsim/SystemRunner.h"
#include "gtest/gtest.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

namespace {
constexpr std::string_view config =
    R"({"deadlock_window":null,"max_domain_cycles":{},"max_ticks":8,"schema":"pycircuit-model-config","version":"1"})";
constexpr std::string_view tripleSpec =
    R"({"event":"mixed","items":[{"kind":"value","ordinal":0},{"kind":"value","ordinal":1},{"kind":"value","ordinal":2}],"level":"info"})";
constexpr std::string_view scalarSpec =
    R"({"event":"scalar","items":[{"kind":"value","ordinal":0}],"level":"info"})";

gfsim::RunnerObservation metadata(std::uint32_t ordinal, std::string spec,
                                  std::uint32_t index = 0,
                                  std::uint32_t size = 1) {
  gfsim::RunnerObservation result;
  result.stableOrdinal = ordinal;
  result.kind = "log";
  result.instance = "root.child";
  result.registration = "registration";
  result.site = "site";
  result.specJson = std::move(spec);
  result.hasValue = true;
  result.groupIndex = index;
  result.groupSize = size;
  return result;
}

struct StagePlan {
  gfsim::SlotValue value;
  bool stage = true, valid = true, path = true;
  std::uint64_t epochOffset = 0;
};

class GroupModule final : public gfsim::SimModule {
public:
  GroupModule(gfsim::ObservationSlots &slots, std::vector<StagePlan> &plans)
      : SimModule("groups"), slots(slots), plans(plans) {}
  gfsim::SimSystem *system = nullptr;
  unsigned builds = 0, works = 0, committedWrites = 0;

private:
  void Build() override { ++builds; }
  void Reset() noexcept override { pending = false; }
  void Work() override {
    ++works;
    pending = true;
    // Work scheduling order does not determine the published operand order.
    for (std::size_t index = plans.size(); index-- > 0;) {
      const auto &plan = plans[index];
      if (plan.stage)
        EXPECT_TRUE(slots.Stage(index, plan.value, plan.valid, plan.path,
                                system->cycle() + plan.epochOffset));
    }
  }
  void Xfer() noexcept override {
    if (pending)
      ++committedWrites;
    pending = false;
  }
  void DiscardNext() noexcept override { pending = false; }
  bool HasWork() const noexcept override { return true; }
  gfsim::ObservationSlots &slots;
  std::vector<StagePlan> &plans;
  bool pending = false;
};

class GroupSystem final : public gfsim::SimSystem {
public:
  bool checksPass = true;

private:
  bool Precheck() noexcept override { return checksPass; }
};

struct GroupFixture {
  // A scalar precedes the group. A later malformed group must prevent even
  // this earlier scalar from being published for that epoch.
  std::vector<gfsim::ObservationDescriptor> descriptors{
      {2, 1, 1, 1, gfsim::ObservationKind::Event},
      {7, 1, 1, 2, gfsim::ObservationKind::Event},
      {8, 1, 1, 2, gfsim::ObservationKind::Event},
      {9, 1, 1, 2, gfsim::ObservationKind::Event},
      {20, 1, 1, 3, gfsim::ObservationKind::Event},
      {30, 1, 1, 4, gfsim::ObservationKind::Event},
      {31, 1, 1, 4, gfsim::ObservationKind::Event},
      {40, 1, 1, 5, gfsim::ObservationKind::Gauge},
  };
  std::vector<gfsim::RunnerObservation> entries{
      metadata(2, std::string(scalarSpec)),
      metadata(7, std::string(tripleSpec), 0, 3),
      metadata(8, std::string(tripleSpec), 1, 3),
      metadata(9, std::string(tripleSpec), 2, 3),
      metadata(20, R"({"items":[{"kind":"literal","text":"literal"}]})"),
      metadata(30, std::string(scalarSpec)),
      metadata(31, std::string(scalarSpec)),
      metadata(40, R"({"name":"progress"})"),
  };
  std::vector<StagePlan> plans{
      {gfsim::SlotValue::Unsigned(5)},
      {gfsim::SlotValue::Bool(true)},
      {gfsim::SlotValue::Signed(-7)},
      {gfsim::SlotValue::Unsigned(18446744073709551615ull)},
      {gfsim::SlotValue::Bool(true)},
      {gfsim::SlotValue::Unsigned(11)},
      {gfsim::SlotValue::Unsigned(13)},
      {gfsim::SlotValue::Unsigned(42)},
  };
  gfsim::ObservationSlots slots;
  GroupModule module{slots, plans};
  GroupSystem system;
  unsigned limit = 1;
  std::function<void(std::uint64_t)> beforeDrive;

  explicit GroupFixture(
      std::function<void(std::vector<gfsim::ObservationDescriptor> &)> mutate =
          {}) {
    entries[4].kind = "print";
    entries[4].hasValue = false;
    entries[7].kind = "report";
    entries[7].reportName = "progress";
    if (mutate)
      mutate(descriptors);
    EXPECT_TRUE(slots.Configure(descriptors, descriptors.size()));
    EXPECT_TRUE(system.AttachObservations(slots));
    EXPECT_TRUE(system.AddModule(module));
    module.system = &system;
  }
  static bool drive(void *context, std::uint64_t epoch) {
    auto &fixture = *static_cast<GroupFixture *>(context);
    if (fixture.beforeDrive)
      fixture.beforeDrive(epoch);
    return epoch < fixture.limit;
  }
};

struct RunResult {
  int status;
  std::vector<std::string> lines;
};

RunResult run(GroupFixture &fixture) {
  static std::atomic<unsigned> serial{0};
  const auto directory =
      std::filesystem::temp_directory_path() /
      ("pycircuit-runner-groups-" +
       std::to_string(
           std::chrono::steady_clock::now().time_since_epoch().count()) +
       "-" + std::to_string(serial++));
  std::filesystem::create_directory(directory);
  const auto events = directory / "events.jsonl";
  RunResult result;
  {
    std::string binary = "runner", flag = "--events", path = events.string();
    char *argv[] = {binary.data(), flag.data(), path.data()};
    gfsim::SystemRunner runner(3, argv, config);
    EXPECT_TRUE(runner.ready());
    result.status =
        runner.Run(fixture.system, fixture.slots, fixture.entries,
                   {&fixture, nullptr, GroupFixture::drive, nullptr});
  }
  std::ifstream input(events);
  for (std::string line; std::getline(input, line);)
    result.lines.push_back(line);
  std::filesystem::remove_all(directory);
  return result;
}

std::string record(std::string_view kind, std::string_view spec,
                   std::string_view values, unsigned epoch = 0) {
  return "{\"kind\":\"" + std::string(kind) +
         "\",\"instance\":\"root.child\",\"registration\":\"registration\","
         "\"site\":\"site\",\"evaluation_epoch\":\"" +
         std::to_string(epoch) + "\",\"commit_epoch\":\"" +
         std::to_string(epoch + 1) + "\",\"spec\":" + std::string(spec) +
         ",\"values\":[" + std::string(values) + "]}";
}

TEST(RunnerObservationGroupsTest,
     GroupsValuesInCanonicalOrderWithoutMergingScalars) {
  GroupFixture fixture;
  // Existing scalar metadata initializers rely on the appended field defaults.
  fixture.entries[5] = {30,           "log",
                        "root.child", "registration",
                        "site",       std::string(scalarSpec),
                        true,         ""};
  fixture.entries[6] = {31,           "log",
                        "root.child", "registration",
                        "site",       std::string(scalarSpec),
                        true,         ""};
  const auto result = run(fixture);
  ASSERT_EQ(result.status, 0);
  ASSERT_EQ(result.lines.size(), 7u); // Six observations and the final result.
  EXPECT_EQ(result.lines[0],
            record("log", scalarSpec, R"({"kind":"integer","value":"5"})"));
  EXPECT_EQ(
      result.lines[1],
      record(
          "log", tripleSpec,
          R"({"kind":"bool","value":true},{"kind":"integer","value":"-7"},{"kind":"integer","value":"18446744073709551615"})"));
  EXPECT_EQ(result.lines[2], record("print", fixture.entries[4].specJson, ""));
  EXPECT_EQ(result.lines[3],
            record("log", scalarSpec, R"({"kind":"integer","value":"11"})"));
  EXPECT_EQ(result.lines[4],
            record("log", scalarSpec, R"({"kind":"integer","value":"13"})"));
  EXPECT_EQ(result.lines[5], record("report", fixture.entries[7].specJson,
                                    R"({"kind":"integer","value":"42"})"));
  EXPECT_NE(result.lines[6].find(R"("status":"TERMINATED","epoch_time":"1")"),
            std::string::npos);
  EXPECT_EQ(fixture.module.committedWrites, 1u);
}

TEST(RunnerObservationGroupsTest, FullyInactiveGroupsDoNotProduceEmptyEvents) {
  for (unsigned mode = 0; mode < 3; ++mode) {
    SCOPED_TRACE(mode);
    GroupFixture fixture;
    for (unsigned index = 1; index <= 3; ++index) {
      if (mode == 0)
        fixture.plans[index].stage = false;
      if (mode == 1)
        fixture.plans[index].valid = false;
      if (mode == 2)
        fixture.plans[index].path = false;
    }
    const auto result = run(fixture);
    ASSERT_EQ(result.status, 0);
    ASSERT_EQ(result.lines.size(), 6u);
    EXPECT_TRUE(std::none_of(
        result.lines.begin(), result.lines.end(), [](const auto &line) {
          return line.find(R"("event":"mixed")") != std::string::npos;
        }));
    EXPECT_EQ(fixture.module.committedWrites, 1u);
  }
}

TEST(RunnerObservationGroupsTest, MetadataTableOrderDoesNotReorderBoundLanes) {
  GroupFixture original;
  const auto reference = run(original);
  ASSERT_EQ(reference.status, 0);
  GroupFixture permuted;
  std::swap(permuted.entries[1], permuted.entries[2]);
  std::swap(permuted.entries[0], permuted.entries[7]);
  const auto result = run(permuted);
  EXPECT_EQ(result.status, 0);
  EXPECT_EQ(result.lines, reference.lines);
}

TEST(RunnerObservationGroupsTest, MultipleGroupsKeepTheirOwnValueArrays) {
  GroupFixture fixture;
  const std::string pairSpec =
      R"({"event":"pair","items":[{"kind":"value","ordinal":0},{"kind":"value","ordinal":1}],"level":"info"})";
  fixture.entries[5].specJson = fixture.entries[6].specJson = pairSpec;
  fixture.entries[5].groupSize = fixture.entries[6].groupSize = 2;
  fixture.entries[6].groupIndex = 1;
  const auto result = run(fixture);
  ASSERT_EQ(result.status, 0);
  ASSERT_EQ(result.lines.size(), 6u);
  EXPECT_EQ(
      result.lines[1],
      record(
          "log", tripleSpec,
          R"({"kind":"bool","value":true},{"kind":"integer","value":"-7"},{"kind":"integer","value":"18446744073709551615"})"));
  EXPECT_EQ(
      result.lines[3],
      record(
          "log", pairSpec,
          R"({"kind":"integer","value":"11"},{"kind":"integer","value":"13"})"));
}

TEST(RunnerObservationGroupsTest, EachEpochPublishesOnlyItsCurrentGroupValues) {
  GroupFixture fixture;
  fixture.limit = 2;
  fixture.beforeDrive = [&](std::uint64_t epoch) {
    if (epoch == 1) {
      fixture.plans[1].value = gfsim::SlotValue::Bool(false);
      fixture.plans[2].value = gfsim::SlotValue::Signed(-9);
      fixture.plans[3].value = gfsim::SlotValue::Unsigned(123);
    }
  };
  const auto result = run(fixture);
  ASSERT_EQ(result.status, 0);
  ASSERT_EQ(result.lines.size(), 13u);
  EXPECT_EQ(
      result.lines[1],
      record(
          "log", tripleSpec,
          R"({"kind":"bool","value":true},{"kind":"integer","value":"-7"},{"kind":"integer","value":"18446744073709551615"})"));
  EXPECT_EQ(
      result.lines[7],
      record(
          "log", tripleSpec,
          R"({"kind":"bool","value":false},{"kind":"integer","value":"-9"},{"kind":"integer","value":"123"})",
          1));
  EXPECT_EQ(fixture.module.committedWrites, 2u);
}

TEST(RunnerObservationGroupsTest, PartialGroupsRefuseBeforeAnyEpochOutput) {
  for (unsigned mode = 0; mode < 3; ++mode) {
    SCOPED_TRACE(mode);
    GroupFixture fixture;
    if (mode == 0)
      fixture.plans[2].stage = false;
    if (mode == 1)
      fixture.plans[2].valid = false;
    if (mode == 2)
      fixture.plans[2].path = false;
    const auto result = run(fixture);
    EXPECT_EQ(result.status, 2);
    EXPECT_TRUE(result.lines.empty());
    EXPECT_EQ(fixture.system.cycle(), 1u);
    EXPECT_EQ(fixture.module.committedWrites, 1u);
  }
}

TEST(RunnerObservationGroupsTest,
     MalformedMetadataRefusesBeforeModelExecution) {
  const std::vector<std::function<void(GroupFixture &)>> mutations{
      [](auto &f) { f.entries.pop_back(); },
      [](auto &f) { f.entries[2].stableOrdinal = f.entries[1].stableOrdinal; },
      [](auto &f) {
        std::swap(f.entries[1].groupIndex, f.entries[2].groupIndex);
      },
      [](auto &f) { f.entries[1].groupSize = 0; },
      [](auto &f) { f.entries[1].groupIndex = 1; },
      [](auto &f) { f.entries[2].groupIndex = 0; },
      [](auto &f) { f.entries[2].groupSize = 2; },
      [](auto &f) { f.entries[3].groupIndex = 3; },
      [](auto &f) { f.entries[2].kind = "print"; },
      [](auto &f) { f.entries[2].instance = "foreign"; },
      [](auto &f) { f.entries[2].registration = "foreign"; },
      [](auto &f) { f.entries[2].site = "foreign"; },
      [](auto &f) { f.entries[2].specJson = std::string(scalarSpec); },
      [](auto &f) { f.entries[2].hasValue = false; },
      [](auto &f) { f.entries[7].groupSize = 2; },
  };
  for (std::size_t index = 0; index < mutations.size(); ++index) {
    SCOPED_TRACE(index);
    GroupFixture fixture;
    mutations[index](fixture);
    const auto result = run(fixture);
    EXPECT_EQ(result.status, 2);
    EXPECT_TRUE(result.lines.empty());
    EXPECT_EQ(fixture.module.builds, 0u);
    EXPECT_EQ(fixture.module.works, 0u);
    EXPECT_EQ(fixture.system.cycle(), 0u);
  }
}

TEST(RunnerObservationGroupsTest,
     MatchingDisplayMetadataCannotJoinDifferentPhysicalIdentities) {
  using Descriptors = std::vector<gfsim::ObservationDescriptor>;
  const std::vector<std::function<void(Descriptors &)>> mutations{
      [](auto &descriptors) {
        for (std::size_t index = 3; index < descriptors.size(); ++index)
          descriptors[index].ownerKey = 2;
      },
      [](auto &descriptors) {
        for (std::size_t index = 3; index < descriptors.size(); ++index)
          descriptors[index].registrationKey = 2;
      },
      [](auto &descriptors) { descriptors[3].siteKey = 3; },
      [](auto &descriptors) {
        descriptors[3].kind = gfsim::ObservationKind::Gauge;
      },
  };
  for (std::size_t index = 0; index < mutations.size(); ++index) {
    SCOPED_TRACE(index);
    GroupFixture fixture(mutations[index]);
    const auto result = run(fixture);
    EXPECT_EQ(result.status, 2);
    EXPECT_TRUE(result.lines.empty());
    EXPECT_EQ(fixture.module.builds, 0u);
    EXPECT_EQ(fixture.module.works, 0u);
    EXPECT_EQ(fixture.system.cycle(), 0u);
  }
}

TEST(RunnerObservationGroupsTest,
     LaterIncompleteEpochPreservesOnlyEarlierCompleteOutput) {
  GroupFixture fixture;
  fixture.limit = 2;
  fixture.beforeDrive = [&](std::uint64_t epoch) {
    if (epoch == 1)
      fixture.plans[2].path = false;
  };
  const auto result = run(fixture);
  EXPECT_EQ(result.status, 2);
  ASSERT_EQ(result.lines.size(), 6u);
  EXPECT_TRUE(std::all_of(
      result.lines.begin(), result.lines.end(), [](const auto &line) {
        return line.find(R"("evaluation_epoch":"0","commit_epoch":"1")") !=
               std::string::npos;
      }));
  EXPECT_EQ(fixture.system.cycle(), 2u);
}

TEST(RunnerObservationGroupsTest,
     BadEvaluationEpochRejectsWholeStepWithoutEvents) {
  GroupFixture fixture;
  fixture.plans[2].epochOffset = 1;
  const auto result = run(fixture);
  EXPECT_EQ(result.status, 1);
  ASSERT_EQ(result.lines.size(), 1u);
  EXPECT_NE(result.lines[0].find(
                R"("kind":"result","status":"FAILED","epoch_time":"0")"),
            std::string::npos);
  EXPECT_EQ(fixture.module.committedWrites, 0u);
  EXPECT_EQ(fixture.system.cycle(), 0u);
  EXPECT_TRUE(fixture.slots.Events().empty());
}

TEST(RunnerObservationGroupsTest,
     FailedWholeSystemCheckDiscardsAllStagedGroups) {
  GroupFixture fixture;
  fixture.system.checksPass = false;
  const auto result = run(fixture);
  EXPECT_EQ(result.status, 1);
  ASSERT_EQ(result.lines.size(), 1u);
  EXPECT_NE(result.lines[0].find(
                R"("kind":"result","status":"FAILED","epoch_time":"0")"),
            std::string::npos);
  EXPECT_EQ(fixture.module.committedWrites, 0u);
  EXPECT_EQ(fixture.system.cycle(), 0u);
  EXPECT_TRUE(fixture.slots.Events().empty());
}
} // namespace
