"""Check navigation and current M5 product guidance without stale API oracles."""

from __future__ import annotations

import re
from pathlib import Path

import pytest
import yaml

ROOT = Path(__file__).resolve().parents[2]
pytestmark = pytest.mark.unit


def _nav_pages(value: object) -> set[str]:
    if isinstance(value, str):
        return {value}
    if isinstance(value, list):
        return set().union(*(_nav_pages(item) for item in value))
    if isinstance(value, dict):
        return set().union(*(_nav_pages(item) for item in value.values()))
    return set()


def _current_markdown_pages() -> set[str]:
    return {
        path.relative_to(ROOT / "docs").as_posix()
        for path in (ROOT / "docs").rglob("*.md")
        if "logs" not in path.relative_to(ROOT / "docs").parts
    }


def test_every_current_document_is_reachable_from_navigation() -> None:
    raw = (
        (ROOT / "mkdocs.yml")
        .read_text(encoding="utf-8")
        .replace(
            "!!python/name:pymdownx.superfences.fence_code_format",
            "pymdownx.superfences.fence_code_format",
        )
    )
    config = yaml.safe_load(raw)
    nav_pages = {
        link.split("#", 1)[0]
        for link in _nav_pages(config["nav"])
        if link.endswith(".md") or ".md#" in link
    }

    existing = _current_markdown_pages()
    missing = sorted(nav_pages - existing)
    assert not missing, f"navigation links to missing documents: {missing}"
    required = {
        "index.md",
        "getting-started/installation.md",
        "getting-started/quickstart.md",
        "getting-started/tutorial.md",
        "development/agent-frontend-guide.md",
        "reference/language.md",
    }
    assert required <= nav_pages


def test_m5_onboarding_names_the_public_driver_and_bounded_profile() -> None:
    paths = [
        ROOT / "README.md",
        ROOT / "docs/getting-started/installation.md",
        ROOT / "docs/getting-started/quickstart.md",
        ROOT / "docs/getting-started/tutorial.md",
        ROOT / "docs/development/agent-frontend-guide.md",
    ]
    content = {path: path.read_text(encoding="utf-8") for path in paths}

    for path in paths[:3]:
        text = content[path]
        for command in ("pycircuit compile", "pycircuit link", "pycircuit emit"):
            assert command in text, f"{path.relative_to(ROOT)} omits {command}"
    tutorial = content[ROOT / "docs/getting-started/tutorial.md"]
    assert "examples/pycircuit/counter/" in tutorial
    assert "cmake --build" in tutorial
    quickstart = content[ROOT / "docs/getting-started/quickstart.md"]
    for contract in (
        "portless",
        "@module",
        "@rule",
        "default clock",
        "empty static arguments",
    ):
        assert contract in quickstart
    assert (
        "agentic_circuit frontend -> acc.py -> verified ACIR -> acc"
        not in content[ROOT / "README.md"]
    )


def test_agent_frontend_guide_describes_only_the_current_source_profile() -> None:
    guide = (ROOT / "docs/development/agent-frontend-guide.md").read_text(
        encoding="utf-8"
    )
    agents = (ROOT / "AGENTS.md").read_text(encoding="utf-8")

    assert "docs/development/agent-frontend-guide.md" in agents
    for contract in (
        "one portless `@module` function per implementation source",
        "nested `@rule` functions",
        "one default clock",
        "empty static argument list",
        "pycircuit compile",
        "pycircuit link",
        "pycircuit emit",
    ):
        assert contract in guide
    assert "legacy CycleAwareSignal" in guide
    for retired_surface in ("from agentic_circuit", "build_cycle_aware(", "pycc"):
        assert retired_surface not in guide
    assert "complete `@system`" in guide.lower()


def test_active_onboarding_does_not_use_numbered_section_headings() -> None:
    roots = (
        ROOT / "docs/getting-started",
        ROOT / "docs/reference",
        ROOT / "docs/development",
    )
    numbered = re.compile(r"^#{2,6} \d+(?:\.\d+)*(?:[.)])? ")
    offenders: list[str] = []
    for root in roots:
        for path in root.rglob("*.md"):
            # The implementation guide uses numbers as ordered build steps.
            if path == ROOT / "docs/development/implementation-guide.md":
                continue
            fenced = False
            fence = ""
            for number, line in enumerate(
                path.read_text(encoding="utf-8").splitlines(), 1
            ):
                stripped = line.lstrip()
                if stripped.startswith(("```", "~~~")):
                    marker = stripped[:3]
                    if not fenced:
                        fenced, fence = True, marker
                    elif marker == fence:
                        fenced, fence = False, ""
                    continue
                if not fenced and numbered.match(line):
                    offenders.append(f"{path.relative_to(ROOT)}:{number}")
    assert offenders == []
