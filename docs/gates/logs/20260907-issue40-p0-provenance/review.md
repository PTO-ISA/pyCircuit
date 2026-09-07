# Issue #40 P0 provenance evidence

Decision 0148 now defines one read-side occurrence rule for `ForwardSignal` and
internal `StateSignal`. Explicitly saved `CycleAwareSignal` values remain
immutable. `domain.cycle(value)` inserts one register and tags its result at the
resolved source occurrence plus one.

## Focused behavior

- Method and function forms of priority encode, popcount, leading/trailing zero
  count, concatenate, and mux use the same current-view coercion.
- A source saved at cycle 0 remains cycle 0 when the domain cursor moves to 3;
  two explicit `cycle()` calls produce cycles 1 and 2, and alignment against a
  cycle-3 input inserts exactly one `_v6_bal_1` register.
- Flat and hierarchical composition reject missing and extra keys. Nested flat
  wrappers propagate consumption to their caller. Composed scalar values retain
  CAS provenance and require the exact domain and width.
- The focused backend case observes `(first, second, mixed)` as `(5, 0, 7)`,
  `(5, 5, 7)`, and `(5, 5, 12)` on successive active edges. Both generated C++
  and Verilator pass those expectations.

## P1 boundary

Direct `compile_cycle_aware(..., eager=True).emit_mlir()` produces the expected
three-register graph, but that raw emitter does not stamp the hardened frontend
attributes required by `pycc` (`PYC901`, `PYC903`-`PYC906`, `PYC951`, and
`PYC952`). The focused executable parity therefore uses the current CLI/JIT
entrypoint. Converging the eager and JIT compile paths remains the next P1 item
in issue #40.
