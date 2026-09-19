# Modular ACC composite parent closure

Date: 2026-09-19

Decision: 0270

## Result

- Separately compiled parent modules may instantiate zero or more declared
  children with heterogeneous inputs/outputs, repeated instances, typed static
  arguments, child-to-child Queues, multi-output children, and compiler-owned
  atomic fanout.
- Each Python implementation source still owns one `.ac`; parents consume only
  source-owned declaration headers. Native ACC links and verifies the package
  before backend generation.
- Structured generated C++ owns internal Queues and nested broadcast blocks,
  assigns non-overlapping object IDs, and uses compiler-reserved readable
  runtime instance path segments so Queue and module paths cannot collide.
- Composite package DUTs run through `configure_activation_scheduler()` and the
  public typed header. The gate no longer hides nested activation defects by
  manually visiting every dispatch row.
- The diagnostic registry includes the composite frontend codes, complete
  implementation-source coverage, and the AC package ownership/path failure.

## Evidence

- Focused frontend/source-unit/ACC CLI: 49 passed.
- Composite AC package lit: 2 passed. Both independently compile source ACs,
  link and verify the package, emit a multi-TU C++ bundle, build with
  CMake/Ninja, link a typed external consumer, and execute it through the
  activation scheduler. The fanout fixture includes a multi-output child.
- `CodeGenTests`: 109 passed.
- `CompilerTests`: 9 passed.
- `GfsimTests`: 261 passed.
- Repository contracts, diagnostic catalog, API hygiene, strict MkDocs, and
  `git diff --check` passed.

## Open boundary

- Composite child wiring is an acyclic graph. Cross-child requester/responder
  feedback is not yet admitted; the existing bounded single-block
  `ac.feedback` primitive is not a substitute. A later change must define typed
  forward binding, QueueGraph state-cycle verification, runtime same-Queue
  transaction rules, and C++/Verilog parity before SSM relies on feedback loops.
