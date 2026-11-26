"""The ``routeBits`` codec and the 16 B route entry."""

from __future__ import annotations

import pytest

from wse_model.calendar.entry import (
    ENTRY_SIZE_BYTES,
    ROUTE_BITS_NODE_CAPACITY,
    CalendarRouteEntry,
    CalendarRouteRegs,
    RouteEntryFlags,
    entry_layout_size_bytes,
)
from wse_model.calendar.route_bits import BitPair, RouteBits
from wse_model.errors import IllegalBitPairError, LandSetError, WseModelError
from wse_model.fixtures import (
    GOLDEN_LAND_NODES,
    GOLDEN_PASS_NODES,
    GOLDEN_ROUTE_BITS,
    golden_route_bits,
)
from wse_model.topology import CALENDAR_BASELINE, WHITEPAPER_HARDWARE

pytestmark = pytest.mark.unit

NODES_40 = CALENDAR_BASELINE.topology.node_count
NODES_48 = WHITEPAPER_HARDWARE.topology.node_count


# -- pairs -----------------------------------------------------------------


def test_bit_pair_uses_low_bit_for_land_and_high_bit_for_pass() -> None:
    assert int(BitPair.ABSENT) == 0b00
    assert int(BitPair.PASS) == 0b10
    assert int(BitPair.LAND) == 0b11
    assert int(BitPair.ILLEGAL) == 0b01

    assert BitPair.PASS.passes and not BitPair.PASS.lands
    assert BitPair.LAND.passes and BitPair.LAND.lands
    assert not BitPair.ABSENT.passes and not BitPair.ABSENT.lands
    # The illegal pair is L=1 with P=0.
    assert BitPair.ILLEGAL.lands and not BitPair.ILLEGAL.passes


def test_from_flags_round_trips_every_pair() -> None:
    for pair in BitPair:
        rebuilt = BitPair.from_flags(p=pair.passes, l=pair.lands)
        assert rebuilt is pair


# -- golden vector ---------------------------------------------------------


def test_golden_vector_matches_the_published_bytes() -> None:
    """Calendar §2.2.1: the consistency anchor every tool must agree on."""
    bitmap = golden_route_bits()
    assert bitmap.to_hex() == GOLDEN_ROUTE_BITS
    assert bitmap.pass_nodes == GOLDEN_PASS_NODES
    assert bitmap.land_nodes == GOLDEN_LAND_NODES
    assert bitmap.land_count == len(GOLDEN_LAND_NODES)
    assert bitmap.width_bytes == 10
    assert bitmap.width_bits == 80


def test_golden_vector_byte_layout_is_lsb_first() -> None:
    """Byte 0 holds nodes 0..3, node i at bits 2i (L) and 2i+1 (P)."""
    bitmap = golden_route_bits()
    data = bitmap.to_bytes()
    # nodes 0..3 are pass-only (10) => 0b10101010
    assert data[0] == 0xAA
    # node 4 lands and passes (11), nodes 5..7 absent
    assert data[1] == 0x03
    # node 10 passes (10) at bits 4..5 => 0b00100000
    assert data[2] == 0x20
    assert data[3] == 0x00
    # node 18 passes at bits 4..5 and node 19 lands+passes at bits 6..7
    assert data[4] == 0xE0
    assert data[5:] == b"\x00" * 5


def test_golden_vector_decodes_back_from_bytes() -> None:
    decoded = RouteBits.from_bytes(bytes.fromhex("AA032000E00000000000"), NODES_40)
    assert decoded == golden_route_bits()


# -- encoding --------------------------------------------------------------


def test_from_sets_rejects_a_land_node_that_does_not_pass() -> None:
    with pytest.raises(IllegalBitPairError) as excinfo:
        RouteBits.from_sets(NODES_40, pass_nodes={1, 2}, land_nodes={1, 5})
    assert excinfo.value.node == 5


def test_from_bytes_preserves_illegal_pairs_for_validation() -> None:
    """Decoding must not silently repair bad input; the validator rejects it."""
    bitmap = RouteBits.from_int(0b01, NODES_40)
    assert bitmap.pair(0) is BitPair.ILLEGAL
    assert bitmap.illegal_nodes() == (0,)


