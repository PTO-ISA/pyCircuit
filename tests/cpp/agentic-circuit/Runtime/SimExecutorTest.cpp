#include "gfsim/SimExecutor.h"
#include "gfsim/SimDFF.h"
#include "gtest/gtest.h"

#include <array>
#include <cstdint>
#include <locale>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

std::string bufferText(const AgenticModelBufferV1 &buffer) {
  return buffer.data ? std::string(reinterpret_cast<const char *>(buffer.data),
                                   static_cast<size_t>(buffer.size))
                     : std::string{};
}

AgenticModelStatusV1 configure(gfsim::SimExecutor &executor,
                               const std::string &json) {
  return executor.ConfigureJson(
      reinterpret_cast<const std::uint8_t *>(json.data()), json.size());
}

class ExecutorModule final : public gfsim::SimModule {
public:
  ExecutorModule(gfsim::ObservationSlots &slots, int initial = 3)
      : SimModule("executor-module"), slots_(slots), state_(initial) {}

  int value() const noexcept { return state_.Read(); }

  bool hasWork = true;
  bool throwOnBuild = false;
  bool throwOnWork = false;
  bool write = true;
  bool checkPasses = true;
  int nextValue = 7;
  int observed = -1;
  unsigned builds = 0, works = 0, xfers = 0, discards = 0, resets = 0;

private:
  void Build() override {
    ++builds;
    if (throwOnBuild)
      throw std::runtime_error("build failure");
  }
  void Work(std::uint64_t epoch) override {
    ++works;
    if (throwOnWork)
      throw std::runtime_error("work failure");
    observed = state_.Read();
    staged_ = true;
    stagedEpoch_ = epoch;
    ASSERT_TRUE(slots_.Stage(0, gfsim::SlotValue::Unsigned(observed), true,
                             true, epoch));
  }
  void Xfer() noexcept override {
    ++xfers;
    if (staged_)
      state_.Write(nextValue, write);
    state_.Xfer();
    staged_ = false;
  }
  void DiscardNext() noexcept override {
    ++discards;
    staged_ = false;
    state_.DiscardNext();
  }
  void Reset() noexcept override {
    ++resets;
    staged_ = false;
    state_.Reset();
  }
  void ReportStat() override {}
  bool HasWork() const noexcept override { return hasWork; }

  gfsim::ObservationSlots &slots_;
  gfsim::SimDFFE<int> state_;
  bool staged_ = false;
  std::uint64_t stagedEpoch_ = 0;
};

class ExecutorSystem final : public gfsim::SimSystem {
public:
  explicit ExecutorSystem(ExecutorModule *module = nullptr) : module(module) {}
  ExecutorModule *module;

private:
  bool Precommit(std::uint64_t) noexcept override {
    return !module || module->checkPasses;
  }
};

struct Fixture {
  Fixture(bool withModule = true, bool attachExpectedSlots = true)
      : module(slots), system(withModule ? &module : nullptr) {
    const std::array descriptors{gfsim::ObservationDescriptor{
        9, 1, 1, 1, gfsim::ObservationKind::Gauge}};
    EXPECT_TRUE(slots.Configure(descriptors, 0));
    EXPECT_TRUE(system.AttachObservations(attachExpectedSlots ? slots : other));
    if (withModule)
      EXPECT_TRUE(system.AddModule(module));
  }

  gfsim::ObservationSlots slots;
  gfsim::ObservationSlots other;
  ExecutorModule module;
  ExecutorSystem system;
};

std::vector<gfsim::ReportGaugeDescriptor> reports() {
  return {{9, "root", "completed"}};
}

