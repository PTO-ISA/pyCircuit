# CUBE.WBK — H2 assembly

NDF refinement: **L2 microarchitecture**. Result formatting and acknowledged writeback.

Proposed source: `designs/davincioo/cube/wbk/assembly.py`. No implementation is claimed.

## Input boundary proposal

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| converted_fragment | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |
| destination_descriptor | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |
| write_ack | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |
| cancel | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |

## Output boundary proposal

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| tile_write_request | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |
| engine_resolve | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |
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

- [DAV-CUBE-WBK-FMT-0001](fmt.md)
- [DAV-CUBE-WBK-WBF-0001](wbf.md)
- [DAV-CUBE-WBK-WQ-0001](wq.md)

[Architecture](../../ARCHITECTURE.md) · [Complete inventory](../../MODULE_CHECKLIST.md)
