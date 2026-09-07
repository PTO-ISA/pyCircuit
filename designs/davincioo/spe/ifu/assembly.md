# SPE.IFU — H2 assembly

NDF refinement: **L2 microarchitecture**. Fetch, prediction, byte joins and instruction buffering.

Proposed source: `designs/davincioo/spe/ifu/assembly.py`. No implementation is claimed.

## Input boundary proposal

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| fetch_grant | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |
| redirect | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |
| translation_response | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |
| instruction_response | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |
| predictor_update | TBD typed transaction | Assembly boundary proposal; reconcile exact producer and owner. | proposed |

## Output boundary proposal

| Name | Payload/type | Meaning | Evidence status |
| --- | --- | --- | --- |
| instruction_window | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |
| translation_request | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |
| instruction_request | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |
| fetch_fault | TBD typed transaction | Assembly boundary proposal; freeze completion and backpressure. | proposed |

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

- [DAV-SPE-IFU-BF0-0001](bf0.md)
- [DAV-SPE-IFU-BF1-0001](bf1.md)
- [DAV-SPE-IFU-BF2-0001](bf2.md)
- [DAV-SPE-IFU-BF3-0001](bf3.md)
- [DAV-SPE-IFU-BF4-0001](bf4.md)
- [DAV-SPE-IFU-BIM-0001](bim.md)
- [DAV-SPE-IFU-BPFIFO-0001](bpfifo.md)
- [DAV-SPE-IFU-BPU-0001](bpu.md)
- [DAV-SPE-IFU-BRQ-0001](brq.md)
- [DAV-SPE-IFU-CBTB-0001](cbtb.md)
- [DAV-SPE-IFU-F0-0001](f0.md)
- [DAV-SPE-IFU-F1-0001](f1.md)
- [DAV-SPE-IFU-F2-0001](f2.md)
- [DAV-SPE-IFU-F3-0001](f3.md)
- [DAV-SPE-IFU-F4-0001](f4.md)
- [DAV-SPE-IFU-FQ-0001](fq.md)
- [DAV-SPE-IFU-GHR-0001](ghr.md)
- [DAV-SPE-IFU-GHRQ-0001](ghrq.md)
- [DAV-SPE-IFU-IB-0001](ib.md)
- [DAV-SPE-IFU-IBTB-0001](ibtb.md)
- [DAV-SPE-IFU-IDATA-0001](idata.md)
- [DAV-SPE-IFU-IMMQ-0001](immq.md)
- [DAV-SPE-IFU-ITAG-0001](itag.md)
- [DAV-SPE-IFU-ITLB-0001](itlb.md)
- [DAV-SPE-IFU-LBUF-0001](lbuf.md)
- [DAV-SPE-IFU-LOOP-0001](loop.md)
- [DAV-SPE-IFU-MBTB-0001](mbtb.md)
- [DAV-SPE-IFU-NLP-0001](nlp.md)
- [DAV-SPE-IFU-PBTB-0001](pbtb.md)
- [DAV-SPE-IFU-RAFE-0001](rafe.md)
- [DAV-SPE-IFU-RAHQ-0001](rahq.md)
- [DAV-SPE-IFU-RAS-0001](ras.md)
- [DAV-SPE-IFU-SC-0001](sc.md)
- [DAV-SPE-IFU-TAGE-0001](tage.md)
- [DAV-SPE-IFU-TGTFIFO-0001](tgtfifo.md)
- [DAV-SPE-IFU-UBTB-0001](ubtb.md)
- [DAV-SPE-IFU-UCACHE-0001](ucache.md)
- [DAV-SPE-IFU-UTLB-0001](utlb.md)

[Architecture](../../ARCHITECTURE.md) · [Complete inventory](../../MODULE_CHECKLIST.md)
