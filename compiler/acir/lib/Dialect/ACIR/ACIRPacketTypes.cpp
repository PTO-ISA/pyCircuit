#include "acir/Dialect/ACIR/ACIRTypes.h"

#include "mlir/IR/BuiltinTypes.h"
#include "mlir/IR/Diagnostics.h"

using namespace mlir;

namespace acir::ac {

LogicalResult
StructType::verify(llvm::function_ref<InFlightDiagnostic()> emitError,
                   StringAttr name) {
  if (!name || name.getValue().empty())
    return emitError() << "struct type requires a qualified nominal name";
  return success();
}

LogicalResult
RegType::verify(llvm::function_ref<InFlightDiagnostic()> emitError,
                Type elementType) {
  if (auto integer = dyn_cast<IntegerType>(elementType)) {
    if (integer.isSignless() && integer.getWidth() >= 1 &&
        integer.getWidth() <= 64)
      return success();
  } else if (isa<StructType>(elementType)) {
    return success();
  }
  return emitError()
         << "reg element must be finite signless i1..i64 or a nominal record";
}

} // namespace acir::ac
