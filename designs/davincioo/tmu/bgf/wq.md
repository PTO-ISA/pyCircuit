# TMU.BGF.WQ — Write Queue

- Source candidate: `DAV-TMU-BGF-WQ-0001`
- Hardware hierarchy: **H3**, within H1 `TMU` / H2 `BGF`
- NDF refinement: **L2 microarchitecture**; module-specific L1 behavior links still require review.
- Recommended disposition: **state_schema** (proposal, not registry approval)
- Implementation placement: use the accepted containing owner or contract file under `designs/davincioo/`; no independent leaf is authorized by this inventory disposition.
- Current design-program execution status: **source implementation present;
  gfsim verification pending**. The implementation is parent-composed and does
  not claim independent WQ ownership.

WQ is where a write waits: one queue per source class, holding requests that MAP
has already placed but the fabric cannot take yet.

## What problem it solves

MAP has already worked out where a write goes, but the bank it needs may be busy
this tick because another source class is using it. The request has to wait
somewhere, and WQ is that somewhere -- one queue per source class.

The important part is what WQ does *not* do. It makes no decision of any kind:
it never looks at the contents of the record it holds, never chooses which
request goes first, and carries no bank dimension. Choosing what goes first is
ARB's job. That is checkable rather than only stated: the lowered IR of
`bgf_wq_system` contains no `ac.route` and no `ac.var.get` at all.

One queue per class is what the isolation rests on. A class that backs up cannot
consume another class's space, and a blocked PE instance cannot stall another.

WQ is also not an independent state owner. It names the parent-owned write Queue
arrays; a queued write is real state, but that state belongs to the BGF assembly
that contains these queues and does not justify a second policy owner.

## Where it sits

A cell write crosses BGF in a fixed order: a client addresses a cell, MAP decodes
where that cell lives, WQ holds the request until the fabric can take it, ARB
fans it out to banks and resolves conflicts, XBAR delivers the grant, and BANK
stores the bytes. WQ owns one step of that -- the waiting.

| Module | Owns | Does not own |
| --- | --- | --- |
| CUBE / VEC / TLSU | Which cell to write, and the data | Which bank holds it |
| [MAP](map.md) | Decoding `cell_key` into `(bank, row)` | Queueing or conflict resolution |
| **WQ** | **One queue per source class, and FIFO order within a class** | **The record's contents, any choice between requests, any bank dimension** |
| [ARB](arb.md) | The bank crossbar, conflict resolution, fairness and aging | Storing requests |
| [XBAR](xbar.md) | Delivering a granted request to its target bank | Which request is granted |
| [BANK](../trf/bank.md) | The data bytes themselves | Where a cell lives, or which request goes first |

Reads are the mirror image: [rq.md](rq.md) is the same schema for `CellReadReq`,
and WQ does not order its write heads against RQ's read heads. Further out on the
Tile side, [FRE](../trn/fre.md) decides which physical block a version occupies
and [REF](../trf/ref.md) counts how many readers it still has. Neither is
visible from here.

## How one request passes through

A `CellWriteReq` arrives from MAP already carrying `source_class`, `bank`, `row`
and `out_of_range`. Its `source_class` selects the queue it joins, and that queue
has `CLASS_RESIDENCY_DEPTH` slots. The head of each class queue is exposed to
ARB, and a head is consumed only once ARB has actually taken it. While ARB, XBAR
or a BANK acknowledgement slot is blocked, the head stays where it is and the
backpressure reaches that one class only.

The record passes through unchanged, data included; the identity transform exists
to carry `CLASS_RESIDENCY_DEPTH`, because the queue front end has no standalone
buffer operator and an external port is always `depth 1`.

`CLASS_RESIDENCY_DEPTH` is the whole in-flight allowance of one class, not a
per-bank allowance. `BANK_PARTITIONS` is its starting value: ARB retires at
most one request per bank per cycle, so a class holding more than
`BANK_PARTITIONS` resident requests cannot have them all conflict-free. This
is a starting point, not a frozen sizing.

## Inputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| cube_write / vec_write / tlsu_write | CellWriteReq | Generation-qualified whole-cell write of one source class of one PE, already tagged and placement-decoded by [map.md](map.md) | implemented seam |

## Outputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| cube_write / vec_write / tlsu_write | CellWriteReq | One resident write stream per source class, exposed to this PE's ARB through parent wiring; 3 streams per PE instance | implemented seam |

Payload names in proposed rows are design pseudotypes until fields, widths and nominal identity are frozen. Queue transport is inferred by the compiler, not a requested public Queue wrapper. Clock/reset/time domain are execution context and must not be invented as ordinary payload ports.

