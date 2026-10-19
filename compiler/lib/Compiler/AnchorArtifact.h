#ifndef ACIR_LIB_COMPILER_ANCHORARTIFACT_H
#define ACIR_LIB_COMPILER_ANCHORARTIFACT_H

#include "SourceLink.h"
#include <cstdint>

namespace acir::compiler {

// Format-owned transport caps, also enforced by the native direct APIs before
// allocating payload buffers or invoking the MLIR fragment parser.
struct AnchorFormatLimits {
  static constexpr uint64_t manifestBytes = 4 * 1024 * 1024;
  static constexpr uint64_t payloadFiles = 4096;
  static constexpr uint64_t directories = 8192;
  static constexpr uint64_t depth = 32;
  static constexpr uint64_t pathBytes = 1024;
  static constexpr uint64_t fileBytes = 16 * 1024 * 1024;
  static constexpr uint64_t totalBytes = 64 * 1024 * 1024;
};

struct AnchorSourceProvenance {
  SourceLinkUnit unit;
  std::string bodySha256;
  // Exact original interface bytes, retained without reprinting or mutation.
  std::string interfaceText;
};

struct LoadedAnchor {
  mlir::OwningOpRef<mlir::ModuleOp> package;
  std::string contentIdentity;
  std::string configurationIdentity;
  std::string root;
  mlir::DictionaryAttr sourceOwner;
  std::string verifiedPackageSha256;
};

std::string anchorSha256(llvm::StringRef bytes);

// Fills an existing empty private staging directory, never publishes/renames a
// public destination. All source units must have passed normal source admission.
mlir::LogicalResult writeAnchorArtifacts(
    mlir::ModuleOp routed, llvm::ArrayRef<AnchorSourceProvenance> sources,
    llvm::StringRef privateDirectory, ac::detail::EmitError emitError);

// Captures a bounded no-symlink tree, verifies its identities and exact specs,
// lazily assembles the actual semantic closure, then verifies routed semantics
// before deriving configuration. No source, routing or refinement is run.
mlir::FailureOr<LoadedAnchor>
loadVerifiedAnchor(llvm::StringRef directory, mlir::MLIRContext &context,
                   ac::detail::EmitError emitError);

// Revalidates the loaded package, then uses the existing independent refinement
// and proof. The input remains unchanged, including on failure.
mlir::FailureOr<mlir::OwningOpRef<mlir::ModuleOp>>
refineVerifiedAnchor(const LoadedAnchor &anchor,
                     ac::detail::EmitError emitError);

} // namespace acir::compiler
#endif
