#ifndef ACIR_DIALECT_GFIR_GFIROPS_H
#define ACIR_DIALECT_GFIR_GFIROPS_H

#include "pycircuit/Dialect/ACIR/ACIRTypes.h"
#include "pycircuit/Interfaces/LogicalStateInterfaces.h"
#include "mlir/Bytecode/BytecodeOpInterface.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/OpDefinition.h"
#include "mlir/Interfaces/SideEffectInterfaces.h"

#define GET_OP_CLASSES
#include "pycircuit/Dialect/GFIR/GFIROps.h.inc"

#endif // ACIR_DIALECT_GFIR_GFIROPS_H
