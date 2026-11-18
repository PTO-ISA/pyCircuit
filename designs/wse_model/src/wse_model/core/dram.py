"""Per-core local DRAM: 384 MB at 1 TB/s with a 2 KB page granularity.

Whitepaper §3.2 and §3.4: each AICORE owns its local DRAM outright, and **2 KB is
the actual read/write unit**. Anything smaller wastes bandwidth, and weight
layouts should be blocked on 2 KB boundaries.

Because the GM was removed, local DRAM is also the only place a core's weights
and large tensors live, so its capacity is the hard limit that decides how many
experts fit on one core (whitepaper §17.3). The weight layout itself, and whether
the fill into L0B goes through L1, are open item ``Q8``; the model therefore
takes the layout as a declaration rather than assuming one.
"""

from __future__ import annotations

from dataclasses import dataclass

from wse_model.analysis.platform import (
    LOCAL_DRAM_BYTES,
    LOCAL_DRAM_BYTES_PER_SEC,
    LOCAL_DRAM_PAGE_BYTES,
)
from wse_model.errors import OpenItemError, WseModelError

__all__ = ["LOCAL_DRAM_PAGE_BYTES", "LocalDram", "LocalDramLayout"]


@dataclass(frozen=True)
class LocalDramLayout:
    """How a tensor is blocked in local DRAM (open item ``Q8``).

    The design states the 2 KB page rule but not how a weight matrix is laid out
    or how the fill reaches L0B. This declaration makes the choice explicit
    instead of baking one in.
    """

    row_stride_bytes: int
    page_aligned: bool = True
    note: str = "blocked on 2 KB pages as whitepaper §3.4 requires"

    def __post_init__(self) -> None:
        if self.row_stride_bytes <= 0:
            raise WseModelError("row_stride_bytes must be positive")

    def check(self) -> None:
        if self.page_aligned and self.row_stride_bytes % LOCAL_DRAM_PAGE_BYTES != 0:
            raise WseModelError(
                f"row stride {self.row_stride_bytes} is not a multiple of the "
                f"{LOCAL_DRAM_PAGE_BYTES} B page; a sub-page random access wastes "
                "bandwidth, so weight layouts must be 2 KB blocked "
                "(whitepaper §3.4)"
            )

    def require_fill_path(self) -> None:
        """Whether the fill into L0B goes through L1 is open item ``Q8``."""
        raise OpenItemError(
            "the local-DRAM-to-L0B fill path (direct MTE, or staged through L1) "
            "is open item Q8 (whitepaper Appendix A.2) and has no value in the "
            "design sources; supply it as a declared input before timing a fill"
        )

    def describe(self) -> dict[str, object]:
        return {
            "row_stride_bytes": self.row_stride_bytes,
            "page_aligned": self.page_aligned,
            "page_bytes": LOCAL_DRAM_PAGE_BYTES,
            "note": self.note,
        }


@dataclass(frozen=True)
class LocalDram:
    """One core's private memory."""

    capacity_bytes: int = LOCAL_DRAM_BYTES
    bandwidth_bytes_per_sec: float = LOCAL_DRAM_BYTES_PER_SEC
    page_bytes: int = LOCAL_DRAM_PAGE_BYTES

    @property
    def gib(self) -> float:
        return self.capacity_bytes / (1024**3)

    def pages_touched(self, nbytes: int, *, aligned: bool = True) -> int:
        """2 KB pages a transfer of ``nbytes`` touches.

        An unaligned transfer can straddle a page boundary, which is why the
        design makes alignment a layout rule rather than a hint.
        """
        if nbytes < 0:
            raise WseModelError("byte count must be non-negative")
        if nbytes == 0:
            return 0
        if aligned:
            return (nbytes + self.page_bytes - 1) // self.page_bytes
        return nbytes // self.page_bytes + 2

    def pages_for_rows(self, *, rows: int, row_bytes: int, row_stride_bytes: int) -> int:
        """Pages a row-blocked tensor spans, including the padding rows cost."""
        if rows < 0 or row_bytes < 0 or row_stride_bytes <= 0:
            raise WseModelError("invalid tensor geometry")
        if row_bytes > row_stride_bytes:
            raise WseModelError(f"row_bytes {row_bytes} exceeds row stride {row_stride_bytes}")
        span = (rows - 1) * row_stride_bytes + row_bytes if rows else 0
        return self.pages_touched(span)

    def read_seconds(self, nbytes: int) -> float:
        """Time to move ``nbytes`` at the per-core DRAM bandwidth."""
        if nbytes < 0:
            raise WseModelError("byte count must be non-negative")
        return nbytes / self.bandwidth_bytes_per_sec

    def bandwidth_efficiency(self, *, rows: int, row_bytes: int, row_stride_bytes: int) -> float:
        """Useful bytes over fetched bytes for a row-blocked access.

        The padding between ``row_bytes`` and ``row_stride_bytes`` is fetched but
        unused, which is the concrete meaning of "2 KB is the real granularity".
        """
        if rows <= 0 or row_bytes <= 0:
            return 0.0
        useful = rows * row_bytes
        fetched = rows * row_stride_bytes
        return useful / fetched

    def expert_capacity(self, *, expert_bytes: int) -> int:
        """How many whole experts of ``expert_bytes`` fit (whitepaper §17.3)."""
        if expert_bytes <= 0:
            raise WseModelError("expert_bytes must be positive")
        return self.capacity_bytes // expert_bytes

    def describe(self) -> dict[str, object]:
        return {
            "capacity_bytes": self.capacity_bytes,
            "capacity_gib": round(self.gib, 3),
            "bandwidth_bytes_per_sec": self.bandwidth_bytes_per_sec,
            "bandwidth_GB_per_s": self.bandwidth_bytes_per_sec / 1e9,
            "page_bytes": self.page_bytes,
        }
