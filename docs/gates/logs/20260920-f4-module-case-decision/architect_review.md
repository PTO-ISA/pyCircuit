# Bounded architecture review: F4 finite module case bodies

## Scope reviewed

This review covers only the executable body carrier and cross-layer closure for
the finite static families accepted by Decision 0275. It refines Decisions 0274
and 0275; it does not approve an implementation, reopen structural
specialization equality, or close Decisions 0274-0276.

## Accepted architecture

- One `ac.module` family symbol owns ordered non-symbol `ac.module.case`
  concrete High ACIR regions. An unparameterized module normalizes to one
  empty-argument case.
- `StaticParameterDecl`, case arguments, and dependent nominal type
  applications remain typed and ordered. A dictionary, ordinal, suffix, or
  case symbol is not identity.
- Every case owns its state, resources, rules, proof facts, architecture
  obligations, body provenance, and verification closure.
- A source-owned dependent interface skeleton materializes one checked concrete
  signature per case. Calls and imports identify the family plus ordered typed
  arguments, and header/link verification covers the full case set regardless
  of caller reachability.
- QueueGraph carries typed `ModuleFamilyPlan` and `ModuleCasePlan` records. PYC
  preserves and verifies a typed family/case carrier before C++ or RTL emission.
- Python authoring uses `static_parameter`, `static_int`, `one_of`,
  `integer_range`, `case`, `module_decl`, and `module(declaration=...)`.
- C++ explicitly materializes every case under one unchanged readable family
  identifier. RTL emits one typed parameter/generate family with exact case
  coverage.
- The hard break removes concrete specialization symbols and suffixes,
  dictionary schemas, per-case symbols/files, and
  `specializations.json`-style sidecars without compatibility readers or dual
  modes.

## Invariants and testable risks

1. Family and case inventories must remain identical and ordered across source,
   header/import, link, QueueGraph, PYC, C++, and RTL.
2. Dependent interface materialization must be total and must match each
   concrete body signature before the body can enter a plan.
3. Case-local proofs and obligations must not be accidentally shared merely
   because two cases have structurally similar bodies.
4. Unused declared cases must survive all lowering and emission stages.
5. The unparameterized normalization must not retain a second legacy body path.
6. Backend convenience must not reconstruct cases from names, dictionaries,
   caller observations, or manifests.

The Decision 0276 acceptance and negative matrices directly exercise these
risks and are mandatory implementation evidence.

## Review verdict

The finite concrete case-region model is sufficiently closed for F4
implementation. It preserves one source/family identity, keeps dependent
interfaces verifiable without child bodies, makes case-local semantic closure
explicit, and gives both backends a single typed carrier. Status remains
`gap-in-scope`; current concrete-symbol code or tests do not satisfy the
decision.

## Deferred boundary

Open or symbolic parametric bodies, wildcard/generated case sets,
caller-driven monomorphization, runtime-selected static arguments, shared
parametric body IR, proof generalization across cases, and richer dependent
types require a later decision.