TEST(SimExecutorTest, AdmissionStateRetryAndOutputPreservation) {
  Fixture fixture;
  auto metadata = reports();
  gfsim::SimExecutor executor(fixture.system, fixture.slots, metadata);
  metadata.front().objectPath = "mutated";
  metadata.front().name = "mutated";
  metadata.clear();
  EXPECT_TRUE(executor.created());
  EXPECT_EQ(executor.state(), gfsim::SimExecutorState::Created);
  EXPECT_EQ(fixture.module.builds, 1u);

  AgenticModelStepResultV1 untouched{7, 77, 88, 99, 111};
  const auto original = untouched;
  EXPECT_EQ(executor.Step(&untouched), AGENTIC_MODEL_STATUS_V1_ABI_MISMATCH);
  EXPECT_EQ(untouched.struct_size, original.struct_size);
  EXPECT_EQ(untouched.state, original.state);
  EXPECT_EQ(untouched.epoch_time, original.epoch_time);
  EXPECT_EQ(untouched.epoch_delta, original.epoch_delta);
  EXPECT_EQ(untouched.reserved, original.reserved);
  EXPECT_EQ(executor.Step(nullptr), AGENTIC_MODEL_STATUS_V1_INVALID_ARGUMENT);
  EXPECT_EQ(executor.cycles(), 0u);
  AgenticModelStepResultV1 notReady{sizeof(notReady), 77, 88, 99, 111};
  const auto notReadyBefore = notReady;
  EXPECT_EQ(executor.Step(&notReady), AGENTIC_MODEL_STATUS_V1_INVALID_STATE);
  EXPECT_EQ(notReady.state, notReadyBefore.state);
  EXPECT_EQ(notReady.epoch_time, notReadyBefore.epoch_time);
  AgenticModelBufferV1 untouchedBuffer{
      reinterpret_cast<const std::uint8_t *>(0x1), 77};
  EXPECT_EQ(executor.StatisticsJson(&untouchedBuffer),
            AGENTIC_MODEL_STATUS_V1_INVALID_STATE);
  EXPECT_EQ(untouchedBuffer.data, reinterpret_cast<const std::uint8_t *>(0x1));
  EXPECT_EQ(untouchedBuffer.size, 77u);

  EXPECT_EQ(configure(executor, R"({"unknown":1})"),
            AGENTIC_MODEL_STATUS_V1_INVALID_ARGUMENT);
  EXPECT_EQ(executor.state(), gfsim::SimExecutorState::Created);
  AgenticModelBufferV1 error{};
  ASSERT_EQ(executor.LastError(&error), AGENTIC_MODEL_STATUS_V1_OK);
  EXPECT_NE(bufferText(error).find("invalid_argument"), std::string::npos);
  EXPECT_EQ(configure(executor, "{}"), AGENTIC_MODEL_STATUS_V1_OK);
  EXPECT_EQ(executor.state(), gfsim::SimExecutorState::Configured);
  EXPECT_EQ(executor.Reset(), AGENTIC_MODEL_STATUS_V1_OK);
  EXPECT_EQ(executor.state(), gfsim::SimExecutorState::Ready);
  EXPECT_EQ(fixture.module.builds, 1u);
  ASSERT_EQ(executor.LastError(&error), AGENTIC_MODEL_STATUS_V1_OK);
  EXPECT_EQ(error.size, 0u);
}

TEST(SimExecutorTest, ConstructorRejectsBuildAndReportMetadataFailures) {
  {
    Fixture fixture;
    fixture.module.throwOnBuild = true;
    auto metadata = reports();
    gfsim::SimExecutor executor(fixture.system, fixture.slots, metadata);
    EXPECT_FALSE(executor.created());
    EXPECT_EQ(executor.state(), gfsim::SimExecutorState::Failed);
    AgenticModelBufferV1 error{};
    ASSERT_EQ(executor.LastError(&error), AGENTIC_MODEL_STATUS_V1_OK);
    EXPECT_NE(bufferText(error).find("Build failed"), std::string::npos);
  }
  {
    Fixture fixture;
    auto metadata = reports();
    metadata.push_back(metadata.front());
    gfsim::SimExecutor executor(fixture.system, fixture.slots, metadata);
    EXPECT_FALSE(executor.created());
    EXPECT_EQ(executor.state(), gfsim::SimExecutorState::Failed);
  }
}

