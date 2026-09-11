# TMU.TUL.RCM — Recommit

- Source candidate: `DAV-TMU-TUL-RCM-0001`
- Hardware hierarchy: **H3**, within H1 `TMU` / H2 `TUL`
- NDF refinement: **L2 microarchitecture**; module-specific L1 behavior links still require review.
- Recommended disposition: **alias** (proposal, not registry approval)
- Implementation placement: use the accepted containing owner or contract file under `designs/davincioo/`; no independent leaf is authorized by this inventory disposition.
- Current design-program execution status: **alias; no implementation exists and
  none should**. The owners named below are the implementation record.

Legacy scalar T/U recommit name; selected owner is SPE.OOO commit state and BROB publication path.

## What the name meant

Recommit is the second half of a two-phase commit: work is micro-committed first,
and later made architecturally visible. The two phases exist because a block of
work can be internally complete long before it is allowed to be seen. This
candidate named that for T/U registers.

## Where the function lives now

| Phase | Owner | What becomes true |
| --- | --- | --- |
| Scalar micro-commit | [SPE.OOO.CMT](../../spe/ooo/cmt.md) | The instruction's result is committed within the block |
| Block publication | [BROB](../../spe/bctrl/brob.md) | The block becomes architecturally visible |
| Tile version publication | [TMU.TRN.STS](../trn/sts.md) | This version's contents and descriptor are final |
| Tile name publication | [TMU.TRN.RAT](../trn/rat.md) | This name now means this version, architecturally |

The last two rows are one publication split across two owners, and the split is
deliberate. [sts.md](../trn/sts.md) states it: RAT publishes a *name*, STS
publishes a *version*, a coordinator does both, and either alone is incomplete.
That is the same two-phase shape this candidate named, expressed as ownership
rather than as a phase counter.

## Why TMU must not own it

[architecture](../../ARCHITECTURE.md) records that ROB and BROB have different
micro-commit and block-publication duties. A TMU recommit owner would be a third
authority over when work becomes visible, and it would need its own view of block
completion, which is [BROB](../../spe/bctrl/brob.md)'s. Publication inside TMU is
already two acknowledged transactions with one owner each; adding a phase owner
above them would not make the sequence atomic, because one rule activation still
writes one row.

## If you arrived here

- Committing **scalar** work: [SPE.OOO.CMT](../../spe/ooo/cmt.md), then
  [BROB](../../spe/bctrl/brob.md) for publication.
- Publishing a **tile**: publish the version in [STS](../trn/sts.md) and the name
  in [RAT](../trn/rat.md). STS validates descriptor compatibility before anything
  visible changes, and RAT's publish is generation-qualified so it cannot promote
  a newer version by mistake.
- Publishing to **several destinations at once**: [sts.md](../trn/sts.md) records
  that this has no transaction yet, and why.

## Inputs

No independent ports: this work item is contained state or a migration alias. Resolve its containing owner; do not allocate another Queue/state owner.

## Outputs

No independent ports: this work item is contained state or a migration alias. Resolve its containing owner; do not allocate another Queue/state owner.

Payload names in proposed rows are design pseudotypes until fields, widths and nominal identity are frozen. Queue transport is inferred by the compiler, not a requested public Queue wrapper. Clock/reset/time domain are execution context and must not be invented as ordinary payload ports.

## Owned or containing state

None. Scalar commit state belongs to [SPE.OOO.CMT](../../spe/ooo/cmt.md), block
publication to [BROB](../../spe/bctrl/brob.md), version status to
[STS](../trn/sts.md), and the architectural version of a name to
[RAT](../trn/rat.md).

## Required capabilities to verify

None. An alias executes nothing. The capability that a real two-phase publication
would need -- one transaction spanning several destinations -- is recorded as an
open decision on [sts.md](../trn/sts.md), where the storage that would have to
support it lives.

## Behavioral acceptance

- No TMU implementation or scalar state — **holds**; no `rcm.py` exists under
  `tmu/tul/`
- Microcommit/architectural publication phases remain with SPE/BROB owners —
  **holds**; the phases are enumerated above with one owner each, and TMU's two
  publications are about tiles rather than about scalar phases

No gfsim or PYC/RTL evidence is owed. There is nothing here to execute.

## Open decisions

- **Complete the formal old-ID migration/disposition record.**
- **Who sequences the tile pair.** Publishing a version and publishing its name
  are two transactions, and nothing currently orders them; the coordinator is
  [LTR](../trn/ltr.md), which is not implemented. Until then, "published" is
  ambiguous for the interval between the two acknowledgements, and a reader must
  say which one it means.

## Contributor closure

- [ ] Claim the candidate and identify its parent/containing state owner.
- [x] Resolve disposition; aliases and contained state must not duplicate hardware.
      Resolved as an alias: the owners are listed under "Where the function lives now".
- [ ] Link the relevant NDF L0 intent and L1 behavior to this L2 implementation.
- [x] Freeze port payload fields/widths, producer/consumer, parent seam, state/reset and timing profile.
      The frozen answer is that there are none.
- [ ] Complete the old-ID migration record.
- [ ] Record which owner sequences version publication and name publication.

## Source evidence

- `docs/architecture/core/l3/TILE_MEMORY_PACKETS.md:121` — RCM migrates to SPE.OOO commit state.

Source paths use the frozen external repository spelling, including legacy `l3/` directories; they are provenance, not the new hierarchy vocabulary. File hashes and the original catalog disposition are in [catalog.json](../../catalog.json). See [architecture](../../ARCHITECTURE.md) for unresolved global decisions.
