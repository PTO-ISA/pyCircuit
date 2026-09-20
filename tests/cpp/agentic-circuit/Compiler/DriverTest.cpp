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
  ac.module @top source #ac.source_owner<"pkg/core.py", "pkg/core.py">
      schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[]>, #ac.source_owner<"pkg/core.py", "pkg/core.py">, []> {
    ac.module.case arguments #ac.static_arguments<[]> type () -> ()
        {ac.definition_name = "top"}
        source #ac.source_provenance<"pkg/core.py", 1, 1, 1, 1> graph {
      ac.process @workload kind "workload" { ac.yield_sim }
      ac.return
    }
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
        {ac.source_file = "pkg/token.py"}
  } {dlti.dl_spec = #dlti.dl_spec<!ac.struct<@types::@Token> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 1 : i64}>}
  ac.system @main root @Top as "root" tick 0 "cycle"
      seed {kind = "fixed", value = 0 : i64}
      instrumentation [] results {id = "default", format = "json"} selected true
  ac.module @Leaf source #ac.source_owner<"pkg/pipeline.py", "pkg/pipeline.py">
      schema #ac.module_family_schema<
        #ac.static_parameters<[#ac.static_parameter<"width", #ac.static_type<#ac.static_int_type<8, false>>, true, [], #ac.source_provenance<"pkg/pipeline.py", 1, 1, 1, 1>>]>,
        #ac.static_cases<[
          #ac.static_arguments<[#ac.static_argument<"width", #ac.static_value<#ac.static_int_value<#ac.static_int_type<8, false>, 8 : i8>>>]>,
          #ac.static_arguments<[#ac.static_argument<"width", #ac.static_value<#ac.static_int_value<#ac.static_int_type<8, false>, 16 : i8>>>]>
        ]>, #ac.module_interface<[]>, #ac.source_owner<"pkg/pipeline.py", "pkg/pipeline.py">, []> {
    ac.module.case arguments #ac.static_arguments<[#ac.static_argument<"width", #ac.static_value<#ac.static_int_value<#ac.static_int_type<8, false>, 8 : i8>>>]> type () -> () {ac.definition_name = "Leaf"} source #ac.source_provenance<"pkg/pipeline.py", 1, 1, 1, 1> graph { ac.return }
    ac.module.case arguments #ac.static_arguments<[#ac.static_argument<"width", #ac.static_value<#ac.static_int_value<#ac.static_int_type<8, false>, 16 : i8>>>]> type () -> () {ac.definition_name = "Leaf"} source #ac.source_provenance<"pkg/pipeline.py", 1, 1, 1, 1> graph { ac.return }
  }
  ac.module @Wrapper source #ac.source_owner<"pkg/wrapper.py", "pkg/wrapper.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[]>, #ac.source_owner<"pkg/wrapper.py", "pkg/wrapper.py">, []> {
    ac.module.case arguments #ac.static_arguments<[]> type () -> () {ac.definition_name = "Wrapper"} source #ac.source_provenance<"pkg/wrapper.py", 1, 1, 1, 1> graph {
    ac.instance @wide of @Leaf() static #ac.static_arguments<[#ac.static_argument<"width", #ac.static_value<#ac.static_int_value<#ac.static_int_type<8, false>, 16 : i8>>>]>
        id "wide" path "wide" : () -> ()
    ac.instance @narrow of @Leaf() static #ac.static_arguments<[#ac.static_argument<"width", #ac.static_value<#ac.static_int_value<#ac.static_int_type<8, false>, 8 : i8>>>]>
        id "narrow" path "narrow" : () -> ()
    ac.return
    }
  }
  ac.module @LeafAlias source #ac.source_owner<"pkg/pipeline.py", "pkg/pipeline.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[]>, #ac.source_owner<"pkg/pipeline.py", "pkg/pipeline.py">, []> {
    ac.module.case arguments #ac.static_arguments<[]> type () -> () {ac.definition_name = "LeafAlias"} source #ac.source_provenance<"pkg/pipeline.py", 1, 1, 1, 1> graph {
    ac.instance @leaf of @Leaf() static #ac.static_arguments<[#ac.static_argument<"width", #ac.static_value<#ac.static_int_value<#ac.static_int_type<8, false>, 8 : i8>>>]>
        id "leaf" path "leaf" : () -> ()
    ac.return
    }
  }
  ac.module @Top source #ac.source_owner<"pkg/core.py", "pkg/core.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[]>, #ac.source_owner<"pkg/core.py", "pkg/core.py">, []> {
    ac.module.case arguments #ac.static_arguments<[]> type () -> () {ac.definition_name = "Top"} source #ac.source_provenance<"pkg/core.py", 1, 1, 1, 1> graph {
    ac.instance @wrapper of @Wrapper() static #ac.static_arguments<[]>
        id "wrapper" path "wrapper" : () -> ()
    ac.instance @leaf_alias of @LeafAlias() static #ac.static_arguments<[]>
        id "leaf_alias" path "leaf_alias" : () -> ()
    ac.return
    }
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
  ac.type_scope @types {
    ac.struct @Upper fields [{name = "value", type = i8}]
        {ac.source_file = "pkg/Foo.py"}
    ac.struct @Lower fields [{name = "value", type = i8}]
        {ac.source_file = "pkg/foo.py"}
  } {dlti.dl_spec = #dlti.dl_spec<!ac.struct<@types::@Upper> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 1 : i64}, !ac.struct<@types::@Lower> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 1 : i64}>}
  ac.module @Top source #ac.source_owner<"pkg/core.py", "pkg/core.py"> schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[]>, #ac.source_owner<"pkg/core.py", "pkg/core.py">, []> {
    ac.module.case arguments #ac.static_arguments<[]> type () -> () {ac.definition_name = "Top"} source #ac.source_provenance<"pkg/core.py", 1, 1, 1, 1> graph {
    ac.return
    }
  }
}
)mlir";

