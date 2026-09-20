# Architecture Rule F2: whole-design rule effect graph

Date: 2026-09-20

Decision: 0271 (F1 + F2 implemented and verified)

## Implemented contract

- `RuleEffectGraph` is a read-only ACIR analysis over verified exact rule or
  firing summaries. It emits deterministic rule, state owner/footprint,
  Queue/Slot resource, conflict, ordering, recovery, arbitration-domain, and
  reserved obligation-linkage nodes and edges.
- Recovery is explicitly `absent` when undeclared. Obligation linkage is an
  explicitly absent F3 slot. F2 does not infer obligations or relax verifier
  rejection.
- `VerifyValueConstraints` and the graph both call
  `analyzeWriterArbitration`. There is one overlap implementation. Its report
  distinguishes field-disjoint, index-disjoint, predicate-exclusive,
  declarative allocation order, explicit priority, unresolved/rejected, and
  cross-owner priority-cycle outcomes with proof provenance.
- `ac-build-rule-effect-graph` accepts optional `json-output` and `dot-output`
  paths. Both views are debug/evidence only, contain no hash/fingerprint/SHA
  identity, and do not modify IR or participate in normal code generation.
- Graph nodes and edges sort by structural names and exact summary content.
  Source and traversal order do not select a winner or alter emitted bytes.
- Conflict/interaction identities use canonical owner stable IDs and both exact
  footprint identities. Compact canonical DAG ranks intern closed structural
  tuples bottom-up and replace local ordinals without recursively expanding
  shared/deep DAGs, so intra-rule effect reordering cannot select a different
  proof or grow output superlinearly.
- Read/read and read/write relations reuse the shared field/index/predicate
  proof primitives and record committed-old-state observation on overlap.
- Owner-specific ordering evidence is independent of global cycle adjacency;
  the same rule pair retains a separate proof record for every owner.
- Accepted/rejected write-pair classification comes only from the shared
  writer-arbitration report. Duplicate rank/identity, unresolved overlap, and
  cross-owner cycles publish only explicit rejected reasons.
- Mixed rule/declarative interactions map the rule-side operation back to its
  exact footprint node and key the declarative side by owner-stable endpoint
  identity. Reordering a rule's overlapping `f0` and disjoint `f1` proposals
  retains separate ordered/coexist records and byte-identical JSON/DOT.
- JSON/DOT publication rejects normalized same paths, prepares both temporary
  files before publication, preserves prior outputs on failure, and escapes
  every DOT control byte.

## Verification

- Built from the current checkout:
  `cmake --build .pycircuit_out/issue126-127/toolchain/build --target acir-opt acir-opt-internal acir-queue-plan acir-queue-cxxgen acir-queue-pycgen acc ACIRModelAnalysisTests -j4`.
- Focused F2 lit (`rule-exact-effect-order.mlir`,
  `writer-arbitration.mlir`, and `rule-slot-release-lowering.mlir`): **3/3
  passed**.
  - A/B declaration-disjoint fields report `field_disjoint` + `coexist`.
  - A/C same-field writers report `explicit_priority` + `ordered`.
  - ABC and CBA JSON and DOT are byte-identical.
  - Dynamic index and predicate exclusions retain the shared
    `ACDataFlowAnalyzer` proof provenance.
  - Unresolved same-field overlap emits a rejected debug record and retains the
    existing fail-closed diagnostic.
  - Cross-owner priority cycles emit `arbitration_cycle_rejected: true` and
    retain the existing rejection diagnostic.
  - Queue and Slot resources, Slot read/release footprints, absent recovery,
    and absent obligation-linkage slots are present.
  - Missing exact summaries reject; existing tamper tests cover malformed or
    live-body-mismatched summaries.
- Relevant rule-analysis lit after rebuilding dependent tools: **26/26
  passed**.
- Review expansion adds intra-rule effect-order byte comparison, read/read and
  read/write overlap/disjoint permutations, duplicate lexical Table/Slot
  symbols, multi-owner ordering, normalized same-path rejection, atomic
  publication failure, and exact DOT control-byte escaping.
- `ACIRModelAnalysisTests`: **36/36 passed**, including
  `RuleEffectGraphTest.DotEscapesEveryControlByte`.

## Boundary and review status

- No backend semantic or runtime file changed. The normal lowering/codegen
  pipeline does not invoke the graph pass.
- F3 obligation IR and F7 recovery identity remain separate work. Their graph
  positions are explicit absent/reserved slots, not guessed semantics.
- Independent code-reviewer verdict: **PASS**.
- Independent verifier verdict: **PASS**.
- Decision 0271 and PC-F2 are verified. No F3-or-later status is promoted here;
  Architecture Obligation IR remains separate Decision 0272 work.
- `ledger-snapshot.json` is the review-time F2 snapshot. The active authority
  remains `.omx/ultragoal/checklist-ledger.jsonl`; historical F0/F1 evidence is
  unchanged.
