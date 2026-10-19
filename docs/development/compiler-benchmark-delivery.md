# Compiler benchmark delivery

User objective: fix the compiler findings, integrate the implementation, pass
the affected benchmarks and submit a PR. The audited findings are recorded in
`benchmark/findings/compiler-audit-20261019.md`. Passing prior gates does not
close the remaining findings.

The four-state follow-up is implemented in PR #321. Explicit literals,
verification predicates, finite System samples and Bits4 observations share the
existing common IR and generated execution flow. Root reset policy is `strict`
by default or explicit `if_one`, identically on both targets. Current validation
is recorded in [execution status](issue-307-319-execution-status.md); the old
checkpoints at the end of this page are historical, not outstanding blockers.

## Current authoring unification sequence

User-approved order: ordinary statements → dynamic Bits access → removal of
unnecessary wrappers/workarounds → Memory and instance collection calls.

- [x] Unify Module ordinary-local statement dispatch with the existing helper
  branch/value machinery in both behavioral and typed-transaction Module paths.
  Module annotated declarations remain persistent; writes remain Rule-owned.
  Runtime-controlled allocation/registration and early returns are not silently
  admitted. Preserve pure four-state joins and statement-time snapshots.
- [x] Add Bits indexing through existing full-width shift/extract operations,
  retaining the source Table-style in-range proof, exact selected X/Z payload,
  and unknown-index propagation. Dynamic-width slices and writes require their
  own ownership/payload-preservation checks; a shift's out-of-range zero is not
  a substitute for an index policy.
- [x] Replace single-Table `Scenarios` wrappers with already-supported typed
  helper locals; remove computation-helper detours after direct statements pass.
  Preserve every stimulus row, cycle count, expected value and independent oracle.
- [ ] Unify Memory argument/call normalization with existing primitive owners.
  First specify asynchronous versus registered read timing and request/result
  availability; never disguise a delayed response as a same-call read result.
- [ ] Support homogeneous instance collection calls through actual lane identity,
  existing collections, selected-owner writes and per-element availability.
  Static selection, dynamic dispatch and broadcast are distinct cases. Existing
  routed-port/transaction proofs must be completed before removing their guards.

The first two bounded contracts have independent architect/decomposition review.
Implementation and test ownership stay separate. Memory/collection design is
being reviewed in parallel; no new public spelling or scheduler is assumed by
this checklist. Each step must pass its focused common-IR and dual-backend
checks before the next step removes its source workaround.

## Memory and collection interface contract for review

The user selected **existing synchronous timing with request/result snapshots**.
No response-consumption state, extra response FIFO, hidden wait or new scheduler
is part of this change.

Named allocations reuse existing primitive names and static bindings:
`memory = ac.sync_mem[T](depth=N)` (or `sync_mem_dp` / `byte_mem`). Allocation is
Module-scoped. An immutable alias denotes the same owner. Existing pin-oriented
conformance calls retain their current meaning; ordinary designs use operations
inside their owning Rule transaction.

- Synchronous `memory.request(address, port=0)` submits a read on a static read
  port. Port is a closed static Integer (0 for sync_mem, 0/1 for sync_mem_dp).
  `memory.result(port=0)` reads that port's committed response snapshot;
  it does not consume it. One read port exists for sync_mem, two for sync_mem_dp.
- The response becomes visible only after the existing successful Xfer. Reads
  sample old memory, including same-address read/write. The current two-idle-edge
  expiry and reset invalidation remain unchanged. Request and result in the same
  Rule observe the previous response, never the newly requested address.
- Result availability comes from the primitive's real live state, independent
  of payload knownness. An unavailable result inhibits its outer transaction;
  a valid X/Z payload does not. The common primitive contract must carry the live
  sideband to both emitters; do not reconstruct it using `is_known(data)`.
  Live/expiry state must remain synthesizable when it guards functional calls;
  only invalid-payload X poisoning may remain simulation-only.
- Asynchronous `byte_mem.read(address)` reads committed memory combinationally.
  It has no request/result pair or additional latency. Cold/unknown data retains
  existing primitive semantics. An inactive asynchronous read port receives a
  known safe idle address, so an unselected call does not introduce a read fault.
  Its combinational address follows the reached read-demand prefix, not final
  transaction fire: read data may determine later admission. Raw reached/selector
  checks remain independent, so idle-drive selection cannot erase an unknown
  effect-selector fault. Request/write proposals use final atomic admission.
