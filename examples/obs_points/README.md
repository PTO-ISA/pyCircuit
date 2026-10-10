# Current input and committed register observations

The original design has two observable outputs: combinational `y=(x+1) mod256`
and registered `q`, which captures y on a rising edge and resets to zero. The
testbench checks x10 -> pre y11/q0, post q11, then x20 -> pre y21/q11,
post q21. It does not declare a source probe or event schema.

The module declares ordinary state `q: ac.u8 = 0`. Its rule computes `y = x + 1`,
constructs `ObsPointsResult(y=y, q=q)`, then assigns `q = y`. Saving the result
before the assignment retains the incoming q while exposing the current
combinational y. MLIR infers one storage owner with an unconditional update
enable. Python takes only `x`; the host drives generated physical clock/reset
pins. See [actual generated excerpts](GENERATED.md).

```sh
cmake -S examples/obs_points -B /absolute/build/obs-points -G Ninja \
  -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/obs-points --parallel 4
ctest --test-dir /absolute/build/obs-points --output-on-failure --no-tests=error
```

One Work sample reflects current x in y and old Q in q. After an edge's Xfer,
a following Work with unchanged x/clock can observe the committed Q without
another rising edge. The implementation keeps one inferred register and the original reset/phase
behavior; it adds no Eval phase. Independent native/RTL oracles check both
outputs across 532 samples, wrap, hold and reset. Native and full-DUT Icarus
four-state checks preserve each field's separate known/Z mask: y may become X
while q remains known. The standard DFFE uses a constant-one enable and the RTL
test retains its original reset preamble.

## Scenario bench

`bench.py` expresses the original byte sweep as three short piecewise
expressions. The input and both expectations are independently derived from
phase. During the sweep, eight-bit subtraction wraps modulo 256; full 64-bit
phase comparisons keep high counter values from aliasing the sweep. Startup
and final exceptions remain explicit, including expected q=21 at phase 4 and
y=0 at phase 259. From phase 263 through the largest known u64 value, the
helper tuple `(x, expected_y, expected_q)` stays `(0, 1, 8)`.

The original phase counter increments modulo 2⁶⁴. The two assertions remain
under `phase < 264`, the two observations remain unconditional, and
`check_and_advance()` still registers before `advance(phase)`. Expected values
never read DUT outputs or derive from the input expression. The frozen tail is
scenario data, not a promise of DUT steady state after the assertion window.

All original rising-edge inputs, including the complete 256-byte sweep, remain
in the 264-cycle regular-clock run with its final registered-state observation.
Original reset cycles are data cycles along this resetless system trajectory.
Independent source checks compare all 792 active field values and prove the
complete known-u64 tail and wrap transition. They model eight-bit arithmetic
explicitly; this is not a simulation through 2⁶⁴ cycles.

Twenty complete native and RTL traces match all 1,056 observations and the
terminal result after excluding only changed source-position metadata; native
workers 1 and 2 are included. Baseline and compact versions each pass the
standalone module and system gates, 2/2. See the module
[excerpts and receipt](GENERATED.md).

The original 532-sample physical-clock, reset/hold and separate four-state
module oracles remain unchanged. This compact system does not claim arbitrary
X/Z-phase equivalence or complete physical-scenario migration.

## Local cost comparison

The source shrinks from 3,175 lines / 136,319 bytes to
119 lines / 3,465 bytes. Measurements use the same checkout-built
compiler, LLVM 22 C++ toolchain and `-O0` on one machine. Compile, emit, build
and first-run times are single serial observations; warm medians use three
alternating full-length pairs. Every fresh process includes initialization.
These measurements do not guarantee results on other platforms or optimization
levels.

| Measurement | Expanded cases | Compact source |
| --- | ---: | ---: |
| Compile bench | 2.527 s | 0.146 s |
| Emit C++ | 2.956 s | 0.224 s |
| Emit Verilog | 2.382 s | 0.209 s |
| Build C++ | 1.293 s | 0.606 s |
| Build Verilog | 1.619 s | 1.301 s |
| First measured full C++ run | 1.019 s | 0.313 s |
| First measured full Verilog run | 0.293 s | 0.268 s |
| Warm C++ median, one worker | 0.500688 s | 0.027709 s |
| Warm Verilog median | 0.012424 s | 0.012543 s |
| Warm native peak RSS median | 4.36 MiB | 1.86 MiB |

| Artifact size | Expanded cases | Compact source |
| --- | ---: | ---: |
| Final IR | 14,126,432 bytes | 476,052 bytes |
| Generated C++ total | 2,598,398 bytes | 126,740 bytes |
| Generated Verilog total | 559,461 bytes | 32,486 bytes |

The Verilog warm median increases by about 1% (0.012424 to 0.012543 s), despite
large reductions in source and generated-code size. Three pairs do not establish
a statistically significant difference; no RTL runtime speedup is claimed.

Complete source-import/transformed artifacts reproduce the published units, and
the public and measured builds share identical final IR and generated outputs.
DUTs, drivers, configuration, cycle limits and timeouts remain unchanged.

```sh
pycircuit run examples/obs_points --target cpp --cycles 264
pycircuit run examples/obs_points --target verilog --cycles 264
```
