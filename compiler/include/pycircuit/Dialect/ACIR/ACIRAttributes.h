#ifndef ACIR_DIALECT_ACIR_ACIRATTRIBUTES_H
#define ACIR_DIALECT_ACIR_ACIRATTRIBUTES_H

#include "mlir/IR/BuiltinAttributes.h"
#include "llvm/ADT/APSInt.h"
#include "llvm/ADT/SmallString.h"

#include "pycircuit/Dialect/ACIR/ACIREnums.h.inc"

#define GET_ATTRDEF_CLASSES
#include "pycircuit/Dialect/ACIR/ACIRAttributes.h.inc"

#endif // ACIR_DIALECT_ACIR_ACIRATTRIBUTES_H
