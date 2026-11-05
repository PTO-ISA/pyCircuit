"""Flit formatting and header overhead.

Calendar §1.8: every flit carries the **same** header — ``routeBits``,
``opcode``, ``redOp``, and ``epochTag`` — and the hardware replicates it when a
contiguous byte run is split into flits. Software never programs flit by flit.

The header is the price of a stateless NoC, so the model computes it exactly:

===============  =========================  ==============================
field            width                      note
===============  =========================  ==============================
``routeBits``    ``2 x node_count`` bit    80 bit = 10 B at 40 nodes;
                                           96 bit = 12 B at 48 (open item Q1)
``opcode``       3 bit                      shares a byte with ``redOp``
``redOp``        3 bit
``epochTag``     8-16 bit                   open item C-5; 8 bit baseline
===============  =========================  ==============================

At 40 nodes and an 8 bit epoch tag the header is 12 B, which is **18.75%** of a
64 B link — the "about 19%" of the design document. At 48 nodes it is 14 B,
about 22%.
"""

from __future__ import annotations

from dataclasses import dataclass

from wse_model.calendar.collective import OPCODE_RESERVED, RedOp
from wse_model.calendar.epoch import EPOCH_TAG_MAX_BITS, EPOCH_TAG_MIN_BITS
from wse_model.calendar.route_bits import RouteBits
from wse_model.errors import CalendarError, WseModelError

__all__ = [
    "DEFAULT_EPOCH_TAG_BITS",
    "DEFAULT_LINK_WIDTH_BYTES",
    "Flit",
    "FlitHeader",
    "flit_count",
    "split_payload",
]

#: Calendar §1.8 proposes 8-16 bit for ``epochTag`` (open item C-5).
DEFAULT_EPOCH_TAG_BITS = EPOCH_TAG_MIN_BITS

#: The Calendar baseline link width: 64 B at 2 GHz = 128 GB/s per direction.
DEFAULT_LINK_WIDTH_BYTES = 64


@dataclass(frozen=True)
class FlitHeader:
    """The header every flit of a send carries identically."""

    route_bits: RouteBits
    opcode: int
    red_op: RedOp = RedOp.NONE
    epoch_tag: int = 1
    epoch_tag_bits: int = DEFAULT_EPOCH_TAG_BITS
    link_width_bytes: int = DEFAULT_LINK_WIDTH_BYTES

    def __post_init__(self) -> None:
        if self.opcode in OPCODE_RESERVED:
            raise CalendarError(
                f"opcode {self.opcode} is reserved; a flit carrying it must fault "
                "or be dropped and reported (Calendar §2.3)"
            )
        if not EPOCH_TAG_MIN_BITS <= self.epoch_tag_bits <= EPOCH_TAG_MAX_BITS:
            raise WseModelError(
                f"epochTag width {self.epoch_tag_bits} is outside the proposed "
                f"{EPOCH_TAG_MIN_BITS}-{EPOCH_TAG_MAX_BITS} bit range (C-5)"
            )
        capacity = (1 << self.epoch_tag_bits) - 1
        if not 1 <= self.epoch_tag <= capacity:
            raise WseModelError(
                f"epochTag {self.epoch_tag} is outside 1..{capacity} for a "
                f"{self.epoch_tag_bits} bit field; collection epochs start at 1 "
                "(Calendar §1.5)"
            )
        if self.link_width_bytes <= 0:
            raise WseModelError("link width must be positive")

    # -- size ----------------------------------------------------------------

    @property
    def route_bits_bytes(self) -> int:
        return self.route_bits.width_bytes

    @property
    def opcode_redop_bytes(self) -> int:
        """``opcode(3b) + redOp(3b)`` share one byte."""
        return 1

    @property
    def epoch_bytes(self) -> int:
        return (self.epoch_tag_bits + 7) // 8

    @property
    def header_bytes(self) -> int:
        """Total per-flit header: 12 B at 40 nodes, 14 B at 48."""
        return self.route_bits_bytes + self.opcode_redop_bytes + self.epoch_bytes

    @property
    def payload_bytes(self) -> int:
        """Usable payload per flit after the header."""
        remaining = self.link_width_bytes - self.header_bytes
        if remaining <= 0:
            raise WseModelError(
                f"the {self.header_bytes} B header does not fit the {self.link_width_bytes} B link"
            )
        return remaining

    @property
    def overhead_fraction(self) -> float:
        return self.header_bytes / self.link_width_bytes

    @property
    def tail_padding_bytes(self) -> int:
        """Padding the last flit of a run needs to fill the link width."""
        return self.link_width_bytes - self.header_bytes

    def describe(self) -> dict[str, object]:
        return {
            "routeBits_bytes": self.route_bits_bytes,
            "opcode_bits": 3,
            "redOp_bits": 3,
            "epochTag_bits": self.epoch_tag_bits,
            "header_bytes": self.header_bytes,
            "link_width_bytes": self.link_width_bytes,
            "payload_bytes_per_flit": self.payload_bytes,
            "overhead_fraction": round(self.overhead_fraction, 6),
            "overhead_percent": round(100.0 * self.overhead_fraction, 2),
        }


@dataclass(frozen=True)
class Flit:
    """One link-width transfer: a replicated header plus its payload slice."""

    header: FlitHeader
    payload: bytes
    tail_padding: int = 0

    @property
    def payload_bytes(self) -> int:
        return len(self.payload)

    @property
    def wire_bytes(self) -> int:
        return self.header.header_bytes + self.payload_bytes + self.tail_padding


def flit_count(total_payload_bytes: int, header: FlitHeader) -> int:
    """Number of flits needed to carry ``total_payload_bytes``."""
    if total_payload_bytes < 0:
        raise WseModelError("payload length must be non-negative")
    if total_payload_bytes == 0:
        return 0
    per_flit = header.payload_bytes
    return (total_payload_bytes + per_flit - 1) // per_flit


def split_payload(payload: bytes, header: FlitHeader) -> tuple[Flit, ...]:
    """Split a byte run into flits, replicating the header on each."""
    per_flit = header.payload_bytes
    flits: list[Flit] = []
    for offset in range(0, len(payload), per_flit):
        chunk = payload[offset : offset + per_flit]
        padding = per_flit - len(chunk)
        flits.append(Flit(header=header, payload=chunk, tail_padding=padding))
    return tuple(flits)
