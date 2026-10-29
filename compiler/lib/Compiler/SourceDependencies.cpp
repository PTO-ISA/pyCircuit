#include "SourceUnit.h"

using namespace mlir;

namespace acir::compiler {
LogicalResult verifyPublishedDependencies(ac::ModuleOp definition,
                                          ac::ModuleImportOp declaration,
                                          ac::detail::EmitError emitError) {
  // Transactional interfaces publish resource signatures. Effects and admission
  // are resolved from the authenticated body after explicit link closure.
  if (declaration.getSemanticKind() != "resources" ||
      declaration.getTransactionSummaryAttr() ||
      declaration.getDependencySummaryAttr())
    return emitError() << "published interface requires a resource signature: @"
                       << definition.getSymName();
  return success();
}
} // namespace acir::compiler
