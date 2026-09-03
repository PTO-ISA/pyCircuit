#ifndef ACIR_TRANSFORMS_PASSES
#define ACIR_TRANSFORMS_PASSES

include "mlir/Pass/PassBase.td"

def VerifyModelPass : Pass<"ac-verify-model", "::mlir::ModuleOp"> {
  let summary = "Verify complete Agentic Circuit model semantics";
  let description = [{
    Runs local ACIR verification together with deterministic whole-model
    hierarchy, ownership, purity, zero-delay dependency, and frozen-integrity
    checks.
  }];
}

def CanonicalizeModelPass
    : Pass<"ac-canonicalize-model", "::mlir::ModuleOp"> {
  let summary = "Canonicalize Agentic Circuit model topology";
  let description = [{
    Sorts symbol and Graph declarations by stable semantic keys and removes
    source-location variance before deterministic topology freeze.
  }];
}

def FreezeTopologyPass : Pass<"ac-freeze-topology", "::mlir::ModuleOp"> {
  let summary = "Freeze deterministic Agentic Circuit topology";
  let description = [{
    Verifies the selected static architecture, proves freeze-phase contracts,
    binds elaborated owner paths and stable IDs, and persists an integrity
    digest that makes later topology mutation a hard error.
  }];
}

def LowerProcessStatePass : Pass<"ac-lower-process-state", "::mlir::ModuleOp"> {
  let summary = "Plan ACSim process state from frozen ACIR";
  let description = [{
    Analyzes a selected topology-frozen model without mutation and produces
    an immutable ProcessStatePlanSet.
  }];
}

#endif // ACIR_TRANSFORMS_PASSES
