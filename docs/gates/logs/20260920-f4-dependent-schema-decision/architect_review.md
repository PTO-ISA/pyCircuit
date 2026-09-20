# Architect review: exact dependent family and PYC mapping schema

Date: 2026-09-20

Verdict: accept Decision 0278 before implementing dependent family
materialization or backend family emission. Decision 0277 names the required
record families, but independent implementations could still disagree on
literal precision, type bounds, packing, Queue carriers, controls, or required
metadata.

## Frozen micro-schema

- `DependentValueAttr` is a closed union of arbitrary-precision signed
  mathematical integer literal, typed static literal, parameter, rooted
  non-empty field path, add, subtract, multiply, index-width, and count-width
  records. Dependent arguments are ordered `(name, value)` records.
- Arithmetic is exact over mathematical integers. Width/cardinality and typed
  integer representability are checked only at the consuming boundary;
  wrapping or host-width behavior is forbidden.
- Logical types are exactly concrete, bits, unsigned half-open range, value
  array, non-empty tuple, nominal declaration plus dependent arguments, and
  Queue payload plus explicit lanes/rate. Queue materialization requires
  `lanes > 0` and `1 <= rate <= lanes`.
- Source ownership always contains implementation and declaration paths.
  Interface ports carry exact input/output direction, logical type, and typed
  source provenance; dictionary or path-only provenance is insufficient.
- PYC preserves typed field/tuple/array projection steps and paths, packed
  leaves/layouts, physical port direction/index/type/role/lane/layout, logical
  port mappings, implicit control origins, control mappings, full module port
  mappings, and complete module case signatures.
- Every case has explicit implicit-origin clock/reset physical inputs at
  indices 0/1. A multi-lane Queue has per-lane valid/data carriers and exactly
  one shared ready carrier in the opposite physical direction.
- Family, case, import, and instance verification recomputes full canonical
  signatures and rejects omitted, reordered, stale, inferred, stringly, or
  backend-reconstructed fields.

## Hard-break boundary

The implementation must remove the old width-limited
`DependentLiteralAttr`, string or postfix dependent expressions, path strings,
string nominal applications, omitted Queue lanes/rate, per-lane or inferred
ready, inferred layouts, unrecorded clock/reset origins, dictionary
provenance, and partial case-signature metadata in one change. No reader,
upgrader, alias, fallback, or optional metadata mode is admitted.

## Required implementation evidence

- Parse/print, canonicalization, equality, and verifier coverage for every
  record/union member and malformed combination.
- Positive/negative arbitrary-precision arithmetic, width-function,
  representability, half-open range, tuple/array/nominal, and Queue tests.
- Complete family/case/import/instance signature recomputation tests.
- PYC scalar, aggregate, multi-lane Queue, shared-ready, projection/layout,
  implicit-control, and physical-signature tests.
- Deterministic re-emission and repository/API absence gates for every removed
  form.

Until that evidence exists, Decision 0278 remains `gap-in-scope` and dependent
family publication and backend family emission stay blocked.
