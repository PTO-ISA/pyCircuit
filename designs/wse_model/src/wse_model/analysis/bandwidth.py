"""Bandwidth arithmetic and the cost of a stateless NoC.

Whitepaper §4.4 gives the four bandwidths that constrain every partitioning
decision:

======================  ==============  ==========================================
link                    bandwidth       note
======================  ==============  ==========================================
local DRAM (per core)   1 TB/s          the specification
NoC link, one direction 128 GB/s        64 B x 2 GHz
UB bus, fabric          224 GB/s        8 x 224 Gbps
UB bus, host CPU        14 GB/s         1 x 112 Gbps
======================  ==============  ==========================================

The load-bearing observation is that **per-core local DRAM bandwidth is about
eight times one NoC link**. Reading weights on core is cheap; moving anything of
weight scale between cores cancels WSE's advantage. That is the whole reason the
FFN partition slices the N (output) axis — the collective then carries
activations, not weights.

The other cost is the flit header. Because the NoC is stateless, every flit
carries the full ``{routeBits, opcode, redOp, epochTag}`` header, about 12 B of a
64 B link at 40 nodes — roughly 19% (whitepaper §5.4, Calendar §1.8).
"""

from __future__ import annotations

from dataclasses import dataclass

from wse_model.analysis.platform import (
    BATCHER_UB_FABRIC_BYTES_PER_SEC,
    BATCHER_UB_HOST_BYTES_PER_SEC,
    LOCAL_DRAM_BYTES_PER_SEC,
    NOC_CLOCK_HZ,
    NOC_LINK_BYTES_PER_SEC,
    NOC_LINK_WIDTH_BYTES,
)
from wse_model.calendar.route_bits import RouteBits
from wse_model.errors import WseModelError
from wse_model.noc.flit import (
    DEFAULT_EPOCH_TAG_BITS,
    DEFAULT_LINK_WIDTH_BYTES,
    FlitHeader,
    flit_count,
)

__all__ = [
    "Bandwidth",
    "BANDWIDTHS",
    "collective_transfer_seconds",
    "flit_header_cost",
    "link_time_seconds",
    "local_dram_vs_noc_ratio",
    "two_phase_allgather_bytes",
]

_GBPS = 1e9


@dataclass(frozen=True)
class Bandwidth:
    """A named link bandwidth in bytes per second."""

    name: str
    bytes_per_sec: float
    source: str

    @property
    def gb_per_s(self) -> float:
        return self.bytes_per_sec / _GBPS

    def seconds_for(self, nbytes: float) -> float:
        if self.bytes_per_sec <= 0:
            raise WseModelError(f"{self.name}: non-positive bandwidth")
        return nbytes / self.bytes_per_sec

    def bytes_in(self, seconds: float) -> float:
        return self.bytes_per_sec * seconds

    def describe(self) -> dict[str, object]:
        return {
            "name": self.name,
            "bytes_per_sec": self.bytes_per_sec,
            "GB_per_s": round(self.gb_per_s, 3),
            "source": self.source,
        }


BANDWIDTHS: dict[str, Bandwidth] = {
    "local_dram": Bandwidth("local_dram", LOCAL_DRAM_BYTES_PER_SEC, "whitepaper §1.2 / §4.4"),
    "noc_link": Bandwidth(
        "noc_link",
        NOC_LINK_BYTES_PER_SEC,
        "whitepaper §4.4: 64 B x 2 GHz",
    ),
    "ub_fabric": Bandwidth(
        "ub_fabric",
        BATCHER_UB_FABRIC_BYTES_PER_SEC,
        "whitepaper §6.3: 8 x 224 Gbps = 224 GB/s",
    ),
    "ub_host": Bandwidth(
        "ub_host",
        BATCHER_UB_HOST_BYTES_PER_SEC,
        "whitepaper §6.3: 1 x 112 Gbps = 14 GB/s",
    ),
}


def local_dram_vs_noc_ratio() -> float:
    """Per-core local DRAM bandwidth over one NoC link, about 8x."""
    return LOCAL_DRAM_BYTES_PER_SEC / NOC_LINK_BYTES_PER_SEC


