# TMU.BGF.MAP — Mapping

- Source candidate: `DAV-TMU-BGF-MAP-0001`
- Hardware hierarchy: **H3**, within H1 `TMU` / H2 `BGF`
- NDF refinement: **L2 microarchitecture**; module-specific L1 behavior links still require review.
- Recommended disposition: **leaf** (proposal, not registry approval)
- Proposed implementation path, if accepted as an independent leaf: `designs/davincioo/tmu/bgf/map.py`
- Current design-program execution status: **source implementation present;
  gfsim verification pending**. The implementation is the BGF entry decode
  stage; residency and bank state remain outside this leaf.

MAP turns a client's `cell_key` into the physical `(bank, row)` pair that names
one cell, and changes nothing else about the request.

## What problem it solves

A client -- CUBE, VEC or TLSU -- asks for a cell by `cell_key` alone. It does not
know, and must not know, which bank holds that cell or which row inside the
bank. MAP is the stage that turns the name into a place.

MAP is the BGF entry stage and the only owner of that decision: no other BGF
module recomputes bank or row. This is what keeps bank geometry private to BGF
-- a geometry change touches this leaf and the shared contract, not CUBE, VEC or
TLSU.

## Where it sits

Inside BGF a cell access passes through a fixed chain: client, then MAP to
decode placement, then [RQ](rq.md)/[WQ](wq.md) to queue by class, then
[ARB](arb.md) to fan out and resolve conflicts, then [XBAR](xbar.md), then
[BANK](../trf/bank.md) where the bytes actually move. MAP is the first link:

| Module | Owns | Does not own |
| --- | --- | --- |
| **MAP** | **Decoding `cell_key` into `(bank, row)`** | **Queueing, arbitration, or the data** |
| [RQ](rq.md) / [WQ](wq.md) | Read and write queues, one per source class | Payload contents, any decision, or a bank dimension |
| [ARB](arb.md) | The bank crossbar, the conflict decision, and fairness/aging | Where a cell lives; it only reads what MAP wrote |
| [XBAR](xbar.md) | Carrying an accepted grant to its target bank | Which contender won |
| [BANK](../trf/bank.md) | The data bytes themselves | Who owns them, or who may access them |

`bank` therefore has exactly one writer and one reader. [rq.md](rq.md) and
[wq.md](wq.md) between them hold the record without inspecting it, and
[arb.md](arb.md) consumes `bank` to select a conflict tree. That is why it is a
payload field rather than a queue dimension: it has to survive the queueing
stage to reach its single consumer.

On the TRN/TRF side the rest of a Tile's lifetime has its own owners:
[FRE](../trn/fre.md) decides which physical block a version holds,
[REF](../trf/ref.md) counts how many readers still hold a version, and
[STS](../trn/sts.md) owns the descriptor and publication status. MAP consults
none of them. It answers one arithmetic question about a `cell_key` and nothing
else.

## How one decode completes

Six client lanes arrive, one per `(source_class, path)` pair: CUBE, VEC and
TLSU, read and write. Each lane keeps its own queue, so a blocked read lane
cannot stall a write lane or another class.

**Tag.** Each lane stamps its own `source_class` onto the request. A lane is a
compile-time position, so a request can never appear under another class's tag.

**Decode.** `bank = cell_key & BANK_INDEX_MASK` and
`row = cell_key >> BANK_INDEX_BITS`. The low-order interleave is its own inverse
split, so one `cell_key` names exactly one `(bank, row)` pair.

**Check the row.** `out_of_range` is set exactly when `row >= ROWS_PER_BANK`. It
is a report, not an action: nothing is dropped, faulted, or rerouted.

**Pass on.** Everything else in the record travels unchanged to
[rq.md](rq.md)/[wq.md](wq.md). Only the four placement fields are written.

The whole decode is combinational arithmetic. The one registered stage per lane
is the optional pipeline this card allows.

## Inputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| cube_read / vec_read / tlsu_read | CellReadReq | Generation-qualified read offered by one source class of one PE, carrying `cell_key` and identity but no placement | implemented seam |
| cube_write / vec_write / tlsu_write | CellWriteReq | Generation-qualified whole-cell write of one source class of one PE, on the same terms | implemented seam |

## Outputs

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| mapped read lanes, one per source class | CellReadReq | Same record with `source_class`, `bank`, `row` and `out_of_range` written; consumed by [rq.md](rq.md) | implemented seam |
| mapped write lanes, one per source class | CellWriteReq | Same record on the write path; consumed by [wq.md](wq.md) | implemented seam |

`bank` and `row` are `u16` because a field update requires exact width equality and the frontend exposes no narrowing or cast primitive, so both must match the width of a `cell_key` expression even though they need 3 and 13 bits. Mapping epoch is not a field: no owner has been resolved for it, and inventing one here would duplicate state.

Payload names in proposed rows are design pseudotypes until fields, widths and nominal identity are frozen. Queue transport is inferred by the compiler, not a requested public Queue wrapper. Clock/reset/time domain are execution context and must not be invented as ordinary payload ports.

## Owned or containing state

- Static scope base/range tables -- **not elaborated**; the reference
  implementation decodes one flat interleaved space and consults no scope table
- Static bank_groups, banks_per_group, rows_per_bank, mapping_mode -- **partly**;
  the geometry constants are frozen in the shared contract and imported from it,
  but `mapping_mode` is not elaborated and low-order interleave is the only decode
- Optional one-row pipeline per lane -- **elaborated**

MAP holds no mutable state. The decode is combinational, so nothing here is a Table
or a persistent variable.

## Reference implementation

[`map.py`](map.py) defines `bgf_map_system`, elaborated once per PE. It carries
the six client lanes and, per lane, tags `source_class`, decodes `bank` and
`row` from `cell_key`, and reports the row bounds result. The payload records
and the decode geometry are shared through
[`contracts/tmu_bgf.py`](../../contracts/tmu_bgf.py), because MAP writes those
fields and [arb.py](arb.py) reads them.

The decoder is pure: it appends placement fields and never replaces source
identity, drops a request, or reorders a lane. Because `bank` is a mask over
`BANK_INDEX_MASK` it cannot leave `[0, BANK_PARTITIONS)`, and the consumers
reapply that mask in their route keys, so the bound stays structural rather than
resting on this contract alone.

## Departures from the original proposal

Two things differ from what this card first proposed.

**Decode rides the lane instead of being a request/response service.** The
proposed `cell_map_req`/`cell_map_resp` pair would require the request payload
to wait somewhere while the decode result returned, which means a second queue
structure and a rejoin inside BGF. Since the arithmetic is combinational, the
seam instead carries it on the transaction itself; the one registered stage per
lane is the optional pipeline this card allows.

**The scope base/range tables and `mapping_mode` are not elaborated.** Those
geometries are not frozen, and the lookup they need is not established: a static
tuple indexed by a runtime field does not lower, and a field update admits no
narrowing, so a table result would have to match the source expression width.
This profile therefore decodes one mode.

## Decode profile

**This profile decodes one mode.** `bank = cell_key & BANK_INDEX_MASK` and
`row = cell_key >> BANK_INDEX_BITS`, a low-order interleave. The scope
base/range tables, `mapping_mode` and the Local/Shared geometries are not frozen
and are therefore not elaborated. `BANK_PARTITIONS` must stay a power of two:
`apply` and `route` expressions provide `&` and `>>` but neither `%` nor `//`.

**The row bound belongs to TRF.** `ROWS_PER_BANK` is imported from the TRF
contract, where [bank.md](../trf/bank.md) freezes it at 256. MAP must not declare
its own bound: BANK masks every row index to that range, so an independently
declared bound would let MAP report a row as in-range that BANK then folds onto a
different cell. The bound stays well below the `cell_key` range, so
`out_of_range` remains reachable rather than constantly false.
`out_of_range` is reported and never acted on -- the owner of the
rejection policy is an open decision below.

