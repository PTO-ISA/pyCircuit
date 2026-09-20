#ifndef PYC_DIALECT_PYC_DIALECT_TD
#define PYC_DIALECT_PYC_DIALECT_TD
//===- PYCDialect.td ------------------------------------------------------===//
//
// PYC: a common IR for hardware construction and compilation.
//
//===----------------------------------------------------------------------===//

include "mlir/IR/DialectBase.td"

def PYCDialect : Dialect {
  let name = "pyc";
  let cppNamespace = "::pyc";
  let summary = "pyCircuit hardware dialect";
  let useDefaultAttributePrinterParser = 1;
  let description = [{
PYC is a small, backend-agnostic circuit IR with explicit multi-clock modeling.

The goal is to represent common circuit components (reg, mux, fifo, logic ops)
and then lower into backend-specific template libraries (C++ / Verilog).
  }];
}

#endif // PYC_DIALECT_PYC_DIALECT_TD
