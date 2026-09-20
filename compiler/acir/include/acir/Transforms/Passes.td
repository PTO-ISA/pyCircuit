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

def VerifyValueConstraintsPass
    : Pass<"ac-verify-value-constraints", "::mlir::ModuleOp"> {
  let summary = "Prove dynamic Agentic Circuit state indices are in range";
  let description = [{
    Runs ACDataFlowAnalyzer's bounded unsigned value-domain analysis and
    rejects shaped ac.var and rule/firing Table accesses whose dynamic index
    cannot be proven within the declared extent.
  }];
}

def BuildRuleEffectGraphPass
    : Pass<"ac-build-rule-effect-graph", "::mlir::ModuleOp"> {
  let summary = "Build the read-only whole-design rule effect graph";
  let description = [{
    Consumes independently verified exact rule summaries and the shared value
    constraint/writer arbitration analysis. Optional JSON and DOT files are
    deterministic debug/evidence views, never identity or release formats.
  }];
  let options = [
    Option<"jsonOutput", "json-output", "std::string", /*default=*/"",
           "Write the deterministic debug graph as JSON">,
    Option<"dotOutput", "dot-output", "std::string", /*default=*/"",
           "Write the deterministic debug graph as DOT">,
  ];
}

def CanonicalizeModelPass : Pass<"ac-canonicalize-model", "::mlir::ModuleOp"> {
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
    binds elaborated owner paths and stable IDs.
  }];
}

def LowerVariableStatePass
    : Pass<"ac-lower-variable-state", "::mlir::ModuleOp"> {
  let summary = "Select committed storage for persistent ac.var declarations";
}

def LowerValueContractsPass
    : Pass<"ac-lower-value-contracts", "::mlir::ModuleOp"> {
  let summary = "Lower recursive equality and reusable value invariants";
}

def PruneInternalPayloadsPass
    : Pass<"ac-prune-internal-payloads", "::mlir::ModuleOp"> {
  let summary = "Narrow verifier-proven private Transform payloads";
  let description = [{
    Rewrites eligible single-producer/single-consumer private struct Queue
    edges to exact tuple carriers after proving every live consumer use is a
    direct field projection. Public, observed, stateful, module, rule, and
    control edges remain unchanged.
  }];
}

def InlinePureHelpersPass
    : Pass<"ac-inline-pure-helpers", "::mlir::ModuleOp"> {
  let summary = "Inline explicitly annotated pure ACIR helper functions";
  let description = [{
    Expands calls to single-block private func.func definitions carrying
    `ac.inline = true`.  Whole-model purity verification remains the authority
    for rejecting unresolved, external, recursive, effectful, or looping
    helper closures.
  }];
}

def InferRuleTypesPass : Pass<"ac-infer-rule-types", "::mlir::ModuleOp"> {
  let summary = "Refine transient rule type-constraint markers";
}

def InferRuleEffectsPass : Pass<"ac-infer-rule-effects", "::mlir::ModuleOp"> {
  let summary = "Infer rule provenance and effect summaries";
}

def InferRuleActivationPass
    : Pass<"ac-infer-rule-activation", "::mlir::ModuleOp"> {
  let summary = "Infer typed rule activation and transaction resources";
}

def MaterializeRuleChecksPass
    : Pass<"ac-materialize-rule-checks", "::mlir::ModuleOp"> {
  let summary = "Materialize path-qualified rule checks";
}

def MaterializeRuleHandshakePass
    : Pass<"ac-materialize-rule-handshake", "::mlir::ModuleOp"> {
  let summary = "Materialize rule Queue handshake obligations";
}

def DischargeRuleObligationsPass
    : Pass<"ac-discharge-rule-obligations", "::mlir::ModuleOp"> {
  let summary =
      "Discharge materialized rule obligations with explicit evidence";
}

def ResolveRuleSchedulePass
    : Pass<"ac-resolve-rule-schedule", "::mlir::ModuleOp"> {
  let summary = "Resolve rule selection and scheduling contracts";
}

def LowerRulesToFiringPass
    : Pass<"ac-lower-rules-to-firing", "::mlir::ModuleOp"> {
  let summary = "Lower closed transient rules to internal firing IR";
}

def CanonicalizePureFiringsPass
    : Pass<"ac-canonicalize-pure-firings", "::mlir::ModuleOp"> {
  let summary = "Prove and canonicalize pure firings as Queue transforms";
}

def VerifyRuleClosurePass : Pass<"ac-verify-rule-closure", "::mlir::ModuleOp"> {
  let summary = "Reject transient rules and unresolved markers";
}

#endif // ACIR_TRANSFORMS_PASSES
