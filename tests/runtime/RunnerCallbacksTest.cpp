#include "gfsim/SimModule.h"
#include "gfsim/SystemRunner.h"
#include "gtest/gtest.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace {
class Module final : public gfsim::SimModule {
public:
  Module() : SimModule("test") {}
  unsigned builds = 0, resets = 0, works = 0, transfers = 0;
  bool active = true, failWork = false;
  bool initialized = false;
  void Build() override { ++builds; }
  void Reset() noexcept override { ++resets; }
  void Work() override {
    ++works;
    if (failWork)
      throw std::runtime_error("fault");
  }
  void Xfer() noexcept override { ++transfers; }
  bool HasWork() const noexcept override { return active; }
};
class System final : public gfsim::SimSystem {
  bool Precheck() noexcept override { return true; }
};
struct Fixture {
  Module module;
  System system;
  gfsim::ObservationSlots slots;
  unsigned initializations = 0, drives = 0, samples = 0;
  unsigned limit = 2;
  bool injectFailure = false;
  Fixture() {
    EXPECT_TRUE(slots.Configure({}, 0));
    EXPECT_TRUE(system.AttachObservations(slots));
    EXPECT_TRUE(system.AddModule(module));
  }
  static void initialize(void *p) {
    auto &self = *static_cast<Fixture *>(p);
    EXPECT_EQ(self.module.resets, 0u);
    EXPECT_EQ(self.module.works, 0u);
    ++self.initializations;
    self.module.initialized = true;
  }
  static bool drive(void *p, std::uint64_t epoch) {
    auto &self = *static_cast<Fixture *>(p);
    EXPECT_TRUE(self.module.initialized);
    EXPECT_EQ(self.module.resets, 1u);
    EXPECT_EQ(epoch, self.system.cycle());
    ++self.drives;
    self.module.failWork = self.injectFailure && epoch == 1;
    return epoch < self.limit;
  }
  static void sample(void *p, std::uint64_t committedEpoch) {
    auto &self = *static_cast<Fixture *>(p);
    ++self.samples;
    EXPECT_EQ(committedEpoch, self.samples);
    EXPECT_EQ(self.system.cycle(), committedEpoch);
    EXPECT_EQ(self.module.transfers, committedEpoch + 1);
  }
  gfsim::RunnerCallbacks callbacks() {
    return {this, initialize, drive, sample};
  }
};
int run(Fixture &fixture, std::string domainLimits = "{}",
        std::string maxTicks = "32") {
  const auto suffix =
      std::chrono::steady_clock::now().time_since_epoch().count();
  const auto directory =
      std::filesystem::temp_directory_path() /
      ("pycircuit-runner-callbacks-" + std::to_string(suffix));
  std::filesystem::create_directory(directory);
  const auto path = directory / "config.json";
  {
    std::ofstream file(path);
    file << "{\"deadlock_window\":null,\"max_domain_cycles\":" << domainLimits
         << ",\"max_ticks\":" << maxTicks
         << ",\"schema\":\"pycircuit-model-config\",\"version\":\"1\"}";
  }
  std::string binary = "runner", flag = "--config", config = path.string();
  char *argv[] = {binary.data(), flag.data(), config.data()};
  gfsim::SystemRunner runner(3, argv);
  EXPECT_TRUE(runner.ready());
  int result =
      runner.Run(fixture.system, fixture.slots, {}, fixture.callbacks());
  std::filesystem::remove_all(directory);
  return result;
}
TEST(RunnerCallbacksTest, DomainLimitRejectsBeforeAnyModelOrHostCallback) {
  for (const std::string ticks : {"null", "32"}) {
    Fixture fixture;
    EXPECT_EQ(run(fixture, "{\"default\":3}", ticks), 2);
    EXPECT_EQ(fixture.module.builds, 0u);
    EXPECT_EQ(fixture.module.resets, 0u);
    EXPECT_EQ(fixture.module.works, 0u);
    EXPECT_EQ(fixture.module.transfers, 0u);
    EXPECT_EQ(fixture.initializations, 0u);
    EXPECT_EQ(fixture.drives, 0u);
    EXPECT_EQ(fixture.samples, 0u);
    EXPECT_EQ(fixture.system.cycle(), 0u);
  }
}
TEST(RunnerCallbacksTest,
     InitializationPrecedesResetAndDriveStopsWithoutExtraStep) {
  Fixture fixture;
  EXPECT_EQ(run(fixture), 0);
  EXPECT_EQ(fixture.initializations, 1u);
  EXPECT_EQ(fixture.module.builds, 1u);
  EXPECT_EQ(fixture.module.resets, 1u);
  EXPECT_EQ(fixture.module.works, 2u);
  EXPECT_EQ(fixture.module.transfers, 3u);
  EXPECT_EQ(fixture.samples, 2u);
  EXPECT_EQ(fixture.drives, 3u);
  EXPECT_EQ(fixture.system.cycle(), 2u);
}
TEST(RunnerCallbacksTest, FailureAndQuiescenceNeverProduceSuccessfulSample) {
  Fixture failing;
  failing.injectFailure = true;
  EXPECT_NE(run(failing), 0);
  EXPECT_EQ(failing.module.works, 2u);
  EXPECT_EQ(failing.samples, 1u);
  EXPECT_EQ(failing.module.transfers, 2u);
  EXPECT_EQ(failing.system.cycle(), 1u);
  Fixture quiescent;
  quiescent.module.active = false;
  EXPECT_EQ(run(quiescent), 0);
  EXPECT_EQ(quiescent.module.works, 0u);
  EXPECT_EQ(quiescent.samples, 0u);
  EXPECT_EQ(quiescent.drives, 1u);
  EXPECT_EQ(quiescent.system.cycle(), 0u);
}
constexpr std::string_view embeddedConfig =
    R"({"deadlock_window":null,"max_domain_cycles":{},"max_ticks":32,"schema":"pycircuit-model-config","version":"1"})";

