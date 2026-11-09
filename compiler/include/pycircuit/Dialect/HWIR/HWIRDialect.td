#ifndef PYCIRCUIT_DIALECT_HWIR_HWIRDIALECT_TD
#define PYCIRCUIT_DIALECT_HWIR_HWIRDIALECT_TD

include "mlir/IR/DialectBase.td"

def HWIR_Dialect : Dialect {
  let name = "hwir";
  let cppNamespace = "::acir::hwir";
  let summary = "pycircuit outgoing hardware IR";
  let description = [{
    HWIR is the executable hardware boundary that replaces the final ACIR
    package. It keeps the same hardware meaning: reusable module definitions,
    independent instances and collections, module-local combinational rules,
    explicit module outputs, closed trusted imports, a selected system root,
    retained checks and managed observations.

    Payload types, static parameters and the admitted pure computation are
    shared with ACIR instead of being copied. Structural operations have their
    own owner so they do not depend on the retired final stage marker.

    A package is a builtin module carrying `hwir.contract = 1 : i64`. Nominal
    struct and enum declarations remain top-level package declarations next to
    module definitions, because the shared ACIR type verifiers require that
    placement. `hwir.system` selects the root of the instance tree.

    Verification is split: operation verifiers check local structure, while the
    public `verifyHWIRPackage` entry checks package closure, definition
    resolution, connected operands and the admitted operation set.
  }];
  let dependentDialects = ["::acir::ac::ACIRDialect"];
  let hasOperationAttrVerify = 1;
}

#endif // PYCIRCUIT_DIALECT_HWIR_HWIRDIALECT_TD
