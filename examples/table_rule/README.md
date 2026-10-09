# Atomic table replacement

`table_rule.py` keeps two persistent `Entry` rows. Each incoming record selects
a row with its one-bit index, replaces the complete row, and returns the old
record. The saved old value remains a snapshot even after the rule assigns the
new value. Both initial rows are zero; row one's initial stored index is also
zero. The seven-bit payload and one-bit index retain their original layout.

The source queue holds two records and the result queue holds one. Both have
latency one and explicitly allow full replacement with `downstream_pop`.
Starting empty, an input accepted on E0 installs on E1 and its result can first
retire on E2. Under steady flow, one record can install per edge. A full stalled
result blocks table updates and source consumption. The source can still accept
offers while it has free slots. Three queued records use 24 payload bits; the
two persistent table rows add 16 bits and no queue capacity.

The Python source uses one ordinary `ac.table`, one rule and two `ac.queue`
allocations. The rule receives the actual transfer condition, saves the old
row, and conditionally assigns the replacement. MLIR infers the storage owners
and write enables. Table update, source pop and result push prepare during Work
and commit together through the existing whole-system Xfer. Clock/reset and
proposal wiring stay out of the Python source. Reset clears both rows and both
queues; held/falling clocks and discarded epochs do not install a record.

Build and run against a compiler/Runtime installation from this checkout:

```sh
cmake -S examples/table_rule -B /absolute/build/table_rule -G Ninja \
  -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/table_rule --parallel 2
ctest --test-dir /absolute/build/table_rule --output-on-failure --no-tests=error
```

The shared example helper compiles this source, publishes its unit, links the
complete closure, and emits C++ and Verilog from the same verified artifact.
Independent table/queue scoreboards check generated native workers one/two and
RTL, including old-value lifetime, backpressure, reset, discard/retry and X/Z
transport. Generated excerpts and bound execution receipts are published in
`GENERATED.md` and `GENERATED.json` after successful verification.

## Generated system usage

`bench.py` exports `example_table_rule.bench.ExerciseTableRule`. Compile
`table_rule.py`, then `bench.py`, import the published DUT interface, and link
that explicit system root through the public compile/link/emit flow. Run
`pycircuit run examples/table_rule --target cpp --cycles 1343` or select
`--target verilog`. Imported records use their original nominal declarations
and supply every field explicitly.

The system bench stores the complete trajectory as an immutable 1,343-row
`Scenario` table. Each row names the four stimulus fields and four independent
expected fields. A clamped phase selects the row, so phases after the checked
trajectory retain the final idle stimulus without widening the phase counter or
changing the 1,343-cycle assertion boundary.

All original known-stream data edges are represented in this regular-clock
scenario, with fixed independent expectations from the retained native oracle
along a resetless trajectory. The 1343 cycles include a final observation.
The original DUT, native/RTL drivers, finite configuration, and any four-state,
reset/discard, latency and token-ledger matrices remain unchanged. Physical
held-level and midstream-reset scenarios still require those original module
oracles; this system does not claim complete physical-scenario equivalence.

## Measured frontend simplification

The scenario data replaces the former nested conditional expressions without
changing any of the 1,343 rows. An independent unit oracle reconstructs the
input schedule and old-value queue/table model, then checks all eight fields.
Both backends retain 10,744 observations over 2,686 sampling epochs; source
locations change with the rewrite, while event names, values and epochs agree.

| Artifact | Previous conditional tree | Scenario table |
| --- | ---: | ---: |
| Python source bytes | 823,402 | 188,920 |
| Python lines | 16,871 | 1,403 |
| Python AST nodes | 76,123 | 25,799 |
| Linked common IR bytes | 79,341,868 | 38,976,988 |
| Emitted C++ `.hpp` + `.cpp` bytes | 13,831,843 | 10,904,110 |
| Emitted RTL `.v` + `.sv` bytes | 3,011,034 | 2,170,002 |

In a local macOS arm64 run with the same compiler and default generated-build
options, capture took 21.33 → 1.76 seconds and native source lowering with
source-import retention took 3.65 → 1.70 seconds. Separate public link and emit
measurements also decreased. Two alternating full-length executions measured
C++ median time of 14.31 → 9.58 seconds, including observation output. Verilator
execution was about 0.02 seconds in both cases, too close to startup cost to
claim a runtime speedup. These are observed local measurements, not performance
thresholds, hardware synthesis results or cross-platform guarantees.
