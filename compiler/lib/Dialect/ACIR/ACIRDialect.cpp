#include "pycircuit/Dialect/ACIR/ACIRDialect.h"

#include "pycircuit/Dialect/ACIR/ACIRDialect.cpp.inc"

using namespace mlir;

namespace acir::ac {

LogicalResult ACIRDialect::verifyOperationAttribute(Operation *operation,
                                                    NamedAttribute attribute) {
  (void)operation;
  (void)attribute;
  return success();
}

} // namespace acir::ac
