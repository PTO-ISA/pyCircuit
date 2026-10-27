#include "ACIRSourceContracts.h"
#include "pycircuit/Dialect/ACIR/ACIROps.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"

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

LogicalResult SourcePredicateOp::verify() {
  auto package = (*this)->getParentOfType<mlir::ModuleOp>();
  // The bridge preserves an original four-state control value in both source
  // SCF and transactional v2 ACIR. Its i1 result is structural, not evidence
  // that the input is known and not an executable two-state conversion.
  HardwareAnalysis analysis(package);
  HardwareBindings bindings;
  bindings.owner = (*this)->getParentOfType<ModuleOp>();
  auto width = analysis.getPackedWidth(getInput().getType(), bindings, *this);
  if (failed(width) || *width != 1)
    return emitOpError() << "source SCF predicate requires width-one Bits";
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
