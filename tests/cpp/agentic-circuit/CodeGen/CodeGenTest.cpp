#include "acir/CodeGen/Emitter.h"
#include "acir/CodeGen/Manifest.h"

#include "llvm/Support/Error.h"
#include "llvm/Support/JSON.h"
#include "gtest/gtest.h"

#include <algorithm>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace acir::codegen {
namespace {

// ── Fingerprint ───────────────────────────────────────────────────────

static Fingerprint repeatedFingerprint(char digit) {
  return "sha256:" + std::string(64, digit);
}

static bool hasError(llvm::Error error) {
  if (!error)
    return false;
  llvm::consumeError(std::move(error));
  return true;
}

static BuildManifest makeCompleteManifestFixture() {
  BuildManifest manifest;
  manifest.project = {"demo", "project:demo"};
  manifest.system = {"top", "system:top"};
  manifest.sourceFiles = {
      {"src/generated/model.cpp", repeatedFingerprint('1')}};
  manifest.normalizedAcirSha256 = repeatedFingerprint('2');
  manifest.compiler = {"clang++", "clang-22.1.8", "arm64-apple-darwin"};
  manifest.passPipeline = {"acsim-verify", "acsim-emit-cxx"};
  manifest.providers = {
      {"ac", repeatedFingerprint('3'), repeatedFingerprint('4')}};
  manifest.componentSpecializations = {
      {"ac.Queue", repeatedFingerprint('5'), repeatedFingerprint('6')}};
  manifest.protocolIdentities = {{"ready_valid", repeatedFingerprint('7')}};
  manifest.artifacts = {
      {"bin/model", ArtifactKind::Executable, repeatedFingerprint('8')}};
  manifest.validationGates = {
      {"compile", ValidationStatus::Passed, std::nullopt}};
  manifest.buildProfile = "validated";
  manifest.instrumentationLayers = {"trace"};
  manifest.specializationInputs.push_back(
      {"depth", "ui32", llvm::json::Value(int64_t{4})});
  manifest.buildFingerprint = repeatedFingerprint('f');
  return manifest;
}

TEST(CodeGenManifestTest, FingerprintsUseNormativeSpelling) {
  EXPECT_EQ(computeFingerprint("hello"),
            "sha256:"
            "2cf24dba5fb0a30e26e83b2ac5b9e29e1b161e5c1fa7425e73043362938b9824");
  EXPECT_TRUE(isValidFingerprint(computeFingerprint("hello")));
  EXPECT_FALSE(isValidFingerprint(std::string(64, '0')));
  EXPECT_FALSE(isValidFingerprint("sha256:" + std::string(63, '0')));
  EXPECT_FALSE(isValidFingerprint("sha256:" + std::string(64, 'A')));
}

TEST(CodeGenManifestTest, CanonicalManifestMatchesClosedSchemaShape) {
  auto manifest = makeCompleteManifestFixture();
  EXPECT_FALSE(hasError(manifest.validate()));

  auto canonical = manifest.canonicalJson();
  if (!canonical) {
    ADD_FAILURE() << llvm::toString(canonical.takeError());
    return;
  }

  constexpr std::string_view expected =
      R"json({"artifacts":[{"kind":"executable","path":"bin/model","sha256":"sha256:8888888888888888888888888888888888888888888888888888888888888888"}],"build_fingerprint":"sha256:ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff","build_profile":"validated","compiler":{"build_id":"clang-22.1.8","name":"clang++","toolchain_target":"arm64-apple-darwin"},"component_specializations":[{"canonical_name":"ac.Queue","schema_fingerprint":"sha256:5555555555555555555555555555555555555555555555555555555555555555","specialization_fingerprint":"sha256:6666666666666666666666666666666666666666666666666666666666666666"}],"contract_epoch":"0.5","instrumentation_layers":["trace"],"normalized_acir_sha256":"sha256:2222222222222222222222222222222222222222222222222222222222222222","pass_pipeline":["acsim-verify","acsim-emit-cxx"],"project":{"identity":"project:demo","name":"demo"},"protocol_identities":[{"fingerprint":"sha256:7777777777777777777777777777777777777777777777777777777777777777","name":"ready_valid"}],"providers":[{"implementation_fingerprint":"sha256:4444444444444444444444444444444444444444444444444444444444444444","namespace":"ac","schema_fingerprint":"sha256:3333333333333333333333333333333333333333333333333333333333333333"}],"schema":"agentic-circuit-build-manifest","source_files":[{"path":"src/generated/model.cpp","sha256":"sha256:1111111111111111111111111111111111111111111111111111111111111111"}],"specialization_inputs":[{"acir_type":"ui32","canonical_value":4,"name":"depth"}],"system":{"identity":"system:top","name":"top"},"validation_gates":[{"name":"compile","report_sha256":null,"status":"passed"}],"version":"0.1"})json";
  EXPECT_EQ(*canonical, expected);
}

TEST(CodeGenManifestTest, ManifestRejectsMissingIdentityAndEscapingPath) {
  auto missingIdentity = makeCompleteManifestFixture();
  missingIdentity.project.name.clear();
  EXPECT_TRUE(hasError(missingIdentity.validate()));

  auto escapingPath = makeCompleteManifestFixture();
  escapingPath.sourceFiles.front().path = "../escape.cpp";
  EXPECT_TRUE(hasError(escapingPath.validate()));
}

TEST(CodeGenManifestTest, ManifestCanonicalizesSetLikeCollections) {
  auto first = makeCompleteManifestFixture();
  first.instrumentationLayers = {"trace", "statistics"};
  first.providers.push_back(
      {"ac.extra", repeatedFingerprint('9'), repeatedFingerprint('a')});

  auto second = makeCompleteManifestFixture();
  second.instrumentationLayers = {"statistics", "trace"};
  second.providers = {
      {"ac.extra", repeatedFingerprint('9'), repeatedFingerprint('a')},
      {"ac", repeatedFingerprint('3'), repeatedFingerprint('4')}};

  auto firstJson = first.canonicalJson();
  auto secondJson = second.canonicalJson();
  if (!firstJson || !secondJson) {
    if (!firstJson)
      ADD_FAILURE() << llvm::toString(firstJson.takeError());
    if (!secondJson)
      ADD_FAILURE() << llvm::toString(secondJson.takeError());
    return;
  }
  EXPECT_EQ(*firstJson, *secondJson);
}

TEST(CodeGenManifestTest, BuildFingerprintUsesClosedVersionedPreimage) {
  auto first = makeCompleteManifestFixture();
  auto second = makeCompleteManifestFixture();
  first.buildFingerprint.clear();
  second.buildFingerprint.clear();
  EXPECT_FALSE(hasError(first.finalizeBuildFingerprint()));
  EXPECT_FALSE(hasError(second.finalizeBuildFingerprint()));
  EXPECT_TRUE(isValidFingerprint(first.buildFingerprint));
  EXPECT_EQ(first.buildFingerprint, second.buildFingerprint);

  second.compiler.buildId = "clang-22.1.9";
  EXPECT_FALSE(hasError(second.finalizeBuildFingerprint()));
  EXPECT_NE(first.buildFingerprint, second.buildFingerprint);
}

// ── CppEmitter ────────────────────────────────────────────────────────

TEST(CodeGenEmitterTest, EmitsPragmaOnce) {
  std::ostringstream os;
  CppEmitter e(os);
  e.emitPragmaOnce();
  EXPECT_EQ(os.str(), "#pragma once\n\n");
}

TEST(CodeGenEmitterTest, EmitsInclude) {
  std::ostringstream os;
  CppEmitter e(os);
  e.emitInclude("string", true);
  e.emitInclude("local.h");
  EXPECT_NE(os.str().find("<string>"), std::string::npos);
  EXPECT_NE(os.str().find("\"local.h\""), std::string::npos);
}

TEST(CodeGenEmitterTest, EmitsNamespace) {
  std::ostringstream os;
  CppEmitter e(os);
  e.beginNamespace("test");
  e.emitComment("hello");
  e.endNamespace();
  EXPECT_NE(os.str().find("namespace test"), std::string::npos);
  EXPECT_NE(os.str().find("// hello"), std::string::npos);
}

TEST(CodeGenEmitterTest, EmitsClassWithBase) {
  std::ostringstream os;
  CppEmitter e(os);
  e.beginClass("Foo", "Bar");
  e.emitPublic();
  e.endClass();
  EXPECT_NE(os.str().find("class Foo : public Bar"), std::string::npos);
  EXPECT_NE(os.str().find("public:"), std::string::npos);
}

TEST(CodeGenEmitterTest, EmitsEnum) {
  std::ostringstream os;
  CppEmitter e(os);
  e.emitEnum("Color", {"Red", "Green", "Blue"});
  EXPECT_NE(os.str().find("enum class Color"), std::string::npos);
  EXPECT_NE(os.str().find("Red"), std::string::npos);
  EXPECT_NE(os.str().find("Green"), std::string::npos);
}

TEST(CodeGenEmitterTest, EmitsConstructor) {
  std::ostringstream os;
  CppEmitter e(os);
  e.emitConstructor("Foo", {{"int", "x"}, {"int", "y"}}, {"x_(x)", "y_(y)"},
                    "  init();\n");
  EXPECT_NE(os.str().find("Foo(int x, int y)"), std::string::npos);
  EXPECT_NE(os.str().find(": x_(x)"), std::string::npos);
  EXPECT_NE(os.str().find("init()"), std::string::npos);
}

TEST(CodeGenEmitterTest, ConstructorDeclarationOmitsInitializers) {
  std::ostringstream os;
  CppEmitter emitter(os);
  emitter.emitConstructor("Foo", {{"int", "x"}}, {"x_(x)"});
  EXPECT_EQ(os.str(), "Foo(int x);\n");
}

TEST(CodeGenEmitterTest, EmitsMethod) {
  std::ostringstream os;
  CppEmitter e(os);
  e.emitMethod("void", "run", {}, true, true, true);
  auto s = os.str();
  EXPECT_NE(s.find("virtual void run() const override"), std::string::npos);
}

TEST(CodeGenEmitterTest, EmitsSwitch) {
  std::ostringstream os;
  CppEmitter e(os);
  e.emitSwitch("x");
  e.emitCase("1");
  e.emitBreak();
  e.emitDefault();
  e.emitBreak();
  e.endSwitch();
  EXPECT_NE(os.str().find("switch (x)"), std::string::npos);
  EXPECT_NE(os.str().find("case 1:"), std::string::npos);
  EXPECT_NE(os.str().find("default:"), std::string::npos);
}

// ── Deterministic code generation ─────────────────────────────────────

TEST(CodeGenGenTest, GenerateDispatchHeaderIsDenseAndDeterministic) {
  std::vector<DispatchEntry> entries = {
      {1, "model.consumer"},
      {0, "model.producer"},
  };
  std::vector<ActivationEdge> edges = {{1, 1}, {0, 1}, {0, 0}, {0, 1}};
  auto first =
      generateDispatchHeader("generated::soc", "SocModel", entries, edges);
  std::reverse(entries.begin(), entries.end());
  std::reverse(edges.begin(), edges.end());
  auto second =
      generateDispatchHeader("generated::soc", "SocModel", entries, edges);

  constexpr std::string_view expected = R"(#pragma once

#include "gfsim/dispatch.h"

#include <array>
#include <cstdint>

namespace generated::soc {

inline std::array<gfsim::DispatchRow, 2>
makeDispatchTable(SocModel &model) {
  return {
      gfsim::makeDispatchRow(&model.producer),
      gfsim::makeDispatchRow(&model.consumer),
  };
}

inline constexpr std::array<uint32_t, 3> kActivationOffsets = {0, 2, 3};
inline constexpr std::array<gfsim::ObjectId, 3> kActivationTargets = {0, 1, 1};

} // namespace generated::soc
)";
  EXPECT_EQ(first.content, expected);
  EXPECT_EQ(first.content, second.content);
  EXPECT_EQ(first.fingerprint, second.fingerprint);
  EXPECT_NE(first.content.find("gfsim/dispatch.h"), std::string::npos);
  EXPECT_NE(first.content.find("std::array<gfsim::DispatchRow, 2>"),
            std::string::npos);
  EXPECT_NE(first.content.find("makeDispatchTable(SocModel &model)"),
            std::string::npos);
  EXPECT_EQ(first.relativePath, "include/generated/soc/dispatch.h");
  size_t producer = first.content.find("model.producer");
  size_t consumer = first.content.find("model.consumer");
  ASSERT_NE(producer, std::string::npos);
  ASSERT_NE(consumer, std::string::npos);
  EXPECT_LT(producer, consumer);
}

TEST(CodeGenGenTest, GenerateDispatchHeaderRejectsNonDenseIds) {
  EXPECT_THROW(generateDispatchHeader("generated::soc", "SocModel",
                                      {{1, "model.consumer"}}),
               std::invalid_argument);
}

TEST(CodeGenGenTest, GenerateDispatchHeaderRejectsInvalidActivationEdge) {
  EXPECT_THROW(generateDispatchHeader("generated::soc", "SocModel",
                                      {{0, "model.producer"}}, {{0, 1}}),
               std::invalid_argument);
}

} // namespace
} // namespace acir::codegen
