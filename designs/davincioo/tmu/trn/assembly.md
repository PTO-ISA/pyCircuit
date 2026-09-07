# TMU.TRN — H2 assembly

NDF refinement: **L2 microarchitecture**. Descriptor rename, map, status and publication ownership.

Proposed source: `designs/davincioo/tmu/trn/assembly.py`. No implementation is claimed.

## Input boundary proposal

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| tile_rename | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |
| descriptor_query | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |
| publish_request | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |
| reclaim_request | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |

## Output boundary proposal

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| rename_grant | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |
| descriptor_response | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |
| publish_ack | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |
| reclaim_ack | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |

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

- [DAV-TMU-TRN-CHK-0001](chk.md)
- [DAV-TMU-TRN-FRE-0001](fre.md)
- [DAV-TMU-TRN-LRM-0001](lrm.md)
- [DAV-TMU-TRN-LTR-0001](ltr.md)
- [DAV-TMU-TRN-RAT-0001](rat.md)
- [DAV-TMU-TRN-STS-0001](sts.md)

[Architecture](../../ARCHITECTURE.md) · [Complete inventory](../../MODULE_CHECKLIST.md)
