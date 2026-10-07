#include "mlir/IR/BuiltinOps.h"
#include "pycircuit/Dialect/ACIR/ACIROps.h"
namespace acir::ac {
mlir::LogicalResult SystemOp::verify() {
  if (!mlir::isa<mlir::ModuleOp>((*this)->getParentOp()) ||
      getDomain() != "default")
    return emitOpError()
           << "system requires package placement and default domain";
  auto entry = getEntry();
  if (!entry || entry.size() != 3 ||
      !entry.getAs<mlir::FlatSymbolRefAttr>("callee") ||
      !entry.getAs<mlir::ArrayAttr>("parameters") ||
      !entry.getAs<mlir::ArrayAttr>("type_arguments"))
    return emitOpError()
           << "system entry requires callee, parameters and type_arguments";
  for (auto arg : entry.getAs<mlir::ArrayAttr>("parameters"))
    if (!mlir::isa<StaticExprAttr>(arg))
      return emitOpError() << "root integer arguments require StaticExpr";
  for (auto arg : entry.getAs<mlir::ArrayAttr>("type_arguments")) {
    auto type = mlir::dyn_cast<mlir::TypeAttr>(arg);
    if (!type || !mlir::isa<BitsType, StructType>(type.getValue()))
      return emitOpError()
             << "root type arguments require finite hardware payload";
  }
  return mlir::success();
}
} // namespace acir::ac
