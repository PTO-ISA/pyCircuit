# F6 sequential SRAM and exact four-state verification

## Scope and verdict

This run implements and verifies Decision 0273. The synchronous SRAM profile
uses mandatory static `live_window = 1`, exact value/known/Z comparison, active
knownness checks, one-cycle Q lifetime, old-data read-during-write, and the
canonical always-capture/NBA-safe selection pattern.

Four-state behavior is isolated to verification. The ordinary `Wire<W>` and
technology-independent synthesizable SRAM bodies remain two-state; C++ exposes
an explicit `FourState<W>` observation and RTL enables aggressive X behavior
only under `PYC_VERIFY_AGGRESSIVE_SRAM`.

## Implemented contracts

- `FourState<W>` carries equal-width value, known, and Z masks, enforces
  `(known & z) == 0`, requires exact known/Z mask equality, and compares value
  bits only under the shared known mask.
- `pyc.sync_mem` and `pyc.sync_mem_dp` require `live_window = 1`; missing or
  different values fail PYC verification. C++ templates and Verilog parameters
  carry the same static value.
- SRAM Q begins unknown. One enabled read makes Q live after the capture edge;
  a back-to-back read refreshes Q; the next edge without a read invalidates Q.
  Dual-port outputs expire independently.
- Reset invalidates Q. Enabled controls must be known. Enabled read address and
  enabled write address/data/strobe must be known, while inactive unknown data
  paths are accepted.
- Read-during-write to the same address returns old data while the write commits
  at that edge.
- `pycircuit.lib.sram.SRAM` emits an always-capture register and selects live Q
  in the read/capture cycle, then captured Q afterward, preserving NBA
  visibility without extending primitive Q lifetime.
- The frontend-contract verifier rejects an exact redundant
  `select(enable, update, q)` feedback path when the same register enable can
  provide natural hold.

## Parity evidence

`sram-four-state-parity.mlir` runs independent C++ and RTL encoders for known,
X, and Z observations. Both produce:

```text
5a:ff:00
00:00:00
00:00:ff
```

The same RTL fixture verifies initial unknown Q, old-data read-during-write,
back-to-back reads, N=1 stale-Q invalidation, inactive unknown gating, active
unknown-address failure, and Verilator structural lint.

Native C++ tests independently cover exact mask mismatch, invalid mask overlap,
single- and dual-port live windows, reset invalidation, active/inactive
knownness, write data/strobe checks, and old-data behavior. ASAN covers the new
verification state.

## Boundaries

- This change does not model arbitrary four-state arithmetic. Unsupported
  four-state operations remain outside the admitted PYC semantic set rather
  than coercing to two-state behavior.
- `Table` bounds and ranks remain separate from SRAM live-window N.
- Recovery and stale-update semantics remain owned by F7.
