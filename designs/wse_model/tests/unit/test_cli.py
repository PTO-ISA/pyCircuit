"""The CLI is a stable, scriptable surface."""

from __future__ import annotations

import json

import pytest

from wse_model.cli import main

pytestmark = pytest.mark.unit


def _run(capsys, *argv: str) -> tuple[int, object]:
    code = main(list(argv))
    captured = capsys.readouterr()
    return code, captured


def _json(capsys, *argv: str) -> tuple[int, dict]:
    code, captured = _run(capsys, *argv)
    return code, json.loads(captured.out)


def test_version_exits_zero(capsys) -> None:
    with pytest.raises(SystemExit) as excinfo:
        main(["--version"])
    assert excinfo.value.code == 0
    assert "wse-model" in capsys.readouterr().out


def test_topology_show_emits_the_profile(capsys) -> None:
    code, payload = _json(capsys, "--json", "topology", "show")
    assert code == 0
    assert payload["profile"] == "calendar-40"
    assert payload["node_count"] == 40
    assert payload["route_bits_bits"] == 80
    assert payload["open_item"] == "Q1"


def test_topology_show_links(capsys) -> None:
    code, payload = _json(capsys, "--json", "topology", "show", "--links")
    assert code == 0
    # 5 x 8 mesh: 5*7 vertical + 4*8 horizontal links.
    assert len(payload["links"]) == 5 * 7 + 4 * 8


def test_calendar_encode_reproduces_the_golden_vector(capsys) -> None:
    code, payload = _json(capsys, "--json", "calendar", "encode", "--key", "golden")
    assert code == 0
    assert payload["hex"] == "AA 03 20 00 E0 00 00 00 00 00"
    assert payload["matches_published"] is True
    assert payload["land_nodes"] == [4, 19]


def test_calendar_encode_row_allgather(capsys) -> None:
    code, payload = _json(
        capsys,
        "--json",
        "calendar",
        "encode",
        "--key",
        "row-allgather",
        "--source",
        "0",
    )
    assert code == 0
    assert payload["group"] == [0, 1, 2, 3, 4, 5, 6, 7]
    assert payload["hex"] == "FE FF 00 00 00 00 00 00 00 00"
    assert payload["land_count"] == 7


def test_calendar_encode_col_allgather(capsys) -> None:
    code, payload = _json(
        capsys,
        "--json",
        "calendar",
        "encode",
        "--key",
        "col-allgather",
        "--source",
        "0",
    )
    assert code == 0
    assert payload["group"] == [0, 8, 16, 24]
    assert payload["hex"] == "02 00 03 00 03 00 03 00 00 00"
    assert payload["land_count"] == 3


def test_calendar_encode_rejects_a_non_member_source(capsys) -> None:
    code, captured = _run(capsys, "calendar", "encode", "--key", "row-allgather", "--source", "39")
    assert code == 2
    assert "not an FFN cell" in captured.err


def test_calendar_validate_passes_for_the_builtin_table(capsys) -> None:
    code, payload = _json(capsys, "--json", "calendar", "validate")
    assert code == 0
    assert payload["ok"] is True
    assert payload["key_count"] == 2
    assert payload["table_bytes"] == 1280
    assert "send-receive-conservation" in payload["checks_run"]


def test_calendar_emit_writes_an_aligned_binary_segment(capsys, tmp_path) -> None:
    segment_path = tmp_path / "ffn.rodata"
    code, payload = _json(capsys, "--json", "calendar", "emit", "--out", str(segment_path))
    assert code == 0
    raw = segment_path.read_bytes()
    assert len(raw) == payload["segment_bytes"] == 1280
    assert len(raw) % payload["alignment_bytes"] == 0


def test_calendar_validate_round_trips_a_written_table(capsys, tmp_path) -> None:
    from wse_model.fixtures import ffn_example

    table_path = tmp_path / "ffn.json"
    table_path.write_text(json.dumps(ffn_example().table.to_dict()), encoding="utf-8")
    code, report = _json(capsys, "--json", "calendar", "validate", "--table", str(table_path))
    assert code == 0
    assert report["ok"] is True
    assert report["key_count"] == 2


def test_calendar_validate_reports_a_corrupted_table(capsys, tmp_path) -> None:
    from wse_model.fixtures import ffn_example

    payload = ffn_example().table.to_dict()
    # Corrupt one entry's expectedRxBytes so conservation fails.
    entry = payload["keys"][0]["entries"][0]
    entry["expected_rx_bytes"] = 1
    table_path = tmp_path / "bad.json"
    table_path.write_text(json.dumps(payload), encoding="utf-8")
    code, report = _json(capsys, "--json", "calendar", "validate", "--table", str(table_path))
    assert code == 1
    assert report["ok"] is False
    assert any(error["code"] == "V-CONSERVE" for error in report["errors"])


def test_report_subcommands_return_the_documented_numbers(capsys) -> None:
    code, roofline = _json(capsys, "--json", "report", "roofline")
    assert code == 0
    assert [point["precision"] for point in roofline["points"]] == ["fp16", "fp8", "fp4"]

    code, bandwidth = _json(capsys, "--json", "report", "bandwidth")
    assert code == 0
    assert bandwidth["links"]["ub_fabric"]["GB_per_s"] == 224.0

    code, overhead = _json(capsys, "--json", "report", "overhead")
    assert code == 0
    by_profile = {row["profile"]: row for row in overhead["profiles"]}
    assert by_profile["calendar-40"]["header"]["header_bytes"] == 12
    assert by_profile["whitepaper-48"]["header"]["header_bytes"] == 14

    code, aicore = _json(capsys, "--json", "report", "aicore")
    assert code == 0
    assert aicore["on_chip_buffers"]["UB"] == 384 * 1024


def test_open_items_reports_every_item_and_filters(capsys) -> None:
    code, payload = _json(capsys, "--json", "open-items")
    assert code == 0
    assert payload["total"] == 31
    assert payload["by_resolution"]["open"] > 0

    code, open_only = _json(capsys, "--json", "open-items", "--status", "open")
    assert code == 0
    assert all(item["resolution"] == "open" for item in open_only["items"])
    assert open_only["returned"] == open_only["by_resolution"]["open"]


def test_run_ffn_allgather_completes_both_phases(capsys) -> None:
    code, payload = _json(capsys, "--json", "run", "ffn-allgather")
    assert code == 0
    assert payload["all_complete"] is True
    phases = payload["phases"]
    assert [phase["phase"] for phase in phases] == [
        "phase_b_row_allgather",
        "phase_c_col_allgather",
    ]
    # Phase B and phase C share opcode 1, so the core counters yield 1 then 2.
    assert [phase["epoch"] for phase in phases] == [1, 2]
    assert phases[0]["states"][0]["expVal"] == 10752
    assert phases[1]["states"][0]["expVal"] == 36864
    assert all(state["complete"] for phase in phases for state in phase["states"])


def test_run_ffn_single_phase(capsys) -> None:
    code, payload = _json(capsys, "--json", "run", "ffn-allgather", "--phase", "c")
    assert code == 0
    assert len(payload["phases"]) == 1
    assert payload["phases"][0]["phase"] == "phase_c_col_allgather"
