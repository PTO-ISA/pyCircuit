#ifndef ACIR_DIALECT_GFIR_GFIRDIALECT_TD
#define ACIR_DIALECT_GFIR_GFIRDIALECT_TD

include "mlir/IR/DialectBase.td"

def GFIR_Dialect : Dialect {
  let name = "gfir";
  let cppNamespace = "::acir::gfir";
  let summary = "pycircuit routed General Flow IR stage dialect";
  let description = [{
    GFIR owns the routed package's logical state operations. Routing
    constructs gfir.state as the authenticated replacement of one source
    ac.state declaration and gfir.state.update as its single candidate and
    write-intent binding; no other operation belongs to this dialect and the
    stage verifier admits exactly these two classes. Payloads keep their
    common !ac.* logical types; the dialect declares no types or attributes
    of its own.
  }];
}

#endif // ACIR_DIALECT_GFIR_GFIRDIALECT_TD
