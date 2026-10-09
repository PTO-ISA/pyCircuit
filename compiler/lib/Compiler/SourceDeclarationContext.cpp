#include "SourceDeclarationContext.h"
namespace acir::compiler::detail {
SourceDeclarationContext::SourceDeclarationContext(
    std::unique_ptr<Implementation> implementation)
    : implementation(std::move(implementation)) {}
SourceDeclarationContext::~SourceDeclarationContext() = default;
mlir::LogicalResult SourceDeclarationContext::prepare() {
  return implementation->prepare();
}
llvm::ArrayRef<MemoryCallPlan> SourceDeclarationContext::memoryCalls() const {
  return implementation->memoryCalls();
}
llvm::ArrayRef<ModuleDomainPlan> SourceDeclarationContext::moduleDomains() const {
  return implementation->moduleDomains();
}
mlir::FailureOr<mlir::OwningOpRef<mlir::ModuleOp>>
SourceDeclarationContext::consume() { return implementation->lower(); }
} // namespace acir::compiler::detail
