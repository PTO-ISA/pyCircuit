"""Platform constants, transcribed from the design documents.

Every number here carries its source. Nothing is rounded or inferred: if a value
is not in the design sources it is a declared parameter or an open item, never a
constant (see :mod:`wse_model.open_items`).
"""

from __future__ import annotations

from dataclasses import dataclass

from wse_model.errors import WseModelError

__all__ = [
    "AICORE_CLOCK_HZ",
    "BATCHER_UB_FABRIC_BYTES_PER_SEC",
    "BATCHER_UB_HOST_BYTES_PER_SEC",
    "BATCHER_UB_HOST_LANE_GBPS",
    "BATCHER_UB_LANES",
    "BATCHER_UB_LANE_GBPS",
    "FIXPIPE_BYTES_PER_CYCLE",
    "LOCAL_DRAM_BYTES",
    "LOCAL_DRAM_BYTES_PER_SEC",
    "NOC_CLOCK_HZ",
    "NOC_LINK_WIDTH_BYTES",
    "ON_CHIP_BUFFERS",
    "PRECISIONS",
    "UB_BYTES",
    "VECTOR_BYTES_PER_CYCLE",
    "AicoreSpec",
    "PrecisionSpec",
    "precision",
]

#: Whitepaper §3: one AICORE = 1 Cube + 1 Vector at 1.4 GHz.
AICORE_CLOCK_HZ = 1.4e9

#: Whitepaper §2.2 / §5.1: the intra-reticle NoC runs synchronously at 2 GHz.
NOC_CLOCK_HZ = 2.0e9

#: Whitepaper §5.1: 64 B link at 2 GHz gives 128 GB/s per direction.
NOC_LINK_WIDTH_BYTES = 64
NOC_LINK_BYTES_PER_SEC = NOC_LINK_WIDTH_BYTES * NOC_CLOCK_HZ

#: Whitepaper §3.2 / §1.2: each AICORE owns 384 MB of local DRAM at 1 TB/s.
LOCAL_DRAM_BYTES = 384 * 1024 * 1024
LOCAL_DRAM_BYTES_PER_SEC = 1.0e12
#: Whitepaper §3.4: the actual read/write granularity is a 2 KB page.
LOCAL_DRAM_PAGE_BYTES = 2048

#: Whitepaper §3.1: Vector parallelism, and FixPipe matching it.
VECTOR_BYTES_PER_CYCLE = 256
FIXPIPE_BYTES_PER_CYCLE = 256

#: Whitepaper §3.2: the on-chip storage hierarchy.
ON_CHIP_BUFFERS: dict[str, int] = {
    "L0A": 128 * 1024,
    "L0B": 512 * 1024,
    "L0C": 256 * 1024,
    "L1": 1 * 1024 * 1024,
    "UB": 384 * 1024,
}

#: Size of the intra-core Unified Buffer, in bytes.
UB_BYTES = ON_CHIP_BUFFERS["UB"]

#: Whitepaper §6.3: the external UB bus carries 8 fabric lanes at 224 Gbps each
#: and 1 host lane at 112 Gbps. ``8 x 224 Gbps = 224 GB/s`` and
#: ``1 x 112 Gbps = 14 GB/s``; the design document states both, so the model keeps
#: the lane rates in gigabits and derives the byte rates.
BATCHER_UB_LANES = 8
BATCHER_UB_LANE_GBPS = 224
BATCHER_UB_HOST_LANE_GBPS = 112
BATCHER_UB_FABRIC_GBPS = BATCHER_UB_LANES * BATCHER_UB_LANE_GBPS
BATCHER_UB_FABRIC_BYTES_PER_SEC = BATCHER_UB_FABRIC_GBPS * 1e9 / 8
BATCHER_UB_HOST_BYTES_PER_SEC = BATCHER_UB_HOST_LANE_GBPS * 1e9 / 8


