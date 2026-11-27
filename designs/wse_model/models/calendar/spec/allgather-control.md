---
doc_id: WSEMIG-DOC-CLOSEDLOOP
status: draft
authority: normative
owner: wse-calendar
---

# Completion-driven multi-node Calendar control

## Member control and receive state {#WSEMIG-CLOSEDLOOP-MEMBER-001}
<!-- ndf: kind=requirement modality=must refinement=L2 domain=noc status=draft refines=WSEMIG-RX-REQ-001 -->

Every one of the 32 participants MUST own a CalRegAdmission, EpochBank and
ReceiveEngine. Installation MUST complete before execution. The two phases
MUST preserve installed configuration and advance each member's actual opcode-1
counter exactly once. Receive posts MUST use that local counter; sends MUST use
the value agreed by all member counters. The local decoded table entry MUST
supply compiled expected bytes, checked against the phase geometry.

A member MUST issue a receive post once, wait while external post admission is
withheld, and retain its account through retries. No incomplete account may be
retired. These rules follow Calendar O3/E1 and the ordinary core's run_allgather.

## Feedback controls phase progression {#WSEMIG-CLOSEDLOOP-001}
<!-- ndf: kind=requirement modality=must refinement=L2 domain=noc status=draft refines=WSEMIG-ALLGATHER-001 -->

The controller MUST advance through installation, epoch allocation, posting,
sending, joining and retirement using actual member feedback. It MUST wait for
all posts before sending, retain the current segment identity until every
remote destination accepts it, and wait for every account to complete with no
pending entries before retirement. The next key MUST begin only after all
accounts retire. A member fault or divergent epochs MUST stop progression.
The second retirement MUST enter a stable done state.

This remains the existing serialized functional carrier: eight stateless hop
slots serve the active group, 32 receive accounts remain resident, and each
multicast finishes before the next begins. There is no separate link queue;
zero pending receive entries plus the acknowledged final multicast and retired
accounts establish an empty carrier at the phase boundary. This is not a
physical NoC drain detector or a hardware Calendar timing schedule. CalReg
images remain opaque. External per-member permit masks belong to the test
adapter; they assign no hardware resource sizes or link delays.

## Baseline and controlled delays {#WSEMIG-CLOSEDLOOP-VER-001}
<!-- ndf: kind=verification modality=must refinement=L3 domain=verification status=draft verifies=WSEMIG-CLOSEDLOOP-001,WSEMIG-CLOSEDLOOP-MEMBER-001 -->

Both generated backends MUST run the same 40-node profile with 32 participants,
8-bit epochs, two early slots and 56 identity slots per member. Route vectors,
addresses, payload lengths and capacities MUST match the original two-phase
FFN fixture. Expected results MUST use the ordinary Python semantic core outside
the compiled model.

The baseline permits every post and landing immediately. The controlled variant
changes only permit inputs: member 31 cannot post until cycle 5; member 30's
landing permit is withheld during cycles 261..263. The host MUST verify that
posting waits, the final row-phase segment is retried with the same identity,
other destinations do not double-count it, and the column phase starts only
after actual completion and retirement. It MUST compare every member's 32
observed fields and every controller state at both sampling points, including
two final cycles holding done. Expected run lengths are 523 and 529 cycles,
respectively; they are test observations, not controller deadlines.
