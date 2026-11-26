"""Landing geometry: symmetric addressing, ``selfOff``, ``expVal``, R-1/А3."""

from __future__ import annotations

import pytest

from wse_model.calendar.geometry import (
    ARENA_SEGMENT_ALIGNMENT_BYTES,
    ArenaSegment,
    PayloadGeometry,
    SymmetricArena,
)
from wse_model.errors import CalendarError, ConservationError

pytestmark = pytest.mark.unit


def test_gaps_are_derived_from_the_strides() -> None:
    geometry = PayloadGeometry(
        row_count=8,
        row_bytes=192,
        send_row_stride=256,
        recv_row_count=8,
        recv_row_stride=1536,
    )
    assert geometry.src_gap == 64
    assert geometry.dst_gap == 1344
    assert geometry.n_burst == 8
    assert geometry.capacity == 8 * 1536


def test_recv_row_count_must_match_row_count() -> None:
    with pytest.raises(CalendarError):
        PayloadGeometry(
            row_count=8,
            row_bytes=192,
            send_row_stride=192,
            recv_row_count=4,
            recv_row_stride=1536,
        )


def test_r1_requires_a_32_byte_multiple_source_stride() -> None:
    aligned = PayloadGeometry(
        row_count=1, row_bytes=192, send_row_stride=192, recv_row_count=1, recv_row_stride=192
    )
    assert aligned.aligned_for_source
    aligned.check_source_alignment()

    unaligned = PayloadGeometry(
        row_count=1, row_bytes=192, send_row_stride=200, recv_row_count=1, recv_row_stride=192
    )
    assert not unaligned.aligned_for_source
    with pytest.raises(CalendarError):
        unaligned.check_source_alignment()


def test_r2_row_bytes_need_not_be_a_link_multiple() -> None:
    """R-2 is efficiency only: align mode exists precisely for a non-multiple tail."""
    geometry = PayloadGeometry(
        row_count=1, row_bytes=100, send_row_stride=128, recv_row_count=1, recv_row_stride=128
    )
    assert geometry.aligned_for_source
    assert not geometry.flit_aligned_rows


def test_exp_val_is_row_count_times_row_bytes_times_land_count() -> None:
    geometry = PayloadGeometry(
        row_count=8,
        row_bytes=192,
        send_row_stride=192,
        recv_row_count=8,
        recv_row_stride=1536,
    )
    # Calendar §2.2.1: 8 x 192 x 7 = 10752 for FFN phase B.
    assert geometry.exp_val(7) == 10752
    # and 8 x 1536 x 3 = 36864 for phase C.
    phase_c = PayloadGeometry(
        row_count=8,
        row_bytes=1536,
        send_row_stride=1536,
        recv_row_count=8,
        recv_row_stride=6144,
    )
    assert phase_c.exp_val(3) == 36864
    assert geometry.exp_val(0) == 0


def test_packed_arena_self_off_is_rank_times_row_bytes() -> None:
    arena = SymmetricArena.packed(
        recv_sym_base=0x1000,
        row_count=8,
        row_bytes=192,
        member_ranks=(0, 1, 2, 3, 4, 5, 6, 7),
    )
    assert arena.member_count == 8
    assert arena.row_stride == 8 * 192
    for rank in range(8):
        assert arena.self_off(rank) == rank * 192
        assert arena.dst_address(rank, 0) == 0x1000 + rank * 192
        assert arena.dst_address(rank, 3) == 0x1000 + 3 * 1536 + rank * 192


def test_a3_requires_a_gapless_tiling_of_every_row() -> None:
    good = SymmetricArena(
        recv_sym_base=0,
        row_count=1,
        row_stride=256,
        segments=(
            ArenaSegment(rank=0, offset=0, nbytes=128),
            ArenaSegment(rank=1, offset=128, nbytes=128),
        ),
    )
    good.validate_fill()

    overlap = SymmetricArena(
        recv_sym_base=0,
        row_count=1,
        row_stride=256,
        segments=(
            ArenaSegment(rank=0, offset=0, nbytes=192),
            ArenaSegment(rank=1, offset=128, nbytes=128),
        ),
    )
    with pytest.raises(CalendarError) as excinfo:
        overlap.validate_fill()
    assert "A3" in str(excinfo.value)

    gap = SymmetricArena(
        recv_sym_base=0,
        row_count=1,
        row_stride=256,
        segments=(
            ArenaSegment(rank=0, offset=0, nbytes=64),
            ArenaSegment(rank=1, offset=128, nbytes=64),
        ),
    )
    with pytest.raises(CalendarError):
        gap.validate_fill()


def test_conservation_matches_the_entry_expected_bytes() -> None:
    arena = SymmetricArena.packed(
        recv_sym_base=0,
        row_count=8,
        row_bytes=192,
        member_ranks=(0, 1, 2, 3, 4, 5, 6, 7),
    )
    land_counts = dict.fromkeys(range(8), 7)
    # 7 x 8 x 192 = 10752 fits the 8 x 1536 = 12288 B row.
    arena.check_conservation(land_counts=land_counts, row_bytes=192, row_count=8)

    declared = dict.fromkeys(range(8), 10752)
    arena.check_conservation(
        land_counts=land_counts,
        row_bytes=192,
        row_count=8,
        declared_expected_bytes=declared,
    )

    declared[3] = 1000
    with pytest.raises(ConservationError) as excinfo:
        arena.check_conservation(
            land_counts=land_counts,
            row_bytes=192,
            row_count=8,
            declared_expected_bytes=declared,
        )
    assert excinfo.value.destination == 3

    too_small = SymmetricArena(recv_sym_base=0, row_count=8, row_stride=192, segments=())
    with pytest.raises(ConservationError):
        too_small.check_conservation(land_counts={0: 7}, row_bytes=192, row_count=8)


def test_segment_alignment_constant_is_32_bytes() -> None:
    assert ARENA_SEGMENT_ALIGNMENT_BYTES == 32
