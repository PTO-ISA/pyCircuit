# SPE.BCTRL — H2 assembly

NDF refinement: **L2 microarchitecture**. Block admission, ordered Tile rename, resolve and retirement.

Proposed source: `designs/davincioo/spe/bctrl/assembly.py`. No implementation is claimed.

## Input boundary proposal

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| tile_command | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |
| micro_commit | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |
| engine_resolve | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |
| publication_ack | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |
| recovery_request | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |

## Output boundary proposal

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| engine_issue | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |
| tile_rename_request | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |
| architectural_publication | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |
| recovery_actions | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |

## Composition contract

- Parent owns each child-to-child seam; child state has one owner.
- Preserve FlowKey, epoch/generation, payload and distinct completion meanings.
- Freeze shared-resource arbitration and request/response/cancel transactions.
- A composition is not one whole-core atomic rule; retain multi-cycle protocol state explicitly.
- Exact payload fields, widths, multiplicities, depths and timing are open until reviewed.

## Checklist

- [ ] Link the implementing NDF L2 detail to L1 behavior and L0 intent.
- [ ] Accept child/contained-state dispositions and instance geometry.
- [ ] Freeze external ports and a producer-consumer-seam table.
- [ ] Implement parameter propagation and specialization reuse.
- [ ] Run composition tests for independent backpressure, cancellation, isolation and quiescence.
- [ ] Record gfsim results and admitted PYC/RTL boundary.

## Included H3 candidates

- [DAV-SPE-BCTRL-BFU-0001](bfu.md)
- [DAV-SPE-BCTRL-BIFU-0001](bifu.md)
- [DAV-SPE-BCTRL-BIQ-0001](biq.md)
- [DAV-SPE-BCTRL-BISQ-0001](bisq.md)
- [DAV-SPE-BCTRL-BMDB-0001](bmdb.md)
- [DAV-SPE-BCTRL-BROB-0001](brob.md)
- [DAV-SPE-BCTRL-CBRG-0001](cbrg.md)
- [DAV-SPE-BCTRL-F5-0001](f5.md)

[Architecture](../../ARCHITECTURE.md) · [Complete inventory](../../MODULE_CHECKLIST.md)
