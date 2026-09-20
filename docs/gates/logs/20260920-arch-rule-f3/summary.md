# Architecture Rule F3 Evidence

Decision 0272 remains `implemented-unverified`. This evidence covers only the
bounded gfsim F3 slice; matched cpp/SVA emission, trace, and coverage linkage
remain F5 acceptance work.

## Implemented contract

- Exactly one module-owned symbol operation, `ac.arch_obligation`, with an
  explicit structural stable ID and the closed 14-kind, severity, status,
  target, sampling-kind, and sampling-edge vocabularies.
- Conditions use scope-qualified `(ac.arch_expression_table, rule, node)`
  references. Multiple obligations from one source rule receive deterministic
  unique scope keys carrying the typed `owner_rule`, so no source-order scope
  replacement occurs. QueueGraph requires the condition, active, and disable
  scopes to be owned by the sampled firing and validates every reachable
  `rule_input` ordinal and type against that firing's exact Queue interface.
  F1 and obligations share the live-SSA typed expression
  normalizer. F2 JSON, ranks, footprint debug IDs, and hashes are not read.
- Pass order after exact effect/activation summaries and before rule lowering:
  infer, prove, materialize. Predicate-exclusive same-field writers are proved
  through the shared `WriterArbitrationAnalysis`; field-disjoint writers are
  not misclassified as mutual exclusion. Each proof ID includes sorted declared
  endpoint identities; two conflict pairs on one owner and rule remain distinct,
  and duplicate recomputed proof identities reject instead of being dropped.
  Proof pairs are oriented by those endpoint identities before condition-DAG
  normalization, certificate root assignment, provenance selection, and proved
  elision extraction. A forward/reversed declaration-order fixture compares the
  complete emitted obligation IR and canonical QueueGraph JSON byte-for-byte.
- Closure independently recomputes inferred proofs and runtime range programs,
  rejects removed/duplicate/dangling/forged/pending/rejected/unsupported or
  target-incomplete records, and rejects transient rule markers. QueueGraph
  extraction and direct plan validation are fail-closed.
- QueueGraph preserves and revalidates the module/local ID, closed enums,
  condition and optional predicate roots, complete expression-node attributes,
  sampling union, source rules, typed owners, proof/materialization linkage,
  NDF IDs, and canonical source provenance. Canonical JSON sorts expression
  scopes and obligations and emits every one of those fields.
- Proved obligations never enter the executable QueueGraph obligation list.
  Verified ACIR extraction converts them to closed `proved_obligation_elisions`
  containing the ID, module, kind, predicate-exclusive reason, endpoints,
  owner identity, selected canonical source provenance, and property root.
  Direct `status=proved` plans reject for
  range, onehot0, and single-writer kinds; an opaque certificate cannot admit
  backend behavior.
- The executable subset is a scalar range condition `value <= maximum`,
  `pre_publish`, selected target `gfsim`. The exact ID is preserved through
  QueueGraphPlan JSON and generated code. Failure occurs before prepare or
  publication, sets the unsuccessful-run diagnostic to that ID, and cancels
  the commit group. Structured failure retains ID, severity, source, module,
  and instance path through `TerminationResult`; fatal stops before the next
  Work object, while error permits only the already-bounded Work snapshot and
  then fails before arbitration. The check is ordinary control flow and
  survives `NDEBUG`.

## Focused evidence

All commands ran from the current checkout.

```text
ninja -C .pycircuit_out/build-llvm22 acir-opt CodeGenTests GfsimTests ACIRModelAnalysisTests ACIROpsTests
PASS

acir-opt --pass-pipeline='builtin.module(ac-lower-rules)' architecture-obligation-inference.mlir | FileCheck architecture-obligation-inference.mlir
PASS (one source rule, two same-owner conflict pairs plus one second-owner pair,
three distinct proved records, and printed-IR reparse)

acir-opt architecture-obligation.mlir | FileCheck architecture-obligation.mlir
PASS

acir-opt -split-input-file -verify-diagnostics architecture-obligation-invalid.mlir
PASS

acir-opt --ac-verify-rule-closure -verify-diagnostics architecture-obligation-{closure,forged,missing-rule}.mlir
PASS (three direct negative files)

GfsimTests --gtest_filter=QueueBlocksTest.*Obligation*
2 passed (Release/NDEBUG build; structured diagnostics and fatal/error control)

CodeGenTests --gtest_filter=QueueGraphPlanTest.RuntimeObligationPlanIsExactAndGenerated
1 passed; four generated `-DNDEBUG` C++ executables ran. Boundary 127 consumed
and published exactly once with one state update; 200 left input/output/state
unchanged; inactive and reset/recovery-disabled monitors skipped while the
firing still consumed, published, and updated state.
The same test rejects cross-rule condition/active/disable scopes, wrong reachable
input ordinals/types, materialization tamper, every direct proved plan, and
validates canonical proved-elision JSON.

CodeGenTests --gtest_filter=QueueGraphPlanTest.WriterProofOrientationIsIndependentOfDeclarationOrder
1 passed; forward and reversed writer declarations emitted byte-identical
obligation IR and canonical QueueGraph JSON.

ACIRModelAnalysisTests
36 passed (includes F1/F2 exact-summary, value-constraint, writer-arbitration,
and RuleEffectGraph regressions)

ACIROpsTests
1843 passed, including recomputed three-pair closure removal and certificate
tamper negatives

GfsimTests
263 passed

python3 flows/tools/check_api_hygiene.py python/pycircuit/src/pycircuit examples/pycircuit docs README.md
PASS

mkdocs build --strict
PASS
```

An earlier unfiltered `ACIROpsTests` run exposed the expected registry delta
from stacked `ac.module.import` plus `ac.arch_obligation`; the exact registry
expectations were updated and the two affected tests then passed. No failure
was hidden or marked XFAIL.

## Bounded validation gaps

- `python3 tools/agentic-circuit/check-contracts.py` could not run because the
  checkout environment lacks the locked `jsonschema` development dependency;
  it reported `jsonschema is unavailable; install requirements-dev.lock`.
- The aggregate `check-acir` target cannot execute in this existing build
  directory because CMake configured its lit driver as `/usr/bin/true`; the
  target invokes that binary through Python and fails with a syntax error.
  Six affected obligation files were instead run directly with `acir-opt`,
  `-verify-diagnostics`, and FileCheck where applicable.
- The decision-status checker still reports the pre-existing in-scope gaps
  0273 and 0274. F3 did not relabel, XFAIL, or otherwise absorb those failures.
- The complete `CodeGenTests` binary still has nine existing exact-summary
  fixture failures (`verified firing requires exact typed rule footprints`);
  the F3 focused generated-code test passes. These failures are not XFAILed or
  claimed as F3 success.
- Repeated generated structured-module instances are not added by this bounded
  F3 slice; instance-path preservation is exercised through two independently
  constructed runtime objects with the same local name. Full generated
  repeated-instance composition remains Decision 0274/F4-F5 work.
- cpp/SVA, liveness/temporal semantics, trace, and coverage are intentionally
  rejected or deferred to F5; they are not claimed by this evidence.

## Independent review verdicts

- Code reviewer: **PASS**. No remaining F3 P0/P1 finding; proved obligations
  are non-executable elisions, runtime checks remain fail-closed, and writer
  proof orientation is independent of declaration order.
- Verifier: **PASS**. The bounded gfsim F3 slice and its negative matrix are
  supported by fresh direct-MLIR, generated `NDEBUG`, runtime, F1/F2
  regression, API, documentation, and diff evidence above.
