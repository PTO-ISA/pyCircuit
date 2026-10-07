# Typed FIFO loopback

One queue preserves the full8-bit payload with depth2, latency1 and explicit
`downstream_pop` readiness. There is no transform, extra queue or empty bypass.
The logical results remain in_ready/out_valid/out_data, now fields of typed
`FifoResult`: ready9, valid8 and data7:0 in the 10-bit packed result. This is the
existing struct transport; drivers map observations accordingly. No separate
consumer requiring the old three physical output pins was found. Python does
not expose clock/reset or storage proposals.

A full pop/push edge retires the old head, appends the new token and keeps
occupancy unchanged. Available-head valid stays asserted when out_ready is0;
it is not the transfer/fire signal. Empty data after initialization is zero.
Rising reset empties the queue; held and falling levels do not transfer. Raw
payload value/known/Z planes are preserved.

## Original smoke and observation phases

The original testbench uses two asserted-reset cycles, one deasserted settling
cycle, then four constant-stream cycles with both requests1 and data0x2A. It
contains no assertions. Preserve that14-sample sequence with new independent
assertions: four births, three retirements and one remaining token. An extended
test may drain independently; it must not alter the original smoke history.

Old post-commit observations map to the following nonedge Work sample in the
current runner, without adding a clock edge or storage stage. The old native
constructor was known-empty/two-state; current native and RTL construction is
cold X until reset, and current four-state failure behavior is checked separately.
The normal reset-first smoke retains the original observable behavior.

## Build

```sh
cmake -S examples/fifo_loopback -B /absolute/build/fifo_loopback -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/fifo_loopback --parallel 4
ctest --test-dir /absolute/build/fifo_loopback --output-on-failure --no-tests=error
```

The installed shared helper compiles this source independently, links its
explicit closure and emits both targets from one final artifact. Build outputs
stay outside the source tree. The main and four-state gates also run in the aggregate examples build.

## Verification

Actual generated native workers1/2, Verilator and genuine Icarus agree on658
known Work samples:14 original SMOKE drive rows,7 labelled held-clock OBS_SMOKE
reads,2 separate DRAIN rows and635 EXT rows. The extra observations add no clock
edge or transfer. Smoke accepts4/retires3/leaves1; the separate drain retires that
last token. EXT accepts301/retires297/reset-drops4 and finishes empty at peak2.
All256 data values, capacity, old-head/full replacement and repeated wraps are
covered.

Native and Icarus also agree on213 FOUR samples, with85 accepted/81 retired/4
reset-dropped, empty finish and peak2. Forty raw patterns include every bit as
X/Z with both latent values and dense patterns; all copied value/known/Z bits
are exact. Three terminal unknown-control cases require Reset before further
Step, while RTL negatives use isolated processes. Cold X is labelled as the
current/RTL contract separately from the reset-first historical smoke. All
runners are bounded; the main config allows4000 ticks.