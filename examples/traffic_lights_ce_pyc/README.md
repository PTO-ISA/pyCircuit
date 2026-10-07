# Traffic lights with pause and emergency control

[Generated MLIR, C++ and RTL examples](GENERATED.md)

This example implements the `applications/traffic_lights_ce_pyc` circuit,
including its existing display conversion and countdown timing quirks. The
source-owned configuration is a four-edge divider and durations 3/1/2/1; the
original public parameter set is empty.

`TopTrafficLights(go: ac.u1, emergency: ac.u1)` owns five ordinary variables:
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

The second test links `EncodeCountdown` from the **same published source unit**
as another selected root. Separate generated products and executables verify
all eight known inputs and all 64 three-bit four-state patterns. No substitute
encoder source or controller debug ports are added. Both checks use the current
public compile/link/emit flow and Runtime. Acceptance requires actual native
worker-1/worker-2 and RTL execution. The verified controller has 431 matching
Work samples and six genuine Icarus X/Z scenarios; all 64 encoder patterns and
immediate known-value recovery pass. The generated excerpts above come from
these actual successful runs.
