"""Run the real pinned NDF CLI against all generated starter projects."""

from __future__ import annotations

import os
import subprocess
from pathlib import Path

import pytest
from pycircuit._project import init_project
from pycircuit._spec import check_spec

pytestmark = pytest.mark.system


@pytest.mark.parametrize("template", ["compute", "stateful", "backpressure"])
def test_generated_starter_passes_real_ndf(tmp_path, template):
    executable = os.environ.get("PYC_TEST_NDF")
    if not executable:
        pytest.skip("PYC_TEST_NDF must name the pinned external NDF CLI")
    binary = Path(executable).resolve(strict=True)
    source = os.environ.get("PYC_TEST_NDF_SOURCE")
    if source is None:
        result = subprocess.run(
            ["git", "-C", str(binary.parent), "rev-parse", "--show-toplevel"],
            capture_output=True,
            text=True,
            timeout=10,
        )
        if result.returncode:
            pytest.skip("PYC_TEST_NDF_SOURCE must name the pinned NDF checkout")
        source = result.stdout.strip()
    project = init_project(tmp_path / template, template)
    dependency = project / ".pycircuit_out/ndf"
    dependency.parent.mkdir()
    dependency.symlink_to(Path(source).resolve(strict=True), target_is_directory=True)
    report = check_spec(str(project), str(binary), str(tmp_path / "evidence"))
    assert report["status"] == "PASS"
    assert report["approval"] == "NOT_RECORDED"
    assert report["semantic_validation"] == "NOT_RUN"
    assert len(report["bindings"]) == 3
    coverage = report["agent_gate"]["coverage_bound"]
    assert coverage["interpretation"] == "NOT_APPLICABLE_NO_ACTIVE_L1_MUST"
    assert coverage["draft_l1_must"] == ["ndf://telemetry-model/MODEL-REQ-001"]
