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

The C++ target is deliberately **not** split. It does emit a runner-facing
`FinalSystem : gfsim::SimSystem` wrapper, but that is the structural counterpart
of the stepping API rather than a second copy of it: the stepping, epoch and
observation logic lives in the gfsim runtime and is not duplicated per design.
There is therefore no separately-splittable simulation observation wrapper on
the C++ side.

## Verification

Current-checkout LLVM/MLIR 22.1.8, macOS arm64; both emitters actually executed.

| Lane | Result |
| --- | --- |
| All 20 `ACIR*Tests` binaries | **355 tests, 0 failures / 0 errors / 0 skipped / 0 disabled** (`native/all-native-summary.json`, per-binary `.xml`/`.log`) |
| Python system selectors | **73 passed, 0 failed, 0 skipped**; 2 V44 cases still deselected and DEFERRED to M6 (`python.xml`). This lane covers the five system files listed in the reproduction command, so it also carries the M4-D2 boundary cases and the N0-U1 masked-next cases. |
| Role-split system tests | 9 new cases in `tests/system/test_source_design_bridge.py` (34 total in that file), including a path-alias case that proves the rollback, not the string-equality guard, removes a half-published bundle |
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


## Independent review

Reviewer: a separate reviewer agent in an independent context, not the author of
any file in this packet. Verdict on `fcdb75e6`: **PASS** — every fail trigger was
tested and disproven; the change is a genuine file-role boundary with
byte-identical combined output and no semantics, CLI, runtime, schema or `ac.*`
surface change. One major evidence-provenance defect and three minor defects were
raised and are all resolved here.

| # | Finding | Severity | Resolution |
| --- | --- | --- | --- |
| 1 | `overlay-sha256.txt` recorded hashes for `FinalEmitVerilog.cpp` and `FinalProgramTest.cpp` that matched no committed blob, because the hashes were taken before the final `clang-format` pass; the shipped logs therefore did not pin the reviewed revision | major | every lane was re-run from a clean rebuild of the committed sources and `overlay-sha256.txt` is recomputed from the exact bytes that are committed. The reviewer independently reproduced byte-identical emitted Verilog before and after that format-only edit. |
| 2 | "C++ has no `FinalModelSim` equivalent" was overstated: `FinalEmitCppSystem.cpp` does emit a runner-facing `FinalSystem` wrapper | minor | restated above as "no separately-splittable simulation observation wrapper", with stepping/epoch/observation logic in the gfsim runtime |
| 3 | the rollback `llvm::sys::fs::remove` discarded its `error_code`, so a failed rollback could leave a half bundle while returning rc=1 | minor | the error is now reported on stderr |
| 4 | `FinalEmit.cpp` duplicated the verify-before / `isEmitReady` / verify-after discipline instead of sharing it | minor | one `EmissionGuard` now defines that sequence once for every final emitter |

The reviewer also showed that only the `[dangling-symlink]` case exercised the
rollback, and proved that case non-vacuous by comparing compiled copies with and
without the `remove` line. It is now joined by an aliased-path case
(`--glue-output` as `./name` against `--output` as `name`) that bypasses the
string-equality guard and reaches the rollback.

## Evidence provenance

The exact bytes of every file this packet verifies are recorded in
`overlay-sha256.txt`, taken after all formatting and before the commit, and each
entry is checked against the committed blob. The earlier mismatch is why this
section exists: hashes are no longer computed from a working tree that can still
change.

Deliberately unchanged: no `.td`, runtime, schema, Python or public CLI file is
touched. `acir-design-harness` has no CMake `install()` rule, so `--glue-output`
is build-tree-only private tooling rather than a public flag.
