#include "pycircuit/Interfaces/LogicalStateInterfaces.h"

#include "mlir/IR/BuiltinAttributes.h"
#include "mlir/IR/Operation.h"

#define GET_OP_INTERFACE_DEFINITIONS
namespace acir {
#include "pycircuit/Interfaces/LogicalStateInterfaces.cpp.inc"
} // namespace acir

namespace acir {

mlir::LogicalResult verifyLogicalStateStructure(mlir::Operation *operation) {
  auto state = llvm::dyn_cast<LogicalStateOpInterface>(operation);
  if (!state)
    return operation->emitOpError()
           << "is not a routed logical state operation";
  if (operation->getNumOperands() != 3 || operation->getNumResults() != 1 ||
      operation->getNumRegions() || operation->getNumSuccessors())
    return operation->emitOpError()
           << "logical state requires three operands, one result and no "
              "regions or successors";
  if (state.getStateClock() != operation->getOperand(0) ||
      state.getStateReset() != operation->getOperand(1) ||
      state.getStateInitial() != operation->getOperand(2) ||
      state.getStateQ() != operation->getResult(0))
    return operation->emitOpError()
           << "logical state getters must alias the fixed operand and "
              "result slots";
  if (state.getStateInstanceName() !=
          operation->getAttrOfType<mlir::StringAttr>("instance_name") ||
      state.getStateOccurrence() !=
          operation->getAttrOfType<mlir::DictionaryAttr>("occurrence"))
    return operation->emitOpError()
           << "logical state name and occurrence getters must alias their "
              "attributes";
  return mlir::success();
}

mlir::LogicalResult
verifyLogicalStateUpdateStructure(mlir::Operation *operation) {
  auto update = llvm::dyn_cast<LogicalStateUpdateOpInterface>(operation);
  if (!update)
    return operation->emitOpError()
           << "is not a routed logical state update operation";
  if (operation->getNumOperands() != 3 || operation->getNumResults() ||
      operation->getNumRegions() || operation->getNumSuccessors())
    return operation->emitOpError()
           << "logical state update requires three operands and no results "
              "or regions";
  if (update.getUpdatedStateQ() != operation->getOperand(0) ||
      update.getStateCandidate() != operation->getOperand(1) ||
      update.getStateWriteIntent() != operation->getOperand(2))
    return operation->emitOpError()
           << "logical state update getters must alias the fixed operand "
              "slots";
  return mlir::success();
}

} // namespace acir
