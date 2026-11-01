#ifndef PYCIRCUIT_DIALECT_ACIR_VALUEUPDATES_H
#define PYCIRCUIT_DIALECT_ACIR_VALUEUPDATES_H
#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinOps.h"
#include "pycircuit/Dialect/ACIR/ACIROps.h"
namespace acir::ac {
// Absolute intervals are used only for conflict checks. Reconstruction follows
// nominal fields and a leaf-relative Bits interval, preserving four-state data.
struct ValueUpdateTarget {
  mlir::ArrayAttr fields;
  mlir::Type leafType, targetType;
  uint64_t low = 0, high = 0, sliceLow = 0, sliceHigh = 0;
  bool sliced = false;
};
mlir::FailureOr<ValueUpdateTarget>
resolveValueUpdateTarget(mlir::Type rootType, mlir::ArrayAttr fields,
                         StaticExprAttr low, StaticExprAttr high,
                         mlir::Operation *site);
mlir::FailureOr<mlir::Value> replaceValue(mlir::OpBuilder &builder,
                                          mlir::Location location,
                                          mlir::Value base,
                                          mlir::Value replacement,
                                          const ValueUpdateTarget &target);
mlir::LogicalResult normalizeValueUpdates(mlir::ModuleOp package);
} // namespace acir::ac
#endif
