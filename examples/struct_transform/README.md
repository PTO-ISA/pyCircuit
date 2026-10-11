# Registered packet transform

[Generated MLIR, C++ and RTL examples](GENERATED.md)

This example implements the `features/struct_transform` hardware with its
fixed 32-bit word width. One `Packet` register stores `op: ac.u4`, `dst: ac.u6`,
`word: ac.u32` and `valid: ac.u1`. Each transaction captures the four scalar
inputs into that register. The typed result reports the old registered `op`,
`dst` and `valid`, and reports `(old.word + old.op + 1) mod 2^32` as `word`.

The reset image is recursively zero. Consequently, the first successful Work
after reset reports `op=0`, `dst=0`, `word=1` and `valid=0`; an input captured on
a rising edge becomes the old state observed by a later Work. The arithmetic is
fixed-width unsigned bits arithmetic, so unknown bits remain in the computation
and known overflow wraps at 32 bits.
The rule's `op_wide: ac.u32 = state.op` explicitly zero-extends the four-bit
opcode before addition. This existing typed boundary preserves its low X/Z
bits; the arithmetic itself does not silently promote mixed widths.

`StructTransform` directly calls the same-source state-owning
`StructTransformStorage` module. The stateless root inherits the generated
sampling and reset pins. The Python source contains no clock, reset, DFFE or
current/next API. Its public inputs are `op`, `dst`, `word` and `valid`, and its
single physical `result` value is the ordered nominal `Packet` struct.

Build this design through the shared example helper and an installed pyCircuit
package:

```sh
cmake -S examples/struct_transform -B /absolute/build/struct-transform -G Ninja \
  -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/struct-transform --parallel 4
ctest --test-dir /absolute/build/struct-transform --output-on-failure --no-tests=error
```

Acceptance requires the generated C++ model with one and two workers and the
RTL model to agree with independent old-Q and wraparound oracles. Source
presence alone is not execution evidence.

## Scenario bench

`bench.py` stores the original 72 scenarios in an immutable typed Table with
eight fields: four inputs and four independent expectations, each group retaining
widths 4/6/32/1. It selects the current row below phase 72 and row 71 thereafter.
There is no zero sentinel. Row zero expects the reset result word=1; every known
phase from 71 onward keeps inputs `(1, 2, 3, 1)` and expectations `(1, 2, 5, 1)`.
The final row must not be confused with row 70's still-pending registered result.

The packet's valid field remains data, not an update enable. The four ordered
DUT arguments and all field checks remain intact, including valid=0 scenarios.
The u64 counter still increments modulo 2⁶⁴; four assertions remain under
`phase < 72`, four observations remain unconditional, and checking registers
before advance.

Independent source checks compare all 576 active values and prove the complete
known-u64 nonzero tail. Twenty complete native and RTL traces match all 576
observations and the terminal result after excluding only changed source-position
metadata, including native workers 1 and 2. Both versions pass the two standalone
CTests. See the module [excerpts and receipt](GENERATED.md).

The 72-cycle closed system retains all 71 original rising-edge inputs plus the
final observation along a resetless regular-clock trajectory. Original module
drivers retain 149 known Work samples and their discard, valid X/Z and unknown
word/op arithmetic probes. The source proof does not simulate a u64 rollover or
establish arbitrary X/Z-phase equivalence.

## Direct RTL four-state check

The two CTests run native workers 1/2 and known-input Verilator. They do not
enable the existing `PYC_STRUCT_TRANSFORM_FOUR_STATE` testbench branch. Baseline and
compact builds separately passed that unchanged branch under Icarus, covering
two valid X/Z and four word/op arithmetic cases, hold and reset recovery. Run it against the generated
module artifacts after the standalone build:

```sh
iverilog -g2012 -DPYC_STRUCT_TRANSFORM_FOUR_STATE -s tb \
  -o /absolute/build/struct-transform/four-state.vvp \
  /absolute/pycircuit/install/include/verilog/*.v \
  /absolute/build/struct-transform/verilog/design_top.sv \
  /absolute/build/struct-transform/verilog/sources/example_struct_transform/struct_transform.v \
  examples/struct_transform/rtl_tb.sv
vvp /absolute/build/struct-transform/four-state.vvp
```

## Local cost comparison

The source shrinks from 2,036 lines / 74,265 bytes to
129 lines / 10,122 bytes. Measurements use the same checkout-built
compiler, LLVM 22 C++ toolchain and `-O0` on one machine. Compile, link, emit,
build and first-run figures are single serial observations; warm medians use
three alternating full-length pairs. Every fresh process includes initialization.
These local results do not guarantee performance on other platforms or
optimization levels.

| Measurement | Expanded cases | Compact source |
| --- | ---: | ---: |
| Compile bench | 1.610 s | 0.369 s |
| Link system | 2.362 s | 0.615 s |
| Emit C++ | 2.034 s | 0.576 s |
| Emit Verilog | 1.662 s | 0.482 s |
| Build C++ | 1.104 s | 0.784 s |
| Build Verilog | 1.430 s | 1.307 s |
| First measured full C++ run | 0.417 s | 0.287 s |
| First measured full Verilog run | 0.524 s | 0.278 s |
| Warm C++ median, one worker | 0.103926 s | 0.009365 s |
| Warm Verilog median | 0.008483 s | 0.008451 s |
| Warm native peak RSS median | 3.62 MiB | 2.16 MiB |

| Artifact size | Expanded cases | Compact source |
| --- | ---: | ---: |
| Final IR | 9,513,490 bytes | 2,209,765 bytes |
| Generated C++ total | 1,879,088 bytes | 653,955 bytes |
| Generated Verilog total | 404,461 bytes | 133,362 bytes |

The warm RTL median is effectively unchanged in this small sample. Three pairs
do not establish statistical significance for such small differences.

Complete source-import/transformed artifacts reproduce the published units, and
the public and measured builds share identical final IR and generated outputs.
DUTs, drivers, configuration, cycle limits and timeouts remain unchanged.

```sh
pycircuit run examples/struct_transform --target cpp --cycles 72
pycircuit run examples/struct_transform --target verilog --cycles 72
```
