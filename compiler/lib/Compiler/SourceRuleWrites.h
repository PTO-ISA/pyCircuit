#ifndef ACIR_LIB_COMPILER_SOURCERULEWRITES_H
#define ACIR_LIB_COMPILER_SOURCERULEWRITES_H

#include "PythonImportAST.h"
#include "PythonImportEnumSyntax.h"
#include "mlir/Pass/Pass.h"
#include "llvm/ADT/StringMap.h"
#include <memory>

namespace acir::compiler::detail {

struct SourceWriteSelector {
  enum class Kind { Field, Index };
  Kind kind;
  std::string field;
  AstNode occurrence;
  ac::MathIntAttr directConstantInteger;
};
struct SourceWritePath {
  llvm::SmallVector<SourceWriteSelector> selectors;
  AstNode assignment;
};
struct SourceOwnerWrite {
  AstNode owner;
  std::string formal;
  llvm::SmallVector<SourceWritePath, 0> paths;
};
struct SourceRuleCallPlan {
  AstNode module, call, rule;
  llvm::SmallVector<SourceOwnerWrite, 0> writes;
};

struct SourcePendingWritePair {
  AstNode module, owner, firstCall, secondCall;
  AstNode firstAssignment, secondAssignment;
};

/// Source intent is retained before SSA lowering, including identity writes.
class SourceRuleWritesAnalysis {
public:
  explicit SourceRuleWritesAnalysis(mlir::Operation *operation);
  bool isPlanValid() const { return valid; }
  llvm::ArrayRef<SourcePendingWritePair> pendingPairs() const {
    return pending;
  }
  uint64_t spentWork() const { return work; }
  const SourceRuleCallPlan *lookup(const AstNode &module,
                                   const AstNode &call) const;
  bool hasDecorator(const AstNode &node, llvm::StringRef name) const;
  bool resolves(const AstNode &node, MarkerKind marker) const;

private:
  mlir::LogicalResult analyze();
  bool charge(uint64_t amount = 1);
  std::optional<ResolvedSourceBinding> binding(const AstNode &node) const;
  CapturedSource source;
  struct ImportBinding {
    std::string module, remote;
    bool nameSpace = false;
  };
  llvm::StringMap<ImportBinding> imports;
  llvm::SmallVector<SourceRuleCallPlan, 0> calls;
  llvm::SmallVector<SourcePendingWritePair, 0> pending;
  uint64_t work = 0;
  mlir::Operation *operation;
  bool valid = false;
};

std::unique_ptr<mlir::Pass> createAnalyzeRuleWritesPass();
void registerSourceCompilerPasses();

} // namespace acir::compiler::detail
#endif
