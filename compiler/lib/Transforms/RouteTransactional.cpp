#include "Dialect/ACIR/ACIRSourceContracts.h"
#include "Dialect/ACIR/HardwareSourceCheckPreflight.h"
#include "HardwareStateLowering.h"
#include "pycircuit/Dialect/ACIR/SourceUnitValidation.h"
#include "pycircuit/Dialect/ACIR/TransactionalRouting.h"
#include "pycircuit/Transforms/Passes.h"

#include "mlir/IR/Builders.h"
#include "mlir/Pass/Pass.h"
#include "llvm/ADT/STLExtras.h"

using namespace mlir;

namespace acir {
namespace {
using ac::detail::SourceCheckBudget;
using detail::prepayClone;

LogicalResult mergedEnvelope(mlir::ModuleOp package) {
  if (package->hasAttr("ac.stage") || package->hasAttr("ac.routed_contract"))
    return package.emitError()
           << "transaction lowering requires a merged package without ac.stage";
  for (StringRef name : {"ac.source_owner", "ac.unit_kind", "ac.interfaces",
                         "ac.exports", "ac.import_bindings"})
    if (package->hasAttr(name))
      return package.emitError()
             << "transaction lowering cannot consume a single-source envelope: "
             << name;
  if (!package->getAttrOfType<ArrayAttr>("ac.source_units"))
    return package.emitError()
           << "transaction lowering requires merged ac.source_units";
  if (failed(ac::collectFinalSourceUnits(package,
                                         [&] { return package.emitError(); })))
    return failure();
  unsigned systems = 0;
  for (Operation &op : package.getBody()->getOperations()) {
    if (!isa<ac::ModuleOp, ac::ModuleImportOp, ac::StructOp, ac::EnumOp,
             ac::SystemOp>(op))
      return op.emitOpError() << "transaction lowering requires closed merged "
                                 "hardware declarations";
    systems += isa<ac::SystemOp>(op);
    if (auto imported = dyn_cast<ac::ModuleImportOp>(op))
      if (imported.getSemanticKind() != "hardware" ||
          (!imported->getAttrOfType<StringAttr>("primitive_kind") &&
           !imported->hasAttr("ac.external_contract")))
        return imported.emitOpError()
               << "transaction lowering requires resolved provider definitions";
  }
  if (systems != 1)
    return package.emitError()
           << "transaction lowering requires exactly one merged ac.system";
  return success();
}

void acceptTransactionCandidate(mlir::ModuleOp input, mlir::ModuleOp staged) {
  // Check the complete candidate before the first mutation of the input.
  input->setAttrs(staged->getAttrs());
  input->getRegion(0).takeBody(staged->getRegion(0));
}

class RouteTransactionsPass
    : public PassWrapper<RouteTransactionsPass, OperationPass<mlir::ModuleOp>> {
public:
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(RouteTransactionsPass)
  RouteTransactionsPass() = default;
  RouteTransactionsPass(const RouteTransactionsPass &other)
      : PassWrapper(other) {}
  Option<std::string> top{*this, "top",
                          llvm::cl::desc("Closed transactional root Module"),
                          llvm::cl::init("")};
  StringRef getArgument() const final { return "ac-route-transactions"; }
  StringRef getDescription() const final {
    return "Route sampled transactions to independently verified logical-state "
           "ACIR";
  }
  void runOnOperation() final {
    auto input = getOperation();
    if (input->hasAttr("ac.transactional")) {
      ac::HardwareSourceCheckLimits limits;
      limits.maxWorkUnits = 1024 * 1024;
      SourceCheckBudget budget(limits);
      auto candidate = ac::routeTransactionalPackage(
          input, top, [&](uint64_t units, Operation *site) {
            return budget.debit(units, site);
          });
      if (failed(candidate) ||
          failed(ac::verifyRoutedPackage(**candidate, limits))) {
        signalPassFailure();
        return;
      }
      acceptTransactionCandidate(input, **candidate);
      return;
    }
    if (auto stage = input->getAttrOfType<StringAttr>("ac.stage");
        stage && stage.getValue() == "routed") {
      if (failed(ac::verifyRoutedPackage(input)))
        signalPassFailure();
      return;
    }
    if (!ac::detail::findUnloweredTransaction(input)) {
      // Already-normal hardware has no source transaction or logical state to
      // authenticate or normalize. Preserve ordinary graph admission and its
      // budget instead of imposing source transaction analysis. The exact clone
      // changes only the versioned envelope; routed verification also admits a
      // complete ordinary external declaration without executable hardware.
      ac::HardwareSourceCheckLimits graphLimits;
      SourceCheckBudget graphBudget(graphLimits);
      if (failed(mergedEnvelope(input)) ||
          failed(prepayClone(input, graphBudget))) {
        signalPassFailure();
        return;
      }
      OwningOpRef<mlir::ModuleOp> staged = cast<mlir::ModuleOp>(input->clone());
      Builder attributes(input.getContext());
      (*staged)->setAttr("ac.stage", attributes.getStringAttr("routed"));
      (*staged)->setAttr("ac.routed_contract",
                         attributes.getI64IntegerAttr(
                             llvm::any_of(staged->getOps<ac::SystemOp>(),
                                          [](ac::SystemOp system) {
                                            return system->hasAttr(
                                                "ac.entry_boundary");
                                          })
                                 ? 3
                                 : 2));
      if (failed(ac::verifyRoutedPackage(*staged, graphLimits))) {
        signalPassFailure();
        return;
      }
      acceptTransactionCandidate(input, *staged);
      return;
    }
    input.emitError()
        << "legacy source transactions are no longer executable; regenerate "
           "from Python source with transactional version 2 and source "
           "contract 5";
    signalPassFailure();
  }
};

} // namespace

std::unique_ptr<Pass> createRouteTransactionsPass() {
  return std::make_unique<RouteTransactionsPass>();
}
} // namespace acir
