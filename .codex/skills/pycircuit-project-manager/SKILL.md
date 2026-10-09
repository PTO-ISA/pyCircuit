---
name: pycircuit-project-manager
description: Manage pyCircuit work with bounded ownership, architecture/decomposition roles, independent tests/review, persistent user authorization, and candidate-bound evidence. Use for planning, dispatch, integration, or acceptance in this repository.
---

# pyCircuit project manager

Read `AGENTS.md`,
[`docs/development/project-governance.md`](../../../docs/development/project-governance.md),
the active
[`docs/development/source-unit-workflow.md`](../../../docs/development/source-unit-workflow.md),
and affected exact approval records in `docs/rfcs/contracts/approvals/`.
The user's latest clear direction and existing authorization govern the assigned
work; do not request redundant approval for already-authorized local changes.
Identify genuinely new unresolved semantic interfaces as precise proposals and
obtain independent review where needed before treating them as established
contracts. A reviewed plan or an approval record is not implementation evidence.

Target one Python frontend, generic MLIR analysis/insertion/lowering passes,
common verified hardware IR, and gfsim C++ / Verilog from that IR. Enforce
NO HARDCODE and NO SHIM. Current `pycircuit compile/link/emit`, source-owned
`.ac` groups, current typed module/standard-leaf/Work-Xfer profile and Runtime
export remain current until exact cutover. The portless/default-clock/empty-static
source-unit cutover profile is historical; current admission is in the active language reference.
Naming,
full capability coverage, exact Python basenames, and flat one-design example
migration must be verified rather than inferred from the target. Preserve retained supported examples and hardware oracles. Report detected
discrepancies immediately and assign concrete corrective work.

For complex changes use the real `architect` preset and a separate decomposition
instance. Dispatch small tasks with inputs, exclusive files, dependencies,
expected hardware behavior, gates, and removal scope. Executors implement the
packet rather than redesigning around test failures. Keep architecture validation,
implementation, independent tests, and code review separate where required.
Record actual role/model/effort and the requested implementation/review default;
never claim a review occurred merely because a role was planned.

Own readiness, integration, candidate identity, evidence, and acceptance. Remove
obsolete recipe/duplicate tests while preserving ownership, range/type,
current/next, hold/discard/reset, zero-commit-on-failure, source-unit, and output
protection oracles. Acceptance matches exact tested scope; missing capability
cannot be supplied through a retired route.

Follow the [local evidence policy](../../../docs/gates/README.md). Keep task
packets, independent reviews and candidate-bound evidence together under
`docs/gates/logs/<run-id>/`. Historical local work/review directories remain ignored. Report commands, status,
evidence, skips, and remaining callers. Keep this project skill local; do not
modify OMX state as part of ordinary project management.

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
