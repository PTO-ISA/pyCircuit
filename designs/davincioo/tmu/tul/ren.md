# TMU.TUL.REN — Rename

- Source candidate: `DAV-TMU-TUL-REN-0001`
- Hardware hierarchy: **H3**, within H1 `TMU` / H2 `TUL`
- NDF refinement: **L2 microarchitecture**; module-specific L1 behavior links still require review.
- Recommended disposition: **alias** (proposal, not registry approval)
- Implementation placement: use the accepted containing owner or contract file under `designs/davincioo/`; no independent leaf is authorized by this inventory disposition.
- Current design-program execution status: **alias; no implementation exists and
  none should**. The owners named below are the implementation record.

Normative candidate is deprecated migration provenance; scalar T/U rename belongs to SPE.OOO.REN/MPQ and is not a TMU leaf.

## What the name meant

Rename is the step that turns an architectural register name into a physical one,
so that several in-flight writers of the same name do not collide. The T/U
registers were once expected to be renamed inside TMU, and this candidate is what
that expectation left behind.

## Where the function lives now

| Resource | Owner | What it does |
| --- | --- | --- |
| Scalar T/U registers | [SPE.OOO.REN](../../spe/ooo/ren.md) with [MPQ](../../spe/ooo/mpq.md) | The rename itself, and the map it updates |
| Tile logical names | [TMU.TRN.RAT](../trn/rat.md) | Maps a logical tile name to a physical version, and keeps the history that undoes it |
| Tile physical storage | [TMU.TRN.FRE](../trn/fre.md) | Hands out the block a renamed version writes into |

The second and third rows are not this candidate under a different name. A tile
version and a scalar register are different resources with different lifetimes;
[RAT](../trn/rat.md) exists because tiles need their own map, not because scalar
rename moved into TMU. Both run under one flow, in parallel.

## Why TMU must not own it

[architecture](../../ARCHITECTURE.md) records the rule: scalar T/U ownership
belongs to SPE.OOO, and TUL names are migration work items rather than permission
to create a second rename owner. A rename owner is a map plus a free list plus a
recovery history, and every one of those already has exactly one owner. A second
one would not add capability; it would add a second answer to "what does this
name mean right now", which is the one question a rename map exists to answer
unambiguously.

## If you arrived here

- Renaming a **scalar** T/U register: go to [SPE.OOO.REN](../../spe/ooo/ren.md).
- Renaming a **tile**: go to [RAT](../trn/rat.md) for the name and
  [FRE](../trn/fre.md) for the storage; [rat.md](../trn/rat.md) records how a swap
  and its history entry commit together.
- Adding a source file here: do not. `designs/davincioo/AGENTS.md` forbids empty
  modules created to make the inventory look implemented, and a forwarding module
  would add a transport hop and a second name for one owner.

## Inputs

No independent ports: this work item is contained state or a migration alias. Resolve its containing owner; do not allocate another Queue/state owner.

## Outputs

No independent ports: this work item is contained state or a migration alias. Resolve its containing owner; do not allocate another Queue/state owner.

Payload names in proposed rows are design pseudotypes until fields, widths and nominal identity are frozen. Queue transport is inferred by the compiler, not a requested public Queue wrapper. Clock/reset/time domain are execution context and must not be invented as ordinary payload ports.

## Owned or containing state

None, and that is the disposition rather than an omission. Any state this card
appeared to need is already owned: the scalar map by
[SPE.OOO.REN](../../spe/ooo/ren.md)/[MPQ](../../spe/ooo/mpq.md), the tile map and
its speculative history by [RAT](../trn/rat.md), and the physical free list by
[FRE](../trn/fre.md).

## Required capabilities to verify

None. An alias executes nothing, so it can demand nothing of the framework. The
capability obligations belong to the owners above; [rat.md](../trn/rat.md) and
[fre.md](../trn/fre.md) record theirs, including the gaps they hit.

## Behavioral acceptance

- No TMU source registration — **holds**; no `ren.py` exists under `tmu/tul/`, and
  this is checkable by absence rather than by test
- No duplicate scalar map/free/history state — **holds**; the three state kinds
  are enumerated above with one owner each
- Migration preserves audit linkage only — **holds**; this card retains the old
  candidate ID and its source evidence so the name stays traceable

No gfsim or PYC/RTL evidence is owed. There is nothing here to execute.

## Open decisions

- **Close `DAV-OQ-OOO-0001` and record the final alias/deprecation disposition.**
  Until it is closed, this card is the only place the deprecation is written down,
  which is why it is retained rather than deleted.
- **Whether the candidate ID is retired or kept as vocabulary.** Nothing in the
  implementation depends on it; the argument for keeping it is that "TMU rename"
  is still said out loud and a reader needs somewhere to land.

## Contributor closure

- [ ] Claim the candidate and identify its parent/containing state owner.
- [x] Resolve disposition; aliases and contained state must not duplicate hardware.
      Resolved as an alias: the owners are listed under "Where the function lives now".
- [ ] Link the relevant NDF L0 intent and L1 behavior to this L2 implementation.
- [x] Freeze port payload fields/widths, producer/consumer, parent seam, state/reset and timing profile.
      The frozen answer is that there are none.
- [ ] Close `DAV-OQ-OOO-0001` and decide whether the candidate ID is retired.
- [ ] Confirm the owner cards state the tile/scalar distinction consistently.

## Source evidence

- `docs/specification/davincioo/ndf-next/tile/tmu.md:61` — Deprecated boundary; SPE.OOO REN/MPQ/CMT/FLS and BROB are selected owners.

Source paths use the frozen external repository spelling, including legacy `l3/` directories; they are provenance, not the new hierarchy vocabulary. File hashes and the original catalog disposition are in [catalog.json](../../catalog.json). See [architecture](../../ARCHITECTURE.md) for unresolved global decisions.
