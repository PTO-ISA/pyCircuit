"""Exercise the real external NDF CLI on a small independent model contract."""

import json
import os
import shutil

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
        "# Counter\n\n## Intent {#COUNT-INTENT-001}\n"
        "<!-- ndf: kind=requirement modality=must refinement=L0 status=draft -->\n"
        "The model MUST count accepted requests.\n\n"
        "## Behavior {#COUNT-REQ-001}\n"
        "<!-- ndf: kind=requirement modality=must refinement=L1 status=draft refines=COUNT-INTENT-001 -->\n"
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
    assert report["agent_gate"]["binding_diff"]["moved"] == []
    coverage = report["agent_gate"]["coverage_bound"]
    assert coverage["eligible_active_l1_must"] == []
    assert coverage["draft_l1_must"] == ["ndf://counter/COUNT-REQ-001"]
    assert coverage["interpretation"] == "NOT_APPLICABLE_NO_ACTIVE_L1_MUST"
    baseline = tmp_path / "first-draft"
    shutil.copytree(source, baseline)
    # Standard evidence must remain in the bound graph without promoting draft.
    manifest = source / "ndf.yaml"
    manifest.write_text(
        manifest.read_text().replace("[contract.md]", "[contract.md, evidence.json]")
    )
    (source / "evidence.json").write_text(
        json.dumps(
            {
                "format_version": "0.1",
                "id": "COUNT-EVID-001",
                "command": "fixture only",
                "result": "inconclusive",
                "tool_version": "fixture",
                "source_revision": "0" * 40,
                "timestamp": "2026-11-02T00:00:00Z",
                "links": [{"clause": "COUNT-VER-001", "type": "evidenced-by"}],
            }
        )
    )
    evidence = check_spec(
        str(source), executable, str(tmp_path / "with-evidence"), str(baseline)
    )
    assert evidence["status"] == "PASS"
    assert evidence["approval"] == "NOT_RECORDED"
    assert evidence["clauses"] == report["clauses"] + 1
    difference = evidence["agent_gate"]["version_comparison"]["spec_diff"]
    assert difference["added"] == ["ndf://counter/COUNT-EVID-001"]
    assert difference["modified"] == difference["moved"] == []
    trace = evidence["agent_gate"]["trace"]
    assert "ndf://counter/COUNT-INTENT-001" in trace["nodes"]
    assert "ndf://counter/COUNT-EVID-001" in trace["nodes"]
    assert {edge["type"] for edge in trace["edges"]} >= {
        "references",
        "implements",
        "verifies",
        "refines",
        "evidenced-by",
    }
    # Native coverage is not the adapter's transitive draft-binding check.
    # Activating L1 does not invent a direct verifies edge from its L2 check.
    contract = source / "contract.md"
    original = contract.read_text()
    contract.write_text(
        original.replace("refinement=L1 status=draft", "refinement=L1 status=active")
    )
    active = check_spec(str(source), executable, str(tmp_path / "active-gap"))
    active_coverage = active["agent_gate"]["coverage_bound"]
    assert active_coverage["eligible_active_l1_must"] == ["ndf://counter/COUNT-REQ-001"]
    assert active_coverage["native_report"]["unverified"] == [
        "ndf://counter/COUNT-REQ-001"
    ]
    assert active_coverage["interpretation"] == "GAPS"
    contract.write_text(original)
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