## Owned or containing state

- Queue occupancy belongs to the BGF parent, not an independent WQ module

## Reference implementation

[`wq.py`](wq.py) defines `bgf_wq_system`; `CellWriteReq` and the placement
geometry are shared through
[`contracts/tmu_bgf.py`](../../contracts/tmu_bgf.py). WQ holds one queue per
client class of `CLASS_RESIDENCY_DEPTH` slots and does nothing else. Payload
travels as one record; no split, coalesce, or partial transfer
is introduced.

The FlowKey mapping, the `launch_generation` field, the placement decode, the
`bank`/`row` width rationale and the `CLASS_RESIDENCY_DEPTH` rationale are
shared with `CellReadReq` and documented in [rq.md](rq.md) and
[map.md](map.md). The containing BGF assembly remains the state owner.

## Departures from the original proposal

**WQ has no bank dimension.** Bank is a scheduling coordinate, so the whole
bank dimension belongs to [arb.md](arb.md), the leaf that resolves bank
conflicts. `bank` and `row` ride in the payload -- decoded once by
[map.md](map.md), read once by ARB -- and WQ transports them without inspecting
them. Splitting ARB's crossbar so that its input side sat in WQ would elaborate
exactly the same `(source_class, path, bank)` edges and provide neither more nor
less isolation, because the head-of-line limit sits at the fan-out input in
either arrangement.

**FlowKey no longer selects anything.** The card treats a flow as a routing
dimension, but one elaboration of `bgf_wq_system` is one PE, so the routing use
of FlowKey disappears into instance geometry. FlowKey still rides in the record
for identity and cancellation matching, which is why the mapping in
[rq.md](rq.md) still has to be frozen even though nothing in WQ reads it.

**Queue depth rides on an identity transform.** There is no requested public
Queue wrapper to instantiate and no standalone buffer operator at a queue front
end, so the only place to attach `CLASS_RESIDENCY_DEPTH` is a transform whose
body returns its input untouched. The upside is that the empty body is what
makes "WQ decides nothing" a property of the lowered IR instead of a claim in
this card.

## Physical premise

The four PE control flows in [architecture](../../ARCHITECTURE.md) are
physically independent: each PE owns a private group of cell-register banks and
its own arbitration. [bank.md](../trf/bank.md) records 32 single-port 128-byte
banks in private groups, that is **8 banks per PE**.

This seam therefore describes exactly one PE. PE identity is expressed by
instantiating `bgf_wq_system` once per PE, not by a routing dimension inside
it, and no queue, cursor, age, or fairness state is shared across PEs. The
parent assembly performs the four-way instantiation.

## Proposed queue partition (review required)

WQ is a parent-owned queue schema, but writes use the same source-class
partition discipline as reads: the partition is `source_class` alone. The
following topology is a proposal for review, not a registry-approved
implementation:

```text
one PE instance
CUBE / VEC / TLSU                      (address cells; no bank concept)
              |
              v
             MAP                       (tag source_class; decode bank/row)
              |
              v
     write_q[source_class]             (WQ; residency only, bank-blind)
              |
              v
             ARB                       (owns the bank crossbar and conflicts)
              |
              v
          XBAR/BANK                    (per PE, no cross-PE sharing)
```

- `source_class` is a closed class tag over `CUBE`, `VEC` and `TLSU`, the only
  units that access cell registers; MAP writes it before this seam and it
  remains attached through ARB.
- PE identity is instance geometry, not a partition key inside the module.
  Because each PE is elaborated separately, equal logical IDs from different PE
  flows cannot share ordering or cancellation state by construction.
- `bank` is a PE-private bank index in `[0, BANK_PARTITIONS)`, the physical
  bank within that PE. Within BGF it is a payload field precisely so that WQ
  needs no bank dimension: MAP writes it and ARB reads it.
- Per-bank independence is provided by ARB, which buffers every
  `(source_class, path, bank)` edge of its crossbar separately, so a bank
  blocked downstream holds only its own edges.
- A queue entry carries the complete `CellWriteReq` identity and effect:
  `FlowKey`, epoch/generation, `TileVersion`/`CellKey`, request ID, response
  route and write payload. MAP appends `source_class`, `bank`,
  `row` and `out_of_range`; nothing in this seam replaces source identity.

## Queue protocol and write ordering

- `write_enqueue` is accepted only when the selected class queue has capacity
  and the downstream write/acknowledgement path can retain the entry.
