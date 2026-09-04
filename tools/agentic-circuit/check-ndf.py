#!/usr/bin/env python3
"""Validate the repository's restricted NDF documentation profile."""

from __future__ import annotations

import argparse
import re
import shlex
import sys
from dataclasses import dataclass, field
from pathlib import Path


CLAUSE_RE = re.compile(r"^#{2,6}\s+.+?\s+\{#([A-Z][A-Z0-9-]*)\}\s*$")
METADATA_RE = re.compile(r"<!--\s*ndf:\s*(.*?)\s*-->")
CROSS_REFERENCE_RE = re.compile(r"\[\[([A-Z][A-Z0-9-]*)\]\]")
HEX40_RE = re.compile(r"^[0-9a-f]{40}$")
REQUIRED_METADATA = ("kind", "level", "layer", "status")
DEFAULT_ROOTS = (Path("docs/acir/spec"), Path("docs/rfcs/acir"))
EDGE_KEYS = (
    "refines",
    "depends-on",
    "conflicts-with",
    "couples-with",
    "verifies",
    "affects",
    "blocks",
)


@dataclass
class Clause:
    identifier: str
    path: Path
    line: int
    metadata: dict[str, str] = field(default_factory=dict)
    text: list[str] = field(default_factory=list)


def _targets(value: str) -> list[str]:
    return [item for item in re.split(r"[,;]", value) if item]


def _load(roots: list[Path]) -> tuple[list[Clause], list[str]]:
    clauses: list[Clause] = []
    errors: list[str] = []
    documents = sorted(
        document for root in roots for document in root.rglob("*.md")
    )
    for document in documents:
        current: Clause | None = None
        for line_number, line in enumerate(
            document.read_text(encoding="utf-8").splitlines(), start=1
        ):
            heading = CLAUSE_RE.match(line)
            if heading:
                current = Clause(heading.group(1), document, line_number)
                clauses.append(current)
                continue
            if current is None:
                continue
            current.text.append(line)
            for raw in METADATA_RE.findall(line):
                try:
                    tokens = shlex.split(raw)
                except ValueError as error:
                    errors.append(f"{document}:{line_number}: {error}")
                    continue
                for token in tokens:
                    if "=" not in token:
                        errors.append(
                            f"{document}:{line_number}: invalid metadata {token!r}"
                        )
                        continue
                    key, value = token.split("=", 1)
                    current.metadata[key] = value
    return clauses, errors


def validate(roots: list[Path]) -> tuple[list[Clause], list[str]]:
    clauses, errors = _load(roots)
    by_identifier: dict[str, Clause] = {}
    for clause in clauses:
        if clause.identifier in by_identifier:
            errors.append(
                f"{clause.path}:{clause.line}: duplicate clause {clause.identifier}"
            )
        by_identifier[clause.identifier] = clause
        for key in REQUIRED_METADATA:
            if key not in clause.metadata:
                errors.append(
                    f"{clause.path}:{clause.line}: {clause.identifier} missing {key}"
                )

    for clause in clauses:
        related = {
            target
            for key in EDGE_KEYS
            for target in _targets(clause.metadata.get(key, ""))
        }
        related.update(
            reference
            for line in clause.text
            for reference in CROSS_REFERENCE_RE.findall(line)
        )
        for target in sorted(related):
            if target not in by_identifier:
                errors.append(
                    f"{clause.path}:{clause.line}: dangling reference {target}"
                )
        if clause.identifier.startswith("REF-"):
            if clause.metadata.get("origin-kind") != "git":
                errors.append(
                    f"{clause.path}:{clause.line}: {clause.identifier} requires git origin"
                )
            if not HEX40_RE.fullmatch(clause.metadata.get("revision", "")):
                errors.append(
                    f"{clause.path}:{clause.line}: {clause.identifier} has invalid revision"
                )
            if clause.metadata.get("origin-status") not in {
                "verbatim",
                "paraphrase",
                "interpretation",
            }:
                errors.append(
                    f"{clause.path}:{clause.line}: {clause.identifier} has invalid origin status"
                )

    verified = {
        target
        for clause in clauses
        if clause.metadata.get("kind") == "verif"
        for target in _targets(clause.metadata.get("verifies", ""))
    }
    for clause in clauses:
        if (
            clause.metadata.get("kind") == "req"
            and clause.metadata.get("level") == "must"
            and clause.metadata.get("layer") == "L1"
            and clause.identifier not in verified
        ):
            errors.append(
                f"{clause.path}:{clause.line}: {clause.identifier} lacks verification"
            )
    return clauses, errors


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "roots",
        nargs="*",
        type=Path,
        default=list(DEFAULT_ROOTS),
    )
    arguments = parser.parse_args()
    profile_root = DEFAULT_ROOTS[0]
    if not (profile_root / "ndf.yaml").is_file():
        print(f"{profile_root}: missing ndf.yaml", file=sys.stderr)
        return 1
    missing_roots = [root for root in arguments.roots if not root.is_dir()]
    if missing_roots:
        for root in missing_roots:
            print(f"{root}: missing NDF document root", file=sys.stderr)
        return 1
    clauses, errors = validate(arguments.roots)
    if errors:
        print("\n".join(errors), file=sys.stderr)
        return 1
    print(f"NDF profile: OK ({len(clauses)} clauses)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
