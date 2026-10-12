#include "SourceUnit.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "pycircuit/Dialect/ACIR/SourceMethodAnalysis.h"

using namespace mlir;

namespace acir::compiler {
LogicalResult verifyPublishedDependencies(ac::ModuleOp definition,
                                          ac::ModuleImportOp declaration,
                                          ac::detail::EmitError emitError) {
  auto unit = definition->getParentOfType<mlir::ModuleOp>();
  ac::SourceMethodSummariesAnalysis methods(unit);
  if (!methods.isValid())
    return failure();
  auto transactions = methods.requiresTransactionPublication(definition);
  if (failed(transactions))
    return failure();
  StringRef expected = *transactions ? "transactions" : "hardware";
  if (declaration.getSemanticKind() != expected)
    return emitError()
           << "published semantic kind differs from module SSA: @"
           << definition.getSymName();
  if (*transactions)
    return ac::verifyPublishedTransactionSummary(
        definition, declaration.getTransactionSummaryAttr(), methods);
  ac::HardwareAnalysis analysis(unit);
  ac::HardwareBindings bindings;
  bindings.owner = definition;
  auto actual = analysis.analyzeModule(definition, bindings);
  auto published = analysis.getImportDependencies(declaration, bindings);
  if (failed(actual) || failed(published))
    return failure();
  if (ac::serializeOutputDependencies(unit.getContext(),
                                      actual->dependencies) !=
      ac::serializeOutputDependencies(unit.getContext(), *published))
    return emitError()
           << "published dependency summary differs from module SSA: @"
           << definition.getSymName();
  return success();
}
} // namespace acir::compiler
