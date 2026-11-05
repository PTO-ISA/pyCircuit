"""Roofline, bandwidth, and latency analysis for the WSE design.

These modules hold the closed-form arithmetic of the whitepaper: the balance
point that motivates the architecture (§1, §19.2), the four bandwidths that
constrain partitioning (§4.4), and the five-block latency budget (§19.1). They
depend on no simulation state — the trace-based volume numbers come from
:mod:`wse_model.collective` and :mod:`wse_model.noc`.
"""

from wse_model.analysis.bandwidth import (
    BANDWIDTHS,
    Bandwidth,
    collective_transfer_seconds,
    flit_header_cost,
    link_time_seconds,
    local_dram_vs_noc_ratio,
    two_phase_allgather_bytes,
)
from wse_model.analysis.dcache import DCacheSpec, RouteTableResidency, dcache_budget
from wse_model.analysis.latency import LatencyBlock, LatencyBudget
from wse_model.analysis.platform import (
    AICORE_CLOCK_HZ,
    LOCAL_DRAM_BYTES,
    LOCAL_DRAM_BYTES_PER_SEC,
    NOC_LINK_BYTES_PER_SEC,
    NOC_LINK_WIDTH_BYTES,
    ON_CHIP_BUFFERS,
    PRECISIONS,
    AicoreSpec,
    PrecisionSpec,
    precision,
)
from wse_model.analysis.roofline import (
    RooflinePoint,
    arithmetic_intensity_decode,
    balance_point,
    min_batch_to_escape_memory_bound,
    roofline,
    roofline_table,
)

__all__ = [
    "AICORE_CLOCK_HZ",
    "BANDWIDTHS",
    "LOCAL_DRAM_BYTES",
    "LOCAL_DRAM_BYTES_PER_SEC",
    "NOC_LINK_BYTES_PER_SEC",
    "NOC_LINK_WIDTH_BYTES",
    "ON_CHIP_BUFFERS",
    "PRECISIONS",
    "AicoreSpec",
    "Bandwidth",
    "DCacheSpec",
    "RouteTableResidency",
    "LatencyBlock",
    "LatencyBudget",
    "PrecisionSpec",
    "RooflinePoint",
    "arithmetic_intensity_decode",
    "balance_point",
    "collective_transfer_seconds",
    "dcache_budget",
    "flit_header_cost",
    "link_time_seconds",
    "local_dram_vs_noc_ratio",
    "min_batch_to_escape_memory_bound",
    "precision",
    "roofline",
    "roofline_table",
    "two_phase_allgather_bytes",
]
