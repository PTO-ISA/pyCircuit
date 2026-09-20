#pragma once

#include "mlir/Bytecode/BytecodeOpInterface.h"
#include "mlir/IR/OpDefinition.h"
#include "mlir/IR/SymbolTable.h"
#include "mlir/Interfaces/InferTypeOpInterface.h"
#include "mlir/Interfaces/SideEffectInterfaces.h"

#include "pyc/Dialect/PYC/PYCDialect.h"
#include "pyc/Dialect/PYC/PYCAttributes.h"
#include "pyc/Dialect/PYC/PYCTypes.h"

#define GET_OP_CLASSES
#include "pyc/Dialect/PYC/PYCOps.h.inc"

namespace pyc {

acir::ac::DependentArgumentsAttr
dependentArgumentsFromStatic(acir::ac::StaticArgumentsAttr arguments);

mlir::FailureOr<acir::ac::StaticArgumentsAttr>
staticArgumentsFromDependent(acir::ac::DependentArgumentsAttr arguments);

} // namespace pyc
