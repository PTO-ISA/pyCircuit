#ifndef ACIR_DIALECT_GFIR_GFIRDIALECT_TD
#define ACIR_DIALECT_GFIR_GFIRDIALECT_TD

include "mlir/IR/DialectBase.td"

def GFIR_Dialect : Dialect {
  let name = "gfir";
  let cppNamespace = "::acir::gfir";
  let summary = "pycircuit structured General Flow IR dialect";
  let description = [{
    GFIR represents reusable module graphs, static instance identities,
    ordinary interface calls and a selected package root. Native operation
    verification establishes local structure; the closed-package verifier
    establishes executable admission. The dialect also retains routed logical
    state declarations and updates for the existing staged pipeline. Payloads
    use common !ac.* logical types; GFIR declares no types or attributes of its
    own.
  }];
}

#endif // ACIR_DIALECT_GFIR_GFIRDIALECT_TD
