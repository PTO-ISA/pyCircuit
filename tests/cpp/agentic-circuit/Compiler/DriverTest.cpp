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
  ac.module @top() parameters {} attributes {ac.definition_name = "top"} graph {
    ac.process @workload kind "workload" { ac.yield_sim }
    ac.return
  }
}
)mlir";

constexpr llvm::StringLiteral kReusableAcir = R"mlir(
module attributes {
  ac.model_kind = "queue_graph",
  ac.queue_graph_domain = "cycle"
} {
  ac.type_scope @types {
    ac.struct @Token fields [{name = "value", type = i8}]
        {ac.source_file = "pkg/types.py"}
  } {dlti.dl_spec = #dlti.dl_spec<!ac.struct<@types::@Token> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 1 : i64}>}
  ac.system @main root @Top as "root" tick 0 "cycle"
      seed {kind = "fixed", value = 0 : i64}
      instrumentation [] results {id = "default", format = "json"} selected true
  ac.module @Leaf__width_16() parameters {width = 16 : i64}
      attributes {ac.definition_name = "Leaf", ac.source_file = "pkg/pipeline.py"} graph {
    ac.return
  }
  ac.module @Leaf__width_8() parameters {width = 8 : i64}
      attributes {ac.definition_name = "Leaf", ac.source_file = "pkg/pipeline.py"} graph {
    ac.return
  }
  ac.module @Wrapper() parameters {}
      attributes {ac.definition_name = "Wrapper", ac.source_file = "pkg/wrapper.py"} graph {
    ac.instance @wide of @Leaf__width_16() static {width = 16 : i64}
        id "wide" path "wide" : () -> ()
    ac.instance @narrow of @Leaf__width_8() static {width = 8 : i64}
        id "narrow" path "narrow" : () -> ()
    ac.return
  }
  ac.module @LeafAlias() parameters {}
      attributes {ac.definition_name = "LeafAlias", ac.source_file = "pkg/pipeline.py"} graph {
    ac.instance @leaf of @Leaf__width_8() static {width = 8 : i64}
        id "leaf" path "leaf" : () -> ()
    ac.return
  }
  ac.module @Top() parameters {}
      attributes {ac.definition_name = "Top", ac.source_file = "pkg/core.py"} graph {
    ac.instance @wrapper of @Wrapper() static {}
        id "wrapper" path "wrapper" : () -> ()
    ac.instance @leaf_alias of @LeafAlias() static {}
        id "leaf_alias" path "leaf_alias" : () -> ()
    ac.return
  }
}
)mlir";

