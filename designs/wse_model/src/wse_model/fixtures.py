"""Canonical fixtures from the design documents.

Every value here is transcribed from, or mechanically derived from, a specific
section of the design sources. Nothing is invented:

* :data:`GOLDEN_ROUTE_BITS` is the consistency anchor of Calendar §2.2.1, the
  vector the compiler, packager, RTL, simulator, and runtime dumps must all
  agree on byte for byte.
* :func:`ffn_key_definitions` encodes the FFN example of Calendar §2.2.1 and
  §2.8: an ``AllGather`` over the row group (``keyId 0``) and over the column
  group (``keyId 1``), with 32 member cells and the two frozen expected-byte
  values.

.. warning::

   The FFN rows are a **fixture**, not a routing algorithm. Calendar §2.7 is
   explicit that software must not rewrite the NoC's routing algorithm; the
   production bitmaps arrive from the NoC provider. What this module does is
   reconstruct the two documented group *shapes* so the model has a
   self-checking example, and it asserts the two published hex rows so the
   reconstruction cannot drift.
"""

from __future__ import annotations

from dataclasses import dataclass
from typing import Any

from wse_model.calendar.collective import Collective, RedOp
from wse_model.calendar.entry import CalendarRouteEntry, RouteEntryFlags
from wse_model.calendar.geometry import PayloadGeometry, SymmetricArena
from wse_model.calendar.key import GroupRole, RouteKey
from wse_model.calendar.route_bits import RouteBits
from wse_model.calendar.table import (
    CalendarRouteTable,
    KeyDefinition,
    TableLayout,
    build_table,
)
from wse_model.errors import CalendarError
from wse_model.noc.calreg import CalRegImage, CalRegSlot
from wse_model.topology import (
    CALENDAR_BASELINE,
    FFN_CELL_ROWS,
    FFN_CELLS,
    MeshTopology,
)

__all__ = [
    "FFN_PHASE_B_ROW_BYTES",
    "FFN_PHASE_C_ROW_BYTES",
    "GOLDEN_PASS_NODES",
    "GOLDEN_LAND_NODES",
    "GOLDEN_ROUTE_BITS",
    "FFNExample",
    "ffn_example",
    "golden_route_bits",
    "group_allgather_bitmap",
    "row_groups",
    "col_groups",
]

#: Calendar §2.2.1 golden vector: ``pass`` and ``land`` node sets.
GOLDEN_PASS_NODES = frozenset({0, 1, 2, 3, 4, 10, 18, 19})
GOLDEN_LAND_NODES = frozenset({4, 19})
GOLDEN_ROUTE_BITS = "AA 03 20 00 E0 00 00 00 00 00"
GOLDEN_SOURCE = 0

#: FFN phase B row geometry: ``rowBytes = 192`` (Calendar §2.2.1, §4.8).
FFN_PHASE_B_ROW_BYTES = 192
#: FFN phase C row geometry: ``rowBytes = 1536``.
FFN_PHASE_C_ROW_BYTES = 1536
#: Both phases carry 8 rows.
FFN_ROW_COUNT = 8


def golden_route_bits() -> RouteBits:
    """The Calendar §2.2.1 consistency anchor as a decoded bitmap."""
    return RouteBits.from_hex(GOLDEN_ROUTE_BITS, CALENDAR_BASELINE.topology.node_count)


def group_allgather_bitmap(
    topology: MeshTopology,
    group: tuple[int, ...],
    source: int,
    *,
    self_delivery: bool = False,
) -> RouteBits:
    """Build the documented AllGather bitmap for one group and source.

    Calendar §2.2.1 states the shape explicitly for both FFN keys: ``P`` is the
    whole group and ``L`` is the whole group except the source, because the
    source's own segment is written by a local UB-to-UB copy when the fabric
    does not self-deliver (Step4s).
    """
    if source not in group:
        raise CalendarError(f"source {source} is not a member of group {group}")
    land = frozenset(group) if self_delivery else frozenset(group) - {source}
    return RouteBits.from_sets(
        topology.node_count,
        pass_nodes=group,
        land_nodes=land,
    )


def row_groups(topology: MeshTopology, rows: int = FFN_CELL_ROWS) -> dict[int, tuple[int, ...]]:
    """Map each FFN cell to its ROW group, ``{source: group}``."""
    groups: dict[int, tuple[int, ...]] = {}
    for row in range(rows):
        group = topology.row_group(row)
        for node in group:
            groups[node] = group
    return groups


def col_groups(topology: MeshTopology, rows: int = FFN_CELL_ROWS) -> dict[int, tuple[int, ...]]:
    """Map each FFN cell to its COL group, ``{source: group}``."""
    groups: dict[int, tuple[int, ...]] = {}
    for col in range(topology.cols):
        group = tuple(topology.node_at(row, col) for row in range(rows))
        for node in group:
            groups[node] = group
    return groups


