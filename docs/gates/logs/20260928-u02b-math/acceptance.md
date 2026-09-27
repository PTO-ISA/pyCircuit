# U02-B source-math foundation acceptance

Accepted isolated foundation: `ca5bf204` followed by repair commits
`05df7fe1`, `1018c501`, and shared tests `ac821900`. [Review A](review-a.md)
found three high-severity contract gaps; [review B](review-b.md)
independently confirmed their closure. PM's clean current-checkout
[integration check](../20260928-u02b-foundation/integration/results.md) passed
CTest 5/5 and 78/78 source-system cases, without skips.

This acceptance covers the staged source-math op subset
`constant/from_bits/binary(add,and_bits)/to_bits`, calculation ownership,
declaration-backed local domain provenance, rule input authority, and owning
header port comparison. Other approved operators, `scf.if`, checked-producer
provenance, numeric witnesses, whole linked/final legality, legalization,
both emitters, and runtime oracle remain open. The candidate remains isolated.
