from __future__ import annotations

import ast
import os
import subprocess
import tempfile
from pathlib import Path

import agentic_circuit as ac
import pytest

from designs.davincioo.spe.iex.wba import wba_system

ROOT = Path(__file__).resolve().parents[5]
DEFAULT_ACIR = ROOT / ".pycircuit_out/acir/dev-llvm22/bin/acir-opt"


def test_wba_uses_nominal_attempt_equality_without_expanded_leaf_chains() -> None:
    source_path = ROOT / "designs/davincioo/spe/iex/wba.py"
    source = source_path.read_text(encoding="utf-8")
    tree = ast.parse(source)
    comparisons = [node for node in ast.walk(tree) if isinstance(node, ast.Compare)]
    aggregate_attempt_comparisons = [
        node
        for node in comparisons
        if "attempt" in ast.unparse(node) and "==" in ast.unparse(node)
    ]
    assert len(aggregate_attempt_comparisons) >= 6
    for legacy_field in (
        ".core_id ==",
        ".pe_id ==",
        ".instruction_sequence ==",
        ".original_pc ==",
        ".attempt_generation ==",
    ):
        assert legacy_field not in source


def test_wba_source_closure_reaches_frozen_queuegraph_and_rejects_table_pyc() -> None:
    from agentic_circuit._queue_frontend import RULE_LOWERING_PIPELINE

    optimizer = Path(os.environ.get("ACIR_OPT", DEFAULT_ACIR))
    cxxgen = optimizer.with_name("acir-queue-cxxgen")
    pycgen = optimizer.with_name("acir-queue-pycgen")
    compiler = os.environ.get("CXX", "c++")
    if not optimizer.is_file():
        pytest.skip(f"current-checkout acir-opt is unavailable: {optimizer}")
    if not cxxgen.is_file() or not pycgen.is_file():
        pytest.skip("current-checkout QueueGraph generators are unavailable")
    specialization = ac.jit(wba_system, workspace=ROOT)
    raw = specialization.lower_acir()
    assert raw.count('ac.var.cmp "eq"') >= 6
    assert "ac.var.decl @entries" in raw
    assert "ac.var.decl @cancel_tombstones" in raw
    with tempfile.TemporaryDirectory(prefix="davincioo-wba-") as directory:
        raw_path = Path(directory) / "raw.mlir"
        frozen_path = Path(directory) / "frozen.mlir"
        raw_path.write_text(raw, encoding="utf-8")
        lowered = subprocess.run(
            (
                str(optimizer),
                f"--pass-pipeline={RULE_LOWERING_PIPELINE}",
                str(raw_path),
                "-o",
                str(frozen_path),
            ),
            text=True,
            capture_output=True,
            check=False,
        )
        assert lowered.returncode == 0, lowered.stderr
        frozen = frozen_path.read_text(encoding="utf-8")
        assert "ac.var.invariant" not in frozen
        assert "ac.firing" in frozen
        assert "ac.table" in frozen

        generated = subprocess.run(
            (str(cxxgen), str(frozen_path)),
            text=True,
            capture_output=True,
            check=False,
        )
        assert generated.returncode == 0, generated.stderr
        model_path = Path(directory) / "model.cpp"
        model_path.write_text(
            generated.stdout
            + """
int main() {
  ac_generated::WbaSystem model;
  auto rows = model.dispatch_rows();
  const gfsim::Epoch epoch{1, 0};
  for (auto &row : rows) row.work(row.object, epoch);
  for (auto &row : rows)
    row.xfer(row.object, epoch, gfsim::XferPhase::Arbitrate);
  for (auto &row : rows)
    row.xfer(row.object, epoch, gfsim::XferPhase::Probe);
  for (auto &row : rows)
    row.xfer(row.object, epoch, gfsim::XferPhase::Commit);
  return model.sink_0_values().empty() && model.sink_1_values().empty() &&
                 model.sink_2_values().empty() && model.sink_3_values().empty()
             ? 0
             : 1;
}
""",
            encoding="utf-8",
        )
        executable = Path(directory) / "model"
        compiled = subprocess.run(
            (
                compiler,
                "-std=c++20",
                "-I",
                str(ROOT / "simulator/gfsim/include"),
                str(model_path),
                "-o",
                str(executable),
            ),
            text=True,
            capture_output=True,
            check=False,
        )
        assert compiled.returncode == 0, compiled.stderr
        executed = subprocess.run(
            (str(executable),), text=True, capture_output=True, check=False
        )
        assert executed.returncode == 0, executed.stderr

        pyc = subprocess.run(
            (str(pycgen), str(frozen_path)),
            text=True,
            capture_output=True,
            check=False,
        )
        assert pyc.returncode != 0
        assert (
            "module-preserving QueueGraph PYC lowering is not implemented" in pyc.stderr
            or "unsupported provisional Table" in pyc.stderr
        )