TEST(RunnerCallbacksTest, EmbeddedFiniteConfigRunsWithoutAnExternalFile) {
  Fixture fixture;
  std::string binary = "generated-system", flag = "--workers", workers = "2";
  char *argv[] = {binary.data(), flag.data(), workers.data()};
  gfsim::SystemRunner runner(3, argv, embeddedConfig);
  ASSERT_TRUE(runner.ready());
  EXPECT_EQ(runner.workers(), 2u);
  EXPECT_EQ(runner.Run(fixture.system, fixture.slots, {}, fixture.callbacks()), 0);
  EXPECT_EQ(fixture.initializations, 1u);
  EXPECT_EQ(fixture.samples, 2u);
  EXPECT_EQ(fixture.system.cycle(), 2u);
}

TEST(RunnerCallbacksTest, EmbeddedConfigUsesTheExistingFiniteLimitValidator) {
  const std::vector<std::string> invalid = {
      "not JSON", "{}",
      R"({"deadlock_window":null,"max_domain_cycles":{},"max_ticks":null,"schema":"pycircuit-model-config","version":"1"})",
      R"({"deadlock_window":null,"max_domain_cycles":{},"max_ticks":0,"schema":"pycircuit-model-config","version":"1"})"};
  std::string binary = "generated-system";
  char *argv[] = {binary.data()};
  for (const auto &config : invalid) {
    Fixture fixture;
    gfsim::SystemRunner runner(1, argv, config);
    EXPECT_FALSE(runner.ready());
    EXPECT_EQ(runner.Run(fixture.system, fixture.slots, {}, fixture.callbacks()), 2);
    EXPECT_EQ(fixture.module.builds, 0u);
    EXPECT_EQ(fixture.initializations, 0u);
    EXPECT_EQ(fixture.samples, 0u);
  }
}

TEST(RunnerCallbacksTest, ExplicitConfigFailureNeverFallsBackToEmbeddedConfig) {
  Fixture fixture;
  std::string binary = "generated-system", flag = "--config";
  std::string path =
      (std::filesystem::temp_directory_path() /
       ("pycircuit-no-config-" + std::to_string(
                                    std::chrono::steady_clock::now()
                                        .time_since_epoch().count())))
          .string();
  char *argv[] = {binary.data(), flag.data(), path.data()};
  gfsim::SystemRunner runner(3, argv, embeddedConfig);
  EXPECT_FALSE(runner.ready());
  EXPECT_EQ(runner.Run(fixture.system, fixture.slots, {}, fixture.callbacks()), 2);
  EXPECT_EQ(fixture.module.builds, 0u);
  EXPECT_EQ(fixture.initializations, 0u);
}
} // namespace
