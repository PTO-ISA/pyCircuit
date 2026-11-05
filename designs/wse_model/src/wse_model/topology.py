"""NoC topology descriptors.

The WSE NoC is a 2D mesh. Two readings of the node count exist and the design
sources do not reconcile them (whitepaper Appendix A, `Q1`):

* the **Calendar baseline** assumes **40 NoC nodes** arranged as 5 x 8 with
  ``node = r * 8 + c``, which is what fixes ``routeBits`` at 80 bit; and
  conversely
* the **whitepaper hardware chapter** assumes a **6 x 8 = 48** mesh, which
  would make ``routeBits`` 96 bit and the flit header about 14 B instead of
  about 12 B.

Both are first-class profiles in this model. Node identifiers are the decimal
node index, written ``N00`` .. ``N39`` / ``N47`` in the design documents.
"""

from __future__ import annotations

from collections.abc import Iterator
from dataclasses import dataclass

from wse_model.errors import TopologyError

__all__ = [
    "CALENDAR_BASELINE",
    "FFN_CELL_ROWS",
    "PROFILES",
    "WHITEPAPER_HARDWARE",
    "WHITEPAPER_HARDWARE_40_AICORE",
    "MeshTopology",
    "TopologyProfile",
    "node_name",
    "topology_profile",
]


def node_name(node: int) -> str:
    """Return the design-document spelling of a node id, e.g. ``N04``."""
    return f"N{node:02d}"


@dataclass(frozen=True)
class MeshTopology:
    """A rectangular 2D mesh with dimension-ordered physical adjacency.

    The mesh defines *physical* adjacency only. Which of those links a flit
    actually uses is decided entirely by the ``{P, L}`` bit pairs carried in the
    flit header (Calendar §2.2); this class never routes.
    """

    rows: int
    cols: int
    name: str = "mesh"

    def __post_init__(self) -> None:
        if self.rows < 1 or self.cols < 1:
            raise TopologyError(
                f"{self.name}: mesh dimensions must be positive, got {self.rows} x {self.cols}"
            )

    # -- shape ---------------------------------------------------------------

    @property
    def node_count(self) -> int:
        return self.rows * self.cols

    @property
    def nodes(self) -> tuple[int, ...]:
        return tuple(range(self.node_count))

    @property
    def route_bits_width_bits(self) -> int:
        """Width of ``routeBits``: two bits per NoC node (Calendar §2.2)."""
        return 2 * self.node_count

    @property
    def route_bits_width_bytes(self) -> int:
        """Byte width of ``routeBits``: 10 B at 40 nodes, 12 B at 48 (Q1)."""
        return (self.route_bits_width_bits + 7) // 8

    # -- coordinate mapping --------------------------------------------------

    def node_at(self, row: int, col: int) -> int:
        """Map ``(row, col)`` to a node id using ``node = row * cols + col``."""
        if not (0 <= row < self.rows and 0 <= col < self.cols):
            raise TopologyError(
                f"{self.name}: coordinate ({row}, {col}) is outside the "
                f"{self.rows} x {self.cols} mesh"
            )
        return row * self.cols + col

    def coords(self, node: int) -> tuple[int, int]:
        """Map a node id back to ``(row, col)``."""
        self._check_node(node)
        return divmod(node, self.cols)

    def row_of(self, node: int) -> int:
        return self.coords(node)[0]

    def col_of(self, node: int) -> int:
        return self.coords(node)[1]

    def _check_node(self, node: int) -> None:
        if not 0 <= node < self.node_count:
            raise TopologyError(f"{self.name}: node {node} is outside 0..{self.node_count - 1}")

    # -- adjacency -----------------------------------------------------------

    def neighbours(self, node: int) -> tuple[int, ...]:
        """Physical neighbours of ``node``, in ascending node-id order."""
        row, col = self.coords(node)
        out = []
        if row > 0:
            out.append(self.node_at(row - 1, col))
        if col > 0:
            out.append(self.node_at(row, col - 1))
        if col + 1 < self.cols:
            out.append(self.node_at(row, col + 1))
        if row + 1 < self.rows:
            out.append(self.node_at(row + 1, col))
        return tuple(sorted(out))

    def is_adjacent(self, a: int, b: int) -> bool:
        return b in self.neighbours(a)

    def links(self) -> Iterator[tuple[int, int]]:
        """Yield each undirected physical link once, as ``(low, high)``."""
        for node in self.nodes:
            for peer in self.neighbours(node):
                if node < peer:
                    yield (node, peer)

    def induced_edges(self, nodes: frozenset[int] | set[int]) -> list[tuple[int, int]]:
        """Physical links whose *both* endpoints are in ``nodes``.

        This is the induced subgraph the Calendar rule ② actually uses: every
        hop copies a flit to *all* physical neighbours with ``P = 1``, so any
        two adjacent members of the ``{P=1}`` set share a live link whether or
        not the algorithm intended it (Calendar §2.2).
        """
        member = set(nodes)
        return [(a, b) for a, b in self.links() if a in member and b in member]

    # -- groups --------------------------------------------------------------

    def row_group(self, row: int) -> tuple[int, ...]:
        return tuple(self.node_at(row, col) for col in range(self.cols))

    def col_group(self, col: int) -> tuple[int, ...]:
        return tuple(self.node_at(row, col) for row in range(self.rows))

    def describe(self) -> dict[str, object]:
        return {
            "name": self.name,
            "rows": self.rows,
            "cols": self.cols,
            "node_count": self.node_count,
            "route_bits_bits": self.route_bits_width_bits,
            "route_bits_bytes": self.route_bits_width_bytes,
        }


