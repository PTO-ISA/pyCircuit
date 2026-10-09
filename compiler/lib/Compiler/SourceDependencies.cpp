#include "SourceUnit.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"

using namespace mlir;

namespace acir::compiler {
LogicalResult verifyPublishedDependencies(ac::ModuleOp definition,
                                          ac::ModuleImportOp declaration,
                                          ac::detail::EmitError emitError) {
  auto unit = definition->getParentOfType<mlir::ModuleOp>();
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