TEST(SimExecutorTest, CanonicalConfigLimitsAndTiePriority) {
  const std::string canonical =
      R"({"deadlock_window":null,"max_domain_cycles":{"default":1},"max_ticks":1,"schema":"agentic-model-config","version":"1"})";
  for (const std::string &json :
       std::vector<std::string>{"{}", "{}\n", canonical}) {
    Fixture fixture;
    auto metadata = reports();
    gfsim::SimExecutor executor(fixture.system, fixture.slots, metadata);
    ASSERT_EQ(configure(executor, json), AGENTIC_MODEL_STATUS_V1_OK) << json;
  }
  for (const std::string &json : {
           " { }",
           "{}\n\n",
           R"({"deadlock_window":1})",
           R"({"max_ticks":0})",
           R"({"max_ticks":-1})",
           R"({"max_ticks":18446744073709551616})",
           R"({"max_domain_cycles":{"other":1}})",
           R"({"max_ticks":1,"max_ticks":1})",
       }) {
    Fixture fixture;
    auto metadata = reports();
    gfsim::SimExecutor executor(fixture.system, fixture.slots, metadata);
    EXPECT_EQ(configure(executor, json),
              AGENTIC_MODEL_STATUS_V1_INVALID_ARGUMENT)
        << json;
    EXPECT_TRUE(executor.created());
  }
  {
    Fixture fixture;
    auto metadata = reports();
    gfsim::SimExecutor executor(fixture.system, fixture.slots, metadata);
    const std::string invalidUtf8(1, static_cast<char>(0xff));
    EXPECT_EQ(configure(executor, invalidUtf8),
              AGENTIC_MODEL_STATUS_V1_INVALID_ARGUMENT);
    EXPECT_TRUE(executor.created());
  }

  Fixture fixture;
  auto metadata = reports();
  gfsim::SimExecutor executor(fixture.system, fixture.slots, metadata);
  ASSERT_EQ(configure(executor, canonical), AGENTIC_MODEL_STATUS_V1_OK);
  ASSERT_EQ(executor.Reset(), AGENTIC_MODEL_STATUS_V1_OK);
  AgenticModelStepResultV1 result{sizeof(result)};
  ASSERT_EQ(executor.Step(&result), AGENTIC_MODEL_STATUS_V1_OK);
  EXPECT_EQ(result.state, AGENTIC_MODEL_STEP_V1_TERMINATED);
  EXPECT_EQ(result.epoch_time, 1u);
  EXPECT_EQ(executor.state(), gfsim::SimExecutorState::Completed);
  const auto completedResult = result;
  EXPECT_EQ(executor.Step(&result), AGENTIC_MODEL_STATUS_V1_INVALID_STATE);
  EXPECT_EQ(result.state, completedResult.state);
  EXPECT_EQ(result.epoch_time, completedResult.epoch_time);
  AgenticModelBufferV1 statistics{};
  ASSERT_EQ(executor.StatisticsJson(&statistics), AGENTIC_MODEL_STATUS_V1_OK);
  EXPECT_NE(
      bufferText(statistics)
          .find(
              R"("name":"stop_reason","object_path":"@runtime","sum":0,"value":1)"),
      std::string::npos);
}

