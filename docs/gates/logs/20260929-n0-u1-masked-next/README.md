# N0-U1 masked owned-register next assignment — verification evidence (2026-09-29)

Scope: the packet named as next by the Python numeric bridge review — an owned
`range(256)` register with `state = (state + 1) & 255`, initialised to 254. The
packet must retain the mask witness, the explicit integer boundary, the
assignment's original UseID/RequiredUse and the exact data/enable-to-target
yield binding, and must fail closed when those are redirected or dropped.

This packet adds **tests and evidence only**. It introduced no product change:
the reconnaissance below shows the lowering and a dedicated `U1` verifier
already implement the requirements, so the work was to prove it independently
rather than to reimplement it.

## What was already there, measured

Probing the real chain (`--lower-numeric` → link → emit) on the Counter fixture:

- the lowered source body keeps `arith.andi` for the mask, two
  `ac.numeric.proof` ops (the add, and the boundary proof carrying
  `checks = [{id = ...`), an `ac.expect` with `kind = "range"`, an
  `ac.value.use`, `ac.required_uses` with `role = "next"` and
  `kind = "next_scalar"`, and `ac.yield_bindings` with
  `data_operand = 0 : i32, enable_operand = 1 : i32`;
- the linked final artifact retains all of them (the reset image is re-encoded
  there, which is why the 254 literal is asserted on the source body and the
  value is proved by the runtime oracle instead);
- the emitted RTL carries `initial0 = 8'd254`, the full mask `8'd255`, `d0` and
  the enable `q0_e`.

## Verification

| Lane | Result |
| --- | --- |
| `tests/system/test_masked_next_register.py` | **10 passed** (3 conformance/oracle + 5 redirected-witness + 2 dropped-obligation cases) |
| Python system selectors | **73 passed, 0 failed, 0 skipped**; 2 V44 cases still deselected and DEFERRED to M6 (`python.xml`) |
| Native suite | unchanged by this packet; the same frozen revision passes 355 tests across all 20 `ACIR*Tests` binaries |

### Structural and runtime conformance

- `test_masked_next_retains_mask_boundary_use_and_yield_witnesses` asserts the
  reset image and `range(256)` domain on the source body, and asserts the mask
  witness, boundary proof with its check binding, range check, next-role use,
  `next_scalar` target kind and the data/enable yield binding in **both** the
  source body and the linked final artifact.
- `test_masked_next_rtl_keeps_the_reset_image_and_the_full_mask` asserts the
  emitted reset constant, the full mask, the next-state assignment and the
  enable.
- `test_masked_next_counts_every_reachable_state_in_hardware` runs the emitted
  RTL under Icarus Verilog with a testbench that reads the state through a
  **hierarchical probe** (the generated `FinalModel` exposes `q0`, and the state
  is otherwise unobservable because the design is portless). It starts at 254,
  asserts the two-phase hold — the old value is still visible before each commit
  edge — then commits 256 times and ends back at 254, so every one of the 256
  reachable states is exercised. Success prints `MASKED_NEXT_OK 254`.
  The probe is a testbench technique, not a product interface: no accessor,
  port, CLI flag or runtime API is added.

### Fail-closed rejections

All seven mutations are rejected by the compiler with a specific diagnostic and
publish no output:

| Mutation | Diagnostic |
| --- | --- |
| `data_operand 0→3` | `U1 YieldBinding is not the exact scalar contribution` |
| `enable_operand 1→0` | `U1 YieldBinding is not the exact scalar contribution` |
| `role = "next"` → `"use"` | `U1 ValueUse does not bind the required boundary value` |
| `kind = "next_scalar"` → `"read"` | `U1 RequiredUse is not the boundary value assignment` |
| `kind = "range"` → `"assert"` | `U1 requires exact low_bits and boundary witnesses` |
| drop the `ac.numeric.proof` op | `U1 lowered inventory is not closed` |
| drop the `ac.expect` op | `U1 lowered inventory is not closed` |

## Reproduction

```bash
P=/Users/zhoubot/.codex/worktrees/migration-capture/pyCircuit
cd "$P"
M2_BUILD="$P/.pycircuit_out/w10-pm/build"
GATE="$P/docs/gates/logs/20260929-n0-u1-masked-next"
ACIR_SOURCE_UNIT_HARNESS="$M2_BUILD/bin/acir-source-unit-harness" \
ACIR_BACKEND_CLOSURE_HARNESS="$M2_BUILD/bin/acir-backend-closure-harness" \
ACIR_DESIGN_HARNESS="$M2_BUILD/bin/acir-design-harness" \
PYTHONPATH="$P/python/pycircuit/src:$P/python/semantic-core/src:$P/python/agentic-circuit/src" \
pytest -q tests/system/test_masked_next_register.py \
  tests/system/test_source_design_bridge.py \
  tests/system/test_v41_v42_source_fixtures.py \
  tests/system/test_source_numeric_next.py \
  tests/system/test_unified_register_backends.py -k 'not v44' \
  --junitxml="$GATE/python.xml"
```

`IVERILOG` and `VVP` must be on `PATH` (or set those environment variables) for
the runtime oracle; the other cases need only the two private harnesses.

## Non-claims

- This is **not** a new lowering. The packet proves the existing implementation
  and its verifier; if a future change weakens the witnesses, these tests fail.
- The runtime oracle observes the state through a simulation hierarchical probe
  because the approved portless root exposes no ports. A DUT that exposes typed
  external state still depends on the unapproved D5 external typed DUT contract.
- `graph/final/backend numeric` admission for *other* numeric shapes, parallel
  scheduling/source reorder (V44) and the public entry remain open and are
  unchanged by this packet.
