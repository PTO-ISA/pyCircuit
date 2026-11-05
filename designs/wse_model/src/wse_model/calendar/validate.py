"""Structural validation of route bitmaps and route tables.

These are the checks Calendar §2.7.2 requires PyPTO to run on the NoC
algorithm's output. They are independent of the algorithm implementation and
cost only a linear pass per node:

===============================  ==================================================
check                            rejects
===============================  ==================================================
bit pairs legal                  any ``01`` pair (``L=1`` with ``P=0``)
induced subgraph is a tree       cyclic or disconnected ``{P=1}`` (non-terminating
                                 forwarding, duplicate landing)
``P_self = 1``                   the source is not on its own path
land set complete                ``{L=1}`` differs from the semantics' destination
                                 set; ``landCount`` disagrees with ``popcount(L)``
send/receive conservation        ``sum(rowCount x rowBytes)`` per destination differs
                                 from ``expectedRxBytes`` — the device-side ``expVal``
``selfLand`` consistent          the source's own ``L`` bit disagrees with
                                 ``FabricSelfDelivery`` (a missed or doubled local
                                 segment)
static table emittable           ``keyCount`` over budget, misaligned emission, or a
                                 per-core D-cache residency over budget
topology isomorphic              the instance's topology / PG differs from the
                                 compile-time assumption
===============================  ==================================================

The two timing guarantees of §2.7.1 — "no conflict within one ``opcode``" and
"no two collectives share an ``opcode`` concurrently" — are **not** re-derived
here. They require the NoC timing model (ports, VCs, quotas, pipeline latency)
and are the algorithm's responsibility, returned as ``conflictProof``. The model
records the proof's presence and refuses to claim the property itself.
"""

from __future__ import annotations

from collections.abc import Iterable
from dataclasses import dataclass, field
from enum import Enum

from wse_model.calendar.entry import entry_layout_size_bytes
from wse_model.calendar.route_bits import RouteBits
from wse_model.errors import CalendarError
from wse_model.topology import MeshTopology

__all__ = [
    "D_CACHE_LINE_BYTES",
    "MAX_KEY_COUNT",
    "Diagnostic",
    "Severity",
    "ValidationReport",
    "check_induced_tree",
    "validate_route_bits",
]

#: D-cache line granularity used by the Calendar residency budget.
D_CACHE_LINE_BYTES = 64

#: Calendar §2.6 / §3.8.4: the emitted table is capped at 16 logical identities.
MAX_KEY_COUNT = 16


class Severity(str, Enum):
    ERROR = "error"
    WARNING = "warning"


@dataclass(frozen=True)
class Diagnostic:
    """One validation finding."""

    code: str
    severity: Severity
    message: str
    key_id: int | None = None
    node: int | None = None

    def __str__(self) -> str:
        location = ""
        if self.key_id is not None:
            location += f" keyId={self.key_id}"
        if self.node is not None:
            location += f" node={self.node}"
        return f"[{self.severity.value}] {self.code}{location}: {self.message}"

    def describe(self) -> dict[str, object]:
        return {
            "code": self.code,
            "severity": self.severity.value,
            "message": self.message,
            "key_id": self.key_id,
            "node": self.node,
        }


@dataclass
class ValidationReport:
    """An accumulated set of diagnostics."""

    diagnostics: list[Diagnostic] = field(default_factory=list)
    checks_run: list[str] = field(default_factory=list)

    def add(self, diagnostic: Diagnostic) -> None:
        self.diagnostics.append(diagnostic)

    def error(self, code: str, message: str, **location: int) -> None:
        self.add(Diagnostic(code, Severity.ERROR, message, **location))

    def warn(self, code: str, message: str, **location: int) -> None:
        self.add(Diagnostic(code, Severity.WARNING, message, **location))

    def ran(self, check: str) -> None:
        """Record that a check executed.

        The list behaves as a set with insertion order: a check that runs once per
        core would otherwise inflate it into noise, and "which checks ran" is the
        question a caller is asking.
        """
        if check not in self.checks_run:
            self.checks_run.append(check)

    def merge(self, other: ValidationReport) -> None:
        """Absorb another report's diagnostics **and** its check list.

        Both halves matter: a sub-report's ``checks_run`` is how a caller can see
        that a check actually executed, so merging only the diagnostics would make
        a skipped check look like a passing one.
        """
        self.diagnostics.extend(other.diagnostics)
        for check in other.checks_run:
            self.ran(check)

    def extend(self, diagnostics: Iterable[Diagnostic]) -> None:
        self.diagnostics.extend(diagnostics)

    @property
    def errors(self) -> tuple[Diagnostic, ...]:
        return tuple(d for d in self.diagnostics if d.severity is Severity.ERROR)

    @property
    def warnings(self) -> tuple[Diagnostic, ...]:
        return tuple(d for d in self.diagnostics if d.severity is Severity.WARNING)

    @property
    def ok(self) -> bool:
        return not self.errors

    def raise_if_failed(self) -> None:
        if self.errors:
            detail = "\n  ".join(str(error) for error in self.errors)
            raise CalendarError(f"Calendar validation failed:\n  {detail}")

    def describe(self) -> dict[str, object]:
        return {
            "ok": self.ok,
            "checks_run": list(self.checks_run),
            "errors": [d.describe() for d in self.errors],
            "warnings": [d.describe() for d in self.warnings],
        }

    def format(self) -> str:
        if not self.diagnostics:
            return f"OK ({len(self.checks_run)} checks)"
        lines = [str(diagnostic) for diagnostic in self.diagnostics]
        status = "OK" if self.ok else "FAILED"
        lines.append(f"{status}: {len(self.errors)} error(s), {len(self.warnings)} warning(s)")
        return "\n".join(lines)


