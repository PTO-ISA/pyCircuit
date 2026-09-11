# TMU.BGF.RQ — Read Queue

- Source candidate: `DAV-TMU-BGF-RQ-0001`
- Hardware hierarchy: **H3**, within H1 `TMU` / H2 `BGF`
- NDF refinement: **L2 microarchitecture**; module-specific L1 behavior links still require review.
- Recommended disposition: **state_schema** (proposal, not registry approval)
- Implementation placement: use the accepted containing owner or contract file under `designs/davincioo/`; no independent leaf is authorized by this inventory disposition.
- Current design-program execution status: **source implementation present;
  gfsim verification pending**. The implementation is parent-composed and does
  not claim independent RQ ownership.

RQ is where a read waits: one queue per source class, holding requests that MAP
has already placed but the fabric cannot take yet.

## What problem it solves

MAP has already worked out where a read goes, but the bank it needs may be busy
this tick because another source class is using it. The request has to wait
somewhere, and RQ is that somewhere -- one queue per source class.

The important part is what RQ does *not* do. It makes no decision of any kind:
it never looks at the contents of the record it holds, never chooses which
request goes first, and carries no bank dimension. Choosing what goes first is
ARB's job. That is checkable rather than only stated: the lowered IR of
`bgf_rq_system` contains no `ac.route` and no `ac.var.get` at all.

One queue per class is what the isolation rests on. A class that backs up cannot
consume another class's space, and a blocked PE instance cannot stall another.

RQ is also not an independent state owner. It names the parent-owned read Queue
arrays, and queue occupancy stays with the BGF assembly that contains them; RQ
must not become a policy or storage owner separate from that assembly.

## Where it sits

A cell read crosses BGF in a fixed order: a client addresses a cell, MAP decodes
where that cell lives, RQ holds the request until the fabric can take it, ARB
fans it out to banks and resolves conflicts, XBAR delivers the grant, and BANK
moves the bytes. RQ owns one step of that -- the waiting.

| Module | Owns | Does not own |
| --- | --- | --- |
| CUBE / VEC / TLSU | Which cell to read | Which bank holds it |
| [MAP](map.md) | Decoding `cell_key` into `(bank, row)` | Queueing or conflict resolution |
| **RQ** | **One queue per source class, and FIFO order within a class** | **The record's contents, any choice between requests, any bank dimension** |
| [ARB](arb.md) | The bank crossbar, conflict resolution, fairness and aging | Storing requests |
| [XBAR](xbar.md) | Delivering a granted request to its target bank | Which request is granted |
| [BANK](../trf/bank.md) | The data bytes themselves | Where a cell lives, or which request goes first |

Writes are the mirror image: [wq.md](wq.md) is the same schema for
`CellWriteReq`, and RQ does not order its read heads against WQ's write heads.
Further out on the Tile side, [FRE](../trn/fre.md) decides which physical block a
version occupies and [REF](../trf/ref.md) counts how many readers it still has.
Neither is visible from here.

## How one request passes through

A `CellReadReq` arrives from MAP already carrying `source_class`, `bank`, `row`
and `out_of_range`. Its `source_class` selects the queue it joins, and that queue
has `CLASS_RESIDENCY_DEPTH` slots. The head of each class queue is exposed to
ARB, and a head is consumed only once ARB has actually taken it. While ARB, XBAR
or a response slot is blocked, the head stays where it is and the backpressure
reaches that one class only.

The record passes through unchanged; the identity transform exists to carry
`CLASS_RESIDENCY_DEPTH`, because the queue front end has no standalone buffer
operator and an external port is always `depth 1`.

`CLASS_RESIDENCY_DEPTH` is the whole in-flight allowance of one class, not a
per-bank allowance. `BANK_PARTITIONS` is its starting value: ARB retires at
most one request per bank per cycle, so a class holding more than
`BANK_PARTITIONS` resident requests cannot have them all conflict-free. This
is a starting point, not a frozen sizing.

## Inputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| cube_read / vec_read / tlsu_read | CellReadReq | Generation-qualified read of one source class of one PE, already tagged and placement-decoded by [map.md](map.md) | implemented seam |

## Outputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| cube_read / vec_read / tlsu_read | CellReadReq | One resident request stream per source class, exposed to this PE's ARB through parent wiring; 3 streams per PE instance | implemented seam |

Payload names in proposed rows are design pseudotypes until fields, widths and nominal identity are frozen. Queue transport is inferred by the compiler, not a requested public Queue wrapper. Clock/reset/time domain are execution context and must not be invented as ordinary payload ports.

## Owned or containing state

- Queue occupancy belongs to the BGF parent, not an independent RQ module

## Reference implementation

[`rq.py`](rq.py) defines `bgf_rq_system`; `CellReadReq` and the placement
geometry are shared through
[`contracts/tmu_bgf.py`](../../contracts/tmu_bgf.py). RQ holds one queue per
client class of `CLASS_RESIDENCY_DEPTH` slots and does nothing else. This is a
transport schema; the containing BGF assembly remains the state owner.

## Departures from the original proposal

**RQ has no bank dimension.** Bank is a scheduling coordinate, so the whole
bank dimension belongs to [arb.md](arb.md), the leaf that resolves bank
conflicts. `bank` and `row` ride in the payload -- decoded once by
[map.md](map.md), read once by ARB -- and RQ transports them without inspecting
them. Splitting ARB's crossbar so that its input side sat in RQ would elaborate
exactly the same `(source_class, path, bank)` edges and provide neither more nor
less isolation, because the head-of-line limit sits at the fan-out input in
either arrangement.

**FlowKey no longer selects anything.** The card treats a flow as a routing
dimension, but one elaboration of `bgf_rq_system` is one PE, so the routing use
of FlowKey disappears into instance geometry. FlowKey still rides in the record
for identity and cancellation matching, which is why the mapping below still has
to be frozen even though nothing in RQ reads it.

**Queue depth rides on an identity transform.** There is no requested public
Queue wrapper to instantiate and no standalone buffer operator at a queue front
end, so the only place to attach `CLASS_RESIDENCY_DEPTH` is a transform whose
body returns its input untouched. The upside is that the empty body is what
makes "RQ decides nothing" a property of the lowered IR instead of a claim in
this card.

## Physical premise

The four PE control flows in [architecture](../../ARCHITECTURE.md) are
physically independent: each PE owns a private group of cell-register banks and
its own arbitration. [bank.md](../trf/bank.md) records 32 single-port 128-byte
banks in private groups, that is **8 banks per PE**.

This seam therefore describes exactly one PE. PE identity is expressed by
instantiating `bgf_rq_system` once per PE, not by a routing dimension inside
it, and no queue, cursor, age, or fairness state is shared across PEs. The
parent assembly performs the four-way instantiation.

## Proposed queue partition (review required)

RQ is a parent-owned queue schema, but the BGF profile still needs an explicit
logical partition. The partition is `source_class` alone. The following
topology is a proposal for review, not a registry-approved implementation:

```text
one PE instance
CUBE / VEC / TLSU                      (address cells; no bank concept)
              |
              v
             MAP                       (tag source_class; decode bank/row)
              |
              v
      read_q[source_class]             (RQ; residency only, bank-blind)
              |
              v
             ARB                       (owns the bank crossbar and conflicts)
              |
              v
          XBAR/BANK                    (per PE, no cross-PE sharing)
```

- `source_class` is a closed class tag over `CUBE`, `VEC` and `TLSU`, the only
  units that access cell registers; MAP writes it before this seam and it must
  remain attached into `BankGrantReq`.
- PE identity is instance geometry, not a partition key inside the module.
  Because each PE is elaborated separately, four PE flows with equal logical
  IDs cannot share FIFO ordering by construction.
- `bank` is a PE-private bank index in `[0, BANK_PARTITIONS)`. Since each PE
  owns its banks privately, this index is the physical bank within that PE and
  needs no separate bank-group level. Within BGF it is a payload field
  precisely so that RQ needs no bank dimension: MAP writes it and ARB reads it,
  and nothing between them is elaborated per bank.
