# Queue pipeline fixture notes

The retained fixtures use ordinary typed modules, registered rules, Tables and
`ac.queue` through the current source compile/link/emit flow. Reference models
and independent checkers live beside this file. They never provide DUT values or
enter product lowering.

Historical sources are the three pipeline sources at
`8887e6dec7b4cc530a9967c860dc6a224d79a4ab`. Their old `source`, `route`, `credit`,
`reorder`, `merge` and `sink` authoring operations are historical references,
not current primitives or compatibility APIs. Current `@system` remains an
active language feature. The testbench is a separate source fixture.

## Credit pipeline

`CreditToken` contains `sequence: u4`, `cycles: u4`, and `value: u16` (24 bits).
`CreditResult` contains queue capacity `ready`, output `available`, and the
unchanged token (26 bits). Two depth-4, latency-1 queues use
`local_occupancy`; the two credit slots are a module register.

Admission reads old slot occupancy. The lowest free slot wins. Admission stores
the declared cost without decrementing on that edge. Occupied slots count down;
a done slot returns its credit only when its push into the completed queue
commits. Lowest slot index wins simultaneous completion. A slot freed on an edge
is first available to admission on the following edge. `ready` stays input queue
capacity, independent of credit availability and cost validity.

Inside `Advance`, an available issued head with a free credit slot asserts
`cost != 0`, with the message `credit_nonpositive_cost`. A zero-cost head is
not inspected while both slots are occupied. At the first effective view with
a free slot, failure discards the whole system's proposals. This restores the
historical fault boundary; permanent stall with continued sibling commits is
not the current fault contract. The fixture keeps its existing admission masks,
queue depths, latency and policies.

Only the historical two-slot configuration is implemented by this fixture.
Its constants do not establish arbitrary source parameter elaboration.

## Route and priority merge

The payload is `u64`: it is both the selector and the arithmetic operand.
Selectors exactly 0 and 1 route left and right, then add 10 and 20 respectively.
Six queues retain their depths: input 2, left/right 2, left/right completed 1,
merged 2. Every queue has latency 1 and `local_occupancy`. A full queue cannot
reuse a same-edge pop. The depth-1 transform queues therefore retain their
historical half-rate cadence.

Selection inspects only the input head. Merge gives strict priority to the left
completed queue; it has no aging, round-robin state or global source-order
promise. A registered `CheckRoute` rule asserts that every available head has
selector 0 or 1, with message `route_selector_out_of_range`. This inspection
is independent of downstream capacity. Invalid selectors never pop or push;
the failed sampling epoch also prevents every sibling commit.

## Reorder pipeline

`Token` has `sequence: u32` and `value: u32`. The input queue has depth 8,
latency 1; the output queue has depth 4, latency 1. Both use
`local_occupancy`. The table has 16 entries, and `next_key` resets to 0.
Capacity counts occupied entries; it is not a key window or a modulo address.
Keys at least 16 remain legal. Entries store a zero-extended `u64` key and the
complete token. `next_key` is `u64`, matching the historical block's semantic
counter domain without changing the token's declared 32-bit sequence.

All searches inspect committed table state before writes. Retirement requires
a matching `next_key` and output space. Admission requires old free capacity;
a table slot retired on an edge cannot be reused on that same edge. A missing
key waits and is never skipped. Admission and retirement target disjoint slots.

Inside `ReorderStep`, an available head with free table capacity asserts that
its key is not stale and is not already present. Messages are
`reorder_stale_key` and `reorder_duplicate_key`. A full table defers key
inspection even when a matching entry could retire on that edge. Failure
prevents input pop, table insertion, output push, key advancement, and every
sibling commit. Unsigned `sequence` makes the historical negative-key branch
inexpressible. The existing `next_key` and `fault` result fields are retained;
they do not replace runtime checking or gate the data path. The packed result
remains 131 bits.

## Execution evidence and remaining boundary

`fault_systems.py` supplies separate genuine closed test systems using the
asserted DUTs. Immediate route, zero-cost, stale and duplicate failures are
accompanied by a sibling counter and a delayed queue. Additional systems cover
route inspection with a full selected downstream queue, zero-cost inspection
deferred by occupied credit slots and duplicate inspection deferred by a full
reorder table. There are no log/report observations; checks
must remain effective even when DUT result values are unused.

The existing `queue-source.py --fault-checks-only` harness compiles each DUT
independently, compiles the separate system bench against their published
interfaces, and links/emits the complete explicit closure through the public
flow. Token constructors supply every field through the imported nominal type.
It does not change product code, introduce a runner, or execute DUT Python.
Native read-access instrumentation operates on copies of emitted headers and
the collection storage header, changing visibility alone.
The native and managed RTL probes compare all committed register and queue
state, queue storage, pointer/count state, clock history and delayed queue timing
across failed Work, denied Xfer, Discard and retry. Native uses workers 1 and 2.
Reset/replay is bounded. Current checks inspect each sampling Work view, including
the falling-clock view after a successful edge; they are not silently restricted
to rising edges.

`AtomicSiblingFault` separately rejects the first rising Work with an ordinary
source assertion. Read-only probes first verify a prepared counter update,
token push, delayed tick and clock proposal, then check that failure leaves all
committed data and history unchanged. This strengthens the sibling atomicity
proof without changing the actual pipeline faults' sampling timing. A first-edge
empty-queue probe does not establish discard on a queue maturity edge.

The generic native protocol code is `source_check_failed`; the messages identify
these four faults. This does not claim restoration of the old runtime failure
codes as protocol identities. RTL uses the existing generic check-failure
channel. A command or fixture is not a passing result; consult the candidate's
recorded commands and receipts for actual outcomes.

The original full-duration throughput, capacity, timing, physical-clock/reset
and rival-policy vectors remain in their independent owners. The credit
independent checker's exported `vectors()` is now serialized as
`credit_independent` by the same native/RTL vector harness, with its own execution
entry. Its command-line checker remains a reference-model self-check and must
not be reported as an emitted-artifact checker.

Checked ordinary module roots use the required managed RTL
phase/permission/error boundary and the existing native whole-system Precheck.
The original vector drivers prepare every row, freeze permission from the
complete error channel, and commit or discard. Failed rows have no public sample;
the drivers retain and drive the complete failed suffix.

Native failure remains latched until host Reset. A physical reset pin alone
cannot recover that execution. `execution_record()` preserves every original
stimulus and oracle value, and separately labels host-Reset recovery segments
with independently derived empty-state reset expectations. Successful
physical-reset scenarios still compare their original pre-reset samples.
The credit independent model's eight sections also require explicit host
resets at their existing boundaries; its complete 384 rows are retained.

Separate system fault passes do not establish full positive vector closure.
Full acceptance requires actual runs of these complete vectors on native
workers 1 and 2 and managed RTL, including native/Icarus four-state scenarios,
plus the independent checkers. Consult the candidate's recorded commands and
receipts; fixture registration or dual emission is not a passing nightly gate.
