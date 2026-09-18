#include "CompilerInternal.h"

#include "acir/Dialect/ACIR/ACIROps.h"
#include "acir/Dialect/ACIR/GraphRegion.h"
#include "acir/InitAllDialects.h"
#include "acir/InitAllPasses.h"
#include "acir/Transforms/Passes.h"

#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/Location.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Parser/Parser.h"
#include "mlir/Pass/PassManager.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/Errc.h"
#include "llvm/Support/raw_ostream.h"

#include <iterator>
#include <mutex>
#include <set>
#include <utility>

namespace acir::compiler {
namespace {

struct DriverState {
  mlir::MLIRContext context{mlir::MLIRContext::Threading::DISABLED};
  mlir::OwningOpRef<mlir::ModuleOp> module;
};

std::string severityName(mlir::DiagnosticSeverity severity) {
  switch (severity) {
  case mlir::DiagnosticSeverity::Error: return "error";
  case mlir::DiagnosticSeverity::Warning: return "warning";
  case mlir::DiagnosticSeverity::Remark: return "remark";
  case mlir::DiagnosticSeverity::Note: return "note";
  }
  return "error";
}

struct DiagnosticSourceFrame { std::string message; SourceLocation source; };

std::vector<DiagnosticSourceFrame>
sourceLocations(mlir::Location location,
                llvm::StringRef message = "source definition") {
  if (auto file = mlir::dyn_cast<mlir::FileLineColLoc>(location))
    return {{message.str(), {file.getFilename().str(), file.getLine(), file.getColumn()}}};
  if (auto named = mlir::dyn_cast<mlir::NameLoc>(location))
    return sourceLocations(named.getChildLoc(), named.getName().getValue());
  if (auto call = mlir::dyn_cast<mlir::CallSiteLoc>(location)) {
    auto result = sourceLocations(call.getCallee(), message);
    auto callers = sourceLocations(call.getCaller(), "inline callsite");
    result.insert(result.end(), std::make_move_iterator(callers.begin()),
                  std::make_move_iterator(callers.end()));
    return result;
  }
  if (auto fused = mlir::dyn_cast<mlir::FusedLoc>(location)) {
    std::vector<DiagnosticSourceFrame> result;
    for (mlir::Location child : fused.getLocations()) {
      auto origins = sourceLocations(child, "alternate source origin");
      result.insert(result.end(), std::make_move_iterator(origins.begin()),
                    std::make_move_iterator(origins.end()));
    }
    return result;
  }
  return {};
}

std::string defaultDiagnosticCode(CompilerStage stage) {
  switch (stage) {
  case CompilerStage::AcirParse: return "ACIR-PARSE-001";
  case CompilerStage::AcirVerify: return "ACIR-VERIFY-001";
  case CompilerStage::AcirNormalize: return "ACIR-NORMALIZE-001";
  case CompilerStage::TopologyClosure: return "ACIR-CLOSURE-001";
  }
  return "ACIR-COMPILER-001";
}

CompilerDiagnostic makeDiagnostic(CompilerStage stage, llvm::StringRef code,
                                  llvm::StringRef message) {
  return {.stage = compilerStageName(stage).str(), .code = code.str(),
          .severity = "error", .message = message.str()};
}

llvm::Error compilerFailure(CompilerStage stage, llvm::StringRef code,
                            llvm::StringRef message) {
  return llvm::make_error<CompilerError>(
      std::vector<CompilerDiagnostic>{makeDiagnostic(stage, code, message)});
}

class DiagnosticCapture {
public:
  explicit DiagnosticCapture(mlir::MLIRContext &context)
      : handler_(&context, [&](mlir::Diagnostic &diagnostic) {
          auto sources = sourceLocations(diagnostic.getLocation());
          CompilerDiagnostic captured{
              .stage = compilerStageName(stage_).str(),
              .code = detail::diagnosticCodeFromMetadata(diagnostic).value_or(
                  defaultDiagnosticCode(stage_)),
              .severity = severityName(diagnostic.getSeverity()),
              .message = diagnostic.str(),
              .source = sources.empty() ? std::nullopt
                                        : std::optional(sources.front().source)};
          for (const auto &frame : llvm::ArrayRef(sources).drop_front())
            captured.related.push_back({.message = frame.message,
                                        .source = frame.source});
          diagnostics_.push_back(std::move(captured));
          return mlir::success();
        }) {}
  void setStage(CompilerStage stage) { stage_ = stage; }
  llvm::Error takeFailure(CompilerStage stage) {
    if (diagnostics_.empty())
      diagnostics_.push_back(makeDiagnostic(stage, defaultDiagnosticCode(stage),
                                            "compiler stage failed"));
    return llvm::make_error<CompilerError>(std::move(diagnostics_));
  }
  std::vector<CompilerDiagnostic> takeDiagnostics() {
    return std::move(diagnostics_);
  }
private:
  CompilerStage stage_ = CompilerStage::AcirParse;
  std::vector<CompilerDiagnostic> diagnostics_;
  mlir::ScopedDiagnosticHandler handler_;
};

std::string printModule(mlir::ModuleOp module) {
  std::string bytes;
  llvm::raw_string_ostream output(bytes);
  module.print(output);
  return bytes;
}

bool requested(const CompilerRequest &request, ArtifactKind kind) {
  return llvm::is_contained(request.emits, kind);
}
bool namesStage(llvm::ArrayRef<std::string> names, CompilerStage stage) {
  return llvm::is_contained(names, compilerStageName(stage));
}
void addArtifact(CompilerResult &result, std::string path, ArtifactKind kind,
                 std::string bytes) {
  result.artifacts.push_back({std::move(path), kind, std::move(bytes)});
}

llvm::Error validateRequest(const CompilerRequest &request,
                            llvm::ArrayRef<CompilerStage> pipeline) {
  if (request.acirBytes.empty())
    return compilerFailure(CompilerStage::AcirParse, "ACIR-PARSE-001",
                           "ACIR input is empty");
  std::set<ArtifactKind> unique;
  for (ArtifactKind kind : request.emits) {
    if (!unique.insert(kind).second)
      return compilerFailure(CompilerStage::AcirParse, "ACIR-EMIT-001",
                             "artifact emission request is duplicated");
    if (kind == ArtifactKind::Acir &&
        (pipeline.empty() || pipeline.back() != CompilerStage::TopologyClosure))
      return compilerFailure(CompilerStage::TopologyClosure, "ACIR-EMIT-001",
                             "verified ACIR requires topology closure");
  }
  return llvm::Error::success();
}

mlir::LogicalResult runPass(DriverState &state,
                            std::unique_ptr<mlir::Pass> pass) {
  mlir::PassManager manager(&state.context);
  manager.addPass(std::move(pass));
  return manager.run(state.module.get());
}

llvm::Error runStage(CompilerStage stage, const CompilerRequest &request,
                     DriverState &state, CompilerResult &result,
                     DiagnosticCapture &capture) {
  capture.setStage(stage);
  switch (stage) {
  case CompilerStage::AcirParse:
    state.module = mlir::parseSourceString<mlir::ModuleOp>(request.acirBytes,
                                                           &state.context);
    return state.module ? llvm::Error::success() : capture.takeFailure(stage);
  case CompilerStage::AcirVerify:
    return mlir::succeeded(runPass(state, createVerifyACIRFilePass()))
               ? llvm::Error::success() : capture.takeFailure(stage);
  case CompilerStage::AcirNormalize: {
    mlir::PassManager manager(&state.context);
    if (request.profile == CompilerProfile::Custom) {
      llvm::raw_null_ostream errors;
      if (mlir::failed(mlir::parsePassPipeline(*request.customPipeline,
                                               manager, errors)))
        return compilerFailure(stage, "ACIR-PIPELINE-001",
                               "custom pipeline is invalid");
    } else {
      addRuleLoweringPipeline(manager);
      manager.addPass(createNormalizeACIRFilePass());
    }
    return mlir::succeeded(manager.run(state.module.get()))
               ? llvm::Error::success() : capture.takeFailure(stage);
  }
  case CompilerStage::TopologyClosure:
    if (mlir::failed(runPass(state, createFreezeTopologyPass())))
      return capture.takeFailure(stage);
    if (requested(request, ArtifactKind::Acir))
      addArtifact(result, "verified.ac.mlir", ArtifactKind::Acir,
                  printModule(*state.module));
    return llvm::Error::success();
  }
  llvm_unreachable("closed CompilerStage is exhaustive");
}

} // namespace

char CompilerError::ID = 0;
CompilerError::CompilerError(std::vector<CompilerDiagnostic> diagnostics)
    : diagnostics_(std::move(diagnostics)) {}
void CompilerError::log(llvm::raw_ostream &output) const {
  output << (diagnostics_.empty() ? "compiler failed without a diagnostic"
                                 : diagnostics_.front().code + ": " +
                                       diagnostics_.front().message);
}
std::error_code CompilerError::convertToErrorCode() const {
  return llvm::inconvertibleErrorCode();
}
llvm::StringRef compilerStageName(CompilerStage stage) {
  switch (stage) {
  case CompilerStage::AcirParse: return "acir-parse";
  case CompilerStage::AcirVerify: return "acir-verify";
  case CompilerStage::AcirNormalize: return "acir-normalize";
  case CompilerStage::TopologyClosure: return "topology-closure";
  }
  llvm_unreachable("closed CompilerStage is exhaustive");
}

std::optional<std::string>
detail::diagnosticCodeFromMetadata(mlir::Diagnostic &diagnostic) {
  for (mlir::DiagnosticArgument &argument : diagnostic.getMetadata()) {
    if (argument.getKind() != mlir::DiagnosticArgument::DiagnosticArgumentKind::Attribute)
      continue;
    auto fields = mlir::dyn_cast<mlir::DictionaryAttr>(argument.getAsAttribute());
    if (fields)
      if (auto code = fields.getAs<mlir::StringAttr>("diagnostic.code");
          code && !code.getValue().empty())
        return code.getValue().str();
  }
  return std::nullopt;
}

llvm::Expected<CompilerResult> runCompiler(const CompilerRequest &request) {
  static std::once_flag passesRegistered;
  std::call_once(passesRegistered, [] { registerAllPasses(); });
  auto pipeline = detail::selectPipeline(request);
  if (!pipeline)
    return compilerFailure(CompilerStage::AcirParse, "ACIR-PIPELINE-001",
                           llvm::toString(pipeline.takeError()));
  if (auto error = validateRequest(request, *pipeline)) return std::move(error);
  mlir::DialectRegistry registry;
  registerAllDialects(registry);
  DriverState state;
  state.context.appendDialectRegistry(registry);
  state.context.loadAllAvailableDialects();
  DiagnosticCapture capture(state.context);
  CompilerResult result;
  for (CompilerStage stage : *pipeline) {
    if (state.module && namesStage(request.dumpBefore, stage))
      addArtifact(result, "dumps/" + compilerStageName(stage).str() + "-before.mlir",
                  ArtifactKind::Report, printModule(*state.module));
    if (auto error = runStage(stage, request, state, result, capture))
      return std::move(error);
    if (request.verifyAfterEach && state.module &&
        mlir::failed(mlir::verify(state.module.get())))
      return capture.takeFailure(stage);
    if (state.module && (request.dumpAfterEach || namesStage(request.dumpAfter, stage)))
      addArtifact(result, "dumps/" + compilerStageName(stage).str() + "-after.mlir",
                  ArtifactKind::Report, printModule(*state.module));
  }
  result.diagnostics = capture.takeDiagnostics();
  return result;
}

} // namespace acir::compiler
