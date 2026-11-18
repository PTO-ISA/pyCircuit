"""Cube and Vector throughput, and the roofline verdict for one tile.

Whitepaper §3.1 gives the Cube as an ``M x K x N`` MAC block per cycle and §19.2
draws the conclusion that matters: for a decode-stage GEMV the weight bytes, not
the FLOPs, set the time, so lower precision helps because it halves the bytes
rather than because it multiplies the throughput.

This module turns a matmul shape into cycles, weight bytes, and which of the two
dominates, using the one-to-four-to-eight MAC ratio the document states.

It deliberately reports **two** verdicts, because they are not the same question:

* :attr:`MatmulTiming.first_order_is_memory_bound` reproduces the whitepaper's
  balance-point analysis, which counts ``2 x batch`` FLOP per weight element and
  is what produces the published minimum batches 6 / 23 / 46; and
* :attr:`MatmulTiming.is_memory_bound` is tile-accurate: it charges the Cube for
  a partially filled ``M`` dimension, so at batch 1 with a 16-row block the array
  runs at 1/16 of its row capacity.

At small batch the two can disagree, and reporting only one of them would hide a
real analytical tension rather than surface it. See
``docs/decisions/0006-two-roofline-verdicts.md``.
"""

from __future__ import annotations

from dataclasses import dataclass

from wse_model.analysis.platform import (
    AICORE_CLOCK_HZ,
    LOCAL_DRAM_BYTES_PER_SEC,
    PRECISIONS,
    VECTOR_BYTES_PER_CYCLE,
    PrecisionSpec,
    precision,
)
from wse_model.errors import WseModelError

__all__ = ["MatmulShape", "MatmulTiming", "VectorTiming", "matmul_timing", "vector_timing"]


@dataclass(frozen=True)
class MatmulShape:
    """A dense matmul ``[M, K] x [K, N]`` in elements."""

    m: int
    k: int
    n: int

    def __post_init__(self) -> None:
        for name in ("m", "k", "n"):
            if getattr(self, name) < 0:
                raise WseModelError(f"MatmulShape.{name} must be non-negative")

    @property
    def macs(self) -> int:
        return self.m * self.k * self.n

    @property
    def flops(self) -> int:
        """Two FLOPs per MAC, the document's convention (Appendix A.4)."""
        return 2 * self.macs

    @property
    def is_gemv(self) -> bool:
        """A decode-stage GEMV: one token, so ``M = 1``."""
        return self.m == 1

    def describe(self) -> dict[str, object]:
        return {
            "shape": f"[{self.m}, {self.k}] x [{self.k}, {self.n}]",
            "macs": self.macs,
            "flops": self.flops,
            "gemv": self.is_gemv,
        }


