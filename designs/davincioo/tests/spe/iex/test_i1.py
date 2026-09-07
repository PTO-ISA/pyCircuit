from __future__ import annotations

import ast
import os
import subprocess
import tempfile
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[5]
DEFAULT_ACIR = ROOT / ".pycircuit_out/acir/dev-llvm22/bin/acir-opt"
CONTRACTS = ROOT / "designs/davincioo/contracts/spe.py"
I1_SOURCE = ROOT / "designs/davincioo/spe/iex/i1.py"


def _lower_i1() -> str:
    from agentic_circuit._queue_frontend import lower_queue_source

    statements: list[ast.stmt] = []
    for path in (CONTRACTS, I1_SOURCE):
        tree = ast.parse(path.read_text(encoding="utf-8"), filename=str(path))
        statements.extend(
            statement
            for statement in tree.body
            if not isinstance(statement, ast.Import | ast.ImportFrom)
        )
    statements.extend(
        ast.parse(
            """
@ac.system
def i1_system(
    selected: IssueAttempt,
    cancel: IssueCancel,
    read_decision: OperandReadDecision,
) -> tuple[OperandReadRequest, OperandReadDecisionAck, IssueCancelAck]:
    read_request, read_decision_ack, cancel_ack = i1(
        selected, cancel, read_decision
    )
    return read_request, read_decision_ack, cancel_ack
"""
        ).body
    )
    source = ast.unparse(ast.fix_missing_locations(ast.Module(statements, [])))
    return lower_queue_source(source, "i1_system")


def test_i1_uses_canonical_identity_and_operand_invariant() -> None:
    source = I1_SOURCE.read_text(encoding="utf-8")
    tree = ast.parse(source)
    comparisons = [node for node in ast.walk(tree) if isinstance(node, ast.Compare)]

    assert len(comparisons) == 9
    assert "selected.key.identity == selected.entry.identity" in source
    assert "pending.key == request.key" in source
    assert source.count("valid_operand_source(") == 2
    assert ".core_id ==" not in source
    assert ".attempt_generation ==" not in source
    assert "OperandReadResponse" not in source
    assert "isq_release" not in source


def test_i1_source_closure_owns_only_contract_and_leaf_sources() -> None:
    from agentic_circuit._source_closure import capture_source_closure

    closure = capture_source_closure(I1_SOURCE, ROOT)
    assert tuple(entry.path for entry in closure.entries) == (
        "designs/davincioo/contracts/spe.py",
        "designs/davincioo/spe/iex/i1.py",
    )


def test_i1_lowers_value_contracts_and_atomic_state_rules() -> None:
    from agentic_circuit._queue_frontend import RULE_LOWERING_PIPELINE

    optimizer = Path(os.environ.get("ACIR_OPT", DEFAULT_ACIR))
    if not optimizer.is_file():
        pytest.skip(f"current-checkout acir-opt is unavailable: {optimizer}")
    raw = _lower_i1()
    assert 'name "OperandSourceDescriptor.valid_operand_source"' in raw
    assert 'ac.var.cmp "eq"' in raw
    assert "!ac.var<!ac.struct<@types::@IssueIdentity>>" in raw
    assert "!ac.var<!ac.struct<@types::@IssueAttemptKey>>" in raw
    assert raw.count("ac.rule ") == 4
    assert "ac.var.assign @active" in raw
    assert "ac.var.assign @pending" in raw
    assert "ac.var.assign @rf_read_mask" in raw
    assert "ac.var.assign @forward_mask" in raw
    assert "ac.var.assign @read_request_outstanding" in raw
    assert raw.index('name "accept_read_decision"') < raw.index('name "cancel_attempt"')

    with tempfile.TemporaryDirectory(prefix="davincioo-i1-") as directory:
        raw_path = Path(directory) / "raw.mlir"
        raw_path.write_text(raw, encoding="utf-8")
        lowered = subprocess.run(
            (
                str(optimizer),
                f"--pass-pipeline={RULE_LOWERING_PIPELINE}",
                str(raw_path),
            ),
            text=True,
            capture_output=True,
            check=False,
        )
    assert lowered.returncode == 0, lowered.stderr
    assert "ac.var.invariant" not in lowered.stdout
    assert lowered.stdout.count("ac.firing") >= 4
    assert "ac.output_presence" in lowered.stdout


def test_i1_generates_current_checkout_gfsim_model() -> None:
    from agentic_circuit._jit import _lower_acir_to_cpp

    optimizer = Path(os.environ.get("ACIR_OPT", DEFAULT_ACIR))
    if (
        not optimizer.is_file()
        or not optimizer.with_name("acir-queue-cxxgen").is_file()
    ):
        pytest.skip("current-checkout ACIR/gfsim toolchain is unavailable")
    generated = _lower_acir_to_cpp(_lower_i1())

    assert "class I1" in generated or "class i1" in generated
    assert "read_request_outstanding" in generated
    assert "OperandReadDecisionAck" in generated
    assert "IssueCancelAck" in generated
    with tempfile.TemporaryDirectory(prefix="davincioo-i1-gfsim-") as directory:
        source = Path(directory) / "model.cpp"
        executable = Path(directory) / "model"
        source.write_text(
            generated
            + """
int main() {
  ac_generated::I1System model;
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
                 model.sink_2_values().empty()
             ? 0
             : 1;
}
""",
            encoding="utf-8",
        )
        compiled = subprocess.run(
            (
                "c++",
                "-std=c++20",
                "-I",
                str(ROOT / "simulator/gfsim/include"),
                str(source),
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
