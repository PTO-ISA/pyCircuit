#ifndef ACIR_COMPILER_DRIVER_H
#define ACIR_COMPILER_DRIVER_H

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/JSON.h"

#include <cstdint>
#include <optional>
#include <string>
#include <system_error>
#include <vector>

namespace mlir { class Diagnostic; }

namespace acir::compiler {

enum class CompilerStage {
  AcirParse,
  AcirVerify,
  AcirNormalize,
  TopologyClosure,
};
enum class CompilerProfile { Fast, Validated, Custom };
enum class ArtifactKind { Acir, Report };

struct SourceLocation { std::string file; uint64_t line = 0; uint64_t column = 0; };
struct CompilerRelated {
  std::string message;
  std::optional<SourceLocation> source;
  std::optional<std::string> objectPath;
};
struct CompilerFixIt { std::string message; };
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
  CompilerProfile profile = CompilerProfile::Fast;
  std::optional<CompilerStage> stopAfter;
  std::vector<ArtifactKind> emits;
  std::vector<std::string> dumpBefore;
  std::vector<std::string> dumpAfter;
  bool dumpAfterEach = false;
  bool verifyAfterEach = false;
  std::optional<std::string> customPipeline;
};
struct CompilerArtifact {
  std::string logicalPath;
  ArtifactKind kind = ArtifactKind::Report;
  std::string bytes;
};
struct CompilerResult {
  std::vector<CompilerArtifact> artifacts;
  std::vector<CompilerDiagnostic> diagnostics;
};

class CompilerError : public llvm::ErrorInfo<CompilerError> {
public:
  static char ID;
  explicit CompilerError(std::vector<CompilerDiagnostic> diagnostics);
  const std::vector<CompilerDiagnostic> &diagnostics() const { return diagnostics_; }
  void log(llvm::raw_ostream &output) const override;
  std::error_code convertToErrorCode() const override;
private:
  std::vector<CompilerDiagnostic> diagnostics_;
};

llvm::StringRef compilerStageName(CompilerStage stage);
namespace detail {
std::optional<std::string> diagnosticCodeFromMetadata(mlir::Diagnostic &diagnostic);
}
llvm::Expected<CompilerResult> runCompiler(const CompilerRequest &request);

} // namespace acir::compiler
#endif
