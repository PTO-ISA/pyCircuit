#ifndef ACIR_LIB_COMPILER_RULEINVENTORY_H
#define ACIR_LIB_COMPILER_RULEINVENTORY_H

#include "acir/Dialect/ACIR/ACIROps.h"

#include "llvm/ADT/StringRef.h"

namespace acir::compiler {

// Classifies whether a rule carries an explicit numeric obligation.
//
// This is deliberately not "does the rule contain value provenance". Final
// materialization synthesizes ac.value.binding / ac.value.use for *every*
// assignment, including an ordinary direct-current copy or a literal
// assignment, so those ops alone are not a numeric claim and must not force a
// U1 or composition closure.
//
// A numeric obligation is declared only by an explicit carrier:
//   - a non-empty ac.required_numeric inventory,
//   - an ac.numeric.proof op,
//   - ac.math.* source math ops,
//   - an op carrying an ac.check_template (helper/expansion stage).
//
// Callers keep their own verification responsibility: this helper only
// classifies. It must not be used to skip a closure check for a rule that does
// carry an obligation, and dialect-name heuristics (for example treating every
// arith.* op as numeric) are intentionally not used here.
inline bool ruleHasNumericObligation(ac::RuleOp rule) {
  if (!rule)
    return false;
  if (auto required = rule->getAttrOfType<mlir::ArrayAttr>("ac.required_numeric");
      required && !required.empty())
    return true;
  for (mlir::Operation &operation : rule.getBody().front()) {
    llvm::StringRef name = operation.getName().getStringRef();
    if (name.starts_with("ac.math.") || name == "ac.numeric.proof" ||
        operation.hasAttr("ac.check_template"))
      return true;
  }
  return false;
}

} // namespace acir::compiler

#endif // ACIR_LIB_COMPILER_RULEINVENTORY_H
