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
  EXPECT_EQ((load >> PtoScheduleDescriptor::kOpcodeShift) & 7u, 1u);
  EXPECT_EQ((extract >> PtoScheduleDescriptor::kOpcodeShift) & 7u, 2u);
  EXPECT_EQ((matmul >> PtoScheduleDescriptor::kOpcodeShift) & 7u, 3u);
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
      R"({"sequence_id":0,"opcode":"TLOAD","input_tiles":[],"scalar_inputs":[],"output_tiles":[{"address":"0x0","shape":[67108864],"dtype":"uint8"}]})"
      "\n");
  PtoTraceProvider provider;
  EXPECT_THROW(provider.load("pto", trace.path()), std::runtime_error);
}

} // namespace
} // namespace gfsim
