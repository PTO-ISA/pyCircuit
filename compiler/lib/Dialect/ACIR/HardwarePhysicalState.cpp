#include "HardwarePhysicalState.h"
#include "HardwareSampledAdmission.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include <utility>

using namespace mlir;
namespace acir::ac::detail {
// A Collection has scalar DFFE elements and replicated control columns.
// Authenticate that replication before recovering the shared sampled control.
FailureOr<Value> getPhysicalStateControl(Operation *storage, unsigned ordinal) {
  Value value = storage->getOperand(ordinal);
  auto collection = dyn_cast<CollectionOp>(storage);
  if (!collection) return value;
  auto splat = value.getDefiningOp<TableSplatOp>();
  if (!splat || splat.getShape() != collection.getShape() ||
      !isa<BitsType>(splat.getInput().getType()) || !areEquivalentHardwareTypes(
          value.getType(), TableType::get(storage->getContext(), collection.getShape(),
                                          splat.getInput().getType())))
    return storage->emitOpError()
           << "transaction Collection requires replicated scalar controls";
  return splat.getInput();
}
LogicalResult verifyPhysicalStateAllocation(
    Operation *storage, const HardwareAnalysis &analysis) {
  StringRef kind =
      analysis.getPrimitiveKind(
          isa<InstanceOp>(storage)
              ? analysis.resolveCallee(cast<InstanceOp>(storage))
              : analysis.resolveCallee(cast<CollectionOp>(storage)));
  bool dff = kind == "dff";
  if ((!dff && kind != "dffe") ||
      storage->getNumOperands() != (dff ? 4u : 5u) ||
      storage->getNumResults() != 1)
    return storage->emitOpError()
           << "transaction inventory requires a direct verified DFF/DFFE";
  unsigned controls = dff ? 2 : 3;
  for (unsigned ordinal = 0; ordinal < controls; ++ordinal)
    if (failed(getPhysicalStateControl(storage, ordinal))) return failure();
  return success();
}
LogicalResult bindPhysicalStateEndpoint(
    StateEndpoint &result, Operation *storage, const HardwareBindings &bound,
    const HardwareAnalysis &analysis) {
  auto leaf = isa<InstanceOp>(storage)
                  ? analysis.bindInstance(cast<InstanceOp>(storage), bound)
                  : analysis.bindInstance(cast<CollectionOp>(storage), bound);
  if (failed(leaf)) return failure();
  result.storageBindings = std::move(*leaf);
  auto clock = getPhysicalStateControl(storage, 0);
  auto reset = getPhysicalStateControl(storage, 1);
  if (failed(clock) || failed(reset)) return failure();
  result.clock = *clock;
  result.reset = *reset;
  bool dff = analysis.getPrimitiveKind(result.storageBindings.owner) == "dff";
  result.form = dff ? StateEndpointForm::DFF
                   : isa<CollectionOp>(storage) ? StateEndpointForm::DFFECollection
                                                : StateEndpointForm::DFFE;
  if (dff) {
    result.data = storage->getOperand(2);
    result.initial = storage->getOperand(3);
  } else {
    auto enable = getPhysicalStateControl(storage, 2);
    if (failed(enable)) return failure();
    result.enable = *enable;
    result.data = storage->getOperand(3);
    result.initial = storage->getOperand(4);
  }
  return success();
}
} // namespace acir::ac::detail