@dataclass(frozen=True)
class TreeAnalysis:
    """The result of analysing the ``{P=1}`` induced subgraph."""

    nodes: frozenset[int]
    edges: tuple[tuple[int, int], ...]
    reached: frozenset[int]
    components: tuple[frozenset[int], ...]
    is_connected: bool
    is_acyclic: bool
    contains_source: bool
    roots: tuple[int, ...]

    @property
    def is_tree(self) -> bool:
        return self.is_connected and self.is_acyclic and self.contains_source


def check_induced_tree(
    topology: MeshTopology, route_bits: RouteBits, *, source: int
) -> TreeAnalysis:
    """Analyse the ``{P=1}`` induced subgraph rooted at ``source``.

    The subgraph is *induced*: rule (2) of Calendar §2.2 copies a flit to **all**
    physical neighbours with ``P=1``, so any two adjacent members share a live
    link whether or not the algorithm intended it. A 2 x 2 block all set to
    ``P=1`` therefore necessarily forms a cycle and the flit replicates forever.
    """
    nodes = route_bits.pass_nodes
    edges = tuple(topology.induced_edges(nodes))
    contains_source = source in nodes

    adjacency: dict[int, list[int]] = {node: [] for node in nodes}
    for a, b in edges:
        adjacency[a].append(b)
        adjacency[b].append(a)

    # Connected components over the induced subgraph.
    unseen = set(nodes)
    components: list[frozenset[int]] = []
    while unseen:
        start = min(unseen)
        stack = [start]
        seen = {start}
        while stack:
            node = stack.pop()
            for peer in adjacency[node]:
                if peer not in seen:
                    seen.add(peer)
                    stack.append(peer)
        unseen -= seen
        components.append(frozenset(seen))

    reached = frozenset()
    if contains_source:
        stack = [source]
        seen = {source}
        while stack:
            node = stack.pop()
            for peer in adjacency[node]:
                if peer not in seen:
                    seen.add(peer)
                    stack.append(peer)
        reached = frozenset(seen)

    is_connected = len(components) <= 1 and bool(nodes)
    # A connected graph is a tree exactly when |E| = |V| - 1. For a disconnected
    # graph, being a forest requires |E| = |V| - |components|.
    expected_edges = len(nodes) - len(components) if components else 0
    is_acyclic = len(edges) == expected_edges

    roots = tuple(
        sorted(
            node for node in nodes if not any(peer in nodes for peer in topology.neighbours(node))
        )
    )

    return TreeAnalysis(
        nodes=nodes,
        edges=edges,
        reached=reached,
        components=tuple(components),
        is_connected=is_connected,
        is_acyclic=is_acyclic,
        contains_source=contains_source,
        roots=roots,
    )


