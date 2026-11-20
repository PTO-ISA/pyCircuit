"""The external UB bus: 8 fabric lanes and 1 host lane.

Whitepaper §6.3. Two observations from the design drive everything downstream:

* the fabric's 224 GB/s is far below a single core's 1 TB/s local DRAM, so the
  bus can carry activations but never weights — which is exactly why weights must
  reside and why the three-way partition is viable at all; and
* the host's 14 GB/s is a control-plane scale, adequate for commands and small
  metadata but never on the inference critical path.
"""

from __future__ import annotations

from dataclasses import dataclass
from enum import Enum

from wse_model.analysis.platform import (
    BATCHER_UB_FABRIC_BYTES_PER_SEC,
    BATCHER_UB_HOST_BYTES_PER_SEC,
    BATCHER_UB_LANES,
)
from wse_model.errors import WseModelError

__all__ = [
    "FABRIC_LANES",
    "HOST_LANES",
    "TOTAL_LANES",
    "UbBus",
    "UbLane",
    "local_dram_over_fabric_ratio",
]

#: Whitepaper §6.3: 8 fabric lanes, 1 host lane, 9 lanes total.
FABRIC_LANES = BATCHER_UB_LANES
HOST_LANES = 1
TOTAL_LANES = FABRIC_LANES + HOST_LANES


class UbLane(str, Enum):
    """Which side of the bus a transfer uses."""

    FABRIC = "fabric"
    HOST = "host"

    @property
    def lanes(self) -> int:
        return FABRIC_LANES if self is UbLane.FABRIC else HOST_LANES

    @property
    def bytes_per_sec(self) -> float:
        return (
            BATCHER_UB_FABRIC_BYTES_PER_SEC
            if self is UbLane.FABRIC
            else BATCHER_UB_HOST_BYTES_PER_SEC
        )

    @property
    def gb_per_s(self) -> float:
        return self.bytes_per_sec / 1e9


@dataclass(frozen=True)
class UbBus:
    """The external bus, with a lane-budget check."""

    fabric_lanes: int = FABRIC_LANES
    host_lanes: int = HOST_LANES

    def __post_init__(self) -> None:
        if self.fabric_lanes < 0 or self.host_lanes < 0:
            raise WseModelError("lane counts must be non-negative")
        if self.fabric_lanes + self.host_lanes > TOTAL_LANES:
            raise WseModelError(
                f"the UB bus has {TOTAL_LANES} lanes but "
                f"{self.fabric_lanes} fabric + {self.host_lanes} host were requested"
            )

    def bandwidth(self, lane: UbLane) -> float:
        return lane.bytes_per_sec

    def transfer_seconds(self, nbytes: int, *, lane: UbLane = UbLane.FABRIC) -> float:
        if nbytes < 0:
            raise WseModelError("byte count must be non-negative")
        return nbytes / self.bandwidth(lane)

    def bytes_in(self, seconds: float, *, lane: UbLane = UbLane.FABRIC) -> float:
        if seconds < 0:
            raise WseModelError("seconds must be non-negative")
        return self.bandwidth(lane) * seconds

    def weights_can_reside_only(self, *, weight_bytes: int, local_dram_seconds: float) -> bool:
        """Whether crossing the bus would dominate reading the same weights locally.

        The design's premise: when the bus time exceeds the local read time, the
        weights must reside rather than be streamed.
        """
        return self.transfer_seconds(weight_bytes) > local_dram_seconds

    def describe(self) -> dict[str, object]:
        return {
            "fabric_lanes": self.fabric_lanes,
            "host_lanes": self.host_lanes,
            "total_lanes": self.fabric_lanes + self.host_lanes,
            "fabric_GB_per_s": UbLane.FABRIC.gb_per_s,
            "host_GB_per_s": UbLane.HOST.gb_per_s,
        }


def local_dram_over_fabric_ratio(*, local_dram_bytes_per_sec: float = 1.0e12) -> float:
    """Per-core local DRAM bandwidth over the fabric bus (about 4.5x)."""
    return local_dram_bytes_per_sec / UbLane.FABRIC.bytes_per_sec
