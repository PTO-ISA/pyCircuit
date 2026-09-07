# MEM.COH — H2 assembly

NDF refinement: **L2 microarchitecture**. Profile-defined coherence and maintenance.

Proposed source: `designs/davincioo/mem/coh/assembly.py`. No implementation is claimed.

## Input boundary proposal

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| snoop | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |
| invalidate | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |
| flush | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |
| writeback_response | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |

## Output boundary proposal

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| snoop_response | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |
| writeback_request | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |
| maintenance_ack | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |

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

- [DAV-MEM-COH-FLU-0001](flu.md)
- [DAV-MEM-COH-INV-0001](inv.md)
- [DAV-MEM-COH-SNP-0001](snp.md)
- [DAV-MEM-COH-WB-0001](wb.md)

[Architecture](../../ARCHITECTURE.md) · [Complete inventory](../../MODULE_CHECKLIST.md)
