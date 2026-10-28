# Reorder buffer

This four-entry reorder buffer accepts out-of-order completion and retires in
order. `Rob` groups its external request pins into one `RobRequest` and passes
that structured value directly to `RobStorage`.

The Module owns the entry Table and `head`, `tail`, and `count` variables.
One autonomous `transact` Rule calls four local Methods in statement order:

1. `flush` clears the entries and indices.
2. `complete` marks a matching live tag as done.
3. `retire` can see that completion and remove the oldest entry.
4. `allocate` can use the space released by retirement, including from a full buffer.

The calls share one transaction, so each Method sees the earlier calls'
candidates. `transact` assembles one `RobResult`; the Module returns its bare
Rule name as the output connection. Its declaration activates it without an
explicit call. Independent Rules would read committed state. The compiler owns
candidate visibility, conflicts and state commit.

`Entry.valid` defaults to one for a newly constructed allocation. The explicit
`table(..., init=0)` and flush still create all-zero invalid storage. Reset
restores empty state; failed or discarded Work commits no proposed state.
Tag matching rejects unrelated completions, but does not solve reuse of an
identical tag after wraparound.

The independent Module driver checks 569 Work samples with native workers 1/2
and RTL. The 284-cycle System provides a regular-clock scenario for the same
sequential operations. The hardware source has no authored clocks or write enables.

```sh
cmake -S examples/rob -B /absolute/build/rob -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/absolute/pycircuit/install
cmake --build /absolute/build/rob --parallel 4
ctest --test-dir /absolute/build/rob --output-on-failure --no-tests=error
```

[Generated excerpts and verification inputs](GENERATED.md).
