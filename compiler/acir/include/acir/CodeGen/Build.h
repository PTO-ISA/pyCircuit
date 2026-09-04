#ifndef ACIR_CODEGEN_BUILD_H
#define ACIR_CODEGEN_BUILD_H

#include "acir/CodeGen/Generator.h"

#include "mlir/IR/BuiltinOps.h"
#include "llvm/Support/Error.h"

#include <string>
#include <vector>

namespace acir::codegen {

struct ToolchainIdentity {
  std::string compilerPath;
  std::string compilerName;
  std::string compilerBuildId;
  std::string targetTriple;
  std::string standardLibrary;
  std::string abiMode;
  std::string objectFormat;
  std::vector<std::string> contractFlags;
  Fingerprint fingerprint;
};

struct PrebuiltProvenance {
  std::string compilerBuildId;
  std::string targetTriple;
  std::string standardLibrary;
  std::string abiMode;
  std::string objectFormat;
  std::string contractEpoch = "0.5";
  std::vector<std::string> contractFlags;
  Fingerprint toolchainFingerprint;
  Fingerprint sourceFingerprint;
};

struct PrebuiltInput {
  std::string path;
  std::string kind;
  PrebuiltProvenance provenance;
  bool sourceAvailable = false;
};

struct CompileCommand {
  std::vector<std::string> arguments;
  std::string output;
};

struct LinkInputIdentity {
  std::string path;
  Fingerprint fingerprint;
};

struct FrontendProvenance {
  // Empty only for the internal canonical-ACSim driver. Frontend-originated
  // builds populate this record completely and are validated atomically.
  std::vector<FileHash> sourceFiles;
  Artifact acpy;
  std::string acpyBytes;
  Artifact canonicalAcir;
  std::string canonicalAcirBytes;
  std::string pythonVersion;
  std::vector<NamedFingerprint> helperIdentities;
};

struct CompilePlan {
  std::string schema = "acsim-compile-plan-0.1";
  std::vector<std::string> sourceUnits;
  std::vector<std::string> objectOutputs;
  std::vector<std::string> includeRoots;
  std::vector<std::string> definitions;
  std::vector<std::string> compilerFlags;
  std::vector<std::string> linkerFlags;
  std::vector<LinkInputIdentity> linkInputs;
  std::vector<PrebuiltInput> prebuiltInputs;
  std::vector<CompileCommand> compileCommands;
  CompileCommand linkCommand;
  std::string executablePath;
  Fingerprint sourceFingerprint;
  Fingerprint toolchainFingerprint;
  Fingerprint fingerprint;

  llvm::Error validate() const;
  llvm::Expected<std::string> canonicalJson() const;
};

struct BuildRequest {
  Identity project;
  Identity system;
  FrontendProvenance frontend;
  mlir::ModuleOp canonicalACSim;
  std::string frozenAcirBytes;
  std::string canonicalACSimBytes;
  std::string bindingLockBytes;
  std::string profile;
  std::vector<std::string> passPipeline;
  std::vector<std::string> instrumentationLayers;
  std::vector<std::string> providerInputs;
  ToolchainIdentity toolchain;
  std::vector<PrebuiltInput> prebuiltInputs;
  std::vector<std::string> includeRoots;
  std::vector<std::string> definitions;
  std::vector<std::string> compilerFlags;
  std::vector<std::string> linkerFlags;
  std::vector<std::string> linkInputs;
  std::string outputRoot;
};

struct BuildResult {
  std::string buildDirectory;
  std::string executable;
  Fingerprint buildFingerprint;
  bool cacheHit = false;
};

llvm::Expected<ToolchainIdentity>
identifyToolchain(std::string compilerPath, std::string standardLibrary,
                  std::string abiMode, std::string objectFormat,
                  std::vector<std::string> contractFlags);

llvm::Error preflightBuildRequest(const BuildRequest &request);

llvm::Expected<CompilePlan> createCompilePlan(const BuildRequest &request,
                                              const SourceBundle &bundle);

llvm::Expected<BuildResult> buildGeneratedModel(const BuildRequest &request);

} // namespace acir::codegen

#endif // ACIR_CODEGEN_BUILD_H
