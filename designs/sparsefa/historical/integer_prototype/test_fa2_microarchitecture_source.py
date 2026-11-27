"""Source-level checks for the executable FA2 hardware boundaries."""

from pathlib import Path

SOURCE = Path(__file__).with_name("fa2_microarchitecture.py")


def test_microarchitecture_publishes_all_hardware_boundaries() -> None:
    text = SOURCE.read_text(encoding="utf-8")
    for name in (
        "LaneAddressExpander",
        "TransferTagTable",
        "AttentionController",
        "RowStateCommit",
        "FlashAttention2Microarchitecture",
    ):
        assert f"def {name}(" in text


def test_microarchitecture_keeps_physical_controls_compiler_owned() -> None:
    text = SOURCE.read_text(encoding="utf-8")
    assert "clk:" not in text
    assert "reset:" not in text
    assert "DFF" not in text


def test_microarchitecture_retains_identity_and_atomic_commit_fields() -> None:
    text = SOURCE.read_text(encoding="utf-8")
    for field in ("operation", "generation", "response_last", "row_update_valid"):
        assert field in text
