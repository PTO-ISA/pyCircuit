The resident providers restore the two-instance circular ROB and oldest-ready ISQ
algorithms with persistent entries, generation/epoch handling, readiness bits,
and the original old-Q grant competition. The inert `baseline/` assets preserve
exact historical source and the complete original strong test file.

`resident_pairs.py` exposes two actual Core instances through ordinary typed
borrowed-channel ports. `run.py` executes every original host action against the
compiled C++ and RTL modules using test-environment depth-one queues. These
queues commit only literal host stimuli and actual DUT take/publish results.
Expected snapshots never supply DUT state or results. Read-only native header
visibility copies and RTL hierarchy expose all storage. The independent oracle
checks every complete before/Work/after snapshot: 974 ROB bits (ten queues,
eight scalars, eight entries) and 616 ISQ bits (six queues, eight entries,
128 readiness bits). The original histories run 61/81 epochs and independent
reservation witnesses run 38/49, retaining the original finite ceiling of 256.
Both native worker counts and Verilator must agree.

`resident_systems.py` contains genuine zero-argument `@system` roots owning real
`queue` intrinsics, each depth one, latency one, local occupancy. Their providers
are independently compiled before the bench, under package `history_resident`:

- `history_resident.resident_systems.reusable_circular_rob`: 61 cycles; ten queues,
  two ROB instances, the complete ordinary original ROB host schedule.
- `history_resident.resident_systems.reusable_oldest_ready_isq`: 81 cycles; six
  queues, two ISQ instances, a separate ordinary schedule without committed
  pre-Work host insertions. This is partial system migration of the original ISQ
  host-action scenario, not an equivalent retiming of that trace.

Source order is `reusable_circular_rob.py`, `reusable_oldest_ready_isq.py`, then
`resident_systems.py`. System checks require accepted input offers and available
output takes. The external independent oracle checks every low/high old-Q public
view, queue capacity and complete visible payload, transaction grant, publication
payload, and final finite duration. Exact original exceptional readiness commits
remain covered by the strong module harness. No author-defined clock/reset or
hidden state mutation is introduced.

The historical Boolean fields `RobEvent.done`, `IssueEntry.valid`, and
`Readiness.ready`, and the private readiness table, use explicitly unsigned
one-bit physical carriers. Field order, packed widths, zero reset, and Boolean
0/1 encoding are preserved. This approved source translation does not establish
general nominal Boolean-field or Boolean-Table authoring support. The Core freezes
the completion index once and reuses that old-Q scalar for the write and grant;
the rejected direct aggregate-projection address proof remains unsupported.
This bounded gate checks known-state masks/values and does not claim X/Z matrices.

Run the existing compiler-lit entry point with the configured build tree:

```sh
.venv/bin/lit -sv .pycircuit_out/toolchain/build/tests/compiler/lit \
  --filter source-historical-residents
```

The gate publishes command/status records, exact source/helper/runtime hashes,
per-case receipts, read-only visibility hashes, source-unit artifacts, and both
backend bundles under its disposable lit output directory. `receipt.json` is
removed before execution and written only after all checks and unchanged-input
hash verification succeed.
