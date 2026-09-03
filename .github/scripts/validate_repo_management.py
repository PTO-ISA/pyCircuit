#!/usr/bin/env python3
"""Fail-closed checks for pyCircuit governance, CI, and release metadata."""

from __future__ import annotations

import re
import sys
from pathlib import Path

import yaml

ROOT = Path(__file__).resolve().parents[2]


def read(path: str) -> str:
    target = ROOT / path
    if not target.is_file():
        return ""
    return target.read_text(encoding="utf-8")


def require(condition: bool, message: str, errors: list[str]) -> None:
    if not condition:
        errors.append(message)


def main() -> int:
    errors: list[str] = []

    workflows = sorted((ROOT / ".github" / "workflows").glob("*.yml"))
    for path in workflows:
        workflow_text = path.read_text(encoding="utf-8")
        try:
            yaml.safe_load(workflow_text)
        except yaml.YAMLError as exc:
            errors.append(f"invalid YAML: {path.relative_to(ROOT)}: {exc}")
        for action, revision in re.findall(
            r"uses:\s*([^@\s]+)@([^\s#]+)", workflow_text
        ):
            require(
                re.fullmatch(r"[0-9a-f]{40}", revision) is not None,
                f"external action must be pinned to a commit SHA: {action}@{revision}",
                errors,
            )

    codeowners = read(".github/CODEOWNERS")
    codeowner_rules = [
        line.split()
        for line in codeowners.splitlines()
        if line.strip() and not line.lstrip().startswith("#")
    ]
    expected_codeowners = ["@zhoubot", "@xiekunpeng"]
    require(
        any(
            rule[0] == "*" and rule[1:] == expected_codeowners
            for rule in codeowner_rules
        ),
        "CODEOWNERS must assign the default owners @zhoubot and @xiekunpeng",
        errors,
    )
    require(
        all(rule[1:] == expected_codeowners for rule in codeowner_rules),
        "every CODEOWNERS rule must assign @zhoubot and @xiekunpeng",
        errors,
    )
    require(
        "@PTO-ISA/pycircuit-maintainers" not in codeowners,
        "CODEOWNERS must not retain the legacy maintainer team",
        errors,
    )

    ci = read(".github/workflows/ci.yml")
    for command in (
        "pre-commit run --files",
        "pytest tests/unit -m unit",
        "mkdocs build",
        "check_api_hygiene.py",
        "validate_repo_management.py",
    ):
        require(command in ci, f"CI is missing required gate: {command}", errors)
    require(
        "LLVM_INSTALL_SCRIPT_SHA256" in ci and "sha256sum --check --strict" in ci,
        "CI must verify the downloaded LLVM installer",
        errors,
    )

    verilator_action = read(".github/actions/setup-verilator/action.yml")
    require(
        "sha256sum --check --strict" in verilator_action,
        "Verilator setup must verify downloaded archives",
        errors,
    )

    release = read(".github/workflows/release.yml")
    require(
        '--wheel-version "${VERSION}"' in release,
        "release must pass the tag-derived version to wheel creation",
        errors,
    )
    require(
        "Validate release version" in release,
        "release must validate tag/project/wheel version consistency",
        errors,
    )
    require(
        release.count("github.repository == 'PTO-ISA/pyCircuit'") >= 4,
        "every release job must be restricted to the canonical PTO-ISA repository",
        errors,
    )
    global_permissions = release.split("env:", 1)[0]
    require(
        "contents: write" not in global_permissions
        and "packages: write" not in global_permissions
        and "id-token: write" not in global_permissions,
        "release write permissions must be scoped to publishing jobs",
        errors,
    )

    packaging = read("packaging/wheel/setup.py")
    require("PTO-ISA Contributors" in packaging, "wheel author must be PTO-ISA", errors)
    require(
        "github.com/PTO-ISA/pyCircuit" in packaging,
        "wheel URLs must use PTO-ISA",
        errors,
    )
    require(
        "LinxISA" not in packaging, "wheel metadata must not reference LinxISA", errors
    )

    contributing = read("CONTRIBUTING.md")
    require(
        "Python 3.10" in contributing, "CONTRIBUTING must require Python 3.10+", errors
    )
    require(
        "LLVM/MLIR 22" in contributing, "CONTRIBUTING must require LLVM/MLIR 22", errors
    )
    require(
        "pyc6" in contributing.lower(),
        "CONTRIBUTING must describe the pyc6 surface",
        errors,
    )
    for path in (
        "docs/rfcs/pyc6-decisions.md",
        "docs/pyc6-plan.md",
        "docs/gates/decision_status_v6.md",
    ):
        require(
            (ROOT / path).is_file(), f"missing pyc6 source of truth: {path}", errors
        )

    security = read("SECURITY.md")
    require(
        re.search(
            r"https://github\.com/PTO-ISA/pyCircuit/security/advisories/new", security
        )
        is not None,
        "SECURITY must provide an actionable private-report URL",
        errors,
    )

    if errors:
        sys.stderr.write("repo-management validation failed:\n")
        for error in errors:
            sys.stderr.write(f"- {error}\n")
        return 1
    sys.stdout.write("repo-management validation passed\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
