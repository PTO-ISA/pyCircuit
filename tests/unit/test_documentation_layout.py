from __future__ import annotations

import subprocess
from pathlib import Path

import pytest
import yaml

ROOT = Path(__file__).resolve().parents[2]
pytestmark = pytest.mark.unit

DOC_SECTIONS = {
    "acir",
    "architecture",
    "development",
    "gates",
    "getting-started",
    "legal",
    "reference",
    "research",
    "rfcs",
}
ROOT_PAGES = {"index.md", "pyc6-plan.md"}


def _nav_pages(value: object) -> set[str]:
    if isinstance(value, str):
        return {value}
    if isinstance(value, list):
        return set().union(*(_nav_pages(item) for item in value))
    if isinstance(value, dict):
        return set().union(*(_nav_pages(item) for item in value.values()))
    return set()


def test_documentation_root_contains_only_stable_entrypoints() -> None:
    tracked = subprocess.run(
        ("git", "ls-files", "docs"),
        cwd=ROOT,
        text=True,
        capture_output=True,
        check=True,
    ).stdout.splitlines()
    root_pages = {Path(path).name for path in tracked if len(Path(path).parts) == 2}
    sections = {Path(path).parts[1] for path in tracked if len(Path(path).parts) > 2}

    assert root_pages == ROOT_PAGES
    assert sections == DOC_SECTIONS


def test_every_active_document_is_reachable_from_navigation() -> None:
    raw = (
        (ROOT / "mkdocs.yml")
        .read_text(encoding="utf-8")
        .replace(
            "!!python/name:pymdownx.superfences.fence_code_format",
            "pymdownx.superfences.fence_code_format",
        )
    )
    config = yaml.safe_load(raw)
    nav_pages = _nav_pages(config["nav"])
    tracked = subprocess.run(
        ("git", "ls-files", "docs"),
        cwd=ROOT,
        text=True,
        capture_output=True,
        check=True,
    ).stdout.splitlines()
    active_pages = {
        path.removeprefix("docs/")
        for path in tracked
        if path.endswith(".md") and not path.startswith("docs/gates/logs/")
    }

    assert nav_pages == active_pages
