"""The Batcher: the device's only external entry point.

Whitepaper §6.1 and §6.2: every byte that enters the AICORE array passes through
the Batcher, and every result is written back through it. It carries four
responsibilities at three different frequencies, and ``Batcher.mem`` plays two
roles at once — data-plane staging **and** the refill source for instruction and
constant cache misses.

That second role is easy to overlook and it is what makes the D-cache account
cheap: 40 cores read the *same* DDR image, so the shared ``Batcher.mem`` absorbs
all but one request per distinct line. The model makes that split explicit
instead of leaving it as a footnote.

``Batcher.mem``'s capacity and bandwidth, how many cores one Batcher serves, and
whether dispatch is multi-channel are open item ``Q3``; they are required inputs
here, never defaults.
"""

from __future__ import annotations

from dataclasses import dataclass
from enum import Enum

from wse_model.analysis.platform import LOCAL_DRAM_PAGE_BYTES
from wse_model.errors import WseModelError
from wse_model.open_items import require_resolved

__all__ = [
    "BATCHER_MEM_PATH",
    "INSTRUCTION_BLOCK_BYTES",
    "BatcherMem",
    "BatcherResponsibility",
    "BatcherSpec",
    "DispatchPath",
    "RefillAccount",
    "dispatch_paths",
]

#: Whitepaper §4.1: instruction cache refills arrive as 2 KB blocks.
INSTRUCTION_BLOCK_BYTES = 2048

#: Whitepaper §4.1: data cache refills arrive as 64 B lines.
DATA_CACHE_LINE_BYTES = 64

BATCHER_MEM_PATH = (
    "DDR -> Batcher.mem -> I$/D$; never through local DRAM and never through L1 "
    "(whitepaper §4.1, §13)"
)


class BatcherResponsibility(str, Enum):
    """The four things the Batcher carries (whitepaper §6.1)."""

    KERNEL_BINARY = "kernel binary (.text + .rodata, one batch)"
    WEIGHTS = "weights to each core's local DRAM"
    ACTIVATIONS = "input activations dispatched to the AICOREs"
    RESULTS = "per-core results gathered and written back"

    @property
    def direction(self) -> str:
        return "in" if self is not BatcherResponsibility.RESULTS else "out"


class DispatchFrequency(str, Enum):
    """How often a dispatch happens (whitepaper §6.1)."""

    ONCE_PER_MODEL = "once per model load"
    ONCE_PER_KERNEL = "once per kernel"
    EVERY_INFERENCE = "every inference"


@dataclass(frozen=True)
class DispatchPath:
    """One Batcher responsibility with its direction, destination, and frequency."""

    responsibility: BatcherResponsibility
    frequency: DispatchFrequency
    destination: str
    goes_through_batcher_mem: bool = True
    note: str = ""

    def describe(self) -> dict[str, object]:
        return {
            "responsibility": self.responsibility.value,
            "direction": self.responsibility.direction,
            "frequency": self.frequency.value,
            "destination": self.destination,
            "through_batcher_mem": self.goes_through_batcher_mem,
            "note": self.note,
        }


def dispatch_paths() -> tuple[DispatchPath, ...]:
    """The four dispatch paths of whitepaper §12.1."""
    return (
        DispatchPath(
            BatcherResponsibility.KERNEL_BINARY,
            DispatchFrequency.ONCE_PER_KERNEL,
            "DDR code and data segments; refilled into I$/D$ on a miss",
            note=(
                "misses go DDR -> Batcher.mem -> cache, not through local DRAM and "
                "not through L1; to the toolchain this is an ordinary dispatch"
            ),
        ),
        DispatchPath(
            BatcherResponsibility.WEIGHTS,
            DispatchFrequency.ONCE_PER_MODEL,
            "each core's local DRAM",
            note="2 KB page aligned (whitepaper §3.4); never redistributed at run time",
        ),
        DispatchPath(
            BatcherResponsibility.ACTIVATIONS,
            DispatchFrequency.EVERY_INFERENCE,
            "each core's UB / L1",
            note="the only recurring inbound traffic in steady state",
        ),
        DispatchPath(
            BatcherResponsibility.RESULTS,
            DispatchFrequency.EVERY_INFERENCE,
            "back through the Batcher to the requesting device",
            note="every coprocessor call pays this round trip, which is why the "
            "granularity must be a whole operator",
        ),
    )


@dataclass(frozen=True)
class BatcherSpec:
    """The Batcher parameters the design sources leave open (``Q3``).

    Every field defaults to ``None``, meaning *not declared*. Asking for a timing
    figure without declaring them raises rather than substituting a plausible
    number.
    """

    mem_capacity_bytes: int | None = None
    mem_bandwidth_bytes_per_sec: float | None = None
    cores_per_batcher: int | None = None
    parallel_channels: int | None = None

    @property
    def is_declared(self) -> bool:
        return all(
            value is not None
            for value in (
                self.mem_capacity_bytes,
                self.mem_bandwidth_bytes_per_sec,
                self.cores_per_batcher,
                self.parallel_channels,
            )
        )

    def require(self, *, consumer: str) -> None:
        """Refuse to produce a timing figure while ``Q3`` is undeclared."""
        if not self.is_declared:
            require_resolved("Q3", consumer=consumer)

    def dispatch_seconds(self, nbytes: int, *, consumer: str = "BatcherSpec") -> float:
        self.require(consumer=consumer)
        if nbytes < 0:
            raise WseModelError("byte count must be non-negative")
        assert self.mem_bandwidth_bytes_per_sec is not None  # narrowed by require()
        return nbytes / self.mem_bandwidth_bytes_per_sec

    def describe(self) -> dict[str, object]:
        return {
            "declared": self.is_declared,
            "open_item": "Q3",
            "mem_capacity_bytes": self.mem_capacity_bytes,
            "mem_bandwidth_bytes_per_sec": self.mem_bandwidth_bytes_per_sec,
            "cores_per_batcher": self.cores_per_batcher,
            "parallel_channels": self.parallel_channels,
        }


