// Conversion-common helpers shared verbatim by the ACIRToGFIR and GFIRToHWIR
// conversion targets. They belong to neither conversion stage: budget
// prepayment, admission folding and atomic candidate acceptance are used by
// both routing and refinement, so hosting them in either target would force a
// forbidden cross-conversion dependency. The bodies are moved unchanged from
// the former Transforms/LowerTransactions.cpp conversion shell.
#ifndef PYCIRCUIT_CONVERSION_CONVERSIONCOMMON_H
#define PYCIRCUIT_CONVERSION_CONVERSIONCOMMON_H

#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/Operation.h"
#include "mlir/IR/Value.h"

namespace acir::ac::detail {
class SourceCheckBudget;
}

namespace acir::detail {
bool charge(ac::detail::SourceCheckBudget &budget, mlir::Operation *site,
            uint64_t units = 1);
bool prepayOperationHeader(mlir::Operation *op,
                           ac::detail::SourceCheckBudget &budget);
mlir::LogicalResult prepayClone(mlir::Operation *root,
                                ac::detail::SourceCheckBudget &budget);

/// Prove an admitted Bits1 SSA value is exactly constant one. Only closed
/// constants and bitwise/compare/select combinations are folded; any unknown
/// operand or unsupported operation is false, never a guess. This decides the
/// plain DFF leaf from the actually materialized commit enable.
bool knownTrue(mlir::Value value, const ac::HardwareAnalysis &hardware,
               ac::detail::SourceCheckBudget &budget, unsigned depth = 0);

/// All verification finishes before the first original mutation.
void acceptTransactionCandidate(mlir::ModuleOp input, mlir::ModuleOp staged);
} // namespace acir::detail
#endif
