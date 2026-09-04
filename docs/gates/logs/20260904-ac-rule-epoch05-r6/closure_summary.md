# Agentic Circuit epoch 0.5 rule-lowering closure

Run ID: `20260904-ac-rule-epoch05-r6`

This clean run validates the epoch 0.5 hard break and the first rule-oriented
frontend-to-backend vertical slice from the final reviewed checkout. G0/G1 use
the source tree before the build-tree native extension, while G2 rebuilds and
installs the integrated Agentic Circuit + pyCircuit 6 toolchain from the same
checkout.

## Agentic Circuit gates

- Python frontend: 128 tests passed, 2 optional tests skipped.
- CLI: 53/53 tests passed, including public `@ac.rule` capture and Frozen ACIR.
- ACIR/ACSim lit: 138/138 passed.
- CTest: 15/15 passed, including all six structured workspaces, DavinciOO,
  gfsim, codegen, verifier, installed-prefix, and rule-retirement E2E coverage.
- `@ac.rule` lowering reached typed internal `ac.firing`, proved the supported
  pure subset equivalent to `ac.transform`, generated QueueGraph C++, and ran
  the bounded non-wrapping retirement model in gfsim.
- The public `specialization.materialize_pyc()` path ran the same native MLIR
  pipeline before generating PYC. The generated PYC C++ model executed and
  verified retirement order, payload preservation, and `done`; Verilog was
  generated and linted.
- Flat QueueGraph freeze/planning rejects missing model-kind/domain evidence,
  structured declarations, unresolved markers/rules, forged firing or lowered
  rule proofs, raw unfrozen graphs, and mismatched topology digests.

## pyCircuit 6 closure

- Root unit gate: 53/53 passed.
- API hygiene: passed.
- Examples: passed.
- Normal simulations: passed.
- Nightly simulations: passed.
- V6 semantic regressions: passed in C++ and Verilator.
- `mkdocs build --strict`: passed.
- Strict decision status: 157 rows, with no deferred or unverified decisions.
- Repository contracts: 12 public schemas, 36 standard-library components,
  ACIR/ACSim inventory coverage, current generated ledger, and contract epoch
  0.5 all passed.

## Supported slice and remaining work

The implemented frontend slice supports one type-preserving Queue input, one
output, one total return path, the explicit singleton `cycle` QueueGraph
domain, and pure payload computation. Python `ac.atomic()` and
`Queue.firing()` are removed. The checks pass establishes an empty phase-one
check contract and rejects dynamic-check obligations until executable checked
IR exists.

CFG joins, executable path-qualified dynamic checks, multi-Queue and Table/Reg
proposals, explicit arbitration, and a circular allocation/completion ROB
remain future stateful rule slices. The included `rob.py` example is
intentionally a bounded, non-wrapping retirement/reorder demonstration.
