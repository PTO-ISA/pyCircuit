# CUBE.MMA — H2 assembly

NDF refinement: **L2 microarchitecture**. Matrix pipeline and retained K-progress.

Proposed source: `designs/davincioo/cube/mma/assembly.py`. No implementation is claimed.

## Input boundary proposal

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| matrix_operands | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |
| operation_control | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |
| cancel | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |

## Output boundary proposal

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| accumulator_fragment | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |
| operation_progress | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |
| fault | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |
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

- [DAV-CUBE-MMA-KACC-0001](kacc.md)
- [DAV-CUBE-MMA-MAC-0001](mac.md)
- [DAV-CUBE-MMA-PIPE-0001](pipe.md)
- [DAV-CUBE-MMA-SYST-0001](syst.md)

[Architecture](../../ARCHITECTURE.md) · [Complete inventory](../../MODULE_CHECKLIST.md)
