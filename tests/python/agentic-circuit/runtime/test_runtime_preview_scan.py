from __future__ import annotations

import json
from pathlib import Path

from acir_runtime_preview_scan import PreviewMatrixParser, mark_pareto, merge_sources, parse_preview


ROOT = Path(__file__).resolve().parents[4]


def test_preview_parser_reads_design_class_tables() -> None:
    parser = PreviewMatrixParser()
    parser.feed(
        """
        <h2>1. Integer / Fixed</h2>
        <table><tr><th>ID</th><th>Design Class</th><th>Format / Configuration</th><th>Level</th><th>Current</th><th>Priority</th><th>Domain</th></tr>
        <tr><td>INT-01</td><td>Adder</td><td>W8/W16</td><td>L0</td><td>P</td><td>P0</td><td>CPU</td></tr></table>
        """
    )
    assert len(parser.targets) == 1
    assert parser.targets[0]["family"] == "1. Integer / Fixed"
    assert parser.targets[0]["target_id"] == "INT-01"
    assert parser.targets[0]["format"] == "W8/W16"


def test_pareto_is_group_local() -> None:
    rows = [
        {"correctness": "PASS", "synthesis": "PASS", "mapped_area": 10, "logic_depth": 3},
        {"correctness": "PASS", "synthesis": "PASS", "mapped_area": 12, "logic_depth": 4},
        {"correctness": "FAIL", "synthesis": "PASS", "mapped_area": 1, "logic_depth": 1},
    ]
    mark_pareto(rows)
    assert rows[0]["pareto"] is True
    assert rows[1]["pareto"] is False
    assert rows[2]["pareto"] is False


def test_merge_sources_does_not_enable_optional_sources_by_default(tmp_path: Path) -> None:
    base = tmp_path / "base.json"
    extra = tmp_path / "extra.json"
    out = tmp_path / "merged.json"
    base.write_text(json.dumps({"sources": [{"project": "cached", "enabled": True}]}), encoding="utf-8")
    extra.write_text(json.dumps({"sources": [{"project": "remote", "enabled": False}]}), encoding="utf-8")
    merge_sources(base, extra, include_disabled=False, enable_extra=False, output=out)
    assert [item["project"] for item in json.loads(out.read_text(encoding="utf-8"))["sources"]] == ["cached"]

    merge_sources(base, extra, include_disabled=False, enable_extra=True, output=out)
    merged = json.loads(out.read_text(encoding="utf-8"))["sources"]
    assert merged[-1]["project"] == "remote"
    assert merged[-1]["enabled"] is True


def test_real_preview_matrix_is_nonempty() -> None:
    preview = ROOT / "docs" / "runtime" / "preview.html"
    if preview.exists():
        rows = parse_preview(preview)
        assert len(rows) >= 30
        assert any(item["target_id"] == "INT-05" for item in rows)
