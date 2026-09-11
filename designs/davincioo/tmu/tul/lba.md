# TMU.TUL.LBA — Late Binding Allocation

- Source candidate: `DAV-TMU-TUL-LBA-0001`
- Hardware hierarchy: **H3**, within H1 `TMU` / H2 `TUL`
- NDF refinement: **L2 microarchitecture**; module-specific L1 behavior links still require review.
- Recommended disposition: **alias** (proposal, not registry approval)
- Implementation placement: use the accepted containing owner or contract file under `designs/davincioo/`; no independent leaf is authorized by this inventory disposition.
- Current design-program execution status: **alias; no implementation exists and
  none should**. The owners named below are the implementation record.

Legacy scalar T/U late-binding name; selected owner is existing SPE.OOO rename state.

## What the name meant

Late binding allocation is deciding *which* physical resource a write lands in as
late as possible, rather than at the moment the instruction is named. It buys
allocation that can still be taken back when speculation resolves the wrong way.
This candidate named that policy for T/U registers.

## Where the function lives now

| Resource | Owner | How late binding appears there |
| --- | --- | --- |
| Scalar T/U | SPE.OOO rename state ([SMAP](../../spe/ooo/smap.md), [MPQ](../../spe/ooo/mpq.md)) | The speculative map holds the binding until it is committed or discarded |
| Tile storage | [TMU.TRN.FRE](../trn/fre.md) | Allocation is split in two: reserve, then commit or cancel |

[fre.md](../trn/fre.md) describes that split in the terms this card cared about:
asking for a block cannot be a one-shot action, because a branch can resolve the
wrong way, so a reservation stays undoable until the transaction that made it
resolves. A cancel returns the block as if it had never been requested.

So the *policy* this name describes is alive in TMU -- it is simply not a module.
It is the shape of FRE's operation set, and a separate late-binding owner would
have to re-decide bindings FRE already owns.

## Why TMU must not own it

[architecture](../../ARCHITECTURE.md) records that all scalar T/U state belongs to
SPE.OOO. A parallel late-binding owner would be a second answer to "which physical
resource does this write use", which is exactly the question one allocator must
answer alone; [fre.md](../trn/fre.md) records that FRE is the only module in the
design program that reserves storage, and that the candidate which looked like a
second allocator folds into it.

## If you arrived here

- Binding a **scalar** T/U register late: SPE.OOO rename state.
- Binding **tile** storage late: [FRE](../trn/fre.md)'s reserve/commit/cancel is
  the mechanism; a repeated reserve returns the block it already holds rather than
  taking a second one.
- Looking for capacity projection: [alc.md](../trf/alc.md) folds it into FRE, and
  [fre.md](../trn/fre.md) records how much of it FRE can actually answer.

## Inputs

No independent ports: this work item is contained state or a migration alias. Resolve its containing owner; do not allocate another Queue/state owner.

## Outputs

No independent ports: this work item is contained state or a migration alias. Resolve its containing owner; do not allocate another Queue/state owner.

Payload names in proposed rows are design pseudotypes until fields, widths and nominal identity are frozen. Queue transport is inferred by the compiler, not a requested public Queue wrapper. Clock/reset/time domain are execution context and must not be invented as ordinary payload ports.

## Owned or containing state

None. The speculative scalar binding belongs to SPE.OOO rename state; the tile
free list and its cancellable reservations belong to [FRE](../trn/fre.md).

## Required capabilities to verify

None. An alias executes nothing. [fre.md](../trn/fre.md) records the capabilities
late binding actually needs, including the one it does not have: an exhausted pool
reports rather than stalls.

## Behavioral acceptance

- No TMU implementation or scalar state — **holds**; no `lba.py` exists under
  `tmu/tul/`
- No parallel late-binding owner — **holds**; the two bindings above have one
  owner each, and FRE's reservation table is the only cancellable tile binding

No gfsim or PYC/RTL evidence is owed. There is nothing here to execute.

## Open decisions

- **Complete the formal old-ID migration/disposition record.**
- **Whether "late binding" should remain a design term at all.** It describes
  FRE's reserve/commit/cancel shape accurately, so retiring the ID does not retire
  the idea; if the term is kept, [fre.md](../trn/fre.md) is where it should be
  defined, so the word and the mechanism stay in one place.

## Contributor closure

- [ ] Claim the candidate and identify its parent/containing state owner.
- [x] Resolve disposition; aliases and contained state must not duplicate hardware.
      Resolved as an alias: the owners are listed under "Where the function lives now".
- [ ] Link the relevant NDF L0 intent and L1 behavior to this L2 implementation.
- [x] Freeze port payload fields/widths, producer/consumer, parent seam, state/reset and timing profile.
      The frozen answer is that there are none.
- [ ] Complete the old-ID migration record.
- [ ] Decide whether the term is defined on [fre.md](../trn/fre.md) or retired.

## Source evidence

- `docs/architecture/core/l3/TILE_MEMORY_PACKETS.md:120` — LBA migrates to SPE.OOO rename state.
- `docs/architecture/core/l3/TILE_MEMORY_PACKETS.md:30` — All scalar T/U state belongs to SPE.OOO.

Source paths use the frozen external repository spelling, including legacy `l3/` directories; they are provenance, not the new hierarchy vocabulary. File hashes and the original catalog disposition are in [catalog.json](../../catalog.json). See [architecture](../../ARCHITECTURE.md) for unresolved global decisions.
