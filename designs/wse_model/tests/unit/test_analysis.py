"""Roofline, bandwidth, and the open-item gate."""

from __future__ import annotations

import json

import pytest

from wse_model.analysis import (
    ON_CHIP_BUFFERS,
    AicoreSpec,
    LatencyBudget,
    arithmetic_intensity_decode,
    balance_point,
    flit_header_cost,
    local_dram_vs_noc_ratio,
    min_batch_to_escape_memory_bound,
    precision,
    roofline_table,
    two_phase_allgather_bytes,
)
from wse_model.analysis.bandwidth import BANDWIDTHS
from wse_model.analysis.dcache import dcache_budget
from wse_model.errors import OpenItemError, WseModelError
from wse_model.open_items import OPEN_ITEMS, Resolution, open_item, require_resolved

pytestmark = pytest.mark.unit


# -- roofline --------------------------------------------------------------


def test_cube_flops_match_the_whitepaper_spec_table() -> None:
    """Whitepaper §3.1: 11.5 / 45.9 / 91.8 TFLOPS at 1.4 GHz."""
    assert round(precision("fp16").tflops(), 1) == 11.5
    assert round(precision("fp8").tflops(), 1) == 45.9
    assert round(precision("fp4").tflops(), 1) == 91.8


def test_mac_ratio_is_one_to_four_to_eight() -> None:
    fp16 = precision("fp16").macs_per_cycle
    assert precision("fp8").macs_per_cycle == 4 * fp16
    assert precision("fp4").macs_per_cycle == 8 * fp16


def test_balance_points_and_minimum_batch() -> None:
    """Whitepaper §1 and §19.2: 11.5 / 45.9 / 91.8 FLOP/Byte, batch 6 / 23 / 46."""
    assert round(balance_point("fp16"), 1) == 11.5
    assert round(min_batch_to_escape_memory_bound("fp16")) == 6
    assert round(min_batch_to_escape_memory_bound("fp8")) == 23
    assert round(min_batch_to_escape_memory_bound("fp4")) == 46


def test_decode_intensity_is_twice_the_batch() -> None:
    assert arithmetic_intensity_decode(1) == 2.0
    assert arithmetic_intensity_decode(32) == 64.0


def test_small_batch_decode_is_memory_bound_and_large_batch_is_not() -> None:
    fp16 = roofline_table()[0]
    assert fp16.is_memory_bound(1)
    assert fp16.is_memory_bound(5)
    assert not fp16.is_memory_bound(7)


def test_precision_lookup_rejects_unknown_names() -> None:
    with pytest.raises(WseModelError):
        precision("bf16")


# -- bandwidth -------------------------------------------------------------


def test_the_four_published_bandwidths() -> None:
    """Whitepaper §4.4: 1 TB/s, 128 GB/s, 224 GB/s, 14 GB/s."""
    assert BANDWIDTHS["local_dram"].gb_per_s == 1000.0
    assert BANDWIDTHS["noc_link"].gb_per_s == 128.0
    assert BANDWIDTHS["ub_fabric"].gb_per_s == 224.0
    assert BANDWIDTHS["ub_host"].gb_per_s == 14.0


def test_local_dram_is_about_eight_noc_links() -> None:
    """Whitepaper §4.4: the constraint behind slicing the N axis."""
    assert local_dram_vs_noc_ratio() == 7.8125


def test_flit_header_overhead_across_the_q1_readings() -> None:
    forty = flit_header_cost(node_count=40)
    forty_eight = flit_header_cost(node_count=48)
    assert forty.header_bytes == 12
    assert round(forty.overhead_fraction * 100, 2) == 18.75
    assert forty_eight.header_bytes == 14
    assert round(forty_eight.overhead_fraction * 100, 2) == 21.88


def test_ffn_two_phase_payload_matches_the_published_expected_bytes() -> None:
    payload = two_phase_allgather_bytes(
        row_count=8,
        phase_b_row_bytes=192,
        phase_c_row_bytes=1536,
        row_member_count=8,
        col_member_count=4,
        node_count=40,
    )
    assert payload["payload_bytes_per_destination"]["phase_b"] == 10752
    assert payload["payload_bytes_per_destination"]["phase_c"] == 36864


def test_aicore_cycle_budget_matches_the_design_note() -> None:
    """Whitepaper §3.1: 512 B of weights per cycle against about 714 B supplied."""
    spec = AicoreSpec()
    assert 700 < spec.local_dram_bytes_per_cycle < 720
    assert spec.weights_per_cycle_fp16 == 256
    assert spec.weights_per_cycle_fp16 * 2 == 512
    assert spec.vector_elements_per_cycle_fp16 == 128


def test_on_chip_buffer_sizes_match_the_whitepaper() -> None:
    assert ON_CHIP_BUFFERS == {
        "L0A": 128 * 1024,
        "L0B": 512 * 1024,
        "L0C": 256 * 1024,
        "L1": 1024 * 1024,
        "UB": 384 * 1024,
    }


# -- latency ---------------------------------------------------------------


