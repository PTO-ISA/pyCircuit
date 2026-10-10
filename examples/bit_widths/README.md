# Exact-width queued record transform

[Generated MLIR, C++ and RTL examples](GENERATED.md)

| Field | Width | Transformation |
| --- | --- | --- |
| `value` | 13 | `(input.value & input.mask) ^ 1` |
| `mask` | 13 | Preserved |
| `rotated` | 13 | Rotate the original `input.value` left by one bit |
| `sequence` | 37 | Preserved |

The rule starts with an ordinary record value copy and updates two fields.
Both expressions read the original input, so rotating the masked/XOR result
would be incorrect. Arithmetic retains the declared unsigned widths.

## Interface and timing

`BitWidths(valid, data, take)` returns a typed result containing `ready`, `valid`
and `data`. A rising edge accepts input when input `valid & ready` is one and
consumes output when output `valid & take` is one. The runner supplies the
generated physical clock/reset pins; Python contains no clock/reset, DFFE or
proposal API.

The original source queue and the `.apply()` result queue each defaulted to
depth one and latency one. This design therefore has exactly two one-token
queues. It explicitly selects `downstream_pop` on both to preserve the original
RTL FIFO's full-queue replacement behavior. The new frontend's default depth
two/local-occupancy policy is deliberately not used for this historical design.

If the first input is accepted at edge E0, its transformed value enters the
second queue at E1 and can be consumed at E2. The second queue becomes visible
in the Work following E1. There is no empty flow-through. Continuous valid/take
can sustain one token per edge after startup. Stalling the output fills two
slots; a later simultaneous output pop can permit both internal transfer and a
new input on the same edge. Held clock levels do not transfer tokens.

Reset empties both stages and makes empty output data known zero. Work samples
observe committed old state; Xfer does not refresh an already-computed output
snapshot. All clocked transfers use the shared Runtime lifecycle.

## Historical boundary

The historical same-named compiler smoke used a different three-bit record and
only compilation/lint checks. It is not a runtime oracle for this `MaskedTag`
design. Implementation acceptance uses the original source, queue definitions and
independently derived token/edge expectations.

## Build and verification

```sh
cmake -S examples/bit_widths -B /absolute/build/bit-widths -G Ninja \
  -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/bit-widths --parallel 4
ctest --test-dir /absolute/build/bit-widths --output-on-failure --no-tests=error
```

The independent integer/token oracle checks 195 Work snapshots in native
workers1/2 and Verilator. Its inputs include all 76 walking packed bits, the
highest 37-bit sequence values and patterns that distinguish rotation of the
original value from rotation after masking. It checks startup, full replacement,
backpressure, drain, reset, and active repeated high/low clocks.

The separate commit-qualified host ledger checks 87 accepted inputs, 85 retired
outputs and two reset-dropped tokens, with no outstanding tokens and peak
occupancy exactly two. Held Work samples do not add history entries.

Four X/Z packet cases run through the actual native DUT for both worker counts
and the complete generated RTL in Icarus. They check masked unknowns, rotation
across the packed word boundary and unchanged mask/sequence planes. CTest runs
both the shared known-data gate and the explicit Icarus gate; their receipts
bind the source, runtime assets, generated model and test inputs.

## Scenario bench

`bench.py` stores all 184 original scenarios in an immutable typed Table. Its
12-field, 156-bit `Stimulus` retains the original schema and field order. Row 184
is all zero, preserving the helper fallback for every known u16 phase from 184
through 65,535. This fallback is distinct from the system counter, which holds
at phase 183. The run retains its original 184-cycle limit.

The four packet fields remain 13/13/13/37 bits. Both sequence fields retain
values above 32 bits, up to 2³⁷−1, and the overwritten `input_rotated` remains
part of the stimulus. No field is narrowed or expected transform recomputed.

The complete system retains all 6 unconditional assertions, 7 observations,
ordered DUT inputs and `advance(phase)` before `check()`. Independent source
checks compare all 786,432 field values across the complete known-u16 helper
domain. Twenty complete native and RTL traces match all 2,576 observations
and the terminal result after excluding only changed source-position metadata;
native workers 1 and 2 are included. Baseline and compact versions each pass
all three standalone gates. See the module [excerpts and receipt](GENERATED.md).

The closed system covers a regular-clock known-state trajectory. Original
physical reset, held-clock and four-state histories remain with the unchanged
independent module drivers described above. Known-u16 source equivalence does
not establish arbitrary X/Z-phase equivalence or full physical-scenario migration.

## Local cost comparison

The source shrinks from 2,240 lines / 63,322 bytes to
268 lines / 45,348 bytes. Measurements use the same checkout-built
compiler, LLVM 22 C++ toolchain and `-O0` on one machine. Compile, link, emit,
build and first-run values are single serial observations; warm medians use
three alternating full-length pairs. Every fresh process includes initialization.
These local results do not guarantee performance on other platforms or
optimization levels.

| Measurement | Expanded cases | Typed Table |
| --- | ---: | ---: |
| Compile bench | 1.595 s | 1.161 s |
| Link system | 2.443 s | 1.844 s |
| Emit C++ | 2.446 s | 1.784 s |
| Emit Verilog | 1.788 s | 1.391 s |
| Build C++ | 1.416 s | 1.087 s |
| Build Verilog | 1.611 s | 1.430 s |
| First measured full C++ run | 0.592 s | 0.474 s |
| First measured full Verilog run | 0.299 s | 0.313 s |
| Warm C++ median, one worker | 0.301403 s | 0.028854 s |
| Warm Verilog median | 0.026450 s | 0.025250 s |
| Warm native peak RSS median | 4.45 MiB | 3.12 MiB |

| Artifact size | Expanded cases | Typed Table |
| --- | ---: | ---: |
| Final IR | 10,347,048 bytes | 7,995,838 bytes |
| Generated C++ total | 2,695,160 bytes | 2,249,731 bytes |
| Generated Verilog total | 599,784 bytes | 442,549 bytes |

The first measured Verilog run increases from 0.299 to 0.313 s even though the
warm median decreases. Keep that single-run regression separate from the warm
result; this is not an across-the-board timing improvement.

Three warm pairs do not establish statistical significance for small RTL changes.
Complete source-import/transformed artifacts reproduce the published units, and
the public and measured builds share identical final IR and generated outputs.
DUTs, drivers, configuration, cycle limits and timeouts remain unchanged.

```sh
pycircuit run examples/bit_widths --target cpp --cycles 184
pycircuit run examples/bit_widths --target verilog --cycles 184
```
