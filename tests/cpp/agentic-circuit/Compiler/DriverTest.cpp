#include "acir/Compiler/Driver.h"
#include "llvm/Support/Error.h"
#include "gtest/gtest.h"

namespace acir::compiler {
namespace {

constexpr llvm::StringLiteral kValidAcir = R"mlir(
module  {
  ac.system @main root @top as "root" tick 0 "cycle"
      workload @top::@workload seed {kind = "fixed", value = 0 : i64}
      instrumentation [] results {id = "default", format = "json"} selected true
  ac.module @top() parameters {} graph {
    ac.process @workload kind "workload" { ac.yield_sim }
    ac.return
  }
}
)mlir";

CompilerRequest validRequest() {
  CompilerRequest request;
  request.acirBytes = kValidAcir.str();
  request.stopAfter = CompilerStage::TopologyClosure;
  request.emits = {ArtifactKind::Acir};
  return request;
}

std::vector<CompilerDiagnostic> diagnostics(llvm::Error error) {
  std::vector<CompilerDiagnostic> found;
  llvm::handleAllErrors(std::move(error), [&](const CompilerError &failure) {
    found = failure.diagnostics();
  });
  return found;
}

TEST(CompilerDriverTest, FreezesAndReturnsStructuralArtifact) {
  auto result = runCompiler(validRequest());
  ASSERT_TRUE(bool(result)) << llvm::toString(result.takeError());
  ASSERT_EQ(result->artifacts.size(), 1u);
  EXPECT_EQ(result->artifacts.front().logicalPath, "verified.ac.mlir");
  EXPECT_EQ(result->artifacts.front().kind, ArtifactKind::Acir);
  EXPECT_NE(result->artifacts.front().bytes.find("ac.topology_frozen = true"),
            std::string::npos);
}

TEST(CompilerDriverTest, ParseFailureReturnsStructuredDiagnostic) {
  CompilerRequest request = validRequest();
  request.acirBytes = "not mlir";
  auto result = runCompiler(request);
  ASSERT_FALSE(bool(result));
  auto found = diagnostics(result.takeError());
  ASSERT_FALSE(found.empty());
  EXPECT_EQ(found.front().stage, "acir-parse");
  EXPECT_EQ(found.front().code, "ACIR-PARSE-001");
}

TEST(CompilerDriverTest, CustomProfileRequiresPipeline) {
  CompilerRequest request = validRequest();
  request.profile = CompilerProfile::Custom;
  auto result = runCompiler(request);
  ASSERT_FALSE(bool(result));
  auto found = diagnostics(result.takeError());
  ASSERT_EQ(found.size(), 1u);
  EXPECT_EQ(found.front().code, "ACIR-PIPELINE-001");
}

} // namespace
} // namespace acir::compiler
