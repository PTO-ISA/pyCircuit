# Selected byte arithmetic and four increments

The design selects byte addition, subtraction, XOR or AND for op0,
op1, op2 or op3, then performs four increments. Every arithmetic boundary wraps
to eight bits. The original a1/b2/op0 case produces result7 in the same epoch.
The four rounds are fixed internal configuration; the root has no public
parameters, state or clock.

The source uses `ac.u8` operands and `ac.u2` selection, and returns the nominal
`JitResult(result: ac.u8)` in one physical `result` packet. Bits addition and
subtraction wrap at eight bits before selection and after each increment.
The pure conditional-expression chain selects addition, then subtraction,
then XOR, with AND as its final fallback; unknown selectors use four-state
mux merging. Ordinary local names share these combinational values.

```sh
cmake -S examples/jit_control_flow -B /absolute/build/jit-control-flow -G Ninja \
  -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/jit-control-flow --parallel 4
ctest --test-dir /absolute/build/jit-control-flow --output-on-failure --no-tests=error
```

The historical name is retained, but this source follows normal compile/link/
emit through common IR. Immutable local SSA stages reuse the existing bits
arithmetic, equality and select lowering. Every arithmetic value has the same
eight-bit representation at the selection boundary.
Independent native/RTL oracles check all operation choices, byte boundaries,
same-epoch input changes, unknown data/selectors and recovery. The
`PYC_JIT_FOUR_STATE` mode runs the complete generated DUT in Icarus.
See [the generated-output guide](GENERATED.md) for actual source-owned IR,
C++ and RTL from the public flow.

## Generated system usage

`bench.py` exports `example_jit_control_flow.bench.ExerciseJitControlFlow`. Compile sources in
order `jit_control_flow.py`, `bench.py`, then link that system root and emit C++ or
Verilog through the public `pycircuit compile`, `link`, and `emit` commands.
Run `pycircuit run examples/jit_control_flow --target cpp --cycles 65` or select
`--target verilog`. Each managed cycle checks one original known-input row
in both sampling epochs; all 65 original rows are represented.

The original `driver.cpp`, `rtl_tb.sv`, configuration, and their independent
oracles remain intact. This source bench covers the complete known-input table;
host X/Z construction and recovery checks, where present, remain in those
retained native/RTL oracles and are not claimed by the generated system run.
