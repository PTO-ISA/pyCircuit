# Traffic lights with pause and emergency control

[Generated MLIR, C++ and RTL examples](GENERATED.md)

This controller retains its display conversion and countdown timing quirks. The
source-owned configuration is a four-edge divider and durations 3/1/2/1; the
original public parameter set is empty.

`TopTrafficLights(go: bits[1], emergency: bits[1])` owns five ordinary variables:
prescaler u2=0, phase u2=0, EW count u3=3, NS count u3=4 and blink u1=0.
Two direct calls to the stateless `EncodeCountdown` module read old counter
values. One rule computes all state updates. Arithmetic and width-conversion
temporaries live inside rules, so they allocate no additional storage.

The typed `TrafficResult` contains EW/NS eight-bit displays and red/yellow/green
bits for each direction. These outputs observe old state and current emergency
combinationally. Emergency immediately forces both displays to `0x88`, both reds
on and the other lights off. It pauses the main counters without resetting them.
Pause also holds the main counters; blink separately clears outside yellow and
toggles only on a yellow tick.

Conditional expressions preserve the original data-selection behavior, including
X/Z merging. The rule saves its result before assigning the five state owners.
The second blink selection complements the original blink value. A Work sample
observes the state before that edge's Xfer; the next Work sees committed values.
The Python source exposes no clock/reset or current/next state API.

## Historical quirks retained

The former three-bit literal constructor truncated the BCD thresholds before
width promotion. The new source spells out those masks and keeps the complete
comparison/select chain. Its actual count-to-display mapping is:

| Count | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Display hex | 48 | 49 | 58 | 59 | 5A | 5B | 5C | 5D |

Reset therefore displays `0x59/0x5A`. Every three-bit count containing X or Z
produces `0xxxxxxx`; simplifying the comparison chain from known values would
change that behavior. Correcting the display algorithm is a separate change.

Transitions test the old count after it has reached zero, giving phase lengths
4/2/3/2 ticks and a full cycle of 44 enabled rising edges. EW/NS green, then yellow,
alternate with the original blink and reload ordering. The four-edge divider's
position survives pauses and emergency intervals.

## Build and verify

```sh
cmake -S examples/traffic_lights_ce_pyc -B /absolute/build/traffic-lights -G Ninja \
  -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/traffic-lights --parallel 4
ctest --test-dir /absolute/build/traffic-lights --output-on-failure --no-tests=error
```

The controller uses the shared generated-DUT SystemRunner. Its independent
checker derives outputs from an 11-tick state table and covers complete cycles,
pause/emergency, reset, discard/retry and four-state controls.

The encoder test links `EncodeCountdown` from the **same published source unit**
as another selected root. Separate generated products and executables verify
all eight known inputs and all 64 three-bit four-state patterns. No substitute
encoder source or controller debug ports are added. Both checks use the current
public compile/link/emit flow and Runtime. Acceptance requires actual native
worker-1/worker-2 and RTL execution. The verified controller has 431 matching
Work samples and six genuine Icarus X/Z scenarios; all 64 encoder patterns and
immediate known-value recovery pass. The generated excerpts above come from
these actual successful runs.

## Scenario bench

`bench.py` exports `example_traffic_lights_ce_pyc.bench.ExerciseTopTrafficLights` and still runs
209 complete cycles. Interval data reduces source repetition; it does not skip
simulation epochs, add an interval counter or change the u64 phase update.

The ten input/expected fields use 55 maximal constant intervals. Each row has
an inclusive `end: bits[64]`; the first row covers phases 0–3 and the last covers
207 through UINT64_MAX. The existing `Table.first` query selects the lowest
matching index with `phase <= row.end`, followed by an ordinary row projection.
The final endpoint guarantees a match for every known u64 phase.

Pause/resume boundaries at 90/91 and 95/96, emergency entry at 130/131 and
release at 139, the single-cycle emergency at 178, and the final display/light
change at 206/207 remain exact. All 2,090 original active field values are
preserved. The original truncated countdown/display behavior is unchanged.

The source retains its original DUT calls, guards, assertions, messages, logs
and check-before-advance registration. Independent source proof verifies exact
interval endpoints, typed payloads, complete known-u64 coverage and the remaining
AST. Arbitrary X/Z phase equivalence is not claimed; the closed phase retains
its known initialization and increment. `found` uses the existing query binding
and adds no control or logging.

Twenty complete native/RTL system traces preserve all 3,344 observations and the
terminal result, including native workers 1/2, after excluding only changed
source-position metadata. Regular-clock system tests remain distinct from the
original physical clock/reset histories.

Both versions pass the three existing CTests: controller, generated system and
encoder. The controller retains 431 known Work samples and its native lifecycle,
failure and X/Z checks. Explicit runs of its original Icarus macro preserve six
four-state cases and complete output. The encoder separately checks eight known
and 56 X/Z patterns from the same published source unit; its CTest already runs
Icarus. Encoder coverage does not substitute for the controller macro or system.

See [generated excerpts and verification inputs](GENERATED.md).

## Local cost comparison

The source shrinks from 514 lines / 17,827 bytes to
138 lines / 9,044 bytes. Measurements use the same checkout-built
compiler, LLVM 22 C++ toolchain and `-O0` on one machine. Compile, link, emit,
build and first-run values are single serial observations; warm medians use
three alternating full-length pairs. Every fresh process includes initialization.
These local results do not guarantee performance on other platforms or
optimization levels.

| Measurement | Expanded expressions | Interval Table |
| --- | ---: | ---: |
| Compile bench | 0.487 s | 0.347 s |
| Link system | 0.893 s | 0.656 s |
| Emit C++ | 0.786 s | 0.647 s |
| Emit Verilog | 0.691 s | 0.511 s |
| Build C++ | 0.897 s | 0.801 s |
| Build Verilog | 1.434 s | 1.372 s |
| First measured full C++ run | 0.482 s | 0.522 s |
| First measured full Verilog run | 0.327 s | 0.332 s |
| Warm C++ median, one worker | 0.121407 s | 0.069799 s |
| Warm Verilog median | 0.032170 s | 0.032961 s |
| Warm native peak RSS median | 2.45 MiB | 2.31 MiB |

| Artifact size | Expanded expressions | Interval Table |
| --- | ---: | ---: |
| Final IR | 3,305,841 bytes | 2,424,882 bytes |
| Generated C++ total | 851,130 bytes | 832,219 bytes |
| Generated Verilog total | 164,711 bytes | 147,975 bytes |

Recorded first-run regressions: native 0.482 → 0.522 s, RTL 0.327 → 0.332 s.
They remain separate from warm medians; one observation does not identify a cause.

Warm RTL increases from 0.032170 to 0.032961 s (2.46%, about 0.792 ms).
All three paired runs are slower; this consistent observed regression is retained
as a known tradeoff alongside smaller source/artifacts and improved native runs.
The available data does not establish its cause.

Three warm pairs describe these local runs; they do not establish causes or
performance across platforms.
Complete source-import/transformed artifacts reproduce the published units, and
the public and measured builds share identical final IR and generated outputs.
DUTs, drivers, configuration, cycle limits and timeouts remain unchanged.

```sh
pycircuit run examples/traffic_lights_ce_pyc --target cpp --cycles 209
pycircuit run examples/traffic_lights_ce_pyc --target verilog --cycles 209
```
