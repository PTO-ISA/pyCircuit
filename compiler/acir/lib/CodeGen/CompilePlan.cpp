#include "acir/CodeGen/Build.h"

#include "acir/Bindings/Binding.h"

#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/Twine.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/JSON.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/Program.h"
#include "llvm/Support/raw_ostream.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <optional>
#include <set>
#include <string_view>
#include <system_error>
#include <utility>

namespace acir::codegen {
namespace {

llvm::Error buildError(llvm::StringRef code, const llvm::Twine &message) {
  return llvm::createStringError(
      std::make_error_code(std::errc::invalid_argument),
      llvm::Twine(code) + ": " + message);
}

bool isNormalizedRelativePath(llvm::StringRef path) {
  if (path.empty() || path.starts_with('/') || path.ends_with('/') ||
      path.contains('\\') || path.contains('\0'))
    return false;
  while (!path.empty()) {
    auto [component, remainder] = path.split('/');
    if (component.empty() || component == "." || component == "..")
      return false;
    path = remainder;
  }
  return true;
}

bool isCanonicalIncludeRoot(llvm::StringRef path) {
  if (isNormalizedRelativePath(path))
    return true;
  if (!llvm::sys::path::is_absolute(path) || path.contains('\0') ||
      path.contains('\\') || path.contains("//") || path.ends_with('/'))
    return false;
  llvm::SmallString<256> normalized(path);
  llvm::sys::path::remove_dots(normalized, true);
  return normalized == path;
}

bool isSortedUnique(const std::vector<std::string> &values) {
  return std::adjacent_find(values.begin(), values.end(),
                            std::greater_equal<>()) == values.end();
}

std::string trimOutput(llvm::StringRef value) { return value.trim().str(); }

llvm::Expected<std::string>
runAndCapture(llvm::StringRef program,
              const std::vector<std::string> &ownedArguments) {
  llvm::SmallString<256> outputPath;
  if (std::error_code error = llvm::sys::fs::createTemporaryFile(
          "acir-toolchain", "txt", outputPath))
    return llvm::createStringError(error, "cannot create toolchain probe file");
  struct Cleanup {
    llvm::SmallString<256> path;
    ~Cleanup() { llvm::sys::fs::remove(path); }
  } cleanup{outputPath};

  llvm::SmallVector<llvm::StringRef> arguments;
  for (const std::string &argument : ownedArguments)
    arguments.push_back(argument);
  const std::array<std::optional<llvm::StringRef>, 3> redirects = {
      std::nullopt, outputPath.str(), outputPath.str()};
  const int status = llvm::sys::ExecuteAndWait(program, arguments, std::nullopt,
                                               redirects, 30);
  if (status != 0)
    return buildError("ACLOWER-FINGERPRINT", "compiler identity probe failed");
  auto buffer = llvm::MemoryBuffer::getFile(outputPath);
  if (!buffer)
    return llvm::createStringError(buffer.getError(),
                                   "cannot read toolchain probe output");
  return trimOutput(buffer.get()->getBuffer());
}

llvm::json::Array stringsJson(const std::vector<std::string> &values) {
  llvm::json::Array result;
  for (const std::string &value : values)
    result.push_back(value);
  return result;
}

llvm::json::Object provenanceJson(const PrebuiltProvenance &provenance) {
  return llvm::json::Object{
      {"compiler_build_id", provenance.compilerBuildId},
      {"target_triple", provenance.targetTriple},
      {"standard_library", provenance.standardLibrary},
      {"abi_mode", provenance.abiMode},
      {"object_format", provenance.objectFormat},
      {"contract_epoch", provenance.contractEpoch},
      {"contract_flags", stringsJson(provenance.contractFlags)},
      {"toolchain_fingerprint", provenance.toolchainFingerprint},
      {"source_fingerprint", provenance.sourceFingerprint}};
}

llvm::json::Object prebuiltJson(const PrebuiltInput &input) {
  return llvm::json::Object{{"path", input.path},
                            {"kind", input.kind},
                            {"provenance", provenanceJson(input.provenance)},
                            {"source_available", input.sourceAvailable}};
}

llvm::json::Object commandJson(const CompileCommand &command) {
  return llvm::json::Object{{"arguments", stringsJson(command.arguments)},
                            {"output", command.output}};
}

llvm::json::Object linkInputJson(const LinkInputIdentity &input) {
  return llvm::json::Object{{"path", input.path},
                            {"fingerprint", input.fingerprint}};
}

llvm::json::Object planJson(const CompilePlan &plan,
                            llvm::StringRef fingerprint) {
  llvm::json::Array prebuilts;
  for (const PrebuiltInput &input : plan.prebuiltInputs)
    prebuilts.push_back(prebuiltJson(input));
  llvm::json::Array commands;
  for (const CompileCommand &command : plan.compileCommands)
    commands.push_back(commandJson(command));
  llvm::json::Array linkInputs;
  for (const LinkInputIdentity &input : plan.linkInputs)
    linkInputs.push_back(linkInputJson(input));
  return llvm::json::Object{
      {"schema", plan.schema},
      {"source_units", stringsJson(plan.sourceUnits)},
      {"object_outputs", stringsJson(plan.objectOutputs)},
      {"include_roots", stringsJson(plan.includeRoots)},
      {"definitions", stringsJson(plan.definitions)},
      {"compiler_flags", stringsJson(plan.compilerFlags)},
      {"linker_flags", stringsJson(plan.linkerFlags)},
      {"link_inputs", std::move(linkInputs)},
      {"prebuilt_inputs", std::move(prebuilts)},
      {"compile_commands", std::move(commands)},
      {"link_command", commandJson(plan.linkCommand)},
      {"executable_path", plan.executablePath},
      {"source_fingerprint", plan.sourceFingerprint},
      {"toolchain_fingerprint", plan.toolchainFingerprint},
      {"fingerprint", fingerprint}};
}

llvm::Expected<Fingerprint>
toolchainFingerprint(const ToolchainIdentity &toolchain) {
  std::vector<std::string> flags = toolchain.contractFlags;
  std::sort(flags.begin(), flags.end());
  llvm::json::Object identity{{"domain", "acsim-toolchain-0.1"},
                              {"compiler_path", toolchain.compilerPath},
                              {"compiler_name", toolchain.compilerName},
                              {"compiler_build_id", toolchain.compilerBuildId},
                              {"target_triple", toolchain.targetTriple},
                              {"standard_library", toolchain.standardLibrary},
                              {"abi_mode", toolchain.abiMode},
                              {"object_format", toolchain.objectFormat},
                              {"contract_flags", stringsJson(flags)}};
  return fingerprintCanonicalJson(llvm::json::Value(std::move(identity)));
}

bool provenanceMatches(const PrebuiltProvenance &provenance,
                       const ToolchainIdentity &toolchain) {
  std::vector<std::string> expectedFlags = toolchain.contractFlags;
  std::vector<std::string> actualFlags = provenance.contractFlags;
  std::sort(expectedFlags.begin(), expectedFlags.end());
  std::sort(actualFlags.begin(), actualFlags.end());
  return provenance.compilerBuildId == toolchain.compilerBuildId &&
         provenance.targetTriple == toolchain.targetTriple &&
         provenance.standardLibrary == toolchain.standardLibrary &&
         provenance.abiMode == toolchain.abiMode &&
         provenance.objectFormat == toolchain.objectFormat &&
         provenance.contractEpoch == "0.5" && actualFlags == expectedFlags &&
         provenance.toolchainFingerprint == toolchain.fingerprint &&
         isValidFingerprint(provenance.sourceFingerprint);
}

llvm::Expected<Fingerprint> fingerprintJsonValue(llvm::json::Value value) {
  auto canonical = bindings::canonicalizeJson(value);
  if (!canonical)
    return canonical.takeError();
  return computeFingerprint(*canonical);
}

std::string objectPath(llvm::StringRef source) {
  std::string result = "obj/";
  for (char character : source) {
    if (std::isalnum(static_cast<unsigned char>(character)) ||
        character == '_' || character == '-')
      result.push_back(character);
    else
      result.push_back('_');
  }
  result.append(".o");
  return result;
}

} // namespace

llvm::Expected<ToolchainIdentity>
identifyToolchain(std::string compilerPath, std::string standardLibrary,
                  std::string abiMode, std::string objectFormat,
                  std::vector<std::string> contractFlags) {
  if (compilerPath.empty() || standardLibrary.empty() || abiMode.empty() ||
      objectFormat.empty())
    return buildError("ACLOWER-FINGERPRINT",
                      "toolchain fields must be explicit");
  auto version = runAndCapture(compilerPath, {compilerPath, "--version"});
  if (!version)
    return version.takeError();
  auto target = runAndCapture(compilerPath, {compilerPath, "-dumpmachine"});
  if (!target)
    return target.takeError();

  const llvm::StringRef versionText(*version);
  ToolchainIdentity result;
  result.compilerPath = std::move(compilerPath);
  result.compilerBuildId = versionText.split('\n').first.str();
  result.compilerName = versionText.split(" version ").first.trim().str();
  result.targetTriple = llvm::StringRef(*target).split('\n').first.trim().str();
  result.standardLibrary = std::move(standardLibrary);
  result.abiMode = std::move(abiMode);
  result.objectFormat = std::move(objectFormat);
  result.contractFlags = std::move(contractFlags);
  auto fingerprint = toolchainFingerprint(result);
  if (!fingerprint)
    return fingerprint.takeError();
  result.fingerprint = std::move(*fingerprint);
  return result;
}

llvm::Error preflightBuildRequest(const BuildRequest &request) {
  if (request.project.name.empty() || request.project.identity.empty() ||
      request.system.name.empty() || request.system.identity.empty())
    return buildError("ACLOWER-FINGERPRINT",
                      "project and system identities must be explicit");
  if (request.profile != "fast" && request.profile != "validated" &&
      request.profile != "custom")
    return buildError("ACLOWER-PROFILE",
                      "build profile is outside the closed set");
  if (request.canonicalACSim &&
      (request.passPipeline.empty() ||
       std::any_of(request.passPipeline.begin(), request.passPipeline.end(),
                   [](const std::string &pass) { return pass.empty(); })))
    return buildError("ACLOWER-PROFILE", "build pass pipeline is incomplete");
  const FrontendProvenance &frontend = request.frontend;
  const bool hasFrontend =
      !frontend.sourceFiles.empty() || !frontend.acpy.path.empty() ||
      !frontend.acpyBytes.empty() || !frontend.canonicalAcir.path.empty() ||
      !frontend.canonicalAcirBytes.empty() || !frontend.pythonVersion.empty() ||
      !frontend.helperIdentities.empty();
  if (hasFrontend) {
    if (frontend.sourceFiles.empty() || frontend.pythonVersion.empty())
      return buildError("ACLOWER-FINGERPRINT",
                        "frontend source and Python identities are required");
    std::set<std::string> frontendPaths;
    for (const FileHash &source : frontend.sourceFiles)
      if (!isNormalizedRelativePath(source.path) ||
          !isValidFingerprint(source.sha256) ||
          !frontendPaths.insert(source.path).second)
        return buildError("ACLOWER-FINGERPRINT",
                          "frontend source identity is invalid");
    auto validFrontendArtifact = [](const Artifact &artifact,
                                    ArtifactKind expected,
                                    llvm::StringRef bytes) {
      return artifact.kind == expected &&
             isNormalizedRelativePath(artifact.path) &&
             isValidFingerprint(artifact.sha256) &&
             artifact.sha256 == computeFingerprint(bytes);
    };
    if (!validFrontendArtifact(frontend.acpy, ArtifactKind::Acpy,
                               frontend.acpyBytes) ||
        !validFrontendArtifact(frontend.canonicalAcir, ArtifactKind::Acir,
                               frontend.canonicalAcirBytes) ||
        frontend.acpy.path == frontend.canonicalAcir.path)
      return buildError("ACLOWER-FINGERPRINT",
                        "frontend artifacts are invalid or inconsistent");
    llvm::StringRef previousHelper;
    for (const NamedFingerprint &helper : frontend.helperIdentities) {
      if (helper.name.empty() || !isValidFingerprint(helper.fingerprint) ||
          (!previousHelper.empty() && previousHelper >= helper.name))
        return buildError("ACLOWER-FINGERPRINT",
                          "frontend helper identities are not canonical");
      previousHelper = helper.name;
    }
  }
  const ToolchainIdentity &toolchain = request.toolchain;
  if (toolchain.compilerPath.empty() || toolchain.compilerName.empty() ||
      toolchain.compilerBuildId.empty() || toolchain.targetTriple.empty() ||
      toolchain.standardLibrary.empty() || toolchain.abiMode.empty() ||
      toolchain.objectFormat.empty() || toolchain.contractFlags.empty() ||
      !isValidFingerprint(toolchain.fingerprint))
    return buildError("ACLOWER-FINGERPRINT",
                      "toolchain identity is incomplete");
  if (std::none_of(toolchain.contractFlags.begin(),
                   toolchain.contractFlags.end(), [](llvm::StringRef flag) {
                     return flag == "-std=c++20" || flag == "-std=gnu++20";
                   }))
    return buildError("ACLOWER-FINGERPRINT",
                      "toolchain does not require C++20");
  auto expectedFingerprint = toolchainFingerprint(toolchain);
  if (!expectedFingerprint || *expectedFingerprint != toolchain.fingerprint)
    return buildError("ACLOWER-FINGERPRINT",
                      "toolchain fingerprint does not match its fields");

  auto observed = identifyToolchain(
      toolchain.compilerPath, toolchain.standardLibrary, toolchain.abiMode,
      toolchain.objectFormat, toolchain.contractFlags);
  if (!observed || observed->compilerBuildId != toolchain.compilerBuildId ||
      observed->targetTriple != toolchain.targetTriple ||
      observed->fingerprint != toolchain.fingerprint)
    return buildError("ACLOWER-FINGERPRINT",
                      "compiler probe does not match declared toolchain");

  std::set<std::string> paths;
  for (const PrebuiltInput &input : request.prebuiltInputs) {
    if (!isNormalizedRelativePath(input.path) || input.kind.empty() ||
        !paths.insert(input.path).second)
      return buildError("ACLOWER-FINGERPRINT",
                        "prebuilt input identity is invalid");
    if (!provenanceMatches(input.provenance, toolchain) &&
        !input.sourceAvailable)
      return buildError("ACLOWER-FINGERPRINT",
                        "prebuilt provenance does not match toolchain");
  }

  if (request.canonicalACSim) {
    if (request.frozenAcirBytes.empty() || request.bindingLockBytes.empty())
      return buildError("ACLOWER-FINGERPRINT",
                        "frozen ACIR and binding lock bytes are required");
    auto model = buildModelPlan(request.canonicalACSim);
    if (!model)
      return model.takeError();
    if (!request.frozenAcirBytes.empty() &&
        computeFingerprint(request.frozenAcirBytes) !=
            model->frozenAcirFingerprint)
      return buildError("ACLOWER-FINGERPRINT",
                        "frozen ACIR bytes do not match ACSim");
    if (!request.bindingLockBytes.empty()) {
      auto canonicalLock =
          bindings::canonicalizeJsonText(request.bindingLockBytes);
      if (!canonicalLock)
        return canonicalLock.takeError();
      if (computeFingerprint(*canonicalLock) != model->bindingLockFingerprint)
        return buildError("ACLOWER-FINGERPRINT",
                          "binding lock bytes do not match ACSim");
    }
    auto profileFingerprint =
        fingerprintJsonValue(llvm::json::Value(request.profile));
    auto targetFingerprint =
        fingerprintJsonValue(llvm::json::Value(toolchain.targetTriple));
    if (!profileFingerprint)
      return profileFingerprint.takeError();
    if (!targetFingerprint)
      return targetFingerprint.takeError();
    if (*profileFingerprint != model->profileFingerprint ||
        *targetFingerprint != model->toolchainFingerprint)
      return buildError("ACLOWER-PROFILE",
                        "profile or target identity does not match ACSim");

    std::vector<std::string> providers = request.providerInputs;
    std::sort(providers.begin(), providers.end());
    providers.erase(std::unique(providers.begin(), providers.end()),
                    providers.end());
    llvm::json::Array providerArray;
    for (const std::string &provider : providers)
      providerArray.push_back(provider);
    auto providerFingerprint =
        fingerprintJsonValue(llvm::json::Value(std::move(providerArray)));
    if (!providerFingerprint)
      return providerFingerprint.takeError();
    if (*providerFingerprint != model->providerFingerprint)
      return buildError("ACLOWER-FINGERPRINT",
                        "provider identity set does not match ACSim");

    if (!request.canonicalACSimBytes.empty()) {
      std::string printed;
      llvm::raw_string_ostream stream(printed);
      mlir::ModuleOp canonicalACSim = request.canonicalACSim;
      canonicalACSim.print(stream);
      stream.flush();
      if (printed != request.canonicalACSimBytes)
        return buildError("ACLOWER-FINGERPRINT",
                          "canonical ACSim bytes do not match the module");
    }
  }
  return llvm::Error::success();
}

llvm::Error CompilePlan::validate() const {
  if (schema != "acsim-compile-plan-0.1")
    return buildError("ACLOWER-FINGERPRINT", "compile plan schema is invalid");
  if (!isValidFingerprint(sourceFingerprint) ||
      !isValidFingerprint(toolchainFingerprint) ||
      !isValidFingerprint(fingerprint) ||
      sourceUnits.size() != objectOutputs.size() ||
      sourceUnits.size() != compileCommands.size() || executablePath.empty())
    return buildError("ACLOWER-FINGERPRINT",
                      "compile plan identity or command arity is invalid");
  if (!isSortedUnique(includeRoots) || !isSortedUnique(definitions))
    return buildError("ACLOWER-FINGERPRINT",
                      "compile plan set-like fields are not canonical");
  for (llvm::StringRef root : includeRoots)
    if (!isCanonicalIncludeRoot(root))
      return buildError("ACLOWER-FINGERPRINT",
                        "compile plan include root is not canonical");
  for (llvm::StringRef path : sourceUnits)
    if (!isNormalizedRelativePath(path))
      return buildError("ACLOWER-FINGERPRINT",
                        "compile plan contains a non-normalized source path");
  for (llvm::StringRef path : objectOutputs)
    if (!isNormalizedRelativePath(path))
      return buildError("ACLOWER-FINGERPRINT",
                        "compile plan contains a non-normalized object path");
  for (const LinkInputIdentity &input : linkInputs)
    if (!isCanonicalIncludeRoot(input.path) ||
        !isValidFingerprint(input.fingerprint))
      return buildError("ACLOWER-FINGERPRINT",
                        "compile plan link input identity is invalid");
  if (!isNormalizedRelativePath(executablePath) ||
      linkCommand.arguments.empty() || linkCommand.output != executablePath)
    return buildError("ACLOWER-FINGERPRINT",
                      "compile plan link command is invalid");
  for (auto [index, command] : llvm::enumerate(compileCommands)) {
    if (command.arguments.empty() || command.output != objectOutputs[index] ||
        command.arguments.back() != command.output)
      return buildError("ACLOWER-FINGERPRINT",
                        "compile command is not a closed argument vector");
  }
  return llvm::Error::success();
}

llvm::Expected<std::string> CompilePlan::canonicalJson() const {
  if (auto error = validate())
    return std::move(error);
  return bindings::canonicalizeJson(
      llvm::json::Value(planJson(*this, fingerprint)));
}

llvm::Expected<CompilePlan> createCompilePlan(const BuildRequest &request,
                                              const SourceBundle &bundle) {
  if (auto error = preflightBuildRequest(request))
    return std::move(error);
  if (!isValidFingerprint(bundle.sourceFingerprint) ||
      !isValidFingerprint(bundle.buildFingerprint))
    return buildError("ACLOWER-FINGERPRINT",
                      "source bundle build identity is invalid");
  llvm::StringRef previousPath;
  for (const GeneratedFile &file : bundle.files) {
    if (!isNormalizedRelativePath(file.relativePath) ||
        (!previousPath.empty() && previousPath >= file.relativePath) ||
        file.fingerprint != computeFingerprint(file.content) ||
        file.content.find('\r') != std::string::npos)
      return buildError("ACLOWER-FINGERPRINT",
                        "source bundle is not canonical and complete");
    previousPath = file.relativePath;
  }

  CompilePlan plan;
  plan.includeRoots = request.includeRoots;
  llvm::StringRef llvmIncludeRoots(ACIR_CODEGEN_LLVM_INCLUDE_DIRS);
  while (!llvmIncludeRoots.empty()) {
    auto [root, remainder] = llvmIncludeRoots.split('|');
    if (!root.empty())
      plan.includeRoots.push_back(root.str());
    llvmIncludeRoots = remainder;
  }
  plan.includeRoots.push_back("include");
  plan.definitions = request.definitions;
  std::sort(plan.includeRoots.begin(), plan.includeRoots.end());
  plan.includeRoots.erase(
      std::unique(plan.includeRoots.begin(), plan.includeRoots.end()),
      plan.includeRoots.end());
  std::sort(plan.definitions.begin(), plan.definitions.end());
  plan.definitions.erase(
      std::unique(plan.definitions.begin(), plan.definitions.end()),
      plan.definitions.end());
  for (llvm::StringRef path : plan.includeRoots)
    if (!isCanonicalIncludeRoot(path))
      return buildError("ACLOWER-FINGERPRINT",
                        "include root is not a normalized relative path");
  plan.compilerFlags = request.compilerFlags;
  plan.linkerFlags = request.linkerFlags;
  plan.toolchainFingerprint = request.toolchain.fingerprint;
  plan.sourceFingerprint = bundle.sourceFingerprint;
  plan.executablePath = "bin/model";

  plan.prebuiltInputs = request.prebuiltInputs;
  std::erase_if(plan.prebuiltInputs, [&](const PrebuiltInput &input) {
    return input.sourceAvailable &&
           !provenanceMatches(input.provenance, request.toolchain);
  });
  std::sort(plan.prebuiltInputs.begin(), plan.prebuiltInputs.end(),
            [](const PrebuiltInput &left, const PrebuiltInput &right) {
              return left.path < right.path;
            });

  for (const std::string &path : request.linkInputs) {
    if (!isCanonicalIncludeRoot(path))
      return buildError("ACLOWER-FINGERPRINT",
                        "link input path is not canonical");
    auto buffer = llvm::MemoryBuffer::getFile(path);
    if (!buffer)
      return llvm::createStringError(
          buffer.getError(), "cannot read link input '%s'", path.c_str());
    plan.linkInputs.push_back(
        {.path = path,
         .fingerprint = computeFingerprint(buffer.get()->getBuffer())});
  }

  for (const GeneratedFile &file : bundle.files) {
    if (!llvm::StringRef(file.relativePath).ends_with(".cpp"))
      continue;
    plan.sourceUnits.push_back(file.relativePath);
    plan.objectOutputs.push_back(objectPath(file.relativePath));
  }
  for (auto [source, output] :
       llvm::zip_equal(plan.sourceUnits, plan.objectOutputs)) {
    CompileCommand command;
    command.arguments.push_back(request.toolchain.compilerPath);
    command.arguments.insert(command.arguments.end(),
                             request.toolchain.contractFlags.begin(),
                             request.toolchain.contractFlags.end());
    command.arguments.insert(command.arguments.end(),
                             plan.compilerFlags.begin(),
                             plan.compilerFlags.end());
    for (const std::string &definition : plan.definitions)
      command.arguments.push_back("-D" + definition);
    for (const std::string &include : plan.includeRoots)
      command.arguments.push_back("-I" + include);
    command.arguments.insert(command.arguments.end(),
                             {"-c", source, "-o", output});
    command.output = output;
    plan.compileCommands.push_back(std::move(command));
  }

  plan.linkCommand.arguments.push_back(request.toolchain.compilerPath);
  plan.linkCommand.arguments.insert(plan.linkCommand.arguments.end(),
                                    plan.objectOutputs.begin(),
                                    plan.objectOutputs.end());
  for (const PrebuiltInput &input : plan.prebuiltInputs)
    plan.linkCommand.arguments.push_back(input.path);
  plan.linkCommand.arguments.insert(plan.linkCommand.arguments.end(),
                                    request.linkInputs.begin(),
                                    request.linkInputs.end());
  plan.linkCommand.arguments.insert(plan.linkCommand.arguments.end(),
                                    plan.linkerFlags.begin(),
                                    plan.linkerFlags.end());
  plan.linkCommand.arguments.insert(plan.linkCommand.arguments.end(),
                                    {"-o", plan.executablePath});
  plan.linkCommand.output = plan.executablePath;

  llvm::json::Object preimage{{"domain", "acsim-compile-plan-0.1"},
                              {"plan", planJson(plan, "")}};
  auto fingerprint =
      fingerprintCanonicalJson(llvm::json::Value(std::move(preimage)));
  if (!fingerprint)
    return fingerprint.takeError();
  plan.fingerprint = std::move(*fingerprint);
  if (auto error = plan.validate())
    return std::move(error);
  return plan;
}

} // namespace acir::codegen
