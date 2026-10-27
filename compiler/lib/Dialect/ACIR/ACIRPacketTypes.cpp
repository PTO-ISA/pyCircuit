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
namespace {
LogicalResult resourcePayload(llvm::function_ref<InFlightDiagnostic()> error,
                              Type payload) {
  if (!isa<BitsType, StructType, EnumType, TableType, TypeParamType>(payload))
    return error() << "resource payload must be hardware data, not a resource or module reference";
  return success();
}
}
LogicalResult PrimitiveType::verify(llvm::function_ref<InFlightDiagnostic()> error, Type payload) {
  return resourcePayload(error, payload);
}
LogicalResult StateType::verify(llvm::function_ref<InFlightDiagnostic()> error, Type payload) {
  return resourcePayload(error, payload);
}
LogicalResult QueueType::verify(llvm::function_ref<InFlightDiagnostic()> error, Type payload) {
  return resourcePayload(error, payload);
}
LogicalResult ModuleRefType::verify(llvm::function_ref<InFlightDiagnostic()> error,
                                   FlatSymbolRefAttr module) {
  if (!module || module.getValue().empty() || module.getValue().contains('\0'))
    return error() << "module reference requires a nonempty module symbol";
  return success();
}
} // namespace acir::ac
