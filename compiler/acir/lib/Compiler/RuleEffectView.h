#ifndef ACIR_LIB_COMPILER_RULEEFFECTVIEW_H
#define ACIR_LIB_COMPILER_RULEEFFECTVIEW_H

#include "Dialect/ACIR/ACIRSourceContracts.h"
#include "acir/Dialect/ACIR/ACIROps.h"

#include "llvm/ADT/SmallVector.h"

#include <string>

namespace acir::compiler::detail {

struct RuleEffect {
  mlir::DictionaryAttr state;
  mlir::DictionaryAttr logicalType;
  mlir::Value currentHandle;
  mlir::Value nextHandle;
  bool read = false;
  bool write = false;
  std::string precision = "exact";
  llvm::SmallVector<mlir::DictionaryAttr> origins;
};

struct RuleEffectView {
  llvm::SmallVector<RuleEffect> effects;
};

mlir::FailureOr<RuleEffectView>
inferSourceRuleEffects(ac::RuleOp rule, ac::detail::EmitError emitError);
mlir::FailureOr<RuleEffectView>
inferSourceModuleEffects(ac::ModuleOp module, ac::detail::EmitError emitError);

} // namespace acir::compiler::detail

#endif // ACIR_LIB_COMPILER_RULEEFFECTVIEW_H
