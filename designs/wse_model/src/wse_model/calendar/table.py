"""The compiled route table: ``kCalendarRoute[keyId][node]``.

Calendar §2.6 fixes the table shape and §2.7.3 the compile-time five steps. This
module holds the compiled artifact — one 16 B entry per (key, source node) — plus
its two layouts and its emitters:

* **key-major** (the document's baseline): ``keyId * node_count + node``, so a
  core's entries sit 640 B apart and occupy one D-cache line each;
* **node-major** (open item ``C-9``): ``node * key_count + keyId``, so a core's
  entries are contiguous and occupy ``ceil(keyCount / 4)`` lines. Instruction
  count, addressing shape, and total bytes are identical; only the locality
  changes, and the per-core residency drops 4x.

The table also carries the three version numbers that replace the missing
"one instruction atomically selects route and timeslot" anchor (whitepaper
§10.3, HW-13), and the ``conflictProof`` returned by the NoC algorithm. The model
records that proof; it does not re-derive the timing guarantees of §2.7.1.
"""

from __future__ import annotations

from collections.abc import Iterable
from dataclasses import dataclass, field
from enum import Enum
from typing import Any

from wse_model.calendar.collective import Collective, RedOp
from wse_model.calendar.entry import (
    ENTRY_SIZE_BYTES,
    CalendarRouteEntry,
    RouteEntryFlags,
    entry_layout_size_bytes,
)
from wse_model.calendar.geometry import PayloadGeometry
from wse_model.calendar.key import CalendarKeyRef, GroupRole, RouteKey
from wse_model.calendar.route_bits import RouteBits
from wse_model.calendar.validate import (
    D_CACHE_LINE_BYTES,
    MAX_KEY_COUNT,
    ValidationReport,
    validate_entry_layout,
    validate_route_bits,
    validate_table_budget,
)
from wse_model.errors import CalendarError, WseModelError
from wse_model.topology import MeshTopology

__all__ = [
    "CalendarRouteTable",
    "KeyDefinition",
    "TableLayout",
]

#: Alignment the emitted ``.rodata`` segment must satisfy (prohibition F3).
TABLE_ALIGNMENT_BYTES = 64


class TableLayout(str, Enum):
    """Where a key's entries sit relative to each other."""

    KEY_MAJOR = "key-major"
    NODE_MAJOR = "node-major"

    @property
    def open_item(self) -> str | None:
        return "C-9" if self is TableLayout.NODE_MAJOR else None

    @property
    def d_cache_lines_per_core(self) -> Any:
        """Returns a callable ``(key_count) -> lines`` for this layout."""
        if self is TableLayout.NODE_MAJOR:

            def lines(key_count: int) -> int:
                return (key_count * ENTRY_SIZE_BYTES + D_CACHE_LINE_BYTES - 1) // D_CACHE_LINE_BYTES

            return lines
        return lambda key_count: key_count


