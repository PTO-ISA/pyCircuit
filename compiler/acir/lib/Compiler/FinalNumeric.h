#ifndef ACIR_LIB_COMPILER_FINALNUMERIC_H
#define ACIR_LIB_COMPILER_FINALNUMERIC_H

#include "CheckGraph.h"
#include "ProposalGraph.h"

#include "llvm/ADT/DenseMap.h"

namespace acir::compiler {

struct FinalNumericValueSnapshot {
  ac::ValueBindingOp operation;
  mlir::DictionaryAttr attributes;
  mlir::DictionaryAttr id;
  mlir::DictionaryAttr domain;
  mlir::Value value;
  mlir::Value valid;
  mlir::Value path;
};

struct FinalNumericProofSnapshot {
  ac::NumericProofOp operation;
  mlir::DictionaryAttr attributes;
  llvm::SmallVector<mlir::Value, 7> operands;
};

struct FinalNumericCarrierSnapshot {
  mlir::Operation *operation = nullptr;
  mlir::DictionaryAttr attributes;
  llvm::SmallVector<mlir::Value> operands;
  llvm::SmallVector<mlir::Type> results;
};

struct FinalNumericRuleSnapshot {
  bool composition = false;
  llvm::SmallVector<FinalNumericCarrierSnapshot> composedCarriers;
  InstanceView *owner = nullptr;
  ac::RuleOp rule;
  mlir::DictionaryAttr stateID;
  mlir::DictionaryAttr relativeState;
  mlir::DictionaryAttr useID;
  mlir::DictionaryAttr sourceID;
  mlir::DictionaryAttr checkID;
  mlir::ArrayAttr recipe;
  mlir::ArrayAttr requiredUses;
  mlir::ArrayAttr yieldBindings;
  mlir::DictionaryAttr proofScope;
  llvm::SmallVector<FinalNumericValueSnapshot, 6> values;
  llvm::SmallVector<FinalNumericProofSnapshot, 2> proofs;
  mlir::Value current;
  ac::ValueUseOp use;
  mlir::DictionaryAttr useAttributes;
  mlir::Value useValue;
  mlir::Value useValid;
  mlir::Value usePath;
  mlir::Value yieldData;
  mlir::Value yieldEnable;
  mlir::Value checkCondition;
  mlir::Value checkPath;
};

bool isSupportedFinalNumericRule(ac::RuleOp rule);
bool isRetainedFinalNumericCarrier(
    llvm::ArrayRef<FinalNumericRuleSnapshot> snapshots,
    mlir::Operation *operation);

mlir::FailureOr<llvm::SmallVector<FinalNumericRuleSnapshot, 0>>
freezeFinalNumericEvidence(const ModuleGraph &modules,
                           const ProposalGraph &proposals,
                           const CheckGraph &checks,
                           ac::detail::EmitError emitError);
mlir::FailureOr<llvm::SmallVector<FinalNumericRuleSnapshot, 0>>
freezeFinalNumericEvidenceFromHardware(const ModuleGraph &modules,
                                       const ProposalGraph &proposals,
                                       const CheckGraph &checks,
                                       ac::detail::EmitError emitError);

void rebaseFinalNumericEvidence(
    llvm::MutableArrayRef<FinalNumericRuleSnapshot> snapshots,
    const llvm::DenseMap<mlir::Value, mlir::Value> &replacements);

mlir::LogicalResult verifyFinalNumericEvidence(
    llvm::ArrayRef<FinalNumericRuleSnapshot> snapshots,
    const ModuleGraph &modules, const ProposalGraph &proposals,
    const CheckGraph &checks, ac::detail::EmitError emitError);

} // namespace acir::compiler

#endif // ACIR_LIB_COMPILER_FINALNUMERIC_H
