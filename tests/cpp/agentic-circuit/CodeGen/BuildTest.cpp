#include "acir/CodeGen/Build.h"
#include "BuildInternal.h"
#include "TestToolchain.h"

#include "acir/Dialect/ACSim/ACSimDialect.h"

#include "mlir/IR/MLIRContext.h"
#include "mlir/Parser/Parser.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/raw_ostream.h"
#include "gtest/gtest.h"

#include <algorithm>
#include <array>
#include <string>

namespace acir::codegen {
namespace {

constexpr llvm::StringLiteral kFingerprint =
    "sha256:0000000000000000000000000000000000000000000000000000000000000000";

bool hasError(llvm::Error error) {
  if (!error)
    return false;
  llvm::consumeError(std::move(error));
  return true;
}

std::string readFile(llvm::StringRef path) {
  auto buffer = llvm::MemoryBuffer::getFile(path);
  return buffer ? buffer.get()->getBuffer().str() : std::string();
}

void writeFile(llvm::StringRef path, llvm::StringRef content) {
  std::error_code error;
  llvm::raw_fd_ostream output(path, error);
  ASSERT_FALSE(error);
  output << content;
}

class TempOutputRoot {
public:
  TempOutputRoot() {
    EXPECT_FALSE(
        llvm::sys::fs::createUniqueDirectory("acir-build-test", path_));
  }
  ~TempOutputRoot() { llvm::sys::fs::remove_directories(path_); }
  std::string path() const { return path_.str().str(); }

private:
  llvm::SmallString<256> path_;
};

llvm::Expected<Fingerprint> jsonFingerprint(llvm::json::Value value) {
  return fingerprintCanonicalJson(value);
}

FrontendProvenance makeFrontendProvenance() {
  const std::string acpy = "{\"schema\":\"agentic-circuit-acpy\"}\n";
  const std::string acir = "module attributes {ac.contract_epoch = \"0.5\"}\n";
  FrontendProvenance frontend;
  frontend.sourceFiles = {
      {"architecture.py", computeFingerprint("architecture source\n")}};
  frontend.acpy = {"input/model.acpy.json", ArtifactKind::Acpy,
                   computeFingerprint(acpy)};
  frontend.acpyBytes = acpy;
  frontend.canonicalAcir = {"input/model.ac.mlir", ArtifactKind::Acir,
                            computeFingerprint(acir)};
  frontend.canonicalAcirBytes = acir;
  frontend.pythonVersion = "CPython 3.12";
  frontend.helperIdentities = {{"agentic-circuit", computeFingerprint("0.1")}};
  return frontend;
}

class PublishFixture {
public:
  explicit PublishFixture(llvm::StringRef outputRoot) {
    context_.loadDialect<acsim::ACSimDialect>();
    auto toolchain = identifyToolchain(ACIR_TEST_CXX_COMPILER, "libc++",
                                       "default", "mach-o", {"-std=c++20"});
    EXPECT_TRUE(static_cast<bool>(toolchain));
    if (!toolchain)
      return;
    request_.toolchain = std::move(*toolchain);
    const std::string frozenBytes = "minimal frozen acir\n";
    const std::string lockBytes = "[]";
    auto profile = jsonFingerprint(llvm::json::Value("fast"));
    auto target =
        jsonFingerprint(llvm::json::Value(request_.toolchain.targetTriple));
    auto emptySet = jsonFingerprint(llvm::json::Value(llvm::json::Array{}));
    EXPECT_TRUE(static_cast<bool>(profile));
    EXPECT_TRUE(static_cast<bool>(target));
    EXPECT_TRUE(static_cast<bool>(emptySet));
    if (!profile || !target || !emptySet)
      return;

    const std::string zero = kFingerprint.str();
    std::string source =
        "builtin.module attributes {ac.contract_epoch = \"0.5\"} {\n"
        "  acsim.model @minimal epoch \"0.5\" root @Top construction [] "
        "destruction [] fingerprints {frozen_acir = \"" +
        computeFingerprint(frozenBytes) + "\", binding_lock = \"" +
        computeFingerprint(lockBytes) + "\", provider = \"" + *emptySet +
        "\", profile = \"" + *profile + "\", toolchain = \"" + *target +
        "\", schema_set = \"" + *emptySet +
        "\"} {\n"
        "    acsim.module @Top interface {ports = [], resources = [], results "
        "= []} static [] specialization \"" +
        zero + "\" exports [] { acsim.return }\n  }\n}\n";
    module_ = mlir::parseSourceString<mlir::ModuleOp>(source, &context_);
    EXPECT_TRUE(static_cast<bool>(module_));
    if (!module_)
      return;

    request_.project = {"project", "project.example"};
    request_.system = {"system", "system.example"};
    request_.frontend = makeFrontendProvenance();
    request_.canonicalACSim = *module_;
    request_.frozenAcirBytes = frozenBytes;
    request_.bindingLockBytes = lockBytes;
    request_.profile = "fast";
    request_.passPipeline = {"acsim-emit-cxx", "compile", "link"};
    request_.includeRoots = {ACIR_TEST_SOURCE_DIR "/simulator/gfsim/include"};
    request_.linkInputs = {ACIR_TEST_BINARY_DIR "/gfsim/libgfsim.a",
                           ACIR_TEST_BINARY_DIR
                           "/lib/Bindings/libACIRBindings.a"};
    request_.linkerFlags = test::llvmLinkerFlags();
    if (llvm::StringRef(ACIR_TEST_SANITIZER_FLAG).size())
      request_.linkerFlags.push_back(ACIR_TEST_SANITIZER_FLAG);
    request_.outputRoot = outputRoot.str();
  }