TEST(SimExecutorTest, OldQCommitDiscardFailureAndResetRecovery) {
  Fixture fixture;
  auto metadata = reports();
  gfsim::SimExecutor executor(fixture.system, fixture.slots, metadata);
  ASSERT_EQ(configure(executor, "{}"), AGENTIC_MODEL_STATUS_V1_OK);
  ASSERT_EQ(executor.Reset(), AGENTIC_MODEL_STATUS_V1_OK);
  AgenticModelStepResultV1 result{sizeof(result)};
  ASSERT_EQ(executor.Step(&result), AGENTIC_MODEL_STATUS_V1_OK);
  EXPECT_EQ(result.state, AGENTIC_MODEL_STEP_V1_RUNNING);
  EXPECT_EQ(fixture.module.observed, 3);
  EXPECT_EQ(fixture.module.value(), 7);
  EXPECT_EQ(executor.cycles(), 1u);

  fixture.module.write = false;
  fixture.module.nextValue = 11;
  ASSERT_EQ(executor.Step(&result), AGENTIC_MODEL_STATUS_V1_OK);
  EXPECT_EQ(result.state, AGENTIC_MODEL_STEP_V1_RUNNING);
  EXPECT_EQ(fixture.module.value(), 7);
  EXPECT_EQ(executor.cycles(), 2u);

  fixture.system.module->checkPasses = false;
  EXPECT_EQ(executor.Step(&result), AGENTIC_MODEL_STATUS_V1_RUNTIME_FAILURE);
  EXPECT_EQ(result.state, AGENTIC_MODEL_STEP_V1_FAILED);
  EXPECT_EQ(result.epoch_time, 2u);
  EXPECT_EQ(fixture.module.value(), 7);
  EXPECT_EQ(executor.state(), gfsim::SimExecutorState::Failed);
  AgenticModelBufferV1 firstError{};
  ASSERT_EQ(executor.LastError(&firstError), AGENTIC_MODEL_STATUS_V1_OK);
  const std::string committedError = bufferText(firstError);
  EXPECT_NE(committedError.find(R"("phase":"check")"), std::string::npos);
  const unsigned discarded = fixture.module.discards;
  EXPECT_EQ(executor.Step(&result), AGENTIC_MODEL_STATUS_V1_INVALID_STATE);
  EXPECT_EQ(fixture.module.discards, discarded);
  AgenticModelBufferV1 repeatedError{};
  ASSERT_EQ(executor.LastError(&repeatedError), AGENTIC_MODEL_STATUS_V1_OK);
  EXPECT_EQ(bufferText(repeatedError), committedError);
  EXPECT_EQ(executor.Step(nullptr), AGENTIC_MODEL_STATUS_V1_INVALID_ARGUMENT);
  ASSERT_EQ(executor.LastError(&repeatedError), AGENTIC_MODEL_STATUS_V1_OK);
  EXPECT_EQ(bufferText(repeatedError), committedError);
  AgenticModelStepResultV1 wrongLayout{7, 71, 72, 73, 74};
  const auto wrongLayoutBefore = wrongLayout;
  EXPECT_EQ(executor.Step(&wrongLayout), AGENTIC_MODEL_STATUS_V1_ABI_MISMATCH);
  EXPECT_EQ(wrongLayout.state, wrongLayoutBefore.state);
  EXPECT_EQ(wrongLayout.epoch_time, wrongLayoutBefore.epoch_time);
  ASSERT_EQ(executor.LastError(&repeatedError), AGENTIC_MODEL_STATUS_V1_OK);
  EXPECT_EQ(bufferText(repeatedError), committedError);
  EXPECT_EQ(configure(executor, "{}"), AGENTIC_MODEL_STATUS_V1_INVALID_STATE);
  ASSERT_EQ(executor.LastError(&repeatedError), AGENTIC_MODEL_STATUS_V1_OK);
  EXPECT_EQ(bufferText(repeatedError), committedError);

  fixture.module.checkPasses = true;
  fixture.module.write = true;
  ASSERT_EQ(executor.Reset(), AGENTIC_MODEL_STATUS_V1_OK);
  EXPECT_EQ(executor.state(), gfsim::SimExecutorState::Ready);
  EXPECT_EQ(executor.cycles(), 0u);
  EXPECT_EQ(fixture.module.value(), 3);
  ASSERT_EQ(executor.Step(&result), AGENTIC_MODEL_STATUS_V1_OK);
  EXPECT_EQ(fixture.module.observed, 3);
  EXPECT_EQ(fixture.module.value(), 11);
}

TEST(SimExecutorTest, WorkExceptionUsesEvaluatePhaseAndResetRecovers) {
  Fixture fixture;
  fixture.module.throwOnWork = true;
  auto metadata = reports();
  gfsim::SimExecutor executor(fixture.system, fixture.slots, metadata);
  ASSERT_EQ(configure(executor, "{}"), AGENTIC_MODEL_STATUS_V1_OK);
  ASSERT_EQ(executor.Reset(), AGENTIC_MODEL_STATUS_V1_OK);
  AgenticModelStepResultV1 result{sizeof(result)};
  EXPECT_EQ(executor.Step(&result), AGENTIC_MODEL_STATUS_V1_RUNTIME_FAILURE);
  EXPECT_EQ(result.state, AGENTIC_MODEL_STEP_V1_FAILED);
  EXPECT_EQ(result.epoch_time, 0u);
  EXPECT_EQ(fixture.module.value(), 3);
  AgenticModelBufferV1 error{};
  ASSERT_EQ(executor.LastError(&error), AGENTIC_MODEL_STATUS_V1_OK);
  EXPECT_NE(bufferText(error).find(R"("phase":"evaluate")"), std::string::npos);
  fixture.module.throwOnWork = false;
  ASSERT_EQ(executor.Reset(), AGENTIC_MODEL_STATUS_V1_OK);
  EXPECT_EQ(executor.state(), gfsim::SimExecutorState::Ready);
  EXPECT_EQ(executor.cycles(), 0u);
}