@dataclass
class KeyDefinition:
    """One logical identity's compiled rows: one entry per source node."""

    key_id: int
    route_key: RouteKey
    rows: tuple[CalendarRouteEntry, ...]
    geometry: PayloadGeometry | None = None
    member_nodes: frozenset[int] = frozenset()
    #: Per-source collective group. Under SPMD one kernel text can only name a
    #: role like ``ROW(my_row)``; the compiler resolves it per core, so the
    #: destination set differs per source and cannot be a single set.
    groups: dict[int, tuple[int, ...]] = field(default_factory=dict)
    self_delivery: bool = False
    #: ``armLeadCycles[opcode]`` from the NoC algorithm: the window after
    #: alignment during which MTE4 and every MTE3 descriptor must be enqueued.
    arm_lead_cycles: int | None = None
    #: The algorithm's proof that the timing guarantees of §2.7.1 hold.
    conflict_proof: str | None = None

    def __post_init__(self) -> None:
        if not 0 <= self.key_id < MAX_KEY_COUNT:
            raise CalendarError(f"keyId {self.key_id} is outside the 0..{MAX_KEY_COUNT - 1} budget")

    @property
    def node_count(self) -> int:
        return len(self.rows)

    @property
    def opcode(self) -> int:
        return self.route_key.opcode

    @property
    def collective(self) -> Collective:
        return self.route_key.collective

    def entry(self, node: int) -> CalendarRouteEntry:
        if not 0 <= node < len(self.rows):
            raise CalendarError(
                f"keyId {self.key_id}: node {node} is outside 0..{len(self.rows) - 1}"
            )
        return self.rows[node]

    def land_counts(self) -> dict[int, int]:
        """Per destination, the number of sources that land on it."""
        counts = dict.fromkeys(range(self.node_count), 0)
        for row in self.rows:
            for destination in row.route_bits.land_nodes:
                counts[destination] += 1
        return counts

    def group_of(self, node: int) -> tuple[int, ...] | None:
        """The source's resolved group, i.e. its expected destination set."""
        if self.groups:
            return self.groups.get(node)
        if self.member_nodes:
            return tuple(sorted(self.member_nodes))
        return None

    def key_ref(self, *, route_version: int, red_op: RedOp | None = None) -> CalendarKeyRef:
        if red_op is None:
            red_op = RedOp.SUM if self.collective.is_reduction else RedOp.NONE
        return CalendarKeyRef(
            key_id=self.key_id,
            opcode=self.collective,
            red_op=red_op,
            route_version=route_version,
        )

    def describe(self) -> dict[str, object]:
        return {
            "key_id": self.key_id,
            "route_key": self.route_key.describe(),
            "node_count": self.node_count,
            "member_nodes": sorted(self.member_nodes),
            "groups": {str(k): list(v) for k, v in sorted(self.groups.items())},
            "self_delivery": self.self_delivery,
            "arm_lead_cycles": self.arm_lead_cycles,
            "conflict_proof": self.conflict_proof,
            "geometry": self.geometry.describe() if self.geometry else None,
            "land_counts": {str(k): v for k, v in sorted(self.land_counts().items())},
        }


