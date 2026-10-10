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

## Scenario bench

`bench.py` stores all 344 original scenarios in an immutable typed Table. Its
6-field, 20-bit `Stimulus` retains the original field order, including payloads
supplied or observed while valid is zero. Row 344 is all zero, preserving the
helper fallback for every known u16 phase from 344 through 65,535. This fallback
is distinct from the system counter, which holds at phase 343.

The complete system retains its ordered DUT inputs, 3 unconditional assertions,
4 unconditional observations and `advance(phase)` before `exercise()`.
The regular-clock run still lasts 344 cycles.

Independent source checks compare all 393,216 field values across the complete
known-u16 helper domain. Twenty complete native and RTL traces agree on all
2,752 observations and the terminal result after excluding only changed
source-position metadata; native workers 1 and 2 are included. Baseline and
compact versions each pass all 3 standalone gates. See the module
[excerpts and receipt](GENERATED.md).

The unchanged physical drivers retain 658 known and 213 four-state Work samples,
including the separate original smoke, nonedge observations, drain, extended
history and terminal unknown-control cases described above.

The closed system covers a regular-clock known-state trajectory. Full physical
reset/held-clock scenarios remain with their independent module drivers;
known-u16 source equivalence does not prove arbitrary X/Z-phase equivalence.

## Local cost comparison

The source shrinks from 2,309 lines / 58,406 bytes to
408 lines / 41,572 bytes. Measurements use the same checkout-built
compiler, LLVM 22 C++ toolchain and `-O0` on one machine. Compile, link, emit,
build and first-run values are single serial observations; warm medians use
three alternating full-length pairs. Every fresh process includes initialization.
These local results do not guarantee performance on other platforms or
optimization levels.

| Measurement | Expanded cases | Typed Table |
| --- | ---: | ---: |
| Compile bench | 1.638 s | 1.814 s |
| Link system | 2.434 s | 3.054 s |
| Emit C++ | 2.147 s | 2.635 s |
| Emit Verilog | 1.771 s | 2.202 s |
| Build C++ | 1.321 s | 2.647 s |
| Build Verilog | 1.549 s | 2.509 s |
| First measured full C++ run | 0.850 s | 0.281 s |
| First measured full Verilog run | 0.441 s | 0.275 s |
| Warm C++ median, one worker | 0.860111 s | 0.047933 s |
| Warm Verilog median | 0.047082 s | 0.040410 s |
| Warm native peak RSS median | 4.44 MiB | 3.08 MiB |

| Artifact size | Expanded cases | Typed Table |
| --- | ---: | ---: |
| Final IR | 10,507,749 bytes | 7,599,461 bytes |
| Generated C++ total | 2,678,452 bytes | 2,060,014 bytes |
| Generated Verilog total | 586,790 bytes | 430,063 bytes |

Native execution, source size, final IR and generated-code size improve in this
comparison. Several single-run compile, link, emission and build timings
increase; the table reports those costs without attributing their cause from
one observation. This is not an across-the-board performance improvement, and
three warm pairs do not establish statistical significance for small RTL changes.

Complete source-import/transformed artifacts reproduce the published units, and
the public and measured builds share identical final IR and generated outputs.
DUTs, drivers, configuration, cycle limits and timeouts remain unchanged.

```sh
pycircuit run examples/fifo_loopback --target cpp --cycles 344
pycircuit run examples/fifo_loopback --target verilog --cycles 344
```
