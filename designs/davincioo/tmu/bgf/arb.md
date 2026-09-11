# TMU.BGF.ARB — Arbiter

- Source candidate: `DAV-TMU-BGF-ARB-0001`
- Hardware hierarchy: **H3**, within H1 `TMU` / H2 `BGF`
- NDF refinement: **L2 microarchitecture**; module-specific L1 behavior links still require review.
- Recommended disposition: **leaf** (proposal, not registry approval)
- Proposed implementation path, if accepted as an independent leaf: `designs/davincioo/tmu/bgf/arb.py`
- Current design-program execution status: **source implementation present;
  gfsim verification pending**. The implementation is a parent-composed
  arbitration seam; RQ/WQ residency remains outside this leaf, while the bank
  crossbar and the conflict decision are both owned here.

ARB decides which one of the competing CUBE, VEC and TLSU accesses gets a bank
of a PE this cycle, and delivers the winner to that bank.

## What problem it solves

Each of a PE's eight cell-register banks is single-port: it can serve one
access per cycle. But CUBE, VEC and TLSU all read and write cells, so six
streams can want the same bank in the same cycle. Which one goes through this
tick? That is ARB.

ARB owns both the bank crossbar and the conflict decision, because they are one
thing rather than two: choosing a winner for a bank and steering that winner to
the bank cannot be separated without giving the bank dimension two owners. Bank
is a scheduling coordinate, so it exists as a payload field upstream and as a
structure only here.

## Where it sits

Inside BGF a cell access passes through a fixed chain: client, then
[MAP](map.md) to decode placement, then [RQ](rq.md)/[WQ](wq.md) to queue by
class, then ARB to fan out and resolve conflicts, then [XBAR](xbar.md), then
[BANK](../trf/bank.md) where the bytes actually move. ARB owns exactly one link
of that chain:

| Module | Owns | Does not own |
| --- | --- | --- |
| [MAP](map.md) | Decoding `cell_key` into `(bank, row)` | Queueing, arbitration, or the data |
| [RQ](rq.md) / [WQ](wq.md) | Read and write queues, one per source class | Payload contents, any decision, or a bank dimension |
| **ARB** | **The bank crossbar, the conflict decision, and fairness/aging** | **Holding requests; the queues belong to RQ/WQ** |
| [XBAR](xbar.md) | Carrying an accepted grant to its target bank | Which contender won |
| [BANK](../trf/bank.md) | The data bytes themselves | Who owns them, or who may access them |

On the TRN/TRF side a Tile's other properties have their own owners:
[FRE](../trn/fre.md) decides which physical block a version holds,
[REF](../trf/ref.md) counts how many readers still hold a version, and
[STS](../trn/sts.md) owns the descriptor and publication status. ARB sees none
of that. By the time a request reaches it, placement is already decided and the
only remaining question is who goes first.

## How one grant is produced

Six streams enter one PE's ARB -- CUBE, VEC and TLSU, read and write, held by
[rq.md](rq.md) and [wq.md](wq.md). None of them carries a bank structure; each
request carries the `bank` field that [map.md](map.md) decoded, and nothing
more.

**Fan out.** Each stream is routed across `BANK_PARTITIONS` on that field,
giving one buffered edge per `(source_class, path, bank)`. A request has now
chosen its bank tree and left the class stream behind.

**Merge.** Eight independent bank trees each merge the six edges of their own
bank with a priority merge in the baseline order (`cube_w`, `cube_r`, `vec_w`,
`vec_r`, `tlsu_w`, `tlsu_r`) and take one of them. Because a bank is
single-port, at most one access per bank is accepted per cycle.

**Publish.** The selected transaction is annotated with the accepted bank and
forked into the client and XBAR publication paths, so both consumers observe
the same accepted grant. A separate cancellation acknowledgement path preserves
the generation-qualified cancellation payload.

The per-bank edges are what make banks independent: a bank blocked downstream
holds only its own edges while other banks keep draining. That independence
stops at the fan-out input, where a stream whose head targets a full bank
blocks that stream's later requests. This head-of-line limit is inherent to
fanning one stream out -- `ac.route` advances one head per cycle and stalls when
the selected output is full -- and is unchanged by which leaf the fan-out is
drawn in.

## Inputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| cube_read / vec_read / tlsu_read | BankGrantReq | Mapped read stream of one source class of one PE, as exposed by RQ; carries the decoded `bank` but no bank structure | proposed |
| cube_write / vec_write / tlsu_write | BankGrantReq | Mapped write stream of one source class of one PE, as exposed by WQ; carries the decoded `bank` but no bank structure | proposed |
| grant_cancel | GrantCancelReq | Cancel an unaccepted generation-qualified grant | proposed |

## Outputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| client_grants[] | BankGrant | The accepted grant returned to the client, elaborated as one port per bank rather than one per class -- see "Departures" | proposed |
| grant_to_xbar | BankGrant | The same accepted grant handed to XBAR, likewise one port per bank | proposed |
| grant_cancel_ack | GrantCancelAck | Cancellation result | proposed |

Payload names in proposed rows are design pseudotypes until fields, widths and nominal identity are frozen. Queue transport is inferred by the compiler, not a requested public Queue wrapper. Clock/reset/time domain are execution context and must not be invented as ordinary payload ports.

## Owned or containing state

All state below is per PE instance; nothing is shared across PEs. **None of it is
elaborated yet.** The reference implementation carries the baseline fixed priority
order only, which is combinational and holds nothing, so this list is the target
that the grant policy below requires rather than a description of what exists:

- Per-bank, per-source-class age counters, used to drive escalation to
  `granted` -- **not elaborated**
- One committed round-robin cursor per bank, resolving contenders of equal
  rank and advanced only on an accepted transfer -- **not elaborated**
- One retained grant register per bank -- **not elaborated**
- Escalation threshold configuration -- **not elaborated**

The fixed order needs no per-class deficit counter; fairness inside a rank comes
from the cursor and fairness across ranks comes from escalation. Do not add
deficit counters as unowned state.

## Reference implementation

[`arb.py`](arb.py) defines the common `BankGrant`/`BankGrantReq` payload and
`bgf_arb_system`, elaborated once per PE. It takes the six bank-blind class
streams of one PE and owns the whole bank dimension itself.

The source uses one common request/grant record so read and write streams enter
the same conflict scheduler without dropping identity or payload fields.
`BANK_PARTITIONS = 8` is the PE-private bank count, shared through
[`contracts/tmu_bgf.py`](../../contracts/tmu_bgf.py).

`bank` is a payload field, written by [map.md](map.md) and read only here.
`granted_bank` stays separate and is written as a compile-time constant, so a
grant records which tree accepted it rather than which tree it requested. The
route key re-masks `bank` with `BANK_INDEX_MASK`, which keeps
`route_selector_out_of_range` structurally unreachable instead of dependent on
an upstream promise; that mask is not a second placement decision, since the
value still comes from MAP. `row` is the placement coordinate that MAP computes
and BANK consumes. `GrantCancel` carries neither field, because `cell_key`
locates the target and the selector is derivable from it.

## Departures from the original proposal

Two things differ from what this card first proposed.

**The crossbar is not split between RQ/WQ and ARB.** ARB owns the bank crossbar
because ARB owns the conflict decision. Bank is a scheduling coordinate and
nothing upstream needs it, so RQ and WQ hold one queue per class and carry
`bank` in the payload untouched. Fanning out in RQ/WQ and merging here would
elaborate exactly the same edges, since the fan-out queues and this leaf's input
ports are the same queues under two names; it would only widen the RQ/WQ-to-ARB
seam from 6 ports to 48 and give the bank dimension two owners.

**Grants leave on one port per bank, not one per class.** A grant is issued *by a
bank*: the eight bank trees arbitrate independently, so the accepted transaction is
already identified with the bank that took it. Merging the eight results back into
one per-class port would re-introduce exactly the coupling those separate trees
exist to remove, because a blocked consumer on one bank would then stall a grant
from another. So `client_grants[]` and `grant_to_xbar` each elaborate as eight
ports. The card's port names stay as the catalog proposes them, and both remain
`proposed` rather than frozen.

**The elaborated logic is the baseline priority order only.** Two policy tiers
of the grant policy below -- aging escalation to `granted`, and the committed
round-robin cursor over the escalated contender set -- need state this leaf does
not yet own, so they remain explicitly pending the stateful profile decisions
listed under "Open decisions". `escalated` and `accepted` are already carried in
the seam, so the later stateful owner can add the policy without changing the
transaction ABI.

## Physical premise

The four PE control flows in [architecture](../../ARCHITECTURE.md) are
physically independent: each PE owns a private group of cell-register banks,
and **arbitration is per PE**. [bank.md](../trf/bank.md) records 32 single-port
128-byte banks in private groups, that is 8 banks per PE.

ARB is therefore elaborated once per PE. No age, cursor, deficit counter, or
retained grant is shared across PEs, and no PE competes with another for a
bank. A cross-PE fairness policy would be a different design and is not part of
this profile.

Within one PE instance, **"requester" means a source class** -- `CUBE`, `VEC`
or `TLSU` -- as tagged by `source_class` in [rq.md](rq.md) and [wq.md](wq.md).
It does not mean a PE, and it is not the `requester` field of
`CellReadReq`/`CellWriteReq`, whose meaning is still unresolved in the FlowKey
mapping. These three are the only units that access cell registers, so
contention is between them for one of this PE's 8 single-port banks.

