# Combinational legality and bounded progress

This design separates existing combinational admission from a proposed, opt-in
progress monitor. **The monitor is not implemented.** There is no new source API,
pass, attribute, runtime option or default timeout in this document's delivery.
Runtime monitoring, high-level C++ integration and incremental execution remain
separate implementation and acceptance work.

## Current support

| Property | Current behavior |
| --- | --- |
| Same-evaluation dependencies | Typed field/SSA analysis checks actual instance bindings and rejects illegal combinational cycles, including dead computations. |
| Temporal cuts | Logical Q and verified registered outputs cut their own combinational dependencies. Storage ownership does not cut every output. |
| Ready and memory address dependencies | Downstream-pop FIFO ready depends on `out_ready`; asynchronous memory data depends on `raddr`. Those edges remain combinational. |
| Availability | Separate availability equations and cycle checks apply, independently of payload dependencies. |
| Quiescence | No root reports work. Unchanged Q or a held clock does not establish quiescence or completion. |
| Execution bounds | `max_ticks` is a finite cap on accepted epochs. Non-null `deadlock_window` is rejected. |
| Commit | Complete Work, Precheck and observation Precommit precede every root Xfer. Failure discards pending state and observations. |
| Progress monitor | Not admitted. Current source-check kinds are `assert`, `range` and `guard_known`. |
| Incremental monitor scheduling | Not implemented. Existing work partitioning proves preparation independence, not permission to skip temporal effects. |

The owning implementation is `HardwareAnalysisDependencies.cpp`,
`HardwareAnalysisValidation.cpp`, `HardwareValueAvailability.cpp` and
`HardwareSourceChecks.cpp`. Runtime boundaries are in `SimSystem.h`,
`ObservationSlot.h`, `sim_executor.cpp` and `system_runner.cpp`. These existing
owners remain authoritative; the proposal does not introduce another graph or
diagnostic runtime.

## Relations and evidence

Reuse verified analysis facts with their endpoint and source witnesses. Keep
relation kinds distinct:

| Relation | Meaning | Treatment |
| --- | --- | --- |
| Combinational value | An output/field depends on an input/field in the same evaluation. | Existing mandatory cycle checks. |
| Combinational availability/control | Ready, availability, guard or selection depends on another same-evaluation value. | Retain its equation/operand witness; storage does not remove it. |
| Temporal transition | Committed state contributes to a later state under declared sampling/reset behavior. | Record the exact state endpoint and boundary; exclude this relation from the same-evaluation cycle graph. |
| Resource wait | An action requires a resource or environment condition before it can proceed. | Conditional blocking, not automatically a combinational edge or deadlock proof. |
| Call/control precedence | An action precedes another under existing invocation, registration or explicit ordering. | Retain provenance; add combinational edges only where SSA/equations require them. Infer no fairness. |

Combinational edges point from producer to consumer. Wait relations point from
the waiting action to its required condition. An untyped strongly connected
component is insufficient evidence for rejecting a design.

Each proposed evidence record identifies the candidate/stage and analyzed scope,
the property, definition and actual occurrence or Collection coordinates,
endpoint/field path, operation/source location, dependency witness, assumptions
and whether they were checked, and any precision or analysis-budget limit.
This reserves reporting information; it does not add a serialized schema.

Evidence status is separate from diagnostic severity:

| Status | Meaning |
| --- | --- |
| PROVEN | A specific scoped claim follows from an exact witness or completed analysis, such as an illegal same-evaluation dependency cycle. It does not abbreviate a combinational result to “liveness proven.” |
| CONDITIONAL | The stated conclusion depends on enumerated assumptions; an unchecked environment declaration remains an assumption. |
| RISK | A possible blocking pattern exists, but feasibility, reachability or persistence is unproved. |
| UNKNOWN | Missing contracts, unsupported constructs, abstraction loss or exhausted analysis budget prevent a conclusion. |

For example, compact Table analysis can fail to prove cycle absence. That may
remain a mandatory admission failure with UNKNOWN evidence, not a proven
deadlock. Existing admission is unchanged. Optional resource-wait assessment is
a bounded diagnostic design, not an enabled warning/error mode. A risk alone
does not authorize a runtime assertion: an explicit property and assumptions
are required.

## Proposed first monitor