TEST(SimExecutorTest, QuiescentDiffersFromClockedHold) {
  Fixture idle(false);
  auto idleMetadata = reports();
  gfsim::SimExecutor idleExecutor(idle.system, idle.slots, idleMetadata);
  ASSERT_EQ(configure(idleExecutor, "{}"), AGENTIC_MODEL_STATUS_V1_OK);
  ASSERT_EQ(idleExecutor.Reset(), AGENTIC_MODEL_STATUS_V1_OK);
  AgenticModelStepResultV1 idleResult{sizeof(idleResult)};
  ASSERT_EQ(idleExecutor.Step(&idleResult), AGENTIC_MODEL_STATUS_V1_OK);
  EXPECT_EQ(idleResult.state, AGENTIC_MODEL_STEP_V1_QUIESCENT);
  EXPECT_EQ(idleResult.epoch_time, 0u);
  EXPECT_EQ(idleExecutor.state(), gfsim::SimExecutorState::Ready);

  Fixture hold;
  hold.module.write = false;
  auto holdMetadata = reports();
  gfsim::SimExecutor holdExecutor(hold.system, hold.slots, holdMetadata);
  ASSERT_EQ(configure(holdExecutor, "{}"), AGENTIC_MODEL_STATUS_V1_OK);
  ASSERT_EQ(holdExecutor.Reset(), AGENTIC_MODEL_STATUS_V1_OK);
  AgenticModelStepResultV1 holdResult{sizeof(holdResult)};
  ASSERT_EQ(holdExecutor.Step(&holdResult), AGENTIC_MODEL_STATUS_V1_OK);
  EXPECT_EQ(holdResult.state, AGENTIC_MODEL_STEP_V1_RUNNING);
  EXPECT_EQ(holdResult.epoch_time, 1u);
  EXPECT_EQ(hold.module.value(), 3);
}

TEST(SimExecutorTest, StatisticsSnapshotReportResetAndBufferLifetime) {
  Fixture fixture;
  std::vector<gfsim::ReportGaugeDescriptor> metadata{
      {9, std::string("root"), std::string("completed")}};
  gfsim::SimExecutor executor(fixture.system, fixture.slots, metadata);
  metadata.clear();
  ASSERT_EQ(configure(executor, "{}"), AGENTIC_MODEL_STATUS_V1_OK);
  ASSERT_EQ(executor.Reset(), AGENTIC_MODEL_STATUS_V1_OK);
  AgenticModelStepResultV1 result{sizeof(result)};
  ASSERT_EQ(executor.Step(&result), AGENTIC_MODEL_STATUS_V1_OK);
  AgenticModelBufferV1 statistics{reinterpret_cast<const uint8_t *>(0x1), 77};
  ASSERT_EQ(executor.StatisticsJson(&statistics), AGENTIC_MODEL_STATUS_V1_OK);
  std::string committed = bufferText(statistics);
  EXPECT_NE(committed.find(R"("object_path":"root")"), std::string::npos);
  EXPECT_NE(committed.find(R"("name":"completed")"), std::string::npos);
  EXPECT_NE(committed.find(R"("value":3)"), std::string::npos);

  fixture.module.checkPasses = false;
  EXPECT_EQ(executor.Step(&result), AGENTIC_MODEL_STATUS_V1_RUNTIME_FAILURE);
  AgenticModelBufferV1 afterFailure{};
  ASSERT_EQ(executor.StatisticsJson(&afterFailure), AGENTIC_MODEL_STATUS_V1_OK);
  EXPECT_EQ(bufferText(afterFailure), committed);

  ASSERT_EQ(executor.Reset(), AGENTIC_MODEL_STATUS_V1_OK);
  AgenticModelBufferV1 afterReset{};
  ASSERT_EQ(executor.StatisticsJson(&afterReset), AGENTIC_MODEL_STATUS_V1_OK);
  std::string reset = bufferText(afterReset);
  EXPECT_NE(reset.find(
                R"("name":"completed","object_path":"root","sum":0,"value":0)"),
            std::string::npos);
  EXPECT_NE(reset.find(R"("name":"cycles")"), std::string::npos);
}