## Grant policy

Arbitration for one bank of one PE is a fixed priority order with an aging
escalation tier. For one bank, the candidates in a cycle are that bank's
crossbar edges, one per source class on each path:

```text
one PE instance
  RQ: read_q[cube|vec|tlsu]     WQ: write_q[cube|vec|tlsu]   (bank-blind)
                 \                        /
                  route on the decoded `bank` field          (ARB crossbar)
                 /                        \
bank b of PE p
  read  candidates: edges (cube_b, vec_b, tlsu_b)
  write candidates: edges (cube_b, vec_b, tlsu_b)
  -> at most one accepted access per cycle (single-port bank)
```

The selected order, highest first:

| Rank | Contender | Note |
| --- | --- | --- |
| 1 | `cube_w` | CUBE write |
| 2 | `cube_r` | CUBE read |
| 3 | `granted` | any escalated request, see aging below |
| 4 | `vec_w` | VEC write |
| 5 | `vec_r` | VEC read |
| 6 | `tlsu_w` | TLSU write |
| 7 | `tlsu_r` | TLSU read |

Two properties follow from the order and are intentional: writes outrank reads
of the same class, and class rank dominates the read/write distinction, so
`cube_r` outranks `vec_w`.

### Round-robin within a rank

Contenders at the same rank are resolved by a round-robin cursor, not by class
order or by age. The cursor is committed: it advances only after an accepted
transfer, so a grant that XBAR has not accepted does not move it, and the same
contender is re-offered. This matches the `round_robin` merge policy in the
ACIR surface.

Because each `(source_class, path, bank)` crossbar edge presents exactly one
head, the ranks in the table above hold one contender each, and `granted` is
the only rank that can hold several at once. The cursor is therefore one per
bank over the escalated contender set.

### Aging escalation

A CUBE/VEC/TLSU request that has waited on its crossbar edge beyond the aging
threshold without being served has its priority escalated to `granted`, which
sits directly below `cube_r` and above `vec_w`. This is the anti-starvation
mechanism: without it, sustained CUBE traffic would starve `tlsu_r`
indefinitely under a plain fixed priority.

Escalated requests are equal peers: several of them contend under the
round-robin cursor above, so no escalated class can be starved by another
escalated class.

Escalation has no observable effect on CUBE, because `cube_w` and `cube_r`
already rank above `granted`; escalating them would lower their priority. The
mechanism is therefore effective for VEC and TLSU. Whether CUBE requests
maintain an age counter at all is an open decision below.

### Not yet decided

The order above covers every contender, since CUBE, VEC and TLSU are the only
units that access cell registers. The following remain open:

- **The aging threshold.** One shared value or per-class values, and the unit
  it is measured in.
- **Escalation lifetime.** Whether `granted` persists until the request is
  served, and when the age counter resets.
- **Whether CUBE maintains an age counter**, given escalation cannot raise its
  priority.
- **Preemption of a retained grant.** Whether a higher-ranked contender, or a
  newly escalated one, may displace a retained grant that XBAR has not yet
  accepted, or whether a retained grant is final until accepted or cancelled.
- **Per-bank issue width.** Whether any profile grants more than one access per
  bank per cycle; the baseline is one, because [bank.md](../trf/bank.md)
  records single-port banks.

## Required capabilities to verify

- Atomic consume plus two-output publish
- Payload-keyed fan-out of a class stream across the bank dimension, with
  independent buffering per `(source_class, path, bank)` edge
- Explicit conflict scheduling across source classes and single-port banks
- Fixed-priority selection over the six per-bank contenders
- Per-class age counters with threshold comparison and escalation
- Committed round-robin cursor over same-rank contenders
- Parameterized Queue arrays
- Reg/Array state with accepted-transfer cursor updates
- Per-PE instance isolation with no shared fairness state

These are requirements, not proof that the current framework is missing each one. First test the current revision; a demonstrated gap becomes a generic framework/primitive issue and regression before a dependent design PR.

## Behavioral acceptance

- At most one accepted access per single-port bank per cycle
- Banks of the same PE are granted independently; a blocked bank holds only its
  own crossbar edges and does not stall the grant of another bank
- A stream whose head targets a blocked bank stalls that stream's later
  requests; this head-of-line effect is expected and belongs to the fan-out,
  not to a defect in bank independence
- A blocked or quiescent PE instance does not affect another PE
- Blocked grant outputs retain Queue head, payload, and fairness ownership
- With no escalated contender present, the grant follows the stated order
  exactly: `cube_w` > `cube_r` > `vec_w` > `vec_r` > `tlsu_w` > `tlsu_r`
