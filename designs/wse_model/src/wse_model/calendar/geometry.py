"""Landing geometry: symmetric addresses, ``selfOff``, and ``expVal``.

Calendar §2.8: the Calendar topology is opaque to software, so a send never
names a peer's physical address. It uses a **SPMD symmetric address plus this
core's segment offset**, and the hardware resolves the per-peer landing point::

    dst(s, r) = recvSymBase + r * recvRowStride + selfOff(s)
    selfOff(s) = rankInGroup(s) * rowBytes        // packed AllGather

Three invariants ride on this:

* **A1 symmetric same-address** — every member's ``recvTile`` sits at the same
  local offset; under SPMD one constant ``TASSIGN`` satisfies it.
* **A2 forwarding does not move the landing point** — ``routeBits`` says which
  nodes are passed and landed, never where in the arena.
* **A3 fill** — the members' ``selfOff`` segments tile every arena row without
  overlap.

The payload geometry is derived from tile types, not passed in, and produces the
two gaps the send instruction encodes::

    srcGap = sendRowStride - rowBytes
    dstGap = recvRowStride - rowBytes

``expVal`` is derived three ways that must agree (Calendar §2.8): the value the
user passes, the value derived from geometry, and the table's
``expectedRxBytes``.
"""

from __future__ import annotations

from dataclasses import dataclass, field

from wse_model.errors import CalendarError, ConservationError

__all__ = [
    "ARENA_SEGMENT_ALIGNMENT_BYTES",
    "ArenaSegment",
    "PayloadGeometry",
    "SymmetricArena",
]

#: Calendar §2.8 R-1: a UB source row must start on a 32 B boundary. This is a
#: correctness constraint inherited from the reused align-byte DMA.
ARENA_SEGMENT_ALIGNMENT_BYTES = 32


@dataclass(frozen=True)
class PayloadGeometry:
    """The two-dimensional geometry of one collective's payload."""

    row_count: int
    row_bytes: int
    send_row_stride: int
    recv_row_count: int
    recv_row_stride: int
    #: Bytes per link flit, used only for efficiency notes, never for semantics
    #: (Calendar §4.8: the link width does not enter the software interface).
    link_width_bytes: int = 64

    def __post_init__(self) -> None:
        for name in ("row_count", "row_bytes", "recv_row_count"):
            value = getattr(self, name)
            if value < 0:
                raise CalendarError(f"PayloadGeometry.{name} must be non-negative")
        if self.send_row_stride < self.row_bytes:
            raise CalendarError(
                f"sendRowStride {self.send_row_stride} is smaller than rowBytes {self.row_bytes}"
            )
        if self.recv_row_stride < self.row_bytes:
            raise CalendarError(
                f"recvRowStride {self.recv_row_stride} is smaller than rowBytes {self.row_bytes}"
            )
        if self.recv_row_count != self.row_count:
            raise CalendarError(
                f"recvRowCount {self.recv_row_count} must equal rowCount "
                f"{self.row_count}; the design derives both from the send and "
                "receive tiles (Calendar §2.8)"
            )

    @property
    def src_gap(self) -> int:
        return self.send_row_stride - self.row_bytes

    @property
    def dst_gap(self) -> int:
        return self.recv_row_stride - self.row_bytes

    @property
    def n_burst(self) -> int:
        """``nBurst`` for the whole-tile send form: one instruction, rows unfolded."""
        return self.row_count

    @property
    def capacity(self) -> int:
        """The receive range bound; a write past it must fault."""
        return self.recv_row_count * self.recv_row_stride

    @property
    def aligned_for_source(self) -> bool:
        """R-1: the send row stride is a 32 B multiple."""
        return self.send_row_stride % ARENA_SEGMENT_ALIGNMENT_BYTES == 0

    @property
    def flit_aligned_rows(self) -> bool:
        """Efficiency only: a row is a whole number of link flits (R-2)."""
        return self.link_width_bytes > 0 and self.row_bytes % self.link_width_bytes == 0

    def exp_val(self, land_count: int) -> int:
        """``expVal = rowCount x rowBytes x landCount`` (Calendar §2.8).

        ``landCount`` is the number of **remote** land sources, so the source's
        own segment never counts (contract 1).
        """
        if land_count < 0:
            raise CalendarError(f"landCount must be non-negative, got {land_count}")
        return self.row_count * self.row_bytes * land_count

    def check_source_alignment(self) -> None:
        if not self.aligned_for_source:
            raise CalendarError(
                f"sendRowStride {self.send_row_stride} is not a multiple of "
                f"{ARENA_SEGMENT_ALIGNMENT_BYTES} B; R-1 makes 32 B row-start "
                "alignment a correctness requirement (Calendar §2.8)"
            )

    def describe(self) -> dict[str, object]:
        return {
            "rowCount": self.row_count,
            "rowBytes": self.row_bytes,
            "sendRowStride": self.send_row_stride,
            "recvRowCount": self.recv_row_count,
            "recvRowStride": self.recv_row_stride,
            "srcGap": self.src_gap,
            "dstGap": self.dst_gap,
            "nBurst": self.n_burst,
            "capacity": self.capacity,
            "r1_aligned": self.aligned_for_source,
            "flit_aligned_rows": self.flit_aligned_rows,
        }


