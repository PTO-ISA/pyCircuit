# M5 source-map independent design review

Date: 2026-09-30. Reviewer: native instance `/root/m5_map_design_review`,
gpt-6-astra / high. Author: `/root/decl_arch_conformance`, gpt-6-astra / xhigh,
with PM editorial consolidation. Review was read-only; no implementation tests.

Proposal: [C3-SM](../rfcs/migration/c3-source-map.md).
Implementation reference: `a21bb596eaae8ff9090baa48fb3e51a42a2a954c`.

Revision A SHA-256:
`572f0536c9152a57f7f14aa7bb2a53ff842321863f7f348c27c4d5679e685f7e`.
Verdict: revise. Missing explicit Decision 0250/0266 supersession boundary and
executable gate commands. Precision limitation itself was clear and feasible.

Revision B SHA-256:
`3b0bf687a29835bc9cdfb3039ad9f3c671f0e63040260410ab0fe40a65e8b13f`.
Verdict: **approval-ready**. Both findings resolved. The decision delta identifies
replaced map guarantees while preserving supported compiler provenance retention
and naming rules. Commands distinguish planned tests from existing evidence and
preserve M4 behavioral oracles. Retained-tuple inventory completeness,
deduplication, cross-target equality and structural validation are testable;
generated-line precision and reconstruction of absent provenance are excluded.

This is approval readiness, not user approval or implementation acceptance.
The user subsequently approved this exact revision in the current conversation;
see [approval record](../rfcs/migration/approvals/c3-source-map.md).