@dataclass(frozen=True)
class TopologyProfile:
    """A mesh plus its AICORE / I/O node assignment.

    ``Q1`` also asks whether a 6 x 8 mesh carries 48 AICOREs or 40 AICOREs plus
    8 I/O nodes. The profile keeps the mesh (which sizes ``routeBits``) separate
    from the core assignment (which sizes the collective member sets).
    """

    name: str
    topology: MeshTopology
    aicore_nodes: frozenset[int]
    io_nodes: frozenset[int]
    description: str
    open_item: str | None = "Q1"

    def __post_init__(self) -> None:
        overlap = self.aicore_nodes & self.io_nodes
        if overlap:
            raise TopologyError(f"{self.name}: nodes {sorted(overlap)} are both AICORE and I/O")
        covered = self.aicore_nodes | self.io_nodes
        expected = frozenset(self.topology.nodes)
        if covered != expected:
            missing = sorted(expected - covered)
            extra = sorted(covered - expected)
            raise TopologyError(
                f"{self.name}: node assignment does not cover the mesh "
                f"(missing {missing}, unknown {extra})"
            )

    @property
    def aicore_count(self) -> int:
        return len(self.aicore_nodes)

    def describe(self) -> dict[str, object]:
        return {
            **self.topology.describe(),
            "profile": self.name,
            "aicore_count": self.aicore_count,
            "io_count": len(self.io_nodes),
            "description": self.description,
            "open_item": self.open_item,
        }


#: Calendar §1 and §2 baseline: 40 nodes, 5 x 8, every node carries an AICORE.
CALENDAR_BASELINE = TopologyProfile(
    name="calendar-40",
    topology=MeshTopology(rows=5, cols=8, name="mesh-5x8"),
    aicore_nodes=frozenset(range(40)),
    io_nodes=frozenset(),
    description=(
        "Calendar baseline: 40 NoC nodes as a 5 x 8 mesh, node = r * 8 + c. "
        "Fixes routeBits at 80 bit."
    ),
)

#: Whitepaper §2 and §5: 6 x 8 mesh, all 48 positions carrying an AICORE.
WHITEPAPER_HARDWARE = TopologyProfile(
    name="whitepaper-48",
    topology=MeshTopology(rows=6, cols=8, name="mesh-6x8"),
    aicore_nodes=frozenset(range(48)),
    io_nodes=frozenset(),
    description=(
        "Whitepaper hardware chapter: 6 x 8 mesh with 48 AICOREs. Would make "
        "routeBits 96 bit and the flit header about 14 B."
    ),
)

#: The other reading of Q1: a 6 x 8 mesh where only 40 positions have an AICORE.
WHITEPAPER_HARDWARE_40_AICORE = TopologyProfile(
    name="whitepaper-48-40c",
    topology=MeshTopology(rows=6, cols=8, name="mesh-6x8"),
    aicore_nodes=frozenset(range(40)),
    io_nodes=frozenset(range(40, 48)),
    description=(
        "Q1 alternate reading: 6 x 8 mesh, 40 positions with an AICORE and 8 "
        "I/O positions (nominal 96 bit routeBits, 40 collective members)."
    ),
)

PROFILES: dict[str, TopologyProfile] = {
    profile.name: profile
    for profile in (
        CALENDAR_BASELINE,
        WHITEPAPER_HARDWARE,
        WHITEPAPER_HARDWARE_40_AICORE,
    )
}

#: The FFN example uses the first four rows, i.e. 32 cells, of the 5 x 8 mesh.
FFN_CELL_ROWS = 4
FFN_CELLS = frozenset(range(FFN_CELL_ROWS * CALENDAR_BASELINE.topology.cols))


def topology_profile(name: str) -> TopologyProfile:
    """Look up a profile by name, raising :class:`TopologyError` if unknown."""
    try:
        return PROFILES[name]
    except KeyError:
        known = ", ".join(sorted(PROFILES))
        raise TopologyError(f"unknown topology profile {name!r}; known profiles: {known}") from None