The future profile supports one outstanding obligation per monitor occurrence.
An explicit accepted event starts it, and a named meaningful progress event
discharges it. Activity, Rule evaluation, changing Q and unrelated transfers are
not implicit progress.

The semantic contract contains an identity/source location, owner occurrence,
one existing sampled domain, start and discharge predicates, and explicit DUT
bound `N`. Its environment mode is one of:

- `none`;
- `unbounded_wait(permission)`;
- `bounded_wait(permission, B)`.

These names describe a proposed contract, not available source syntax. Bounds
`N` and optional `B` are positive compile-time integers fitting `uint64_t`.
There is no omitted/default bound.

Predicates are typed Bits1 SSA values with verified bindings. Environment
permission must describe the selected external boundary: its pure fan-in may
reach only selected environment input ports and constants. DUT state, pending
state and internal backpressure loops are excluded. A DUT cannot exempt its own
stall by labelling its internal ready signal “environment permission.”

Bounded environment waiting means that, while an obligation is outstanding and
permission is absent, permission must arrive within `B` eligible samples unless
discharge occurs first. It is an observable response contract, not general
request/response matching. Weak/strong fairness, eventual clocking, arbitration
fairness and eventual external response are never inferred. Multiple outstanding
tagged requests are outside this first profile.

### Sampling and transitions

The unit is a **rising sample of the declared domain**, using existing sampled
clock evaluation. It is not a host Step count, wall time or a complete clock
period. Runtime epochs count successful Step commits; held and falling samples
must not be conflated with domain progress.

Each occurrence owns committed `active`, `dut_misses` and, for bounded waiting,
`environment_misses`, plus separate pending state and completion/epoch evidence.
The canonical inactive tuple is `(false, 0, 0)`, omitting the last field when
there is no environment counter. Each Work preparation seeds pending state from
committed state. Initialization, inactive/no-start, immediate start-and-discharge, active
discharge, sampled reset and whole-system Reset all assign the canonical inactive
tuple. Arming assigns `(true, 0, 0)`. No inactive counter retains a previous
obligation's history.

On an eligible, non-reset rising sample:

1. Require known `start`.
2. If inactive and start is false, remain inactive; other predicates are
   irrelevant. If start is true, require known discharge. True discharge finishes
   immediately; otherwise arm with zero counters. The starting sample does not
   count against either bound.
3. If active, another true start is an overlapping-obligation contract failure,
   even with simultaneous discharge. Otherwise require known discharge. True
   discharge clears the obligation, including on its deadline sample.
4. With no discharge, inspect permission if configured and require it known.
   Absent permission holds the DUT counter. Unbounded waiting permits indefinite
   waiting. Bounded waiting fails the environment contract on the `B`-th
   consecutive unavailable sample; earlier samples increment its counter.
5. With permission, or mode `none`, clear the environment counter. The `N`-th
   eligible sample without discharge fails DUT bounded progress; earlier samples
   increment `dut_misses`. Environment waits pause, rather than reset, this
   counter.

Compare against the deadline before incrementing, avoiding wraparound. Failure
is prospective: neither that increment nor any DUT update commits. Held/falling
clocks retain current pending-history behavior without aging the obligation.
Unknown clock and root reset policy retain existing semantics. A sampled active
reset clears pending monitor state, but another failure prevents partial reset
commit. Whole-system `Reset()` clears monitor state through its existing lifecycle.

### Waiting, stopping and completion

Legal backpressure is waiting allowed by the selected environment contract, or
admitted misses before a deadline. Unbounded external wait yields no eventual-
progress conclusion. A stopped clock consumes no domain-sample bound and implies
no wall-time guarantee.

Discharge completes this obligation, not the whole design. An armed obligation
is temporal/check work and cannot disappear through an “all clean” incremental
decision. It does not create autonomous ticking or a new scheduler.

Keep current host-stop/run-cap termination status. Stopping with an outstanding
obligation is incomplete observation, not a successful liveness result. Quiescence
keeps its existing no-work meaning; an inactive monitor adds no obligation.

## Future integration and failure order

Use compiler-owned instrumentation in the same common IR. Existing logical
state/update semantics can represent monitor state, and existing check operations
can carry predicates and actual occurrence/CheckID/source bindings. An explicit
verified plan must tie that added state/checks to the declared monitor contract.
Monitor outputs must not drive DUT data, ready, arbitration, clocks or reset.

