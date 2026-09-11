# Python multi-state module gate summary

Decision 0188 allows one ordinary typed `@ac.module` to declare and update
multiple scalar lexical variables. The frontend emits one transient `ac.rule`;
MLIR derives two owner footprints and one atomic Queue-plus-state transaction.

## Evidence

- Python Queue frontend suite: 75/75 passed.
- Focused stateful module integration: 2/2 passed.
- The later full integration run after Decision 0189 passed 20 cases and
  skipped only the unavailable optional DavinciOO trace fixture.
- Raw multi-state module ACIR contains two `ac.var.decl`, two `ac.var.read`, and
  two `ac.var.assign` operations in one rule and contains no concrete Table.
- Canonical QueueGraph contains two Table owners and two state writes.
- Generated C++ contains one reused `Tally` specialization class and one
  multi-owner `QueueStateTransition`; repeated placements construct independent
  owner sets.
- The runtime test withholds sink Work through tick 5. The second left input
  remains in its committed input Queue while the first output applies
  backpressure, proving neither state owner commits early. After release, left
  inputs `1,2` produce `2,5`; right input `10` independently produces `11`.

## Remaining scope

Decision 0189 subsequently adds direct input/output arity and multiple rule
calls. Python modules still need arbitrary internal graphs, conditional state
updates and CFG joins, shaped local state, static parameters, inferred fanout,
and incremental activation.
