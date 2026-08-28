#include "gfsim/object.h"
#include "gfsim/pto_trace.h"

#include "gtest/gtest.h"

#include <cstdio>
#include <fstream>
#include <stdexcept>
#include <string>

namespace gfsim {
namespace {

class TemporaryTrace {
public:
  explicit TemporaryTrace(std::string contents)
      : path_("/tmp/agentic-circuit-pto-trace-test.jsonl") {
    std::ofstream output(path_);
    output << contents;
  }
  ~TemporaryTrace() { std::remove(path_.c_str()); }
  const std::string &path() const { return path_; }

private:
  std::string path_;
};

const char *kThreeRecordTrace =
    R"({"sequence_id":0,"opcode":"TLOAD","input_tiles":[{"address":"0x1000","shape":[4,4],"dtype":"float32"}],"scalar_inputs":[],"output_tiles":[{"address":"0x0","shape":[4,4],"dtype":"float32"}]})"
    "\n"
    R"({"sequence_id":1,"opcode":"TEXTRACT","input_tiles":[{"address":"0x0","shape":[4,4],"dtype":"float32"}],"scalar_inputs":[],"output_tiles":[{"address":"0x40","shape":[2,2],"dtype":"float32"}]})"
    "\n"
    R"({"sequence_id":2,"opcode":"TMATMUL","input_tiles":[{"address":"0x40","shape":[2,2],"dtype":"float32"},{"address":"0x80","shape":[2,2],"dtype":"float32"}],"scalar_inputs":[],"output_tiles":[{"address":"0xc0","shape":[2,2],"dtype":"float32"}]})"
    "\n";

TEST(PtoTraceProviderTest, IndexedLookupDoesNotAdvanceImplicitState) {
  TemporaryTrace trace(kThreeRecordTrace);
  PtoTraceProvider provider;
  provider.load("pto", trace.path());

  EXPECT_EQ(provider.open("pto"), 0u);
  TraceNextResult first = provider.next("pto", 0);
  TraceNextResult retried = provider.next("pto", 0);
  EXPECT_TRUE(first.advanced);
  EXPECT_EQ(first.cursor, 1u);
  EXPECT_EQ(first.handle, 0u);
  EXPECT_EQ(retried.cursor, first.cursor);
  EXPECT_EQ(retried.handle, first.handle);
  EXPECT_FALSE(provider.eof("pto", 2));
  EXPECT_TRUE(provider.eof("pto", 3));
}

TEST(PtoTraceProviderTest, BuildsOpcodeWorkloadAndDependencies) {
  TemporaryTrace trace(kThreeRecordTrace);
  PtoTraceProvider provider;
  provider.load("pto", trace.path());

  uint64_t load = provider.decode(0);
  uint64_t extract = provider.decode(1);
  uint64_t matmul = provider.decode(2);
  EXPECT_EQ((load >> PtoScheduleDescriptor::kOpcodeShift) &
                PtoScheduleDescriptor::kOpcodeMask,
            1u);
  EXPECT_EQ((extract >> PtoScheduleDescriptor::kOpcodeShift) &
                PtoScheduleDescriptor::kOpcodeMask,
            2u);
  EXPECT_EQ((matmul >> PtoScheduleDescriptor::kOpcodeShift) &
                PtoScheduleDescriptor::kOpcodeMask,
            3u);
  EXPECT_EQ((load >> PtoScheduleDescriptor::kWorkloadShift) &
                PtoScheduleDescriptor::kMaxWorkload,
            64u);
  EXPECT_EQ((extract >> PtoScheduleDescriptor::kWorkloadShift) &
                PtoScheduleDescriptor::kMaxWorkload,
            16u);
  EXPECT_EQ((matmul >> PtoScheduleDescriptor::kWorkloadShift) &
                PtoScheduleDescriptor::kMaxWorkload,
            8u);
  EXPECT_EQ((extract >> PtoScheduleDescriptor::kDependencyValidShift) & 7u,
            1u);
  EXPECT_EQ((extract >> PtoScheduleDescriptor::kDependency0Shift) & 0xffu,
            0u);
  EXPECT_EQ((matmul >> PtoScheduleDescriptor::kDependencyValidShift) & 7u,
            1u);
  EXPECT_EQ((matmul >> PtoScheduleDescriptor::kDependency0Shift) & 0xffu, 1u);
}

TEST(PtoTraceProviderTest, RejectsNonContiguousSequence) {
  TemporaryTrace trace(
      R"({"sequence_id":1,"opcode":"TASSIGN","input_tiles":[],"scalar_inputs":[],"output_tiles":[]})"
      "\n");
  PtoTraceProvider provider;
  EXPECT_THROW(provider.load("pto", trace.path()), std::runtime_error);
}

TEST(PtoTraceProviderTest, RejectsUnknownOpcode) {
  TemporaryTrace trace(
      R"({"sequence_id":0,"opcode":"NOT_AN_OPCODE","input_tiles":[],"scalar_inputs":[],"output_tiles":[]})"
      "\n");
  PtoTraceProvider provider;
  EXPECT_THROW(provider.load("pto", trace.path()), std::runtime_error);
}

TEST(PtoTraceProviderTest, RejectsWorkloadThatExceedsDescriptor) {
  TemporaryTrace trace(
      R"({"sequence_id":0,"opcode":"TLOAD","input_tiles":[],"scalar_inputs":[],"output_tiles":[{"address":"0x0","shape":[16777216],"dtype":"uint8"}]})"
      "\n");
  PtoTraceProvider provider;
  EXPECT_THROW(provider.load("pto", trace.path()), std::runtime_error);
}

TEST(PtoTraceProviderTest, DecodesFlashAttentionVectorOpcodes) {
  TemporaryTrace trace(
      R"({"sequence_id":0,"opcode":"TLOAD","input_tiles":[],"output_tiles":[{"address":"0x0","shape":[4,4],"dtype":"float16"}]})"
      "\n"
      R"({"sequence_id":1,"opcode":"TCOLMAX","input_tiles":[{"address":"0x0","shape":[4,4],"dtype":"float16"}],"output_tiles":[{"address":"0x40","shape":[4],"dtype":"float16"}]})"
      "\n"
      R"({"sequence_id":2,"opcode":"TEXP","input_tiles":[{"address":"0x40","shape":[4],"dtype":"float16"}],"output_tiles":[{"address":"0x48","shape":[4],"dtype":"float16"}]})"
      "\n");
  PtoTraceProvider provider;
  provider.load("pto", trace.path());
  uint64_t reduce = provider.decode(1);
  uint64_t exp = provider.decode(2);
  EXPECT_EQ((reduce >> PtoScheduleDescriptor::kOpcodeShift) &
                PtoScheduleDescriptor::kOpcodeMask,
            8u);
  EXPECT_EQ((exp >> PtoScheduleDescriptor::kOpcodeShift) &
                PtoScheduleDescriptor::kOpcodeMask,
            11u);
  EXPECT_EQ((reduce >> PtoScheduleDescriptor::kWorkloadShift) &
                PtoScheduleDescriptor::kMaxWorkload,
            32u);
}

TEST(PtoTraceProviderTest, ChromeTraceEmitsDependencyFlows) {
  TemporaryTrace trace(kThreeRecordTrace);
  SimSystem system;
  system.loadPtoTrace("pto", trace.path());
  system.recordTraceEvent("Tlsu", "begin", 0);
  system.recordTraceEvent("Tlsu", "end", 0);
  system.recordTraceEvent("Vector", "begin", 1);
  system.recordTraceEvent("Vector", "end", 1);
  system.recordTraceEvent("Cube", "begin", 2);
  system.recordTraceEvent("Cube", "end", 2);
  system.recordTraceCounter("ROB", 3);
  system.recordTraceCounter("IQVector", 2);
  std::string json = system.chromeTraceJson();
  EXPECT_NE(json.find("\"deps\":[0]"), std::string::npos) << json;
  EXPECT_NE(json.find("\"deps\":[1]"), std::string::npos) << json;
  EXPECT_NE(json.find("\"ph\":\"s\""), std::string::npos) << json;
  EXPECT_NE(json.find("\"ph\":\"f\""), std::string::npos) << json;
  EXPECT_NE(json.find("\"from\":0,\"to\":1"), std::string::npos) << json;
  EXPECT_NE(json.find("\"from\":1,\"to\":2"), std::string::npos) << json;
  EXPECT_LT(json.find("\"ph\":\"B\""), json.find("\"ph\":\"s\""));
  EXPECT_LT(json.find("\"ph\":\"s\""), json.find("\"ph\":\"E\""));
  EXPECT_NE(json.find("\"name\":\"Tlsu\""), std::string::npos) << json;
  EXPECT_NE(json.find("\"name\":\"Vector\""), std::string::npos) << json;
  EXPECT_NE(json.find("\"name\":\"Cube\""), std::string::npos) << json;
  EXPECT_NE(json.find("\"name\":\"ROB occupancy\""), std::string::npos) << json;
  EXPECT_NE(json.find("\"name\":\"IQ Vector occupancy\""), std::string::npos)
      << json;
  EXPECT_NE(json.find("\"ph\":\"C\""), std::string::npos) << json;
  EXPECT_NE(json.find("\"occupancy\":3"), std::string::npos) << json;
  EXPECT_NE(json.find("\"occupancy\":2"), std::string::npos) << json;
  EXPECT_EQ(json.find("capacity"), std::string::npos) << json;
  EXPECT_EQ(json.find("IQ Scalar"), std::string::npos) << json;
  EXPECT_EQ(json.find("\"name\":\"IQ Vector\""), std::string::npos) << json;
  EXPECT_EQ(json.find("\"name\":\"entries\""), std::string::npos) << json;
  EXPECT_EQ(json.find("\"name\":\"Frontend\""), std::string::npos) << json;
  EXPECT_EQ(json.find("\"name\":\"Dispatch\""), std::string::npos) << json;
  EXPECT_EQ(json.find("\"name\":\"Complete\""), std::string::npos) << json;
  EXPECT_EQ(json.find("\"name\":\"ROB\""), std::string::npos) << json;
}

} // namespace
} // namespace gfsim
