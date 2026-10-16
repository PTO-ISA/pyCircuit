---
name: pycircuit-design-review
description: Independently review pyCircuit semantic proposals for common-IR coherence, hardware behavior, ownership, hard breaks and bounded testability under existing user authorization.
---

# pyCircuit design review

Use an instance independent of the design author. Read `AGENTS.md`, the proposal,
[project governance](../../../docs/development/project-governance.md), affected
technical contracts and exact approval records. Check current admission in the
[language reference](../../../docs/reference/language.md) and current owners in
the [source-unit workflow](../../../docs/development/source-unit-workflow.md).
Do not edit the proposal or implementation being reviewed.

## Assess the proposal

Check concrete before/after interfaces, declared types, ownership, timing,
effects, failure/commit behavior and source-unit authority. Trace the intended
hardware through existing MLIR analyses and verified common IR to both backends.
Use the [affected-layer checklist](../../../docs/development/contributing-workflow.md#trace-a-change-through-its-owners)
for callers, registrations, package boundaries, docs and deletion scope.

Reject fixture recognition, fallback compilers, backend-only semantic fixes or
new wrappers that bypass an existing owner. Distinguish proposed capability from
implemented support and from demonstrated execution. Preserve required generated
System and four-state evidence; a smaller fixture or verifier-only change cannot
establish broader hardware behavior.

Require testable acceptance criteria: positive behavior, unsupported/rejection
boundaries, independent oracles, rollback/output protection and applicable gate
commands. For performance or reuse changes, check input scale, semantic checks,
cache identity/lifetime and measured comparison plans. Respect deferred long
coverage; do not turn the review into an unrequested nightly run.

## Report the decision

Return `approval-ready`, `revise` or `blocked`, with concrete findings, exact
reviewed content identity, applicable existing authority, unresolved decisions,
required evidence and actual role/model/effort. Record the review following the
[local evidence policy](../../../docs/gates/README.md).

`Approval-ready` is a proposal-review result, not user approval, implementation
or acceptance. Existing authorization continues for its scope. New unresolved
interfaces need a concrete decision; changed design content needs re-review.
Use [`pycircuit-pr-review`](../pycircuit-pr-review/SKILL.md) for implementation
findings and PR feedback instead of treating design review as code acceptance.
