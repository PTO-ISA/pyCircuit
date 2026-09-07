# SPE.IEX — H2 assembly

NDF refinement: **L2 microarchitecture**. Scalar operand capture, execution and writeback.

Proposed source: `designs/davincioo/spe/iex/assembly.py`. No implementation is claimed.

## Input boundary proposal

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| issue_request | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |
| operand_response | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |
| dependency_update | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |
| write_ack | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |
| cancel_request | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |

## Output boundary proposal

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| operand_request | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |
| execution_result | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |
| selected_effects | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |
| apply_ack | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |
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

- [DAV-SPE-IEX-AGU-0001](agu.md)
- [DAV-SPE-IEX-ALU-0001](alu.md)
- [DAV-SPE-IEX-BRU-0001](bru.md)
- [DAV-SPE-IEX-CMDIQ-0001](cmdiq.md)
- [DAV-SPE-IEX-CMDP-0001](cmdp.md)
- [DAV-SPE-IEX-DIV-0001](div.md)
- [DAV-SPE-IEX-E1-0001](e1.md)
- [DAV-SPE-IEX-E2-0001](e2.md)
- [DAV-SPE-IEX-E3-0001](e3.md)
- [DAV-SPE-IEX-E4-0001](e4.md)
- [DAV-SPE-IEX-E5-0001](e5.md)
- [DAV-SPE-IEX-E6-0001](e6.md)
- [DAV-SPE-IEX-FSU-0001](fsu.md)
- [DAV-SPE-IEX-GPR-0001](gpr.md)
- [DAV-SPE-IEX-I1-0001](i1.md)
- [DAV-SPE-IEX-I2-0001](i2.md)
- [DAV-SPE-IEX-P0-0001](p0.md)
- [DAV-SPE-IEX-P1-0001](p1.md)
- [DAV-SPE-IEX-S2-0001](s2.md)
- [DAV-SPE-IEX-STD-0001](std.md)
- [DAV-SPE-IEX-W1-0001](w1.md)
- [DAV-SPE-IEX-W2-0001](w2.md)
- [DAV-SPE-IEX-W3-0001](w3.md)
- [DAV-SPE-IEX-WBA-0001](wba.md)

[Architecture](../../ARCHITECTURE.md) · [Complete inventory](../../MODULE_CHECKLIST.md)
