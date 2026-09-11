# QueueGraph incremental activation gate summary

Decision 0190 connects compiler-derived Queue/Table wake relationships to
gfsim's dense `DispatchTable` and CSR `ActivationPlan` for direct leaf module
specializations.

## Semantic evidence

- QueueGraph plans contain deterministic local activation nodes, exact
  resource-to-transaction-closure edges, and an initial zero-input frontier.
- The verifier independently re-derives activation and rejects a plan after one
  edge is removed.
- Physical codegen binds one specialization's local block/Table nodes to each
  placement's independent dense ObjectId interval and binds interface nodes to
  caller Queue IDs.
- Generated roots expose `activation_offsets()`, `activation_targets()`,
  `initial_work_ids()`, and an explicit `activation_complete()` capability.
- Zero-input Queue/Table transitions no longer report vacuous permanent
  runnability; initial and owner/output activation control retries.

## Reusable ROB evidence

- Scan reference and incremental `SimSystem` execution produce identical first
  allocation and retirement results for left value `100` and right value
  `200`.
- The incremental run terminates as `Completed`; it does not report
  zero-input-rule no-progress.
- The graph has 32 dispatch rows. Incremental execution invokes Work 134 times,
  below the 256 invocations of an eight-tick full scan, while conservatively
  traversing 306 closure edges.

## Focused gates

- QueueGraph stateful specialization plan/codegen test: passed.
- Zero-input retirement/backpressure gfsim test: passed.
- Reusable circular ROB frontend-to-gfsim integration: passed.
- Full ACIR lit suite: 159/159 passed.
- Full CodeGen suite: 105/105 passed.
- Full gfsim suite: 256/256 passed.
- Python frontend suite: 75/75 passed.
- Full Queue codegen integration: 20 passed, 1 skipped because the optional
  DavinciOO reference trace fixture was unavailable.
- C++/Python format, changed-file lint, repository contracts, strict 190-row
  decision status, MkDocs strict, and `git diff --check`: passed.

## Remaining scope

- Move activation-source and transaction-closure evidence into typed MLIR
  rather than deriving the final local plan solely during QueueGraph extraction.
- Recursively bind nested/internal Queue specializations.
- Automatically schedule external Queue proposals/dequeues.
- Separate scheduled block Work from same-epoch resource Xfer closure.
- Compare every ROB tick, Queue/Table state, timeline, and full recovery scenario.
- Suppress Table wakeups when replacement does not change committed value.

Decision 0191 subsequently moves rule-backed activation/transaction resources
into enum-typed MLIR and adds generated external input offer adapters.
Decision 0193 subsequently separates Work from Xfer closure and reduces the
same ROB scenario from 134 to 40 Work calls.
