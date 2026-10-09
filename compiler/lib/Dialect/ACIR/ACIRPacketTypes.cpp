#include "mlir/IR/Diagnostics.h"
#include "pycircuit/Dialect/ACIR/ACIRTypes.h"
using namespace mlir;
namespace acir::ac {
LogicalResult StructType::verify(llvm::function_ref<InFlightDiagnostic()> error,
                                 StringAttr name) {
  if (!name || name.getValue().empty() || name.getValue().contains('\0'))
    return error() << "struct type requires a nonempty nominal name";
  return success();
}
LogicalResult
TypeParamType::verify(llvm::function_ref<InFlightDiagnostic()> error,
                      FlatSymbolRefAttr owner, StringAttr name) {
  if (!owner || !name || name.getValue().empty() ||
      name.getValue().contains('\0'))
    return error() << "type parameter requires owner and nonempty name";
  return success();
}
} // namespace acir::ac