def _rows_for(
    topology: MeshTopology,
    groups: dict[int, tuple[int, ...]],
    *,
    expected_rx_bytes: dict[int, int],
    self_delivery: bool,
) -> tuple[CalendarRouteEntry, ...]:
    rows: list[CalendarRouteEntry] = []
    for node in topology.nodes:
        group = groups.get(node)
        if group is None:
            # Calendar §2.2.1: a node outside the FFN cells carries an all-zero
            # row for both logical identities.
            rows.append(CalendarRouteEntry(route_bits=RouteBits.from_int(0, topology.node_count)))
            continue
        bitmap = group_allgather_bitmap(topology, group, node, self_delivery=self_delivery)
        flags = RouteEntryFlags.IS_MEMBER
        if not self_delivery or node in group:
            # Each AllGather source is the root of its own routing tree.
            flags |= RouteEntryFlags.IS_ROOT
        if self_delivery:
            flags |= RouteEntryFlags.SELF_LAND
        rows.append(
            CalendarRouteEntry(
                route_bits=bitmap,
                expected_rx_bytes=expected_rx_bytes[node],
                flags=flags,
            )
        )
    return tuple(rows)


@dataclass(frozen=True)
class FFNExample:
    """The FFN two-phase AllGather example, fully compiled."""

    table: CalendarRouteTable
    phase_b_geometry: PayloadGeometry
    phase_c_geometry: PayloadGeometry
    phase_b_groups: dict[int, tuple[int, ...]]
    phase_c_groups: dict[int, tuple[int, ...]]
    members: frozenset[int]

    def geometry(self, key_id: int) -> PayloadGeometry:
        return self.phase_b_geometry if key_id == 0 else self.phase_c_geometry

    def groups(self, key_id: int) -> dict[int, tuple[int, ...]]:
        return self.phase_b_groups if key_id == 0 else self.phase_c_groups

    def calreg(self, *, arm_lead_cycles: int = 64) -> CalRegImage:
        """The timeslot mirror the loader installs.

        Both FFN phases use opcode 1 and therefore share one ``CalReg[1]`` entry,
        so that single slot lists both identities: the sharing constraint of
        Calendar §2.3.1 then shows up in the artifact, not only in the rule.
        """
        identities = tuple(
            definition.route_key.phase_id for definition in self.table.ordered_keys()
        )
        opcodes = sorted({definition.opcode for definition in self.table.ordered_keys()})
        return CalRegImage(
            slots=tuple(
                CalRegSlot(
                    opcode=opcode,
                    content=bytes([opcode]),
                    arm_lead_cycles=arm_lead_cycles,
                    identities=identities,
                )
                for opcode in opcodes
            ),
            calendar_version=self.table.calendar_version,
        )

    def arena(self, key_id: int, *, recv_sym_base: int = 0x1000) -> SymmetricArena:
        """One group's arena; the layout is identical for every group."""
        groups = self.groups(key_id)
        geometry = self.geometry(key_id)
        sample = next(iter(sorted(groups)))
        group = groups[sample]
        return SymmetricArena.packed(
            recv_sym_base=recv_sym_base,
            row_count=geometry.row_count,
            row_bytes=geometry.row_bytes,
            member_ranks=group,
            self_delivery=False,
        )

    def describe(self) -> dict[str, object]:
        return {
            "topology": self.table.topology.name,
            "members": sorted(self.members),
            "keys": [definition.describe() for definition in self.table.ordered_keys()],
            "phase_b_geometry": self.phase_b_geometry.describe(),
            "phase_c_geometry": self.phase_c_geometry.describe(),
        }