- Per-bank independence is provided by ARB, which buffers every
  `(source_class, path, bank)` edge of its crossbar separately, so a bank
  blocked downstream holds only its own edges.
- A queue entry carries, at minimum, the complete `CellReadReq` identity:
  `FlowKey`, epoch/generation, `TileVersion`/`CellKey`, request ID and response
  route. There is no sub-cell region to carry: access granularity is one whole
  cell, so the shared record has no element-range or byte-mask fields. MAP appends
  `source_class`, `bank`, `row` and `out_of_range`; nothing in this seam replaces
  source identity.

## FlowKey to `CellReadReq` field mapping (review required)

`FlowKey(core_id, pe_id, stid, launch_generation)` is defined in
[architecture](../../ARCHITECTURE.md). The shared `CellReadReq` has not
frozen its correspondence to that tuple; the table below is a proposal for
review, not approved field semantics. FlowKey no longer participates in
routing -- it is carried for identity and cancellation matching -- but the
mapping still has to be frozen for cancellation to work at all.

| FlowKey component | Candidate field | Type | Note | Evidence status |
| --- | --- | --- | --- | --- |
| `core_id` | none | — | Implied by instance context if BGF is instantiated per core; an explicit field is required if a bank group is shared across cores. | unresolved |
| `pe_id` | none needed for routing | — | Implied by instance context: one `bgf_rq_system` elaboration is one PE. An explicit field is still required if a consumer downstream of the parent must recover which PE issued the request. | unresolved |
| `stid` | `thread_id` | `u16` | Candidate. The reference elaboration assumes one `stid` per PE; more than one would require a further partition inside the instance. | proposed |
| `launch_generation` | `launch_generation` | `u16` | Carried in the shared contract; `allocation_generation` is the Tile allocation generation and differs in meaning, so it must not be reused. Producer/consumer semantics are not yet frozen. | implemented field, semantics proposed |
| whole handle | `flow_id` | `u16` | If `flow_id` is a parent-allocated FlowKey handle, it subsumes the components above. | proposed |

The table admits two mutually exclusive interpretations, and one must be
frozen:

- **Interpretation A (handle)**: `flow_id` is the complete FlowKey handle, and
  `requester` and `thread_id` are derived debug/routing information.
- **Interpretation B (components)**: FlowKey is composed of `(core_id`,
  `pe_id`, `thread_id`, `launch_generation)`, where `core_id` and `pe_id` are
  implied by instance context; `flow_id` then means something else or is
  removed.

Per-PE instantiation removes the partition-key consequence of this choice, so
the remaining consequence is cancellation matching, which must match `FlowKey`
and epoch/generation without removing a newer generation. Without a launch
generation that match cannot distinguish the same flow before and after
recovery, so `rq.py` and `wq.py` carry `launch_generation` as an explicit
`u16` field under either interpretation.

The following fields are request identity rather than flow identity. They take
no part in FlowKey but must still be retained in full per the requirement
above: `block_id`, `operation_id`, `tile_id`, `tile_version`,
`allocation_generation`, `request_id`, `cell_key`, `response_route`.

## Queue protocol and ordering

- `read_enqueue` is accepted only when the selected class queue has capacity
  and the downstream reservation policy permits eventual progress.
- Each class head is consumed only on an accepted transfer into ARB. A blocked
  ARB, XBAR or response slot must retain that head entry and propagate
  backpressure to its own class only; no class can stall another, and no PE can
  stall another. Backpressure from one bank reaches a class only through ARB's
  crossbar edge for that class, so it cannot reach a different class at all.
- FIFO order is required per `source_class` within a PE instance. Cross-class
  ordering is not provided by RQ; ARB owns conflict scheduling and fairness
  after mapping.
- RQ must not merge requests from different classes. It exposes one head per
  class and performs no selection of its own; any round-robin or priority
  choice belongs to ARB, whose fixed-priority order and aging escalation are
  recorded in [arb.md](arb.md). In particular, RQ does not order its read heads
  against the WQ write heads of the same bank; ARB does.