def test_latency_budget_refuses_a_total_without_the_open_inputs() -> None:
    """Q3 and Q9 have no values in the design sources, so no total is produced."""
    budget = LatencyBudget(device_transfer_bytes=1024, compute_weight_bytes=1024)
    with pytest.raises(OpenItemError) as excinfo:
        budget.total_seconds()
    assert "Q3" in str(excinfo.value) or "Q9" in str(excinfo.value)
    described = budget.describe()
    assert described["complete"] is False


def test_latency_budget_totals_once_the_open_inputs_are_declared() -> None:
    budget = LatencyBudget(
        device_transfer_bytes=224_000,
        compute_weight_bytes=1_000_000,
        noc_bytes=128_000,
        batcher_seconds=1e-6,
        round_trip_seconds=2e-6,
        tail_seconds=5e-7,
    )
    total = budget.total_seconds()
    assert total > 0
    blocks = budget.blocks()
    assert len(blocks) == 5
    assert budget.describe()["complete"] is True
    assert budget.dominant_block().seconds > 0


# -- open items ------------------------------------------------------------


def test_whitepaper_and_calendar_items_are_all_registered() -> None:
    expected = {f"Q{i}" for i in range(1, 13)}
    expected |= {f"S-{i}" for i in range(1, 8)}
    expected |= {f"C-{i}" for i in range(1, 13)}
    assert expected <= set(OPEN_ITEMS)


def test_every_adopted_reading_names_a_decision_record() -> None:
    for item in OPEN_ITEMS.values():
        if item.resolution is Resolution.ASSUMED:
            assert item.decision, f"{item.id} is ASSUMED without a decision record"


def test_require_resolved_raises_for_open_items_and_passes_for_assumed_ones() -> None:
    with pytest.raises(OpenItemError) as excinfo:
        require_resolved("Q1", consumer="unit test")
    assert "Q1" in str(excinfo.value)
    assert require_resolved("C-2", consumer="unit test").id == "C-2"


def test_open_item_lookup_rejects_unknown_ids() -> None:
    with pytest.raises(KeyError):
        open_item("Q99")


# -- D-cache budget --------------------------------------------------------


def test_ffn_d_cache_residency_matches_the_document() -> None:
    """Calendar §3.8.4: 2 lines key-major, 1 node-major, 128 B / 64 B."""
    from wse_model.calendar.table import TableLayout

    key_major = dcache_budget(key_count=2, node_count=40, layout=TableLayout.KEY_MAJOR)
    node_major = dcache_budget(key_count=2, node_count=40, layout=TableLayout.NODE_MAJOR)
    assert key_major.lines_per_core == 2
    assert key_major.bytes_per_core == 128
    assert node_major.lines_per_core == 1
    assert node_major.bytes_per_core == 64


def test_die_wide_line_count_and_refill_split() -> None:
    """Calendar §3.8.4: 20 distinct lines, 80 requests, only 20 reach DDR."""
    residency = dcache_budget(key_count=2, node_count=40)
    assert residency.lines_per_key == 10
    assert residency.lines_die_wide == 20
    assert residency.refill_requests == 80
    assert residency.ddr_backed_refills == 20
    assert residency.batcher_served_refills == 60


def test_node_major_halves_the_refill_requests_at_keycount_two() -> None:
    """The transpose is free: same bytes, same instructions, halved requests."""
    from wse_model.calendar.table import TableLayout

    node_major = dcache_budget(key_count=2, node_count=40, layout=TableLayout.NODE_MAJOR)
    assert node_major.refill_requests == 40
    assert node_major.ddr_backed_refills == 20


def test_transposed_layout_holds_four_times_more_keys_in_one_line() -> None:
    from wse_model.calendar.table import TableLayout

    key_major = dcache_budget(key_count=16, node_count=40, layout=TableLayout.KEY_MAJOR)
    node_major = dcache_budget(key_count=16, node_count=40, layout=TableLayout.NODE_MAJOR)
    assert key_major.lines_per_core == 16
    assert node_major.lines_per_core == 4
    # Both are inside the keyCount <= 16 budget, but only one stays there as
    # keyCount grows.
    assert key_major.within_budget()
    assert node_major.within_budget()


def test_amplification_is_the_entry_to_line_ratio() -> None:
    residency = dcache_budget(key_count=2, node_count=40)
    assert residency.amplification == 4.0
    assert residency.spec.entries_per_line == 4
    assert residency.spec.lines == 256
    assert round(residency.residency_fraction * 100, 4) == 0.7812


def test_dcache_report_is_available_from_the_cli(capsys) -> None:
    from wse_model.cli import main

    assert main(["--json", "report", "dcache"]) == 0
    payload = json.loads(capsys.readouterr().out)
    ffn = next(
        entry
        for entry in payload["entries"]
        if entry["layout"] == "key-major" and entry["key_count"] == 2
    )
    assert ffn["lines_per_core"] == 2
    assert ffn["refill_requests"] == 80
    assert ffn["ddr_backed_refills"] == 20
