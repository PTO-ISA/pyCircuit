#include "pycircuit/Dialect/HWIR/HWIRDialect.h"

#include "pycircuit/Dialect/ACIR/ACIRDialect.h"

#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/DialectImplementation.h"

#include "pycircuit/Dialect/HWIR/HWIRDialect.cpp.inc"

#define GET_OP_CLASSES
#include "pycircuit/Dialect/HWIR/HWIROps.cpp.inc"

using namespace mlir;

namespace acir::hwir {

void HWIRDialect::initialize() {
  addOperations<
#define GET_OP_LIST
#include "pycircuit/Dialect/HWIR/HWIROps.cpp.inc"
      >();
}

LogicalResult
HWIRDialect::verifyOperationAttribute(Operation *operation,
                                      NamedAttribute attribute) {
  if (attribute.getName() != "hwir.contract")
    return success();
  if (!isa<mlir::ModuleOp>(operation))
    return operation->emitOpError()
           << "hwir.contract belongs only to the top-level package";
  auto version = dyn_cast<IntegerAttr>(attribute.getValue());
  if (!version || !version.getType().isSignlessInteger(64))
    return operation->emitOpError()
           << "hwir.contract requires an i64 version";
  if (version.getValue() != 1 && version.getValue() != 2)
    return operation->emitOpError()
           << "unsupported hwir.contract version " << version.getInt();
  return success();
}

} // namespace acir::hwir