TEST(SimExecutorTest, ModelsAreIndependentAndAttachmentIdentityIsExact) {
  Fixture first, second;
  auto firstMetadata = reports();
  auto secondMetadata = reports();
  gfsim::SimExecutor firstExecutor(first.system, first.slots, firstMetadata);
  gfsim::SimExecutor secondExecutor(second.system, second.slots,
                                    secondMetadata);
  ASSERT_EQ(
      configure(
          firstExecutor,
          R"({"deadlock_window":null,"max_domain_cycles":{},"max_ticks":1,"schema":"agentic-model-config","version":"1"})"),
      AGENTIC_MODEL_STATUS_V1_OK);
  ASSERT_EQ(
      configure(
          secondExecutor,
          R"({"deadlock_window":null,"max_domain_cycles":{"default":2},"max_ticks":null,"schema":"agentic-model-config","version":"1"})"),
      AGENTIC_MODEL_STATUS_V1_OK);
  ASSERT_EQ(firstExecutor.Reset(), AGENTIC_MODEL_STATUS_V1_OK);
  ASSERT_EQ(secondExecutor.Reset(), AGENTIC_MODEL_STATUS_V1_OK);
  AgenticModelStepResultV1 firstResult{sizeof(firstResult)};
  AgenticModelStepResultV1 secondResult{sizeof(secondResult)};
  ASSERT_EQ(firstExecutor.Step(&firstResult), AGENTIC_MODEL_STATUS_V1_OK);
  ASSERT_EQ(secondExecutor.Step(&secondResult), AGENTIC_MODEL_STATUS_V1_OK);
  EXPECT_EQ(firstResult.state, AGENTIC_MODEL_STEP_V1_TERMINATED);
  EXPECT_EQ(secondResult.state, AGENTIC_MODEL_STEP_V1_RUNNING);
  EXPECT_EQ(firstExecutor.cycles(), 1u);
  EXPECT_EQ(secondExecutor.cycles(), 1u);
  ASSERT_EQ(secondExecutor.Step(&secondResult), AGENTIC_MODEL_STATUS_V1_OK);
  EXPECT_EQ(secondResult.state, AGENTIC_MODEL_STEP_V1_TERMINATED);
  EXPECT_EQ(secondExecutor.cycles(), 2u);
  AgenticModelBufferV1 domainStatistics{};
  ASSERT_EQ(secondExecutor.StatisticsJson(&domainStatistics),
            AGENTIC_MODEL_STATUS_V1_OK);
  EXPECT_NE(
      bufferText(domainStatistics)
          .find(
              R"("name":"stop_reason","object_path":"@runtime","sum":0,"value":2)"),
      std::string::npos);

  Fixture wrongAttachment(true, false);
  auto wrongMetadata = reports();
  gfsim::SimExecutor rejected(wrongAttachment.system, wrongAttachment.slots,
                              wrongMetadata);
  EXPECT_FALSE(rejected.created());
  EXPECT_EQ(rejected.state(), gfsim::SimExecutorState::Failed);
  AgenticModelBufferV1 error{};
  ASSERT_EQ(rejected.LastError(&error), AGENTIC_MODEL_STATUS_V1_OK);
  EXPECT_NE(bufferText(error).find("report gauge metadata is invalid"),
            std::string::npos);
}

class GroupedDigits final : public std::numpunct<char> {
private:
  char do_thousands_sep() const override { return '_'; }
  std::string do_grouping() const override { return "\3"; }
};

TEST(SimExecutorTest, StatisticsUseClassicLocale) {
  Fixture fixture;
  auto metadata = reports();
  gfsim::SimExecutor executor(fixture.system, fixture.slots, metadata);
  ASSERT_EQ(
      configure(
          executor,
          R"({"deadlock_window":null,"max_domain_cycles":{},"max_ticks":1000,"schema":"agentic-model-config","version":"1"})"),
      AGENTIC_MODEL_STATUS_V1_OK);
  ASSERT_EQ(executor.Reset(), AGENTIC_MODEL_STATUS_V1_OK);
  AgenticModelStepResultV1 result{sizeof(result)};
  for (unsigned index = 0; index != 1000; ++index)
    ASSERT_EQ(executor.Step(&result), AGENTIC_MODEL_STATUS_V1_OK);
  ASSERT_EQ(result.state, AGENTIC_MODEL_STEP_V1_TERMINATED);

  const std::locale previous = std::locale();
  std::locale::global(std::locale(previous, new GroupedDigits));
  AgenticModelBufferV1 statistics{};
  EXPECT_EQ(executor.StatisticsJson(&statistics), AGENTIC_MODEL_STATUS_V1_OK);
  std::locale::global(previous);
  const std::string text = bufferText(statistics);
  EXPECT_EQ(text.find("1_000"), std::string::npos);
  EXPECT_NE(text.find(R"("name":"cycles")"), std::string::npos);
  EXPECT_NE(text.find(R"("value":1000)"), std::string::npos);
}

} // namespace
