---
doc_id: WSEMIG-DOC-CONTROL-FORWARD
status: draft
authority: normative
owner: wse-calendar
---

# Calendar control and per-hop semantics

## Preserve the Python core contracts {#WSEMIG-INTENT-002}
<!-- ndf: kind=requirement modality=must refinement=L0 domain=noc status=draft -->

The migrated modules MUST preserve the core epoch, configuration-admission and
single-hop forwarding rules under the explicitly selected Calendar test profile.
The profile is 40 nodes in a 5-by-8 mesh and 8-bit epochs. Queue capacities,
CalReg timeslot contents and link timing are outside these modules' scope.

## Stateful control and physical adjacency {#WSEMIG-CONTROL-REQ-001}
<!-- ndf: kind=requirement modality=must refinement=L1 domain=noc status=draft refines=WSEMIG-INTENT-002 -->

Epochs MUST persist independently per opcode and wrap only after drain.
Configuration writes MUST require a drained window. Forwarding MUST choose only
physical neighbors with P=1 and exclude the ingress neighbor; local L controls
landing. The consumer MUST validate the full route's induced tree before use.

## Epoch bank {#WSEMIG-EPOCH-001}
<!-- ndf: kind=requirement modality=must refinement=L2 domain=noc status=draft refines=WSEMIG-CONTROL-REQ-001 -->

Each opcode counter MUST start at zero. An advance produces 1..255; the next advance
requires drained=true and produces 1. Idle and rejected requests preserve state.
Opcodes 0 and 7 fault on advance. This is the accepted 8-bit test profile.

## Resident installation map {#WSEMIG-CALREG-001}
<!-- ndf: kind=requirement modality=must refinement=L2 domain=noc status=draft refines=WSEMIG-CONTROL-REQ-001 -->

CalRegAdmission MUST retain which slots are installed. A drained clear removes
all entries; installing an uninstalled nonreserved opcode requires drained=true.
Installing a duplicate, installing a reserved opcode or looking up an absent
slot faults without changing the map. The drained input is an external
precondition; this module does not measure network traffic or interpret contents.

## Per-hop forwarding {#WSEMIG-FORWARD-001}
<!-- ndf: kind=requirement modality=must refinement=L2 domain=noc status=draft refines=WSEMIG-CONTROL-REQ-001 -->

On the 5-by-8 profile, ForwardHop MUST output a 40-bit neighbor mask, local landing
decision and fault. Node 7 and node 8 are not adjacent. An illegal 01 pair or a
nonpassing current node MUST fault and suppress outputs. Route-wide cycle and
membership validation remains owned by the Python preflight.

## Control history {#WSEMIG-CONTROL-VER-001}
<!-- ndf: kind=verification modality=must refinement=L3 domain=verification status=draft verifies=WSEMIG-EPOCH-001,WSEMIG-CALREG-001 -->

The checker MUST run 261 identical cycles on C++ and Verilog: all 255 nonzero epochs, a second
opcode, a rejected wrap, an idle observation proving retention, an allowed wrap,
opcode isolation and a reserved-opcode rejection. In the same system check
missing/installed lookups, duplicate/reserved installation, undrained clear,
drained clear and another installed opcode. Require 522 checker observations.

## Hop vectors {#WSEMIG-FORWARD-VER-001}
<!-- ndf: kind=verification modality=must refinement=L3 domain=verification status=draft verifies=WSEMIG-FORWARD-001 -->

The checker MUST run nine authored literal vectors on both backends: row traversal, column
traversal through node 32, the row boundary between 7 and 8, a multicast star with
two different ingress ports and an illegal pair at node 39. Compare all output
fields. Require 18 checker observations. These are single-hop functional cases;
they do not establish full-mesh delivery, byte accounting or timing accuracy.
