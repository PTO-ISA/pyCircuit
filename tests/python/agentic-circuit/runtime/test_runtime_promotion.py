from __future__ import annotations

from acir_runtime_promotion import build_manifest


def _report(semantic: str = "SEMANTIC_CONTRACT_MISSING") -> dict:
    return {
        "schema": "acir-runtime-full-validation-v0.1",
        "candidate_results": [{
            "project": "opentitan", "module": "fixture_and", "source_file": "rtl/fixture_and.sv",
            "closure": "PASS", "case_keys": ["opentitan|fixture_and|rtl/fixture_and.sv|s0"],
        }],
        "case_results": [{
            "case_key": "opentitan|fixture_and|rtl/fixture_and.sv|s0", "project": "opentitan",
            "module": "fixture_and", "source_file": "rtl/fixture_and.sv", "repo_url": "https://example.invalid/fixture",
            "commit_sha": "0123456789abcdef", "config": "s0", "closure": "PASS",
            "verilator_lint": "PASS", "simulation": "PASS", "synthesis": "PASS",
            "mapped_area": 1.25, "semantic_contract": semantic, "interface": "canonical/fixture_and",
        }],
    }


def test_structural_candidate_requires_explicit_allowlist_for_promotion() -> None:
    result = build_manifest(_report())
    entry = result["entries"][0]
    assert entry["structural_status"] == "passed"
    assert entry["promotion"] == "review_required"
    assert entry["semantic_status"] == "missing"


def test_allowlisted_candidate_is_marked_structural_only() -> None:
    result = build_manifest(_report(), accept_structural=["opentitan/fixture_and"])
    assert result["entries"][0]["promotion"] == "accepted_structural"


def test_semantic_and_interface_gates_can_reach_runtime_ready() -> None:
    result = build_manifest(_report("fixture::and-v1"))
    assert result["entries"][0]["promotion"] == "runtime_ready"


def test_failed_gate_is_blocked() -> None:
    report = _report()
    report["case_results"][0]["synthesis"] = "FAIL"
    result = build_manifest(report)
    assert result["entries"][0]["promotion"] == "blocked"
    assert result["entries"][0]["failure_reasons"] == ["yosys:fail"]


def test_non_synth_candidate_is_structural_but_not_blocked() -> None:
    report = _report()
    case = report["case_results"][0]
    case["simulation"] = "NOT_APPLICABLE_NON_SYNTH"
    case["synthesis"] = "NOT_APPLICABLE_NON_SYNTH"
    result = build_manifest(report)
    entry = result["entries"][0]
    assert entry["structural_status"] == "passed"
    assert entry["non_synthesizable"] is True
    assert entry["promotion"] == "review_required"
