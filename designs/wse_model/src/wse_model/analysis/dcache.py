"""The D-cache budget and cold-miss accounting for the Calendar table.

Calendar §3.8 answers the obvious objection to putting a die-wide route table in
``.rodata``: does every core end up streaming the whole table? It does not, for
two reasons the design states precisely:

1. a D-cache line is allocated **only** where a load actually names an address,
   so untouched bytes stay in DDR; and
2. the index a core uses (``blockId``) is a constant for the whole kernel, so the
   address set a core touches is enumerable at compile time: exactly two loads
   per key, both inside the same 64 B line.

The one real amplification is **4x**, and it is fixed by the entry size rather
than the node count: a 64 B line holds four 16 B entries for the cells
``4k..4k+3``, and a core uses one of them, so 48 B per line is a neighbour's.
Transposing the table to node-major removes it, which is open item ``C-9``.

The shared ``Batcher.mem`` absorbs most of the die-wide miss traffic: 40 cores
read the **same** DDR image, so only one refill per distinct line ever reaches
DDR.

One caveat the design records explicitly (§3.8.4): the compile framework's
``DataCacheCleanAndInvalid`` at an operator's tail invalidates the Calendar
lines too, so the account must be "one cold miss per operator", not "one per
kernel". The table is never dirty, so the invalidate needs no clean half.
"""

from __future__ import annotations

from dataclasses import dataclass

from wse_model.calendar.entry import ENTRY_SIZE_BYTES
from wse_model.calendar.table import TableLayout
from wse_model.calendar.validate import D_CACHE_LINE_BYTES, MAX_KEY_COUNT
from wse_model.errors import WseModelError

__all__ = ["DCacheSpec", "RouteTableResidency", "dcache_budget"]


@dataclass(frozen=True)
class DCacheSpec:
    """The per-core D-cache as the design describes it (Calendar §3.8.4)."""

    capacity_bytes: int = 16 * 1024
    line_bytes: int = D_CACHE_LINE_BYTES

    @property
    def lines(self) -> int:
        """256 lines at 16 KB / 64 B."""
        return self.capacity_bytes // self.line_bytes

    @property
    def entries_per_line(self) -> int:
        """Four 16 B route entries share one 64 B line."""
        return self.line_bytes // ENTRY_SIZE_BYTES

    def describe(self) -> dict[str, object]:
        return {
            "capacity_bytes": self.capacity_bytes,
            "line_bytes": self.line_bytes,
            "lines": self.lines,
            "entries_per_line": self.entries_per_line,
        }


@dataclass(frozen=True)
class RouteTableResidency:
    """What one compiled route table costs the D-cache.

    All quantities are per kernel launch; multiply the cold-miss figure by the
    number of operators in the kernel when ``tail_invalidate`` is set.
    """

    key_count: int
    node_count: int
    layout: TableLayout = TableLayout.KEY_MAJOR
    spec: DCacheSpec = DCacheSpec()

    def __post_init__(self) -> None:
        if self.key_count < 0 or self.node_count < 0:
            raise WseModelError("keyCount and nodeCount must be non-negative")

    @property
    def lines_per_key(self) -> int:
        """Distinct lines one key's full bitmap occupies."""
        return (
            self.node_count * ENTRY_SIZE_BYTES + self.spec.line_bytes - 1
        ) // self.spec.line_bytes

    @property
    def lines_per_core(self) -> int:
        """Resident lines for one core, which is what the budget caps.

        Key-major scatters a core's entries one per key row (640 B apart at 40
        nodes), so it residents ``keyCount`` lines. Node-major keeps them
        contiguous, so they share ``ceil(keyCount / 4)`` lines.
        """
        if self.layout is TableLayout.NODE_MAJOR:
            return (
                self.key_count * ENTRY_SIZE_BYTES + self.spec.line_bytes - 1
            ) // self.spec.line_bytes
        return self.key_count

    @property
    def bytes_per_core(self) -> int:
        return self.lines_per_core * self.spec.line_bytes

    @property
    def lines_die_wide(self) -> int:
        """Distinct lines in the whole table: ``keyCount`` key rows."""
        return self.key_count * self.lines_per_key

    @property
    def refill_requests(self) -> int:
        """Line refill requests from every core: ``cores x lines_per_core``.

        Each line is shared by ``entries_per_line`` cores, and every one of them
        misses it independently.
        """
        return self.node_count * self.lines_per_core

    @property
    def ddr_backed_refills(self) -> int:
        """Refills that actually reach DDR: one per distinct line."""
        return self.lines_die_wide

    @property
    def batcher_served_refills(self) -> int:
        """Refills absorbed by the shared ``Batcher.mem``."""
        return max(0, self.refill_requests - self.ddr_backed_refills)

    @property
    def amplification(self) -> float:
        """Inherent 4x: a core uses one of the four entries in its line."""
        return float(self.spec.entries_per_line)

    @property
    def residency_fraction(self) -> float:
        """Share of the D-cache the table occupies for one core."""
        if self.spec.lines == 0:
            return 0.0
        return self.lines_per_core / self.spec.lines

    def within_budget(self) -> bool:
        """The design's hard check: per-core residency must not exceed keyCount lines."""
        return self.lines_per_core <= MAX_KEY_COUNT

    def describe(self) -> dict[str, object]:
        return {
            "key_count": self.key_count,
            "node_count": self.node_count,
            "layout": self.layout.value,
            "d_cache": self.spec.describe(),
            "lines_per_key": self.lines_per_key,
            "lines_per_core": self.lines_per_core,
            "bytes_per_core": self.bytes_per_core,
            "residency_percent": round(100.0 * self.residency_fraction, 4),
            "lines_die_wide": self.lines_die_wide,
            "refill_requests": self.refill_requests,
            "batcher_served_refills": self.batcher_served_refills,
            "ddr_backed_refills": self.ddr_backed_refills,
            "amplification": self.amplification,
            "within_budget": self.within_budget(),
            "cold_miss_account": (
                "one refill per distinct line per operator, because the "
                "framework's tail DataCacheCleanAndInvalid invalidates the "
                "Calendar lines too (Calendar §3.8.4)"
            ),
        }


def dcache_budget(
    *,
    key_count: int,
    node_count: int,
    layout: TableLayout = TableLayout.KEY_MAJOR,
    spec: DCacheSpec | None = None,
) -> RouteTableResidency:
    """Build the residency account for one table."""
    return RouteTableResidency(
        key_count=key_count,
        node_count=node_count,
        layout=layout,
        spec=spec or DCacheSpec(),
    )
