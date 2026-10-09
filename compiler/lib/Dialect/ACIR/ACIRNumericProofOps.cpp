#include "ACIRSourceContracts.h"
#include "pycircuit/Dialect/ACIR/ACIROps.h"
using namespace mlir;
namespace acir::ac {
LogicalResult ValueBindingOp::verify() {
  if (failed(detail::verifyValueID(getId(), [&] { return emitOpError(); })))
    return failure();
  return detail::verifyLogicalTypeStructure(getDomain(),
                                            [&] { return emitOpError(); });
}
LogicalResult ValueUseOp::verify() {
  if (failed(detail::verifyUseID(getId(), [&] { return emitOpError(); })))
    return failure();
  return detail::verifyValueID(getSource(), [&] { return emitOpError(); });
}
LogicalResult NumericProofOp::verify() {
  return emitOpError() << "numeric proof requires an implemented common proof "
                          "verifier; retired recipe proofs are unsupported";
}
} // namespace acir::ac
