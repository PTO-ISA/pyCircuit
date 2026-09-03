from __future__ import annotations

import os
import subprocess
from pathlib import Path

import pytest


@pytest.mark.unit
def test_qemu_vs_pyc_gate_does_not_require_linxcore_for_primary_flow(
    tmp_path: Path,
) -> None:
    root = Path(__file__).resolve().parents[2]
    script = root / "integrations/linx/flows/tools/run_linx_qemu_vs_pyc.sh"
    linx_root = tmp_path / "linx-isa"
    (linx_root / "tools/bringup").mkdir(parents=True)
    source = tmp_path / "smoke.s"
    source.write_text("nop\n", encoding="utf-8")

    env = os.environ.copy()
    env.update(
        {
            "LINX_ROOT": str(linx_root),
            "LINXCORE_ROOT": str(tmp_path / "missing-linxcore"),
            "LLVM_BUILD": str(tmp_path / "missing-llvm"),
            "QEMU_BIN": str(tmp_path / "missing-qemu"),
        }
    )
    result = subprocess.run(
        ["bash", str(script), str(source)],
        check=False,
        capture_output=True,
        env=env,
        text=True,
    )

    assert result.returncode == 2
    assert "llvm-mc not found" in result.stderr
    assert "unable to resolve LinxCore root" not in result.stderr


@pytest.mark.unit
def test_qemu_vs_pyc_gate_has_no_short_prefix_promotion_fallback() -> None:
    root = Path(__file__).resolve().parents[2]
    script = root / "integrations/linx/flows/tools/run_linx_qemu_vs_pyc.sh"
    text = script.read_text(encoding="utf-8")

    assert "trying LinxCore fallback" not in text
    assert "fallback-prefix mode" not in text
    assert "LINX_QEMU_VS_PYC_FALLBACK_PREFIX" not in text
    assert 'DIFF_ARGS+=(--limit "$PREFIX_LIMIT")' not in text
