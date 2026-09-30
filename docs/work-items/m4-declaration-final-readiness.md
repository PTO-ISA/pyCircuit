# M4 declaration-source final retention readiness

Status: design gap identified; no implementation or new schema approved here.
Read-only review: other_agent_arch_review, gpt-6-astra/xhigh, 2026-09-30,
against product base `4fbf7996` and approved planning-branch C2/C3/N1.

C3 already requires declaration-source headers and SourceOwner grouping.
Existing ac.type_alias, ac.constant, ac.struct and their source ownership are
approved vocabulary; this work does not justify adding another primitive.

The missing detail is the exact serialized-final declaration-unit envelope and
projection. Current final materialization drops declaration-only units and
aliases/constants; the final closure admits implementation units containing
one module. C2's declaration-unit table describes the source stage. C2 requires
helper executable definitions to disappear, and N1 requires ac.exports and
ac.import_bindings to disappear. Keeping complete source headers or reading
Python/header files again during emission would violate those boundaries.

The next bounded M4 design packet should freeze bool/integer aliases and scalar
constants only, using existing vocabulary, with exact answers for:

- Final placement/container fields and the allowed declaration operations.
- Selection of canonical definitions from verified owning headers, with
  original SourceOwner, duplicate handling and deterministic ordering.
- Handling of unused declarations, empty declaration sources and pure facades.
- Removal of source-only exports, bindings, helpers and unresolved references.
- Shared final verification/reparse and source-owned header emission, without
  implementation TUs for declaration sources.

This narrow serialized-IR projection needs an independently reviewed precise
contract before implementation; the C3 delivery goal itself needs no repeat
approval. SYSTEM/EXPECT revision B is a separate issue and remains unchanged.

Acceptance after approval: declaration source + two implementation sources;
save final and remove source/header inputs; both backends still accept that
same final, owning declaration headers are reproducible, and no extra cpp is
created for declaration-only sources. Invalid owner, duplicate authority and
illegal source-phase residue must reject. No sidecar, fake implementation
module or backend semantic authority is introduced.

## Concrete proposal — 2026-09-30

The gap now has an independently reviewed [C2-DECL revision A proposal](../rfcs/migration/c2-decl-scalar-final.md).
It is approval-ready at SHA-256
`38dd31d13cff150cf7b778e9c3df469f9ab1e0b55c8b2311f7c8d05d49b736b8`,
with a separate [review archive](../reviews/20260930-c2-decl-scalar-final-design-review.md).
This design completion does not approve or implement the new final admission.
