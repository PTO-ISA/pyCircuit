#ifndef ACIR_LIB_COMPILER_SOURCEDECLARATIONCONTEXT_H
#define ACIR_LIB_COMPILER_SOURCEDECLARATIONCONTEXT_H
#include "SourceMemoryBindings.h"
#include <functional>
namespace acir::compiler::detail {
/// The existing importer owns preparation behind this narrow interface. No
/// hardware body is emitted until consume() is called by source lowering.
class SourceDeclarationContext {
public:
  class Implementation {
  public:
    virtual ~Implementation() = default;
    virtual mlir::LogicalResult prepare() = 0;
    virtual llvm::ArrayRef<MemoryCallPlan> memoryCalls() const = 0;
    virtual llvm::ArrayRef<ModuleDomainPlan> moduleDomains() const = 0;
    virtual mlir::FailureOr<mlir::OwningOpRef<mlir::ModuleOp>> lower() = 0;
  };
  explicit SourceDeclarationContext(std::unique_ptr<Implementation> implementation);
  ~SourceDeclarationContext();
  mlir::LogicalResult prepare();
  llvm::ArrayRef<MemoryCallPlan> memoryCalls() const;
  llvm::ArrayRef<ModuleDomainPlan> moduleDomains() const;
  mlir::FailureOr<mlir::OwningOpRef<mlir::ModuleOp>> consume();
private:
  std::unique_ptr<Implementation> implementation;
};
std::unique_ptr<SourceDeclarationContext> createSourceDeclarationContext(
    const CapturedSource &source, const SourceCompilationInputs &inputs,
    const SourceRuleWritesAnalysis &writes, mlir::Operation *transport);
} // namespace acir::compiler::detail
#endif
