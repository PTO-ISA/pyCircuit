#include "ACIRHardwareClosure.h"
#include "acir/Dialect/ACIR/ACIROps.h"

namespace acir::ac {
mlir::LogicalResult SystemOp::verify() {
  auto package = mlir::dyn_cast_or_null<mlir::ModuleOp>((*this)->getParentOp());
  if (!package)
    return emitOpError()
           << "system must be directly owned by its final package";
  return verifyFinalHardware(package);
}
} // namespace acir::ac
