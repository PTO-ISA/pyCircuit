"""Exercise the real external NDF CLI on a small independent model contract."""

import json
import os

import pytest
from pycircuit._spec import check_spec


def test_real_ndf_binding_workflow(tmp_path):
    executable = os.environ.get("PYC_TEST_NDF")
    if executable is None:
        pytest.skip("PYC_TEST_NDF must name the pinned external NDF CLI")
    source = tmp_path / "counter"
    source.mkdir()
    (source / "ndf.yaml").write_text(
        'format_version: "0.2"\nproject: counter\nroots: [contract.md]\n'
        "id_prefixes: [COUNT]\ndomains: [core]\npolicies: {}\n"
    )
    (source / "ndf.lock").write_text('format_version: "0.1"\ndependencies: {}\n')
    (source / "contract.md").write_text(
        "---\ndoc_id: COUNT-DOC-001\nstatus: draft\nauthority: normative\nowner: core\n---\n"
        "# Counter\n\n## Behavior {#COUNT-REQ-001}\n"
        "<!-- ndf: kind=requirement modality=must refinement=L1 status=draft -->\n"
        "The count MUST increment once per request.\n\n"
        "## State {#COUNT-STATE-001}\n"
        "<!-- ndf: kind=requirement modality=must refinement=L2 status=draft refines=COUNT-REQ-001 -->\n"
        "One count owner MUST update atomically.\n\n"
        "## Check {#COUNT-VER-001}\n"
        "<!-- ndf: kind=verification modality=must refinement=L3 status=draft verifies=COUNT-STATE-001 -->\n"
        "A test MUST independently check count 0, 1, 2.\n"
    )
    (source / "model.py").write_text(
        "# ndf: implements=COUNT-STATE-001\ndef Counter(count):\n    return count + 1\n"
    )
    (source / "test_model.py").write_text(
        "# ndf: verifies=COUNT-VER-001\ndef test_counter():\n    assert [0, 1, 2] == list(range(3))\n"
    )
    report = check_spec(str(source), executable, str(tmp_path / "pass"))
    assert report["status"] == "PASS"
    assert {b["symbol"] for b in report["bindings"]} == {"Counter", "test_counter"}
    assert report["approval"] == "NOT_RECORDED"
    assert report["semantic_validation"] == "NOT_RUN"
    # A reference that does not resolve must not leave a success receipt.
    (source / "model.py").write_text(
        "# ndf: implements=COUNT-MISSING-001\ndef Counter(count):\n    return count + 1\n"
    )
    with pytest.raises(ValueError):
        check_spec(str(source), executable, str(tmp_path / "reject"))
    failed = json.loads((tmp_path / "reject/spec-check.json").read_text())
    assert failed["status"] == "INCOMPLETE"
    # The external parser can accept a document whose separated metadata was
    # never parsed as clauses. That must not become a traceability PASS.
    contract = source / "contract.md"
    contract.write_text(contract.read_text().replace("}\n<!-- ndf:", "}\n\n<!-- ndf:"))
    with pytest.raises(ValueError):
        check_spec(str(source), executable, str(tmp_path / "empty-clauses"))
    empty = json.loads((tmp_path / "empty-clauses/spec-check.json").read_text())
    assert empty["status"] == "INCOMPLETE"
