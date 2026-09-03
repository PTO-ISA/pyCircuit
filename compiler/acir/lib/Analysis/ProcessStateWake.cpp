#include "ProcessStatePlanInternal.h"

#include "acir/Dialect/ACIR/ACIROps.h"

#include "mlir/IR/SymbolTable.h"
#include "llvm/ADT/SmallVector.h"

using namespace mlir;

namespace acir::detail {

// Wake planning is primarily done inline during continuation planning
// (see ProcessStateContinuation.cpp). This file exists as a separate
// compilation unit for the wake-specific validation and declaration
// resolution that can be called after the initial control plan is built.
//
FailureOr<std::unique_ptr<PlanSetBuilder::ControlPlan>>
PlanSetBuilder::planProcessWakes(std::unique_ptr<ControlPlan> control,
                                 const ProcessStateLimits &limits) {
  for (auto &wake : control->wakes) {
    wake->callee = ProcessCalleeId(static_cast<uint32_t>(wake->kind));
    if (auto wait = dyn_cast<ac::WaitUntilOp>(wake->operation)) {
      wake->triggeringValue = wait.getCondition();
      wake->target = wake->operationPath;
      continue;
    }
    mlir::FlatSymbolRefAttr target;
    if (auto wait = dyn_cast<ac::WaitForOp>(wake->operation))
      target = wait.getResourceAttr();
    else if (auto await = dyn_cast<ac::AwaitEventOp>(wake->operation))
      target = await.getEventQueueAttr();
    if (!target)
      continue;
    wake->target = target.getValue().str();
    wake->declaration =
        SymbolTable::lookupNearestSymbolFrom(wake->operation, target);
    if (!wake->declaration) {
      wake->operation->emitOpError("cannot resolve suspension target ")
          << target;
      return failure();
    }
  }
  return control;
}

} // namespace acir::detail
