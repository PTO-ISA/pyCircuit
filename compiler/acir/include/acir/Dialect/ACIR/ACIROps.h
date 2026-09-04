#ifndef ACIR_DIALECT_ACIR_ACIROPS_H
#define ACIR_DIALECT_ACIR_ACIROPS_H

#include "acir/Dialect/ACIR/ACIRTypes.h"
#include "mlir/Bytecode/BytecodeOpInterface.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/OpDefinition.h"
#include "mlir/IR/RegionKindInterface.h"
#include "mlir/IR/SymbolTable.h"
#include "mlir/Interfaces/ControlFlowInterfaces.h"
#include "mlir/Interfaces/DataLayoutInterfaces.h"
#include "mlir/Interfaces/FunctionInterfaces.h"
#include "mlir/Interfaces/SideEffectInterfaces.h"
#include "llvm/ADT/StringMap.h"

#include "acir/Dialect/ACIR/ACIROpInterfaces.h.inc"

#define GET_OP_CLASSES
#include "acir/Dialect/ACIR/ACIROps.h.inc"

namespace acir::ac {

/// Verify the complete phase-one proof carried by an ac.transform produced
/// from ac.firing. Plain transforms without any ac.rule_* attributes succeed.
mlir::LogicalResult verifyLoweredRuleTransformContract(TransformOp transform);

/// Verifies symbol resolution and linear-use rules for ACIR topology types on
/// an arbitrary operation. This is called by the whole-file ACIR verifier.
mlir::LogicalResult verifyTopologyTypeUses(mlir::Operation *operation);

/// Resolve an ACIR runtime symbol. Flat `@q` looks up in the enclosing
/// `ac.module`. Nested `@Core::@q` looks up `q` inside module `@Core`.
mlir::Operation *lookupRuntimeSymbol(mlir::Operation *from,
                                     mlir::SymbolRefAttr ref);

/// Leaf symbol of a flat or nested runtime reference (`q` in `@Core::@q`).
llvm::StringRef runtimeSymbolLeaf(mlir::SymbolRefAttr ref);

} // namespace acir::ac

#endif
