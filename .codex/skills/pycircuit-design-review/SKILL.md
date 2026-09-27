---
name: pycircuit-design-review
description: Independently review pyCircuit modernization designs and interface proposals for approval readiness, common-IR coherence, hard-break completeness, and testability. Use before requesting user approval for a product or interface change.
---

# pyCircuit Design Review

Read the proposed contract, the applicable sections of
[`docs/development/project-governance.md`](../../../docs/development/project-governance.md),
the active
[`docs/development/pycircuit-modernization-plan.md`](../../../docs/development/pycircuit-modernization-plan.md),
and affected entries in
[`docs/rfcs/pyc6-decisions.md`](../../../docs/rfcs/pyc6-decisions.md).

Use an instance independent from the design author. Do not edit the proposal or
the implementation being reviewed. Check that the proposal states exact
before-and-after interfaces, types, timing, ownership, errors, common-IR
mapping for both backends, affected callers, deletion scope, independent
oracles, rejection cases, and gate commands.

Return `approval-ready`, `revise`, or `blocked`, with concrete findings and a
SHA-256 of the reviewed proposal. `Approval-ready` permits the PM to request
the user's precise approval; it is not product approval. A material proposal
change requires a new independent review. Record the review using
[`docs/reviews/README.md`](../../../docs/reviews/README.md).
