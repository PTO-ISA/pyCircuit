# Reorder buffer

`rob.py` describes four entries and three ordinary persistent variables in
`RobStorage`. `Rob` calls that module directly and returns its `RobResult` value.
The stateless parent inherits the child's sampling/reset connections through
compiler analysis. A top-level rule computes completion, retirement and
allocation in that order.
The compiler infers storage, per-variable enables and connections. The Python
source contains no clock, reset, register primitive or proposal API.

The rule starts with `result = RobResult()` and assigns only fields whose values
change. Result fields default to zero. Constructed entries default to valid,
so allocation only supplies the tag; retirement explicitly supplies `valid=0`.
The table's explicit `init=0` and flush still produce all-zero invalid entries,
independent of `Entry.valid`'s nonzero constructor default. No dictionary of
string-named outputs or repeated final field copy is required.

Completion may arrive out of order. A valid entry accepts it only when its tag
matches. Retirement removes the completed head; it can consume a completion
from the same transaction. Allocation then uses the space just released, even
when the buffer began full. Flush takes priority and clears all state. Tags
reject mismatches but do not solve reuse of an identical tag after wraparound.

`result.count` and accepted-event fields describe the rule's final local calculation.
They are sampled from successful Work; Xfer commits the proposed state without
recomputing outputs. The testbench alternates a low no-op sample and a high
transaction sample. The generated physical inputs `pyc_clk` and `pyc_rst` use
the existing typed DUT interface and are absent from the authoring source.
The existing identifier encoder spells these reserved names
`pyc_7079635f636c6b` and `pyc_7079635f727374` in generated C++/RTL; the driver
uses those names without changing the compiler's naming rules.
The generated DUT carries one typed `result` struct. The testbench reads its
members (or packed RTL fields in declaration order) and compares the same seven
values with the unchanged 569-sample independent oracle.

Build against a fresh compiler/runtime installation:

```sh
cmake -S examples/rob -B /absolute/build/rob -G Ninja \
  -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/rob --parallel 4
ctest --test-dir /absolute/build/rob --output-on-failure --no-tests=error
```

The unchanged shared example helper compiles the Python source, publishes and
links its unit, emits both backends, and runs the generated C++ model with one
and two workers against an independent scoreboard. The RTL testbench uses the
same finite stimulus and checks its own expected results. The C++ driver also
checks failed Work, refused sampling, discard and retry of generated state.

This example exercises one domain and one state-writing rule with statically
bounded indices. It does not establish general dynamic index checks, multiwriter
arbitration, cross-domain transactions or complete diagnostics for every masked
unknown/no-write control path. Unknown enables reaching standard storage obey
its existing failure and whole-system discard contract.

## Generated system usage

`bench.py` exports `example_rob.bench.ExerciseRob`. The explicit source
closure is `rob.py`, then `bench.py`; link the system root and emit either
backend with the public `pycircuit compile`, `link`, and `emit` flow. Run
`pycircuit run examples/rob --target cpp --cycles 284` or select
`--target verilog`. Each managed cycle uses the same stimulus in its low/high
sampling pair and advances fixture state on the generated edge.

All 284 original transactions, including the complete 256-event deterministic
random stream, run for 284 cycles. Every acceptance, index, retired payload and
occupancy is checked against fixed vectors derived independently from the
retained C++ scoreboard. The authored flush input retains its original meaning.

The original `driver.cpp`, `rtl_tb.sv`, `config.json`, and independent oracle
models remain unchanged. Known held-level and physical-reset scenarios remain
with those module-boundary drivers. Where present, their four-state and
failure/discard matrices remain separate coverage. This regular-clock system
does not claim complete equivalence to those physical scenarios.
