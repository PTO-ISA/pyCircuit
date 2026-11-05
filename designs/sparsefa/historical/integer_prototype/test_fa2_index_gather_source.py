"""Source-level checks for the integer sparse gather authoring slice."""

from pathlib import Path

SOURCE = Path(__file__).with_name("fa2_index_gather.py")


def test_sparse_gather_source_has_required_modules() -> None:
    text = SOURCE.read_text(encoding="utf-8")
    for name in (
        "IndexStore",
        "IndexValidator",
        "KVTokenMap",
        "GatherAddress",
        "SparseIndexGather",
    ):
        assert f"def {name}(" in text


def test_sparse_gather_source_has_no_physical_clock_or_reset_ports() -> None:
    text = SOURCE.read_text(encoding="utf-8")
    assert "clk:" not in text
    assert "reset:" not in text
    assert "DFF" not in text


def test_sparse_gather_source_rejects_duplicate_and_oversized_rows() -> None:
    text = SOURCE.read_text(encoding="utf-8")
    assert "duplicate_hit" in text
    assert "count_error" in text
    assert "write_count <= 16" in text
