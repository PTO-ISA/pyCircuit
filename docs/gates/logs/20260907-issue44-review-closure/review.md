# pyCircuit issue #44 takeover review

Reviewed checkout: `codex/issue-44-typed-system-jit`, based on
`d018f940c639b8c5bfdaefa2870a9ba34e71aab9`. This review inherited the stopped
task's uncommitted implementation and independently reran the affected gates.

## Current work and ordering

| Work | Observed state | Next boundary |
| --- | --- | --- |
| #39: value types and masked matching | Merged in #43 | Preserve recursive type semantics |
| #41: primitive surface cleanup | Merged in #45 | Keep exact ACIR/PYC inventories synchronized |
| #42: scalar-only PYC | Merged in #47 | Preserve packed aggregate C++/RTL parity |
| #44: typed system JIT | Current reviewed implementation | Merge only with the fixes and fresh evidence in this run |
| #46: heterogeneous optional outputs | Open, depends on #44 | One presence per result and all-or-none selected effects |
| #48: aggregate equality and payload invariants | Open, follows #46 | Remove consumer identity boilerplate with shared ACIR semantics |
| #22 / #40 | Separate open backlog | Synthesizable Table lowering / CycleAwareSignal provenance |

## Ranked assessment

| Rank | Finding | Confidence | Basis |
| --- | --- | --- | --- |
| 1 | Source-order bindings and mutual-exclusion proofs are correctness boundaries, not frontend conveniences | High | Concrete wrong-value state-write and region-blind predicate counterexamples |
| 2 | Compile-only consumer checks cannot prove transactional closure | High | The inherited consumer matrix used C++ syntax checks; the new runtime test observes every commit boundary |
| 3 | The public IR inventory and required G0 checks must move with compiler changes | High | Fresh checks caught the missing `ac.var.record` entry, stale ledger and native dependencies in Python-only tests |
| 4 | The next simplification should use recursive type equality and explicit invariants | Medium-high | #48 records repeated identity comparisons; no implementation of that issue is claimed here |
| 5 | Namespace-preserving source bundling is a future frontend improvement | High | Flattening imported files cannot preserve conflicting definitions or renamed/qualified bindings without additional resolution |

## Correctness repairs reviewed in this change

- Preserve the local SSA binding visible at each state proposal, including
  conditions captured before a later rebind. A write before a rebind must not
  read the later value.
- Treat region-bearing/state-dependent values as opaque identities when
  proving complementary paths. Different `find` predicates cannot become the
  same Boolean atom merely because they access one owner.
- Intern structural expression identities and deduplicate shared Boolean DAG
  visits so proof work does not expand exponentially.
- Canonicalize dead reads before inferring footprint/effect metadata. Keep
  the strict agreement check between metadata and live IR.
- Admit `ac.var.assign` and `ac.var.assign_element` as actual state effects in
  raw outputless rules before storage selection.
- Restore initializer-list construction of the owner write batch used by the
  runtime tests, register `ac.var.record`, and regenerate the IR coverage ledger.
- Keep pure frontend tests in G0 and guard separately executed native tests.
- Reject unsupported local import bindings and cross-file definition
  collisions at source capture. Do not silently flatten them into a different
  program.
- Reject negative and oversized static left shifts before allocating the
  shifted integer, using the existing portable I-JSON range.

## Improvement recommendations

1. Complete #46 next with an executable selected/unselected sink matrix.
   Keep backpressure, reservations and commit mechanics in MLIR/runtime.
2. Complete #48's equality and invariant work in shared ACIR before consumer
   refactoring. Preserve protocol state and cancellation checks that cannot be
   derived from payload structure alone.
3. Keep #22 explicit: provisional Table remains gfsim-only. A compiling
   Agentic model does not establish synthesizable storage or RTL equivalence.
4. Use source binding and CFG normalization as explicit compiler boundaries
   before expanding the large Queue frontend further. Decompose that code in
   a separately tested change; this closure does not introduce a broad refactor.
5. Keep review evidence executable and tied to the current checkout. The
   inherited all-green notes did not detect the constructor build failure or
   the missing public IR inventory entry.
6. Measure sparse-reservation saturation before increasing state concurrency.
   `StateReservation` has eight sparse slots; a ninth distinct sparse entry
   conservatively reserves the whole owner. This preserves correctness but
   can reduce parallelism. A follow-up should compare inline-plus-overflow
   storage against a chunked bitmap with a greater-than-eight-entry gate.

## Limits

This run closes the framework behavior of #44. It does not claim the #46
optional multi-output contract, the #48 type-system extension, full consumer
protocol simulation, or a package release. Consumer source remains in its
own checkout. Runtime transaction tests are framework-owned, use generic
payloads, and do not depend on DavinciOO source paths.

Exact commands, raw outputs and exit codes are stored beside this review.
The final `summary.json` is generated from command results rather than copied
from the stopped task.
