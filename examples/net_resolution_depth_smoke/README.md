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
