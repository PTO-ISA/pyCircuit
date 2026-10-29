#include "pycircuit/Dialect/ACIR/ACIROps.h"
#include "pycircuit/Dialect/ACIR/SourceUnitValidation.h"
#include "pycircuit/Dialect/ACIR/TransactionalContracts.h"
#include "pycircuit/Transforms/Passes.h"

#include "mlir/IR/Builders.h"
#include "mlir/Pass/Pass.h"
#include "llvm/ADT/STLExtras.h"

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
    return "Extract authenticated transactional resource interfaces";
  }
  void runOnOperation() final {
    mlir::ModuleOp input = getOperation();
    auto emitError = [&] { return input.emitError(); };
    if (failed(ac::verifySourceBodyStructure(input, emitError))) {
      signalPassFailure();
      return;
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
      attributes.erase("dependency_summary");
      attributes.erase("transaction_summary");
      attributes.set("semantic_kind", builder.getStringAttr("resources"));
      auto methods = ac::getTransactionalMethodInterfaces(definition);
      if (!methods.empty())
        attributes.set("ac.methods", methods);
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
