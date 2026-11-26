"""Frozen values transcribed from the design documents.

A change to any expectation in this file is a design-fidelity change and needs a
reviewed profile update in ``docs/profiles.md``. These are the numbers the documents
publish, so they are the model's contract with its sources.
"""

from __future__ import annotations

import pytest

from wse_model.analysis import balance_point, flit_header_cost, min_batch_to_escape_memory_bound
from wse_model.calendar.collective import OPCODE_RESERVED, OPCODE_WIDTH_BITS, Collective
from wse_model.calendar.entry import ENTRY_SIZE_BYTES
from wse_model.fixtures import (
    FFN_PHASE_B_ROW_BYTES,
    FFN_PHASE_C_ROW_BYTES,
    ffn_example,
    golden_route_bits,
)
from wse_model.topology import CALENDAR_BASELINE, WHITEPAPER_HARDWARE

pytestmark = [pytest.mark.unit]

#: Calendar §2.2.1 consistency anchor.
GOLDEN = "AA 03 20 00 E0 00 00 00 00 00"

#: Calendar §2.2.1 FFN encodings for source N00.
FFN_ROW_ALLGATHER = "FE FF 00 00 00 00 00 00 00 00"
FFN_COL_ALLGATHER = "02 00 03 00 03 00 03 00 00 00"

#: Calendar §2.2.1 expected receive bytes.
FFN_PHASE_B_EXPECTED_RX = 8 * 192 * 7  # 10752
FFN_PHASE_C_EXPECTED_RX = 8 * 1536 * 3  # 36864


def test_golden_route_bits_are_unchanged() -> None:
    assert golden_route_bits().to_hex() == GOLDEN


def test_ffn_key_encodings_are_unchanged() -> None:
    table = ffn_example().table
    assert table.entry(0, 0).route_bits.to_hex() == FFN_ROW_ALLGATHER
    assert table.entry(1, 0).route_bits.to_hex() == FFN_COL_ALLGATHER


def test_ffn_expected_receive_bytes_are_unchanged() -> None:
    table = ffn_example().table
    assert table.entry(0, 0).expected_rx_bytes == FFN_PHASE_B_EXPECTED_RX
    assert table.entry(1, 0).expected_rx_bytes == FFN_PHASE_C_EXPECTED_RX


def test_ffn_land_counts_are_unchanged() -> None:
    table = ffn_example().table
    assert table.entry(0, 0).land_count == 7
    assert table.entry(1, 0).land_count == 3


def test_ffn_geometry_is_unchanged() -> None:
    example = ffn_example()
    assert example.phase_b_geometry.row_bytes == FFN_PHASE_B_ROW_BYTES == 192
    assert example.phase_c_geometry.row_bytes == FFN_PHASE_C_ROW_BYTES == 1536
    assert example.phase_b_geometry.row_count == example.phase_c_geometry.row_count == 8


def test_ffn_non_member_rows_are_all_zero() -> None:
    """Calendar §2.2.1: nodes 32..39 have an all-zero pair for both identities."""
    table = ffn_example().table
    for node in range(32, 40):
        for key_id in (0, 1):
            assert table.entry(key_id, node).route_bits.to_int() == 0


def test_ffn_table_footprint_matches_the_document() -> None:
    """Calendar §3.8.4: 2 x 40 x 16 B = 1280 B, two D-cache lines per core."""
    table = ffn_example().table
    assert table.key_count == 2
    assert table.table_bytes == 1280
    assert table.d_cache_lines_per_core == 2
    assert table.d_cache_bytes_per_core == 128


def test_entry_size_and_opcode_width_are_unchanged() -> None:
    assert ENTRY_SIZE_BYTES == 16
    assert OPCODE_WIDTH_BITS == 3
    assert OPCODE_RESERVED == {0, 7}
    assert [c.opcode for c in Collective if c.is_implemented] == [1, 2]


def test_flit_header_costs_are_unchanged() -> None:
    assert flit_header_cost(node_count=CALENDAR_BASELINE.topology.node_count).header_bytes == 12
    assert flit_header_cost(node_count=WHITEPAPER_HARDWARE.topology.node_count).header_bytes == 14


def test_roofline_numbers_are_unchanged() -> None:
    assert round(balance_point("fp16"), 1) == 11.5
    assert round(balance_point("fp8"), 1) == 45.9
    assert round(balance_point("fp4"), 1) == 91.8
    assert round(min_batch_to_escape_memory_bound("fp16")) == 6
    assert round(min_batch_to_escape_memory_bound("fp8")) == 23
    assert round(min_batch_to_escape_memory_bound("fp4")) == 46


def test_topology_shapes_are_unchanged() -> None:
    assert (CALENDAR_BASELINE.topology.rows, CALENDAR_BASELINE.topology.cols) == (5, 8)
    assert CALENDAR_BASELINE.topology.node_count == 40
    assert (WHITEPAPER_HARDWARE.topology.rows, WHITEPAPER_HARDWARE.topology.cols) == (6, 8)
    assert WHITEPAPER_HARDWARE.topology.node_count == 48


def test_the_two_roofline_verdicts_are_frozen() -> None:
    """The tile-accurate model and the whitepaper's balance point differ at batch 1.

    The whitepaper's first-order analysis (2 x batch FLOP per weight element)
    predicts memory-bound down to batch 6 at FP16. A tile-accurate Cube charges
    for a 1/16-filled M dimension, which makes compute the longer leg at FP16
    batch 1 while FP8 and FP4 remain memory-bound. Both verdicts are pinned so
    that neither can quietly replace the other; see decision 0006.
    """
    from wse_model.core import MatmulShape, matmul_timing

    fp16 = matmul_timing(MatmulShape(1, 4096, 4096), precision_name="fp16")
    assert fp16.first_order_intensity == 2.0
    assert fp16.first_order_is_memory_bound
    assert not fp16.is_memory_bound
    assert fp16.m_utilization == 1 / 16
    assert fp16.effective_macs_per_cycle == 256.0

    for name in ("fp8", "fp4"):
        timing = matmul_timing(MatmulShape(1, 4096, 4096), precision_name=name)
        assert timing.is_memory_bound, name
        assert timing.first_order_is_memory_bound, name
        assert timing.effective_macs_per_cycle > fp16.effective_macs_per_cycle


def test_the_documented_minimum_batches_are_unchanged() -> None:
    """Whichever verdict is used, the published 6 / 23 / 46 must not move."""
    from wse_model.analysis import min_batch_to_escape_memory_bound

    assert round(min_batch_to_escape_memory_bound("fp16")) == 6
    assert round(min_batch_to_escape_memory_bound("fp8")) == 23
    assert round(min_batch_to_escape_memory_bound("fp4")) == 46


def test_red_op_codes_are_frozen_as_a_deliberate_choice() -> None:
    """Calendar §2.4 lists the operators but fixes neither the codes nor their order.

    The model assigns them in listing order, which is a choice rather than a
    transcription, so it is pinned here: a change must be deliberate and cite
    open item C-10, which also gates every reduction path that could use them.
    """
    from wse_model.calendar.collective import ALIGNMENT_SEMAPHORE_WIDTH_BITS, RedOp

    assert [(op.name, int(op)) for op in RedOp] == [
        ("NONE", 0),
        ("SUM", 1),
        ("MAX", 2),
        ("MIN", 3),
        ("PROD", 4),
    ]
    assert not RedOp.NONE.is_active
    # HW-7: the existing 4 bit semaphore cannot count a 32-core phase-B rendezvous.
    assert ALIGNMENT_SEMAPHORE_WIDTH_BITS == 4
