#include "acir/Dialect/ACIR/ACIRTypes.h"

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

} // namespace acir::ac
