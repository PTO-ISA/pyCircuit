#ifndef ACIR_LIB_COMPILER_FINALCOMPOSEDNUMERIC_H
#define ACIR_LIB_COMPILER_FINALCOMPOSEDNUMERIC_H
#include "FinalNumeric.h"
namespace acir::compiler {
mlir::FailureOr<FinalNumericRuleSnapshot>
freezeComposedNumeric(InstanceView &owner, ac::RuleOp rule,
                      ac::detail::EmitError error);
mlir::LogicalResult
verifyComposedNumeric(const FinalNumericRuleSnapshot &snapshot,
                      const ModuleGraph &modules, ac::detail::EmitError error);
} // namespace acir::compiler
#endif
