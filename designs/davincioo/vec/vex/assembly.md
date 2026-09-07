# VEC.VEX — H2 assembly

NDF refinement: **L2 microarchitecture**. Operand buffers, elementwise/reduction execution and writeback.

Proposed source: `designs/davincioo/vec/vex/assembly.py`. No implementation is claimed.

## Input boundary proposal

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| operation_issue | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |
| operand_data | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |
| write_ack | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |
| cancel | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |

## Output boundary proposal

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| operand_request | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |
| staged_result | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |
| resolve | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |
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

- [DAV-VEC-VEX-ACC-0001](acc.md)
- [DAV-VEC-VEX-ALU-0001](alu.md)
- [DAV-VEC-VEX-DBUF-0001](dbuf.md)
- [DAV-VEC-VEX-EXP-0001](exp.md)
- [DAV-VEC-VEX-RED-0001](red.md)
- [DAV-VEC-VEX-SBUF-0001](sbuf.md)
- [DAV-VEC-VEX-WB-0001](wb.md)

[Architecture](../../ARCHITECTURE.md) · [Complete inventory](../../MODULE_CHECKLIST.md)
