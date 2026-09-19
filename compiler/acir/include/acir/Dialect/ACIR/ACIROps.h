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

#include <string>
#include <vector>

#include "acir/Dialect/ACIR/ACIROpInterfaces.h.inc"

#define GET_OP_CLASSES
#include "acir/Dialect/ACIR/ACIROps.h.inc"

namespace acir::ac {

struct ExactRuleFootprintInput {
  mlir::Operation *endpoint = nullptr;
  std::string resource;
  std::string access;
  mlir::Value index;
  std::vector<std::string> fields;
  bool wholeEntry = false;
  mlir::Value predicate;
};

struct ExactRuleEffectSummary {
  mlir::ArrayAttr expressionDAG;
  mlir::ArrayAttr footprints;
};

/// Normalize live rule/firing SSA into Decision 0271's deterministic typed
/// expression DAG and exact state footprints. The supplied footprints must be
/// in source order and point at endpoints nested in `scope`.
mlir::FailureOr<ExactRuleEffectSummary>
buildExactRuleEffectSummary(mlir::Operation *scope,
                            llvm::ArrayRef<ExactRuleFootprintInput> footprints);
mlir::FailureOr<ExactRuleEffectSummary>
buildExactRuleEffectSummary(mlir::Operation *scope);

/// Collect live footprint endpoints in the same deterministic order used by
/// exact-summary construction. Analysis clients use the live values only for
/// shared proof routines; persisted roots remain the debug graph authority.
llvm::SmallVector<ExactRuleFootprintInput>
collectExactRuleFootprintInputs(mlir::Operation *scope);

/// Independently reconstruct and compare a persisted exact summary against the
/// live body. This never trusts derived index/guard classifications.
mlir::LogicalResult
verifyExactRuleEffectSummary(mlir::Operation *scope,
                             mlir::ArrayAttr persistedDAG,
                             mlir::ArrayAttr persistedFootprints);

/// Verify the complete lowered-rule proof carried by an ac.transform produced
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
