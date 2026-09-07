# CUBE.DFL — H2 assembly

NDF refinement: **L2 microarchitecture**. Operand feed, buffering and layout adaptation.

Proposed source: `designs/davincioo/cube/dfl/assembly.py`. No implementation is claimed.

## Input boundary proposal

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| operand_feed_request | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |
| tile_read_response | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |
| group_feed | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |
| cancel | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |

## Output boundary proposal

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| tile_read_request | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |
| matrix_operands | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |
| feed_fault | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |
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

- [DAV-CUBE-DFL-ABUF-0001](abuf.md)
- [DAV-CUBE-DFL-BBUF-0001](bbuf.md)
- [DAV-CUBE-DFL-CELL-0001](cell.md)
- [DAV-CUBE-DFL-CSCL-0001](cscl.md)
- [DAV-CUBE-DFL-LAY-0001](lay.md)
- [DAV-CUBE-DFL-RDB-0001](rdb.md)
- [DAV-CUBE-DFL-XPOS-0001](xpos.md)

[Architecture](../../ARCHITECTURE.md) · [Complete inventory](../../MODULE_CHECKLIST.md)
