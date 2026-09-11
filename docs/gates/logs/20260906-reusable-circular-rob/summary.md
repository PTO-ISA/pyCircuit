# Reusable circular ROB gate summary

Decision 0189 unifies system and rule-backed module body lowering, then moves
the existing four-rule circular ROB behind a reusable typed module interface.

## Structure

- Python module interface: 3 inputs and 2 outputs.
- Lexical state: head, tail, count, epoch, and four entries.
- Rules: recover, allocate, complete, and retire.
- Root placements: two.
- Frozen specialization definitions and generated C++ classes: one.

## Evidence

- The frontend binds typed module arguments to borrowed Queue values and emits
  module returns without exposing source/sink or Queue authoring syntax.
- Frozen ACIR contains four firings once, while the root contains two
  `ac.instance` placements.
- Canonical QueueGraph records one `rob` specialization with 3 inputs, 2
  outputs, 5 Tables, and 4 blocks plus two root instance bindings.
- Generated C++ contains one `Rob_<fingerprint>` class, two independently
  constructed instances, and multi-owner `QueueStateTransition` members.
- Runtime: both instances allocate slot zero independently, then complete and
  retire values `100` and `200`; a later left-only allocation advances only the
  left instance to slot one.
- The existing flat circular ROB regression remains the detailed semantic gate
  for backpressure, full/empty, wrap, stale generation, recovery epoch,
  out-of-order completion, and in-order retirement.
- Python frontend suite: 75/75 passed.
- Full Queue codegen integration suite: 20 passed, 1 skipped because the
  optional DavinciOO reference trace fixture was unavailable.
- Python format/lint, repository contracts, strict 189-row decision status,
  MkDocs strict, bytecode compilation, and `git diff --check`: passed.
- Decision 0190 subsequently adds a complete direct-leaf activation plan and
  proves incremental `SimSystem` results equal the scan reference for the first
  dual-instance allocation/completion/retirement scenario.

## Remaining scope

QueueGraph module codegen still needs arbitrary internal graphs and incremental
activation. Rule semantics still need typed CFG/path effects. A Pythonic ISQ
still needs normal-list selection and bulk wakeup analysis.