- `memory.write(address, value, mask=...)` submits one write. Omitting mask selects
  all existing byte strobes; explicit mask follows the primitive's declared
  strobe width: ceil(payload_width/8) for synchronous memory, with byte_mem
  requiring whole-byte payloads. Both synchronous variants share one write port.
  Ports, domains and write enables remain compiler-owned.
- Calls reuse existing Method/effect/resource analysis and canonical memory
  kernels. The whole outer Rule commits or discards. A late false guard suppresses
  committed effects; it cannot erase already-reached guard/effect-selector checks.
  Source address widths and runtime address/control behavior remain those of the
  existing primitive, including known-OOB synchronous read zero/write no-op.
  The Table source in-range rule is not imposed on raw Memory addresses.
- `result(port)` is a nonconsuming snapshot read: multiple readers and a concurrent
  request on the same port are legal, all observing the previous response.
  Only request/asynchronous-read/write issue operations claim physical ports.
- A physical port is a finite resource. Multiple potentially simultaneous calls
  require an existing exclusivity proof or a diagnostic; same-address repeated
  writes still consume the one write port. Byte-memory reads use its single
  address port; multiple addresses need equality/exclusivity proof or rejection.
  Aliases resolve to one resource key before these checks. Source order/after does
  not create additional ports or impose EHR forwarding onto read-first RAM.
- A producer Rule may request while a separate consumer Rule reads an available
  prior response. This interface does not promise to retain expired responses
  under arbitrary consumer stalls. A reached unavailable result suppresses all
  effects of its outer Rule, including earlier requests/writes; an untaken result
  access does not stall it. Request+result in one initially empty Rule cannot
  bootstrap itself; separate producer and consumer Rules are the normal pattern.
- Expiry counts successful physical rising edges only: request sets age zero;
  the first idle rising edge retains the snapshot, and the second invalidates it
  at Xfer. Work on that second edge still sees the old live snapshot. Low, held,
  falling and discarded epochs do not age it. Reading result does not refresh age.
- Host/physical reset invalidates responses according to existing reset semantics
  but never clears RAM contents. Current memories store known Bits and retain
  enabled write-data/strobe knownness requirements; this feature does not add
  four-state RAM storage. Live/payload independence is tested separately at the
  primitive sideband boundary, without relaxing existing write checks.

The first collection interface is a fixed positive one-dimensional homogeneous
family, written as a static list comprehension such as
`banks = [Bank() for _ in range(4)]`. It creates independent static children and
lowers through `ac.collection`, not runtime Python objects. No heterogeneous
children, variable shape or per-lane static specialization is implied. The
comprehension has one generator over positive static range(N), no filters or
nested comprehensions, and one callee with uniform static/type bindings. Aliases
retain allocation identity; assigning a different member is not allowed.
Static-lane selection may be delivered first, but cannot be reported as dynamic
Collection call support.

`banks[index].operation(...)` selects an existing lane. Static and dynamic
indices share full nonnegative in-range source proof; the selector is evaluated
once. A reached X/Z effect selector faults under existing sampled rules; it must
not silently choose lane zero. Calls require stable collection/lane identity,
per-lane availability and proposal routing. Nonselected lanes have no call
effects, and selected-child stall holds the outer Rule. Conflict checks must
recognize aliased accesses to the same lane while preserving independent lanes.
Unselected children may still execute their autonomous Rules and age their own
memory/domain history: only the selected-call effects are suppressed. For a
three-lane family an unconstrained bits[2] selector is not proven in range;
source range refinement must establish the bound without selector truncation.
Broadcast syntax is not introduced by this first contract.

This section fixes intended semantics for independent review. Neither a frontend
receiver parser nor static per-lane unrolling alone completes Memory or compact
Collection transaction support. Required evidence includes imported providers,
port/lane conflicts, live-X versus unavailable values, reset/expiry, four-state
indices, independent lanes, whole-root discard and generated C++/RTL execution.

## Required work

- [x] Audit each failure through its existing source/IR/backend owner.
- [x] Repair Integer peer normalization and fixed runtime shifts with independent
  native/RTL/four-state and output-protection coverage.
