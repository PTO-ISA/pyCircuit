# Review-followup batch — R1–R5 evidence (2026-09-29)

Scope: the five corrections requested by the DeepSeek review follow-up
(`docs/work-items/deepseek-review-followup-m1-m7.md` in the planning checkout),
plus the S1 stage analysis and the first-class-system slice preparation. This
batch is documentation and tests only: it does **not** implement new IR, CLI or
manifest schema, and it does not change compiler semantics.

## Baseline and candidate

| Item | Value |
| --- | --- |
| Implementation baseline | `6696eb3f1778b710eca3d6b4a167e4ad1ca05a54` |
| Planning baseline | `b7d0dd3d` (plus the other agent's uncommitted proposals) |
| Dirty overlay | the four files in `overlay-sha256.txt`, carried by commit `7e660760` (with the hash-list header dropped in `53855ccb`) |
| Build directory | `.pycircuit_out/w10-pm/build` (single integrator, no concurrent rebuild) |
| Toolchain | `toolchain.txt` (Apple clang 21, LLVM/MLIR 22.1.8, Icarus 12.0, pytest 9.0.2) |

## R1 — approval-status sync

`docs/reviews/20260929-system-role-expect-design-review.md` (planning) now records
both **revision B** proposals with their exact hashes, their independent-review
provenance (**no reviewer artifact is archived**, so the record does not label
them approval-ready) and, separately, their user-approval status (**not
approved**). The stale "C2-SYSTEM revision A, review pending" text is
gone. The record states that `docs/rfcs/migration/approvals/` contains only c1,
c2-c3-foundation, c2-n1 and c2-r1-m1, and that no approval may be inferred or
back-filled. It also records the known conflict between the pending proposal §5
(role is orthogonal to root kind) and the current private `@system ⟹ testbench`
rule.

## R2 — link guard guarantee narrowed, capability limit recorded

- `acir-design-harness.cpp` comment now says the guard guarantees only that the
  **shared final IR can be rebuilt by the emit path**, explicitly not that both
  backends can emit every shape, and that link must not call the emitters to
  widen it. The diff is comment-only (10 lines, no code change).
- New regression
  `test_source_design_bridge.py::test_multi_value_observation_is_a_known_backend_capability_limit`
  reproduces the reported case and asserts the exact per-backend diagnostics:
  `C++ emitter supports scalar observations only` and
  `RTL supports zero or one local observation value`, each with rc != 0, no new
  backend file created, and a pre-existing output left byte-identical.
- The limit is assigned to M3/M4 as a deferred capability gap inside the
  coverage note's "What remains open" list, and is not claimed as completed.

## R3 — link-and-emit now really emits

`test_masked_next_register.py::test_supported_shapes_still_link_and_emit` now
covers the three declared shapes (unconditional, enable-guarded,
conditional-comparison), each compiled from real Python, linked to its own
`counter.ac`, and then emitted by **both** backends to per-case, per-target
paths, asserting exit 0, output exists and output size > 0. The 256-state oracle,
reset/commit-edge checks and the proof/use/yield rejection cases are unchanged.

## R4 — path-alias test now reaches the rollback

`test_source_design_bridge.py::test_glue_output_path_alias_still_publishes_nothing`
passes the second destination as a **raw string** (`<tmp>/./aliased.sv`) so argv
spellings differ; it asserts the strings differ before the call, expects
**rc=1** (execution failure, not the rc=2 argument guard), requires
`cannot create output` and forbids `must differ` in stderr, and asserts the
primary was rolled back, the secondary does not exist and an unrelated sentinel
file is unchanged.

Negative control run during this batch:

| argv spelling | rc | diagnostic | primary rolled back |
| --- | --- | --- | --- |
| raw `<tmp>/./aliased.sv` | 1 | `cannot create output` | yes |
| normalized (same string) | 2 | `must differ` | yes |

This shows the previous test never reached the rollback, and the fixed one does.

## R5 — plain assignment reclassified

The planning ledger section and
`docs/gates/logs/20260929-n0-u1-masked-next/numeric-shape-coverage.md` now record
`state = other` / `state = 7` as a **serialized-final reconstruction gap under the
existing assignment contract** (`FinalUses.cpp:16-65`,
`ACIRFinalContracts.cpp:437-450`), not as a new language decision. The private
bridge's link rejection is documented as a conservative capability limit, not the
fix; plain assignment is not retired; no further user decision about whether
plain assignment is allowed is required. A follow-up packet is defined that must
first freeze the source/final classification invariants with independent
counterexamples. The three `hasNumericInventory` implementations were **not**
touched.

## Verification results

| Lane | Tests | failures | errors | skipped | disabled |
| --- | --- | --- | --- | --- | --- |
| `focused.xml` (two system files) | 54 | 0 | 0 | 0 | — |
| `system.xml` (five files, `-k 'not v44'`) | 81 | 0 | 0 | 0 | — |
| `final-native.xml` (`ACIRFinalProgramTests`) | 51 | 0 | 0 | 0 | 0 |
| `proposal-native.xml` (`ACIRProposalContractsTests`) | 9 | 0 | 0 | 0 | 0 |

Selector inventory of `system.xml`: `test_masked_next_register` 13,
`test_source_design_bridge` 41, `test_source_numeric_next` 6,
`test_unified_register_backends` 10, `test_v41_v42_source_fixtures` 11.
The two V44 cases are explicitly deselected and DEFERRED to M6; they are not
counted as passing. The old "76 PASS" figure is a baseline, not a target; the
count moved because R3 and R2 added cases.

## Not run / not covered

- V44 real parallel scheduling and source reorder: deselected, M6.
- Multi-value observation emission: not implemented; recorded as a capability
  gap with a regression that pins the current diagnostics.
- Cross-file helper `ac.expect` location conformance: current final verifier
  still requires `location.path` to belong to the owning module source owner;
  reported as an implementation limit, not delivered.
- Editing the three `hasNumericInventory` implementations: out of scope.

## Interface changes

None. This batch changed no `.td`, no compiler library source, no runtime, no
public CLI, no manifest schema, and no approval record. The only product file
touched is the private development tool `acir-design-harness.cpp`, and only in a
comment.

## Independent review

Reviewer: a separate reviewer instance in an independent context, not the author
of any file in this batch. Verdict on the first candidate: **FAIL**, triggered by
one overstated claim in this packet plus one unmet R2 checklist item; R2–R5 were
otherwise verified clean, R3/R4 were proven non-vacuous by bisect and by the
normalized-spelling control, and R5's cited code was confirmed to support the
reclassification.

| # | Finding | Severity | Resolution |
| --- | --- | --- | --- |
| 1 | This packet claimed the multi-value observation limit was "listed as a later M3/M4 capability gap", but no deliverable assigned it to a milestone | medium | `numeric-shape-coverage.md` "What remains open" now carries an explicit item deferring multi-value observation emission to M3/M4, with the pinning regression named, and the erratum log records it |
| 2 | The review record's `approval-ready` cells contradicted both proposals' own "pending review" status lines and named no reviewer instance or verdict artifact | medium | the record no longer labels either proposal approval-ready: it registers "user states it passed; no review artifact archived", cites the contradicting status lines, and adds a "review archival gap" section listing what must be filed before the label may be used. The slice plan and this packet were corrected the same way |
| 3 | The record attributed the revision-B hashes to a "handoff record" that contains no such hashes | low | the wording now attributes them to the user's task statement and explicitly says that consistency is not a repository artifact and is not review evidence |
| 4 | This packet's overlay table was stale after the overlay was committed | low | the table now names the carrying commit `7e660760` and the follow-up `53855ccb` |

### Re-verification verdict

The same independent reviewer instance re-verified the fixes on implementation
`3084b9d5` / planning `c9dfdc5b` and returned **PASS**: all four defects closed,
no false claims remaining, and all five R items CLOSED. It independently confirmed
that the fix round changed documentation only (`git diff --name-status` shows
three docs on the implementation side and two on the planning side), that the
three code/test review targets stayed byte-identical (`84dc5d5f…`, `0b28181f…`,
`3e39b7dd…`), that the re-pinned overlay verifies 4/4, that no deliverable still
labels either revision-B proposal approval-ready, and that it re-ran the focused
lane green (54 passed).

Two LOW cosmetic residuals it reported are also fixed: the review record's header
now reads "同日登记两份修订 B 的审阅状态" instead of implying archived
conclusions, and this document's R1 paragraph no longer duplicates a word.

**This packet is accepted for the items it claims, on candidate `3084b9d5`
(implementation) / `c9dfdc5b` (planning).** Acceptance covers R1–R5 only; it does
not mark M4, M5 or M7 done.