  BuildRequest &request() { return request_; }

private:
  mlir::MLIRContext context_;
  mlir::OwningOpRef<mlir::ModuleOp> module_;
  BuildRequest request_;
};

llvm::Expected<BuildRequest> makeBuildRequest() {
  auto toolchain = identifyToolchain(ACIR_TEST_CXX_COMPILER, "libc++",
                                     "default", "mach-o", {"-std=c++20"});
  if (!toolchain)
    return toolchain.takeError();
  BuildRequest request;
  request.project = {"project", "project.example"};
  request.system = {"system", "system.example"};
  request.frontend = makeFrontendProvenance();
  request.profile = "fast";
  request.passPipeline = {"acsim-emit-cxx", "compile", "link"};
  request.toolchain = std::move(*toolchain);
  request.includeRoots = {"vendor/include", "include"};
  request.definitions = {"ZETA=1", "ALPHA=1"};
  request.compilerFlags = {"-Wall"};
  request.linkerFlags = {"-pthread"};
  request.outputRoot = "out";
  return request;
}

SourceBundle makeSourceBundle() {
  SourceBundle bundle;
  bundle.sourceFingerprint = kFingerprint.str();
  bundle.buildFingerprint = kFingerprint.str();
  bundle.files = {
      {.relativePath = "include/generated/model.h",
       .content = "#pragma once\n",
       .fingerprint = computeFingerprint("#pragma once\n")},
      {.relativePath = "src/generated/main.cpp",
       .content = "int main() { return 0; }\n",
       .fingerprint = computeFingerprint("int main() { return 0; }\n")}};
  return bundle;
}

TEST(BuildTest, CompilePlanIsClosedCanonicalAndArgumentVectorBased) {
  auto request = makeBuildRequest();
  ASSERT_TRUE(static_cast<bool>(request));
  auto first = createCompilePlan(*request, makeSourceBundle());
  auto second = createCompilePlan(*request, makeSourceBundle());
  ASSERT_TRUE(static_cast<bool>(first));
  ASSERT_TRUE(static_cast<bool>(second));
  EXPECT_EQ(first->schema, "acsim-compile-plan-0.1");
  EXPECT_TRUE(isValidFingerprint(first->fingerprint));
  EXPECT_EQ(first->fingerprint, second->fingerprint);
  std::vector<std::string> expectedIncludeRoots =
      test::llvmIncludeDirectories();
  expectedIncludeRoots.insert(expectedIncludeRoots.end(),
                              {"include", "vendor/include"});
  std::sort(expectedIncludeRoots.begin(), expectedIncludeRoots.end());
  expectedIncludeRoots.erase(
      std::unique(expectedIncludeRoots.begin(), expectedIncludeRoots.end()),
      expectedIncludeRoots.end());
  EXPECT_EQ(first->includeRoots, expectedIncludeRoots);
  EXPECT_EQ(first->definitions,
            (std::vector<std::string>{"ALPHA=1", "ZETA=1"}));
  ASSERT_EQ(first->compileCommands.size(), 1u);
  EXPECT_EQ(first->compileCommands[0].arguments.front(),
            request->toolchain.compilerPath);
  EXPECT_EQ(first->compileCommands[0].arguments.back(),
            first->compileCommands[0].output);
  EXPECT_FALSE(first->linkCommand.arguments.empty());
  auto firstJson = first->canonicalJson();
  auto secondJson = second->canonicalJson();
  ASSERT_TRUE(static_cast<bool>(firstJson));
  ASSERT_TRUE(static_cast<bool>(secondJson));
  EXPECT_EQ(*firstJson, *secondJson);
}

TEST(BuildTest, LinkInputContentsParticipateInCompilePlanIdentity) {
  TempOutputRoot output;
  const std::string linkInput = output.path() + "/runtime.a";
  writeFile(linkInput, "first link input\n");

  auto request = makeBuildRequest();
  ASSERT_TRUE(static_cast<bool>(request));
  request->linkInputs = {linkInput};
  auto first = createCompilePlan(*request, makeSourceBundle());
  ASSERT_TRUE(static_cast<bool>(first));

  writeFile(linkInput, "second link input\n");
  auto second = createCompilePlan(*request, makeSourceBundle());
  ASSERT_TRUE(static_cast<bool>(second));
  EXPECT_NE(first->fingerprint, second->fingerprint);
}

TEST(BuildTest, RejectsToolchainOrPrebuiltProvenanceMismatch) {
  auto request = makeBuildRequest();
  ASSERT_TRUE(static_cast<bool>(request));
  request->prebuiltInputs.push_back(
      {.path = "lib/provider.o",
       .kind = "provider",
       .provenance = {.compilerBuildId = request->toolchain.compilerBuildId,
                      .targetTriple = request->toolchain.targetTriple,
                      .standardLibrary = request->toolchain.standardLibrary,
                      .abiMode = request->toolchain.abiMode,
                      .objectFormat = request->toolchain.objectFormat,
                      .contractEpoch = "0.5",
                      .contractFlags = request->toolchain.contractFlags,
                      .toolchainFingerprint = request->toolchain.fingerprint,
                      .sourceFingerprint = kFingerprint.str()}});
  EXPECT_FALSE(hasError(preflightBuildRequest(*request)));

  request->prebuiltInputs.front().provenance.compilerBuildId = "different";
  EXPECT_TRUE(hasError(preflightBuildRequest(*request)));

  request->prebuiltInputs.front().sourceAvailable = true;
  EXPECT_FALSE(hasError(preflightBuildRequest(*request)));
  auto plan = createCompilePlan(*request, makeSourceBundle());
  ASSERT_TRUE(static_cast<bool>(plan));
  EXPECT_TRUE(plan->prebuiltInputs.empty());
}

TEST(BuildTest, RejectsProfileAndToolchainIdentityMismatch) {
  auto request = makeBuildRequest();
  ASSERT_TRUE(static_cast<bool>(request));
  request->profile = "debug";
  EXPECT_TRUE(hasError(preflightBuildRequest(*request)));

  request = makeBuildRequest();
  ASSERT_TRUE(static_cast<bool>(request));
  request->toolchain.targetTriple = "different-target";
  EXPECT_TRUE(hasError(preflightBuildRequest(*request)));
}

TEST(BuildTest, PublishedBuildRequiresFrozenAcirAndBindingLockBytes) {
  TempOutputRoot output;
  PublishFixture fixture(output.path());
  fixture.request().frozenAcirBytes.clear();
  EXPECT_TRUE(hasError(preflightBuildRequest(fixture.request())));

  PublishFixture second(output.path());
  second.request().bindingLockBytes.clear();
  EXPECT_TRUE(hasError(preflightBuildRequest(second.request())));
}

TEST(BuildTest, FrontendProvenanceIsClosedAndContentAddressed) {
  auto request = makeBuildRequest();
  ASSERT_TRUE(static_cast<bool>(request));
  EXPECT_FALSE(hasError(preflightBuildRequest(*request)));

  request->frontend.acpy.kind = ArtifactKind::Report;
  EXPECT_TRUE(hasError(preflightBuildRequest(*request)));

  request = makeBuildRequest();
  ASSERT_TRUE(static_cast<bool>(request));
  request->frontend.sourceFiles.push_back(
      request->frontend.sourceFiles.front());
  EXPECT_TRUE(hasError(preflightBuildRequest(*request)));
}

TEST(BuildTest, RejectsNonCanonicalPathsAndSourceFingerprintMismatch) {
  auto request = makeBuildRequest();
  ASSERT_TRUE(static_cast<bool>(request));
  request->includeRoots = {"../escape"};
  auto plan = createCompilePlan(*request, makeSourceBundle());
  EXPECT_FALSE(plan);
  llvm::consumeError(plan.takeError());

  request = makeBuildRequest();
  ASSERT_TRUE(static_cast<bool>(request));
  SourceBundle bundle = makeSourceBundle();
  bundle.files.back().fingerprint = kFingerprint.str();
  plan = createCompilePlan(*request, bundle);
  EXPECT_FALSE(plan);
  llvm::consumeError(plan.takeError());
}

TEST(BuildTest, ExactSecondBuildIsCacheHitAndUnequalInputMisses) {
  TempOutputRoot output;
  PublishFixture fixture(output.path());
  auto first = buildGeneratedModel(fixture.request());
  auto second = buildGeneratedModel(fixture.request());
  if (!first || !second) {
    if (!first)
      ADD_FAILURE() << llvm::toString(first.takeError());
    if (!second)
      ADD_FAILURE() << llvm::toString(second.takeError());
    return;
  }
  EXPECT_FALSE(first->cacheHit);
  EXPECT_TRUE(second->cacheHit);
  EXPECT_EQ(first->buildFingerprint, second->buildFingerprint);
  const std::string manifest =
      readFile(first->buildDirectory + "/build-manifest.json");
  EXPECT_NE(manifest.find("architecture.py"), std::string::npos);
  EXPECT_NE(manifest.find("input/model.acpy.json"), std::string::npos);

  fixture.request().frontend.pythonVersion = "CPython 3.13";
  auto third = buildGeneratedModel(fixture.request());
  ASSERT_TRUE(static_cast<bool>(third));
  EXPECT_FALSE(third->cacheHit);
  EXPECT_NE(first->buildFingerprint, third->buildFingerprint);
}

TEST(BuildTest, FailedStagePreservesPublishedBuildAndCurrentPointer) {
  TempOutputRoot output;
  PublishFixture fixture(output.path());
  auto first = buildGeneratedModel(fixture.request());
  if (!first) {
    ADD_FAILURE() << llvm::toString(first.takeError());
    return;
  }
  const std::string pointerPath = output.path() + "/current.json";
  const std::string previousCurrent = readFile(pointerPath);

  BuildServices services = makeRealBuildServices();
  services.failurePoint = BuildFailurePoint::AfterLink;
  auto failed = buildGeneratedModelForTesting(fixture.request(), services);
  EXPECT_FALSE(failed);
  llvm::consumeError(failed.takeError());
  EXPECT_EQ(readFile(pointerPath), previousCurrent);
  EXPECT_FALSE(
      readFile(first->buildDirectory + "/build-manifest.json").empty());
}

TEST(BuildTest, RejectsEveryEscapingOrNonCanonicalArtifactPath) {
  for (llvm::StringRef path : {"", "/absolute", "../escape", "a/../b", "./file",
                               "a//b", "trailing/"}) {
    auto normalized = normalizeArtifactPath(path);
    EXPECT_FALSE(normalized) << path.str();
    llvm::consumeError(normalized.takeError());
  }
  auto normalized = normalizeArtifactPath("reports/compile.txt");
  ASSERT_TRUE(static_cast<bool>(normalized));
  EXPECT_EQ(*normalized, "reports/compile.txt");
}

TEST(BuildTest, EveryInjectedBoundaryPreservesPublishedState) {
  TempOutputRoot output;
  PublishFixture fixture(output.path());
  auto baseline = buildGeneratedModel(fixture.request());
  if (!baseline) {
    ADD_FAILURE() << llvm::toString(baseline.takeError());
    return;
  }
  const std::string pointerPath = output.path() + "/current.json";
  const std::string manifestPath =
      baseline->buildDirectory + "/build-manifest.json";
  const std::string previousCurrent = readFile(pointerPath);
  const std::string previousManifest = readFile(manifestPath);
  const std::array failurePoints = {BuildFailurePoint::AfterInputValidation,
                                    BuildFailurePoint::AfterSourceWrite,
                                    BuildFailurePoint::AfterContractCheck,
                                    BuildFailurePoint::AfterCompile,
                                    BuildFailurePoint::AfterLink,
                                    BuildFailurePoint::AfterFingerprintQuery,
                                    BuildFailurePoint::AfterManifestWrite,
                                    BuildFailurePoint::AfterImmutableRename,
                                    BuildFailurePoint::BeforeCurrentRename};
  for (BuildFailurePoint failurePoint : failurePoints) {
    BuildServices services = makeRealBuildServices();
    services.failurePoint = failurePoint;
    auto failed = buildGeneratedModelForTesting(fixture.request(), services);
    EXPECT_FALSE(failed);
    if (!failed)
      llvm::consumeError(failed.takeError());
    EXPECT_EQ(readFile(pointerPath), previousCurrent);
    EXPECT_EQ(readFile(manifestPath), previousManifest);
  }
}

} // namespace
} // namespace acir::codegen
