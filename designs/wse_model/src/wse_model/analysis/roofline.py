"""The roofline arithmetic that motivates the whole WSE design.

Whitepaper §1 and §19: WSE trades bandwidth for latency, and the trade is
quantified by one ratio. Per AICORE the balance point is::

    balance = Cube FLOP/s / local DRAM bytes/s

which is **11.5 FLOP/Byte at FP16**, 45.9 at FP8, and 91.8 at FP4. A decode-stage
GEMV has an arithmetic intensity of about ``2 x batch`` FLOP/Byte, because each
weight byte is used once. Dividing the balance point by 2 gives the batch at
which decode stops being memory-bound: about 6 at FP16, 23 at FP8, 46 at FP4.

The design's sharper point (whitepaper §19.2) is that lowering precision helps a
memory-bound operator because it **halves the bytes**, not because it quadruples
the FLOPs — which is why FP8/FP4 accuracy matters more than peak throughput here.
"""

from __future__ import annotations

from dataclasses import dataclass

from wse_model.analysis.platform import (
    LOCAL_DRAM_BYTES_PER_SEC,
    PrecisionSpec,
    precision,
)
from wse_model.errors import WseModelError

__all__ = [
    "RooflinePoint",
    "arithmetic_intensity_decode",
    "balance_point",
    "min_batch_to_escape_memory_bound",
    "roofline",
]


def arithmetic_intensity_decode(batch: int) -> float:
    """Decode-stage GEMV intensity, about ``2 x batch`` FLOP/Byte.

    Whitepaper §1: each weight byte is multiplied once, and a multiply-add is two
    FLOPs, so intensity scales with the batch dimension.

    This is the document's *first-order* convention: it counts two FLOPs per
    weight **element**, which treats a "weight byte" as one element regardless of
    the element's real width. It is what produces the published minimum batches,
    and it assumes the Cube fills its ``M x K x N`` block. The tile-accurate
    counterpart, which charges for a partly idle ``M`` dimension, lives in
    :class:`wse_model.core.cube.MatmulTiming`; decision 0006 records why both are
    reported.
    """
    if batch < 0:
        raise WseModelError(f"batch must be non-negative, got {batch}")
    return 2.0 * batch


def balance_point(name: str, *, bandwidth_bytes_per_sec: float = LOCAL_DRAM_BYTES_PER_SEC) -> float:
    """The FLOP/Byte balance point of one precision."""
    return precision(name).compute_intensity_balance(bandwidth_bytes_per_sec)


def min_batch_to_escape_memory_bound(
    name: str, *, bandwidth_bytes_per_sec: float = LOCAL_DRAM_BYTES_PER_SEC
) -> float:
    """Balance point divided by 2: the batch above which decode is compute-bound.

    Valid under the whitepaper's first-order convention (see
    :func:`arithmetic_intensity_decode`). A tile-accurate model can return a
    different verdict at small batch, because at ``batch = 1`` a 16-row Cube block
    runs 1/16 filled; compare with
    :attr:`wse_model.core.cube.MatmulTiming.is_memory_bound`.
    """
    return balance_point(name, bandwidth_bytes_per_sec=bandwidth_bytes_per_sec) / 2.0


@dataclass(frozen=True)
class RooflinePoint:
    """One precision's roofline summary."""

    spec: PrecisionSpec
    balance_flops_per_byte: float
    min_batch: float

    @property
    def name(self) -> str:
        return self.spec.name

    def is_memory_bound(self, batch: int) -> bool:
        """Whether decode at this batch is still memory-bound."""
        return arithmetic_intensity_decode(batch) < self.balance_flops_per_byte

    def compute_time_per_byte(self, flops_needed: float) -> float:
        """Seconds the Cube needs for ``flops_needed``."""
        return flops_needed / self.spec.flops_per_second()

    def memory_time_per_byte(self, weight_bytes: float) -> float:
        """Seconds local DRAM needs for ``weight_bytes``."""
        return weight_bytes / LOCAL_DRAM_BYTES_PER_SEC

    def describe(self) -> dict[str, object]:
        return {
            "precision": self.name,
            "tflops": round(self.spec.tflops(), 3),
            "balance_flops_per_byte": round(self.balance_flops_per_byte, 3),
            "min_batch_for_compute_bound": round(self.min_batch, 2),
            "weights_bytes_per_element": self.spec.bytes_per_element,
        }


def roofline(
    name: str, *, bandwidth_bytes_per_sec: float = LOCAL_DRAM_BYTES_PER_SEC
) -> RooflinePoint:
    """Build the roofline summary for one precision."""
    spec = precision(name)
    balance = spec.compute_intensity_balance(bandwidth_bytes_per_sec)
    return RooflinePoint(spec=spec, balance_flops_per_byte=balance, min_batch=balance / 2.0)


def roofline_table() -> tuple[RooflinePoint, ...]:
    """The whitepaper §19.2 table, in FP16 / FP8 / FP4 order."""
    return tuple(roofline(name) for name in ("fp16", "fp8", "fp4"))
