# Combinational depth before one register

The transition Rule is nested in its Module and captures the declared state
directly. Combinational depth and the single register boundary are unchanged.

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

## Scenario bench

`bench.py` replaces two expanded condition trees with exact eight-bit piecewise
expressions. `in_x` uses `(phase[:8] - 4)` during phases 4–259; the independent
`expected_y` uses `(phase[:8] - 1)` during phases 5–260. Arithmetic wraps modulo
256, while every branch compares the complete 64-bit phase. Prefix cases retain
expected y=6 at phase 4; phase 260 retains y=3. From phase 263 through the largest
known u64 value, the helper tuple stays `(in_x=0, expected_y=11)`.

The expectation describes old Q and is never computed from the current input
expression or DUT output. Its frozen tail is scenario data, not a promise of
DUT steady state after the assertion window. The original counter increments
modulo 2⁶⁴, the assertion remains under `phase < 264`, the observation remains
unconditional, and checking still registers before advance.

Independent source checks compare all 528 active values and prove the complete
known-u64 piecewise domain and tail. Twenty complete native and RTL traces match
all 528 observations and the terminal result after excluding only changed
source-position metadata, including native workers 1 and 2. Both source versions
pass the two standalone CTests. See the module [excerpts and receipt](GENERATED.md).

The 264-cycle closed system includes the entire byte sweep and final registered
observation along a resetless regular-clock trajectory. Original physical clock,
reset/hold and native X/Z probes remain with the 532-sample module driver. This
source proof does not simulate a u64 rollover or establish arbitrary X/Z-phase
equivalence.

## Direct RTL four-state check

The two CTests run native workers 1/2 and known-input Verilator. They do not
enable the existing `PYC_NET_FOUR_STATE` testbench branch. Baseline and
compact builds separately passed that unchanged branch under Icarus, covering
four sparse/all-X/Z data cases, hold and reset recovery. Run it against the generated
module artifacts after the standalone build:

```sh
iverilog -g2012 -DPYC_NET_FOUR_STATE -s tb \
  -o /absolute/build/net-resolution-depth/four-state.vvp \
  /absolute/pycircuit/install/include/verilog/*.v \
  /absolute/build/net-resolution-depth/verilog/design_top.sv \
  /absolute/build/net-resolution-depth/verilog/sources/example_net_resolution_depth_smoke/net_resolution_depth_smoke.v \
  examples/net_resolution_depth_smoke/rtl_tb.sv
vvp /absolute/build/net-resolution-depth/four-state.vvp
```

## Local cost comparison

The source shrinks from 2,134 lines / 91,526 bytes to
94 lines / 2,800 bytes. Measurements use the same checkout-built
compiler, LLVM 22 C++ toolchain and `-O0` on one machine. Compile, link, emit,
build and first-run figures are single serial observations; warm medians use
three alternating full-length pairs. Every fresh process includes initialization.
These local results do not guarantee performance on other platforms or
optimization levels.

| Measurement | Expanded cases | Compact source |
| --- | ---: | ---: |
| Compile bench | 1.556 s | 0.128 s |
| Link system | 2.243 s | 0.204 s |
| Emit C++ | 2.317 s | 0.197 s |
| Emit Verilog | 1.681 s | 0.187 s |
| Build C++ | 1.029 s | 0.680 s |
| Build Verilog | 1.405 s | 1.301 s |
| First measured full C++ run | 0.804 s | 0.453 s |
| First measured full Verilog run | 0.303 s | 0.291 s |
| Warm C++ median, one worker | 0.381153 s | 0.020551 s |
| Warm Verilog median | 0.008608 s | 0.008579 s |
| Warm native peak RSS median | 3.50 MiB | 1.84 MiB |

| Artifact size | Expanded cases | Compact source |
| --- | ---: | ---: |
| Final IR | 9,826,022 bytes | 380,908 bytes |
| Generated C++ total | 1,754,878 bytes | 105,887 bytes |
| Generated Verilog total | 377,863 bytes | 27,045 bytes |

The warm RTL median is effectively unchanged in this small sample. Three pairs
do not establish statistical significance for such small differences.

Complete source-import/transformed artifacts reproduce the published units, and
the public and measured builds share identical final IR and generated outputs.
DUTs, drivers, configuration, cycle limits and timeouts remain unchanged.

```sh
pycircuit run examples/net_resolution_depth_smoke --target cpp --cycles 264
pycircuit run examples/net_resolution_depth_smoke --target verilog --cycles 264
```
