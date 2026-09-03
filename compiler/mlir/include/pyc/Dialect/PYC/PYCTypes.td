//===- PYCTypes.td --------------------------------------------------------===//
//
// PYC dialect types.
//
//===----------------------------------------------------------------------===//

include "mlir/IR/AttrTypeBase.td"
include "pyc/Dialect/PYC/PYCDialect.td"

def PYC_ClockType : TypeDef<PYCDialect, "Clock"> {
  let mnemonic = "clock";
  let summary = "Clock signal type";
}

def PYC_ResetType : TypeDef<PYCDialect, "Reset"> {
  let mnemonic = "reset";
  let summary = "Reset signal type";
}
