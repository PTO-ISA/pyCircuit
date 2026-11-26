"""The on-chip buffer hierarchy and its fit checks.

Whitepaper §3.2 gives the hierarchy and its one asymmetry: **L0B is 512 KB, four
times L0A**, precisely so more weights can reside and be reused. The Cube reads
its two operands from L0A and L0B, writes accumulators to L0C, L1 is the large
on-chip staging buffer, and UB is the Vector workspace — which is also the
candidate location for the collective arena (open item ``Q7``/``S-2``).

The model exposes capacity and fit arithmetic only. Which level the arena lands
in, and whether a weight fill stages through L1, are open items; consumers must
declare them.
"""

from __future__ import annotations

from dataclasses import dataclass
from enum import Enum

from wse_model.analysis.platform import ON_CHIP_BUFFERS, UB_BYTES
from wse_model.errors import OpenItemError, WseModelError

__all__ = [
    "BUFFER_CAPACITY_BYTES",
    "BufferKind",
    "BufferResidency",
    "TileRequirement",
    "fit_check",
]

#: The hierarchy as a mapping, straight from whitepaper §3.2.
BUFFER_CAPACITY_BYTES: dict[str, int] = dict(ON_CHIP_BUFFERS)


class BufferKind(str, Enum):
    """The on-chip levels the design names."""

    L0A = "L0A"
    L0B = "L0B"
    L0C = "L0C"
    L1 = "L1"
    UB = "UB"

    @property
    def capacity_bytes(self) -> int:
        return BUFFER_CAPACITY_BYTES[self.value]

    @property
    def is_cube_operand(self) -> bool:
        return self in (BufferKind.L0A, BufferKind.L0B)


@dataclass(frozen=True)
class TileRequirement:
    """A tensor that must reside in one buffer level."""

    name: str
    kind: BufferKind
    bytes: int
    double_buffered: bool = False

    def __post_init__(self) -> None:
        if not self.name:
            raise WseModelError("TileRequirement.name must be non-empty")
        if self.bytes < 0:
            raise WseModelError(f"{self.name}: negative size")

    @property
    def required_bytes(self) -> int:
        """Double buffering doubles the footprint, which is the cost of overlap."""
        return self.bytes * (2 if self.double_buffered else 1)

    @property
    def capacity_bytes(self) -> int:
        return self.kind.capacity_bytes

    @property
    def fits(self) -> bool:
        return self.required_bytes <= self.capacity_bytes

    @property
    def occupancy(self) -> float:
        if self.capacity_bytes == 0:
            return 0.0
        return self.required_bytes / self.capacity_bytes

    def describe(self) -> dict[str, object]:
        return {
            "name": self.name,
            "buffer": self.kind.value,
            "bytes": self.bytes,
            "double_buffered": self.double_buffered,
            "required_bytes": self.required_bytes,
            "capacity_bytes": self.capacity_bytes,
            "fits": self.fits,
            "occupancy_percent": round(100.0 * self.occupancy, 2),
        }


@dataclass(frozen=True)
class BufferResidency:
    """The per-level residency of a whole kernel's tiles."""

    requirements: tuple[TileRequirement, ...]

    @property
    def fits(self) -> bool:
        return all(requirement.fits for requirement in self.requirements)

    @property
    def overflows(self) -> tuple[TileRequirement, ...]:
        return tuple(item for item in self.requirements if not item.fits)

    def total_for(self, kind: BufferKind) -> int:
        return sum(item.required_bytes for item in self.requirements if item.kind is kind)

    def arena_placement(self) -> None:
        """The arena's level is open item ``Q7``/``S-2``."""
        raise OpenItemError(
            "whether the collective arena lives in UB or L1 is open item Q7 "
            "(whitepaper Appendix A.2, Calendar S-2); it decides the tile's "
            "address-space qualifier and the double-buffer budget, so it must be "
            "declared rather than assumed"
        )

    def describe(self) -> dict[str, object]:
        return {
            "fits": self.fits,
            "levels": {
                kind.value: {
                    "capacity_bytes": kind.capacity_bytes,
                    "required_bytes": self.total_for(kind),
                }
                for kind in BufferKind
            },
            "tiles": [item.describe() for item in self.requirements],
            "overflows": [item.name for item in self.overflows],
        }


def fit_check(*requirements: TileRequirement) -> BufferResidency:
    """Check a set of tiles against the hierarchy."""
    return BufferResidency(requirements=tuple(requirements))


def ub_bytes() -> int:
    """The intra-core Unified Buffer size, 384 KB (whitepaper §3.2)."""
    return UB_BYTES
