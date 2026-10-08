# Combinational depth before one register

The original design feeds four consecutive additions of one into a single
eight-bit register, with rising-edge clock and reset to zero. Its next value is
`(in_x + 4) mod 256`. The Python source keeps four additions as local values and
one ordinary `y: ac.u8 = 0` state variable. The old `domain.next()` and `q <<= d3`
did not add intermediate registers;
`<<=` was register assignment, not a hardware shift.

The rule saves `NetResolutionDepthResult(y=y)` before assigning `y = d3`.
Each byte addition wraps modulo 256. MLIR infers one storage owner and an
unconditional update enable; the locals `d0` through `d3` remain combinational.
The Python module takes only `in_x`; the generated physical clock/reset pins
are driven by the host testbench. See [actual generated excerpts](GENERATED.md).

```sh
cmake -S examples/net_resolution_depth_smoke -B /absolute/build/net-resolution-depth -G Ninja \
  -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/net-resolution-depth --parallel 4
ctest --test-dir /absolute/build/net-resolution-depth --output-on-failure --no-tests=error
```

The driver explicitly drives clock levels. A successful Work sample observes
old Q; a rising-edge Xfer becomes visible on the following Work without an
additional edge. The original reset and `in_x=1: 0→5`, `in_x=2: 5→6` cases
remain in the 532-sample oracle, alongside every eight-bit input, repeated-level hold,
reset priority and native X/Z data behavior. Native workers 1/2 are compared
with known-input RTL. `PYC_NET_FOUR_STATE` additionally checks the complete DUT
and inferred standard DFFE under Icarus for sparse/all X/Z data, capture,
hold, next-edge recovery and reset priority. These current direct commands
supersede the earlier parser-limit claim. The DFFE enable is always one, retaining
the original register behavior and reset preamble.

## Generated system usage

`bench.py` exports `example_net_resolution_depth_smoke.bench.ExerciseNetResolutionDepthSmoke`. The explicit source
closure is `net_resolution_depth_smoke.py`, then `bench.py`; link the system root and emit either
backend with the public `pycircuit compile`, `link`, and `emit` flow. Run
`pycircuit run examples/net_resolution_depth_smoke --target cpp --cycles 264` or select
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
