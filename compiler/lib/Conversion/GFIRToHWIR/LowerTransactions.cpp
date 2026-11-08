// GFIR→HWIR refinement: verified routed logical state becomes physical
// storage (DFF/DFFE primitive instances) with an exact correspondence proof.
// This is the conversion implementation owned by the GFIRToHWIR target; the
// ACIR→GFIR source-authentication counterpart lives in Conversion/ACIRToGFIR.
#include "pycircuit/Conversion/GFIRToHWIR.h"

// Conversion-common helpers shared with the ACIRToGFIR routing stage.
#include "../Common/ConversionCommon.h"
#include "Dialect/ACIR/ACIRSourceContracts.h"
#include "Dialect/ACIR/SourceCheckBudget.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "pycircuit/Analysis/LogicalStateValidation.h"
#include "pycircuit/Dialect/ACIR/ACIRDialect.h"
#include "pycircuit/Dialect/ACIR/GFIRPackageValidation.h"
#include "pycircuit/Dialect/ACIR/HWIRPackageValidation.h"
#include "pycircuit/Dialect/GFIR/GFIRDialect.h"
#include "pycircuit/Dialect/GFIR/GFIROps.h"
#include "pycircuit/Interfaces/LogicalStateInterfaces.h"

#include "mlir/IR/Builders.h"
#include "mlir/IR/IRMapping.h"
#include "mlir/Pass/Pass.h"
#include "llvm/ADT/STLExtras.h"

using namespace mlir;

