# Three combinational incrementers

The historical `features/hier_modules` source called an ordinary Python helper
three times. Each helper added one and kept the low eight bits; it declared no
registers, clocks or formal hardware module instances. This implementation expresses
the same three combinational sections as one reusable module definition and
three connected instances through direct same-source calls. The result is
`(x + 3) & 255` in the same Work epoch. Every call returns an
`IncrementResult(y: ac.u8)` value; its field connects to the following call.
The root input is `ac.u8`, and its physical `result` packet retains that same
nominal result type. Each `ac.u8` addition wraps at eight bits.

```sh
cmake -S examples/hier_modules -B /absolute/build/hier-modules -G Ninja \
  -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/hier-modules --parallel 4
ctest --test-dir /absolute/build/hier-modules --output-on-failure --no-tests=error
```

The original `x=1` to `y=4` case, rollover at each section and consecutive
samples without register delay are checked by independent native/RTL oracles.
Both worker settings execute the real generated modules. The dependency chain
requires ordered Work; this example does not claim parallel speedup.

Each stage uses the existing fixed-width bits Add lowering. No new
frontend operation, pass or backend special case is required. Execution and
the native and RTL tests bind the behavior to the generated artifacts.
See [the generated-output guide](GENERATED.md) for actual source-owned IR,
C++ and RTL from the public flow.

## Generated system usage

`bench.py` exports `example_hier_modules.bench.ExerciseHierModules`. Compile sources in
order `hier_modules.py`, `bench.py`, then link that system root and emit C++ or
Verilog through the public `pycircuit compile`, `link`, and `emit` commands.
Run `pycircuit run examples/hier_modules --target cpp --cycles 12` or select
`--target verilog`. Each managed cycle checks one original known-input row
in both sampling epochs; all 12 original rows are represented.

The original `driver.cpp`, `rtl_tb.sv`, configuration, and their independent
oracles remain intact. This source bench covers the complete known-input table;
host X/Z construction and recovery checks, where present, remain in those
retained native/RTL oracles and are not claimed by the generated system run.
