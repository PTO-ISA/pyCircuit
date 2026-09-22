# Item V03: ACIR to PYC lowering closure

## Item

`#128` V03 — "applicable ACIR→PYC lowering 后无 residual `scf.*` / `index`，并通过
PYC verifier". A lowering that publishes frozen ACIR has to reach verified PYC
without leaving high-level control flow or index arithmetic behind, and the PYC
verifier has to accept the emitted module.

## Evidence

`tests/python/agentic-circuit/python_frontend/test_pyc_lowering_closure.py` runs
five designs through the full lane — frontend, `ac-freeze-topology`,
`acir-queue-pycgen`, `pycc --emit=none` — and asserts, per design, that freezing
returns 0, PYC generation returns 0, the PYC contains `pyc.*` operations (so the
residual check cannot pass vacuously), no `scf.` or `index.` token survives, and
the PYC verifier accepts the result.

Measured on this revision (the native tools are the only prerequisite; without
them the tests skip, which keeps the Python-only gate lane intact):

| design | frozen ACIR | PYC | residual | `pyc.*` lines | `pycc` |
| --- | --- | --- | --- | --- | --- |
| inline helper in a flat system | ok | 4373 B | none | 11 | accept |
| pure expression module (struct payload) | ok | 7902 B | none | 25 | accept |
| composite hierarchy `In -> Mid -> Out` | ok | 16310 B | none | 51 | accept |
| parameterized family, one concrete case | ok | 30441 B | none | 400 | accept |
| nested-record projection system | ok | 4547 B | none | 16 | accept |

Lane results for the same revision:

```text
tests/python/agentic-circuit/python_frontend (tracked): 417 passed, 1 skipped
tests/python/agentic-circuit (pytest, ignore tools/):   487 passed, 1 skipped, 5 failed
tests/unit -m unit:                                     217 passed
check-acir (tests/mlir):                                258 passed, 13 failed
ctest:                                                  6/6 passed
check-contracts.py / check-ir-coverage.py / catalog:    OK
```

The five failures are pre-existing and unrelated to this item: two are
environmental `cli/test_discovery_commands.py` schema mismatches that also fail
at `40cdb972`, and three are in the untracked user work-in-progress
`test_dual_port_issue_queue.py`. The 13 lit failures are all
`verilator: command not found`.

## Hand-written ACIR coverage

The lit suite already lowers hand-written ACIR through the same generator in 40
`%acir_queue_pycgen` tests, and 11 of them verify the result with `%pycc`; the
PYC dialect verifier is what rejects an illegal lowering. What those tests do not
assert is the absence of residual `scf.*`/`index` operations, and none of them
starts from frontend-published ACIR. This item adds both for the five designs
above.

## Scope

- This item is item V03 only. V04 (GFSim/PYC C++/Verilog parity) still needs a
  Verilog toolchain; V07 (full lane sweep with nightly simulation and strict
  docs) still needs `verilator`, the simulation lanes, and `mkdocs`; V08 is a
  consumer-side obligation under Decisions 0158 and 0235. None of them are
  claimed here.
- The residual check is a token-level assertion over the emitted PYC text. It is
  meaningful because the same run also requires a non-empty `pyc.*` body and an
  accepting `pycc`, so a lowering that produced nothing or that produced
  unverifiable PYC cannot pass.
