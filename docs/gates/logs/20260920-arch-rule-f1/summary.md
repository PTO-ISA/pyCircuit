# Architecture Rule F1: exact local effect summaries

Date: 2026-09-20

Decision: 0271 (F1 only; F2 remains gap-in-scope)

## Staged contract and review fixes

- `ac.rule.expression_dag` and lowered `ac.expression_dag` persist a
  deterministic topological expression table. Each node has a closed typed
  opcode, exact result type, ordered local operand handles, and a closed set of
  typed attributes. Local handles are serialization references, not semantic
  identities; no hash, digest, or fingerprint is present.
- Admitted leaves cover rule-input ordinals, committed Table identities, typed
  constants, and Table match/choose lane identities. Other live leaves or
  unknown expression operations/attributes reject.
- `ac.rule.footprints_exact` and lowered `ac.footprints_exact` retain source
  order, owner/resource, access, declaration-ordered fields, explicit
  all-entry form, exact index and predicate roots, endpoint kind, and source
  provenance. `index_kind` and `guard_kind` remain derived diagnostics only.
- Rule and firing verification independently rebuild the DAG and footprint
  records from live SSA. Serialized attributes are validated for topological
  closure before exact comparison.
- Rule-to-firing lowering preserves the exact records; print/parse provenance
  is anchored on the live endpoints so verification remains stable after
  serialization.
- Footprint records preserve endpoint provenance only. The enclosing firing's
  `ac.ndf_ids` and `ac.ndf_requires` remain the single rule-level NDF authority;
  F1 deliberately does not duplicate those IDs into each footprint.
- Table match nodes now include the normalized predicate-region yield and
  domain metadata. Table choose nodes include mask/key roots, count, policy,
  key ordering, cursor, selection stable identity, and result ordinal. Lane
  nodes reference their committed producer node plus lane ordinal.
- Slot valid/value reads and conditional releases now produce exact committed
  owner footprints. Table and Slot footprints carry resource spelling,
  canonical owner path, stable structural identity, and a closed whole-entry
  versus explicit-field alternative. Owner resolution is lexical to the use,
  so duplicate local symbols in different modules cannot alias.
- Match/choose projected fields are collected only from record values whose
  dataflow descends from that endpoint's lane block argument. Captured rule
  inputs and nested committed-state values are not attributed to the lane.
- Summary construction no longer writes provenance back into endpoints.
  Validated endpoint `ac.source_provenance` is preferred and cross-checked
  against source-language `FileLineColLoc` when present; source-language
  locations are next, and separately validated rule source metadata is the
  fallback. Transport-only `.mlir`/`.ac` parser locations are not identity.
- Expression normalization uses an explicit iterative post-order worklist with
  active/finished state, deterministic operand-before-user order, and no C++
  call-stack growth per expression level.

## Focused verification

- `cmake --build .pycircuit_out/toolchain/build --target acir-opt-internal acir-queue-plan acir-queue-cxxgen acir-queue-pycgen acc ACIRModelAnalysisTests -j4`
  - Result: passed with the current checkout's LLVM/MLIR 22 build.
- `.pycircuit_out/ac-venv/bin/lit -j 4 --filter='Transforms/rule-.*' .pycircuit_out/toolchain/build/compiler/acir/tests/mlir`
  - Result: 23/23 passed.
- `.pycircuit_out/ac-venv/bin/lit -v --filter='ACIR/firing-invalid' .pycircuit_out/toolchain/build/compiler/acir/tests/mlir`
  - Result: 1 passed.
- `.pycircuit_out/toolchain/build/bin/ACIRModelAnalysisTests --gtest_filter='ACDataFlowAnalyzerTest.ExactRuleEffectSummaryRejectsCategoryPreservingTampering:ACDataFlowAnalyzerTest.InfersOrderedStateAccessFootprints'`
  - Result: 2 passed.
  - Negative coverage changes `i` to `j`, constant `3` to `4`, and predicate
    `p` to `q` without changing derived categories. It also rejects cyclic or
    dangling references, wrong result width, forged all-entry form, and
    owner/field/endpoint mutation.
- `rule-cse-footprints.mlir` emits twice and compares the bytes with `diff`.
  - Result: passed as part of the focused lit set.
- `rule-exact-effect-order.mlir` lowers A/B/C and C/B/A source permutations,
  reparses each emitted file, and compares clean emission byte-for-byte.
  - A writes `field0`, B writes declaration-disjoint `field1`, and C writes
    `field0`; A/C carry accepted owner-local priority ranks 0/1.
  - Both permutations retain identical exact index root 2 and predicate root 1
    for every firing, the same field/arbitration conclusions, endpoint source
    provenance, and rule-level NDF IDs/requirements.
- `rule-snapshot-set-lowering.mlir` rejects tampering of match yield/predicate,
  choose key yield/policy/stable identity, producer-lane reference, and exact
  read fields.
- `rule-slot-release-lowering.mlir` proves exact Slot read/release footprints
  and rejects access/field tampering.
- `ExactRuleEffectSummaryUsesIterativeDeepNormalization` proves a 2048-node
  expression chain normalizes with the explicit worklist.
- Full ACIR lit after rebuilding every consumer tool: 207/213 passed. All
  F1-owned rule, firing, runtime-row, module-freeze, Slot, match/choose,
  print/parse, lexical-owner, provenance, and codegen tests pass. Four remaining
  failures are the
  pre-existing D0270 High-ACIR publication boundary in
  `acc-python-driver.mlir`, `acc-package-driver.mlir`,
  `acc-composite-package.mlir`, and `acc-composite-fanout-package.mlir`:
  `acc.py` aborts before producing its source unit with
  `ACIR-EMIT-001: verified ACIR requires topology closure`. Those tests and
  commands are present unchanged at F0 revision `2b85fe15`; repairing the
  High-ACIR source-unit publication boundary is outside F1 exact summaries.
- The other two failures are also restored, unchanged F0 expectations as
  required by review: `acc-driver.mlir` expects raw input rejection while the
  current ACC accepts and lowers it, and `variable-module-state-lowering.mlir`
  expects old root translation-unit includes while the current bundle emits
  `generated/dut.h`. F1 does not change either D0270 surface.
- Full `ACIRModelAnalysisTests`: 35/35 passed.

## Review status and remaining Decision 0271 work

- Independent code-reviewer verdict: **PASS** — exact match/choose semantics,
  lexical Table/Slot ownership, lane-rooted fields, endpoint-first provenance,
  iterative normalization, and strict tamper rejection satisfy the F1 contract
  without a compatibility fallback or F2 behavior.
- Independent verifier verdict: **PASS** — focused rule lit is 23/23, full
  native analysis is 35/35, deterministic print/reparse and hostile tamper
  regressions pass, and every remaining full-lit failure is documented outside
  the F1 semantic boundary.
- P2/F2 whole-design rule effect graph, JSON/DOT views, and reuse-reporting for
  writer/value-constraint proofs are not implemented here.
- The decision remains `gap-in-scope`; no F2-or-later checklist item is marked
  complete by this evidence.
