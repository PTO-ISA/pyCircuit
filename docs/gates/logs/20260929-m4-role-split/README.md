# M4-D1 generated-output role split — verification evidence (2026-09-29)

Scope: separate the hardware Verilog artifact from the simulation wrapper at the
file-role boundary that C3-C already approves
(`header/source/cmake/rtl/runtime-glue/source-map`). This is the approved
contract being executed, not a new interface: no `ac.*` op, type, attribute,
public CLI flag, runtime or schema changes.

## What changed

| Object | Change |
| --- | --- |
| `FinalVerilogEmission` (`FinalProgram.h`) | new struct holding `rtl` (module families + portless `FinalModel` top) and `runtimeGlue` (the `FinalModelSim` observation wrapper that instantiates that top) |
| `emitFinalVerilogParts` | new entry point returning both parts, with the same verify-before/verify-after discipline as the combined emitter |
| `emitFinalVerilog` | unchanged signature and byte-for-byte behaviour: it now returns `rtl + runtimeGlue` |
| `emitFinalVerilogPartsBody` | the emitter builds the hardware section into one stream and the wrapper into a second stream; the split point is the `FinalModel`/`FinalModelSim` boundary |
| `acir-design-harness --glue-output <path>` | private-tool option: with `--target verilog`, `--output` receives the `rtl` artifact and `--glue-output` receives the `runtime-glue` artifact; without it the combined emission is written exactly as before |

`--glue-output` is rejected with rc=2 outside emit mode, for `--target cpp`, and
when it equals `--output`. When it is used, both destinations are checked before
either is created and a failure on the second removes the first, so a rejected
bundle publishes no half artifact.

The C++ target is deliberately **not** split. Its generated model consumes the
GFSIM runtime through `gfsim/SimSystem.h`/`SimDFF.h` rather than duplicating it,
and it emits no `FinalModelSim` equivalent (verified in the boundary probe), so
there is no second role to separate at this layer.

## Verification

Current-checkout LLVM/MLIR 22.1.8, macOS arm64; both emitters actually executed.

| Lane | Result |
| --- | --- |
| All 20 `ACIR*Tests` binaries | **355 tests, 0 failures / 0 errors / 0 skipped / 0 disabled** (`native/all-native-summary.json`, per-binary `.xml`/`.log`) |
| Python system selectors | **60 passed, 0 failed, 0 skipped**; 2 V44 cases still deselected and DEFERRED to M6 (`python.xml`) |
| Role-split system tests | 8 new cases in `tests/system/test_source_design_bridge.py` (33 total in that file) |
| Native role-split assertions | added to `FinalEmittersAreDeterministicAndToolAccepted` in `FinalProgramTest.cpp` |

The evidence that the split is a pure file-role boundary, not a behaviour change:

- `rtl + runtimeGlue` is byte-identical to the combined `emitFinalVerilog`
  output — asserted natively and again through the harness on two different
  linked designs (portless module root and the closed-system fixture).
- The `rtl` artifact contains `module FinalModel(` and **no** `FinalModelSim`.
- The `runtime-glue` artifact contains `module FinalModelSim(` and
  `FinalModel dut(`, and **no** hardware top definition.
- Existing-output protection covers the new second destination for file,
  directory, symlink and dangling-symlink cases, and the dangling-symlink case
  additionally proves the rollback removes an already-written `rtl` file.

## Reproduction

```bash
P=/Users/zhoubot/.codex/worktrees/migration-capture/pyCircuit
cd "$P"
M2_BUILD="$P/.pycircuit_out/w10-pm/build"
GATE="$P/docs/gates/logs/20260929-m4-role-split"
TARGETS=$(tr '\n' ' ' < "$GATE/native/targets.txt")
cmake --build "$M2_BUILD" --target acir-source-unit-harness acir-design-harness $TARGETS -j 6
while read -r t; do
  ACIR_BACKEND_CLOSURE_HARNESS="$M2_BUILD/bin/acir-backend-closure-harness" \
  ACIR_SOURCE_UNIT_HARNESS="$M2_BUILD/bin/acir-source-unit-harness" \
  PYTHONPATH="$P/python/pycircuit/src:$P/python/semantic-core/src:$P/python/agentic-circuit/src" \
  "$M2_BUILD/bin/$t" --gtest_output="xml:$GATE/native/$t.xml" > "$GATE/native/$t.log" 2>&1 || exit 1
done < "$GATE/native/targets.txt"
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

- No `generated.json` bundle writer exists on the new route yet. The approved
  roles are now *producible* as separate files from the private harness, but the
  public emit entry that materialises a role manifest is still W11/M4 work.
- The private `--glue-output` option is a development-tool surface. It is not the
  public driver contract, and C3-C's `-o <program.ac>` text still needs its
  naming amendment (source-derived `<stem>.ac`).
- Independent design/testbench delivery is still not established: the linked
  positive fixture remains a closed self-testing system, and a real DUT with
  typed external ports still depends on the unapproved D5 external typed DUT
  contract.
- This packet does not change the public Python surface, the runtime, or any
  `ac.*` primitive.