def validate_route_bits(
    topology: MeshTopology,
    route_bits: RouteBits,
    *,
    key_id: int | None = None,
    source: int | None = None,
    expected_land_nodes: frozenset[int] | None = None,
    expected_land_count: int | None = None,
    self_delivery: bool | None = None,
) -> ValidationReport:
    """Run the per-source structural checks of Calendar §2.7.2."""
    report = ValidationReport()

    if route_bits.node_count != topology.node_count:
        report.error(
            "V-NODECOUNT",
            f"routeBits covers {route_bits.node_count} nodes but topology "
            f"{topology.name} has {topology.node_count}",
            key_id=key_id,
        )
        return report

    report.ran("bit-pairs-legal")
    illegal = route_bits.illegal_nodes()
    for node in illegal:
        report.error(
            "V-PAIR",
            "bit pair 01 is illegal (L=1 with P=0); the compiler must reject it "
            "and the hardware must fault (Calendar §2.2)",
            key_id=key_id,
            node=node,
        )
    if illegal:
        # Every later structural check assumes a legal bitmap.
        return report

    if source is None:
        return report

    report.ran("source-on-path")
    if not route_bits.pair(source).passes:
        report.error(
            "V-PSELF",
            f"source {source} must have P=1 so the flit starts on its own path",
            key_id=key_id,
            node=source,
        )

    report.ran("induced-subgraph-is-tree")
    analysis = check_induced_tree(topology, route_bits, source=source)
    if not analysis.is_connected:
        component_list = ", ".join(
            "{" + ", ".join(str(n) for n in sorted(c)) + "}" for c in analysis.components
        )
        report.error(
            "V-TREE-CONNECTED",
            f"the {{P=1}} induced subgraph is disconnected: {component_list}; "
            "unreached land nodes would never receive the flit",
            key_id=key_id,
            node=source,
        )
    if not analysis.is_acyclic:
        report.error(
            "V-TREE-ACYCLIC",
            f"the {{P=1}} induced subgraph has {len(analysis.edges)} edges over "
            f"{len(analysis.nodes)} nodes and is therefore not a tree; passive "
            "forwarding would replicate the flit forever (Calendar §2.2)",
            key_id=key_id,
            node=source,
        )

    report.ran("land-set-complete")
    if expected_land_nodes is not None:
        actual = route_bits.land_nodes
        missing = sorted(expected_land_nodes - actual)
        extra = sorted(actual - expected_land_nodes)
        if missing or extra:
            report.error(
                "V-LAND",
                f"land set differs from the semantics' destination set "
                f"(missing {missing}, unexpected {extra})",
                key_id=key_id,
                node=source,
            )
    if expected_land_count is not None and expected_land_count != route_bits.land_count:
        report.error(
            "V-LANDCOUNT",
            f"declared landCount={expected_land_count} but popcount(L)={route_bits.land_count}",
            key_id=key_id,
            node=source,
        )

    report.ran("self-land-consistent")
    if self_delivery is not None:
        source_lands = route_bits.pair(source).lands
        if source_lands != self_delivery:
            report.error(
                "V-SELFLAND",
                f"source {source} has L={int(source_lands)} but "
                f"FabricSelfDelivery={self_delivery}; the local segment would be "
                "missed or written twice (Calendar §2.7.2, HW-10)",
                key_id=key_id,
                node=source,
            )

    return report


def validate_entry_layout(node_count: int, *, key_id: int | None = None) -> ValidationReport:
    """Check that a key's entries fit the frozen 16 B one-GPR-pair layout."""
    report = ValidationReport()
    report.ran("entry-fits-gpr-pair")
    size = entry_layout_size_bytes(node_count)
    if size != 16:
        report.error(
            "V-ENTRY-SIZE",
            f"a {node_count}-node entry needs {size} B, so it cannot be one 16 B "
            "GPR pair (Calendar §2.6). This is the structural consequence of open "
            "item Q1: at 48 nodes routeBits alone is 12 B.",
            key_id=key_id,
        )
    return report


def validate_table_budget(
    *,
    key_count: int,
    node_count: int,
    per_core_keys: int | None = None,
    node_major: bool = False,
) -> ValidationReport:
    """Check the emitted table against the Calendar §3.8.4 budgets."""
    report = ValidationReport()

    report.ran("key-count-budget")
    if key_count > MAX_KEY_COUNT:
        report.error(
            "V-KEYCOUNT",
            f"keyCount={key_count} exceeds the {MAX_KEY_COUNT} key budget; split "
            "the kernel per phase or switch to the node-major layout rather than "
            "compressing the entry format (Calendar §3.8.4)",
        )

    report.ran("table-alignment")
    entry_size = entry_layout_size_bytes(node_count)
    key_row_bytes = node_count * entry_size
    table_bytes = key_count * key_row_bytes
    if key_row_bytes % D_CACHE_LINE_BYTES != 0:
        report.warn(
            "V-ALIGN",
            f"one key row is {key_row_bytes} B, not a multiple of the "
            f"{D_CACHE_LINE_BYTES} B line; the emitted segment must still be "
            "alignas(64) so no entry straddles two lines (Calendar §3.10, "
            "prohibition F3)",
        )
    if table_bytes % D_CACHE_LINE_BYTES != 0:
        report.warn(
            "V-ALIGN-PAD",
            f"the table is {table_bytes} B before padding; the emitter pads the "
            f"segment to {D_CACHE_LINE_BYTES} B (Calendar §2.7.3 step 3)",
        )
    if table_bytes > 10 * 1024:
        report.warn(
            "V-DDR-FOOTPRINT",
            f"the table occupies {table_bytes} B; the design's stated ceiling is "
            "about 10 KB at keyCount=16 (Calendar §3.8.4)",
        )

    report.ran("d-cache-residency")
    keys = per_core_keys if per_core_keys is not None else key_count
    if node_major:
        lines = (keys * entry_size + D_CACHE_LINE_BYTES - 1) // D_CACHE_LINE_BYTES
    else:
        # Key-major scatters a core's entries one per key row, 640 B apart.
        lines = keys
    if lines > MAX_KEY_COUNT:
        report.error(
            "V-DCACHE",
            f"a core would resident {lines} D-cache lines for {keys} keys; the "
            f"budget is {MAX_KEY_COUNT} lines (Calendar §3.8.4)",
        )
    return report
