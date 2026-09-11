# Work/Xfer closure separation gate summary

Decision 0193 separates next-epoch Work activation from same-epoch Queue/Table
participation in the atomic Xfer barrier.

## Evidence

- QueueGraph plans contain independently verified `activation_edges` and
  `work_closure_edges`; deleting an edge from either set is rejected.
- Generated direct, nested, and internal-Queue module hierarchies emit both CSR
  plans using the same recursive ObjectId binding.
- `SimSystem` invokes Work only for active workers, arbitrates workers before
  closure-only resources, probes the entire closure, and then commits it in a
  second loop.
- A spy resource commits through its worker's closure while its Work invocation
  count remains zero.
- External Queue offers use `scheduleExternalXfer`; a Queue proposal commits
  with zero Work invocations.
- The reusable ROB scan and incremental paths remain result-equivalent for
  left/right values `100/200`.
- The 32-row incremental scenario now performs 40 Work calls, 74 cross-epoch
  activation traversals, and 150 same-epoch closure traversals. The prior
  combined-closure activation required 134 Work calls; an eight-tick full scan
  requires 256.

## Focused gates

- Work closure, external Xfer, and activation runtime tests: 3/3 passed.
- Stateful plan/codegen and activation tamper test: passed.
- Direct, nested, and reusable ROB activation integrations: 3/3 passed.
- Stateful module lit plan/codegen test: passed.
- Full ACIR lit: 159/159 passed.
- Full CodeGen: 105/105 passed.
- Full gfsim: 258/258 passed.
- Python frontend: 75/75 passed.
- Full Queue codegen integration: 20 passed, 1 skipped because the optional
  DavinciOO reference trace fixture was unavailable.
- C++/Python format, repository contracts, strict 193-row decision status,
  MkDocs strict, and `git diff --check`: passed.

## Remaining scope

Typed system results still lower to compiler-generated sinks. Sink-free host
integration needs a root result boundary and external dequeue adapter. Full ROB
tick/state/timeline comparison and semantic-change wake filtering also remain.

Decision 0194 subsequently adds the root result boundary and external dequeue
adapter for structured module systems.
