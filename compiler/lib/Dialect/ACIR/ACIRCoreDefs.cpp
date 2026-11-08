// Generated attribute/type definitions of the ac dialect (B2 step-2 design
// r9): per-def storage, getters, TypeID anchors and the default
// parse/print hooks. ACIRCore's archive carries these symbols below the
// operations layer. ACIRRegistration.cpp re-includes the same generated
// classes to instantiate addAttributes/addTypes in initialize(); the
// definitions are weak (standard MLIR pattern), so the duplicate
// compilation is safe under static-archive linking.
#include "pycircuit/Dialect/ACIR/ACIRDialect.h"

#include "mlir/IR/DialectImplementation.h"
#include "llvm/ADT/TypeSwitch.h"

#include "pycircuit/Dialect/ACIR/ACIREnums.cpp.inc"

#define GET_ATTRDEF_CLASSES
#include "pycircuit/Dialect/ACIR/ACIRAttributes.cpp.inc"

#define GET_TYPEDEF_CLASSES
#include "pycircuit/Dialect/ACIR/ACIRTypes.cpp.inc"
