from __future__ import annotations

import re
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


def test_active_markdown_uses_semantic_unnumbered_headings() -> None:
    tracked = subprocess.run(
        ("git", "ls-files", "docs"),
        cwd=ROOT,
        text=True,
        capture_output=True,
        check=True,
    ).stdout.splitlines()
    numbered = re.compile(r"^#{2,6} \d+(?:\.\d+)*(?:[.)])? ")
    offenders = []

    for relative in tracked:
        path = ROOT / relative
        if (
            not path.is_file()
            or path.suffix != ".md"
            or relative.startswith("docs/gates/logs/")
        ):
            continue
        fenced = False
        fence = ""
        for line_number, line in enumerate(
            path.read_text(encoding="utf-8").splitlines(), 1
        ):
            stripped = line.lstrip()
            if stripped.startswith(("```", "~~~")):
                marker = stripped[:3]
                if not fenced:
                    fenced = True
                    fence = marker
                elif marker == fence:
                    fenced = False
                    fence = ""
                continue
            if not fenced and numbered.match(line):
                if line == "### 6.0 release-train decisions":
                    continue
                offenders.append(f"{relative}:{line_number}: {line}")

    assert not offenders, "manually numbered headings:\n" + "\n".join(offenders)


def test_onboarding_uses_current_paths_and_product_language() -> None:
    paths = [ROOT / "README.md", *(ROOT / "docs/getting-started").glob("*.md")]
    forbidden = (
        "docs/acir/migration.md",
        "build/rule",
        "flows/tools/dump_pyctrace.py",
        "phase-one",
        "/tmp/pyc_counter",
        "/tmp/tb_counter",
    )

    for path in paths:
        content = path.read_text(encoding="utf-8")
        for token in forbidden:
            assert token not in content, f"{path.relative_to(ROOT)}: {token}"

    readme = (ROOT / "README.md").read_text(encoding="utf-8")
    assert readme.count("<img src=") == 6
    assert "agentic_circuit frontend -> ACPy 0.5 -> ACIR" in readme
    assert "docs/development/repository-layout.md" in readme


def test_current_product_docs_do_not_present_supported_surfaces_as_legacy() -> None:
    roots = (
        ROOT / "docs/getting-started",
        ROOT / "docs/reference",
        ROOT / "docs/architecture",
        ROOT / "docs/development",
        ROOT / "docs/acir",
    )
    forbidden = re.compile(r"\b(?:legacy|prototype|phase-one)\b", re.IGNORECASE)
    offenders = []

    for root in roots:
        for path in root.rglob("*.md"):
            if path == ROOT / "docs/acir/spec/refs/history.md":
                continue
            match = forbidden.search(path.read_text(encoding="utf-8"))
            if match:
                offenders.append(f"{path.relative_to(ROOT)}: {match.group(0)}")

    assert not offenders, "stale product language:\n" + "\n".join(offenders)


def test_agent_frontend_guide_is_the_agent_authoring_entrypoint() -> None:
    guide_path = ROOT / "docs/development/agent-frontend-guide.md"
    guide = guide_path.read_text(encoding="utf-8")
    agents = (ROOT / "AGENTS.md").read_text(encoding="utf-8")

    assert "docs/development/agent-frontend-guide.md" in agents
    for contract in (
        "CycleAwareSignal",
        "pycircuit.structural.mux()",
        "@ac.rule",
        "@ac.inline",
        "domain.next()",
        "Do not start by writing",
        "Frontend: cycle-aware | structural | agentic",
    ):
        assert contract in guide

    for example in (
        "examples/pycircuit/features/issue_queue_2picker/",
        "examples/pycircuit/features/struct_transform/",
        "examples/agentic-circuit/pipelines/routed_dependency_pipeline.py",
        "examples/agentic-circuit/state/table_rule.py",
    ):
        assert example in guide


def test_cycle_aware_guide_recipe_has_aligned_data_and_valid_latency() -> None:
    guide = (ROOT / "docs/development/agent-frontend-guide.md").read_text(
        encoding="utf-8"
    )
    section = guide.split("## Write cycle-aware hardware", 1)[1].split(
        "## Write structural libraries", 1
    )[0]
    source = re.search(r"```python\n(.*?)\n```", section, re.DOTALL)
    assert source is not None

    namespace = {"__name__": "agent_frontend_guide_recipe"}
    exec(compile(source.group(1), "agent-frontend-guide.md", "exec"), namespace)

    from pycircuit import build_cycle_aware

    mlir = build_cycle_aware(
        namespace["registered_increment"], name="registered_increment"
    ).emit_mlir()
    assert "_v6_bal" not in mlir
    assert mlir.count("pyc.reg") == 2
    assert 'result_names = ["data", "valid"]' in mlir


def test_agentic_guide_recipe_uses_typed_system_boundaries() -> None:
    from agentic_circuit._queue_frontend import lower_queue_source

    guide = (ROOT / "docs/development/agent-frontend-guide.md").read_text(
        encoding="utf-8"
    )
    section = guide.split("## Write transactional architecture hardware", 1)[1].split(
        "## Decompose a complex design", 1
    )[0]
    source = re.search(r"```python\n(.*?)\n```", section, re.DOTALL)
    assert source is not None
    source_text = source.group(1)

    assert "ac.source(" not in source_text
    assert "ac.sink(" not in source_text
    lowered = lower_queue_source(source_text, "transaction_pipeline")
    assert "ac.rule " in lowered
    assert "ac.table" in lowered
    assert "func.func private @increment" in lowered