@dataclass
class CalendarRouteTable:
    """A compiled ``kCalendarRoute`` table plus its metadata."""

    topology: MeshTopology
    keys: dict[int, KeyDefinition] = field(default_factory=dict)
    layout: TableLayout = TableLayout.KEY_MAJOR
    route_version: int = 0
    calendar_version: int = 0
    topology_version: int = 0

    # -- construction --------------------------------------------------------

    def add_key(self, definition: KeyDefinition) -> None:
        if len(definition.rows) != self.topology.node_count:
            raise CalendarError(
                f"keyId {definition.key_id} has {len(definition.rows)} rows but "
                f"topology {self.topology.name} has {self.topology.node_count} "
                "nodes; the table is emitted for all nodes, with no per-core "
                "pruning (Calendar §2.5, §2.7.3 step 3)"
            )
        if definition.key_id in self.keys:
            raise CalendarError(f"keyId {definition.key_id} is already defined")
        self.keys[definition.key_id] = definition

    @property
    def key_count(self) -> int:
        return len(self.keys)

    def key(self, key_id: int) -> KeyDefinition:
        try:
            return self.keys[key_id]
        except KeyError:
            raise CalendarError(f"the table has no keyId {key_id}") from None

    def entry(self, key_id: int, node: int) -> CalendarRouteEntry:
        return self.key(key_id).entry(node)

    def ordered_keys(self) -> tuple[KeyDefinition, ...]:
        return tuple(self.keys[key_id] for key_id in sorted(self.keys))

    # -- addressing ----------------------------------------------------------

    def entry_index(self, key_id: int, node: int) -> int:
        """Flat entry index under the active layout."""
        if self.layout is TableLayout.NODE_MAJOR:
            return node * self.key_count + key_id
        return key_id * self.topology.node_count + node

    def entry_offset(self, key_id: int, node: int) -> int:
        """Byte offset of an entry in the emitted segment."""
        return self.entry_index(key_id, node) * ENTRY_SIZE_BYTES

    @property
    def table_bytes(self) -> int:
        return self.key_count * self.topology.node_count * ENTRY_SIZE_BYTES

    @property
    def d_cache_lines_per_core(self) -> int:
        return self.layout.d_cache_lines_per_core(self.key_count)

    @property
    def d_cache_bytes_per_core(self) -> int:
        return self.d_cache_lines_per_core * D_CACHE_LINE_BYTES

    # -- emission ------------------------------------------------------------

    def to_bytes(self) -> bytes:
        """Emit the ``.rodata`` segment, padded to 64 B alignment."""
        for definition in self.ordered_keys():
            for row in definition.rows:
                if entry_layout_size_bytes(row.node_count) != ENTRY_SIZE_BYTES:
                    raise WseModelError(
                        f"keyId {definition.key_id} cannot be emitted: a "
                        f"{row.node_count}-node entry is "
                        f"{entry_layout_size_bytes(row.node_count)} B, not the "
                        f"{ENTRY_SIZE_BYTES} B layout of Calendar §2.6 (open item Q1)"
                    )
        buffer = bytearray(self.table_bytes)
        for definition in self.ordered_keys():
            for node, row in enumerate(definition.rows):
                offset = self.entry_offset(definition.key_id, node)
                buffer[offset : offset + ENTRY_SIZE_BYTES] = row.to_bytes()
        padding = (-len(buffer)) % TABLE_ALIGNMENT_BYTES
        return bytes(buffer) + b"\x00" * padding

    def regs(self, key_id: int, node: int) -> dict[str, object]:
        """The register pair a send would load for this (key, core)."""
        return self.entry(key_id, node).to_regs().describe()

    # -- validation ----------------------------------------------------------

    def validate(self) -> ValidationReport:
        """Run the full §2.7.2 check set over this table."""
        report = ValidationReport()

        report.ran("entry-fits-gpr-pair")
        for definition in self.ordered_keys():
            report.merge(validate_entry_layout(self.topology.node_count, key_id=definition.key_id))

        report.ran("per-source-structural-checks")
        for definition in self.ordered_keys():
            for source, row in enumerate(definition.rows):
                if definition.member_nodes and source not in definition.member_nodes:
                    # Calendar §2.2.1: a node that is not a member of this logical
                    # identity carries an all-zero row, so it has no path to check.
                    # It never calls the collective.
                    continue
                group = definition.group_of(source)
                if group is None:
                    expected_land = None
                elif definition.self_delivery:
                    expected_land = frozenset(group)
                else:
                    expected_land = frozenset(group) - {source}
                sub = validate_route_bits(
                    self.topology,
                    row.route_bits,
                    key_id=definition.key_id,
                    source=source,
                    expected_land_nodes=expected_land,
                    expected_land_count=row.land_count,
                    self_delivery=definition.self_delivery,
                )
                report.merge(sub)

        report.ran("send-receive-conservation")
        for definition in self.ordered_keys():
            self._check_conservation(definition, report)

        report.ran("arm-reachability")
        for definition in self.ordered_keys():
            if definition.arm_lead_cycles is None:
                report.warn(
                    "V-ARM-UNSET",
                    "no armLeadCycles recorded, so the arm window cannot be "
                    "checked against the MTE4 and MTE3 enqueue cost (Calendar "
                    "§2.7.2)",
                    key_id=definition.key_id,
                )
            elif definition.arm_lead_cycles < 0:
                report.error(
                    "V-ARM-NEGATIVE",
                    f"armLeadCycles={definition.arm_lead_cycles} is negative",
                    key_id=definition.key_id,
                )

        report.ran("noc-timing-proof-present")
        for definition in self.ordered_keys():
            if definition.conflict_proof is None:
                report.warn(
                    "V-CONFLICT-PROOF-MISSING",
                    "no conflictProof recorded; the same-opcode no-conflict and "
                    "no-concurrency guarantees are the NoC algorithm's "
                    "responsibility and cannot be re-derived by software "
                    "(Calendar §2.7.1)",
                    key_id=definition.key_id,
                )

        report.merge(
            validate_table_budget(
                key_count=self.key_count,
                node_count=self.topology.node_count,
                per_core_keys=self.key_count,
                node_major=self.layout is TableLayout.NODE_MAJOR,
            )
        )
        report.ran("table-budget")
        return report

    def _check_conservation(self, definition: KeyDefinition, report: ValidationReport) -> None:
        land_counts = definition.land_counts()
        geometry = definition.geometry
        if geometry is None:
            report.warn(
                "V-GEOMETRY-MISSING",
                "no payload geometry recorded, so expectedRxBytes cannot be "
                "checked against the land-set sum (Calendar §2.7.2)",
                key_id=definition.key_id,
            )
            return
        for destination, land_count in sorted(land_counts.items()):
            derived = geometry.exp_val(land_count)
            declared = definition.entry(destination).expected_rx_bytes
            if derived != declared:
                report.error(
                    "V-CONSERVE",
                    f"destination {destination}: the land set delivers "
                    f"{derived} B (landCount={land_count} x rowCount="
                    f"{geometry.row_count} x rowBytes={geometry.row_bytes}) but "
                    f"the entry declares expectedRxBytes={declared}; this value "
                    "is the device-side expVal and the two must be equal "
                    "(Calendar §2.7.2)",
                    key_id=definition.key_id,
                    node=destination,
                )

    def check_versions(
        self, *, topology_version: int, calendar_version: int, route_version: int
    ) -> None:
        """The three-way load-time version check (whitepaper §10.3, HW-13)."""
        from wse_model.errors import VersionMismatchError

        pairs = (
            ("topologyVersion", self.topology_version, topology_version),
            ("calendarVersion", self.calendar_version, calendar_version),
            ("routeVersion", self.route_version, route_version),
        )
        mismatches = [
            f"{name}: table {have} vs chip {want}" for name, have, want in pairs if have != want
        ]
        if mismatches:
            raise VersionMismatchError(
                "load-time version check failed: "
                + "; ".join(mismatches)
                + ". Any mismatch must fault rather than degrade "
                "(whitepaper §14.2, HW-13)"
            )

    # -- serialization -------------------------------------------------------

    def to_dict(self) -> dict[str, object]:
        return {
            "schema": "wse-model/calendar-route-table/1",
            "topology": self.topology.describe(),
            "layout": self.layout.value,
            "versions": {
                "topologyVersion": self.topology_version,
                "calendarVersion": self.calendar_version,
                "routeVersion": self.route_version,
            },
            "key_count": self.key_count,
            "table_bytes": self.table_bytes,
            "d_cache_lines_per_core": self.d_cache_lines_per_core,
            "keys": [
                {
                    **definition.describe(),
                    "entries": [
                        {
                            "node": node,
                            "route_bits": row.route_bits.to_hex(),
                            "land_count": row.land_count,
                            "flags": int(row.flags),
                            "expected_rx_bytes": row.expected_rx_bytes,
                        }
                        for node, row in enumerate(definition.rows)
                    ],
                }
                for definition in self.ordered_keys()
            ],
        }

    def rodata_hex(self) -> str:
        raw = self.to_bytes().hex().upper()
        return "\n".join(
            " ".join(raw[i : i + 2] for i in range(row, min(row + 32, len(raw)), 2))
            for row in range(0, len(raw), 32)
        )

    @classmethod
    def from_dict(cls, payload: dict[str, Any]) -> CalendarRouteTable:
        """Rehydrate a table written by :meth:`to_dict`.

        This is the compiler boundary: a route table produced anywhere may be
        re-loaded and validated here, but it is never regenerated by the model
        (Calendar §2.7: software must not rewrite the routing algorithm).
        """
        schema = payload.get("schema")
        if schema != "wse-model/calendar-route-table/1":
            raise CalendarError(
                f"unsupported route-table schema {schema!r}; expected "
                "'wse-model/calendar-route-table/1'"
            )
        topology_payload = payload["topology"]
        topology = MeshTopology(
            rows=int(topology_payload["rows"]),
            cols=int(topology_payload["cols"]),
            name=str(topology_payload.get("name", "mesh")),
        )
        versions = payload.get("versions", {})
        table = cls(
            topology=topology,
            layout=TableLayout(payload.get("layout", TableLayout.KEY_MAJOR.value)),
            route_version=int(versions.get("routeVersion", 0)),
            calendar_version=int(versions.get("calendarVersion", 0)),
            topology_version=int(versions.get("topologyVersion", 0)),
        )
        for key_payload in payload.get("keys", []):
            route_key_payload = key_payload["route_key"]
            group_role = route_key_payload.get("group_role", "ROLE(?)")
            kind, _, axis = str(group_role).partition("(")
            axis_value: int | str = axis.rstrip(")")
            if isinstance(axis_value, str) and axis_value.lstrip("-").isdigit():
                axis_value = int(axis_value)
            route_key = RouteKey(
                program_id=str(route_key_payload["program_id"]),
                kernel_id=str(route_key_payload["kernel_id"]),
                phase_id=str(route_key_payload["phase_id"]),
                collective=Collective[str(route_key_payload["collective"])],
                group_role=GroupRole(kind=kind, axis=axis_value),
            )
            entries = key_payload.get("entries", [])
            rows = [
                CalendarRouteEntry(
                    route_bits=RouteBits.from_hex(str(entry["route_bits"]), topology.node_count),
                    expected_rx_bytes=int(entry.get("expected_rx_bytes", 0)),
                    flags=RouteEntryFlags(int(entry.get("flags", 0))),
                    declared_land_count=entry.get("land_count"),
                )
                for entry in entries
            ]
            geometry_payload = key_payload.get("geometry")
            geometry = (
                PayloadGeometry(
                    row_count=int(geometry_payload["rowCount"]),
                    row_bytes=int(geometry_payload["rowBytes"]),
                    send_row_stride=int(geometry_payload["sendRowStride"]),
                    recv_row_count=int(geometry_payload["recvRowCount"]),
                    recv_row_stride=int(geometry_payload["recvRowStride"]),
                )
                if geometry_payload
                else None
            )
            table.add_key(
                KeyDefinition(
                    key_id=int(key_payload["key_id"]),
                    route_key=route_key,
                    rows=tuple(rows),
                    geometry=geometry,
                    member_nodes=frozenset(
                        int(node) for node in key_payload.get("member_nodes", ())
                    ),
                    groups={
                        int(source): tuple(int(node) for node in group)
                        for source, group in key_payload.get("groups", {}).items()
                    },
                    self_delivery=bool(key_payload.get("self_delivery", False)),
                    arm_lead_cycles=key_payload.get("arm_lead_cycles"),
                    conflict_proof=key_payload.get("conflict_proof"),
                )
            )
        return table

    def describe(self) -> dict[str, object]:
        return {
            "topology": self.topology.name,
            "layout": self.layout.value,
            "key_count": self.key_count,
            "table_bytes": self.table_bytes,
            "d_cache_bytes_per_core": self.d_cache_bytes_per_core,
            "versions": {
                "topologyVersion": self.topology_version,
                "calendarVersion": self.calendar_version,
                "routeVersion": self.route_version,
            },
        }


