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
#include "llvm/Support/Error.h"

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

struct NormalizedRuleExpressions {
  mlir::ArrayAttr expressionDAG;
  llvm::SmallVector<int64_t> roots;
};

/// Shared Decision 0271 expression normalizer. Architecture obligations use
/// the same closed live-SSA traversal as exact footprints; callers persist the
/// returned roots as scope-qualified (table, rule, node) references.
mlir::FailureOr<NormalizedRuleExpressions>
normalizeRuleExpressions(mlir::Operation *scope,
                         llvm::ArrayRef<mlir::Value> roots);

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

/// Materialize one family interface for a concrete ordered case. This is the
/// shared verifier/codegen authority for Decision 0278 dependent types.
llvm::Expected<ModuleInterfaceAttr>
materializeModuleInterface(ModuleInterfaceAttr interface,
                           StaticArgumentsAttr arguments,
                           mlir::FunctionType signature = {},
                           mlir::ModuleOp file = {});

/// Materialize one source-owned dependent struct schema for an exact typed
/// application. The returned canonical field dictionaries contain only
/// `{name, type}` with concrete physical types.
llvm::Expected<mlir::ArrayAttr>
materializeStructFields(StructOp structure,
                        DependentArgumentsAttr arguments,
                        mlir::ModuleOp file = {});

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
