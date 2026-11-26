#include "pycircuit/Interfaces/LogicalStateInterfaces.h"

#include "mlir/IR/BuiltinAttributes.h"
#include "mlir/IR/Operation.h"
#include "mlir/IR/BuiltinOps.h"

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
  auto package = operation->getParentOfType<mlir::ModuleOp>();
  auto contract = package ? package->getAttrOfType<mlir::IntegerAttr>("ac.routed_contract") : mlir::IntegerAttr();
  bool current = contract && contract.getInt() == 7;
  unsigned expected = 3;
  mlir::Value operands[3];
  if (current) {
    auto sizes = operation->getAttrOfType<mlir::DenseI32ArrayAttr>("operandSegmentSizes");
    auto str = [&](llvm::StringRef key) { return operation->getAttrOfType<mlir::StringAttr>(key); };
    auto edge = str("edge"), reset = str("reset_kind"), polarity = str("reset_polarity"), init = str("init_kind"), domain = str("domain");
    bool hasReset = reset && reset.getValue() != "none";
    bool hasInitial = init && init.getValue() == "explicit";
    if (!edge || (edge.getValue() != "posedge" && edge.getValue() != "negedge") ||
        !reset || (reset.getValue() != "sync" && reset.getValue() != "async" && reset.getValue() != "none") ||
        (hasReset ? !polarity || (polarity.getValue() != "high" && polarity.getValue() != "low") : bool(operation->getAttr("reset_polarity"))) ||
        !init || (init.getValue() != "explicit" && init.getValue() != "unspecified") || !domain || domain.getValue().empty() ||
        !sizes || sizes.size() != 4 || sizes[0] != 1 || sizes[1] != int(hasReset) || sizes[2] != int(hasInitial) || sizes[3] != int(hasReset))
      return operation->emitOpError() << "contract 7 state requires explicit policy, domain and exact operand segments";
    expected = 1 + 2 * unsigned(hasReset) + unsigned(hasInitial);
    if (operation->getNumOperands() != expected) return operation->emitOpError() << "state operand segments disagree with operands";
    unsigned cursor = 0;
    for (unsigned i = 0; i < 3; ++i) {
      operands[i] = sizes[i] ? operation->getOperand(cursor) : mlir::Value();
      cursor += sizes[i];
    }
  } else {
    for (llvm::StringRef key : {"operandSegmentSizes", "edge", "reset_kind", "reset_polarity", "init_kind", "domain"})
      if (operation->hasAttr(key)) return operation->emitOpError() << "legacy state forbids explicit policy";
    if (operation->getNumOperands() != expected) return operation->emitOpError() << "legacy state requires three operands";
    for (unsigned i = 0; i < 3; ++i) operands[i] = operation->getOperand(i);
  }
  if (operation->getNumResults() != 1 || operation->getNumRegions() || operation->getNumSuccessors())
    return operation->emitOpError() << "logical state requires one result and no regions or successors";
  if (state.getStateClock() != operands[0] || state.getStateReset() != operands[1] ||
      state.getStateInitial() != operands[2] || state.getStateQ() != operation->getResult(0))
    return operation->emitOpError() << "logical state getters must alias declared operand segments";
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