constexpr llvm::StringLiteral kCollidingAcir = R"mlir(
module attributes {
  ac.model_kind = "queue_graph",
  ac.queue_graph_domain = "cycle"
} {
  ac.system @main root @Top as "root" tick 0 "cycle"
      seed {kind = "fixed", value = 0 : i64}
      instrumentation [] results {id = "default", format = "json"} selected true
  ac.module @Foo() parameters {}
      attributes {ac.definition_name = "Foo", ac.source_file = "pkg/Foo.py"} graph {
    ac.return
  }
  ac.module @foo() parameters {}
      attributes {ac.definition_name = "foo", ac.source_file = "pkg/foo.py"} graph {
    ac.return
  }
  ac.module @Top() parameters {}
      attributes {ac.definition_name = "Top", ac.source_file = "pkg/core.py"} graph {
    ac.instance @upper of @Foo() static {}
        id "upper" path "upper" : () -> ()
    ac.instance @lower of @foo() static {}
        id "lower" path "lower" : () -> ()
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
  EXPECT_EQ(result->artifacts.front().logicalPath, "core.ac");
  EXPECT_EQ(result->artifacts.front().kind, ArtifactKind::Acir);
  EXPECT_NE(result->artifacts.front().bytes.find("ac.topology_frozen = true"),
            std::string::npos);
}

TEST(CompilerDriverTest, PublishesSourceOwnedAcPackageUnits) {
  CompilerRequest request = validRequest();
  request.acirBytes = kReusableAcir.str();
  auto result = runCompiler(request);
  ASSERT_TRUE(bool(result)) << llvm::toString(result.takeError());
  ASSERT_EQ(result->artifacts.size(), 7u);
  EXPECT_EQ(result->artifacts[0].logicalPath, "core.ac");
  EXPECT_EQ(result->artifacts[1].logicalPath,
            "interfaces/_compiler/layouts.ac");
  EXPECT_EQ(result->artifacts[2].logicalPath, "interfaces/pkg/types.ac");
  EXPECT_EQ(result->artifacts[3].logicalPath,
            "interfaces/modules/pkg/pipeline.ac");
  EXPECT_EQ(result->artifacts[4].logicalPath, "sources/pkg/pipeline.ac");
  EXPECT_EQ(result->artifacts[5].logicalPath,
            "interfaces/modules/pkg/wrapper.ac");
  EXPECT_EQ(result->artifacts[6].logicalPath, "sources/pkg/wrapper.ac");

  const std::string &core = result->artifacts[0].bytes;
  EXPECT_NE(core.find("ac.unit_kind = \"core\""), std::string::npos);
  EXPECT_NE(core.find("ac.system @main"), std::string::npos) << core;
  EXPECT_NE(core.find("ac.module @Top"), std::string::npos) << core;
  EXPECT_EQ(core.find("ac.module @Leaf"), std::string::npos);
  EXPECT_EQ(core.find("ac.module @Wrapper"), std::string::npos);

  const std::string &interface = result->artifacts[2].bytes;
  EXPECT_NE(interface.find("ac.unit_kind = \"interface\""), std::string::npos);
  EXPECT_NE(interface.find("ac.struct @Token"), std::string::npos);
  EXPECT_NE(interface.find("ac.type_scope @types"), std::string::npos);
  EXPECT_EQ(interface.find("ac.module"), std::string::npos);

  const std::string &pipelineHeader = result->artifacts[3].bytes;
  EXPECT_NE(pipelineHeader.find("ac.module.import @Leaf__width_8"),
            std::string::npos);
  EXPECT_NE(pipelineHeader.find("ac.module.import @LeafAlias"),
            std::string::npos);

  const std::string &pipeline = result->artifacts[4].bytes;
  EXPECT_NE(pipeline.find("ac.unit_kind = \"source\""), std::string::npos);
  EXPECT_NE(pipeline.find("ac.unit_source = \"pkg/pipeline.py\""),
            std::string::npos);
  EXPECT_NE(pipeline.find("ac.module @Leaf__width_8"), std::string::npos);
  EXPECT_NE(pipeline.find("ac.module @Leaf__width_16"), std::string::npos);
  EXPECT_NE(pipeline.find("ac.module @LeafAlias"), std::string::npos) << pipeline;
  EXPECT_EQ(pipeline.find("ac.module @Wrapper"), std::string::npos);
  EXPECT_EQ(pipeline.find("ac.module @Top"), std::string::npos);

  const std::string &wrapper = result->artifacts[6].bytes;
  EXPECT_NE(wrapper.find("ac.module @Wrapper"), std::string::npos) << wrapper;
  EXPECT_EQ(wrapper.find("ac.module @Leaf"), std::string::npos);
  EXPECT_EQ(wrapper.find("ac.module @Top"), std::string::npos);
}

TEST(CompilerDriverTest, RejectsPortableModulePathCollisions) {
  CompilerRequest request = validRequest();
  request.acirBytes = kCollidingAcir.str();
  auto result = runCompiler(request);
  ASSERT_FALSE(bool(result));
  auto found = diagnostics(result.takeError());
  ASSERT_EQ(found.size(), 1u);
  EXPECT_EQ(found.front().stage, "topology-closure");
  EXPECT_EQ(found.front().code, "ACIR-EMIT-002");
  EXPECT_NE(found.front().message.find("collide as portable AC package paths"),
            std::string::npos);
}

TEST(CompilerDriverTest, RejectsMissingDefinitionOwnershipMetadata) {
  CompilerRequest request = validRequest();
  constexpr llvm::StringLiteral marker =
      " attributes {ac.definition_name = \"top\"}";
  size_t position = request.acirBytes.find(marker.str());
  ASSERT_NE(position, std::string::npos);
  request.acirBytes.erase(position, marker.size());
  auto result = runCompiler(request);
  ASSERT_FALSE(bool(result));
  auto found = diagnostics(result.takeError());
  ASSERT_EQ(found.size(), 1u);
  EXPECT_EQ(found.front().stage, "topology-closure");
  EXPECT_EQ(found.front().code, "ACIR-EMIT-002");
  EXPECT_NE(found.front().message.find("requires a non-empty string"),
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
