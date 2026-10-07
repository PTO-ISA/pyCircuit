# Two-stage unsigned increment pipeline

The original spelling `int` resolves to unsigned fixed 64-bit hardware, so
this source uses `ac.u64`. Its inline pure expression increments each token
modulo 2^64: UINT64_MAX becomes zero, and 2^63-1 becomes 2^63. It preserves
the full 64-bit payload. `QueueResult` packs data at bits 0 through 63, valid
at bit 64 and ready at bit 65, for 66 bits total.

Two explicit queues each have depth 2 and availability latency 1. They hold
four complete tokens in total, use old committed head reads, have no empty
bypass and declare `downstream_pop` readiness. The sink adds no storage.
E0 captures an input, E1 commits its incremented old head into the result
queue, and E2 is the earliest external retirement. Depth 2 adds capacity,
without adding a mandatory latency stage.

For old input/result occupancies n0/n1, result readiness is `n1 < 2` or an
old result-head pop. An old input head advances only when that result queue
is ready. Input readiness is `n0 < 2` or an old input-head advance. Full
simultaneous replacement retains FIFO order and occupancy through both
depth-two pointer wraps. Held clock levels do not commit despite changed
offers/take; rising reset clears up to four tokens. The host drives physical
clock/reset levels; the Python source adds no clock or transaction proposal.

Any X/Z bit in an input produces 64 X data bits with no Z. The hidden value
plane of that computed unknown is unspecified. Zero and unknown data remain
valid tokens.

## Build route

Use the accepted installed toolchain and the public example CMake helper,
which compiles the source independently, links its explicit unit closure and
emits both targets from one verified final artifact. Run from the repository
root:

```sh
cmake -S examples/queue_pipeline -B /absolute/build/queue_pipeline -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/queue_pipeline --parallel 4
ctest --test-dir /absolute/build/queue_pipeline --output-on-failure --no-tests=error
```

Keep build artifacts outside the source tree. Both gates also run through the
aggregate examples build.

## Verification

Actual generated native workers 1/2 and Verilator agree on 483 known Work
samples. Genuine full-DUT Icarus agrees with these and 607 four-state Work
samples. The independent oracle uses 64-bit ripple carry/borrow and two
old-state depth-two logical deques, with a separate actual-DUT transaction
ledger. 202 known stimulus tokens and 264 native latent X/Z variants
cover the arithmetic; no exhaustive 64-bit or 128-bit claim is made.

Every carry length 0 through 64, zero/MAX/high-bit/alternating values and
full-width deterministic mixed values are exercised. Four-state tests cover
every input bit as X and Z with both latent values, all-X/all-Z and dense mixed
patterns. Computed unknown data must be all X with zero Z; its hidden value is
not asserted.

Both streams exercise E0/E1/E2, full four-token stalls and replacements,
multiple wraps of both queue pointers, active held clocks with changed offers
and reset, two rising resets dropping four tokens each, drain and recovery.
Known history is 223 accepted / 215 retired / 8 reset-dropped; four-state 285 / 277 / 8. Every run ends empty, reaches peak occupancy four
and conserves all accepted tokens. Runner limits are 12,000 ticks.