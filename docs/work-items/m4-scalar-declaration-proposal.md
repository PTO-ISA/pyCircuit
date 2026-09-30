# M4 scalar declaration projection — design packet

Status: design complete and approval-ready; awaiting precise user approval.
Product implementation is not ready/authorized by this review.
Planning base: `3cf4cf43`. Product base: `a3df1ade`.

User requested the next step after the previous repair. This packet makes the
missing final declaration projection and C++ header mapping concrete for review;
it does not implement an unapproved serialized IR expansion.

## Ownership and authority

- PM owns docs/rfcs/migration/c2-decl-scalar-final.md, this packet, review archive
  and ledger integration. Preserve pre-existing unrelated PM/proposal files.
- Architecture advice: other_agent_arch_review, Astra xhigh, read-only.
- Code facts: declaration_ir_map, explorer, Astra low, read-only; no builds or
  writable reproducer from that role.
- Independent design reviewer: declaration_design_review, separate architect
  instance, Astra xhigh; no authorship or edits.

C2/C3/N1 approval covers the existing primitive vocabulary and declaration-header
goal. The narrow delta requiring precise approval is final declaration-unit
placement/selection/admission plus native C++ declaration mapping. No new
primitive, public CLI/runtime/schema field or SYSTEM/EXPECT B adoption is allowed
under this design packet.

## Design completion criteria

- Exact before/after envelope and declaration attributes, canonical authority,
  unused/private declarations, empty units/facades and unsupported category policy.
- Full MathInt preservation in common IR, exact native C++ scalar carrier mapping
  and target-only range rejection without truncation or tag substitution.
- Shared final verification and fresh-process behavior, with honest source
  completeness/authentication limits and no source/header readback.
- Source-owned header-only groups, collisions/ODR, concrete implementation
  checklist and independent validation commands, all future tests marked planned.
- Independent proposal-hash-bound approval-ready review, lint, durable archive
  and user-visible precise approval request. No implementation acceptance claim.

## Validation boundary

The existing source-parts harness parsed the proposal's declaration-unit fragment
and reached final envelope rejection (missing outer package entry), as expected
for a fragment. This checks spelling of existing ops/properties; it does not
validate the proposed final admission or count as a new final verifier test.
Product files and the native build are unchanged. Only documentation checks are
required for this packet.

## Frozen design result

C2-DECL revision A is independently approval-ready at SHA-256
`38dd31d13cff150cf7b778e9c3df469f9ab1e0b55c8b2311f7c8d05d49b736b8`.
[Review archive](../reviews/20260930-c2-decl-scalar-final-design-review.md).
The proposal itself contains the exact D1–D5 implementation checklist and §8
verification matrix; all implementation gates remain planned. No approval
record is created until the user explicitly approves these reviewed bytes.
