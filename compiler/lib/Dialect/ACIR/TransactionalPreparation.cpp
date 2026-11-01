#include "pycircuit/Dialect/ACIR/TransactionalPreparation.h"
#include "pycircuit/Dialect/ACIR/ValueUpdates.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/IRMapping.h"
#include "mlir/IR/SymbolTable.h"
#include "mlir/IR/Verifier.h"
#include "pycircuit/Dialect/ACIR/TransactionalContracts.h"

using namespace mlir;
namespace acir::ac {
namespace {
class HelperInliner {
public:
  HelperInliner(mlir::ModuleOp package, TransactionalDebit debit)
      : package(package), debit(std::move(debit)) {}
  LogicalResult run() {
    // Templates remain unexpanded until every execution site is processed.
    // This avoids multiplying intermediate templates for nested call trees.
    for (Operation &definition : *package.getBody())
      if (!isa<func::FuncOp>(definition))
        for (Region &region : definition.getRegions())
          if (failed(expand(region)))
            return failure();
    for (auto helper :
         llvm::make_early_inc_range(package.getOps<func::FuncOp>()))
      helper.erase();
    return success();
  }

private:
  mlir::ModuleOp package;
  TransactionalDebit debit;
  LogicalResult expand(Region &region) {
    for (Block &block : region)
      for (Operation &operation : llvm::make_early_inc_range(block))
        if (failed(expandOperation(&operation)))
          return failure();
    return success();
  }
  LogicalResult expandOperation(Operation *operation) {
    auto call = dyn_cast<func::CallOp>(operation);
    if (!call) {
      for (Region &nested : operation->getRegions())
        if (failed(expand(nested)))
          return failure();
      return success();
    }
    auto helper = dyn_cast_or_null<func::FuncOp>(
        SymbolTable::lookupSymbolIn(package, call.getCalleeAttr()));
    if (!helper || helper.isExternal())
      return call.emitOpError("pure helper implementation is missing");
    IRMapping mapping;
    mapping.map(helper.getArguments(), call.getOperands());
    OpBuilder builder(call);
    SmallVector<Operation *> copies;
    for (Operation &body : helper.getBody().front().without_terminator()) {
      auto charged = body.walk([&](Operation *nested) {
        return failed(debit(16 + nested->getNumOperands() +
                                nested->getNumResults(),
                            call))
                   ? WalkResult::interrupt()
                   : WalkResult::advance();
      });
      if (charged.wasInterrupted())
        return failure();
      Operation *copy = builder.clone(body, mapping);
      copy->walk([&](Operation *nested) {
        nested->setLoc(CallSiteLoc::get(nested->getLoc(), call.getLoc()));
      });
      copies.push_back(copy);
    }
    auto returned = cast<func::ReturnOp>(helper.getBody().front().back());
    SmallVector<Value> results;
    for (Value value : returned.getOperands())
      results.push_back(mapping.lookupOrDefault(value));
    call->replaceAllUsesWith(results);
    call.erase();
    // Replace the outer call first: nested call replacement then updates all
    // consumers, including the outer result users, through ordinary SSA uses.
    for (Operation *copy : copies)
      if (failed(expandOperation(copy)))
        return failure();
    return success();
  }
};
} // namespace

namespace {
LogicalResult verifyInput(mlir::ModuleOp package, TransactionalDebit debit) {
  if (!debit)
    return package.emitError("pure helper inlining requires a work budget");
  auto scan = package.walk([&](Operation *op) {
    return failed(debit(16 + op->getNumOperands() + op->getNumResults(), op))
               ? WalkResult::interrupt()
               : WalkResult::advance();
  });
  if (scan.wasInterrupted() || failed(verify(package)) ||
      failed(verifyTransactionalPackage(package)))
    return failure();
  if (!isLinkedTransactionalPackage(package))
    return package.emitError("pure helper inlining requires Linked ACIR");
  return success();
}
LogicalResult expandAndVerify(mlir::ModuleOp package,
                              TransactionalDebit debit) {
  if (failed(HelperInliner(package, debit).run()) || failed(verify(package)))
    return failure();
  return verifyTransactionalPackage(package);
}
} // namespace

LogicalResult inlinePureHelpers(mlir::ModuleOp package,
                                TransactionalDebit debit) {
  if (failed(verifyInput(package, debit)))
    return failure();
  return expandAndVerify(package, debit);
}

FailureOr<std::unique_ptr<TransactionalPreparation>>
TransactionalPreparation::create(mlir::ModuleOp source, StringRef top,
                                 TransactionalDebit debit) {
  if (failed(verifyInput(source, debit)))
    return failure();
  auto prepared = std::unique_ptr<TransactionalPreparation>(
      new TransactionalPreparation(source));
  prepared->expanded_ = cast<mlir::ModuleOp>(source->clone());
  if (failed(expandAndVerify(*prepared->expanded_, debit)) ||
      failed(normalizeValueUpdates(*prepared->expanded_)))
    return failure();
  auto analysis =
      TransactionalAnalysis::create(*prepared->expanded_, top, debit);
  if (failed(analysis))
    return failure();
  prepared->analysis_ = std::move(*analysis);
  return std::move(prepared);
}
} // namespace acir::ac
