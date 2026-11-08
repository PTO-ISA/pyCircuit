#include "ACIRSourceContracts.h"
#include "pycircuit/Dialect/ACIR/ACIROps.h"

using namespace mlir;

namespace acir::ac {
LogicalResult SourceIndexOp::verify() {
  if (failed(detail::verifyOccurrence(getOccurrence(),
                                      [&] { return emitOpError(); })) ||
      failed(detail::verifySourceSpan(getSourceLocation(),
                                      [&] { return emitOpError(); })))
    return failure();
  return success();
}

// Grammar-only native verify (r3 ACIRSourceControlFlowOps row): the
// width-one predicate check runs from the staged predicate row (source-unit
// walk and resolved dispatch).
LogicalResult SourcePredicateOp::verify() {
  for (OpOperand &use : getResult().getUses()) {
    auto name = use.getOwner()->getName().getStringRef();
    if (use.getOperandNumber() != 0 ||
        (name != "scf.if" && name != "scf.condition"))
      return emitOpError()
             << "source SCF predicate may only control scf.if/scf.condition";
  }
  return success();
}
} // namespace acir::ac
