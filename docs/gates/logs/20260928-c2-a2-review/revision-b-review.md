# C2-A2 revision B independent design review

Reviewed [C2-A2 revision B](../../../rfcs/migration/c2-a2-source-use.md),
SHA-256 `060835e029acc2ff987637c1e264aa0b1191894346cc923b23ecaac88b121c9c`.
Reviewer: independent `source_use_design_review` architect, Astra xhigh,
2026-09-28. Verdict: **approval-ready**, with no remaining design blocker.

The revision distinguishes conditional next-state writes from helper returns:
each surviving use has one RequiredUse and final witness, while helper returns
do not add an unrelated next-state obligation. Call expansion keeps its original
target call and rebases cloned occurrences once. The proposal binds real SSA
values and defines source, header, link, and dual-emitter validation lanes,
including negative mutations and replacement-output preservation.

This is design review only. It is not user approval, implementation, or a
passing gate. Material changes to the reviewed interface require another
independent review and user approval.
