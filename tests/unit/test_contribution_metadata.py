"""Independent checks for issue and pull request contribution metadata."""

from __future__ import annotations

import importlib.util
import json
import os
import subprocess
import sys
from pathlib import Path
from urllib.parse import urlparse

import pytest
import yaml

pytestmark = pytest.mark.unit
ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / ".github/scripts/check_contribution.py"
SPEC = importlib.util.spec_from_file_location("check_contribution", SCRIPT)
assert SPEC is not None and SPEC.loader is not None
validator = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(validator)

AREAS = (
    "Capture",
    "MLIR",
    "IR",
    "Link",
    "CPP",
    "Verilog",
    "Runtime",
    "System",
    "CLI",
    "Build",
    "Package",
    "CI",
    "Docs",
    "Other",
)
REQUIRED_HEADINGS = (
    "Summary",
    "Change type",
    "Related issues",
    "Changes",
    "Validation",
    "Review checklist",
    "Reviewer notes",
)
AUTHOR_SECTIONS = (
    "Summary",
    "Related issues",
    "Changes",
    "Validation",
    "Reviewer notes",
)


def pr_body(**overrides: str) -> str:
    sections = {
        "Summary": "Report malformed contribution metadata before review.",
        "Change type": "- [x] Bug fix\n- [ ] Documentation",
        "Related issues": "None; discovered during contribution workflow review.",
        "Changes": "Validate issue areas and authored pull request sections.",
        "Validation": "- [x] Targeted tests\nRan the metadata tests: all passed.",
        "Review checklist": "- [ ] Independent reviewer has checked the change.",
        "Reviewer notes": "Please check Markdown comments cannot satisfy the form.",
    }
    sections.update(overrides)
    return "\n\n".join(f"## {name}\n\n{text}" for name, text in sections.items())


@pytest.mark.parametrize("area", AREAS)
def test_supported_issue_areas(area: str) -> None:
    assert validator.validate_issue_title(f"[{area}] Diagnose malformed input") == []


@pytest.mark.parametrize(
    "title",
    [
        "",
        "Fix malformed input",
        "[Unknown] Fix malformed input",
        "[docs] Fix malformed input",
        "[Docs]",
        "[Docs]   ",
        "[Docs]Valid summary",
        "[Docs] First line\nsecond line",
        "[Docs] First line\r\nsecond line",
    ],
)
def test_issue_title_requires_known_area_and_single_line_summary(title: str) -> None:
    assert validator.validate_issue_title(title)


def test_minimal_authored_pr_and_unchecked_review_items_are_valid() -> None:
    assert validator.validate_pr_body(pr_body()) == []


def test_crlf_pr_body_is_valid() -> None:
    assert validator.validate_pr_body(pr_body().replace("\n", "\r\n")) == []


def test_untouched_template_is_incomplete() -> None:
    template = (ROOT / ".github/PULL_REQUEST_TEMPLATE.md").read_text(encoding="utf-8")
    assert validator.validate_pr_body(template)


@pytest.mark.parametrize("heading", REQUIRED_HEADINGS)
def test_every_required_heading_must_be_present(heading: str) -> None:
    body = pr_body().replace(f"## {heading}\n", f"### {heading}\n")
    assert validator.validate_pr_body(body)


@pytest.mark.parametrize("heading", REQUIRED_HEADINGS)
def test_duplicate_required_headings_are_rejected(heading: str) -> None:
    assert validator.validate_pr_body(pr_body() + f"\n\n## {heading}\nExtra text.")


@pytest.mark.parametrize("heading", AUTHOR_SECTIONS)
@pytest.mark.parametrize(
    "content",
    [
        "",
        "<!-- Please explain this section. -->",
        "### Description\n\n```\n```\n\n-\n*\n+",
        "- [ ] Explain this section\n- [x] Completed prefilled item",
    ],
)
def test_author_sections_require_text_beyond_markdown_scaffolding(
    heading: str,
    content: str,
) -> None:
    assert validator.validate_pr_body(pr_body(**{heading: content}))


