# Arithmetic example

This example implements the `basics/arith` hardware: two 19-bit inputs,
a combinational 19-bit modular sum, a 16-bit lane mask of 65535 and an 8-bit
accumulator width output of 19. The original fixed configuration has eight
16-bit lanes, hence a 19-bit accumulator. No register or clock is introduced.
The source uses `import pycircuit as ac`, `ac.u19` inputs and a nominal
`ArithResult`. Unsigned bits addition wraps at 19 bits. Its ordered fields are
`sum: ac.u19`, `lane_mask: ac.u16 = 65535` and `acc_width: ac.u8 = 19`;
`ArithResult(sum=a + b)` supplies the sum and uses the two declared defaults.
The generated interface carries this value in one physical `result` packet.

With an installed pyCircuit package:

```sh
cmake -S examples/arith -B /absolute/build/arith -G Ninja \
  -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/arith --parallel 4
ctest --test-dir /absolute/build/arith --output-on-failure --no-tests=error
```

The per-design driver and RTL testbench assert independent expected values,
including the original `(1, 2) -> 3` case and overflow boundaries. The shared
helper compares the Work samples from workers 1/2 and RTL. Execution results
are recorded in the implementation inventory; this source's presence alone does
not establish acceptance. See [the generated-output guide](GENERATED.md) for
the source-owned IR, C++ and RTL produced by the public flow.

## Generated system usage

`bench.py` exports `example_arith.bench.ExerciseArith`. Compile sources in
order `arith.py`, `bench.py`, then link that system root and emit C++ or
Verilog through the public `pycircuit compile`, `link`, and `emit` commands.
Run `pycircuit run examples/arith --target cpp --cycles 10` or select
`--target verilog`. Each managed cycle checks one original known-input row
in both sampling epochs; all 10 original rows are represented.

The original `driver.cpp`, `rtl_tb.sv`, configuration, and their independent
oracles remain intact. This source bench covers the complete known-input table;
host X/Z construction and recovery checks, where present, remain in those
retained native/RTL oracles and are not claimed by the generated system run.
