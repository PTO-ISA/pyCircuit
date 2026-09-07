# SMT.THR — H2 assembly

NDF refinement: **L2 microarchitecture**. Per-flow launch, PC and lifecycle ownership.

Proposed source: `designs/davincioo/smt/thr/assembly.py`. No implementation is claimed.

## Input boundary proposal

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| launch_stop | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |
| redirect | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |
| flow_completion | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |
| fetch_progress | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |

## Output boundary proposal

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| flow_state | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |
| runnable_flow | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |
| fetch_context | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |
| launch_stop_ack | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |

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

- [DAV-SMT-THR-CTX-0001](ctx.md)
- [DAV-SMT-THR-MODE-0001](mode.md)
- [DAV-SMT-THR-SWI-0001](swi.md)
- [DAV-SMT-THR-TID-0001](tid.md)
- [DAV-SMT-THR-TPC-0001](tpc.md)
- [DAV-SMT-THR-TSB-0001](tsb.md)

[Architecture](../../ARCHITECTURE.md) · [Complete inventory](../../MODULE_CHECKLIST.md)