- A write outranks a read of the same source class
- A VEC or TLSU request waiting beyond the aging threshold escalates to
  `granted` and is then granted ahead of any non-escalated VEC or TLSU
  contender
- Several escalated contenders rotate under the round-robin cursor; none is
  granted twice while another escalated contender waits
- The cursor advances only on an accepted transfer: a grant that XBAR does not
  accept leaves the cursor unchanged and re-offers the same contender
- No source class starves on either path under sustained higher-priority
  traffic; a `tlsu_r` contender under continuous CUBE traffic is granted within
  a bounded time via escalation
- Selection is deterministic: the same contender set in the same age and cursor
  state grants the same contender every time
- Recovery cancels only work not accepted by XBAR

gfsim execution is the first implementation gate. PYC/RTL obligations apply to the admitted lowering and remain explicit future work where provisional storage is rejected. Compile-only evidence does not establish behavior.

## Open decisions

- **`payload` is one word wide and no longer matches BANK.**
  [bank.md](../trf/bank.md) now stores one whole 128-byte cell per entry and
  carries it as sixteen `u64` fields, so `BankGrant`, `CellReadReq` and
  `CellWriteReq` are the remaining narrow point on the path. Widening them is
  mechanical, since the fields are plain scalars, but it touches MAP, RQ, WQ, ARB
  and their tests together, so it is recorded here rather than done piecemeal.

- **Freeze the aging threshold**: one shared value or per-class values, and its
  unit.
- **Freeze escalation lifetime**: whether `granted` persists until service and
  when the age counter resets.
- Decide whether CUBE maintains an age counter, given escalation cannot raise
  its priority above `cube_r`.
- Decide whether a retained grant is preemptible before XBAR accepts it,
  including by a newly escalated contender.
- Freeze the per-bank issue width; the baseline is one access per cycle because
  the banks are single-port. The bank count is settled at 8 per PE, and the
  requester set is the three source classes.
- Decide whether the fan-out head-of-line limit needs mitigation. A stream is
  blocked by its own head, so a class cannot use a free bank while its head
  waits on a busy one. Real head-of-queue arbitration -- inspecting several
  resident requests and selecting a conflict-free set -- cannot be expressed in
  the current queue front end: `merge` does not inspect payloads, `route` is
  one-to-many, and a rule may not operate on queue topology. Mitigation
  therefore requires either a framework capability or a design workaround, and
  neither is proposed here.
- Confirm that per-PE elaboration is the accepted instance geometry, and record
  it in the BGF assembly alongside RQ/WQ.

## Contributor closure

- [ ] Claim the candidate and identify its parent/containing state owner.
- [ ] Resolve disposition; aliases and contained state must not duplicate hardware.
- [ ] Link the relevant NDF L0 intent and L1 behavior to this L2 implementation.
- [ ] Freeze port payload fields/widths, producer/consumer, parent seam, state/reset and timing profile.
- [ ] Freeze the aging threshold and escalation lifetime.
- [ ] Prove the stated order holds with no escalated contender, and that escalation is the only way it is overridden.
- [ ] Prove the round-robin cursor rotates among escalated contenders and advances only on accepted transfers.
- [ ] Prove no source class starves on either path under sustained contention, including `tlsu_r` under continuous CUBE traffic.
- [ ] Define functional branches, all-or-none effects, contention and cancel/recovery lifecycle.
- [ ] Link a minimal failing gate for each actual framework/primitive gap and merge that shared fix first.
- [ ] Implement the accepted owner and design-local expected-result tests.
- [ ] Prove backpressure, identity/generation, exactly-once effects and isolated instances in gfsim.
- [ ] Integrate into H2/H1 and record admitted PYC/RTL evidence or remaining boundary.

## Source evidence

- `docs/architecture/core/l3/TILE_MEMORY_PACKETS.md:322` — Detailed ARB purpose, state, ports, atomic grant rule, and backpressure.
- `docs/architecture/core/DAVINCIOO_MICROARCHITECTURE.md:744` — Selected Local topology uses single-port banks and aging arbitration.
- `docs/architecture/core/DAVINCIOO_MICROARCHITECTURE.md:738` (via [bank.md](../trf/bank.md)) — 32 single-port 128-byte banks; the first profile uses private groups, giving 8 banks per PE.
- [architecture](../../ARCHITECTURE.md) — four independent PE control flows qualified by `FlowKey(core_id, pe_id, stid, launch_generation)`.

Source paths use the frozen external repository spelling, including legacy `l3/` directories; they are provenance, not the new hierarchy vocabulary. File hashes and the original catalog disposition are in [catalog.json](../../catalog.json). See [architecture](../../ARCHITECTURE.md) for unresolved global decisions.
