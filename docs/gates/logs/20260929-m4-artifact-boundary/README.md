# M4-D2 design / testbench artifact boundary — verification evidence (2026-09-29)

Scope: make the design / testbench / framework boundary a tested capability
instead of a documented intention, using only already-approved interfaces.
This packet adds **no** IR, CLI, runtime or schema surface. It closes the
"prove it, don't rename it" half of the user's correction; the ported-DUT half
remains blocked on the unapproved D5 external typed DUT contract.

## What the boundary now means, concretely

| Layer | Artifact | Content proven by the tests |
| --- | --- | --- |
| Hardware design | the DUT closure linked on its own root, named after its source (`blinker.py` → `blinker.ac`) | contains `demo.blinker.Blinker`, **no** `ac.observe`, **no** testbench symbols, **no** stimulus strings |
| Testbench | a **separate** artifact with the system as its root (`test_blinker.py` → `test_blinker.ac`) | contains `TestBlinker`, `ac.observe` and the DUT, and links only after the DUT unit is present |
| Framework / runtime | the compiler, emitters and the `runtime-glue` role | the design's `rtl` artifact contains `module FinalModel(` and **no** `FinalModelSim` and **no** `AC_OBS`, so the simulation wrapper is not part of the hardware design artifact |

Two properties are what make this a boundary rather than a naming convention:

1. The design artifact links **from the DUT closure alone**. The testbench
   source does not need to exist, be readable, or be part of that link.
2. A DUT with typed external ports **fails closed** as a root today
   (`selected root has an unbound data formal or synthetic root StateID`) and
   creates no artifact. This is the recorded D5 dependency: until the external
   typed DUT contract is approved, the framework must not silently emit a
   portless or stimulus-bearing artifact and call it a design.

## Verification

| Lane | Result |
| --- | --- |
| `tests/system/test_source_design_bridge.py` | **35 passed** (2 new cases) |
| Python system selectors | **73 passed, 0 failed, 0 skipped**; 2 V44 cases still deselected and DEFERRED to M6 (`python.xml`). This lane covers the five system files in the reproduction command, so the same run also carries the M4-D1 role-split cases and the N0-U1 masked-next cases. |
| Native suite | unchanged by this packet (test-file-only change); the preceding M4-D1 run remains 355/355 across all 20 `ACIR*Tests` binaries |

New cases:

- `test_design_and_testbench_are_separate_artifacts` — links the DUT-only
  closure and the testbench closure as two artifacts, asserts the design text
  carries no observation/stimulus, asserts the testbench does, and asserts the
  design's `rtl` role file has no simulation wrapper or `AC_OBS` record.
- `test_ported_module_root_is_rejected_until_the_dut_io_contract` — locks the
  fail-closed behaviour and the D5 dependency with an explicit diagnostic.

## Reproduction

```bash
P=/Users/zhoubot/.codex/worktrees/migration-capture/pyCircuit
cd "$P"
M2_BUILD="$P/.pycircuit_out/w10-pm/build"
GATE="$P/docs/gates/logs/20260929-m4-artifact-boundary"
ACIR_SOURCE_UNIT_HARNESS="$M2_BUILD/bin/acir-source-unit-harness" \
ACIR_BACKEND_CLOSURE_HARNESS="$M2_BUILD/bin/acir-backend-closure-harness" \
ACIR_DESIGN_HARNESS="$M2_BUILD/bin/acir-design-harness" \
PYTHONPATH="$P/python/pycircuit/src:$P/python/semantic-core/src:$P/python/agentic-circuit/src" \
pytest -q tests/system/test_source_design_bridge.py \
  tests/system/test_v41_v42_source_fixtures.py \
  tests/system/test_source_numeric_next.py \
  tests/system/test_unified_register_backends.py -k 'not v44' \
  --junitxml="$GATE/python.xml"
```

## Non-claims

- This is **not** independent DUT delivery. A synthesizable design with typed
  external ports still requires the unapproved D5 external typed DUT contract;
  the current design artifacts are portless roots, which is exactly what C3-C's
  approved base root allows.
- The testbench is linked through the private design harness. The public
  compile/link/emit entry, the per-source generated C++ groups, the standard
  runner and the `generated.json` role manifest are still M4/W11 work.
- The testbench artifact still contains the DUT hardware inline as a child
  instance, which is expected: a testbench consumes the design. The separation
  claim is about what the *design* artifact may contain, not about the
  testbench avoiding the DUT.

## Built-in negative control

The absence assertions are paired with presence assertions in the same test, so
they cannot pass vacuously. Measured on the two artifacts of one run:

| Artifact | `ac.observe` | `TestBlinker` | `dut_started` | `demo.blinker.Blinker` |
| --- | --- | --- | --- | --- |
| design (`blinker.ac`) | 0 | 0 | 0 | 88 |
| testbench (`test_blinker.ac`) | 2 | 99 | 2 | 86 |

The testbench-only log message `dut_started` survives into the testbench
artifact, and the test asserts that, so `assert "dut_started" not in design_text`
is proving real separation rather than a string that never survives a link.
