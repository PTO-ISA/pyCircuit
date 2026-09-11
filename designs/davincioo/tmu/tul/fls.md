# TMU.TUL.FLS — Flush

- Source candidate: `DAV-TMU-TUL-FLS-0001`
- Hardware hierarchy: **H3**, within H1 `TMU` / H2 `TUL`
- NDF refinement: **L2 microarchitecture**; module-specific L1 behavior links still require review.
- Recommended disposition: **alias** (proposal, not registry approval)
- Implementation placement: use the accepted containing owner or contract file under `designs/davincioo/`; no independent leaf is authorized by this inventory disposition.
- Current design-program execution status: **alias; no implementation exists and
  none should**. The owners named below are the implementation record.

Legacy scalar T/U recovery name; implementation ownership has moved to SPE.OOO.FLS and TMU must not acquire scalar state.

## What the name meant

A flush is what happens when speculation turns out to be wrong: the work after
some point is discarded and the machine is put back the way it was. This
candidate named that for T/U registers.

## Where the function lives now

| Step | Owner | What it does |
| --- | --- | --- |
| Scalar recovery | [SPE.OOO.FLS](../../spe/ooo/fls.md) | Discards the scalar work and restores scalar rename state |
| Where to return to | [TMU.TRN.CHK](../trn/chk.md) | Holds the captured frontier and answers how far back recovery goes |
| Undoing tile names | [TMU.TRN.RAT](../trn/rat.md) | Pops its history stack, youngest-first, one rollback at a time |
| Undoing tile storage | [TMU.TRN.FRE](../trn/fre.md) | Cancels a reservation, returning the block untouched |
| Undoing tile status | [TMU.TRN.STS](../trn/sts.md) | Deletes the speculative row, and cannot reach a published one |

A flush that touches Tile state does so as ordinary TMU transactions on those
owners. It does not do so through a TMU flush owner, and the difference is not
cosmetic: each owner re-checks the recovery against its own generation, so a
stale recovery is refused by the owner that would have been corrupted rather than
by a central authority that has to be trusted.

## Why TMU must not own it

A flush owner inside TMU would need to know which scalar work is being discarded,
which is [SPE.OOO.FLS](../../spe/ooo/fls.md)'s state, and it would have to mutate
sibling state to undo it, which the design program forbids outright:
[chk.md](../trn/chk.md) records that even CHK, which exists solely for recovery,
never writes RAT, FRE or STS. It answers with a distance and lets each owner
apply its own rollback.

## If you arrived here

- Flushing **scalar** state: [SPE.OOO.FLS](../../spe/ooo/fls.md).
- Recovering **tile** state: ask [CHK](../trn/chk.md) for the target frontier, then
  issue that many rollbacks to [RAT](../trn/rat.md); cancel outstanding
  reservations in [FRE](../trn/fre.md) and roll back speculative rows in
  [STS](../trn/sts.md).
- Note that nothing currently sequences those three. [chk.md](../trn/chk.md)
  records the missing coordinator as an open decision.

## Inputs

No independent ports: this work item is contained state or a migration alias. Resolve its containing owner; do not allocate another Queue/state owner.

## Outputs

No independent ports: this work item is contained state or a migration alias. Resolve its containing owner; do not allocate another Queue/state owner.

Payload names in proposed rows are design pseudotypes until fields, widths and nominal identity are frozen. Queue transport is inferred by the compiler, not a requested public Queue wrapper. Clock/reset/time domain are execution context and must not be invented as ordinary payload ports.

## Owned or containing state

None. Recovery points belong to [CHK](../trn/chk.md), the undo history to
[RAT](../trn/rat.md), cancellable reservations to [FRE](../trn/fre.md), and scalar
recovery state to [SPE.OOO.FLS](../../spe/ooo/fls.md).

## Required capabilities to verify

None. An alias executes nothing. [chk.md](../trn/chk.md) records the one
capability recovery actually lacks: a rule cannot fire both on a request and on
its own state, so no owner can sequence a multi-step unwind by itself.

## Behavioral acceptance

- No source under TMU — **holds**; no `fls.py` exists under `tmu/tul/`
- No duplicate scalar T/U recovery state — **holds**; the recovery state kinds are
  enumerated above with one owner each
- Disposition points contributors to SPE.OOO.FLS — **holds**; see "If you arrived
  here"

No gfsim or PYC/RTL evidence is owed. There is nothing here to execute.

## Open decisions

- **Complete the formal old-ID migration/disposition record.**
- **Whether scalar and tile recovery share one trigger.** Both are driven by the
  same mispredicted branch, but nothing records whether one flush event fans out
  to both sides or whether they are separately ordered. That belongs in a parent
  contract, not here.

## Contributor closure

- [ ] Claim the candidate and identify its parent/containing state owner.
- [x] Resolve disposition; aliases and contained state must not duplicate hardware.
      Resolved as an alias: the owners are listed under "Where the function lives now".
- [ ] Link the relevant NDF L0 intent and L1 behavior to this L2 implementation.
- [x] Freeze port payload fields/widths, producer/consumer, parent seam, state/reset and timing profile.
      The frozen answer is that there are none.
- [ ] Complete the old-ID migration record.
- [ ] Record whether one flush trigger covers scalar and tile recovery.

## Source evidence

- `docs/specification/davincioo/ndf-next/tile/tmu.md:61` — Deprecated TUL boundary and selected SPE.OOO ownership.
- `docs/architecture/core/l3/TILE_MEMORY_PACKETS.md:119` — FLS explicitly migrates to SPE.OOO.FLS.

Source paths use the frozen external repository spelling, including legacy `l3/` directories; they are provenance, not the new hierarchy vocabulary. File hashes and the original catalog disposition are in [catalog.json](../../catalog.json). See [architecture](../../ARCHITECTURE.md) for unresolved global decisions.
