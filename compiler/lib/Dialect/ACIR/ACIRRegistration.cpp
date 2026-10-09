#include "pycircuit/Dialect/ACIR/ACIRDialect.h"

#include "mlir/IR/Builders.h"
#include "mlir/IR/DialectImplementation.h"
#include "llvm/ADT/TypeSwitch.h"

#include "pycircuit/Dialect/ACIR/ACIREnums.cpp.inc"

#define GET_ATTRDEF_CLASSES
#include "pycircuit/Dialect/ACIR/ACIRAttributes.cpp.inc"

#define GET_TYPEDEF_CLASSES
#include "pycircuit/Dialect/ACIR/ACIRTypes.cpp.inc"

#define GET_OP_CLASSES
#include "pycircuit/Dialect/ACIR/ACIROps.cpp.inc"

void acir::ac::ACIRDialect::initialize() {
  addAttributes<
#define GET_ATTRDEF_LIST
#include "pycircuit/Dialect/ACIR/ACIRAttributes.cpp.inc"
      >();
  addTypes<
#define GET_TYPEDEF_LIST
#include "pycircuit/Dialect/ACIR/ACIRTypes.cpp.inc"
      >();
  addOperations<
#define GET_OP_LIST
#include "pycircuit/Dialect/ACIR/ACIROps.cpp.inc"
      >();
}
