# Two-stage record update pipeline

Both original `int` fields resolve to unsigned fixed 64-bit hardware. `Item`
therefore keeps `value:ac.u64` followed by `remaining:ac.u64`. The typed pure
`update_item` rule reconstructs the complete record, incrementing value and
decrementing remaining independently modulo 2^64. Value UINT64_MAX wraps to
zero; remaining zero underflows to UINT64_MAX. Neither field's arithmetic
carries or borrows into the other field.

The complete 128-bit payload packs value at bits 64 through 127 and remaining
at bits 0 through 63. `StructResult` packs valid at bit 128 and ready at bit 129,
for 130 bits total. Expected output is `(value_out << 64) | remaining_out`;
both outputs derive from the original input fields.

Two explicit queues each have depth 2 and availability latency 1, for four
complete tokens of capacity. Both declare `downstream_pop` readiness, read old
committed heads and have no empty bypass. The sink adds no storage. E0 captures
an input, E1 commits its transformed old head to the result queue, and E2 is
the earliest external retirement. Depth 2 adds capacity without a mandatory
extra latency stage.

For old input/result occupancies n0/n1, result readiness is `n1 < 2` or an
old result-head pop. Advance consumes only the old input head when result
readiness permits. Input readiness is `n0 < 2` or that advance. Full replacement
retains both occupancies and FIFO order through depth-two pointer wraps.
Held clock levels do not commit despite changed offers/take; rising reset
clears up to four tokens. Physical clock/reset levels are host-driven and
absent from Python; the pure rule adds no state or transaction proposal.

Any X/Z in value makes only the computed value field 64 X bits. A known
remaining field still decrements exactly. Any X/Z in remaining similarly
makes only remaining X while a known value increments exactly. Both uncertain
fields yield 128 X bits. Computed unknown fields have no Z and unspecified
hidden value planes. Neither field is an unchanged raw transport field;
zero and uncertain tokens remain valid.

## Build route

Use the accepted installed toolchain and public example CMake helper. It
compiles the source independently, links the explicit unit closure and emits
both targets from one verified final artifact. Run from the repository root:

```sh
cmake -S examples/struct_pipeline -B /absolute/build/struct_pipeline -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/struct_pipeline --parallel 4
ctest --test-dir /absolute/build/struct_pipeline --output-on-failure --no-tests=error
```

Keep build artifacts outside the source tree. Both gates also run through the
aggregate examples build.

## Verification

Actual generated native workers 1/2 and Verilator agree on 8,989 known Work
samples. Genuine full-DUT Icarus agrees with these and 1,151 four-state Work
samples. The independent oracle uses 64-bit ripple carry/borrow and two
old-state depth-two logical deques, with a separate actual-DUT transaction
ledger. 4,455 known stimulus tokens and 536 native latent X/Z variants
cover the arithmetic; no exhaustive 64-bit or 128-bit claim is made.

The known matrix contains all 65×65 carry/borrow pairs, independent field
boundaries, packed-bit one-hots across the 63/64 boundary and mixed full-width
peers. Four-state tests isolate each field's uncertainty, preserving exact
known arithmetic in its peer, and exercise both fields uncertain together.
Computed unknown fields must be all X with no Z; only their latent values are
unconstrained.

Both streams exercise E0/E1/E2, full four-token stalls and replacements,
multiple wraps of both queue pointers, active held clocks with changed offers
and reset, two rising resets dropping four tokens each, drain and recovery.
Known history is 4476 accepted / 4468 retired / 8 reset-dropped; four-state 557 / 549 / 8. Every run ends empty, reaches peak occupancy four
and conserves all accepted tokens. Runner limits are 12,000 ticks.
The separate `bench.py` system checks a finite regular-clock known-state scenario with exact old-state output checks. This is partial system migration: the original independent drivers retain their full physical-clock, midstream-reset and four-state scenarios.

```bash
pycircuit run examples/struct_pipeline --target cpp --cycles 184 --build-dir .pycircuit_out/struct_pipeline/system-cpp
pycircuit run examples/struct_pipeline --target verilog --cycles 184 --build-dir .pycircuit_out/struct_pipeline/system-verilog
```
