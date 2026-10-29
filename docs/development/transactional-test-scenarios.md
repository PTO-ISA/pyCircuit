# Transactional test scenarios

Issue #361 tests are organized around six readable designs. Source files live
beside the existing lit drivers in `tests/compiler/lit/CodeGen/Inputs/`.
Drivers supply independent numerical histories; they do not derive expectations
from emitted IR or another backend. `transactional_test_support.py` only handles
compilation, saved artifacts, public runtime adapters, execution and log decoding.
There is no additional runner.

This cleanup starts at `c9d75cb9`. The implementation series begins at
`d2f2ffee`, after `b986bb0a`. Historical tests changed only for operation spelling,
source printing, fixture discovery or API naming retain their original scope.
The printer's own display tests remain printer tests.

## Scenarios and backend selection

| Group / lit entry | Design and independent observations | Physical C++ | RTL | t-gfsim |
| --- | --- | --- | --- | --- |
| State: `transactional-state-machine` | Paused stages, explicit/unregistered Rules, autorule, old Q, saved snapshots, narrow/wide wrapping and mid-run System reset | workers 1/2 | Verilator | worker 1 |
| Control: `transactional-control-flow` | Module/Rule/Method branches, local joins, early return and selected guard cancellation | workers 1/2 | Verilator; Icarus for X/Z companion | scalar design only |
| Calls: `transactional-sibling-methods` | Two callers share state; separate instance, scalar/tuple/ignored results, nested calls, guard cancellation and independent progress | workers 1/2 | Verilator | worker 1 |
| Queue: `transactional-queue-pipeline` | Finite stream, atomic two-output fanout, blocked consumer, cross-owner effects, exactly-once ordering | workers 1/2 | Verilator | NOT RUN: Queue unsupported |
| Data: `transactional-structured-packet` | Enum/nested Struct, 80-bit payload, independent tuple outputs, stored payload, X/Z transport and occupied-Queue reset | workers 1/2 | Icarus | NOT RUN: Queue/nominal payload unsupported |
| Failure: `transactional-failure` | One legal source assertion fails after successful commits; multi-owner/Queue proposals and pending logs, public failure/reset lifecycle | workers 1/2 | Verilator | NOT RUN: Queue unsupported |

Selections are explicit, before compilation. A compile failure is a failure,
never an automatic skip or an expected-rejection success. The API gate and G1
both select `CodeGen/transactional-*.test`; API nightly uses the complementary
filter, so it does not execute the gate subset again.

The control group has small companions for implicit Queue inputs, nominal
branch results and unused formal availability. They preserve capabilities that
cannot be established by a scalar command processor. t-gfsim runs the same
supported sources and oracle as the physical targets.

## Necessary supplements

| Entry | Why it remains |
| --- | --- |
| `transactional-constraints` | Natural-width arithmetic before widening, wide shift counts, literal constraint propagation, SSA snapshots and ignored Method results |
| `transactional-state-interfaces` | Parent-owned State output binding, global Rule templates, aliases, hold and public Module-port reset adapter |
| `transactional-uninitialized` | Omitted Bits/Enum/Struct initialization is X; typed public outputs verify write, hold and reset |
| `transactional-queue-timing` | Same-Queue pop/push at middle occupancy, empty no-bypass and full no-borrowing |
| `transactional-resource-composition` | Multilayer Module references, mixed State/Queue tuple bindings and separately admitted Rules; existing independent oracle retained |
| `transactional-linked` | Saved Linked → Routed anchor → Final continuation, followed by numerical execution |

The Calls case also builds its generated t-gfsim program against installed
Runtime with LLVM/MLIR discovery disabled. It reuses that case's design and
expected history. Cumulative/batch and other complete examples retain their own
example registrations and functionality; duplicate API instrumentation is removed.

## Remaining verification gaps

The Queue pipeline separates guard cancellation from capacity blocking. The
Method guard is false only at cycle 2, with both outputs below capacity. The
right output becomes full after cycle 3; with the guard true, cycles 4–7 must
retain input and both owners' State without producing either output. The first
right pop at cycle 7 cannot lend capacity to that transaction. It resumes at
cycle 8. Explicit model records check these boundaries, and the full public log
history checks independent progress and all six ordered results.

The State and Data groups execute their retained `@system(samples=...)` reset
schedules through independently compiled units, portable saved Linked reload,
Routed/Final materialization and generated simulators. State reset repeats its
initial transitions after restoring stage, counters and narrow/wide storage.
Packet reset clears occupied Queues and restores structured State before the
complete stream restarts, including its 80-bit X/Z payload. These schedules
subsume the normal histories, so their lit entries replace duplicate normal
runs. Systems without samples retain their default low/high cycle driver.

