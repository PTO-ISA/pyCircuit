from __future__ import annotations

import ast
import json
import os
import shutil
import subprocess
import tempfile
from pathlib import Path

import agentic_circuit as ac
import pytest

from designs.davincioo.spe.iex.i2 import i2_system

ROOT = Path(__file__).resolve().parents[5]
DEFAULT_ACIR = ROOT / ".pycircuit_out/acir/dev-llvm22/bin/acir-opt"
DEFAULT_PLAN = ROOT / ".pycircuit_out/acir/dev-llvm22/bin/acir-queue-plan"
DEFAULT_CXXGEN = ROOT / ".pycircuit_out/acir/dev-llvm22/bin/acir-queue-cxxgen"
I2_SOURCE = ROOT / "designs/davincioo/spe/iex/i2.py"


def test_i2_uses_shared_value_contracts_and_stays_below_comparison_budget() -> None:
    tree = ast.parse(I2_SOURCE.read_text(encoding="utf-8"))
    comparisons = [node for node in ast.walk(tree) if isinstance(node, ast.Compare)]
    assert len(comparisons) <= 80

    source = I2_SOURCE.read_text(encoding="utf-8")
    assert "row.key == key" in source
    assert "pending.key == request.consumer" in source
    assert "source == request.producer" in source
    assert "valid_operand_source(entry.src0)" in source
    assert "was_inactive = not active" in source
    assert "if was_inactive:" in source
    assert "pending_key.flow.core_id" not in source
    assert "load_attempt_generation ==" not in source


def test_i2_lowers_value_contracts_before_frozen_acir() -> None:
    from agentic_circuit._queue_frontend import RULE_LOWERING_PIPELINE

    optimizer = Path(os.environ.get("ACIR_OPT", DEFAULT_ACIR))
    planner = Path(os.environ.get("ACIR_QUEUE_PLAN", DEFAULT_PLAN))
    cxxgen = Path(os.environ.get("ACIR_QUEUE_CXXGEN", DEFAULT_CXXGEN))
    cxx = shutil.which("c++")
    if any(not path.is_file() for path in (optimizer, planner, cxxgen)) or cxx is None:
        pytest.skip("current-checkout ACIR/gfsim toolchain is unavailable")
    specialization = ac.jit(i2_system, workspace=ROOT)
    raw = specialization.lower_acir()
    assert "ac.var.invariant" in raw
    assert 'name "OperandSourceDescriptor.valid_operand_source"' in raw
    assert 'ac.var.cmp "eq"' in raw
    assert raw.count("ac.rule ") == 7
    with tempfile.TemporaryDirectory(prefix="davincioo-i2-") as directory:
        source = Path(directory) / "raw.mlir"
        frozen = Path(directory) / "frozen.mlir"
        generated = Path(directory) / "i2.cpp"
        executable = Path(directory) / "i2"
        source.write_text(raw, encoding="utf-8")
        lowered = subprocess.run(
            (
                str(optimizer),
                f"--pass-pipeline={RULE_LOWERING_PIPELINE}",
                str(source),
                "-o",
                str(frozen),
            ),
            text=True,
            capture_output=True,
            check=False,
        )
        assert lowered.returncode == 0, lowered.stderr
        frozen_text = frozen.read_text(encoding="utf-8")
        assert "ac.var.invariant" not in frozen_text
        assert "unresolved aggregate comparison" not in lowered.stderr
        planned = subprocess.run(
            (str(planner), str(frozen)),
            text=True,
            capture_output=True,
            check=False,
        )
        assert planned.returncode == 0, planned.stderr
        plan = json.loads(planned.stdout)
        assert len(plan["module_specializations"]) == 1
        assert len(plan["module_specializations"][0]["blocks"]) == 7
        emitted = subprocess.run(
            (str(cxxgen), str(frozen)),
            text=True,
            capture_output=True,
            check=False,
        )
        assert emitted.returncode == 0, emitted.stderr
        generated.write_text(
            emitted.stdout
            + """
int main() {
  ac_generated::I2System model;
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
                 model.sink_2_values().empty() &&
                 model.sink_3_values().empty() &&
                 model.sink_4_values().empty() &&
                 model.sink_5_values().empty() &&
                 model.sink_6_values().empty()
             ? 0
             : 1;
}
""",
            encoding="utf-8",
        )
        built = subprocess.run(
            (
                cxx,
                "-std=c++20",
                f"-I{ROOT / 'simulator/gfsim/include'}",
                str(generated),
                "-o",
                str(executable),
            ),
            text=True,
            capture_output=True,
            check=False,
        )
        assert built.returncode == 0, built.stderr
        executed = subprocess.run(
            (str(executable),), text=True, capture_output=True, check=False
        )
        assert executed.returncode == 0, executed.stderr
