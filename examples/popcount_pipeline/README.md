# Buffered population count

`Item` retains the complete 17-bit token: unsigned `value:u13` followed by
`count:u4`. The pure typed `count_item` rule copies the record and replaces only
its count. For known value `v`, the new count is the sum of its 13 individual
bits, in the range 0 through 13. The packed output token is `(v << 4) | count`,
regardless of the incoming count field. `Result` packs ready and valid above
the complete token, for 19 bits total.

There are exactly two depth-one, latency-one queue owners, with explicit
`downstream_pop` readiness, old committed head reads and no empty bypass. Their
total capacity is two complete tokens. The sink adds no storage. E0 captures
an input, E1 commits the transformed old input head to the result queue, and
E2 is the earliest external retirement. Full simultaneous replacement must
retain ordering. Held clock levels do not transfer tokens; a rising reset
drops both queued tokens. Clock/reset levels are supplied by the host driver.

The copied value field must preserve every value/known/Z bit. Any X/Z bit in
value produces four X count bits with no Z; the hidden value plane of that
computed unknown count is unspecified. Uncertainty in the old count alone is
overwritten and cannot contaminate the new count. Zero and uncertain data are
valid tokens.

## Build and verification

```sh
cmake -S examples/popcount_pipeline -B /absolute/build/popcount_pipeline -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/popcount_pipeline --parallel 4
ctest --test-dir /absolute/build/popcount_pipeline --output-on-failure --no-tests=error
```

The installed shared CMake helper independently compiles the source, links its
explicit closure and emits both targets from one verified final artifact. The
same two gates are registered in the aggregate examples build. Build products
belong outside the source tree.

Native workers 1/2 and Verilator check 16,585 known Work samples, including all
8192 values with varied old counts and all 16 prior counts for five focused
values. Genuine Icarus agrees with the known trace and 213 four-state samples.
Native tests cover 52 per-bit value uncertainty cases, 16 overwritten-count
uncertainty cases and eight dense latent variants; copied value planes are
strictly checked, including latent values behind X/Z. Computed unknown count
value planes remain unconstrained. Both paths check recovery and uncertain
stalled tokens using two old-state slots and a separate commit-qualified ledger.

Known history is 8279 accepted / 8275 retired / 4 reset-dropped; four-state
history is 88 / 84 / 4. Both finish empty and reach capacity two. E0/E1/E2,
full replacement, changed offers under held clocks and occupied reset are
checked. Runner limits are finite at 20,000 ticks. Final IR checks exactly two
D1/L1 queues with 17-bit payloads and the reviewed policies; no extra instances
or collections are introduced.
The separate `bench.py` system checks a finite regular-clock known-state scenario with exact old-state output checks. This is partial system migration: the original independent drivers retain their full physical-clock, midstream-reset and four-state scenarios.

```bash
pycircuit run examples/popcount_pipeline --target cpp --cycles 184 --build-dir .pycircuit_out/popcount_pipeline/system-cpp
pycircuit run examples/popcount_pipeline --target verilog --cycles 184 --build-dir .pycircuit_out/popcount_pipeline/system-verilog
```