After `SimSystem::Step()` reports failure, the public generated System cannot
sample again before Reset. Its unchanged observation snapshot proves suppressed
publication, not unchanged internal Q or Queue storage. The failure scenario
checks the failure state, epoch, prior successful numerical history, withheld
logs and clean Reset/restart. Direct post-failure generated State/Queue storage
inspection remains **unverified**. The existing native t-gfsim runtime assertion
oracle is retained as a bounded supplement because public `State::Q()` can
verify discarded proposals there; it uses Reset rather than retry after failure.
That runtime oracle does not establish generated Queue rollback.

These are implementation/observation gaps, not permission to weaken the
behavioral contract. This follow-up restores finite System stimulus admission and propagation; reset
and transaction semantics are unchanged.

## Follow-up validation (2026-10-29)

Local work packages A (backpressure) and B (System samples) are implemented; no
PR has been published. Excluding shared documentation, A changes two existing
files (+25/-2 lines); B changes ten existing files (+82/-26 lines). No test
entry, runner, rejection matrix or structural assertion is added.

| Current candidate check | Result |
| --- | --- |
| Queue pipeline, physical C++ workers 1/2 and RTL | PASS |
| Existing Queue timing supplement | PASS |
| State reset, physical C++ workers 1/2, RTL and t-gfsim worker 1 | PASS |
| Packet reset, physical C++ workers 1/2 and four-state RTL | PASS |
| All 12 transactional lit entries and source display entry | PASS (13 entries) |
| Native `ACIRTransactionalGfsimTests` | PASS |
| Changed-file pre-commit and strict documentation build | PASS |
| Queue/nominal t-gfsim; historical API, examples and nightly | NOT RUN |
| Direct generated Queue storage observation after failure | NOT RUN; gap retained |

Evidence is retained under ignored
`docs/gates/logs/20261029-system-samples/`, including commands, candidate content,
raw logs and statuses. The initial bare-System compiler regression was fixed
before the successful full run. Multi-file Black hung and was interrupted;
individual changed-file invocations of the same pre-commit hooks passed.

## Coverage migration

“Removed” is never reported as PASS. Rejection and structural checks below are
removed from both gate and nightly, together with orphan fixtures and targets.

| Original test family | Retained behavior destination | Removed checks | Outstanding coverage |
| --- | --- | --- | --- |
| `transactional-state`, `source`, `issue361` | State group; Calls/Failure; complete example gates | Native IR scaffolding, mutation, exact structure, duplicate example execution, compile/link rejection | No additional State reset matrix claimed |
| `conditional-methods`, `branch-results`, `early-returns` | Control group and its short companions | Illegal branches, exact `scf.if`/`yield` form, generated variants | No exhaustive syntactic variation matrix |
| `methods`, `sibling-methods` | Calls group | Recursion/illegal-write/unsupported-call rejection, private-state assertions, duplicate compile/link drivers | None claimed beyond the stated scalar/tuple call forms |
| `queue-pipeline`, `queue-timing`, queue replay | Queue group and same-Queue timing supplement | FIFO configuration/proposal tampering, conflict rejection, generated-kernel inspection, depth/guard variant matrices | Failed Queue storage observation above |
| `structured-packet` | Data group; nominal Control companion; uninitialized supplement | Wrong nominal identity rejection, physical mutation, duplicate Linked lane | No additional reset matrix claimed |
| `resource-composition` | Existing positive composition/mixed/separate designs and oracle | Alias-conflict rejection, allocation/IR structure checks | None claimed beyond retained combinations |
| `constraints` | External positive sources and independent numerical oracle | Mismatched Queue type, malformed literal, failed-publication byte checks | Annotated Queue declaration executes in Queue pipeline |
| `state-interfaces`, `uninitialized` | External designs, public output/reset adapters | Mismatched binding rejection, allocation counts, generated source-group inventory | No added type matrix |
| `linked`, `correspondence` | Saved-anchor numerical supplement; Calls saved closure | Source-unit tampering, byte equality, op strings, duplicated direct/resumed execution | None claimed beyond successful continuation |
| `gfsim` | State/Control/Calls sources, Runtime-only build and native runtime foundation | Unsupported-backend rejection, generated harness substitution, duplicate examples/failure modes | Queue/nominal direct target remains unsupported |
| `IR/transactional-*` | Behavior in the six groups and supplements | All nine new rejection/structural/CSE/roundtrip fixtures | Printer functionality stays in `acir-display` |
| `SourceResourceBindingTests`, `TransactionalMethodConstraintsTests` | Calls, parent State, resource composition and type arithmetic histories | New native op counts, attributes, deferred-op shape, mutation and standalone targets | No compiler diagnostic/shape coverage claimed |
| New stale-callable test in `test_saved_anchor.py` | Existing successful saved-anchor tests remain | Old-spelling IR mutation and exact regeneration diagnostic | Historical anchor tests unchanged |
| `TransactionalGfsimTest` | Old-Q/log publication/reset lifecycle and public State rollback oracle | Additional unknown-control failure case and retry-after-failure sequence | Generated Queue rollback remains unverified |

The cleanup does not remove compiler checks, move removed tests to nightly,
rewrite frozen approvals, or reduce the functionality of complete examples.
