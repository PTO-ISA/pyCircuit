---
name: pycircuit-design-review
description: Independently review pyCircuit designs and unresolved interface proposals for generic common-IR coherence, hardware semantics, hard-break completeness, bounded testability, and readiness under existing user authorization.
---

# pyCircuit design review

Read the proposal, `AGENTS.md`, applicable sections of
[`docs/development/project-governance.md`](../../../docs/development/project-governance.md),
the active
[`docs/development/source-unit-workflow.md`](../../../docs/development/source-unit-workflow.md),
and affected decisions and exact records in `docs/rfcs/contracts/approvals/`.
Resolve stale guidance against the user's latest clear direction; existing
approval persists for its exact scope and does not prove implementation.

Use an instance independent of the design author. Do not edit the proposal or
implementation being reviewed. Check exact before/after interfaces, types,
timing, ownership, failure/commit behavior, generic MLIR inference and insertion,
common-IR mapping for both backends, callers, deletion scope, independent
oracles, rejection cases, and gate commands. Reject recipe recognition,
whole-system compile followed by source splitting, backend-only semantics,
parallel semantic compilers, and retired-route fallbacks.

Check the single frontend -> generic MLIR passes -> common verified hardware
IR -> gfsim C++ / Verilog target, unified `pycircuit` naming, exact Python-source
basenames, and coverage of retained supported examples. Distinguish target
requirements from actual current coverage and cutover sequencing. Do not invent
supported system, queue, port, static-parameter, or record execution APIs from a
proposal or partial verifier slice. Report discrepancies immediately.

Return `approval-ready`, `revise`, or `blocked`, concrete findings, the exact
reviewed content hash, actual role/model/effort, and applicable existing authority.
`Approval-ready` is a design-review result, not approval, implementation, or
acceptance. Already-authorized local work proceeds within that scope; genuinely
unresolved interfaces are identified for a concrete decision rather than a
redundant permission handoff. Material design changes require independent
review of the changed content. Record the review with the candidate evidence
following [the local evidence policy](../../../docs/gates/README.md).

## Specialization and example acceptance

Do not add a specialization pipeline or design-recipe compilation path. The
user's exact policy for parameterized definitions and concrete structural
elaboration remains pending; this constraint does not itself ban parameterized
historical examples or establish support for nonempty static arguments. Record
the unresolved policy explicitly rather than choosing it through implementation.

Use the current user-assigned migration scope and `examples/catalog.json`;
do not turn the historical 93-root inventory into a current acceptance count.
Removal or deferral never counts as a passing result. Long coverage and oracle
matrices use the existing nightly entrypoints when requested or scheduled.
Local migration logs/work packets/reviews are ignored, not test dependencies.

Use descriptive compiler responsibility names. The
checked-out owning source tree is `compiler/` with `compiler/include/pycircuit/`;
native roles are `pycircuit-source-unit`, `pycircuit-link`, and `pycircuit-emit`,
with test-owned `pycircuit-backend-test`. Their corresponding environment names
are `PYCIRCUIT_SOURCE_COMPILER`, `PYCIRCUIT_LINKER`, `PYCIRCUIT_EMITTER`, and
`PYCIRCUIT_BACKEND_TEST`. They serve the single public frontend and driver.
ACIR/`ac` remain technical IR names and `pycircuit::pyc6_runtime` remains the
current shared Runtime export. Do not infer generic pass decomposition or expanded
capability implementation merely from this naming/path cutover.

## Hardware-first frontend scope

The Python frontend is a hardware design language: support a practical subset
with explicit finite hardware meaning, not arbitrary Python program behavior.
Generic MLIR passes operate on declared types, actual SSA, effects and owners.
Hardware-oriented frontend constructs are legitimate; fixture names, widths,
constants or recipe shapes do not determine admission. Preserve prior hardware
behavior in rewritten examples and repair actual framework gaps through shared
IR semantics; do not grow an unrelated Python interpreter to claim genericity.

## Transaction and observation review

Use [the serial extension specs](../../../docs/rfcs/serial-rule-method/README.md)
for Rule/Method boundaries, guard entry snapshots, ordered validity, source-plan
authentication and common final verification. Distinguish local false-guard hold
from reached unknown-control failure and whole-system zero state/history commit.
Public straight-line Method evidence does not close the bounded branch slice.
Review branch candidates against
[`branch-state-ssa.md`](../../../docs/rfcs/serial-rule-method/branch-state-ssa.md):
both arms inherit one entry candidate, data and raw intent use four-state joins,
the effective E is finalFire AND rawIntent, identity writes retain intent, and
the `{state,data,enable}` hard break is complete. Require external fire zero-use,
actual Rule parsing for every query, branch source replay, independent final
D/E proof, generated Method System workers1/2 and real four-state rollback.
Review conditional calls/effects against
[`conditional-effects.md`](../../../docs/rfcs/serial-rule-method/conditional-effects.md):
unselected guards bypass; selected false guards cancel the outer transaction;
reached unknown effect selectors preserve the sampled failure contract. Authenticate
source paths and result availability before joins, and prove each effect's final
fire AND selected path independently. Distinguish pure four-state joins from
effect selection-known checks. The six-field sampled schema now uses closed
`controls` kind/path records; do not accept retired `guards` inventories.
This does not establish partial state, resources or changed aggregate joins.
For body checks/observations,
inspect the six-field inventory,
statement-time operands, final-prefix paths, exact terminal slots and both proof
layers. Require generated Method `@system` C++ workers1/2, ordered source-derived
identities, failure publication protection and real four-state RTL evidence.

Review observation failures before commit, not only absence of emitted records.
Incomplete active groups must reject before Xfer/COMMIT while false-path and
all-invalid groups preserve their documented inactive behavior. Check the actual
snapshot, prior events/gauges and epoch. Native X/Z value-plane tests and real
four-state RTL serve different obligations; two-state simulation cannot replace
the latter. Output sink I/O failures after commit cannot promise rollback.

Analysis reuse must preserve exact definition/bindings/occurrence identity and
complete private boundaries. Cache only successful facts within immutable IR
lifetimes; transformations invalidate them. Budget fixes remove repeated work,
not checks, input scale or limits. Reused immutable attribute identity does not
replace parsing and validating the actual Rule. Review raw prepayment against actual verifier
traversals before accepting a larger scope.
