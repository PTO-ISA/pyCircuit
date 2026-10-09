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
review of the changed content. Record the review under
[`docs/reviews/README.md`](../../../docs/reviews/README.md).

## Specialization and example acceptance

Do not add a specialization pipeline or design-recipe compilation path. The
user's exact policy for parameterized definitions and concrete structural
elaboration remains pending; this constraint does not itself ban parameterized
historical examples or establish support for nonempty static arguments. Record
the unresolved policy explicitly rather than choosing it through implementation.

The user ended migration expansion on 2026-10-07. Keep the retained supported
examples and API tests in `examples/catalog.json`; remove uncovered historical
drafts instead of reopening migration work. Removal never counts as a passing
result. Long oracle and coverage matrices use the existing nightly entrypoints.
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
