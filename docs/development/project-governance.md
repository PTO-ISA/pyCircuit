# Project governance

pyCircuit is a hardware programming language and compiler with one Python
capture → MLIR analysis/lowering → verified common IR → C++/Verilog flow.
This governance replaces the former migration-stage delivery process. Current
support belongs in the [language reference](../reference/language.md); exact
technical approvals remain authority for semantics, not evidence of a passing
implementation. Historical migration schedules do not constrain new work.

## Scope and ownership

Follow the user's latest direction and existing authorization. Continue scoped,
reversible local work without repeated permission handoffs. Identify genuinely
new semantic interfaces with a concrete proposal and independent review before
implementation. Do not infer authority for posting, pushing, merging or changing
assignments/projects from a local development task.

Use the [repository-management flow](repository-management.md) and checked-in
Issue/PR templates for all subsequent submissions. Search related work, identify
dependencies and assign bounded ownership. Each implementation task specifies
inputs, exclusive files, dependencies, expected behavior, removal scope and the
smallest useful checks. The integration owner handles shared registries, build
metadata, candidate identity and final reporting. Preserve others' changes and
verify the actual worktree root before staging or committing.

## Independent review

Work directly for small mechanical tasks. Complex semantic changes require a
real architect and an independent decomposition instance before bounded
implementation. Implementation, independent tests and review use separate
instances. Record actual role, model and effort. The requested implementation
and code-review default is `gpt-6.1-sol`; use the real architect preset for
architecture. A role label does not establish that review occurred.

Use the design-review skill for unresolved proposals and the PR-review skill
for implementations. [Review and merge](review-and-merge.md) owns candidate
identity, full feedback inventories and readiness decisions. A clear review
cannot substitute for tests, approvals or merge authority.

## Implementation boundaries

Use the [affected-layer checklist](contributing-workflow.md#trace-a-change-through-its-owners).
Keep capture thin and infer semantics from declared types, actual SSA, effects
and owners in existing MLIR passes. No fixture-specific admission, alternate
semantic compiler, compatibility fallback or backend-only patch may bypass the
common verified IR. Preserve hardware timing, four-state behavior, ownership,
whole-system atomicity and generated System responsibilities.

Keep DUT sources focused on algorithm, state and connections. Independent oracles
belong to tests; consumer designs, ISA logic and adapters belong to consumer
repositories. Frozen technical approvals are not rewritten as implementation
progress. Current behavior and limitations are maintained in active docs.

## Validation and delivery

Build each candidate in its own checkout. Reuse existing API/example entrypoints
and [test tiers](testing-and-gates.md). Long reference, coverage and mutation
matrices stay in nightly and respect explicit deferrals. Keep meaningful
ownership, rejection, timing, reset, output-protection and zero-commit-on-failure
oracles; a removed or unexecuted case is not a pass.

Record commands, exit status, exact revision/content and accessible evidence in
the PR and ignored local `docs/gates/logs/<run-id>/`. Tests must not depend on
management logs, review transcripts or acceptance counters. Preserve required
complete stage artifacts through existing manifests. Recheck remote identity,
feedback and required checks before delivery; keep remaining work explicitly
open. The versioned merge requirements live in
`.github/repository-governance.json` and must match GitHub settings.
