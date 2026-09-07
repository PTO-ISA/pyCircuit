# VEC.SFU — H2 assembly

NDF refinement: **L2 microarchitecture**. Profile-defined nonlinear arithmetic.

Proposed source: `designs/davincioo/vec/sfu/assembly.py`. No implementation is claimed.

## Input boundary proposal

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| numeric_request | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |
| cancel | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |

## Output boundary proposal

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| numeric_result | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |
| numeric_fault | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |
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

- [DAV-VEC-SFU-DIV-0001](div.md)
- [DAV-VEC-SFU-EXP-0001](exp.md)
- [DAV-VEC-SFU-LOG-0001](log.md)
- [DAV-VEC-SFU-RECIP-0001](recip.md)
- [DAV-VEC-SFU-RSQRT-0001](rsqrt.md)
- [DAV-VEC-SFU-SQRT-0001](sqrt.md)

[Architecture](../../ARCHITECTURE.md) · [Complete inventory](../../MODULE_CHECKLIST.md)
