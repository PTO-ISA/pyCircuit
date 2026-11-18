---
doc_id: WSEMIG-DOC-MACHINE
status: draft
authority: normative
owner: wse-calendar
---

# Shared components in the programmed communication machine

## One shared semantic implementation {#WSEMIG-MACHINE-SHARED-001}
<!-- ndf: kind=requirement modality=must refinement=L2 domain=noc status=draft refines=WSEMIG-RX-REQ-001 -->

The programmed machine MUST use the same receive ledger as the original
ReceiveEngine interface. Node-indexed accounts MUST retain independent ranges,
epochs, pending metadata and identity sets. The original wrapper MUST preserve
its sampling and adoption order. Epoch and configuration wrappers MUST call the
same state-owning cores used by the machine.

The machine is a bounded functional profile: 40 logical node banks, 64 KiB per
bank, up to 64-byte beats (8-byte fallback for short or non-64-byte-aligned transfers), one fragment in flight, one resident receive context per
node, two early metadata slots and 56 identity slots. Internal time multiplexing
MUST preserve node isolation and transaction semantics. It assigns no physical
shared-memory topology or port performance.

## Runtime program ownership {#WSEMIG-MACHINE-PROGRAM-001}
<!-- ndf: kind=requirement modality=must refinement=L2 domain=noc status=draft refines=WSEMIG-RX-REQ-001 -->

The machine MUST consume runtime instructions as specified in
[the active interface](../docs/program-interface.md). Programs own group
membership, layout, addresses, fragments and expected receive amounts. The
hardware MUST NOT derive an AllGather phase or expected command sequence from
its program counter, fixed addresses or workload names.

COPY MUST obtain remote landings through ForwardHop execution. It MUST preflight
all remote receives before payload writes. Each write beat MUST require the
fixed remote permission mask, retaining progress when any permit is absent.
The destination's last beat, identity insertion and receive-byte increment MUST
share one transaction. Earlier committed beats remain visible after a later
failure. Ordinary protocol faults permit diagnostic readback; identity-capacity
assertions retain the existing backend termination boundary.

## Independent execution evidence {#WSEMIG-MACHINE-VER-001}
<!-- ndf: kind=verification modality=must refinement=L3 domain=verification status=draft verifies=WSEMIG-MACHINE-SHARED-001,WSEMIG-MACHINE-PROGRAM-001 -->

Component regression MUST retain the original receive/control/forward/node
oracles. Full-machine evidence MUST identify the source hashes, shared final
ACIR, backend, runtime program and raw input/output. AllGather and a branching
multicast using node 33 MUST execute on the same hardware candidate. Independent
oracles remain outside DUT generation. PTO acceptance MUST retain the actual
PyPTO-to-PTOAS-to-program-to-memory-output chain.

These clauses define acceptance conditions. They do not record a completed
full-machine run. Historical metadata fixtures and their contracts remain under
`verification/fixtures/calendar/`, outside this active NDF root.
