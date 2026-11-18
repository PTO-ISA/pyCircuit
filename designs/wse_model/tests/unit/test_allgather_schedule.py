"""Independent timing oracles for the declared fixed Calendar experiment.

All numerical expectations below are hand-derived from the declared first-beat
contract. No expected value is read from a generated model or its output.
"""

import json
from pathlib import Path

import pytest
from test_pto_allgather import lowering_inputs as lowering_inputs

from wse_model.analysis.allgather_schedule import prepare_timing
from wse_model.compiler.allgather import lower_calendar

pytestmark = pytest.mark.unit


@pytest.fixture
def timing_inputs(lowering_inputs):  # noqa: F811 -- pytest injects the imported fixture.
    program, transfers = lowering_inputs
    profile = json.loads(
        (
            Path(__file__).resolve().parents[2] / "models/pto_allgather/timing_profile.json"
        ).read_text()
    )
    return program, lower_calendar(program, transfers), profile


def change(profile, name, value):
    profile["parameters"][name]["value"] = value


def test_default_first_beat_calendar_and_phase_boundaries(timing_inputs):
    result = prepare_timing(*timing_inputs)
    # First 128 B complete at 100 ns. Remaining 11/95 beats use the 1.4 GHz
    # core clock, rounded upward to 2 GHz NoC edges: 216 / 336 NoC cycles.
    assert result["abi_values"] == [216, 216, 30, 336, 336, 237, 1, 2, 2, 16, 8, 1, 1024]
    row, column = result["phases"]
    assert [row["source_service_cycles"], column["source_service_cycles"]] == [473, 916]
    assert [row["phase_cycles"], column["phase_cycles"]] == [7665, 4012]
    assert [row["slot_idle_cycles"], column["slot_idle_cycles"]] == [3857, 324]
    assert column["phase_start_cycle"] == row["phase_end_cycle"] == 7665
    assert result["total_noc_cycles"] == 11677
    assert result["predicted_seconds"] == pytest.approx(5838.5e-9)
    assert result["calendar"]["period_noc_cycles"] == 8192
    for phase in result["phases"]:
        for rank, window in enumerate(phase["window_trace"]):
            assert window["slot_start_cycle"] == phase["phase_start_cycle"] + 24 + rank * 1024
            assert window["completion_cycle"] <= window["slot_limit_cycle"]
            intervals = list(window["stages"].values())
            assert intervals[0]["start_cycle"] == window["slot_start_cycle"]
            for previous, following in zip(intervals[:-1], intervals[1:], strict=True):
                assert previous["end_cycle"] == following["start_cycle"]
            assert intervals[-1]["end_cycle"] == window["completion_cycle"]


@pytest.mark.parametrize(
    ("changes", "expected"),
    [
        ({"ub_read_first_beat_ns": 150, "ub_write_first_beat_ns": 150}, 13837),
        ({"ub_read_bytes_per_core_cycle": 64, "ub_write_bytes_per_core_cycle": 64}, 13745),
    ],
)
def test_ub_service_changes_under_one_fixed_1200_cycle_calendar(timing_inputs, changes, expected):
    program, descriptors, profile = timing_inputs
    # Both controls share the SAME larger Calendar. Only the read/write latency
    # pair or the bandwidth pair changes, with their independent ports retained.
    change(profile, "slot_noc_cycles", 1200)
    baseline = prepare_timing(program, descriptors, profile)
    assert baseline["total_noc_cycles"] == 13437
    for name, value in changes.items():
        change(profile, name, value)
    varied = prepare_timing(program, descriptors, profile)
    assert varied["total_noc_cycles"] == expected
    assert varied["calendar"] == baseline["calendar"]
    assert [p["flits_per_source"] for p in varied["phases"]] == [30, 237]


def test_only_hop_latency_changes_last_source_tail(timing_inputs):
    program, descriptors, profile = timing_inputs
    change(profile, "hop_noc_cycles", 2)
    result = prepare_timing(program, descriptors, profile)
    # Earlier source delays consume reserved idle. Only final source tails add
    # 7 cycles in ROW and 3 in COL; no Calendar slot is automatically enlarged.
    assert [p["phase_cycles"] for p in result["phases"]] == [7672, 4015]
    assert result["total_noc_cycles"] == 11687
    assert result["calendar"]["period_noc_cycles"] == 8192


def test_only_calendar_window_changes_source_spacing(timing_inputs):
    program, descriptors, profile = timing_inputs
    change(profile, "slot_noc_cycles", 1200)
    result = prepare_timing(program, descriptors, profile)
    # Seven ROW + three COL inter-source gaps grow by 176 cycles each.
    assert result["total_noc_cycles"] == 11677 + 10 * 176
    assert [p["source_service_cycles"] for p in result["phases"]] == [473, 916]


def test_packet_overrun_rejects_without_resizing_calendar(timing_inputs):
    program, descriptors, profile = timing_inputs
    # Keep the default 1024-cycle window and all other parameters fixed.
    # Raising both first-beat latencies makes the COL packet need 1116 cycles.
    change(profile, "ub_read_first_beat_ns", 150)
    change(profile, "ub_write_first_beat_ns", 150)
    with pytest.raises(ValueError, match="exceeds fixed Calendar slot"):
        prepare_timing(program, descriptors, profile)
    assert profile["parameters"]["slot_noc_cycles"]["value"] == 1024


def test_coordinate_rank_mapping_is_part_of_fixed_calendar(timing_inputs):
    program, descriptors, profile = timing_inputs
    # Preserve nodes, routes and byte counts; change only two rank assignments.
    group = program["phases"][0]["groups"][0]
    group[0], group[1] = group[1], group[0]
    with pytest.raises(ValueError):
        prepare_timing(program, descriptors, profile)


def test_provisional_values_retain_pending_status(timing_inputs):
    profile = timing_inputs[2]
    documented = {"noc_clock_hz", "core_clock_hz", "link_bytes_per_noc_cycle"}
    for name, entry in profile["parameters"].items():
        assert entry["status"] == ("documented" if name in documented else "pending")
        assert entry["source"]
