---
doc_id: WSEMIG-DOC-ALLGATHER
status: draft
authority: normative
owner: wse-calendar
---

# FFN two-phase functional AllGather

## Resident accounts and routed delivery {#WSEMIG-ALLGATHER-001}
<!-- ndf: kind=requirement modality=must refinement=L2 domain=noc status=draft refines=WSEMIG-RX-REQ-001 -->

The fixture MUST retain 32 independent ReceiveEngine instances across both
FFN phases on the 40-node Calendar carrier. Every destination arrival MUST
depend on the actual ForwardHop outputs along its source-to-destination path.
The explicit test configuration is two early slots and 56 identity slots per
account, 8-bit epochs and 16-bit sequence numbers. These are functional test
parameters, not hardware resource specifications.

The test sends one segment at a time. Eight stateless forwarding instances
serve the active row or column group. This serialized schedule assigns no
physical link latency, concurrent arbitration or Calendar release times.
The original Python core defines one receive segment per payload row;
wire flits do not create additional receive identities.

## Both phases on every member {#WSEMIG-ALLGATHER-VER-001}
<!-- ndf: kind=verification modality=must refinement=L3 domain=verification status=draft verifies=WSEMIG-ALLGATHER-001 -->

The generated C++ and Verilog simulations MUST execute 516 cycles: post,
256 source/row deliveries and retire for each of two phases. All 32 members
MUST receive 10752 bytes from 56 identities in the row phase and 36864 bytes
from 24 identities in the column phase. Both phases use opcode 1, epochs 1
and 2, and source sequence numbers 0..7 followed by 8..15.

The host MUST check every node's routing and receive observations at both
sampling points per cycle against validated core routes, simulate_multicast
and ReceiveAccount. Expected observations MUST remain outside compiled sources.
Execution status and toolchain identity are recorded by tools/verify_calendar.py.
