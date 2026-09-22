# Items V01 and V02: frontend/API coverage and Frozen ACIR plus QueueGraph closure

## V01 — frontend positive/negative examples, public API, and typecheck

Verified on this revision, in the Python-only lane as well as the native lane:

```text
tests/python/agentic-circuit/python_frontend (tracked): 417 passed, 1 skipped
assertRaises call sites across the frontend tests:      304
frontend test modules asserting QueueFrontendError:     13
test_public_api.py:                                     31 passed
tests/python/agentic-circuit/contracts (unittest):       29 passed
tools/agentic-circuit/check-contracts.py:                OK (35 stdlib components)
tools/agentic-circuit/generate-diagnostic-catalog.py:    OK (no stale entry)
```

- **Positive and negative examples.** Every frontend surface is exercised by
  `tests/python/agentic-circuit/python_frontend`, and rejection behaviour is
  pinned by 304 `assertRaises` call sites spread over 13 modules that assert
  `QueueFrontendError` codes and messages. Two examples added in this round of
  work keep the pipeline honest rather than only the parser:
  `test_module_hierarchy.py` and `test_module_nominals.py` assert both the
  accepted shape and its frozen/`pycgen` result, and `test_pyc_lowering_closure.py`
  asserts the accepted shape reaches verified PYC.
- **Public API and typecheck.** `test_public_api.py` (31 tests) pins the public
  surface: the capture-only inventory, public signatures, frozen dataclass
  behaviour, and the banned-name contract. `tests/python/agentic-circuit/contracts`
  (29 tests) validates the published schemas and the stdlib inventory, and
  `check-contracts.py` re-verifies the public schemas plus the 35 stdlib
  components. `check_api_hygiene.py` remains the gate for the pyCircuit package
  surface.
- **Diagnostics.** The diagnostic catalog check proves the published registry and
  every `ACPY-*` code in the compiler source agree, so a negative example cannot
  cite a code that no longer exists.

## V02 — Frozen ACIR verifier/lit coverage and the QueueGraph schema round trip

```text
lit tests under tests/mlir/agentic-circuit/ACIR:            122
lit tests under tests/mlir/agentic-circuit/Transforms:       76
lit tests running ac-freeze-topology:                        95
lit tests running ac-verify-rule-closure:                    53
check-acir (tests/mlir):            258 passed, 13 failed (all verilator: command not found)
ctest:                              6/6 passed
CodeGenTests QueueGraphPlanTest:    110 passed
```

- **Frozen ACIR verification.** `ac-freeze-topology` and
  `ac-verify-rule-closure` are the passes that close a unit, and they are
  exercised by 95 and 53 lit tests respectively inside 122 `ACIR/` and 76
  `Transforms/` fixtures. The frontend tests added in this round additionally
  run the real pipeline (`builtin.module(ac-lower-rules, ac-inline-pure-helpers,
  canonicalize, cse, ac-verify-rule-closure, ac-freeze-topology)`) over
  frontend-published ACIR, so a frontend-only shape cannot pass while the
  verifier rejects the emitted unit.
- **QueueGraph schema round trip.** `acir-queue-plan` emits one canonical JSON
  document with `"schema": "agentic-circuit-queue-graph-plan"` and
  `"version": "0.5"`, including `source_provenance` frame stacks on blocks,
  expressions, instances, and state owners.
  `CodeGenTests --gtest_filter='*QueueGraphPlan*'` covers 110 cases, among them
  `CanonicalJsonIsByteIdenticalAndClosed` (the canonical JSON is byte-identical
  and closed), `VerifiesCanonicalSourceProvenance` (the source-map JSON is
  re-parsed with `llvm::json::parse` and its `table_matches` /
  `table_selections` sections are checked), and
  `RejectsRawMalformedSourceProvenanceBeforeNormalization` (malformed frames are
  rejected before normalization). The 13 lit failures are the environment gap
  (`verilator: command not found`) already recorded for the other matrix items;
  none of them is an ACIR verifier or QueueGraph failure.

## Scope

Items V01 and V02 only. V03 is recorded separately in
`docs/gates/logs/20260922-v03-pyc-closure/`. V04, V05, V07, and V08 stay open:
V04/V07 need a Verilog toolchain, the simulation lanes, and strict
documentation; V05 asks for checked-in source-map goldens covering the helper,
module, projection, and specialization constructs, which is not claimed here;
V08 is a consumer-side obligation under Decisions 0158 and 0235.
