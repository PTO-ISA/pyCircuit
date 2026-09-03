#ifndef ACIR_TRANSFORMS_RESOLVEBINDINGS_H
#define ACIR_TRANSFORMS_RESOLVEBINDINGS_H

#include "acir/Bindings/Registry.h"

#include "mlir/IR/BuiltinOps.h"
#include "mlir/Pass/Pass.h"
#include "llvm/Support/Error.h"

#include <memory>
#include <string>
#include <vector>

namespace acir {

struct ResolveBindingsPassOptions {
  std::vector<bindings::BindingCandidate> candidates;
  std::vector<bindings::BindingRequest> requests;
  std::string profile;
  std::string target;
  std::string lockOutputPath;
};

/// Resolves a frozen module without mutating its topology or creating ACSim
/// operations. The returned immutable result is the Task 12 lowering input.
llvm::Expected<bindings::BindingResolutionResult>
resolveModuleBindings(mlir::ModuleOp module,
                      const ResolveBindingsPassOptions &options);

std::unique_ptr<mlir::Pass>
createResolveBindingsPass(ResolveBindingsPassOptions options);

} // namespace acir

#endif // ACIR_TRANSFORMS_RESOLVEBINDINGS_H
