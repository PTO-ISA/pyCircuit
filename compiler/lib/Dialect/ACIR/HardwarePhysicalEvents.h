#ifndef PYCIRCUIT_HARDWARE_PHYSICAL_EVENTS_H
#define PYCIRCUIT_HARDWARE_PHYSICAL_EVENTS_H
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
namespace acir::ac::detail {
mlir::FailureOr<HardwarePhysicalProjection> getBatchedPhysicalProjection(
    const HardwareAnalysis &analysis, const HardwareSourceCheckPlan &plan,
    HardwareSourceCheckLimits limits);
}
#endif
