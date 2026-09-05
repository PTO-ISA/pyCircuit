# Qualified PYC leading-zero-count evidence

Date: 2026-09-05

Decision: 0165

Scope: vendor-neutral `pyc.count_leading_zeros`, structural and Cycle-Aware
Python, Agentic ACIR/QueueGraph lowering, C++ and gfsim reference semantics,
and Verilog-only selection of repository-owned BSD-3-Clause RTL.

## Results

| Lane | Result |
| --- | --- |
| pyCircuit repository unit lane | PASS: 22 tests |
| Primitive selection/verifier/C++/RTL/manifest system tests | PASS: 16 tests |
| Agentic Python frontend/JIT/tool lane | PASS: 144 tests, 2 environment skips |
| Standalone ACIR/ACSim lit | PASS: 141 tests |
| ACIR operation/verifier C++ tests | PASS: 1845 tests |
| QueueGraph code-generation C++ tests | PASS: 98 tests |
| gfsim exact-width helper and SimQueue block | PASS: 252 tests |
| Repository contracts, API hygiene, and MkDocs strict | PASS |

## Proved contracts

- `pyc.count_leading_zeros` returns `max(1,ceil(log2(N+1)))` bits, counts from
  the MSB, and maps all-zero input to `N` without implementation identity in
  canonical PYC.
- Structural, Cycle-Aware, and Agentic APIs infer parameters from input type;
  Cycle-Aware authoring preserves the input cycle.
- Agentic ACIR lowers to one semantic PYC op. Direct JIT and generated
  QueueGraph C++ execute the typed gfsim reference, including an actual
  13-bit Queue-to-Sink run.
- The selected RTL is a padded balanced zero-detect/count tree. Widths 1, 4,
  13, and 64 cover mixed and all-zero behavior; PYC C++ and selected RTL agree.
- Selection binds `WIDTH` and `COUNT_WIDTH`, verifies the BSD source digest,
  records source closure and manifest bindings, and rejects width 65.
- PR #29 vendor CLZ/LZC sources remain external design inputs; no Solderpad or
  Apache source is imported or relicensed.