namespace acir {
namespace {

using ac::detail::SourceCheckBudget;
// Conversion-common helpers shared with the ACIRToGFIR routing stage;
// refinement call sites keep their original spelling.
using detail::acceptTransactionCandidate;
using detail::charge;
using detail::knownTrue;
using detail::prepayClone;

#include "TransactionHardwareState.inc"

} // namespace

namespace detail {
FailureOr<OwningOpRef<mlir::ModuleOp>> materializeHardwareState(
    mlir::ModuleOp routed, SourceCheckBudget &budget) {
  budget.setPhase("routed hardware state preparation");
  if (failed(ac::verifyRoutedPackage(routed)) ||
      failed(prepayClone(routed, budget)))
    return failure();
  auto primitives = hardwareStatePrimitives(routed, budget);
  if (failed(primitives)) return failure();
  // Pay every inferred allocation and all three Table control splats before
  // cloning or constructing physical state. Width/layout is already verified.
  for (auto module : routed.getOps<ac::ModuleOp>())
    for (auto state : module.getBody().front().getOps<gfir::StateOp>())
      if (!charge(budget, state, 128)) return failure();
  OwningOpRef<mlir::ModuleOp> staged = cast<mlir::ModuleOp>(routed->clone());
  ac::HardwareAnalysis analysis(*staged);
  Builder attributes(routed.getContext());
  for (auto module : staged->getOps<ac::ModuleOp>())
    for (auto state : llvm::make_early_inc_range(
             module.getBody().front().getOps<gfir::StateOp>())) {
      auto update = logicalUpdate(state, budget);
      if (failed(update)) return failure();
      bool dff = unconditionalScalarState(state, *update, analysis, budget);
      auto primitive = dff ? primitives->dff : primitives->dffe;
      if (!primitive)
        return state.emitOpError()
               << "unconditional hardware state requires the verified DFF primitive";
      auto table = dyn_cast<ac::TableType>(state.getQ().getType());
      OperationState leaf(state.getLoc(), table ? ac::CollectionOp::getOperationName()
                                                : ac::InstanceOp::getOperationName());
      leaf.addTypes(state.getQ().getType());
      leaf.addAttribute("instance_name", state.getInstanceNameAttr());
      leaf.addAttribute("occurrence", state.getOccurrence());
      leaf.addAttribute("callee", FlatSymbolRefAttr::get(primitive.getSymNameAttr()));
      leaf.addAttribute("parameters", attributes.getArrayAttr({}));
      leaf.addAttribute("type_arguments", attributes.getArrayAttr({TypeAttr::get(
          table ? table.getElementType() : state.getQ().getType())}));
      if (table) leaf.addAttribute("shape", table.getShape());
      OpBuilder builder(state);
      // Establish Q first so a hold candidate or forward dependency is rebound
      // through ordinary SSA use replacement, without dangling copied Values.
      auto *storage = builder.create(leaf);
      state.getQ().replaceAllUsesWith(storage->getResult(0));
      SmallVector<Value> inputs{state.getClk(), state.getRst()};
      if (!dff) inputs.push_back(update->getIntent());
      inputs.push_back(update->getCandidate());
      inputs.push_back(state.getInitial());
      if (table)
        for (unsigned i = 0; i < 3; ++i) {
          OperationState splat(state.getLoc(), ac::TableSplatOp::getOperationName());
          splat.addOperands(inputs[i]);
          splat.addTypes(ac::TableType::get(state.getContext(), table.getShape(),
                                            inputs[i].getType()));
          splat.addAttribute("shape", table.getShape());
          OpBuilder before(storage);
          inputs[i] = before.create(splat)->getResult(0);
        }
      storage->setOperands(inputs);
      update->erase();
      state.erase();
    }
  staged->getOperation()->setAttr("ac.stage", attributes.getStringAttr("final"));
  return std::move(staged);
}

LogicalResult verifyHardwareStateMaterialization(
    mlir::ModuleOp routed, mlir::ModuleOp candidate, SourceCheckBudget &budget) {
  budget.setPhase("routed hardware state correspondence proof");
  if (!candidate || failed(ac::verifyRoutedPackage(routed))) return failure();
  auto primitives = hardwareStatePrimitives(routed, budget);
  if (failed(primitives) ||
      failed(HardwareStateProof(routed, candidate, budget, *primitives).run()))
    return failure();
  return ac::verifyHardwarePackage(candidate);
}
} // namespace detail

namespace {

class LowerTransactionsPass
    : public PassWrapper<LowerTransactionsPass, OperationPass<mlir::ModuleOp>> {
public:
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(LowerTransactionsPass)
  void getDependentDialects(DialectRegistry &registry) const override {
    // Refinement consumes the routed GFIR state classes concretely.
    registry.insert<ac::ACIRDialect, acir::gfir::GFIRDialect>();
  }
  StringRef getArgument() const final { return "ac-lower-transactions"; }
  StringRef getDescription() const final {
    return "Refine verified routed logical state to final hardware storage";
  }
  void runOnOperation() final {
    auto input = getOperation();
    auto stage = input->getAttrOfType<StringAttr>("ac.stage");
    if (stage && stage.getValue() == "final") {
      if (failed(ac::verifyHardwarePackage(input))) signalPassFailure();
      return;
    }
    if (!stage || stage.getValue() != "routed") {
      input.emitError() << "hardware state lowering requires routed ACIR; run ac-route-transactions first";
      signalPassFailure();
      return;
    }
    ac::HardwareSourceCheckLimits limits;
    // In valid routed IR the only unlowered operations are logical state and
    // its update. State-free graphs retain ordinary hardware limits through
    // clone, exact correspondence proof and final verification in this phase,
    // just as they do through the routing envelope transition above.
    if (ac::detail::findUnloweredTransaction(input))
      limits.maxWorkUnits = 1024 * 1024;
    SourceCheckBudget budget(limits);
    auto candidate = detail::materializeHardwareState(input, budget);
    if (failed(candidate) || failed(detail::verifyHardwareStateMaterialization(
                                 input, **candidate, budget))) {
      signalPassFailure();
      return;
    }
    acceptTransactionCandidate(input, **candidate);
  }
};

} // namespace

std::unique_ptr<Pass> createLowerTransactionsPass() {
  return std::make_unique<LowerTransactionsPass>();
}

} // namespace acir