constexpr llvm::StringLiteral kDirectSourceUnitAcir = R"mlir(
module attributes {ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle"} {
  ac.type_scope @types {
    ac.struct @Token fields [{name = "value", type = i8}]
        {ac.source_file = "pkg/pipeline.py"}
  } {dlti.dl_spec = #dlti.dl_spec<!ac.struct<@types::@Token> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 1 : i64}>}
  ac.system @source_unit root @_acc_source_unit_root as "root" tick 0 "cycle"
      seed {kind = "fixed", value = 0 : i64} instrumentation []
      results {id = "default", format = "json"} selected true
  ac.module @Leaf source #ac.source_owner<"pkg/pipeline.py", "pkg/pipeline.py">
      schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[]>, #ac.source_owner<"pkg/pipeline.py", "pkg/pipeline.py">, [@Token]> {
    ac.module.case arguments #ac.static_arguments<[]> type () -> ()
        {ac.definition_name = "Leaf"}
        source #ac.source_provenance<"pkg/pipeline.py", 1, 1, 1, 1> graph {
      ac.return
    }
  }
  ac.module @_acc_source_unit_root source #ac.source_owner<"pkg/pipeline.py", "pkg/pipeline.py">
      schema #ac.module_family_schema<#ac.static_parameters<[]>, #ac.static_cases<[#ac.static_arguments<[]>]>, #ac.module_interface<[]>, #ac.source_owner<"pkg/pipeline.py", "pkg/pipeline.py">, []> {
    ac.module.case arguments #ac.static_arguments<[]> type () -> ()
        {ac.definition_name = "_acc_source_unit_root"}
        source #ac.source_provenance<"pkg/pipeline.py", 1, 1, 1, 1> graph {
      ac.instance @leaf of @Leaf() static #ac.static_arguments<[]>
          id "leaf" path "leaf" : () -> ()
      ac.return
    }
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

TEST(CompilerDriverTest, RejectsWholeDesignPostCompileSplitting) {
  CompilerRequest request = validRequest();
  request.acirBytes = kReusableAcir.str();
  auto result = runCompiler(request);
  ASSERT_FALSE(bool(result));
  auto found = diagnostics(result.takeError());
  ASSERT_EQ(found.size(), 1u);
  EXPECT_EQ(found.front().stage, "topology-closure");
  EXPECT_EQ(found.front().code, "ACIR-EMIT-002");
  EXPECT_NE(found.front().message.find("invoke acc.py directly"),
            std::string::npos);
}

TEST(CompilerDriverTest, PublishesOneDirectSourceOwnedUnitAndInterface) {
  CompilerRequest request = validRequest();
  request.acirBytes = kDirectSourceUnitAcir.str();
  auto result = runCompiler(request);
  ASSERT_TRUE(bool(result)) << llvm::toString(result.takeError());
  ASSERT_EQ(result->artifacts.size(), 3u);
  EXPECT_EQ(result->artifacts[0].logicalPath,
            "interfaces/_compiler/layouts.ac");
  EXPECT_EQ(result->artifacts[1].logicalPath,
            "interfaces/pkg/pipeline.ac");
  EXPECT_EQ(result->artifacts[2].logicalPath, "sources/pkg/pipeline.ac");
  EXPECT_NE(result->artifacts[1].bytes.find("ac.struct @Token"),
            std::string::npos);
  EXPECT_NE(result->artifacts[1].bytes.find("ac.module.import @Leaf"),
            std::string::npos);
  EXPECT_NE(result->artifacts[2].bytes.find("ac.module @Leaf"),
            std::string::npos);
  EXPECT_EQ(result->artifacts[2].bytes.find("_acc_source_unit_root"),
            std::string::npos);
}

TEST(CompilerDriverTest, RejectsPortableModulePathCollisions) {
  CompilerRequest request = validRequest();
  request.acirBytes = kCollidingAcir.str();
  auto result = runCompiler(request);
  ASSERT_FALSE(bool(result));
  auto found = diagnostics(result.takeError());
  ASSERT_EQ(found.size(), 1u);
  EXPECT_EQ(found.front().stage, "acir-verify");
  EXPECT_EQ(found.front().code, "ACIR-EMIT-002");
  EXPECT_NE(found.front().message.find("collide as portable AC package paths"),
            std::string::npos)
      << found.front().message;
}

TEST(CompilerDriverTest, RejectsMissingDefinitionOwnershipMetadata) {
  CompilerRequest request = validRequest();
  constexpr llvm::StringLiteral marker = " {ac.definition_name = \"top\"}";
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
