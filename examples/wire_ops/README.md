# Registered wire operations

`WireOps` preserves the historical `wire_ops` design: 8-bit inputs `a` and `b`
feed AND when `sel` is true and XOR otherwise. One register produces `y`, with
clocked reset value zero. Changing inputs without a rising clock does not
change the register.

The Python module declares `y: ac.u8 = 0` and calls a stateless rule. The rule
constructs `WireOpsResult(y=y)` before assigning `y`, so the result retains the
incoming state. MLIR infers the storage and its unconditional update enable.
Clock/reset pins belong to the generated physical interface and are driven by
the host testbench; the Python function takes only `a`, `b` and `sel`.

The C++ and RTL testbenches contain independent fixed golden values for 14
sampling epochs, including that case, both selector branches, unchanged-clock
hold, full-width values and reset. C++ samples the successful Work result;
an edge's transferred register value appears in the following sample.
These 14 rows cover known inputs; this example does not claim a separate
full-DUT X/Z oracle. See [actual generated excerpts](GENERATED.md).

```sh
cmake -S examples/wire_ops -B /absolute/build/wire-ops -G Ninja \
  -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/wire-ops --parallel 4
ctest --test-dir /absolute/build/wire-ops --output-on-failure --no-tests=error
```

Compile `wire_ops.py` with package prefix `example_wire_ops`, link the complete
unit closure with top `example_wire_ops.wire_ops.WireOps`, and emit both targets.
The finite runner configuration stops after the testbench finishes its trace.

## Generated system usage

`bench.py` exports `example_wire_ops.bench.ExerciseWireOps`. The explicit source
closure is `wire_ops.py`, then `bench.py`; link the system root and emit either
backend with the public `pycircuit compile`, `link`, and `emit` flow. Run
`pycircuit run examples/wire_ops --target cpp --cycles 7` or select
`--target verilog`. Each managed cycle uses the same stimulus in its low/high
sampling pair and advances fixture state on the generated edge.

All six original rising-edge operand/select triples are exercised plus a
final observation cycle. Fixed old-Q expectations independently apply bitwise
AND/XOR to the previous accepted operands. Reset-priority timing remains in
the original physical-control oracle.

The original `driver.cpp`, `rtl_tb.sv`, `config.json`, and independent oracle
models remain unchanged. Known held-level and physical-reset scenarios remain
with those module-boundary drivers. Where present, their four-state and
failure/discard matrices remain separate coverage. This regular-clock system
does not claim complete equivalence to those physical scenarios.
