#pragma once
#include "mlir/Bytecode/BytecodeOpInterface.h"
#include "mlir/IR/BuiltinTypes.h"
#include "mlir/IR/Dialect.h"
#include "mlir/IR/OpDefinition.h"
#include "mlir/Interfaces/SideEffectInterfaces.h"

#include "ACIRDialect.h.inc"
#define GET_TYPEDEF_CLASSES
#include "ACIRTypes.h.inc"
#define GET_OP_CLASSES
#include "ACIROps.h.inc"
