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

The bench stores 284 immutable `Scenario` rows in one typed Table. Each
70-bit row contains the eight original inputs and seven expected output fields;
zero-valued fields use existing implicit-zero Struct initialization. Literal
rows retain all original transactions, including the complete 256-event
deterministic random stream. The author extracted these values from the frozen
original bench's syntax tree without importing or executing design code; that
extraction is separate from the independent preservation check.

The phase remains `bits[64]`, initialized to zero and incremented by the original
rule. The full phase selects its corresponding row below 284 and row 283
thereafter. The final row asserts flush and zeroes every other input and
expectation, preserving the original sustained-flush tail from phase 283 through
2**64-1. Phase arithmetic still wraps to zero. Selection does not narrow or
saturate phase, and fixture wrap introduces no new DUT reset.

The eight direct `Rob` input projections retain their original order. Seven
assertions keep their messages and `phase < 284` guard with an empty else, while
seven logs remain unconditional. Registration still calls `check_and_advance()`
then `advance(phase)`. The complete managed run remains 284 cycles and checks
acceptance, indices, retired payloads and occupancy against the retained fixed
expectations. The authored flush input retains its original meaning.

The original `driver.cpp`, `rtl_tb.sv`, `config.json`, and independent oracle
models remain unchanged. Known held-level and physical-reset scenarios remain
with those module-boundary drivers. Where present, their four-state and
failure/discard matrices remain separate coverage. This regular-clock system
does not claim complete equivalence to those physical scenarios.

## Verification and measured costs

The current standalone module and system tests pass 2/2. The module retains its
569-sample independent oracle and failed-Work, refused-sampling, discard and
retry probes. The system completes all 284 cycles / 568 sampling epochs, with
seven source-check definitions and 3,976 observations matching across native
workers 1/2 and Verilator. Full original and rewritten system observations also
match. See the current [verified module excerpts and receipt](GENERATED.md).

An independent source check compares all 4,260 input/expected field values and
284 intervals over the complete known u64 domain. It proves the sustained-flush
tail and unchanged wrap, and rejects 21 mutated candidates. The finite run does
not execute 64-bit rollover; source-domain proof and runtime history provide
separate evidence. Injected X/Z phase values are outside this known-phase proof.

The bench shrinks from 6,228 lines / 258,859 bytes to 371 lines / 39,366 bytes.
These measurements use the same machine, installed compiler and LLVM 22 C++
toolchain with `-O0`. Build and emission entries are single observations from
sequential original and rewritten pipelines. Runtime entries are medians of
three additional paired warm runs, alternating order, with all 284 cycles.
They describe this example on this machine, not a cross-platform guarantee.

| Phase | Original conditional expressions | Scenario table |
| --- | ---: | ---: |
| Compile bench | 6.62 s | 1.45 s |
| Link system | 6.48 s | 2.67 s |
| Emit C++ | 6.14 s | 2.68 s |
| Build C++ simulator | 2.46 s | 1.74 s |
| Run C++, one worker (warm median) | 1.111 s | 0.641 s |
| Emit Verilog | 5.05 s | 1.94 s |
| Build Verilog simulator | 2.22 s | 1.51 s |
| Run Verilog (warm median) | 0.036 s | 0.036 s |

The observed native runtime median decreases by about 42%; Verilog runtime is
approximately unchanged. Final IR falls from 28,198,342 to 11,074,159 bytes.
Generated C++ files total 5,513,081 → 3,691,898 bytes; generated Verilog files
total 1,173,696 → 646,451 bytes. Both backends consume the same final IR.
Complete source-import and transformed artifacts are retained locally and
reproduce the published units byte-for-byte. No compiler API, DUT storage,
scenario count, cycle limit or timeout changes are needed by this rewrite.
