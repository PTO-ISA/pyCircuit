#ifndef ACIR_COMPILER_DRIVER_H
#define ACIR_COMPILER_DRIVER_H

#include "acir/CodeGen/Build.h"
#include "acir/CodeGen/Manifest.h"

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/JSON.h"

#include <cstdint>
#include <optional>
#include <string>
#include <system_error>
#include <vector>

namespace acir::compiler {

enum class CompilerStage {
  AcirParse,
  AcirVerify,
  AcirNormalize,
  AcirFreeze,
  AcsimLower,
  AcsimVerify,
  CxxEmit,
  CxxContract,
  Compile,
  Link,
  Publish,
};

enum class CompilerProfile { Fast, Validated, Custom };

struct SourceLocation {
  std::string file;
  uint64_t line = 0;
  uint64_t column = 0;
};

struct CompilerRelated {
  std::string message;
  std::optional<SourceLocation> source;
  std::optional<std::string> objectPath;
};

struct CompilerFixIt {
  std::string message;
};

struct CompilerDiagnostic {
  std::string stage;
  std::string code;
  std::string severity;
  std::string message;
  std::optional<SourceLocation> source;
  std::optional<std::string> objectPath;
  llvm::json::Value expected = nullptr;
  llvm::json::Value actual = nullptr;
  std::vector<CompilerRelated> related;
  std::vector<CompilerFixIt> fixits;
};

struct CompilerRequest {
  std::string acirBytes;
  std::string bindingLockBytes;
  std::string bindingRegistryBytes;
  CompilerProfile profile = CompilerProfile::Fast;
  std::optional<CompilerStage> stopAfter;
  std::vector<codegen::ArtifactKind> emits;
  std::vector<std::string> dumpBefore;
  std::vector<std::string> dumpAfter;
  bool dumpAfterEach = false;
  bool verifyAfterEach = false;
  std::optional<std::string> customPipeline;
  codegen::BuildRequest build;
};

struct CompilerArtifact {
  std::string logicalPath;
  codegen::ArtifactKind kind = codegen::ArtifactKind::Report;
  std::string bytes;
  codegen::Fingerprint sha256;
};

struct CompilerResult {
  std::vector<CompilerArtifact> artifacts;
  std::vector<CompilerDiagnostic> diagnostics;
  std::optional<codegen::BuildResult> build;
};

class CompilerError : public llvm::ErrorInfo<CompilerError> {
public:
  static char ID;

  explicit CompilerError(std::vector<CompilerDiagnostic> diagnostics);

  const std::vector<CompilerDiagnostic> &diagnostics() const {
    return diagnostics_;
  }

  void log(llvm::raw_ostream &output) const override;
  std::error_code convertToErrorCode() const override;

private:
  std::vector<CompilerDiagnostic> diagnostics_;
};

llvm::StringRef compilerStageName(CompilerStage stage);
llvm::Expected<CompilerResult> runCompiler(const CompilerRequest &request);

} // namespace acir::compiler

#endif // ACIR_COMPILER_DRIVER_H
