from __future__ import annotations

import pycircuit
import pytest

from examples.pycircuit.linxcore_frontend_pipeline.linxcore_frontend_pipeline import (
    STAGE_ORDER,
    build,
    reference_split_window,
)

pytestmark = pytest.mark.unit


def test_reference_split_window_handles_mixed_linx_lengths() -> None:
    # 2 + 4 + 6 + 4 bytes. Low nibbles encode the Linx length rule.
    data = bytes(
        [0x00, 0xA0]
        + [0x01, 0xB1, 0xB2, 0xB3]
        + [0x0E, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5]
        + [0x01, 0xD1, 0xD2, 0xD3]
    )

    rows = reference_split_window(data)

    assert [length for _, length in rows] == [2, 4, 6, 4]


def test_reference_split_window_stops_at_first_incomplete_instruction() -> None:
    data = bytes([0x0F, 1, 2, 3, 4, 5, 6])

    assert reference_split_window(data) == []


def test_domain_aware_pipeline_emits_explicit_stage_state_without_balance_regs() -> (
    None
):
    design = pycircuit.compile_cycle_aware(
        build,
        name="linxcore_frontend_pipeline_test",
        stid_width=2,
        pe_width=2,
        rid_width=8,
    )
    mlir = design.emit_mlir()

    for stage in STAGE_ORDER:
        assert f"{stage}__valid" in mlir
        assert f"stage__{stage}__valid" in mlir

    # Temporal coordinates are explicit in source, while real transport cuts
    # are the named state registers. No accidental auto-balance pipeline is
    # needed to close the ready/valid feedback path.
    assert "_v6_bal_" not in mlir
    assert mlir.count("pyc.reg") >= len(STAGE_ORDER)
    assert "out_reservation_token" in mlir
