#ifndef ACIR_CODEGEN_BUILDINTERNAL_H
#define ACIR_CODEGEN_BUILDINTERNAL_H

#include "acir/CodeGen/Build.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/FunctionExtras.h"
#include "llvm/ADT/StringRef.h"

namespace acir::codegen {

enum class BuildFailurePoint {
  None,
  AfterInputValidation,
  AfterSourceWrite,
  AfterContractCheck,
  AfterCompile,
  AfterLink,
  AfterFingerprintQuery,
  AfterManifestWrite,
  AfterImmutableRename,
  BeforeCurrentRename,
};

struct BuildServices {
  BuildFailurePoint failurePoint = BuildFailurePoint::None;
  llvm::unique_function<llvm::Error(llvm::ArrayRef<llvm::StringRef>)> execute;
};

struct PublishedStage {
  std::string path;
  bool cacheHit = false;
};

llvm::Expected<std::string> normalizeArtifactPath(llvm::StringRef path);
llvm::Error writeFileExclusive(llvm::StringRef stageRoot,
                               llvm::StringRef relativePath,
                               llvm::StringRef bytes);
llvm::Expected<std::string> readFileBytes(llvm::StringRef path);
llvm::Expected<PublishedStage>
publishImmutableStage(llvm::StringRef stageRoot, llvm::StringRef outputRoot,
                      llvm::StringRef buildFingerprint,
                      llvm::ArrayRef<Artifact> artifacts,
                      llvm::StringRef manifestBytes);
llvm::Error writeCurrentPointer(llvm::StringRef outputRoot,
                                llvm::StringRef buildFingerprint);

BuildServices makeRealBuildServices();
llvm::Expected<BuildResult>
buildGeneratedModelForTesting(const BuildRequest &request,
                              BuildServices &services);

} // namespace acir::codegen

#endif // ACIR_CODEGEN_BUILDINTERNAL_H
