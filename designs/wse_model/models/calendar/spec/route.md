---
doc_id: WSEMIG-DOC-ROUTE
status: draft
authority: normative
owner: wse-calendar
---

# Calendar entry migration

## Preserve the baseline contract {#WSEMIG-INTENT-001}
<!-- ndf: kind=requirement modality=must refinement=L0 domain=noc status=draft -->

The current pyCircuit consumer MUST preserve the Calendar §1.7 and §2.2
40-node register-pair encoding. The design documents in the parent repository
remain the architectural authority. The 48-node emitted-entry layout remains Q1.

## Observable entry decode {#WSEMIG-REQ-001}
<!-- ndf: kind=requirement modality=must refinement=L1 domain=noc status=draft refines=WSEMIG-INTENT-001 -->

For each fixed node 0..39, the decoder MUST expose its two route bits, the
land-count, flags and expected-byte fields. Pair 01 MUST produce a fault flag.
The caller owns fault handling; this decoder does not repair illegal entries.

## Register boundaries {#WSEMIG-ROUTE-001}
<!-- ndf: kind=requirement modality=must refinement=L2 domain=noc status=draft refines=WSEMIG-REQ-001 -->

DecodeEntry MUST select node 0..31 from rbLo and node 32..39 from rbHi[15:0].
It MUST decode rbHi[23:16] as landCount, [31:24] as flags and [63:32] as
expectedRxBytes, without treating the metadata as route bits. It MUST be stateless.

## Independent boundary vectors {#WSEMIG-ROUTE-VER-001}
<!-- ndf: kind=verification modality=must refinement=L3 domain=verification status=draft verifies=WSEMIG-REQ-001,WSEMIG-ROUTE-001 -->

The RouteSystem checker MUST compare generated C++ and Verilog execution with
four authored literal vectors at nodes 0, 31, 32 and 39. Include the published
golden route, populated high and low register boundaries, metadata, and illegal
01 pairs. Run four cycles with identical vectors on both targets. Keep source
revisions, commands, logs, simulator identity and exit codes. These checks cover
combinational decoding, not network timing or stateful receive behavior.