def build_table(
    *,
    topology: MeshTopology,
    definitions: Iterable[KeyDefinition],
    layout: TableLayout = TableLayout.KEY_MAJOR,
    route_version: int = 0,
    calendar_version: int = 0,
    topology_version: int = 0,
) -> CalendarRouteTable:
    """Assemble a table from key definitions."""
    table = CalendarRouteTable(
        topology=topology,
        layout=layout,
        route_version=route_version,
        calendar_version=calendar_version,
        topology_version=topology_version,
    )
    for definition in definitions:
        table.add_key(definition)
    return table


def rows_from_bitmaps(
    bitmaps: Iterable[str | RouteBits],
    *,
    node_count: int,
    expected_rx_bytes: Iterable[int] | None = None,
    flags: Iterable[RouteEntryFlags] | None = None,
) -> tuple[CalendarRouteEntry, ...]:
    """Build a key's rows from hex bitmaps, one per source node.

    The route bitmap is an *input* to the model: Calendar §2.7 is explicit that
    software must not rewrite the routing algorithm, so bitmaps arrive from the
    NoC provider or from a fixture and are validated here rather than generated.
    """
    decoded = [
        bitmap if isinstance(bitmap, RouteBits) else RouteBits.from_hex(bitmap, node_count)
        for bitmap in bitmaps
    ]
    if len(decoded) != node_count:
        raise CalendarError(
            f"expected {node_count} bitmaps (one per source node), got {len(decoded)}"
        )
    rx = list(expected_rx_bytes) if expected_rx_bytes is not None else [0] * node_count
    fl = list(flags) if flags is not None else [RouteEntryFlags.NONE] * node_count
    if len(rx) != node_count or len(fl) != node_count:
        raise CalendarError("expected_rx_bytes and flags must have one item per node")
    return tuple(
        CalendarRouteEntry(route_bits=row, expected_rx_bytes=rx[i], flags=fl[i])
        for i, row in enumerate(decoded)
    )
