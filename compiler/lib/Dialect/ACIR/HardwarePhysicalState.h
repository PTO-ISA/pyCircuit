#ifndef PYCIRCUIT_HARDWARE_PHYSICAL_STATE_H
#define PYCIRCUIT_HARDWARE_PHYSICAL_STATE_H

#include "mlir/IR/Value.h"
#include "mlir/Support/LogicalResult.h"

namespace acir::ac {
class HardwareAnalysis;
struct HardwareBindings;
namespace detail {
struct StateEndpoint;
mlir::FailureOr<mlir::Value>
getPhysicalStateControl(mlir::Operation *storage, unsigned ordinal);
mlir::LogicalResult
verifyPhysicalStateAllocation(mlir::Operation *storage,
                              const HardwareAnalysis &analysis);
mlir::LogicalResult bindPhysicalStateEndpoint(
    StateEndpoint &result, mlir::Operation *storage,
    const HardwareBindings &bound, const HardwareAnalysis &analysis);
} // namespace detail
} // namespace acir::ac
#endif
