# Combinational lane boundaries

The historical design has one 32-bit input `seed` and one 32-bit output `acc`.
Three lanes produce `(seed + 6) mod 2^32`, `(seed + 12) mod 2^32`, and `seed`;
their sum is reduced to 32 bits. The third lane retains its disabled selector,
gain 7 and bias 11. The original `seed=10` case produces `acc=48`.

The original `_lane` and `_sum3` were ordinary Python helpers. Here they become
reusable module definitions with direct same-source calls, preserving each
modular boundary. `first = Lane(seed, 1, 5, 1)` creates one lane instance and
returns its typed value; the other two calls create separate instances.
`Sum3(first.y, second.y, third.y)` consumes their results. No state or clock
delay is introduced. The two enabled lanes and disabled lane can execute
independently before the summing instance.

The source uses `ac.u32` inputs and modular bits arithmetic. `Lane` and `Sum3`
return `LaneResult(y: ac.u32)`. The root returns
`BoundaryValuePortsResult(acc: ac.u32)` in one physical `result` packet.
The disabled lane uses a pure conditional expression, retaining four-state
selection semantics without a storage enable.

```sh
cmake -S examples/boundary_value_ports -B /absolute/build/boundary-value-ports -G Ninja \
  -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/boundary-value-ports --parallel 4
ctest --test-dir /absolute/build/boundary-value-ports --output-on-failure --no-tests=error
```

The design reuses existing bits arithmetic, selection and module composition.
Independent native and RTL oracles cover the
original result, intermediate and total wrap, high bits, consecutive samples,
and four-state arithmetic followed by known-input recovery. See the
[generated-output guide](GENERATED.md) for actual public-flow IR, C++ and RTL.

## Generated system usage

`bench.py` exports `example_boundary_value_ports.bench.ExerciseBoundaryValuePorts`. Compile sources in
order `boundary_value_ports.py`, `bench.py`, then link that system root and emit C++ or
Verilog through the public `pycircuit compile`, `link`, and `emit` commands.
Run `pycircuit run examples/boundary_value_ports --target cpp --cycles 16` or select
`--target verilog`. Each managed cycle checks one original known-input row
in both sampling epochs; all 16 original rows are represented.

The original `driver.cpp`, `rtl_tb.sv`, configuration, and their independent
oracles remain intact. This source bench covers the complete known-input table;
host X/Z construction and recovery checks, where present, remain in those
retained native/RTL oracles and are not claimed by the generated system run.
