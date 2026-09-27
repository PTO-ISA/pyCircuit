#include "acir/Dialect/ACIR/ACIRDialect.h"

#include "mlir/IR/Builders.h"
#include "mlir/IR/DialectImplementation.h"
#include "llvm/ADT/TypeSwitch.h"

#include "acir/Dialect/ACIR/ACIREnums.cpp.inc"

#define GET_ATTRDEF_CLASSES
#include "acir/Dialect/ACIR/ACIRAttributes.cpp.inc"

#define GET_TYPEDEF_CLASSES
#include "acir/Dialect/ACIR/ACIRTypes.cpp.inc"

#define GET_OP_CLASSES
#include "acir/Dialect/ACIR/ACIROps.cpp.inc"

void acir::ac::ACIRDialect::initialize() {
  addAttributes<
#define GET_ATTRDEF_LIST
#include "acir/Dialect/ACIR/ACIRAttributes.cpp.inc"
      >();
  addTypes<
#define GET_TYPEDEF_LIST
#include "acir/Dialect/ACIR/ACIRTypes.cpp.inc"
      >();
  addOperations<
#define GET_OP_LIST
#include "acir/Dialect/ACIR/ACIROps.cpp.inc"
      >();
}
