#ifndef PYCIRCUIT_DIALECT_HWIR_HWIROPS_H
#define PYCIRCUIT_DIALECT_HWIR_HWIROPS_H

#include "pycircuit/Dialect/ACIR/ACIRTypes.h"

#include "mlir/Bytecode/BytecodeOpInterface.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinTypes.h"
#include "mlir/IR/OpDefinition.h"
#include "mlir/IR/SymbolTable.h"
#include "mlir/IR/RegionKindInterface.h"
#include "mlir/Interfaces/SideEffectInterfaces.h"

#define GET_OP_CLASSES
#include "pycircuit/Dialect/HWIR/HWIROps.h.inc"

#endif // PYCIRCUIT_DIALECT_HWIR_HWIROPS_H
