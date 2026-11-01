# Reorder buffer

This four-entry reorder buffer accepts out-of-order completion and retires in
order. `RobStorage` owns the entry Table and `head`, `tail`, and `count` State.
Its `transact(RobRequest)` Method returns one same-cycle `RobResult`.

The Method computes completion, retirement, and allocation in statement order.
Ordinary locals carry a newly completed head entry and the space released by
retirement, so completion and retirement can succeed together and a full buffer
can retire and allocate in one transaction. Every State read observes old Q.
The final four static Table writes and pointer/count updates commit atomically
with the caller's Rule. Flush clears all entries and indices.

`Entry.valid` defaults to one for a newly constructed allocation. Initial and
flushed entries explicitly use `Entry(valid=0)`. Tag matching rejects unrelated
completions, but does not solve reuse of an identical tag after wraparound.

The System explicitly registers one Rule that calls the existing storage
instance, checks its same-cycle result against the original 284 scenario rows,
and advances the phase. No holding result State or extra observation cycle is
introduced. The hardware source has no authored clocks or write enables.

## Migration status

Deferred by the user on 2026-11-01. Further ROB-specific repairs are paused.
Source compilation passes for both `rob.py` and `bench.py`. Generic compiler
fixes now allow the complete System to pass transaction routing, but physical
lowering exceeds the separate 1 MiB storage-correspondence proof budget
(requested 14 bytes with 1,048,575 already used). This remains a failed linking
attempt, not a simulation pass. The full 284-row scenario history remains intact;
neither increasing the budget nor reducing the history establishes acceptance.

The original independent native/RTL driver and its 569 Work samples remain
unchanged. Its input-bearing `Rob` root currently fails transactional linking
with `transactional link requires a closed concrete root`. Its explicitly
registered Rule also needs an external resource export boundary; storing the
result in State would change the promised same-Work observation timing.
Native workers 1/2, RTL simulation, and System simulation are therefore **NOT
RUN** for this migration. No retained generated artifact establishes a current
pass.

The original build entry remains available for completing these gaps:

```sh
cmake -S examples/rob -B /absolute/build/rob -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/rob --parallel 4
ctest --test-dir /absolute/build/rob --output-on-failure --no-tests=error
```

[Historical generated excerpts and verification inputs](GENERATED.md) describe
the superseded frontend and are retained as historical evidence only.
