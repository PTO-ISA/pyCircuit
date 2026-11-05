"""The five-block latency budget of one WSE coprocessor call.

Whitepaper §19.1 splits a call into five blocks, each with a different owner and
a different amount of give:

============================  ====================================  ==================
block                         limited by                            elasticity
============================  ====================================  ==================
device-to-device transfer     UB bus, 224 GB/s                      only via granularity
Batcher dispatch / gather     Batcher.mem + async boundary           partial by pipelining
on-core compute               local DRAM, 1 TB/s                     lower precision
inter-core communication      NoC 128 GB/s/link + alignment          partition choice
tail synchronisation          the slowest core                       load balance
============================  ====================================  ==================

Two of the five cannot be computed from the design sources: the Batcher
specification is open item ``Q3`` (Batcher.mem capacity and bandwidth, cores per
Batcher, parallel dispatch channels) and the Davinci/WSE-Lite interface's
round-trip latency is open item ``Q9``. The model therefore treats them as
declared inputs and refuses to produce a total without them, rather than
substituting a plausible number.
"""

from __future__ import annotations

from dataclasses import dataclass

from wse_model.analysis.bandwidth import BANDWIDTHS
from wse_model.errors import OpenItemError, WseModelError
from wse_model.open_items import require_resolved

__all__ = ["LatencyBlock", "LatencyBudget"]


@dataclass(frozen=True)
class LatencyBlock:
    """One term of the latency budget."""

    name: str
    seconds: float
    limited_by: str
    elasticity: str
    declared: bool = False

    def describe(self) -> dict[str, object]:
        return {
            "block": self.name,
            "seconds": self.seconds,
            "limited_by": self.limited_by,
            "elasticity": self.elasticity,
            "declared": self.declared,
        }


@dataclass
class LatencyBudget:
    """Accumulates the five blocks for one coprocessor call.

    The three computable blocks are filled in from declared volumes. The two
    open ones are optional here and required by :meth:`total`.
    """

    #: Activation bytes crossing the device boundary in each direction.
    device_transfer_bytes: int = 0
    #: Bytes the Batcher must dispatch inward and gather back.
    batcher_bytes: int = 0
    #: Weight bytes the core reads from local DRAM.
    compute_weight_bytes: int = 0
    #: Payload bytes crossing the NoC.
    noc_bytes: int = 0
    #: Declared Batcher dispatch + gather time, from open item Q3.
    batcher_seconds: float | None = None
    #: Declared device round-trip time, from open item Q9.
    round_trip_seconds: float | None = None
    #: Declared tail-synchronisation cost (the slowest core's excess).
    tail_seconds: float = 0.0

    def blocks(self) -> tuple[LatencyBlock, ...]:
        """The computable blocks. Raises for the two open ones."""
        device = BANDWIDTHS["ub_fabric"].seconds_for(self.device_transfer_bytes)
        compute_bandwidth = BANDWIDTHS["local_dram"]
        compute = compute_bandwidth.seconds_for(self.compute_weight_bytes)
        noc = BANDWIDTHS["noc_link"].seconds_for(self.noc_bytes)
        if self.batcher_seconds is None:
            require_resolved("Q3", consumer="LatencyBudget.blocks")
        if self.round_trip_seconds is None:
            require_resolved("Q9", consumer="LatencyBudget.blocks")
        return (
            LatencyBlock(
                "device-to-device transfer",
                device + (self.round_trip_seconds or 0.0),
                "UB bus 224 GB/s plus the Davinci/WSE-Lite round trip (Q9)",
                "only by enlarging the operator granularity",
                declared=self.round_trip_seconds is not None,
            ),
            LatencyBlock(
                "Batcher dispatch / gather",
                self.batcher_seconds or 0.0,
                "Batcher.mem bandwidth and the asynchronous boundary (Q3)",
                "partly hidable by pipelining",
                declared=True,
            ),
            LatencyBlock(
                "on-core compute",
                compute,
                "local DRAM 1 TB/s",
                "lower precision halves or quarters the bytes",
            ),
            LatencyBlock(
                "inter-core communication",
                noc,
                "NoC 128 GB/s per link plus alignment cost",
                "partition choice; Calendar staggering",
            ),
            LatencyBlock(
                "tail synchronisation",
                self.tail_seconds,
                "the slowest core",
                "load balance; avoid expert skew",
            ),
        )

    def total_seconds(self) -> float:
        """Sum the five blocks, requiring the two open inputs."""
        if self.batcher_seconds is None:
            raise OpenItemError(
                "the Batcher dispatch/gather time is open item Q3 (Batcher.mem "
                "capacity and bandwidth, cores per Batcher, parallel channels) "
                "and was not supplied; no total is produced"
            )
        if self.round_trip_seconds is None:
            raise OpenItemError(
                "the Davinci/WSE-Lite round-trip latency is open item Q9 and was "
                "not supplied; no total is produced"
            )
        return sum(block.seconds for block in self.blocks())

    def dominant_block(self) -> LatencyBlock:
        blocks = self.blocks()
        if not blocks:
            raise WseModelError("empty latency budget")
        return max(blocks, key=lambda block: block.seconds)

    def describe(self) -> dict[str, object]:
        try:
            blocks = self.blocks()
        except OpenItemError as exc:
            return {
                "complete": False,
                "reason": str(exc),
                "declared_inputs": {
                    "batcher_seconds": self.batcher_seconds,
                    "round_trip_seconds": self.round_trip_seconds,
                    "tail_seconds": self.tail_seconds,
                },
                "volumes": {
                    "device_transfer_bytes": self.device_transfer_bytes,
                    "batcher_bytes": self.batcher_bytes,
                    "compute_weight_bytes": self.compute_weight_bytes,
                    "noc_bytes": self.noc_bytes,
                },
            }
        return {
            "complete": True,
            "blocks": [block.describe() for block in blocks],
            "total_seconds": sum(block.seconds for block in blocks),
        }
