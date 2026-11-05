---
doc_id: WSEMIG-DOC-RX
status: draft
authority: normative
owner: wse-calendar
---

# Stateful ingress migration

## Preserve receive accounting {#WSEMIG-RX-REQ-001}
<!-- ndf: kind=requirement modality=must refinement=L1 domain=noc status=draft refines=WSEMIG-INTENT-001 -->

The finite ingress MUST preserve `CoreIngress` and `ReceiveAccount` accounting
for admitted histories. Its test profile has one resident receive context and
explicit early-arrival and identity capacities. Backpressure MUST preserve the
request for retry. These capacities are test configuration, not WSE defaults.
Identity-capacity exhaustion MUST instead terminate with a model-capacity
diagnostic, as accepted in decision 0007; it is not recoverable backpressure.

## Resident state and admission {#WSEMIG-RX-001}
<!-- ndf: kind=requirement modality=must refinement=L2 domain=noc status=draft refines=WSEMIG-RX-REQ-001 -->

The engine MUST retain `{opcode, epoch, dst, capacity, expected, counted}`, a
`{source, seq}` identity set and early arrivals across clock edges. A posted
context adopts its matching early arrivals one per cycle, selecting the lowest
occupied matching slot in this test profile and blocking new external commands
until adoption finishes. This slot ordering is not an architecture ordering
requirement. Other epochs remain buffered. Only committed,
non-self, previously unseen bytes inside the destination range count. Faults are
terminal until reset. Completed contexts are explicitly retired before reuse.
Counter overflow MUST reject the segment and preserve the previous count and
identity records before latching the protocol fault (decision 0008).

## Finite identity capacity {#WSEMIG-RX-CAPACITY-001}
<!-- ndf: kind=requirement modality=must refinement=L2 domain=noc status=draft refines=WSEMIG-RX-REQ-001 -->

A new countable identity with no free slot MUST fail the enclosing transaction
with `WSE_MODEL_CAPACITY: dedup identity slots exhausted` before publication.
No pending removal, identity replacement or byte increment may commit. Existing
duplicate, self-sourced and uncommitted paths MUST retain their core behavior.

## Isolated capacity check {#WSEMIG-RX-CAPACITY-VER-001}
<!-- ndf: kind=verification modality=must refinement=L3 domain=verification status=draft verifies=WSEMIG-RX-CAPACITY-001 -->

The isolated CapacityProbe MUST complete three cycles with capacity available.
With capacity available for only the first two cycles, it MUST fail at sampling
epoch 4 and publish only the first four observations. This tests the shared
assertion and publication boundary, not the complete ReceiveEngine scan logic.
Host encoding tests MUST reject sequences outside 0..65535 without wrapping;
that 16-bit carrier remains a test boundary, not a hardware specification.

## Stateful trace {#WSEMIG-RX-VER-001}
<!-- ndf: kind=verification modality=must refinement=L3 domain=verification status=draft verifies=WSEMIG-RX-REQ-001,WSEMIG-RX-001 -->

The checker MUST run the same twelve-cycle history on C++ and Verilog with
8-bit epochs, one context, two early slots and two identity slots. It MUST check
early adoption, uncommitted then committed redelivery, two sources sharing one
sequence number, duplicate suppression, self exclusion, retirement and identity
reuse in the next epoch. Both runs passed on pinned pyCircuit `6f199cdb7bd9`;
the execution receipts are under `.wse_model_out/loop-fix-pinned-receive/`.

## External receive histories {#WSEMIG-RX-EXTERNAL-VER-001}
<!-- ndf: kind=verification modality=must refinement=L3 domain=verification status=draft verifies=WSEMIG-RX-REQ-001,WSEMIG-RX-001,WSEMIG-RX-CAPACITY-001 -->

`tools/verify_receive.py` MUST compare every public Result field at both samples
of every cycle against the ordinary Python ReceiveAccount, with a test-only
adapter for the stated one-context capacity and adoption timing. External JSON
inputs are compiled into a stimulus-only System; expected results stay on the
host. C++ and Verilog MUST consume the same linked artifact. Input integers MUST
be checked without truncation, including source 0..39 and sequence 0..65535.

Histories MUST cover early-buffer saturation and retry, independent opcode and
epoch keys, identity exclusion priority, retirement/reuse, invalid commands,
range errors on direct and buffered arrivals, wide address/count arithmetic,
and identity exhaustion on direct and buffered arrivals. Capacity failures MUST
retain the exact published prefix and fail at the expected sampling epoch.
The public interface exposes no post-abort snapshot of internal pending/identity
storage; that preservation is checked in the reference and source transaction
structure, while backend checks establish the failure and publication boundary.

The 21-case suite passed with 184 observations per backend, using explicit early
capacities of 2 or 3 and identity capacities of 1, 2 or 3. Evidence:
`.wse_model_out/receive-boundaries-final-02/verification.json`. The external host
checker in `host_checks.py` is bound to this clause. The clause remains draft,
as does the project's formal NDF acceptance gate.
