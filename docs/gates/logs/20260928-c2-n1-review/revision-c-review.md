# C2-N1 revision C independent design review

Verdict: **approval-ready**. R1 closed; no remaining design-review blockers.

Reviewer: namespace_review, independent architect, configured gpt-6-astra/xhigh.
Date: 2026-09-28. Separate from interface_design and PM; no proposal authorship,
implementation, delegation or builds. PM transcribed the independent report.

Reviewed source: docs/rfcs/migration/c2-n1-namespaces.md.
SHA-256: ac7d56a403e21ac03186f17c2f75f9c8eb53a00dcb74677c5286219dc5f1e6c3.
Checkout HEAD: 03760118df71d5c231ae28cd05f2b29fb462be36 plus proposal overlay.
The reviewer compared exact B-to-C diff, rechecked the C hash after inspection,
and confirmed frozen C1/C2/C3 hashes still match recorded approvals.

## R1 closure

- N12 explicitly covers missing mandatory attributes, wrong outer/element types,
  and missing/unknown/wrong-typed nested fields.
- N15 requires unused and later-shadowed bad imports to fail in real sources.
- Positive lowering removes both attrs. Four negatives inject each attr into
  otherwise valid final input and use both real emit targets, checking failure
  before publication and unchanged stable replacement output.
- The independent order oracle fixes Unicode order, numeric AST indices 2/10,
  and header permutation, without narrowing C2 IntegerAttr carrier acceptance.
- Planned structural, source/header, link and final/emit gates have commands,
  case mapping, nonempty discovery/assertion requirements and full evidence.
  Missing, skipped or xfailed obligations cannot close acceptance.

The commands/targets are explicitly planned, not presented as implemented or
passing. Tools must come from the current checkout. Internal native subset
proof cannot replace the C3-driver matrix.

## Architecture and approval boundary

B-to-C changes only the revision marker and validation requirements. There is
no new namespace semantic, CLI/receipt/runtime ABI or alternate route beyond
the already reviewed two-attribute proposal. Namespace and declaration authority
remain distinct; canonical identity, actual provider/name snapshots, mirrors,
explicit headers, cyclic-header/producer-start distinction and final removal
remain coherent.

General C1 scoping questions do not block this bounded named-import addition.
No blanket restrictions on rebinding, private names, unused imports or complete
consistent cyclic headers were introduced. U03/resource implementation is not
required before design approval; eventual full acceptance still requires the
planned gates.

No N1 implementation or product builds/tests ran. Documentation checks reported
by PM were not independently rerun. This verdict permits asking the user to
approve this exact revision C; it is not user interface approval or implementation
authorization. Material text changes require independent re-review.
