# Heterogeneous barrier pipeline

Independent `LeftToken(value: u16)` and `RightToken(value: u32)` streams each
have a depth-two input and depth-two output queue. All four queues have latency
one and explicit `downstream_pop` readiness. Both old input heads move together
only when both outputs have room. Output consumers retire independently. The
two nominal payload types, positional pairing, eight physical contributor slots
and four slots per stream are preserved.

The implementation uses four ordinary `ac.queue` allocations and cross-connected
ready/valid expressions. Existing field-sensitive dependency analysis verifies
the completed connections. There is no extra payload register or barrier
primitive. Both backends consume the same verified common IR.

`BarrierPipeline(left_valid, left_data, left_take, right_valid, right_data,
right_take)` returns `BarrierResult`: left_ready at bit 51, left_valid at 50,
left_data at 49:34, right_ready at 33, right_valid at 32 and right_data at 31:0.
Each token retains its named `value` field. Python clock/reset and storage
proposal details stay hidden.

Work observes old heads; Xfer commits an accepted edge after whole-system
checking. Full replacement retires the old head and retains occupancy. There
is no empty bypass. Rising reset empties all four queues; held and falling
levels do not transfer. Initialized empty data is zero. Payload transport
preserves the complete value/known/Z planes, without arithmetic conversion.

## Build and verification

```sh
cmake -S examples/barrier_pipeline -B /absolute/build/barrier_pipeline -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/barrier_pipeline --parallel 4
ctest --test-dir /absolute/build/barrier_pipeline --output-on-failure --no-tests=error
```

The shared installed helper independently compiles this source, links the explicit
closure and emits both targets. Independent tests must cover unequal input
arrivals, one blocked output preventing both moves, independent output
retirement, per-stream token conservation, capacity, wrap, raw four-state
transport, reset, held clocks and whole-system failure/discard. Native workers
1 and 2, Verilator and genuine Icarus use the actual generated DUT.

Standalone generated-DUT gates pass: native workers 1/2, Verilator and Icarus
agree on 699 known Work samples; native and genuine Icarus agree on another
619 four-state samples. Native checks all copied value/known/Z planes, including
latent values. Known histories accept 313, retire 305 and reset-drop eight tokens
per stream; four-state histories accept 273, retire 265 and drop eight per
stream. Both finish empty and reach the full eight-slot capacity.

The paired ledger records 309 known and 269 four-state atomic movements.
Each schedule requires independently retired outputs in both directions,
blocked-output stalls in both directions, full replacements and two full resets.
Public-owner probes exercise explicit and late-sibling discard/reprepare.
Separate failed-system tests check terminal failure and mandatory Reset recovery;
an isolated RTL process checks the effective-unknown-transfer diagnostic.