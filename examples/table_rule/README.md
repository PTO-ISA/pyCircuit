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
