# Parameterized PYC zero-count family evidence

Date: 2026-09-05

Decision: 0166

Scope: simple leading/trailing Python helpers over one static-direction ACIR,
PYC, gfsim SimQueue, catalog entry, and repository-owned BSD RTL family.

## Results

| Lane | Result |
| --- | --- |
| pyCircuit repository unit lane | PASS: 23 tests |
| Primitive selection/verifier/C++/RTL/manifest system tests | PASS: 16 tests |
| Agentic Python frontend/JIT/tool lane | PASS: 144 tests, 2 environment skips |
| Standalone ACIR/ACSim lit | PASS: 141 tests |
| ACIR operation/verifier C++ tests | PASS: 1845 tests |
| QueueGraph code-generation C++ tests | PASS: 98 tests |
| gfsim exact-width helper and SimQueue blocks | PASS: 252 tests |
| Repository contracts, API hygiene, and MkDocs strict | PASS |

## Proved contracts

- Public Python exposes no direction enum or RTL identity: authors call
  `count_leading_zeros` or `count_trailing_zeros`.
- ACIR and PYC freeze `leading|trailing` as verified static direction metadata.
- One gfsim `CountZeros<Width, Direction>` template and one selected RTL module
  implement both directions; the selection manifest differs only in
  `DIRECTION_LOW` bindings.
- Widths 1, 4, 13, and 64 cover both directions and all-zero result `N`.
- The RTL remains BSD-3-Clause, balanced, digest-closed, and fail-closed above
  the qualified width range. PR #29 vendor sources remain design references.