- Each class head is consumed only on an accepted transfer into ARB. A blocked
  ARB, XBAR or BANK acknowledgement slot retains the complete write and
  propagates backpressure to its own class only; no class can stall another,
  and no PE can stall another. Backpressure from one bank reaches a class only
  through ARB's crossbar edge for that class.
- FIFO order is required per `source_class` within a PE instance. Cross-class
  ordering is not provided by WQ; ARB owns conflict scheduling.
- WQ performs no selection between classes. It exposes one head per class; any
  round-robin or priority choice belongs to ARB, whose fixed-priority order and
  aging escalation are recorded in [arb.md](arb.md).
  That order ranks a write above the read of the same source class, but WQ
  itself does not order against RQ; ARB does.
- Writes are all-or-none at queue admission and at BANK transfer. No implicit
  byte-mask splitting, coalescing, publication, or partial acceptance is allowed
  unless a later profile explicitly defines it, and none is currently
  representable: the shared record carries no mask and one BANK entry is one whole
  cell.
- Recovery may cancel only writes not accepted by BANK/XBAR. Cancellation
  matches `FlowKey` and epoch/generation, and stale generations cannot mutate a
  reused cell.

## Required capabilities to verify

- Parent-owned parameterized Queue arrays
- Borrowed child Queue ports without state duplication
- Per-class residency for CUBE, VEC and TLSU traffic with no bank dimension
- Per-class depth, occupancy and enqueue/dequeue accounting
- Preservation of write identity, class tag and placement fields
  from the MAP handoff through ARB, unread by this seam
- Per-PE instance isolation with no shared queue or fairness state

These are requirements, not proof that the current framework is missing each one. First test the current revision; a demonstrated gap becomes a generic framework/primitive issue and regression before a dependent design PR.

## Behavioral acceptance

- FIFO order per `source_class`
- Retain TileVersion, operation generation, request ID, and response route
- Independent capacity and backpressure for each class; a blocked class does
  not stall another class, and a blocked PE instance does not stall another.
  Per-bank independence is ARB's acceptance obligation, not WQ's
- No reordering within a class
- No partial acceptance or implicit publication
- No placement field is read: the lowered IR contains no field access

gfsim execution is the first implementation gate. PYC/RTL obligations apply to the admitted lowering and remain explicit future work where provisional storage is rejected. Compile-only evidence does not establish behavior.

## Open decisions

- Freeze `CLASS_RESIDENCY_DEPTH` per source class for the selected profile;
  writes may need a different depth from reads because an entry also holds the
  write payload. The bank dimension itself is settled and belongs to ARB:
  8 PE-private banks, 32 across the four PEs, per [bank.md](../trf/bank.md).
- Confirm the one-`stid`-per-PE assumption shared with [rq.md](rq.md). If a PE
  hosts several `stid` values, add a partition inside the instance rather than
  reintroducing a cross-PE routing dimension.
- Define cancellation matching, stale-generation handling, and the handoff from
  WQ to ARB/BANK. Byte-mask semantics are not among them: access granularity is
  one whole cell and the shared record carries no mask.

## Contributor closure

- [ ] Claim the candidate and identify its parent/containing state owner.
- [ ] Resolve disposition; aliases and contained state must not duplicate hardware.
- [ ] Link the relevant NDF L0 intent and L1 behavior to this L2 implementation.
- [ ] Freeze port payload fields/widths, producer/consumer, parent seam, state/reset and timing profile.
- [ ] Freeze CUBE/VEC/TLSU queue dimensions, class tags and per-class depths.
- [ ] Prove per-class FIFO, independent backpressure, all-or-none writes and stale-generation cancellation.
- [ ] Define functional branches, all-or-none effects, contention and cancel/recovery lifecycle.
- [ ] Link a minimal failing gate for each actual framework/primitive gap and merge that shared fix first.
- [ ] Implement the accepted owner and design-local expected-result tests.
- [ ] Prove backpressure, identity/generation, exactly-once effects and isolated instances in gfsim.
- [ ] Integrate into H2/H1 and record admitted PYC/RTL evidence or remaining boundary.

## Source evidence

- `docs/architecture/core/l3/TILE_MEMORY_PACKETS.md:106` — WQ is explicitly recommended as a state schema.
- `docs/architecture/core/DAVINCIOO_MICROARCHITECTURE.md:805` — Uniform bank request lifecycle and Queue ownership.

Source paths use the frozen external repository spelling, including legacy `l3/` directories; they are provenance, not the new hierarchy vocabulary. File hashes and the original catalog disposition are in [catalog.json](../../catalog.json). See [architecture](../../ARCHITECTURE.md) for unresolved global decisions.
