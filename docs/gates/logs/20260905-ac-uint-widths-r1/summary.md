# Agentic Circuit exact unsigned widths

Decision 0160 adds the complete `ac.u1` through `ac.u64` public type family,
same-width bit operations, exact-width gfsim storage, and matching
ACIR-to-PYC lowering.

## Targeted evidence

- Python frontend: 133 passed, 2 skipped
- Python contracts: 37 passed
- Python CLI: 53 passed
- root Python unit tests: 13 passed
- ACIR lit/parser/verifier/codegen: 139 passed
- ACIR CTest suite: 15 passed
- direct QueueGraph C++ generation and syntax compilation: passed
- `i3` ACIR QueueGraph to gfsim C++ and PYC generation: passed
- generated PYC through `pyc-opt`, pyc6 C++, and Verilog/`iverilog`: passed
- gfsim `UInt<1>`, `UInt<3>`, `UInt<7>`, and `UInt<64>`
  truncation/shift tests: passed
- IR inventory/coverage, release layout, pre-commit, API hygiene: passed
- strict decision status: 160 rows, 0 deferred
- MkDocs strict build: passed

Full release closure remains release-only under Decision 0159.
