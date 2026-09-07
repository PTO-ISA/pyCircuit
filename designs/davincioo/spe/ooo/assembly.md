# SPE.OOO — H2 assembly

NDF refinement: **L2 microarchitecture**. Decode, scalar rename, resident issue and micro-commit.

Proposed source: `designs/davincioo/spe/ooo/assembly.py`. No implementation is claimed.

## Input boundary proposal

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| instruction_window | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |
| execution_completion | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |
| history_ack | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |
| block_journal_ack | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |
| recovery_request | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |

## Output boundary proposal

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| execution_issue | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |
| tile_command | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |
| micro_commit | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |
| rename_request | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |
| recovery_actions | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |

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

- [DAV-SPE-OOO-CMAP-0001](cmap.md)
- [DAV-SPE-OOO-CMT-0001](cmt.md)
- [DAV-SPE-OOO-D1-0001](d1.md)
- [DAV-SPE-OOO-D2-0001](d2.md)
- [DAV-SPE-OOO-D3-0001](d3.md)
- [DAV-SPE-OOO-DSAQ-0001](dsaq.md)
- [DAV-SPE-OOO-DSP-0001](dsp.md)
- [DAV-SPE-OOO-EXC-0001](exc.md)
- [DAV-SPE-OOO-FLS-0001](fls.md)
- [DAV-SPE-OOO-IQ-0001](iq.md)
- [DAV-SPE-OOO-ISQ-0001](isq.md)
- [DAV-SPE-OOO-MPQ-0001](mpq.md)
- [DAV-SPE-OOO-PCB-0001](pcb.md)
- [DAV-SPE-OOO-R0-0001](r0.md)
- [DAV-SPE-OOO-R1-0001](r1.md)
- [DAV-SPE-OOO-R2-0001](r2.md)
- [DAV-SPE-OOO-R3-0001](r3.md)
- [DAV-SPE-OOO-R4-0001](r4.md)
- [DAV-SPE-OOO-REN-0001](ren.md)
- [DAV-SPE-OOO-ROB-0001](rob.md)
- [DAV-SPE-OOO-S1-0001](s1.md)
- [DAV-SPE-OOO-S3-0001](s3.md)
- [DAV-SPE-OOO-SMAP-0001](smap.md)
- [DAV-SPE-OOO-SRF-0001](srf.md)

[Architecture](../../ARCHITECTURE.md) · [Complete inventory](../../MODULE_CHECKLIST.md)
