# U02-B math review-A repair boundary

Independent `math_provenance_design` architect, Astra xhigh, 2026-09-28,
confirmed the three [review-A findings](review-a.md). The repair uses approved
C2-C only; it adds no serialized provenance token or public operation.

1. Permit math locally in `source` and `linked` semantic phases, reject it in
   final IR, and require an actual rule computation region or a verified
   source-owned value/record-constructor helper. Nested control inherits only
   that calculation owner. An outer unit attribute alone grants no owner.
2. Resolve `from_bits` from actual SSA: a rule current argument backed by its
   real owned DFFE or formal current port, or a helper logical-integer data
   parameter. Compare full LogicalType, including bounds and interpretation.
   Casts, arbitrary arithmetic, next-only endpoints, unchecked conversions,
   unknown calls, and check-derived values lacking path/valid proof reject.
3. Check a rule's asserted binding/domain against its actual handle, and
   compare body module ports with the explicit owning header at source-unit
   publication. Recursive helper validation must inspect nested regions.

The local phase predicate can be repaired now. Whole linked-program acceptance
still requires its own container and closure verifier; a local math-op test does
not prove linked-program validity. Final validation must reject residual math
operations and runtime `!ac.math_int` types throughout the program, but may
retain `#ac.math_int` constants in numeric witnesses. Source declaration
domains are interface assumptions, not proof of every runtime value. Numeric
legalization, checked-producer provenance, link/final proof and dual backend
execution remain separate work.

Two implementation owners have disjoint files: math verifier/tests, and
rule/header/helper authority/tests. Both must pass independent tests and a new
review before I10 can move from `revise` to accepted source-math subset.