def link_time_seconds(nbytes: float, *, link: str = "noc_link") -> float:
    """Seconds to move ``nbytes`` over one link."""
    try:
        bandwidth = BANDWIDTHS[link]
    except KeyError:
        known = ", ".join(sorted(BANDWIDTHS))
        raise WseModelError(f"unknown link {link!r}; known: {known}") from None
    return bandwidth.seconds_for(nbytes)


def flit_header_cost(
    *,
    node_count: int,
    epoch_tag_bits: int = DEFAULT_EPOCH_TAG_BITS,
    link_width_bytes: int = DEFAULT_LINK_WIDTH_BYTES,
) -> FlitHeader:
    """Build the header for a topology so its overhead can be inspected.

    At 40 nodes the header is 12 B and the overhead is 18.75%; at 48 nodes it is
    14 B, about 21.9% — the concrete cost of the two readings of open item ``Q1``.
    """
    return FlitHeader(
        route_bits=RouteBits.from_int(0, node_count),
        opcode=1,
        epoch_tag=1,
        epoch_tag_bits=epoch_tag_bits,
        link_width_bytes=link_width_bytes,
    )


def collective_transfer_seconds(
    *,
    payload_bytes: int,
    tree_edges: int,
    destination_count: int,
    node_count: int,
    epoch_tag_bits: int = DEFAULT_EPOCH_TAG_BITS,
    link_width_bytes: int = DEFAULT_LINK_WIDTH_BYTES,
    bandwidth_bytes_per_sec: float = NOC_LINK_BYTES_PER_SEC,
) -> float:
    """Wire time for one source's multicast of ``payload_bytes``.

    ``tree_edges`` is the number of link traversals the source's tree expands to
    (``flit_hops`` per flit before replication); ``destination_count`` is how many
    landings the payload reaches. The result is a lower bound on a *single*
    link's occupancy, which is the quantity Calendar's timeslot staggering is
    trying to keep bounded.
    """
    header = flit_header_cost(
        node_count=node_count,
        epoch_tag_bits=epoch_tag_bits,
        link_width_bytes=link_width_bytes,
    )
    flits = flit_count(payload_bytes, header)
    wire_bytes = flits * tree_edges * link_width_bytes
    if bandwidth_bytes_per_sec <= 0:
        raise WseModelError("bandwidth must be positive")
    return wire_bytes / bandwidth_bytes_per_sec


def two_phase_allgather_bytes(
    *,
    row_count: int,
    phase_b_row_bytes: int,
    phase_c_row_bytes: int,
    row_member_count: int,
    col_member_count: int,
    node_count: int,
    epoch_tag_bits: int = DEFAULT_EPOCH_TAG_BITS,
    link_width_bytes: int = DEFAULT_LINK_WIDTH_BYTES,
) -> dict[str, object]:
    """Payload and wire volume of the FFN's two-phase AllGather (Calendar §17.2).

    Phase B gathers along the ROW groups and phase C along the COL groups, each
    with its own logical identity, hence ``keyCount = 2``. Only payload volume is
    reported here; the per-link wire volume is what the NoC trace computes.
    """
    header = flit_header_cost(
        node_count=node_count,
        epoch_tag_bits=epoch_tag_bits,
        link_width_bytes=link_width_bytes,
    )
    b_payload = row_count * phase_b_row_bytes * (row_member_count - 1)
    c_payload = row_count * phase_c_row_bytes * (col_member_count - 1)
    return {
        "node_count": node_count,
        "header_bytes": header.header_bytes,
        "header_overhead_percent": round(100.0 * header.overhead_fraction, 2),
        "payload_bytes_per_destination": {
            "phase_b": b_payload,
            "phase_c": c_payload,
        },
        "total_payload_bytes_per_core": b_payload + c_payload,
        "noc_clock_hz": NOC_CLOCK_HZ,
        "noc_link_width_bytes": NOC_LINK_WIDTH_BYTES,
        "local_dram_vs_noc_ratio": round(local_dram_vs_noc_ratio(), 3),
    }
