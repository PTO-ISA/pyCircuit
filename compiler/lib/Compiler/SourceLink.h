#ifndef ACIR_LIB_COMPILER_SOURCELINK_H
#define ACIR_LIB_COMPILER_SOURCELINK_H

#include "SourceUnit.h"

namespace acir::compiler {

struct SourceLinkUnit {
  mlir::ModuleOp body;
  mlir::ModuleOp header;
};

// Diagnostic snapshots of the same link pipeline, never source authority.
// Updated only after complete link success; failure preserves prior contents.
struct SourceLinkStages {
  std::string beforeTransactions;
  std::string routedTransactions;
  std::string afterTransactions;
};

struct SourceLinkParameterBinding {
  std::string name;
  std::string value; // Canonical signed decimal, never a host-width integer.
};

mlir::FailureOr<SourceHeaderRegistry>
admitSourceLinkUnits(llvm::ArrayRef<SourceLinkUnit> units,
                     ac::detail::EmitError emitError);

// Linked preserves transactions after concrete binding and common analysis.
enum class SourceLinkTarget { Linked, Routed, Hardware };

mlir::FailureOr<mlir::OwningOpRef<mlir::ModuleOp>>
linkSourceUnits(llvm::ArrayRef<SourceLinkUnit> units, llvm::StringRef top,
                  ac::detail::EmitError emitError,
                  SourceLinkTarget target = SourceLinkTarget::Hardware,
                  SourceLinkStages *retainedStages = nullptr,
                  llvm::ArrayRef<SourceLinkParameterBinding> parameters = {},
                  llvm::StringRef resetSemantics = "strict");

// Rebuilds common analysis from the standalone Linked artifact. The input is
// immutable; routing and lowering share the ordinary source-link continuation.
mlir::FailureOr<mlir::OwningOpRef<mlir::ModuleOp>>
continueLinkedSource(mlir::ModuleOp source, ac::detail::EmitError emitError,
                     SourceLinkTarget target = SourceLinkTarget::Hardware,
                     SourceLinkStages *retainedStages = nullptr);

} // namespace acir::compiler

#endif // ACIR_LIB_COMPILER_SOURCELINK_H
