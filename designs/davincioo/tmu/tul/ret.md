# TMU.TUL.RET — Retirement

- Source candidate: `DAV-TMU-TUL-RET-0001`
- Hardware hierarchy: **H3**, within H1 `TMU` / H2 `TUL`
- NDF refinement: **L2 microarchitecture**; module-specific L1 behavior links still require review.
- Recommended disposition: **alias** (proposal, not registry approval)
- Implementation placement: use the accepted containing owner or contract file under `designs/davincioo/`; no independent leaf is authorized by this inventory disposition.
- Current design-program execution status: **alias; no implementation exists and
  none should**. The owners named below are the implementation record.

Legacy scalar T/U retirement candidate; scalar retirement belongs to SPE.OOO/BROB, while Tile publication uses TMU STS/RAT acknowledgements.

## What the name meant

Retirement is where a speculatively executed instruction becomes irrevocable and
its resources are released. This candidate named that step for T/U registers.

It carries the design program's sharpest ownership distinction, which is why the
card is worth keeping even though nothing is built here: **releasing a physical
read and finalizing an architectural lifetime are two different events**. A
version can have no readers left and still be the architectural meaning of a
name; a name can be superseded while readers are still draining. Collapsing the
two is the failure this card exists to prevent.

## Where the function lives now

| Event | Owner | What "done" means there |
| --- | --- | --- |
| Scalar retirement | SPE.OOO retirement with [BROB](../../spe/bctrl/brob.md) | The instruction is architecturally complete |
| Physical read release | [TMU.TRF.REF](../trf/ref.md) | Nobody is reading this version any more; it becomes a reclaim *candidate*, which is not permission to free it |
| Version finalization | [TMU.TRN.STS](../trn/sts.md) | This version's contents and descriptor are final |
| Name finalization | [TMU.TRN.RAT](../trn/rat.md) | This name now means this version, architecturally |
| Physical release | [TMU.TRN.FRE](../trn/fre.md) | The block is free, and only after both preconditions are presented |

[ref.md](../trf/ref.md) states the first half of the rule -- reaching zero
references makes a version a reclaim candidate and nothing more -- and
[fre.py](../trn/fre.py) enforces the second half: a free is refused unless the
requester presents both logical permission and a zero reference count, because
FRE can observe neither on its own.

## Why TMU must not own it

A TMU scalar retirement leaf would have to decide when an instruction is
architecturally complete, which is [BROB](../../spe/bctrl/brob.md)'s duty, and it
would need its own view of readers and lifetimes, both of which already have
owners. [architecture](../../ARCHITECTURE.md) also records that ROB and BROB have
different micro-commit and block-publication duties; a third retirement authority
inside TMU would make that distinction harder to state, not easier.

## If you arrived here

- Retiring a **scalar** instruction: SPE.OOO retirement and
  [BROB](../../spe/bctrl/brob.md).
- Finishing with a **tile version**: release the read in [REF](../trf/ref.md), then
  free the block through [FRE](../trn/fre.md) with both preconditions.
- Making a tile version **architectural**: that is publication, not retirement;
  see [rcm.md](rcm.md).

## Inputs

No independent ports: this work item is contained state or a migration alias. Resolve its containing owner; do not allocate another Queue/state owner.

## Outputs

No independent ports: this work item is contained state or a migration alias. Resolve its containing owner; do not allocate another Queue/state owner.

Payload names in proposed rows are design pseudotypes until fields, widths and nominal identity are frozen. Queue transport is inferred by the compiler, not a requested public Queue wrapper. Clock/reset/time domain are execution context and must not be invented as ordinary payload ports.

## Owned or containing state

None. The reference ledger belongs to [REF](../trf/ref.md), the free list to
[FRE](../trn/fre.md), publication status to [STS](../trn/sts.md), and the name's
architectural version to [RAT](../trn/rat.md).

## Required capabilities to verify

None. An alias executes nothing. The owners above record their own capability
requirements, including the ones they cannot yet meet.

## Behavioral acceptance

- No TMU scalar retirement leaf — **holds**; no `ret.py` exists under `tmu/tul/`
- Tile physical release and logical lifetime remain distinct from scalar
  retirement — **holds, and is enforced rather than asserted**; a free in
  [FRE](../trn/fre.md) requires both `logical_permission` and `refs_zero`, so a
  zero reference count alone cannot release a block

No gfsim or PYC/RTL evidence is owed. There is nothing here to execute.

## Open decisions

- **Complete the formal old-ID migration/disposition record.**
- **Who presents the two free preconditions.** FRE checks them and REF supplies
  one of them, but the coordinator that holds both and issues the free is
  [LTR](../trn/ltr.md), which is not implemented. Until it is, the retirement
  story is complete in its parts and unfinished as a sequence.

## Contributor closure

- [ ] Claim the candidate and identify its parent/containing state owner.
- [x] Resolve disposition; aliases and contained state must not duplicate hardware.
      Resolved as an alias: the owners are listed under "Where the function lives now".
- [ ] Link the relevant NDF L0 intent and L1 behavior to this L2 implementation.
- [x] Freeze port payload fields/widths, producer/consumer, parent seam, state/reset and timing profile.
      The frozen answer is that there are none.
- [ ] Complete the old-ID migration record.
- [ ] Name the coordinator that issues a tile free, and record it on [ltr.md](../trn/ltr.md).

## Source evidence

- `docs/architecture/core/l3/TILE_MEMORY_PACKETS.md:123` — RET migrates to SPE.OOO retirement.
- `docs/specification/davincioo/ndf-next/tile/tmu.md:27` — Physical read-reference release is distinct from architectural source lifetime/finalization.

Source paths use the frozen external repository spelling, including legacy `l3/` directories; they are provenance, not the new hierarchy vocabulary. File hashes and the original catalog disposition are in [catalog.json](../../catalog.json). See [architecture](../../ARCHITECTURE.md) for unresolved global decisions.