def ffn_example(
    *,
    layout: TableLayout = TableLayout.KEY_MAJOR,
    route_version: int = 1,
    calendar_version: int = 1,
    topology_version: int = 1,
) -> FFNExample:
    """Build the FFN example table and geometry from the documented shapes."""
    topology = CALENDAR_BASELINE.topology
    self_delivery = False

    b_groups = row_groups(topology)
    c_groups = col_groups(topology)
    members = FFN_CELLS

    # Calendar §2.2.1: phase B expectedRxBytes = 8 x 192 x 7 = 10752,
    # phase C expectedRxBytes = 8 x 1536 x 3 = 36864.
    b_exp = dict.fromkeys(members, FFN_ROW_COUNT * FFN_PHASE_B_ROW_BYTES * 7)
    c_exp = dict.fromkeys(members, FFN_ROW_COUNT * FFN_PHASE_C_ROW_BYTES * 3)

    b_rows = _rows_for(topology, b_groups, expected_rx_bytes=b_exp, self_delivery=self_delivery)
    c_rows = _rows_for(topology, c_groups, expected_rx_bytes=c_exp, self_delivery=self_delivery)

    b_geometry = PayloadGeometry(
        row_count=FFN_ROW_COUNT,
        row_bytes=FFN_PHASE_B_ROW_BYTES,
        send_row_stride=FFN_PHASE_B_ROW_BYTES,
        recv_row_count=FFN_ROW_COUNT,
        recv_row_stride=8 * FFN_PHASE_B_ROW_BYTES,
    )
    c_geometry = PayloadGeometry(
        row_count=FFN_ROW_COUNT,
        row_bytes=FFN_PHASE_C_ROW_BYTES,
        send_row_stride=FFN_PHASE_C_ROW_BYTES,
        recv_row_count=FFN_ROW_COUNT,
        recv_row_stride=4 * FFN_PHASE_C_ROW_BYTES,
    )

    key_b = KeyDefinition(
        key_id=0,
        route_key=RouteKey(
            program_id="ffn",
            kernel_id="ffn_fused",
            phase_id="phase_b_row_allgather",
            collective=Collective.ALL_GATHER,
            group_role=GroupRole(kind="ROW", axis="my_row"),
        ),
        rows=b_rows,
        geometry=b_geometry,
        member_nodes=members,
        groups=b_groups,
        self_delivery=self_delivery,
        arm_lead_cycles=64,
        conflict_proof="fixture:no-conflict-key0",
    )
    key_c = KeyDefinition(
        key_id=1,
        route_key=RouteKey(
            program_id="ffn",
            kernel_id="ffn_fused",
            phase_id="phase_c_col_allgather",
            collective=Collective.ALL_GATHER,
            group_role=GroupRole(kind="COL", axis="my_col"),
        ),
        rows=c_rows,
        geometry=c_geometry,
        member_nodes=members,
        groups=c_groups,
        self_delivery=self_delivery,
        arm_lead_cycles=64,
        conflict_proof="fixture:no-conflict-key1",
    )

    table = build_table(
        topology=topology,
        definitions=(key_b, key_c),
        layout=layout,
        route_version=route_version,
        calendar_version=calendar_version,
        topology_version=topology_version,
    )

    example = FFNExample(
        table=table,
        phase_b_geometry=b_geometry,
        phase_c_geometry=c_geometry,
        phase_b_groups=b_groups,
        phase_c_groups=c_groups,
        members=frozenset(members),
    )

    # The reconstruction must reproduce the two published rows exactly.
    expected_b = "FE FF 00 00 00 00 00 00 00 00"
    expected_c = "02 00 03 00 03 00 03 00 00 00"
    actual_b = table.entry(0, 0).route_bits.to_hex()
    actual_c = table.entry(1, 0).route_bits.to_hex()
    if actual_b != expected_b:
        raise CalendarError(
            f"FFN keyId 0 row 0 reconstructed as {actual_b} but Calendar §2.2.1 "
            f"publishes {expected_b}"
        )
    if actual_c != expected_c:
        raise CalendarError(
            f"FFN keyId 1 row 0 reconstructed as {actual_c} but Calendar §2.2.1 "
            f"publishes {expected_c}"
        )
    return example


def clean_kernel_object(
    *,
    key_count: int = 2,
    node_count: int = 40,
    path: str = "<fixture>",
    symbols: tuple[Any, ...] | None = None,
    sections: tuple[Any, ...] | None = None,
) -> Any:
    """A declared compiled product that satisfies F1, F2, and F3.

    Used by the tests and by ``wse-model check object --example`` as the
    known-good baseline that the negative cases are built from.
    """
    from wse_model.calendar.entry import ENTRY_SIZE_BYTES
    from wse_model.compiler.selfcheck import (
        ROUTE_TABLE_SYMBOL,
        ObjectFile,
        Section,
        SectionInfo,
        Symbol,
        SymbolKind,
    )

    default_symbols = (
        Symbol("ffn_fused", SymbolKind.FUNCTION, Section.TEXT, size=512),
        Symbol(
            ROUTE_TABLE_SYMBOL,
            SymbolKind.READ_ONLY_OBJECT,
            Section.RODATA,
            size=key_count * node_count * ENTRY_SIZE_BYTES,
            addr_align=64,
            is_global=False,
        ),
    )
    default_sections = (
        SectionInfo(Section.TEXT, addr_align=4, size=512),
        SectionInfo(
            Section.RODATA,
            addr_align=64,
            size=key_count * node_count * ENTRY_SIZE_BYTES,
        ),
    )
    return ObjectFile(
        symbols=default_symbols if symbols is None else tuple(symbols),
        sections=default_sections if sections is None else tuple(sections),
        path=path,
    )


def red_op_for(collective: Collective) -> RedOp:
    """The dormant/active ``redOp`` for a collective (Calendar §2.4)."""
    return RedOp.SUM if collective.is_reduction else RedOp.NONE