@dataclass(frozen=True)
class RefillAccount:
    """The split of cache refill requests between ``Batcher.mem`` and DDR."""

    distinct_lines: int
    requesters_per_line: int
    line_bytes: int

    def __post_init__(self) -> None:
        for name in ("distinct_lines", "requesters_per_line", "line_bytes"):
            if getattr(self, name) < 0:
                raise WseModelError(f"{name} must be non-negative")

    @property
    def requests(self) -> int:
        """Total refill requests: every requester misses independently."""
        return self.distinct_lines * self.requesters_per_line

    @property
    def ddr_backed_requests(self) -> int:
        """Requests that reach DDR: one per distinct line.

        This is the whole benefit of a shared ``Batcher.mem``: the cores read one
        DDR image, so the first request per line fills the memory and the rest are
        served from it.
        """
        return self.distinct_lines

    @property
    def mem_served_requests(self) -> int:
        return max(0, self.requests - self.ddr_backed_requests)

    @property
    def ddr_bytes(self) -> int:
        return self.ddr_backed_requests * self.line_bytes

    @property
    def mem_served_bytes(self) -> int:
        return self.mem_served_requests * self.line_bytes

    def describe(self) -> dict[str, object]:
        return {
            "distinct_lines": self.distinct_lines,
            "requesters_per_line": self.requesters_per_line,
            "line_bytes": self.line_bytes,
            "requests": self.requests,
            "mem_served_requests": self.mem_served_requests,
            "ddr_backed_requests": self.ddr_backed_requests,
            "mem_served_bytes": self.mem_served_bytes,
            "ddr_bytes": self.ddr_bytes,
        }


@dataclass(frozen=True)
class BatcherMem:
    """``Batcher.mem`` as both a data-plane staging buffer and a refill source."""

    spec: BatcherSpec = BatcherSpec()
    #: The counters live here because the memory is shared: a cold start with
    #: many cores missing at once costs far less than ``cores x lines``.
    _hit_lines: frozenset[int] = frozenset()

    def refill(
        self, *, distinct_lines: int, requesters_per_line: int, line_bytes: int
    ) -> RefillAccount:
        """Compute the refill split for a cache's worth of lines."""
        return RefillAccount(
            distinct_lines=distinct_lines,
            requesters_per_line=requesters_per_line,
            line_bytes=line_bytes,
        )

    def instruction_refill(
        self, *, distinct_blocks: int, requesters_per_line: int
    ) -> RefillAccount:
        """I$ refills: 2 KB blocks, shared by the cores running the kernel.

        ``requesters_per_line`` is how many cores miss each block, not the core
        count: the shared ``Batcher.mem`` means the request count is what costs,
        and the distinct-line count is what reaches DDR.
        """
        return self.refill(
            distinct_lines=distinct_blocks,
            requesters_per_line=requesters_per_line,
            line_bytes=INSTRUCTION_BLOCK_BYTES,
        )

    def data_refill(self, *, distinct_lines: int, requesters_per_line: int) -> RefillAccount:
        """D$ refills: 64 B lines, the Calendar table's path.

        The FFN example is ``distinct_lines=20`` (two keys of ten lines) with
        four cores sharing each line, so 80 requests of which only 20 reach DDR
        (Calendar §3.8.4).
        """
        return self.refill(
            distinct_lines=distinct_lines,
            requesters_per_line=requesters_per_line,
            line_bytes=DATA_CACHE_LINE_BYTES,
        )

    def capacity_check(self, *, resident_bytes: int) -> None:
        """Refuse a working set larger than a declared ``Batcher.mem``."""
        if self.spec.mem_capacity_bytes is None:
            require_resolved("Q3", consumer="BatcherMem.capacity_check")
        assert self.spec.mem_capacity_bytes is not None
        if resident_bytes > self.spec.mem_capacity_bytes:
            raise WseModelError(
                f"a {resident_bytes} B working set exceeds the declared "
                f"{self.spec.mem_capacity_bytes} B Batcher.mem"
            )

    def weight_page_check(self, *, weight_bytes: int) -> None:
        """Weights land 2 KB-page aligned (whitepaper §3.4)."""
        if weight_bytes % LOCAL_DRAM_PAGE_BYTES != 0:
            raise WseModelError(
                f"a {weight_bytes} B weight shard is not a multiple of the "
                f"{LOCAL_DRAM_PAGE_BYTES} B page; the dispatch must be page blocked"
            )

    def describe(self) -> dict[str, object]:
        return {
            "spec": self.spec.describe(),
            "instruction_block_bytes": INSTRUCTION_BLOCK_BYTES,
            "data_line_bytes": DATA_CACHE_LINE_BYTES,
            "path": BATCHER_MEM_PATH,
        }
