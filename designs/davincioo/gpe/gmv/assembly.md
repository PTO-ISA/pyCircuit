# GPE.GMV — H2 assembly

NDF refinement: **L2 microarchitecture**. Group operand movement and data readiness.

Proposed source: `designs/davincioo/gpe/gmv/assembly.py`. No implementation is claimed.

## Input boundary proposal

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| movement_request | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |
| participant_data | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |
| cancel | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |

## Output boundary proposal

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| movement_response | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |
| data_ready | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |
| cancel_ack | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |

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

- [DAV-GPE-GMV-DRDY-0001](drdy.md)
- [DAV-GPE-GMV-GBUF-0001](gbuf.md)
- [DAV-GPE-GMV-REQ-0001](req.md)
- [DAV-GPE-GMV-RSP-0001](rsp.md)

[Architecture](../../ARCHITECTURE.md) · [Complete inventory](../../MODULE_CHECKLIST.md)
