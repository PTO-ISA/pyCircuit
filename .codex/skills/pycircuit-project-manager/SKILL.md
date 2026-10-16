---
name: pycircuit-project-manager
description: Manage pyCircuit planning, issue/PR delivery, bounded ownership, independent tests/review, integration and candidate acceptance using existing user authorization and repository contracts.
---

# pyCircuit project manager

Read `AGENTS.md` and [project governance](../../../docs/development/project-governance.md).
Use the user's latest scope and existing authorization. Continue bounded local
work without repeated confirmation; proposals do not establish new semantics.

## Route the work

- Issue intake, existing ownership, dependencies, forks and delivery:
  [repository management](../../../docs/development/repository-management.md).
- Cross-layer implementation and pass/performance evidence:
  [contributing workflow](../../../docs/development/contributing-workflow.md).
- Current source/compiler responsibilities:
  [language reference](../../../docs/reference/language.md) and
  [source-unit workflow](../../../docs/development/source-unit-workflow.md).
- New semantic interfaces: applicable exact contracts and independent
  [`pycircuit-design-review`](../pycircuit-design-review/SKILL.md).
- Implementation review and current-head feedback:
  [`pycircuit-pr-review`](../pycircuit-pr-review/SKILL.md) and
  [review and merge](../../../docs/development/review-and-merge.md).

## Own the delivery

Work directly for small tasks. For complex semantic work, use the real architect
and an independent decomposition instance before dispatching bounded execution.
Each task fixes inputs, exclusive files, dependencies, expected behavior, removal
scope and focused checks. Integrate shared registries/build metadata centrally;
implementation, independent tests and review remain separate as required by
project governance. Record actual roles/models/effort rather than planned labels.

Identify open dependencies and competing work before editing. Keep task-owned
changes separate, verify the actual Git worktree root and PR head, and reuse an
existing matching PR. No assignment, comment, push or merge is implied by simply
invoking this skill; preserve the user's authorized delivery scope.

Use existing [test tiers](../../../docs/development/testing-and-gates.md) and
[local evidence policy](../../../docs/gates/README.md). Bind actual commands,
outputs and independent reviews to the exact candidate. Report deferred or
unavailable checks honestly; never turn reduced coverage into a pass. Preserve
incomplete work recoverably and keep follow-up scope open. Recheck remote head,
feedback and required checks before reporting an authorized delivery complete.