@dataclass(frozen=True)
class MatmulTiming:
    """The Cube's cost for one tile, and its memory counterpart."""

    shape: MatmulShape
    spec: PrecisionSpec
    clock_hz: float = AICORE_CLOCK_HZ
    bandwidth_bytes_per_sec: float = LOCAL_DRAM_BYTES_PER_SEC
    #: Weight bytes actually fetched, which differs from the ideal when the
    #: layout is padded. Defaults to the ideal ``K x N x bytes_per_element``.
    fetched_weight_bytes: int | None = None

    @property
    def cube_cycles(self) -> int:
        """Cycles for the tile: one pass per output sub-tile, per K block.

        The Cube block is ``M x K x N`` MACs per cycle, so each output sub-tile
        needs one pass per K block. This is tile-accurate: it charges for a
        partially filled M dimension.
        """
        return self.m_steps * self.n_steps * self.k_steps

    @property
    def compute_seconds(self) -> float:
        return self.cube_cycles / self.clock_hz

    @property
    def ideal_weight_bytes(self) -> int:
        return int(self.shape.k * self.shape.n * self.spec.bytes_per_element)

    @property
    def weight_bytes(self) -> int:
        return (
            self.ideal_weight_bytes
            if self.fetched_weight_bytes is None
            else self.fetched_weight_bytes
        )

    @property
    def fetch_seconds(self) -> float:
        return self.weight_bytes / self.bandwidth_bytes_per_sec

    @property
    def arithmetic_intensity(self) -> float:
        """FLOP per *fetched weight byte*, using the precision's real width.

        This is the physically exact figure. It differs from the document's
        ``first_order_intensity`` because the whitepaper counts two FLOPs per
        weight *element*; at FP16 the two differ by the two bytes an element
        occupies, which is exactly why the published minimum batches assume
        one byte per element.
        """
        if self.weight_bytes == 0:
            return 0.0
        return self.shape.flops / self.weight_bytes

    @property
    def is_memory_bound(self) -> bool:
        """Tile-accurate verdict: the slower of fetch and compute sets the time.

        Compare with :attr:`first_order_is_memory_bound`, which is the
        whitepaper's balance-point verdict. The two can disagree at small batch,
        because the balance point assumes the Cube fills its M dimension.
        """
        return self.fetch_seconds > self.compute_seconds

    @property
    def balance_point(self) -> float:
        return self.spec.flops_per_second(self.clock_hz) / self.bandwidth_bytes_per_sec

    @property
    def roofline_seconds(self) -> float:
        """The achievable time: the slower of compute and fetch."""
        return max(self.compute_seconds, self.fetch_seconds)

    @property
    def cycle_time_per_weight_byte(self) -> float:
        """Cycles the Cube spends per fetched weight byte at this shape."""
        if self.weight_bytes == 0:
            return 0.0
        return self.cube_cycles / self.weight_bytes

    @property
    def m_steps(self) -> int:
        return _ceil_div(self.shape.m, self.spec.m)

    @property
    def n_steps(self) -> int:
        return _ceil_div(self.shape.n, self.spec.n)

    @property
    def k_steps(self) -> int:
        return _ceil_div(self.shape.k, self.spec.k)

    @property
    def m_utilization(self) -> float:
        """How full the Cube's M dimension runs.

        The Cube's block is ``M x K x N`` per cycle, so an ``M`` below the block
        height leaves rows idle. At batch 1 with a 16-row block the array runs at
        1/16 of its row capacity, which is the classical reason an NPU GEMV is
        inefficient. The whitepaper's balance-point analysis (§1, §19.2) is
        first-order and does not include this quantisation; this property is what
        separates the two views.
        """
        capacity = self.m_steps * self.spec.m
        if capacity == 0:
            return 0.0
        return self.shape.m / capacity

    @property
    def effective_macs_per_cycle(self) -> float:
        """MACs retired per cycle, including the M-dimension under-fill."""
        if self.cube_cycles == 0:
            return 0.0
        return self.shape.macs / self.cube_cycles

    @property
    def first_order_intensity(self) -> float:
        """The document's arithmetic intensity: ``2 x batch`` FLOP per weight element.

        Whitepaper §1 states the decode GEMV intensity as approximately
        ``2 x batch`` FLOP/Byte, counting one MAC (two FLOPs) per weight element.
        That convention is what makes the published minimum batches 6 / 23 / 46,
        so the model keeps it alongside the physically exact per-byte figure
        rather than replacing one with the other.
        """
        return 2.0 * self.shape.m

    @property
    def first_order_is_memory_bound(self) -> bool:
        """The balance-point verdict of whitepaper §1, without tile quantisation."""
        return self.first_order_intensity < self.balance_point

    def describe(self) -> dict[str, object]:
        precise = self.is_memory_bound
        first_order = self.first_order_is_memory_bound
        return {
            **self.shape.describe(),
            "precision": self.spec.name,
            "cube_shape_per_cycle": f"{self.spec.m}x{self.spec.k}x{self.spec.n}",
            "cpp_macs_per_cycle": self.spec.macs_per_cycle,
            "cube_cycles": self.cube_cycles,
            "m_steps": self.m_steps,
            "n_steps": self.n_steps,
            "k_steps": self.k_steps,
            "m_utilization": round(self.m_utilization, 6),
            "effective_macs_per_cycle": round(self.effective_macs_per_cycle, 2),
            "compute_seconds": self.compute_seconds,
            "ideal_weight_bytes": self.ideal_weight_bytes,
            "fetched_weight_bytes": self.weight_bytes,
            "fetch_seconds": self.fetch_seconds,
            "arithmetic_intensity_flops_per_fetched_byte": round(self.arithmetic_intensity, 4),
            "first_order_intensity_flops_per_weight_element": round(self.first_order_intensity, 4),
            "balance_flops_per_byte": round(self.balance_point, 4),
            "bound_by": "memory" if precise else "compute",
            "first_order_bound_by": "memory" if first_order else "compute",
            "verdicts_agree": precise == first_order,
            "roofline_seconds": self.roofline_seconds,
            "cycles_per_weight_byte": round(self.cycle_time_per_weight_byte, 4),
        }


@dataclass(frozen=True)
class VectorTiming:
    """Vector throughput: 256 B/cycle, matching FixPipe (whitepaper §3.1)."""

    bytes_per_cycle: int = VECTOR_BYTES_PER_CYCLE
    clock_hz: float = AICORE_CLOCK_HZ

    def cycles(self, nbytes: int) -> int:
        if nbytes < 0:
            raise WseModelError("byte count must be non-negative")
        return _ceil_div(nbytes, self.bytes_per_cycle)

    def seconds(self, nbytes: int) -> float:
        return self.cycles(nbytes) / self.clock_hz

    def elements_per_second(self, *, bytes_per_element: int = 2) -> float:
        if bytes_per_element <= 0:
            raise WseModelError("bytes_per_element must be positive")
        return self.bytes_per_cycle / bytes_per_element * self.clock_hz

    def describe(self) -> dict[str, object]:
        return {
            "bytes_per_cycle": self.bytes_per_cycle,
            "bytes_per_second": self.bytes_per_cycle * self.clock_hz,
            "elements_per_second_fp16": self.elements_per_second(bytes_per_element=2),
            "clock_hz": self.clock_hz,
        }


def matmul_timing(
    shape: MatmulShape,
    *,
    precision_name: str = "fp16",
    fetched_weight_bytes: int | None = None,
    clock_hz: float = AICORE_CLOCK_HZ,
    bandwidth_bytes_per_sec: float = LOCAL_DRAM_BYTES_PER_SEC,
) -> MatmulTiming:
    """Time one tile on the Cube and compare it with the memory system."""
    return MatmulTiming(
        shape=shape,
        spec=precision(precision_name),
        clock_hz=clock_hz,
        bandwidth_bytes_per_sec=bandwidth_bytes_per_sec,
        fetched_weight_bytes=fetched_weight_bytes,
    )


def vector_timing(
    *, bytes_per_cycle: int = VECTOR_BYTES_PER_CYCLE, clock_hz: float = AICORE_CLOCK_HZ
) -> VectorTiming:
    return VectorTiming(bytes_per_cycle=bytes_per_cycle, clock_hz=clock_hz)


def precision_shapes() -> dict[str, str]:
    """The Cube shape of each precision, as ``MxKxN``."""
    return {name: f"{spec.m}x{spec.k}x{spec.n}" for name, spec in sorted(PRECISIONS.items())}


def _ceil_div(value: int, divisor: int) -> int:
    if divisor <= 0:
        raise WseModelError("divisor must be positive")
    return -(-value // divisor)
