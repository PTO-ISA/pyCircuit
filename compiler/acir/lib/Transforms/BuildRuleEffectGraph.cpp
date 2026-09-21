#include "acir/Transforms/Passes.h"

#include "acir/Analysis/RuleEffectGraph.h"

#include "llvm/ADT/SmallString.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/raw_ostream.h"

using namespace mlir;

namespace acir {
namespace {

struct PendingArtifact {
  std::string finalPath;
  std::string temporaryPath;
  std::string backupPath;
  std::string contents;
  bool hadOriginal = false;
  bool published = false;
};

FailureOr<std::string> normalizedPath(ModuleOp model, StringRef path) {
  if (path.empty())
    return std::string();
  llvm::SmallString<256> normalized(path);
  if (std::error_code error = llvm::sys::fs::make_absolute(normalized)) {
    model.emitError() << "cannot normalize rule effect graph artifact path '"
                      << path << "': " << error.message();
    return failure();
  }
  llvm::sys::path::remove_dots(normalized, true);
  return normalized.str().str();
}

LogicalResult prepareArtifact(ModuleOp model, PendingArtifact &artifact) {
  int descriptor = -1;
  llvm::SmallString<256> temporary;
  if (std::error_code error = llvm::sys::fs::createUniqueFile(
          artifact.finalPath + ".tmp-%%%%%%", descriptor, temporary))
    return model.emitError() << "cannot prepare rule effect graph artifact '"
                             << artifact.finalPath << "': " << error.message();
  artifact.temporaryPath = temporary.str().str();
  llvm::raw_fd_ostream output(descriptor, true);
  output << artifact.contents;
  output.flush();
  if (output.has_error()) {
    llvm::sys::fs::remove(artifact.temporaryPath);
    return model.emitError() << "failed to prepare rule effect graph artifact '"
                             << artifact.finalPath << "'";
  }
  return success();
}

void cleanupTemporary(PendingArtifact &artifact) {
  if (!artifact.temporaryPath.empty())
    llvm::sys::fs::remove(artifact.temporaryPath);
}

LogicalResult publishArtifacts(ModuleOp model,
                               MutableArrayRef<PendingArtifact> artifacts) {
  for (PendingArtifact &artifact : artifacts)
    if (failed(prepareArtifact(model, artifact))) {
      for (PendingArtifact &prepared : artifacts)
        cleanupTemporary(prepared);
      return failure();
    }

  for (PendingArtifact &artifact : artifacts) {
    if (!llvm::sys::fs::exists(artifact.finalPath))
      continue;
    int descriptor = -1;
    llvm::SmallString<256> backup;
    if (std::error_code error = llvm::sys::fs::createUniqueFile(
            artifact.finalPath + ".backup-%%%%%%", descriptor, backup)) {
      for (PendingArtifact &prepared : artifacts)
        cleanupTemporary(prepared);
      for (PendingArtifact &rollback : artifacts)
        if (rollback.hadOriginal)
          llvm::sys::fs::rename(rollback.backupPath, rollback.finalPath);
      return model.emitError()
             << "cannot reserve rule effect graph backup for '"
             << artifact.finalPath << "': " << error.message();
    }
    {
      // `llvm::sys::fs::file_t` is a HANDLE on Windows, so close through the
      // stream wrapper instead of the POSIX-shaped closeFile overload.
      llvm::raw_fd_ostream reserve(descriptor, /*shouldClose=*/true);
      reserve.flush();
    }
    llvm::sys::fs::remove(backup);
    artifact.backupPath = backup.str().str();
    if (std::error_code error =
            llvm::sys::fs::rename(artifact.finalPath, artifact.backupPath)) {
      for (PendingArtifact &prepared : artifacts)
        cleanupTemporary(prepared);
      for (PendingArtifact &rollback : artifacts)
        if (rollback.hadOriginal)
          llvm::sys::fs::rename(rollback.backupPath, rollback.finalPath);
      return model.emitError()
             << "cannot preserve prior rule effect graph artifact '"
             << artifact.finalPath << "': " << error.message();
    }
    artifact.hadOriginal = true;
  }

  for (PendingArtifact &artifact : artifacts) {
    if (std::error_code error =
            llvm::sys::fs::rename(artifact.temporaryPath, artifact.finalPath)) {
      for (PendingArtifact &rollback : artifacts) {
        if (rollback.published)
          llvm::sys::fs::remove(rollback.finalPath);
        cleanupTemporary(rollback);
        if (rollback.hadOriginal)
          llvm::sys::fs::rename(rollback.backupPath, rollback.finalPath);
      }
      return model.emitError()
             << "cannot publish rule effect graph artifact '"
             << artifact.finalPath << "': " << error.message();
    }
    artifact.published = true;
    artifact.temporaryPath.clear();
  }
  for (PendingArtifact &artifact : artifacts)
    if (artifact.hadOriginal)
      llvm::sys::fs::remove(artifact.backupPath);
  return success();
}

#define GEN_PASS_DEF_BUILDRULEEFFECTGRAPHPASS
#include "acir/Transforms/Passes.h.inc"

struct BuildRuleEffectGraphPass
    : impl::BuildRuleEffectGraphPassBase<BuildRuleEffectGraphPass> {
  using Base::Base;

  void runOnOperation() override {
    FailureOr<std::string> normalizedJSON =
        normalizedPath(getOperation(), jsonOutput);
    FailureOr<std::string> normalizedDOT =
        normalizedPath(getOperation(), dotOutput);
    if (failed(normalizedJSON) || failed(normalizedDOT)) {
      signalPassFailure();
      return;
    }
    if (!normalizedJSON->empty() && *normalizedJSON == *normalizedDOT) {
      getOperation().emitError(
          "rule effect graph JSON and DOT outputs resolve to the same path");
      signalPassFailure();
      return;
    }

    RuleEffectGraph graph;
    LogicalResult analysis = buildRuleEffectGraph(getOperation(), graph);
    SmallVector<PendingArtifact> artifacts;
    if (!normalizedJSON->empty()) {
      PendingArtifact artifact{*normalizedJSON};
      llvm::raw_string_ostream output(artifact.contents);
      printRuleEffectGraphJSON(graph, output);
      artifacts.push_back(std::move(artifact));
    }
    if (!normalizedDOT->empty()) {
      PendingArtifact artifact{*normalizedDOT};
      llvm::raw_string_ostream output(artifact.contents);
      printRuleEffectGraphDOT(graph, output);
      artifacts.push_back(std::move(artifact));
    }
    if (failed(publishArtifacts(getOperation(), artifacts)) || failed(analysis))
      signalPassFailure();
  }
};

} // namespace

std::unique_ptr<Pass> createBuildRuleEffectGraphPass() {
  return std::make_unique<BuildRuleEffectGraphPass>();
}

} // namespace acir
