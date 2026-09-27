# U02-B source-math foundation independent review A

Reviewed isolated commit `ca5bf2043267824e6dce75271b494d0153bd7c5f`
against approved C2-C. Independent `code-reviewer`, Sol high, 2026-09-28.
Five committed files were reviewed; their content-manifest SHA-256 is
`912fc5b79d7038ddb5618a59a93a3c1736a0b61b6e3250adeac65d97a267e245`.
Verdict: **REQUEST CHANGES**. This slice is not accepted.

Three high-severity contract gaps remain:

1. The verifier admits only `ac.stage=source`, whereas C2 permits temporary
   math in both source and linked semantic stages and forbids it in final IR.
2. `ac.math.from_bits` trusts a caller-supplied logical domain if its storage
   width matches; it does not derive that domain from verified state, interface,
   or check provenance. The positive test even uses unconstrained fabricated
   bits, so signedness and range could be relabeled.
3. Math op verification checks only the outer builtin module, admitting a
   free-floating graph outside a rule or pure value helper.

A fresh TableGen/C++ build passed and the authored math test passed 6/6;
`git show --check ca5bf204` passed. Those results do not resolve the three
semantic findings. The first-slice operator limitations are recorded as
temporary capability gaps, not permanent C2 restrictions. A separate
architect repair design and revised implementation/tests are required.
