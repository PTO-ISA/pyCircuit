#ifndef ACIR_LIB_COMPILER_SCALARNUMERICLOWERINGDETAIL_H
#define ACIR_LIB_COMPILER_SCALARNUMERICLOWERINGDETAIL_H

#include "Dialect/ACIR/ACIRSourceContracts.h"
#include "acir/Dialect/ACIR/ACIROps.h"
#include "mlir/IR/BuiltinOps.h"
#include "llvm/ADT/SmallVector.h"

namespace acir::compiler::detail {

enum class UnitInventoryKind { NoOp, Source, Lowered };
enum class RuleInventoryKind {
  ExactInputAdd,
  ExactInputScalar,
  InputMask,
  CheckedToBits,
  NumericNextUse,
  Composition
};

struct RuleInventory {
  RuleInventoryKind kind = RuleInventoryKind::ExactInputAdd;
  ac::RuleOp rule;
  ac::SourceReadOp read;
  ac::MathFromBitsOp fromBits;
  ac::MathConstantOp constant;
  ac::MathBinaryOp add;
};

struct UnitInventory {
  UnitInventoryKind kind = UnitInventoryKind::NoOp;
  llvm::SmallVector<ac::RuleOp> sourceRules;
};

mlir::FailureOr<UnitInventory> inspectUnit(mlir::ModuleOp unit,
                                           ac::detail::EmitError emitError);

mlir::FailureOr<RuleInventory> inspectRule(ac::RuleOp rule);

} // namespace acir::compiler::detail

namespace acir::compiler {

mlir::LogicalResult lowerExactInputScalarRule(mlir::ModuleOp unit,
                                              ac::RuleOp rule);
mlir::LogicalResult lowerInputMaskRule(mlir::ModuleOp unit, ac::RuleOp rule);
mlir::LogicalResult lowerCheckedToBitsRule(mlir::ModuleOp unit,
                                           ac::RuleOp rule);
mlir::LogicalResult lowerNumericNextUseRule(mlir::ModuleOp unit,
                                            ac::RuleOp rule);
mlir::LogicalResult lowerNumericCompositionRule(mlir::ModuleOp unit,
                                                ac::RuleOp rule);

} // namespace acir::compiler

#endif // ACIR_LIB_COMPILER_SCALARNUMERICLOWERINGDETAIL_H
