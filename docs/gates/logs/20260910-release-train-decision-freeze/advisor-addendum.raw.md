# Independent release architect raw addendum

The independent advisor returned the following focused clarification after the
semantic reviewer identified missing #23/#25 coverage.

## Multidimensional Table canonicalization

**Selected:** a Table has a non-empty static shape, row-major storage with the
rightmost axis varying fastest, per-axis bounds, a typed versioned
initialization image, and masks over the row-major projected domain.

**Rejected:** backend-dependent layout, initialization that infers shape from
values, producer-dependent images, and partial mutation after bounds/image
failure.

**Fail closed:** static bounds fail verification; dynamic bounds and invalid
images fail before proposal formation.

## Same-field writer arbitration

**Selected:** same-field writers require verifier proof or a deterministic
high-level policy with stable endpoint identity. Conflict arbitration selects
an explicit branch before resource preparation. A losing candidate never forms
a selected or committed transition and none of its Queue, Table, Reg, Slot, or
output effects publish. Decision 0156's replace-over-field ordering remains the
built-in policy for its admitted single-replace case.

**Rejected:** source or execution-order priority, last-writer-wins, and policies
that bypass type, scope, ownership, or atomicity verification.

**Fail closed:** malformed proof or policy fails before mutation; a losing
candidate reserves and publishes nothing.

## Multidimensional selection index

**Selected:** every `TableChoice.index` is the canonical row-major flattened
unsigned scalar for the full Table domain, so Frozen ACIR retains exactly
`2*N` results. Returning per-axis coordinates directly from choose is rejected.

`TableChoice.coordinates` is **not required** as a public API. A later frontend
may optionally expose it only as a pure static tuple derived from the flattened
scalar by constant row-major division/remainder. It is not an additional choose
result, runtime collection, or persistent state.

**Fail closed:** the verifier checks the flattened domain width. Runtime
collections, rank-dependent choose result arity, and backend-specific index
encodings are rejected.

## Verdict

- Multidimensional canonicalization: **APPROVE**.
- Same-field arbitration: **APPROVE, revised to pre-prepare branch-level
  elimination**.
- `TableChoice.coordinates`: **NOT REQUIRED**; optional only and outside the
  required 6.0.0 public contract.
