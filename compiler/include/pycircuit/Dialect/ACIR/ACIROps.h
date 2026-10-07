#ifndef ACIR_DIALECT_ACIR_ACIROPS_H
#define ACIR_DIALECT_ACIR_ACIROPS_H

#include "pycircuit/Dialect/ACIR/ACIRTypes.h"
#include "mlir/Bytecode/BytecodeOpInterface.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/OpDefinition.h"
#include "mlir/IR/BuiltinTypes.h"
#include "mlir/IR/RegionKindInterface.h"
#include "mlir/IR/SymbolTable.h"
#include "mlir/Interfaces/SideEffectInterfaces.h"

#define GET_OP_CLASSES
#include "pycircuit/Dialect/ACIR/ACIROps.h.inc"

namespace acir::ac {

/// Construct the closed dictionary used by module/import static parameters.
mlir::DictionaryAttr
getStaticParameterAttr(mlir::Builder &builder, llvm::StringRef name,
                       StaticExprAttr defaultValue = {});

/// Construct one ordered per-output dependency summary entry.
mlir::DictionaryAttr
getOutputDependencyAttr(mlir::Builder &builder, unsigned output,
                        llvm::ArrayRef<unsigned> inputs);

} // namespace acir::ac

#endif // ACIR_DIALECT_ACIR_ACIROPS_H