@pytest.mark.parametrize(
    "content", ["Bug fix", "- [ ] Bug fix", "<!-- - [x] Bug fix -->"]
)
def test_change_type_requires_a_visible_checked_item(content: str) -> None:
    assert validator.validate_pr_body(pr_body(**{"Change type": content}))


@pytest.mark.parametrize(
    "content",
    [
        "Ran the targeted tests successfully.",
        "- [ ] Targeted tests\nRan the targeted tests successfully.",
        "- [x] Targeted tests",
        "<!-- - [x] Targeted tests -->\nRan the targeted tests successfully.",
        "- [x] Targeted tests\n<!-- all passed -->",
    ],
)
def test_validation_needs_checked_item_and_authored_result(content: str) -> None:
    assert validator.validate_pr_body(pr_body(Validation=content))


def test_validation_can_honestly_report_deferred_checks_with_reason() -> None:
    body = pr_body(
        Validation="- [x] Checks deferred\nCompiler gates deferred: documentation-only change."
    )
    assert validator.validate_pr_body(body) == []


def test_commented_fake_sections_cannot_replace_required_heading() -> None:
    body = pr_body().replace("## Validation\n", "## Other evidence\n")
    body += "\n<!--\n## Validation\n- [x] Tests\nAll passed.\n-->"
    assert validator.validate_pr_body(body)


def test_commented_duplicate_sections_do_not_invalidate_authored_body() -> None:
    body = pr_body() + "\n<!--\n## Summary\n## Validation\n- [x] Tests\n-->"
    assert validator.validate_pr_body(body) == []


def test_shell_syntax_in_metadata_is_only_text(tmp_path: Path) -> None:
    marker = tmp_path / "must-not-exist"
    literal = f"$(touch {marker}) `touch {marker}` ${{{{ github.token }}}}"
    assert validator.validate_issue_title(f"[CI] Explain {literal}") == []
    assert (
        validator.validate_pr_body(pr_body(Summary=f"Handle literal text: {literal}"))
        == []
    )
    assert not marker.exists()


@pytest.mark.parametrize(
    ("event_name", "payload", "expected_status", "expected_output"),
    [
        (
            "issues",
            {"issue": {"title": "[Docs] Clarify contribution checks"}},
            0,
            "invalid=false\n",
        ),
        ("issues", {"issue": {"title": "Missing required area"}}, 0, "invalid=true\n"),
        ("pull_request", {"pull_request": {"body": pr_body()}}, 0, ""),
        ("pull_request", {"pull_request": {"body": None}}, 1, ""),
    ],
)
def test_event_cli_reports_issue_routing_and_pr_completeness(
    tmp_path: Path,
    event_name: str,
    payload: dict,
    expected_status: int,
    expected_output: str,
) -> None:
    event_path = tmp_path / "event.json"
    output_path = tmp_path / "github-output"
    event_path.write_text(json.dumps(payload), encoding="utf-8")
    result = subprocess.run(
        [sys.executable, str(SCRIPT)],
        env={
            **os.environ,
            "GITHUB_EVENT_NAME": event_name,
            "GITHUB_EVENT_PATH": str(event_path),
            "GITHUB_OUTPUT": str(output_path),
        },
        capture_output=True,
        text=True,
        check=False,
    )
    assert result.returncode == expected_status, result.stdout + result.stderr
    assert result.stderr == ""
    if expected_output:
        assert output_path.read_text(encoding="utf-8") == expected_output
    else:
        assert not output_path.exists()


@pytest.mark.parametrize("fence", ["```", "~~~", "````"])
def test_validation_log_headings_are_not_pr_sections(fence: str) -> None:
    body = pr_body(
        Validation=f"- [x] Targeted tests\nRan log parsing tests successfully.\n{fence}text\n## Summary\nCaptured diagnostic text.\n{fence}"
    )
    assert validator.validate_pr_body(body) == []


