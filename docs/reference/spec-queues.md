# Queue storage and dependency semantics

The revision B design (historical local record) defines one
FIFO contract across the common IR and both backends. The latency-one Runtime baseline
is accepted (historical local record). Common QueueOp
verification and dependency analysis are accepted (historical local record).
Python queue authoring and ordinary cross-source calls are
accepted (historical local record) for the bounded
[source contract](language.md#queues).
Generic C++/Verilog queue generation
is accepted (historical local record), including
generated-model lifecycle, family/token layout and installed consumers. See
implementation progress (historical local record).
The Q6 availability extension (historical local record) below
retains the same operation, complete-token capacity and commit boundary.

## Complete tokens and capacity

A queue owns `depth` complete tokens of type `T`. Supported Runtime tokens use
the existing `wire<T>` representation: finite bits, nested hardware structs and
fixed tables retain their value, known and Z planes. A table describes one token;
an enclosing module collection describes independent queues. These dimensions
must not be combined into one capacity. Waiting tokens consume the same slots
as available tokens; increasing availability latency never adds storage slots.

The internal `ac.queue` operation belongs directly to a module. It takes clock,
reset, input-valid, input-data and output-ready wires and returns input-ready,
output-valid and output-data. All control wires have width one. Input/output
payloads retain the same nominal type, not just the same packed width. Each
operation owns distinct state and is not a pure expression.

## Timing and ready policies

Outputs and transfers read committed state:

```text
count = all accepted, unconsumed tokens
eligible_count = available prefix of those tokens
out_valid = eligible_count != 0
out_data = out_valid ? storage[rd] : packed_zero<T>
do_pop = out_valid & out_ready
do_push = in_valid & in_ready
```

| Policy | Input ready | Full queue with both requests |
| --- | --- | --- |
| `local_occupancy` | `count < depth` | Pop only if the head is available |
| `downstream_pop` | `(count < depth) \| do_pop` | Pop and replace if the head is available |

The default policy has no combinational dependency on downstream ready. The
explicit replacement policy does. Valid and data depend only on queue state in
both policies. Dependency analysis still visits every queue input, including
unobserved connections, to reject dead combinational cycles.

There is no empty flow-through. For positive availability latency L, a token
captured on E0 reserves its slot at E0 Xfer, becomes available after E(L-1)
Xfer, and can first be consumed at EL. A Work read on the maturity edge still
observes the preceding unavailable state. A later held-clock Work can observe
the available token, but cannot transfer it without another rising edge.

Initialized queues with no available head return packed-zero data, even if they
contain waiting tokens. This is hardware zero rather than a source constructor
default. A full but unavailable queue cannot replace a token under either
ready policy. Once available, a token stays available under arbitrary stalls.

Simultaneous push/pop retains occupancy and advances both positions, consuming
the old head. Positions wrap at depth-1, including nonpower-of-two depths. At
L1, depth-one local-ready queues have half-rate steady throughput; depth two or
more can sustain one token per edge. At larger L, throughput also depends on
both depth and latency.

Head-read latency remains zero, empty flow false, read-during-write old, reset
synchronous high/empty and invalid data packed zero. Other axes still diagnose.
For L>1 the implementation uses one timestamp per existing slot, a bounded local
edge counter and an available-prefix cursor/count. At most one token matures
per edge, so metadata work is constant; there is no scan over depth or latency.
L1 has no timing substate. This is a complexity bound, not a measured performance
or synthesis-area claim.

## Preparation and commit

The Runtime kernel exposes pure `readReady`, `readValid` and `readData` functions.
They never prepare Work or change clock history. Work stages metadata and at most
one input token. Existing collection and system barriers govern Xfer/discard;
Xfer does not refresh a previously computed output snapshot.

Construction leaves ready, valid and data unknown. Host reset stages a reset;
successful Xfer establishes known-empty metadata and a known-false clock
history, as before. Hardware reset takes effect
on a rising edge and dominates handshake controls. Slots need not be cleared;
empty reads use packed known zero, independently of source constructor defaults.
Payload X/Z is preserved. Unknown controls fail only when an effective transfer
cannot be determined. Discard abandons token, metadata and clock proposals, including proposed age
progress. Age advances only on successful rising edges of that queue’s physical
clock, including idle-request edges. Pure reads, held/falling clocks and failed
whole-system checks never advance it. A sibling failure on a maturity edge must
commit neither the new eligibility nor any sibling state.

## Verification boundaries

Queue configuration uses the existing static-expression grammar, lexical module
scope checks, type substitution and packed-layout analysis. Symbolic definitions
may retain their declared formals. Every evaluable constraint is checked, and
each demanded concrete occurrence must resolve completely. Explicit bound
aliases are checked for cycles; defaults and ordinary Python locals do not
become static facts through this analysis.

Logical widths, `depth * packed_width` and additional timing geometry use
checked u64 arithmetic. Timing constraints are checked even while payload T is
symbolic. Payload storage and timing metadata remain separate quantities. Table axes
and token cardinality retain the existing signed-64 constraints. Target-specific
plane width and single-queue/family object capacity belong to shared emitter
preflight before allocation or publication. No target allocation is performed by
QueueOp validation.

Closed positive-u64 latency values, including UINT64_MAX with small depth, use
explicit unsigned constants in both targets. Ordinary generic module integer
actuals and symbolic arithmetic retain their existing signed-64 target limits.
This does not expand Python parameter elaboration. Finite huge-latency tests
cover construction, early state and reset; they do not execute the whole wait.

This internal operation does not expand scalar module type arguments to tables.
Use a table token around an existing scalar type argument, such as `table<3,T>`.
Queue result dependencies are derived from the real operation; published headers
cannot replace body verification or introduce an undeclared bypass.

The accepted Q0 snapshot rejected QueueOps at shared emission preflight, including
owners with unused outputs. Q23 replaces that guard with both emitters and shared
target-capacity checks. Every queue owner is emitted even when its results are
unused. Independent generated effect/failure and publication checks cover this
boundary. Preflight also validates unused concrete definitions because their
emitted wrappers still allocate storage; it does not infer generic defaults.
