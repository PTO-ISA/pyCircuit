from __future__ import annotations

import ast
import os
import subprocess
import tempfile
from pathlib import Path

import agentic_circuit as ac
import pytest

from designs.davincioo.tests.fixtures.contract_probe import contract_probe

ROOT = Path(__file__).resolve().parents[3]
DEFAULT_ACIR = ROOT / ".pycircuit_out/acir/dev-llvm22/bin/acir-opt"


def test_shared_identity_and_operand_invariant_lower_before_frozen_acir() -> None:
    from agentic_circuit._queue_frontend import RULE_LOWERING_PIPELINE

    optimizer = Path(os.environ.get("ACIR_OPT", DEFAULT_ACIR))
    if not optimizer.is_file():
        pytest.skip(f"current-checkout acir-opt is unavailable: {optimizer}")
    specialization = ac.jit(contract_probe, workspace=ROOT)
    raw = specialization.lower_acir()
    assert "ac.var.invariant" in raw
    assert 'name "OperandSourceDescriptor.valid_operand_source"' in raw
    assert 'ac.var.cmp "eq"' in raw
    with tempfile.TemporaryDirectory(prefix="davincioo-contract-") as directory:
        source = Path(directory) / "raw.mlir"
        source.write_text(raw, encoding="utf-8")
        lowered = subprocess.run(
            (
                str(optimizer),
                f"--pass-pipeline={RULE_LOWERING_PIPELINE}",
                str(source),
            ),
            text=True,
            capture_output=True,
            check=False,
        )
    assert lowered.returncode == 0, lowered.stderr
    assert "ac.var.invariant" not in lowered.stdout
    assert "unresolved aggregate comparison" not in lowered.stderr


def test_issue_entry_has_one_canonical_execution_class_owner() -> None:
    source = (ROOT / "designs/davincioo/contracts/spe.py").read_text(encoding="utf-8")
    tree = ast.parse(source)
    issue_entry = next(
        node
        for node in tree.body
        if isinstance(node, ast.ClassDef) and node.name == "IssueEntry"
    )
    fields = [
        node.target.id
        for node in issue_entry.body
        if isinstance(node, ast.AnnAssign) and isinstance(node.target, ast.Name)
    ]
    assert "identity" in fields
    assert "execution_class" not in fields
