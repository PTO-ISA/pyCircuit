---
doc_id: WSEMIG-DOC-MULTINODE
status: draft
authority: normative
owner: wse-calendar
---

# Connected functional multicast tree

## Independent destination state {#WSEMIG-MULTINODE-001}
<!-- ndf: kind=requirement modality=must refinement=L2 domain=noc status=draft refines=WSEMIG-RX-REQ-001 -->

The functional fixture MUST compose ForwardHop egress outputs into downstream
reachability and use reached AND local landing to admit receive deliveries.
Each destination MUST own independent receive state. The explicit test tree is
1→9→{8,10,17} on the 40-node profile, with one segment at a time. Combinational
hop composition defines a functional test schedule, not hardware link latency.

## Tree closure {#WSEMIG-MULTINODE-VER-001}
<!-- ndf: kind=verification modality=must refinement=L3 domain=verification status=draft verifies=WSEMIG-MULTINODE-001 -->

The 12-cycle fixture MUST post, deliver, repeat an identity, deliver a second
identity, idle and retire in each of two epochs. Center 9 lands in the first
epoch and is transit-only in the second. Five receiver instances each use two
early and two identity slots. Host expectations come from validated route bits,
simulate_multicast and ReceiveAccount, outside the DUT.

Execution status, toolchain identity and retained earlier failures are recorded
separately in `docs/development/pycircuit-migration.md`.
