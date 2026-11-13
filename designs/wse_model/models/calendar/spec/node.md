---
doc_id: WSEMIG-DOC-NODE
status: draft
authority: normative
owner: wse-calendar
---

# Integrated node functional flow

## Connected configuration and communication {#WSEMIG-NODE-001}
<!-- ndf: kind=requirement modality=must refinement=L2 domain=noc status=draft refines=WSEMIG-RX-REQ-001 -->

The node fixture MUST feed the actual EpochBank result into receive requests,
the actual decoded expected byte count into receive admission, and the actual
CalReg admission and ForwardHop egress/landing results into delivery admission.
The explicit test uses source 1 and destination 9 on the 40-node carrier, 8-bit
epochs, one receive context, two early slots and two identity slots. CalReg
content is opaque; the installation cycle supplies an explicit drained
precondition. No link timing or automatic network-drain algorithm is inferred.

## Two rounds after installation {#WSEMIG-NODE-VER-001}
<!-- ndf: kind=verification modality=must refinement=L3 domain=verification status=draft verifies=WSEMIG-NODE-001 -->

The 13-cycle fixture MUST reject an initial send attempt before installation,
install opcode 1, then post/receive/retire two rounds with epochs 1 and 2.
Each round sends two four-byte identities and repeats the first identity.
Expected bytes are 8, capacity is 16, addresses are 100 and 104. The second
round reuses the sequence identities after retirement and keeps the CalReg
installation. A final installed lookup sends no data.

Both generated backends MUST expose and check all 30 connected control,
routing, segment and receive fields at both sampling points of every cycle.
Host expectations use CalendarRouteRegs, EpochTracker, CalRegBank,
simulate_multicast and ReceiveAccount independently of the compiled source.
