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