@pytest.mark.parametrize("fence", ["```", "~~~"])
def test_required_section_inside_code_block_cannot_supply_metadata(fence: str) -> None:
    body = pr_body().replace("## Summary\n", "## Log\n")
    body += f"\n{fence}markdown\n## Summary\nFake authored summary.\n{fence}"
    assert validator.validate_pr_body(body)


@pytest.mark.parametrize("fence", ["```", "~~~"])
def test_checked_change_type_inside_code_block_is_not_a_selection(fence: str) -> None:
    body = pr_body(**{"Change type": f"{fence}markdown\n- [x] Bug fix\n{fence}"})
    assert validator.validate_pr_body(body)


def test_issue_form_areas_match_validation_and_fields_are_well_formed() -> None:
    form = yaml.safe_load((ROOT / ".github/ISSUE_TEMPLATE/issue.yml").read_text())
    assert isinstance(form["name"], str) and form["name"].strip()
    assert isinstance(form["description"], str) and form["description"].strip()
    fields = [field for field in form["body"] if field["type"] != "markdown"]
    field_ids = [field["id"] for field in fields]
    assert len(field_ids) == len(set(field_ids))
    assert all(isinstance(field_id, str) and field_id.strip() for field_id in field_ids)
    by_id = {field["id"]: field for field in fields}
    options = by_id["area"]["attributes"]["options"]
    assert len(options) == len(set(options))
    assert set(options) == set(AREAS) == validator.AREAS
    assert all(
        validator.validate_issue_title(f"[{area}] Form selection") == []
        for area in options
    )
    for field in fields:
        assert field["type"] in {"input", "textarea", "dropdown", "checkboxes"}
        assert field["attributes"]["label"].strip()
        assert isinstance(field["validations"]["required"], bool)
        if field["type"] == "dropdown":
            assert field["attributes"]["options"]
    for field_id in ("kind", "area", "summary", "behavior", "reproduce", "environment"):
        assert by_id[field_id]["validations"]["required"] is True


def test_issue_config_routes_reports_through_form_and_private_security_contact() -> (
    None
):
    config = yaml.safe_load((ROOT / ".github/ISSUE_TEMPLATE/config.yml").read_text())
    assert config["blank_issues_enabled"] is False
    assert config["contact_links"]
    for contact in config["contact_links"]:
        assert contact["name"].strip() and contact["about"].strip()
        url = urlparse(contact["url"])
        assert url.scheme == "https" and url.netloc
    assert any(
        "/security/advisories/new" in link["url"] for link in config["contact_links"]
    )


def test_pr_metadata_workflow_keeps_issue_writes_out_of_pr_job() -> None:
    workflow = yaml.safe_load(
        (ROOT / ".github/workflows/contribution-metadata.yml").read_text()
    )
    assert workflow["permissions"]
    assert all(value == "read" for value in workflow["permissions"].values())
    pr_job = workflow["jobs"]["pr-description"]
    effective_permissions = pr_job.get("permissions", workflow["permissions"])
    assert effective_permissions
    assert all(value == "read" for value in effective_permissions.values())
    write_jobs = [
        job
        for job in workflow["jobs"].values()
        if "write" in job.get("permissions", {}).values()
    ]
    assert write_jobs
    for job in write_jobs:
        assert job["permissions"]["issues"] == "write"
        assert job["if"].strip() == "github.event_name == 'issues'"


def test_unclosed_html_comment_in_fenced_example_preserves_later_sections() -> None:
    body = pr_body(Changes="```html\n<!-- placeholder\n```")
    assert validator.validate_pr_body(body) == []


def test_closed_html_comment_inside_fence_is_authored_code() -> None:
    body = pr_body(Changes="```html\n<!-- documented template placeholder -->\n```")
    assert validator.validate_pr_body(body) == []