Analysis stays read-only; instrumentation is an explicit verified transformation
proving the transition relation and preservation of the original DUT graph on
accepted traces. Do not store authoritative state only in an emitter or a new
diagnostic runtime. Do not silently add a new `SourceExpectOp.kind`: existing
assert checks plus a separately designed and verified provenance/ordering
extension are the intended boundary, and that extension is not implemented.

Generated C++ uses the existing check-capture/Precheck path. Both applicable
backends must implement the same verified final graph. Proposed failure priority:

1. Existing evaluation failure.
2. Missing/incomplete Work/check snapshots.
3. Sampled physical clock/reset faults.
4. Existing source guard faults.
5. Existing ordinary assertion/range failures in their current order.
6. Monitor predicate-knownness/overlap failures.
7. Bounded environment-contract failures.
8. DUT bounded-progress failures.

Within new monitor classes use stable occurrence/site order. Preserve current
Precheck versus observation-Precommit behavior: a selected Precheck failure is
reported first; otherwise observation Precommit follows its existing channel.

Any failure causes zero root/child/monitor Xfer, no committed monitor counter,
clock-history or epoch change, and discard of pending observations. Failure uses
the existing latch and `SimFailureInfo`/executor path. Keep formatter fields and
failure policy. Environment-versus-DUT attribution can use check identity/message;
it does not justify a parallel event protocol. A failing sample is a failure
record, not a committed successful observation.

## Incremental execution obligations

The future monitor is a temporal/check effect, not cached pure computation.
Scheduling must account for committed monitor-state generation, sampled domain/
history/reset, operand versions including availability/four-state values, and
each check epoch/completion witness. Equal ordinary inputs cannot skip an active
monitor. Shared definitions may share immutable plans, never counters, active
flags, pending state or epoch records.

Skipping pure computation cannot skip capture or duplicate events. Discard
invalidates pending work; preparation recomputes from committed state without
double-counting. A failed whole-system Step currently requires Reset before
another Step; do not claim direct failed-Step retry.

Every Collection lane still needs a completion witness before commit. A worklist
cannot simply omit clean lanes and strand that barrier. FIFO maturation and
synchronous-memory response expiry are required counterexamples to skipping work
because external inputs did not change.

## Minimal examples and acceptance

The monitor rows below are specification traces, not generated execution results.

| Case | Expected interpretation |
| --- | --- |
| Concrete `x = not(x)` field dependency | Proven illegal combinational dependency; reject. |
| `next = Q + 1` committed on a sampled edge | Legal temporal feedback if next/control dependencies verify; no liveness conclusion. |
| Two downstream-pop ready outputs feed each other | Illegal combinational ready cycle despite storage. |
| Two local-occupancy status paths form an instance loop | Actual endpoint dependencies decide legality; instance cycles alone do not. |
| Start, `N=3`, permission true; misses on samples 1 and 2, discharge on 3 | Pass; deadline discharge wins. |
| Same trace without discharge on 3; unrelated Q keeps toggling | DUT bounded-progress failure before sample 3 commits. |
| Unbounded environment mode with permission always false | Legal waiting; eventual progress UNKNOWN. |
| `B=2`, permission false on subsequent samples 1 and 2 | Environment-contract failure on 2, not a DUT deadlock claim. |
| Inactive monitor and idle design | No progress obligation; existing quiescence rules apply. |
| Monitor would pass but sibling assertion/observation Precommit fails | Discard all DUT/monitor proposals and pending success observations. |

Existing fixtures cover the current boundary: `combinational-cycle.mlir`,
`queue-dependencies.mlir`, `async-memory-feedback.mlir`, routed-anchor Q/dead-
update/availability tests, and Runtime quiescence/clocked-hold/zero-commit tests.
Forged primitive summaries and coarse Table uncertainty must not be reported as
executed legal temporal feedback or general liveness proofs.

The design-first slice delivers this contract and existing-behavior evidence.
Keep future native admission/materialization, C++/managed RTL/high-profile
execution, eager/incremental differential checks, independent occurrences/lanes,
deadline/X/Z/reset/late-failure/discard/epoch tests and instrumentation-preservation
proofs open. Existing analysis/storage budgets apply before expansion; exhaustion
is not a runtime progress timeout. Historical format investigation stays deferred.
