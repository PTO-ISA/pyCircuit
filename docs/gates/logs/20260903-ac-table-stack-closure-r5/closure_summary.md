# Agentic Circuit Table stack closure

Run ID: `20260903-ac-table-stack-closure-r5`

This run validates the final rebased implementation for Decisions 0151–0156
from one current checkout.

## Agentic Circuit gates

- Python frontend: 122 passed, 1 skipped optional case.
- CLI: 52 passed.
- ACIR lit: 134/134 passed.
- CTest: 14/14 passed, including Workspace E2E, DavinciOO trace, gfsim,
  verifier, codegen, and PYC Verilog backend tests.
- Table E2E: 7/7 passed with executable direct/native allocation parity.
- AC G2: integrated pyc6 and Agentic Circuit toolchain built from this checkout;
  `arbiter`, `atomic-transform`, and `popcount` passed PYC C++/Verilog gates.

## pyCircuit closure

- Root unit gate: 53/53 passed.
- API hygiene: passed after removing stale evidence-only forbidden tokens.
- Examples: passed.
- V6 semantic regressions: passed.
- Normal simulations: passed.
- Nightly simulations: passed.
- `mkdocs build --strict`: passed.
- Strict decision status: 156 rows, no deferred or unverified decisions.

The provisional Table family remains intentionally rejected at the PYC/RTL
boundary. The public resident-entry Issue Table example explicitly does not
claim to solve the cross-Queue late-arrival lost-wakeup problem tracked by
issue #11.