- Recovery may cancel only entries that have not been accepted by the next
  owner. Cancellation matches `FlowKey` and epoch/generation and cannot remove
  a newer generation.

## Required capabilities to verify

- Parent-owned parameterized Queue arrays
- Borrowed child Queue ports without state duplication
- Per-class residency for CUBE, VEC and TLSU traffic with no bank dimension
- Per-class depth, occupancy and enqueue/dequeue accounting
- Preservation of request identity, class tag and placement fields from the
  MAP handoff onward, unread by this seam
- Per-PE instance isolation with no shared queue or fairness state

These are requirements, not proof that the current framework is missing each one. First test the current revision; a demonstrated gap becomes a generic framework/primitive issue and regression before a dependent design PR.

## Behavioral acceptance

- FIFO order per `source_class`
- No request loss under mapping/grant backpressure
- Independent capacity and backpressure for each class; a blocked class does
  not stall another class, and a blocked PE instance does not stall another.
  Per-bank independence is ARB's acceptance obligation, not RQ's
- No reordering within a class
- No independent arbitration or Tile completion semantics
- No placement field is read: the lowered IR contains no field access

gfsim execution is the first implementation gate. PYC/RTL obligations apply to the admitted lowering and remain explicit future work where provisional storage is rejected. Compile-only evidence does not establish behavior.

## Open decisions

- Freeze `CLASS_RESIDENCY_DEPTH` per source class for the selected profile.
  `BANK_PARTITIONS` is a starting value derived from ARB's per-cycle retirement
  bound, not a sized decision. The bank dimension itself is settled and belongs
  to ARB: 8 PE-private banks, 32 across the four PEs, per
  [bank.md](../trf/bank.md).
- Freeze the FlowKey to `CellReadReq` field mapping between interpretation A
  and interpretation B, and settle whether `core_id`/`pe_id` need explicit
  fields for consumers downstream of the parent.
- Confirm the one-`stid`-per-PE assumption. If a PE hosts several `stid`
  values, add a partition inside the instance rather than reintroducing a
  cross-PE routing dimension.
- Define cancellation matching and stale-generation handling for each source
  class; fairness across classes and banks is ARB's, not RQ's.

## Contributor closure

- [ ] Claim the candidate and identify its parent/containing state owner.
- [ ] Resolve disposition; aliases and contained state must not duplicate hardware.
- [ ] Link the relevant NDF L0 intent and L1 behavior to this L2 implementation.
- [ ] Freeze port payload fields/widths, producer/consumer, parent seam, state/reset and timing profile.
- [ ] Freeze CUBE/VEC/TLSU queue dimensions, class tags and per-class depths.
- [ ] Prove per-class FIFO, independent backpressure and stale-generation cancellation.
- [ ] Define functional branches, all-or-none effects, contention and cancel/recovery lifecycle.
- [ ] Link a minimal failing gate for each actual framework/primitive gap and merge that shared fix first.
- [ ] Implement the accepted owner and design-local expected-result tests.
- [ ] Prove backpressure, identity/generation, exactly-once effects and isolated instances in gfsim.
- [ ] Integrate into H2/H1 and record admitted PYC/RTL evidence or remaining boundary.

## Source evidence

- `docs/architecture/core/l3/TILE_MEMORY_PACKETS.md:105` — RQ is explicitly recommended as a state schema.
- `docs/architecture/core/l3/CONTRACT_FOUNDATION.md:86` — Nearest common parent owns seam Queues.
- `docs/architecture/core/DAVINCIOO_MICROARCHITECTURE.md:738` (via [bank.md](../trf/bank.md)) — 32 single-port 128-byte banks; the first profile uses private groups, giving 8 banks per PE.
- [architecture](../../ARCHITECTURE.md) — four independent PE control flows qualified by `FlowKey(core_id, pe_id, stid, launch_generation)`.

Source paths use the frozen external repository spelling, including legacy `l3/` directories; they are provenance, not the new hierarchy vocabulary. File hashes and the original catalog disposition are in [catalog.json](../../catalog.json). See [architecture](../../ARCHITECTURE.md) for unresolved global decisions.