- [x] Repair pure-helper loop arithmetic admission and live ordinal proof.
- [x] Preserve native RTL root identity in public receipts.
- [x] Fix loop resource overreservation without increasing limits or bypassing
  preallocation/audit. Compile and run original vector100r against its upstream TB.
- [x] Support scalar-result generic Modules using existing binding/specialization,
  including parameter overrides and invalid-binding protection.
- [x] Complete public port/domain metadata and external naming at export boundary
  through the existing transparent benchmark adapter; no alias-rewriting compiler
  route was introduced.
- [x] Implement a reviewed ordinary physical RTL export for sequential benchmark
  modules. Preserve transaction atomicity and explicitly specify simulation versus
  physical sampling obligations; no lifecycle tieoffs or frontend clock wiring.
- [x] Run all selected VerilogEval/ResBench implementations with unchanged upstream
  testbenches, plus required API/example and affected publication/package gates.
- [x] Independent current-candidate implementation review; resolve findings.
- [x] Commit task-owned changes, push, submit upstream PR with exact evidence,
  attach it to the task, and inspect required CI/review state.
- [ ] Complete requested integration under repository branch policy; do not
  equate a submitted PR with a merged change.

## Ownership and sequence

The leader owns shared integration, native build/install, export metadata,
candidate evidence and delivery. Astra source author diagnoses loop accounting;
the independent architect designs physical export; an independent planner traces
generic binding and decomposes remaining interfaces. Implementation assignments
follow their concrete review results. Tests and review remain independent.

Use the current worktree, preserve inherited changes, and reuse existing passes,
runtime leaves, manifests and test entrypoints. No benchmark names, values or
statement recipes may determine product admission. Keep original testbenches,
stimuli, timing and four-state failure oracles. Unsupported unselected dataset
tasks remain explicit; no coverage count is a model-generation or PPA score.

Remote verified at intake: canonical `PTO-ISA/pyCircuit`, default `main` at
`412a180928d3bdf1af3a99f14ed4a21a1c1b172a`; working branch
`codex/typed-module-outputs`, HEAD `dcbd84c0905475ec07089fc9c985316633ae6ddd`.
No matching existing PR at intake. Recheck before publishing.

## Historical execution checkpoints

2026-10-19 continuation: tighter conservative decimal-spelling bounds repaired
the reversal failure. `bounded-for.test` passed with independently executed
shift/concat reversals at widths5/100, workers1/2 and retained rejection/rollback
cases. Original VerilogEval vector100r TB reports0 mismatches/200 samples.
Evidence: `docs/gates/logs/20261019-compiler-delivery/loops-final.log`,
`benchmark/runs/compiler-delivery-vector100r/`.

Before physical export integration, the full selected benchmark baseline is
VerilogEval71 passed/17 managed-interface unsupported and ResBench43 passed/
13 unimplemented. Those are historical candidate results, not final acceptance.

Physical first integration passed11 independent atomicity/four-state tests,
but original dff8 fails its initial-value sample: source initial0 differs from
the autonomous primitive's initialX. This remains open pending generic source
initialization repair; dff and dff8r pass. Generic direct calls pass the base
matrix; root CLI overrides, symbolic concat widths and genuine parameter
forwarding remain under active implementation/diagnosis. No PR yet.

Later candidate: full scalar generic suite15/15 passed, including root overrides,
dependent defaults, imported symbolic forwarding and strict malformed-binding
protection. Physical startup now uses verified closed typed initializer constants;
VerilogEval86/88 passes. m2014_q4c/lfsr32 fault on their first edge because the
unchanged upstream TB has not initialized reset; strict versus native-Verilog
reset-X policy is pending direct user clarification. The compiler does not hide
that fault. Remaining acceptance covers wide/nested/Collection startup and
negative physical projection cases; refreshed API/example gates are running.

Final local checkpoint validation: API23/23, Examples13/13, unit627 passed/
5 skipped/82 deselected, strict docs build and changed-file pre-commit checks
passed. Physical runtime/startup24 cases, additional5 initialization rejection/
synthesis cases, isolated projection4 cases and scalar generic15 cases passed;
reviewers closed the reported implementation and test-isolation findings.
Benchmark VerilogEval remains86/88 executed implementations (68 unselected),
ResBench43/43 implemented (13 unimplemented), and four output-corruption
negative controls are detected by original oracles. Full nightly/PPA remain
NOT RUN. No upstream PR/merge claim is made while the reset-X decision is open.
