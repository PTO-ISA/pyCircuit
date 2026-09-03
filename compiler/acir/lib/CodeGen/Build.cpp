#include "BuildInternal.h"

#include "acir/Bindings/Binding.h"

#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/Twine.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/JSON.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/Program.h"
#include "llvm/Support/raw_ostream.h"

#include <algorithm>
#include <array>
#include <map>
#include <optional>
#include <set>
#include <system_error>
#include <utility>

namespace acir::codegen {
namespace {

llvm::Error buildError(const llvm::Twine &message) {
  return llvm::createStringError(
      std::make_error_code(std::errc::invalid_argument),
      "ACLOWER-FINGERPRINT: " + message);
}

llvm::Error injectedFailure(BuildFailurePoint requested,
                            BuildFailurePoint actual) {
  if (requested != actual)
    return llvm::Error::success();
  return buildError("injected build-stage failure");
}

llvm::SmallString<256> stagedPath(llvm::StringRef root,
                                  llvm::StringRef relative) {
  llvm::SmallString<256> path(root);
  llvm::sys::path::append(path, relative);
  return path;
}

llvm::Expected<Fingerprint>
publicationFingerprint(const BuildRequest &request, const SourceBundle &bundle,
                       const CompilePlan &compilePlan) {
  std::vector<std::string> layers = request.instrumentationLayers;
  std::sort(layers.begin(), layers.end());
  layers.erase(std::unique(layers.begin(), layers.end()), layers.end());
  llvm::json::Array instrumentation;
  for (const std::string &layer : layers)
    instrumentation.push_back(layer);
  std::vector<std::string> providerIdentities = request.providerInputs;
  std::sort(providerIdentities.begin(), providerIdentities.end());
  providerIdentities.erase(
      std::unique(providerIdentities.begin(), providerIdentities.end()),
      providerIdentities.end());
  llvm::json::Array providers;
  for (const std::string &provider : providerIdentities)
    providers.push_back(provider);
  std::vector<FileHash> sourceFiles = request.frontend.sourceFiles;
  std::sort(sourceFiles.begin(), sourceFiles.end(),
            [](const FileHash &lhs, const FileHash &rhs) {
              return lhs.path < rhs.path;
            });
  llvm::json::Array frontendSources;
  for (const FileHash &source : sourceFiles)
    frontendSources.push_back(
        llvm::json::Object{{"path", source.path}, {"sha256", source.sha256}});
  llvm::json::Array helpers;
  for (const NamedFingerprint &helper : request.frontend.helperIdentities)
    helpers.push_back(llvm::json::Object{{"name", helper.name},
                                         {"fingerprint", helper.fingerprint}});
  llvm::json::Object frontend{
      {"source_files", std::move(frontendSources)},
      {"acpy", llvm::json::Object{{"path", request.frontend.acpy.path},
                                  {"sha256", request.frontend.acpy.sha256}}},
      {"canonical_acir",
       llvm::json::Object{{"path", request.frontend.canonicalAcir.path},
                          {"sha256", request.frontend.canonicalAcir.sha256}}},
      {"python_version", request.frontend.pythonVersion},
      {"helpers", std::move(helpers)}};
  llvm::json::Object preimage{
      {"domain", "agentic-circuit-generated-build-0.1"},
      {"source_bundle", bundle.sourceFingerprint},
      {"compile_plan", compilePlan.fingerprint},
      {"project", llvm::json::Object{{"name", request.project.name},
                                     {"identity", request.project.identity}}},
      {"system", llvm::json::Object{{"name", request.system.name},
                                    {"identity", request.system.identity}}},
      {"frontend", std::move(frontend)},
      {"profile", request.profile},
      {"pass_pipeline",
       [&] {
         llvm::json::Array pipeline;
         for (const std::string &pass : request.passPipeline)
           pipeline.push_back(pass);
         return pipeline;
       }()},
      {"instrumentation_layers", std::move(instrumentation)},
      {"providers", std::move(providers)}};
  return fingerprintCanonicalJson(llvm::json::Value(std::move(preimage)));
}

llvm::Error retargetBundle(SourceBundle &bundle, llvm::StringRef fingerprint) {
  const std::string previous = bundle.buildFingerprint;
  size_t replacements = 0;
  for (GeneratedFile &file : bundle.files) {
    size_t position = file.content.find(previous);
    if (position == std::string::npos)
      continue;
    if (file.content.find(previous, position + previous.size()) !=
        std::string::npos)
      return buildError("generated fingerprint occurs more than once");
    file.content.replace(position, previous.size(), fingerprint);
    file.fingerprint = computeFingerprint(file.content);
    ++replacements;
  }
  if (replacements != 1)
    return buildError("generated model has no unique embedded fingerprint");
  bundle.buildFingerprint = fingerprint.str();
  return llvm::Error::success();
}

ArtifactKind sourceArtifactKind(llvm::StringRef path) {
  return path.ends_with(".h") ? ArtifactKind::CppHeader
                              : ArtifactKind::CppSource;
}

llvm::Error addArtifact(llvm::StringRef stage, llvm::StringRef path,
                        llvm::StringRef bytes, ArtifactKind kind,
                        std::vector<Artifact> &artifacts) {
  if (auto error = writeFileExclusive(stage, path, bytes))
    return error;
  artifacts.push_back({path.str(), kind, computeFingerprint(bytes)});
  return llvm::Error::success();
}

std::vector<std::string>
resolveArguments(const CompilePlan &plan, llvm::StringRef stage,
                 llvm::ArrayRef<std::string> arguments) {
  std::set<std::string> stagedFiles(plan.sourceUnits.begin(),
                                    plan.sourceUnits.end());
  stagedFiles.insert(plan.objectOutputs.begin(), plan.objectOutputs.end());
  stagedFiles.insert(plan.executablePath);
  for (const PrebuiltInput &input : plan.prebuiltInputs)
    stagedFiles.insert(input.path);

  std::vector<std::string> result;
  result.reserve(arguments.size());
  for (const std::string &argument : arguments) {
    if (stagedFiles.contains(argument)) {
      result.push_back(stagedPath(stage, argument).str().str());
      continue;
    }
    llvm::StringRef value(argument);
    if (value.starts_with("-I")) {
      llvm::StringRef root = value.drop_front(2);
      if (!llvm::sys::path::is_absolute(root))
        result.push_back("-I" + stagedPath(stage, root).str().str());
      else
        result.push_back(argument);
      continue;
    }
    result.push_back(argument);
  }
  return result;
}

llvm::Error execute(BuildServices &services,
                    const std::vector<std::string> &ownedArguments) {
  llvm::SmallVector<llvm::StringRef> arguments;
  for (const std::string &argument : ownedArguments)
    arguments.push_back(argument);
  if (!services.execute)
    return buildError("build execution service is absent");
  return services.execute(arguments);
}

llvm::Expected<std::string> queryFingerprint(llvm::StringRef executable,
                                             llvm::StringRef reportPath) {
  const std::array<llvm::StringRef, 2> arguments = {executable,
                                                    "--build-fingerprint"};
  const std::array<std::optional<llvm::StringRef>, 3> redirects = {
      std::nullopt, reportPath, reportPath};
  const int status = llvm::sys::ExecuteAndWait(executable, arguments,
                                               std::nullopt, redirects, 30);
  if (status != 0) {
    auto report = readFileBytes(reportPath);
    if (!report)
      return report.takeError();
    constexpr size_t maxDiagnosticBytes = size_t{64} * 1024;
    if (report->size() > maxDiagnosticBytes)
      report->resize(maxDiagnosticBytes);
    return buildError(llvm::Twine("embedded fingerprint query failed: ") +
                      *report);
  }
  auto bytes = readFileBytes(reportPath);
  if (!bytes)
    return bytes.takeError();
  return llvm::StringRef(*bytes).trim().str();
}

llvm::Expected<BuildManifest> makeManifest(const BuildRequest &request,
                                           const ModelPlan &model,
                                           const SourceBundle &bundle,
                                           llvm::ArrayRef<Artifact> artifacts) {
  BuildManifest manifest;
  manifest.project = request.project;
  manifest.system = request.system;
  manifest.normalizedAcirSha256 = computeFingerprint(request.frozenAcirBytes);
  manifest.compiler = {request.toolchain.compilerName,
                       request.toolchain.compilerBuildId,
                       request.toolchain.targetTriple};
  manifest.passPipeline = request.passPipeline;
  std::map<std::string, std::string> providerNamespaces;
  std::map<std::string, std::string> typeIdentities;
  for (const TypePlan &type : model.types)
    if (type.kind == TypeKind::Provider)
      providerNamespaces.emplace(type.symbol, type.cppType);
    else
      typeIdentities.emplace(type.symbol, type.cppType);
  std::map<std::string, Fingerprint> providerImplementations;
  for (const BindingPlan &binding : model.bindings) {
    auto provider = providerNamespaces.find(binding.provider);
    if (provider == providerNamespaces.end())
      return buildError("binding references an unknown provider identity");
    auto [iterator, inserted] = providerImplementations.emplace(
        provider->second, binding.providerImplementationFingerprint);
    if (!inserted &&
        iterator->second != binding.providerImplementationFingerprint)
      return buildError(
          "one provider has conflicting implementation identities");
  }
  for (const auto &[nameSpace, implementation] : providerImplementations)
    manifest.providers.push_back(
        {nameSpace, model.schemaSetFingerprint, implementation});
  std::map<std::string, ComponentSpecialization> specializations;
  for (const ModulePlan &module : model.modules)
    specializations.emplace(
        module.symbol,
        ComponentSpecialization{module.symbol, model.schemaSetFingerprint,
                                module.specializationFingerprint});
  for (const BindingPlan &binding : model.bindings) {
    auto schema = typeIdentities.find(binding.componentSchema);
    if (schema == typeIdentities.end())
      return buildError("binding references an unknown component schema");
    ComponentSpecialization specialization{schema->second,
                                           binding.componentSchemaFingerprint,
                                           binding.recordFingerprint};
    auto [iterator, inserted] =
        specializations.emplace(specialization.canonicalName, specialization);
    if (!inserted && (iterator->second.schemaFingerprint !=
                          specialization.schemaFingerprint ||
                      iterator->second.specializationFingerprint !=
                          specialization.specializationFingerprint))
      return buildError(
          "one component has conflicting specialization identities");
    for (const ParameterPlan &parameter : binding.parameters)
      manifest.specializationInputs.push_back(
          {binding.symbol + "." + parameter.name, parameter.acirType,
           parameter.canonicalValue});
  }
  for (const auto &[name, specialization] : specializations)
    manifest.componentSpecializations.push_back(specialization);
  for (const TypePlan &type : model.types)
    if (type.kind == TypeKind::Protocol)
      manifest.protocolIdentities.push_back({type.cppType, type.fingerprint});
  manifest.artifacts.assign(artifacts.begin(), artifacts.end());
  manifest.validationGates = {
      {"generated_source_contract", ValidationStatus::Passed, std::nullopt},
      {"compile", ValidationStatus::Passed, std::nullopt},
      {"link", ValidationStatus::Passed, std::nullopt},
      {"embedded_fingerprint", ValidationStatus::Passed, std::nullopt}};
  manifest.buildProfile = request.profile;
  manifest.instrumentationLayers = request.instrumentationLayers;
  manifest.buildFingerprint = bundle.buildFingerprint;
  manifest.sourceFiles = request.frontend.sourceFiles;
  for (const GeneratedFile &file : bundle.files)
    manifest.sourceFiles.push_back({file.relativePath, file.fingerprint});
  return manifest;
}

} // namespace

BuildServices makeRealBuildServices() {
  BuildServices services;
  services.execute =
      [](llvm::ArrayRef<llvm::StringRef> arguments) -> llvm::Error {
    if (arguments.empty())
      return buildError("cannot execute an empty command vector");
    llvm::SmallString<256> outputPath;
    if (std::error_code error = llvm::sys::fs::createTemporaryFile(
            "acir-build-command", "log", outputPath))
      return llvm::createStringError(error,
                                     "cannot create command report capture");
    struct RemoveCapture {
      llvm::SmallString<256> path;
      ~RemoveCapture() { llvm::sys::fs::remove(path); }
    } cleanup{outputPath};
    const std::array<std::optional<llvm::StringRef>, 3> redirects = {
        std::nullopt, outputPath.str(), outputPath.str()};
    const int status = llvm::sys::ExecuteAndWait(arguments.front(), arguments,
                                                 std::nullopt, redirects, 120);
    if (status != 0) {
      auto report = readFileBytes(outputPath);
      if (!report)
        return report.takeError();
      constexpr size_t maxDiagnosticBytes = size_t{64} * 1024;
      if (report->size() > maxDiagnosticBytes)
        report->resize(maxDiagnosticBytes);
      return buildError(llvm::Twine("compiler or linker command failed: ") +
                        *report);
    }
    return llvm::Error::success();
  };
  return services;
}

llvm::Expected<BuildResult>
buildGeneratedModelForTesting(const BuildRequest &request,
                              BuildServices &services) {
  if (auto error = preflightBuildRequest(request))
    return std::move(error);
  if (!request.canonicalACSim || request.outputRoot.empty())
    return buildError("build requires canonical ACSim and an output root");
  if (auto error = injectedFailure(services.failurePoint,
                                   BuildFailurePoint::AfterInputValidation))
    return std::move(error);

  auto model = buildModelPlan(request.canonicalACSim);
  if (!model)
    return model.takeError();
  auto bundle = generateModelSources(*model);
  if (!bundle)
    return bundle.takeError();
  auto preliminaryPlan = createCompilePlan(request, *bundle);
  if (!preliminaryPlan)
    return preliminaryPlan.takeError();
  auto buildFingerprint =
      publicationFingerprint(request, *bundle, *preliminaryPlan);
  if (!buildFingerprint)
    return buildFingerprint.takeError();
  if (auto error = retargetBundle(*bundle, *buildFingerprint))
    return std::move(error);
  auto compilePlan = createCompilePlan(request, *bundle);
  if (!compilePlan)
    return compilePlan.takeError();

  if (std::error_code error =
          llvm::sys::fs::create_directories(request.outputRoot))
    return llvm::createStringError(error, "cannot create build output root");
  llvm::SmallString<256> stagePrefix(request.outputRoot);
  llvm::sys::path::append(stagePrefix, ".stage");
  llvm::SmallString<256> stage;
  if (std::error_code error =
          llvm::sys::fs::createUniqueDirectory(stagePrefix, stage))
    return llvm::createStringError(error, "cannot create private build stage");
  struct Cleanup {
    llvm::SmallString<256> path;
    ~Cleanup() {
      if (llvm::sys::fs::exists(path))
        llvm::sys::fs::remove_directories(path);
    }
  } cleanup{stage};

  std::vector<Artifact> artifacts;
  std::string canonicalACSimBytes = request.canonicalACSimBytes;
  if (canonicalACSimBytes.empty()) {
    llvm::raw_string_ostream output(canonicalACSimBytes);
    mlir::ModuleOp canonical = request.canonicalACSim;
    canonical.print(output);
    output.flush();
  }
  if (!request.frontend.sourceFiles.empty()) {
    if (auto error = addArtifact(stage, request.frontend.acpy.path,
                                 request.frontend.acpyBytes, ArtifactKind::Acpy,
                                 artifacts))
      return std::move(error);
    if (auto error = addArtifact(stage, request.frontend.canonicalAcir.path,
                                 request.frontend.canonicalAcirBytes,
                                 ArtifactKind::Acir, artifacts))
      return std::move(error);
  }
  if (auto error =
          addArtifact(stage, "input/frozen.acir", request.frozenAcirBytes,
                      ArtifactKind::Acir, artifacts))
    return std::move(error);
  if (auto error = addArtifact(stage, "input/model.acsim", canonicalACSimBytes,
                               ArtifactKind::Acsim, artifacts))
    return std::move(error);
  if (auto error = addArtifact(stage, "input/binding-lock.json",
                               request.bindingLockBytes, ArtifactKind::Report,
                               artifacts))
    return std::move(error);
  for (const GeneratedFile &file : bundle->files)
    if (auto error =
            addArtifact(stage, file.relativePath, file.content,
                        sourceArtifactKind(file.relativePath), artifacts))
      return std::move(error);
  auto compilePlanBytes = compilePlan->canonicalJson();
  if (!compilePlanBytes)
    return compilePlanBytes.takeError();
  if (auto error = addArtifact(stage, "compile-plan.json", *compilePlanBytes,
                               ArtifactKind::Report, artifacts))
    return std::move(error);
  if (auto error = injectedFailure(services.failurePoint,
                                   BuildFailurePoint::AfterSourceWrite))
    return std::move(error);

  if (auto error = validateSourceBundle(*model, *bundle))
    return std::move(error);
  if (auto error = compilePlan->validate())
    return std::move(error);
  if (auto error = addArtifact(stage, "reports/source-contract.txt", "passed\n",
                               ArtifactKind::Report, artifacts))
    return std::move(error);
  if (auto error = injectedFailure(services.failurePoint,
                                   BuildFailurePoint::AfterContractCheck))
    return std::move(error);

  for (const std::string &output : compilePlan->objectOutputs) {
    llvm::SmallString<256> parent = stagedPath(stage, output);
    llvm::sys::path::remove_filename(parent);
    if (std::error_code error = llvm::sys::fs::create_directories(parent))
      return llvm::createStringError(error, "cannot create object directory");
  }
  for (const CompileCommand &command : compilePlan->compileCommands) {
    auto arguments = resolveArguments(*compilePlan, stage, command.arguments);
    if (auto error = execute(services, arguments))
      return std::move(error);
  }
  if (auto error = addArtifact(stage, "reports/compile.txt", "passed\n",
                               ArtifactKind::Report, artifacts))
    return std::move(error);
  if (auto error = injectedFailure(services.failurePoint,
                                   BuildFailurePoint::AfterCompile))
    return std::move(error);

  llvm::SmallString<256> executableParent =
      stagedPath(stage, compilePlan->executablePath);
  llvm::sys::path::remove_filename(executableParent);
  if (std::error_code error =
          llvm::sys::fs::create_directories(executableParent))
    return llvm::createStringError(error, "cannot create executable directory");
  auto linkArguments =
      resolveArguments(*compilePlan, stage, compilePlan->linkCommand.arguments);
  if (auto error = execute(services, linkArguments))
    return std::move(error);
  if (auto error = addArtifact(stage, "reports/link.txt", "passed\n",
                               ArtifactKind::Report, artifacts))
    return std::move(error);
  if (auto error =
          injectedFailure(services.failurePoint, BuildFailurePoint::AfterLink))
    return std::move(error);

  llvm::SmallString<256> fingerprintReport =
      stagedPath(stage, "reports/embedded-fingerprint.txt");
  auto observed = queryFingerprint(
      stagedPath(stage, compilePlan->executablePath), fingerprintReport);
  if (!observed)
    return observed.takeError();
  if (*observed != *buildFingerprint)
    return buildError("embedded build fingerprint does not match plan");
  artifacts.push_back({"reports/embedded-fingerprint.txt", ArtifactKind::Report,
                       computeFingerprint(*observed + "\n")});
  auto executableBytes =
      readFileBytes(stagedPath(stage, compilePlan->executablePath));
  if (!executableBytes)
    return executableBytes.takeError();
  artifacts.push_back({compilePlan->executablePath, ArtifactKind::Executable,
                       computeFingerprint(*executableBytes)});
  for (const std::string &object : compilePlan->objectOutputs) {
    if (std::error_code error =
            llvm::sys::fs::remove(stagedPath(stage, object)))
      return llvm::createStringError(error,
                                     "cannot remove staged object output");
  }
  if (auto error = injectedFailure(services.failurePoint,
                                   BuildFailurePoint::AfterFingerprintQuery))
    return std::move(error);

  auto manifest = makeManifest(request, *model, *bundle, artifacts);
  if (!manifest)
    return manifest.takeError();
  auto manifestBytes = manifest->canonicalJson();
  if (!manifestBytes)
    return manifestBytes.takeError();
  if (auto error =
          writeFileExclusive(stage, "build-manifest.json", *manifestBytes))
    return std::move(error);
  if (auto error = injectedFailure(services.failurePoint,
                                   BuildFailurePoint::AfterManifestWrite))
    return std::move(error);

  auto published = publishImmutableStage(
      stage, request.outputRoot, *buildFingerprint, artifacts, *manifestBytes);
  if (!published)
    return published.takeError();
  if (auto error = injectedFailure(services.failurePoint,
                                   BuildFailurePoint::AfterImmutableRename))
    return std::move(error);
  if (auto error = injectedFailure(services.failurePoint,
                                   BuildFailurePoint::BeforeCurrentRename))
    return std::move(error);
  if (auto error = writeCurrentPointer(request.outputRoot, *buildFingerprint))
    return std::move(error);

  return BuildResult{published->path,
                     published->path + "/" + compilePlan->executablePath,
                     *buildFingerprint, published->cacheHit};
}

llvm::Expected<BuildResult> buildGeneratedModel(const BuildRequest &request) {
  BuildServices services = makeRealBuildServices();
  return buildGeneratedModelForTesting(request, services);
}

} // namespace acir::codegen