## Required capabilities to verify

- Static/JIT parameterized mapping arithmetic — **available** for the shift and
  mask profile; `%` and `//` are not, which is why the geometry must be a power
  of two.
- Bounded array lookup and exact-width index checks — **not established**. The
  scope base/range table is unimplemented for this reason; a static tuple
  indexed by a runtime field does not lower, and field updates admit no
  narrowing, so a table result must match the source expression width.
- Optional parameterized pipeline lanes — **available**; the elaborated profile
  uses one registered stage per lane.

These are requirements, not proof that the current framework is missing each one. First test the current revision; a demonstrated gap becomes a generic framework/primitive issue and regression before a dependent design PR.

## Behavioral acceptance

- Consume only when the decoded lane has capacity; a blocked lane retains the
  request and back-pressures its own source class only
- The elaborated profile is a deterministic bijection: `cell_key` maps to
  exactly one `(bank, row)` pair, and the low-order interleave is its own
  inverse split
- Every lane tags its own `source_class`, and a request never appears on
  another class's lane
- `bank` always lands in `[0, BANK_PARTITIONS)`, so the consuming route
  selector cannot go out of range
- `out_of_range` is set exactly when `row >= ROWS_PER_BANK`, and
  setting it neither drops the request nor changes any other field
- Mapping preserves complete source identity under backpressure: only the four
  placement fields are written
- Reject Local PE cross-ownership and Local/Shared aliasing — **not
  elaborated**; the scope tables are unfrozen and the lookup capability is
  unestablished

gfsim execution is the first implementation gate. PYC/RTL obligations apply to the admitted lowering and remain explicit future work where provisional storage is rejected. Compile-only evidence does not establish behavior.

## Open decisions

- Freeze physical cell-index width and selected Local/Shared bank geometries.
- `rows_per_bank` is frozen at 256 by [bank.md](../trf/bank.md) and imported
  from the TRF contract, so MAP reports `out_of_range` against the same bound
  BANK enforces.
- Assign the owner of the `out_of_range` rejection policy. MAP reports the
  result and does not drop, fault, or reroute the request; no consumer acts on
  it yet.
- Decide whether a mapping epoch is required and, if so, which module owns it.
- Decide whether Local/Shared scope selection belongs in this decode once those
  geometries are frozen; the framework gap above must close first.

## Contributor closure

- [ ] Claim the candidate and identify its parent/containing state owner.
- [ ] Resolve disposition; aliases and contained state must not duplicate hardware.
- [ ] Link the relevant NDF L0 intent and L1 behavior to this L2 implementation.
- [ ] Freeze port payload fields/widths, producer/consumer, parent seam, state/reset and timing profile.
- [ ] Define functional branches, all-or-none effects, contention and cancel/recovery lifecycle.
- [ ] Link a minimal failing gate for each actual framework/primitive gap and merge that shared fix first.
- [ ] Implement the accepted owner and design-local expected-result tests.
- [ ] Prove backpressure, identity/generation, exactly-once effects and isolated instances in gfsim.
- [ ] Integrate into H2/H1 and record admitted PYC/RTL evidence or remaining boundary.

## Source evidence

- `docs/architecture/core/l3/TILE_MEMORY_PACKETS.md:301` — Detailed MAP packet.
- `docs/architecture/core/DAVINCIOO_MICROARCHITECTURE.md:771` — Baseline bank/row mapping and DSE constraints.

Source paths use the frozen external repository spelling, including legacy `l3/` directories; they are provenance, not the new hierarchy vocabulary. File hashes and the original catalog disposition are in [catalog.json](../../catalog.json). See [architecture](../../ARCHITECTURE.md) for unresolved global decisions.
