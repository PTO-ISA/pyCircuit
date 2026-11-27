# ruff: noqa: N802, F823, F841 -- captured System state.
"""Literal single-hop row/column boundaries and a branched multicast tree."""

import pycircuit as ac
from wse_calendar.forward import ForwardHop, HopInput


@ac.struct
class Scenario:
    route: ac.bits[80]
    node: ac.u6
    has_ingress: bool
    ingress: ac.u6
    egress: ac.u40
    lands: bool
    fault: bool


def vectors() -> ac.table[9, Scenario]:
    rows: ac.table[9, Scenario] = (
        # Row 0: source 0 passes; nodes 1..7 pass and land.
        Scenario(route=0xFFFE, node=0, egress=0x2),
        Scenario(route=0xFFFE, node=3, has_ingress=True, ingress=2, egress=0x10, lands=True),
        Scenario(route=0xFFFE, node=7, has_ingress=True, ingress=6, lands=True),
        # Column 0: source 0 passes; nodes 8,16,24,32 pass and land.
        Scenario(
            route=0x00030003000300030002,
            node=8,
            has_ingress=True,
            ingress=0,
            egress=0x10000,
            lands=True,
        ),
        Scenario(route=0x00030003000300030002, node=32, has_ingress=True, ingress=24, lands=True),
        # Node 7 is not adjacent to node 8; only node 9 receives.
        Scenario(route=0xE8000, node=8, egress=0x200),
        # Star centered on 9, with leaves 1,8,10,17. Exclude ingress 1.
        Scenario(route=0xC003B000C, node=9, has_ingress=True, ingress=1, egress=0x20500),
        # Illegal 01 at node 39 faults the entire entry before forwarding.
        Scenario(route=0x40000000000000000002, node=0, fault=True),
        # Same star; changing only ingress must enable north and suppress south.
        Scenario(route=0xC003B000C, node=9, has_ingress=True, ingress=17, egress=0x502),
    )
    return rows


@ac.system
def ForwardSystem():
    phase: ac.u4 = 0
    rows = vectors()
    row = rows[phase % 9]
    result = ForwardHop(
        HopInput(route=row.route, node=row.node, has_ingress=row.has_ingress, ingress=row.ingress)
    )

    # ndf: verifies=WSEMIG-FORWARD-VER-001
    @ac.rule
    def check() -> None:
        assert result.egress == row.egress, "physical neighbors minus ingress"
        assert result.lands == row.lands, "local landing bit"
        assert result.fault == row.fault, "illegal pair fault"
        ac.log("info", "forward_checked", phase, result.egress)
        phase = phase + 1
