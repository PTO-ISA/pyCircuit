# TMU — H1 assembly

NDF refinement: **L2 microarchitecture**. Tile descriptors, versioned ownership and raw banks.

Proposed source: `designs/davincioo/tmu/assembly.py`. No implementation is claimed.

## Input boundary proposal

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| rename_request | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |
| bank_access | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |
| publish_request | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |
| reclaim_request | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |

## Output boundary proposal

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| rename_response | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |
| bank_response | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |
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

- [DAV-TMU-BGF-ARB-0001](bgf/arb.md)
- [DAV-TMU-BGF-MAP-0001](bgf/map.md)
- [DAV-TMU-BGF-RQ-0001](bgf/rq.md)
- [DAV-TMU-BGF-WQ-0001](bgf/wq.md)
- [DAV-TMU-BGF-XBAR-0001](bgf/xbar.md)
- [DAV-TMU-TRF-ALC-0001](trf/alc.md)
- [DAV-TMU-TRF-BANK-0001](trf/bank.md)
- [DAV-TMU-TRF-CELL-0001](trf/cell.md)
- [DAV-TMU-TRF-RDY-0001](trf/rdy.md)
- [DAV-TMU-TRF-REF-0001](trf/ref.md)
- [DAV-TMU-TRN-CHK-0001](trn/chk.md)
- [DAV-TMU-TRN-FRE-0001](trn/fre.md)
- [DAV-TMU-TRN-LRM-0001](trn/lrm.md)
- [DAV-TMU-TRN-LTR-0001](trn/ltr.md)
- [DAV-TMU-TRN-RAT-0001](trn/rat.md)
- [DAV-TMU-TRN-STS-0001](trn/sts.md)
- [DAV-TMU-TUL-FLS-0001](tul/fls.md)
- [DAV-TMU-TUL-LBA-0001](tul/lba.md)
- [DAV-TMU-TUL-RCM-0001](tul/rcm.md)
- [DAV-TMU-TUL-REN-0001](tul/ren.md)
- [DAV-TMU-TUL-RET-0001](tul/ret.md)

[Architecture](../ARCHITECTURE.md) · [Complete inventory](../MODULE_CHECKLIST.md)
