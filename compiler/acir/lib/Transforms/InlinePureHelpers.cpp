#include "acir/Transforms/Passes.h"

#include "Analysis/ModelAnalysisInternal.h"

#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/IRMapping.h"
#include "mlir/IR/SymbolTable.h"
#include "llvm/ADT/SmallVector.h"

using namespace mlir;

namespace acir {
namespace {
#define GEN_PASS_DEF_INLINEPUREHELPERSPASS
#include "acir/Transforms/Passes.h.inc"

struct InlinePureHelpersPass
    : impl::InlinePureHelpersPassBase<InlinePureHelpersPass> {
  void runOnOperation() override {
    ModuleOp module = getOperation();
    if (failed(detail::validatePureQueueCallGraph(module))) {
      signalPassFailure();
      return;
    }
    SymbolTable symbols(module);
    bool changed = true;
    while (changed) {
      changed = false;
      SmallVector<func::CallOp> calls;
      module.walk([&](func::CallOp call) { calls.push_back(call); });
      for (func::CallOp call : calls) {
        auto callee = symbols.lookup<func::FuncOp>(call.getCallee());
        if (!callee || !callee->hasAttrOfType<BoolAttr>("ac.inline") ||
            !callee->getAttrOfType<BoolAttr>("ac.inline").getValue())
          continue;
        if (callee.isExternal() || !callee.getBody().hasOneBlock()) {
          call.emitOpError(
              "ac.inline helper must resolve to one non-external block");
          signalPassFailure();
          return;
        }
        Block &body = callee.getBody().front();
        auto returned = dyn_cast<func::ReturnOp>(body.getTerminator());
        if (!returned || body.getNumArguments() != call.getNumOperands() ||
            returned.getNumOperands() != call.getNumResults()) {
          call.emitOpError("ac.inline helper signature is malformed");
          signalPassFailure();
          return;
        }
        IRMapping mapping;
        for (auto [argument, operand] :
             llvm::zip_equal(body.getArguments(), call.getOperands()))
          mapping.map(argument, operand);
        OpBuilder builder(call);
        for (Operation &operation : body.without_terminator())
          builder.clone(operation, mapping);
        for (auto [result, value] :
             llvm::zip_equal(call.getResults(), returned.getOperands()))
          result.replaceAllUsesWith(mapping.lookupOrDefault(value));
        call.erase();
        changed = true;
      }
    }

    SmallVector<func::FuncOp> dead;
    for (func::FuncOp function : module.getOps<func::FuncOp>())
      if (auto marker = function->getAttrOfType<BoolAttr>("ac.inline");
          marker && marker.getValue() && function->use_empty())
        dead.push_back(function);
    for (func::FuncOp function : dead)
      function.erase();
  }
};
} // namespace

std::unique_ptr<Pass> createInlinePureHelpersPass() {
  return std::make_unique<InlinePureHelpersPass>();
}

} // namespace acir
