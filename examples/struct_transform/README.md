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

## Generated system usage

`bench.py` exports `example_struct_transform.bench.ExerciseStructTransform`. The explicit source
closure is `struct_transform.py`, then `bench.py`; link the system root and emit either
backend with the public `pycircuit compile`, `link`, and `emit` flow. Run
`pycircuit run examples/struct_transform --target cpp --cycles 72` or select
`--target verilog`. Each managed cycle uses the same stimulus in its low/high
sampling pair and advances fixture state on the generated edge.

This bench runs all 71 original rising-edge data/valid inputs plus a
final observation cycle. Literal expectations are derived from the retained
independent C++ scoreboard along the regular-clock, resetless trajectory; they
never drive DUT results. Original reset edges are ordinary data cycles here.
All original deterministic traffic and arithmetic boundaries remain represented.

The original `driver.cpp`, `rtl_tb.sv`, `config.json`, and independent oracle
models remain unchanged. Known held-level and physical-reset scenarios remain
with those module-boundary drivers. Where present, their four-state and
failure/discard matrices remain separate coverage. This regular-clock system
does not claim complete equivalence to those physical scenarios.
