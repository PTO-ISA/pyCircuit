#include "acir/Compiler/Driver.h"

#include "llvm/Support/Error.h"
#include "gtest/gtest.h"

#include <string>
#include <utility>
#include <vector>

namespace acir::compiler {
namespace {

constexpr llvm::StringLiteral kValidAcir = R"mlir(
module attributes {ac.contract_epoch = "0.5"} {
  ac.system @main root @top as "root" tick 0 "cycle"
      workload @top::@workload seed {kind = "fixed", value = 0 : i64}
      instrumentation [] results {id = "default", format = "json"} selected true
  ac.module @top() parameters {} graph {
    ac.process @workload kind "workload" {
      ac.yield_sim
    }
    ac.return
  }
}
)mlir";

CompilerRequest validRequest() {
  CompilerRequest request;
  request.acirBytes = kValidAcir.str();
  request.profile = CompilerProfile::Fast;
  request.stopAfter = CompilerStage::AcsimVerify;
  request.emits = {codegen::ArtifactKind::Acir, codegen::ArtifactKind::Acsim};
  return request;
}

std::vector<std::string> paths(const CompilerResult &result) {
  std::vector<std::string> found;
  for (const CompilerArtifact &artifact : result.artifacts)
    found.push_back(artifact.logicalPath);
  return found;
}

std::vector<CompilerDiagnostic> diagnostics(llvm::Error error) {
  std::vector<CompilerDiagnostic> found;
  llvm::handleAllErrors(std::move(error), [&](const CompilerError &failure) {
    found = failure.diagnostics();
  });
  return found;
}

TEST(CompilerDriverTest, StandardPipelineProducesVerifiedStageArtifacts) {
  auto result = runCompiler(validRequest());
  if (!result) {
    ADD_FAILURE() << llvm::toString(result.takeError());
    return;
  }
  EXPECT_EQ(paths(*result),
            (std::vector<std::string>{"frozen.ac.mlir", "model.acsim.mlir"}));
  EXPECT_TRUE(result->diagnostics.empty());
  for (const CompilerArtifact &artifact : result->artifacts) {
    EXPECT_FALSE(artifact.bytes.empty());
    EXPECT_TRUE(codegen::isValidFingerprint(artifact.sha256));
  }
}

TEST(CompilerDriverTest, ParseFailureReturnsStableStructuredDiagnostic) {
  CompilerRequest request = validRequest();
  request.acirBytes = "not mlir";

  auto result = runCompiler(request);
  ASSERT_FALSE(static_cast<bool>(result));
  auto found = diagnostics(result.takeError());
  ASSERT_FALSE(found.empty());
  EXPECT_EQ(found.front().stage, "acir-parse");
  EXPECT_EQ(found.front().code, "ACIR-PARSE-001");
  EXPECT_EQ(found.front().severity, "error");
  EXPECT_FALSE(found.front().message.empty());
}

TEST(CompilerDriverTest, CustomProfileRequiresAnExplicitPipeline) {
  CompilerRequest request = validRequest();
  request.profile = CompilerProfile::Custom;

  auto result = runCompiler(request);
  ASSERT_FALSE(static_cast<bool>(result));
  auto found = diagnostics(result.takeError());
  ASSERT_EQ(found.size(), 1u);
  EXPECT_EQ(found.front().code, "ACIR-PIPELINE-001");
}

TEST(CompilerDriverTest, CustomPipelineRunsInProcess) {
  CompilerRequest request = validRequest();
  request.profile = CompilerProfile::Custom;
  request.customPipeline = "builtin.module(ac-canonicalize-model)";
  request.stopAfter = CompilerStage::AcirNormalize;
  request.emits.clear();

  auto result = runCompiler(request);
  if (!result) {
    ADD_FAILURE() << llvm::toString(result.takeError());
    return;
  }
  EXPECT_TRUE(result->diagnostics.empty());
}

TEST(CompilerDriverTest, StageDumpsAreDeterministicAndContentAddressed) {
  CompilerRequest request = validRequest();
  request.stopAfter = CompilerStage::AcirFreeze;
  request.emits = {codegen::ArtifactKind::Acir};
  request.dumpBefore = {"acir-freeze"};
  request.dumpAfter = {"acir-freeze"};

  auto first = runCompiler(request);
  auto second = runCompiler(request);
  ASSERT_TRUE(static_cast<bool>(first));
  ASSERT_TRUE(static_cast<bool>(second));
  ASSERT_EQ(first->artifacts.size(), 3u);
  ASSERT_EQ(second->artifacts.size(), 3u);
  for (size_t index = 0; index < first->artifacts.size(); ++index) {
    EXPECT_EQ(first->artifacts[index].logicalPath,
              second->artifacts[index].logicalPath);
    EXPECT_EQ(first->artifacts[index].bytes, second->artifacts[index].bytes);
    EXPECT_EQ(first->artifacts[index].sha256, second->artifacts[index].sha256);
  }
}

TEST(CompilerDriverTest, RejectsArtifactBeyondSelectedStopStage) {
  CompilerRequest request = validRequest();
  request.stopAfter = CompilerStage::AcirFreeze;

  auto result = runCompiler(request);
  ASSERT_FALSE(static_cast<bool>(result));
  auto found = diagnostics(result.takeError());
  ASSERT_EQ(found.size(), 1u);
  EXPECT_EQ(found.front().code, "ACIR-EMIT-001");
}

} // namespace
} // namespace acir::compiler
