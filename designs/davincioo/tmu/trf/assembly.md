# TMU.TRF — H2 assembly

NDF refinement: **L2 microarchitecture**. Raw 128-byte cell storage and access.

Proposed source: `designs/davincioo/tmu/trf/assembly.py`. No implementation is claimed.

## Input boundary proposal

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| bank_read | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |
| bank_write | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |
| cell_allocate | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |
| cell_release | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |

## Output boundary proposal

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| bank_read_data | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |
| bank_write_ack | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |
| cell_grant | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |
| cell_release_ack | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |

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

- [DAV-TMU-TRF-ALC-0001](alc.md)
- [DAV-TMU-TRF-BANK-0001](bank.md)
- [DAV-TMU-TRF-CELL-0001](cell.md)
- [DAV-TMU-TRF-RDY-0001](rdy.md)
- [DAV-TMU-TRF-REF-0001](ref.md)

[Architecture](../../ARCHITECTURE.md) · [Complete inventory](../../MODULE_CHECKLIST.md)