@dataclass(frozen=True)
class PrecisionSpec:
    """One Cube precision shape (whitepaper §3.1).

    ``m``, ``k``, ``n`` are the per-cycle Cube dimensions in elements; a MAC is
    counted as two FLOPs (whitepaper Appendix A.4 assumption 4).
    """

    name: str
    m: int
    k: int
    n: int
    #: Weight bytes per element, which is what a memory-bound operator actually
    #: pays (whitepaper §19.2).
    bytes_per_element: float

    @property
    def macs_per_cycle(self) -> int:
        return self.m * self.k * self.n

    @property
    def flops_per_cycle(self) -> int:
        return 2 * self.macs_per_cycle

    def flops_per_second(self, clock_hz: float = AICORE_CLOCK_HZ) -> float:
        return self.flops_per_cycle * clock_hz

    def tflops(self, clock_hz: float = AICORE_CLOCK_HZ) -> float:
        return self.flops_per_second(clock_hz) / 1e12

    def compute_intensity_balance(
        self, bandwidth_bytes_per_sec: float = LOCAL_DRAM_BYTES_PER_SEC
    ) -> float:
        """FLOP per byte at which the Cube and the memory system balance."""
        if bandwidth_bytes_per_sec <= 0:
            raise WseModelError("bandwidth must be positive")
        return self.flops_per_second() / bandwidth_bytes_per_sec

    def describe(self, clock_hz: float = AICORE_CLOCK_HZ) -> dict[str, object]:
        return {
            "precision": self.name,
            "cube_shape": f"{self.m}x{self.k}x{self.n}",
            "macs_per_cycle": self.macs_per_cycle,
            "tflops": round(self.tflops(clock_hz), 3),
            "bytes_per_element": self.bytes_per_element,
            "balance_flops_per_byte": round(self.compute_intensity_balance(), 3),
        }


#: Whitepaper §3.1 table, with the 1:4:8 MAC ratio made explicit.
PRECISIONS: dict[str, PrecisionSpec] = {
    "fp16": PrecisionSpec("fp16", 16, 16, 16, 2.0),
    "fp8": PrecisionSpec("fp8", 16, 32, 32, 1.0),
    "fp4": PrecisionSpec("fp4", 16, 64, 32, 0.5),
}


def precision(name: str) -> PrecisionSpec:
    """Look up a precision by name."""
    try:
        return PRECISIONS[name.lower()]
    except KeyError:
        known = ", ".join(sorted(PRECISIONS))
        raise WseModelError(f"unknown precision {name!r}; known: {known}") from None


@dataclass(frozen=True)
class AicoreSpec:
    """One AICORE at a glance (whitepaper §3)."""

    clock_hz: float = AICORE_CLOCK_HZ
    local_dram_bytes: int = LOCAL_DRAM_BYTES
    local_dram_bytes_per_sec: float = LOCAL_DRAM_BYTES_PER_SEC
    vector_bytes_per_cycle: int = VECTOR_BYTES_PER_CYCLE
    fixpipe_bytes_per_cycle: int = FIXPIPE_BYTES_PER_CYCLE

    @property
    def local_dram_bytes_per_cycle(self) -> float:
        """About 714 B per AICORE cycle at 1 TB/s and 1.4 GHz."""
        return self.local_dram_bytes_per_sec / self.clock_hz

    @property
    def weights_per_cycle_fp16(self) -> int:
        """Elements the Cube consumes per cycle at FP16: 16 x 16."""
        return PRECISIONS["fp16"].m * PRECISIONS["fp16"].k

    @property
    def vector_elements_per_cycle_fp16(self) -> int:
        return self.vector_bytes_per_cycle // 2

    def describe(self) -> dict[str, object]:
        return {
            "clock_hz": self.clock_hz,
            "local_dram_bytes": self.local_dram_bytes,
            "local_dram_bytes_per_cycle": round(self.local_dram_bytes_per_cycle, 2),
            "vector_bytes_per_cycle": self.vector_bytes_per_cycle,
            "fixpipe_bytes_per_cycle": self.fixpipe_bytes_per_cycle,
            "on_chip_buffers": dict(ON_CHIP_BUFFERS),
            "cube_weights_per_cycle_fp16": self.weights_per_cycle_fp16,
            "vector_elements_per_second_fp16": self.vector_elements_per_cycle_fp16 * self.clock_hz,
        }