def test_from_bytes_requires_the_exact_width() -> None:
    with pytest.raises(ValueError):
        RouteBits.from_bytes(b"\x00" * 9, NODES_40)
    with pytest.raises(ValueError):
        RouteBits.from_bytes(b"\x00" * 13, NODES_48)


def test_width_scales_with_the_node_count() -> None:
    """Open item Q1: 40 nodes give 10 B, 48 nodes give 12 B."""
    assert RouteBits.from_int(0, NODES_40).width_bytes == 10
    assert RouteBits.from_int(0, NODES_48).width_bytes == 12
    assert RouteBits.from_int(0, NODES_40).width_bits == 80
    assert RouteBits.from_int(0, NODES_48).width_bits == 96


def test_with_pair_and_without_are_local_edits() -> None:
    bitmap = golden_route_bits()
    cleared = bitmap.without(4)
    assert cleared.pair(4) is BitPair.ABSENT
    assert cleared.land_nodes == frozenset({19})
    restored = cleared.with_pair(4, BitPair.LAND)
    assert restored == bitmap


def test_int_round_trip_high_node_for_48_node_map() -> None:
    bitmap = RouteBits.from_sets(NODES_48, pass_nodes={0, 47}, land_nodes={47})
    assert bitmap.pair(47) is BitPair.LAND
    assert bitmap.pair(0) is BitPair.PASS
    assert RouteBits.from_bytes(bitmap.to_bytes(), NODES_48) == bitmap


# -- entry -----------------------------------------------------------------


def test_entry_is_one_gpr_pair_at_40_nodes() -> None:
    entry = CalendarRouteEntry(route_bits=golden_route_bits(), expected_rx_bytes=10752)
    payload = entry.to_bytes()
    assert len(payload) == ENTRY_SIZE_BYTES == 16
    assert entry_layout_size_bytes(40) == 16
    assert ROUTE_BITS_NODE_CAPACITY == 40


def test_entry_register_pair_field_boundaries() -> None:
    """Calendar §1.7: rbHi[15:0] routes, [23:16] landCount, [31:24] flags, [63:32] rx."""
    entry = CalendarRouteEntry(
        route_bits=golden_route_bits(),
        expected_rx_bytes=0xDEADBEEF,
        flags=RouteEntryFlags.IS_MEMBER | RouteEntryFlags.SELF_LAND,
    )
    regs = entry.to_regs()
    assert isinstance(regs, CalendarRouteRegs)
    assert regs.land_count == entry.land_count == 2
    assert regs.flags == RouteEntryFlags.IS_MEMBER | RouteEntryFlags.SELF_LAND
    assert regs.expected_rx_bytes == 0xDEADBEEF
    # Only rbHi[15:0] participates in routing.
    assert regs.route_bits_value & 0xFFFF == golden_route_bits().to_int() & 0xFFFF


def test_entry_decodes_from_a_register_pair() -> None:
    entry = CalendarRouteEntry(
        route_bits=golden_route_bits(),
        expected_rx_bytes=10752,
        flags=RouteEntryFlags.IS_MEMBER,
    )
    rebuilt = CalendarRouteEntry.from_regs(entry.to_regs(), NODES_40)
    assert rebuilt.route_bits == entry.route_bits
    assert rebuilt.expected_rx_bytes == entry.expected_rx_bytes
    assert rebuilt.flags == entry.flags


def test_entry_rejects_a_land_count_that_disagrees_with_popcount() -> None:
    with pytest.raises(LandSetError):
        CalendarRouteEntry(route_bits=golden_route_bits(), declared_land_count=3)


def test_entry_rejects_expected_rx_bytes_beyond_uint32() -> None:
    with pytest.raises(WseModelError):
        CalendarRouteEntry(route_bits=golden_route_bits(), expected_rx_bytes=1 << 32)


def test_48_node_entry_cannot_fit_the_gpr_pair_layout() -> None:
    """Open item Q1 made concrete: 12 B of route bits no longer fits 16 B."""
    assert entry_layout_size_bytes(48) == 18
    with pytest.raises(WseModelError) as excinfo:
        CalendarRouteEntry(route_bits=RouteBits.from_int(0, NODES_48)).to_bytes()
    assert "Q1" in str(excinfo.value)
