# TMU.TUL — H2 assembly

NDF refinement: **L2 microarchitecture**. Migration names mapped into SPE.OOO.

No source is proposed. Unlike [BGF](../bgf/assembly.md), [TRF](../trf/assembly.md)
and [TRN](../trn/assembly.md), this assembly has no hardware to compose: every
child under it is a migration alias whose owner lives elsewhere. Writing
`assembly.py` here would create the second owner the dispositions exist to
prevent.

## What this assembly is

TUL is a **name space, not a pipeline stage**. When scalar T/U ownership moved to
SPE.OOO, the old names kept appearing in packets, issues and review comments, so
each one was given a card that says where its function actually went. That is the
whole purpose of this H2: a contributor who arrives holding the word "rename",
"flush", "retirement", "recommit" or "late binding allocation" lands somewhere
that tells them which owner to talk to.

[architecture](../../ARCHITECTURE.md) states the constraint directly: scalar T/U
ownership belongs to SPE.OOO, and TMU.TUL names are migration work items, not
permission to create a second rename or retirement owner.

## Where each name went

| Name | Scalar owner | Tile counterpart in TMU |
| --- | --- | --- |
| [REN](ren.md) | [SPE.OOO.REN](../../spe/ooo/ren.md), [MPQ](../../spe/ooo/mpq.md) | [RAT](../trn/rat.md) maps names to versions; [FRE](../trn/fre.md) hands out storage |
| [LBA](lba.md) | SPE.OOO rename state ([SMAP](../../spe/ooo/smap.md), [MPQ](../../spe/ooo/mpq.md)) | [FRE](../trn/fre.md) splits allocation into reserve and resolve |
| [RCM](rcm.md) | [SPE.OOO.CMT](../../spe/ooo/cmt.md), [BROB](../../spe/bctrl/brob.md) | [STS](../trn/sts.md) publishes a version; [RAT](../trn/rat.md) publishes a name |
| [FLS](fls.md) | [SPE.OOO.FLS](../../spe/ooo/fls.md) | [CHK](../trn/chk.md) and [RAT](../trn/rat.md) rollback; [FRE](../trn/fre.md) cancel |
| [RET](ret.md) | SPE.OOO retirement, [BROB](../../spe/bctrl/brob.md) | [REF](../trf/ref.md) releases physical reads; [RAT](../trn/rat.md)/[STS](../trn/sts.md) finalize lifetime |

The right column is not a rename of the left one. A Tile version and a scalar
register are different resources with different lifetimes, and the two columns
run in parallel under one flow rather than one implementing the other. That
distinction is why each card states its counterpart instead of forwarding to it.

## Input boundary proposal

No independent ports. The TUL boundary is empty by disposition, not by omission:
each child names an owner that already has its own boundary. A port added here
would have to duplicate one that exists in SPE.OOO or in TMU's own TRN/TRF
assemblies.

## Output boundary proposal

No independent ports, for the same reason.

## Composition contract

- Parent owns each child-to-child seam; child state has one owner. Here there are
  no seams, because there are no children with state.
- A migration alias must not acquire ports, state or a source file. That is the
  one composition rule this assembly actually enforces.
- Preserve FlowKey, epoch/generation, payload and distinct completion meanings in
  the owners the children point to.

## Checklist

- [ ] Link the implementing NDF L2 detail to L1 behavior and L0 intent.
- [x] Accept child/contained-state dispositions and instance geometry. Every child
      is an alias with no instance and no state; each card records its owner.
- [x] Freeze external ports and a producer-consumer-seam table. The frozen answer
      is that there are none.
- [ ] Close the remaining old-ID migration records listed on the child cards,
      including `DAV-OQ-OOO-0001` on [ren.md](ren.md).
- [ ] Confirm whether the H2 itself should be retired once every child's migration
      record is closed, or retained as vocabulary.

No gfsim or PYC/RTL evidence is owed by this assembly. There is nothing here to
execute; the evidence obligations belong to the owners in the table above.

## Included H3 candidates

- [DAV-TMU-TUL-FLS-0001](fls.md)
- [DAV-TMU-TUL-LBA-0001](lba.md)
- [DAV-TMU-TUL-RCM-0001](rcm.md)
- [DAV-TMU-TUL-REN-0001](ren.md)
- [DAV-TMU-TUL-RET-0001](ret.md)

[Architecture](../../ARCHITECTURE.md) · [Complete inventory](../../MODULE_CHECKLIST.md)