@dataclass(frozen=True)
class ArenaSegment:
    """One member's slice of the arena, as seen by that member."""

    rank: int
    offset: int
    nbytes: int

    def __post_init__(self) -> None:
        if self.rank < 0:
            raise CalendarError(f"ArenaSegment.rank must be non-negative, got {self.rank}")
        if self.offset < 0 or self.nbytes < 0:
            raise CalendarError(f"ArenaSegment(rank={self.rank}) has a negative offset or length")


@dataclass
class SymmetricArena:
    """The per-member receive arena and its symmetric addressing.

    ``recv_sym_base`` is identical on every member (A1). Segment offsets are the
    only per-core varying operand of a send (``selfOff``).
    """

    recv_sym_base: int
    row_count: int
    row_stride: int
    segments: tuple[ArenaSegment, ...]
    #: ``FabricSelfDelivery``; when false the core writes its own segment with a
    #: local UB-to-UB copy and those bytes never enter ``expVal`` (Step4s).
    self_delivery: bool = False
    _by_rank: dict[int, ArenaSegment] = field(init=False, repr=False, default_factory=dict)

    def __post_init__(self) -> None:
        self._by_rank = {segment.rank: segment for segment in self.segments}
        if len(self._by_rank) != len(self.segments):
            raise CalendarError("ArenaSegment ranks must be unique")

    @property
    def member_count(self) -> int:
        return len(self.segments)

    def segment(self, rank: int) -> ArenaSegment:
        try:
            return self._by_rank[rank]
        except KeyError:
            raise CalendarError(f"rank {rank} is not an arena member") from None

    def self_off(self, rank: int) -> int:
        return self.segment(rank).offset

    def dst_address(self, rank: int, row: int) -> int:
        """``recvSymBase + r * recvRowStride + selfOff(rank)``."""
        if not 0 <= row < self.row_count:
            raise CalendarError(f"row {row} is outside 0..{self.row_count - 1}")
        return self.recv_sym_base + row * self.row_stride + self.self_off(rank)

    def validate_fill(self) -> None:
        """A3: segments tile ``[0, row_stride)`` per row, without overlap.

        A gap or an overlap means the arena is either partially written or
        double written, so the compiled ``selfOff`` set is wrong.
        """
        ordered = sorted(self.segments, key=lambda segment: segment.offset)
        cursor = 0
        for segment in ordered:
            if segment.offset != cursor:
                relation = "overlaps" if segment.offset < cursor else "leaves a gap at"
                raise CalendarError(
                    f"selfOff for rank {segment.rank} starts at {segment.offset} "
                    f"but the arena row cursor is {cursor}: it {relation} "
                    f"{cursor} (invariant A3 requires a gapless, non-overlapping "
                    "tiling of every row)"
                )
            cursor = segment.offset + segment.nbytes
        if cursor != self.row_stride:
            raise CalendarError(
                f"arena segments cover {cursor} B of a {self.row_stride} B row; "
                "A3 requires the segments to fill every row exactly"
            )

    @classmethod
    def packed(
        cls,
        *,
        recv_sym_base: int,
        row_count: int,
        row_bytes: int,
        member_ranks: tuple[int, ...],
        self_delivery: bool = False,
        row_padding: int = 0,
    ) -> SymmetricArena:
        """Build the packed AllGather arena: ``selfOff(rank) = i * rowBytes``."""
        segments = tuple(
            ArenaSegment(rank=rank, offset=index * row_bytes, nbytes=row_bytes)
            for index, rank in enumerate(member_ranks)
        )
        arena = cls(
            recv_sym_base=recv_sym_base,
            row_count=row_count,
            row_stride=len(member_ranks) * row_bytes + row_padding,
            segments=segments,
            self_delivery=self_delivery,
        )
        arena.validate_fill()
        return arena

    def check_conservation(
        self,
        *,
        land_counts: dict[int, int],
        row_bytes: int,
        row_count: int,
        declared_expected_bytes: dict[int, int] | None = None,
    ) -> None:
        """Calendar §2.7.2 conservation for this arena.

        For every destination ``d``, the bytes its land set delivers are
        ``landCount[d] x rowCount x rowBytes``, and that is exactly the
        device-side MTE4 ``expVal``. Two things must therefore hold: the figure
        must fit the arena's row capacity, and it must equal the entry's declared
        ``expectedRxBytes`` when a table is available.
        """
        capacity = self.row_count * self.row_stride
        for rank, land_count in sorted(land_counts.items()):
            derived = land_count * row_count * row_bytes
            if derived > capacity:
                raise ConservationError(rank, capacity, derived)
            if derived > row_count * self.row_stride:
                raise ConservationError(rank, row_count * self.row_stride, derived)
            if declared_expected_bytes is not None:
                declared = declared_expected_bytes.get(rank)
                if declared is not None and declared != derived:
                    raise ConservationError(rank, declared, derived)

    def describe(self) -> dict[str, object]:
        return {
            "recvSymBase": self.recv_sym_base,
            "rowCount": self.row_count,
            "rowStride": self.row_stride,
            "memberCount": self.member_count,
            "selfDelivery": self.self_delivery,
            "segments": [
                {"rank": s.rank, "offset": s.offset, "nbytes": s.nbytes} for s in self.segments
            ],
        }
