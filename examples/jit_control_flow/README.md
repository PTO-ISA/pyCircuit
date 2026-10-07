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
