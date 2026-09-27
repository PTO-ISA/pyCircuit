#ifndef ACIR_DIALECT_ACIR_ACIROPS_H
#define ACIR_DIALECT_ACIR_ACIROPS_H

#include "acir/Dialect/ACIR/ACIRTypes.h"
#include "mlir/Bytecode/BytecodeOpInterface.h"
#include "mlir/IR/OpDefinition.h"
#include "mlir/IR/SymbolTable.h"
#include "mlir/Interfaces/SideEffectInterfaces.h"

#define GET_OP_CLASSES
#include "acir/Dialect/ACIR/ACIROps.h.inc"

#endif // ACIR_DIALECT_ACIR_ACIROPS_H
