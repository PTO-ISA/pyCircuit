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

## Generated system usage

`bench.py` exports `example_obs_points.bench.ExerciseObsPoints`. The explicit source
closure is `obs_points.py`, then `bench.py`; link the system root and emit either
backend with the public `pycircuit compile`, `link`, and `emit` flow. Run
`pycircuit run examples/obs_points --target cpp --cycles 264` or select
`--target verilog`. Each managed cycle uses the same stimulus in its low/high
sampling pair and advances fixture state on the generated edge.

Every original rising-edge input, including the complete 256-byte sweep, is
represented in 264 regular cycles with a final registered-state observation.
Fixed independent expectations distinguish the immediate combinational result
(where present) from the previous registered result. Original reset cycles
are data cycles along this regular-clock trajectory.

The original `driver.cpp`, `rtl_tb.sv`, `config.json`, and independent oracle
models remain unchanged. Known held-level and physical-reset scenarios remain
with those module-boundary drivers. Where present, their four-state and
failure/discard matrices remain separate coverage. This regular-clock system
does not claim complete equivalence to those physical scenarios.
