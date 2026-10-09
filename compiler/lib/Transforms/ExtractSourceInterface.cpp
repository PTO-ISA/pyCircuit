#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "pycircuit/Dialect/ACIR/SourceUnitValidation.h"
#include "pycircuit/Transforms/Passes.h"

#include "mlir/IR/Builders.h"
#include "mlir/Pass/Pass.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringMap.h"

using namespace mlir;

namespace acir {
namespace {
class ExtractSourceInterfacePass
    : public PassWrapper<ExtractSourceInterfacePass,
                         OperationPass<mlir::ModuleOp>> {
public:
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(ExtractSourceInterfacePass)
  StringRef getArgument() const final {
    return "ac-extract-source-interface";
  }
  StringRef getDescription() const final {
    return "Extract source interfaces from open module SSA dependencies";
  }
  void runOnOperation() final {
    mlir::ModuleOp input = getOperation();
    auto emitError = [&] { return input.emitError(); };
    if (failed(ac::verifySourceBodyStructure(input, emitError))) {
      signalPassFailure();
      return;
    }

    // All analysis resolves the intact definitions. Replacing even one early
    // would turn a same-source callee into an import and change later results.
    auto &analysis = getAnalysis<ac::HardwareAnalysis>();
    llvm::StringMap<ArrayAttr> summaries;
    for (ac::ModuleOp definition : analysis.getDefinitions()) {
      ac::HardwareBindings bindings;
      bindings.owner = definition;
      auto facts = analysis.analyzeModule(definition, bindings);
      if (failed(facts)) {
        signalPassFailure();
        return;
      }
      summaries[definition.getSymName()] = ac::serializeOutputDependencies(
          input.getContext(), facts->dependencies);
    }

    // The detached clone is disposable until both native and source-local
    // verification succeed. The input operation and its contents survive any
    // failed analysis or staging check unchanged.
    OwningOpRef<mlir::ModuleOp> staged(input.clone());
    Builder builder(input.getContext());
    (*staged)->setAttr("ac.unit_kind", builder.getStringAttr("interface"));
    for (ac::ModuleOp definition :
         llvm::make_early_inc_range(staged->getOps<ac::ModuleOp>())) {
      OperationState state(definition.getLoc(),
                           ac::ModuleImportOp::getOperationName());
      NamedAttrList attributes(definition->getAttrs());
      attributes.set("dependency_summary",
                     summaries.lookup(definition.getSymName()));
      state.addAttributes(attributes);
      Operation *declaration = Operation::create(state);
      definition->getBlock()->getOperations().insert(definition->getIterator(),
                                                     declaration);
      definition.erase();
    }
    auto stagedError = [&] { return staged->emitError(); };
    if (failed(ac::verifySourceInterfaceStructure(*staged, stagedError))) {
      signalPassFailure();
      return;
    }

    // No fallible step follows the first mutation. Keep the pass root identity
    // stable, and let the pass manager invalidate caches into the replaced SSA.
    input->setAttrs((*staged)->getAttrs());
    input->getRegion(0).takeBody((*staged)->getRegion(0));
  }
};
} // namespace

std::unique_ptr<mlir::Pass> createExtractSourceInterfacePass() {
  return std::make_unique<ExtractSourceInterfacePass>();
}
} // namespace acir
