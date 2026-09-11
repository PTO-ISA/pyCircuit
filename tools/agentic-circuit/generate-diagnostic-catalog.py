#!/usr/bin/env python3
"""Keep the Agentic Circuit diagnostic catalog aligned with implementation codes."""

from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
CATALOG = ROOT / "schemas/agentic-circuit/diagnostics/diagnostics.json"
REGISTRY = CATALOG.with_name("registry.json")
SOURCE_ROOTS = (
    ROOT / "python/agentic-circuit/src/agentic_circuit",
    ROOT / "compiler/acir",
    ROOT / "simulator/gfsim",
    ROOT / "tools/agentic-circuit",
    ROOT / "packaging/sdk",
)
SOURCE_SUFFIXES = frozenset({".cpp", ".h", ".inc", ".py", ".td"})
CODE = re.compile(
    r"\bAC(?:PY|ELAB|IR-[A-Z]+|SIM|LOWER|BUILD|TRACE|RUN|SDK-[A-Z]+)"
    r"-[A-Z0-9]+(?:-[A-Z0-9]+)*\b"
)
OWNERS = {
    "ACBUILD": "agentic-build",
    "ACELAB": "agentic-elaboration",
    "ACIR": "acir",
    "ACLOWER": "acir-lowering",
    "ACPY": "agentic-python",
    "ACRUN": "gfsim-runtime",
    "ACSDK": "sdk",
    "ACSIM": "acsim",
    "ACTRACE": "agentic-trace",
}


def implementation_sites() -> dict[str, set[str]]:
    sites: dict[str, set[str]] = {}
    for root in SOURCE_ROOTS:
        if not root.is_dir():
            continue
        for path in root.rglob("*"):
            if not path.is_file() or path.suffix not in SOURCE_SUFFIXES:
                continue
            try:
                text = path.read_text(encoding="utf-8")
            except UnicodeError:
                continue
            relative = path.relative_to(ROOT).as_posix()
            for code in CODE.findall(text):
                sites.setdefault(code, set()).add(relative)
    return sites


def native_stdout_code_uses() -> list[str]:
    violations: list[str] = []
    for root in SOURCE_ROOTS:
        if not root.is_dir():
            continue
        for path in root.rglob("*"):
            if not path.is_file() or path.suffix not in {".cpp", ".h", ".inc"}:
                continue
            statement = ""
            start_line = 0
            for line_number, line in enumerate(
                path.read_text(encoding="utf-8").splitlines(), start=1
            ):
                if not statement:
                    start_line = line_number
                statement += " " + line.strip()
                if ";" not in line:
                    continue
                if "llvm::outs()" in statement and CODE.search(statement):
                    violations.append(f"{path.relative_to(ROOT)}:{start_line}")
                statement = ""
    return violations


def load_registry() -> dict[str, object]:
    document = json.loads(REGISTRY.read_text(encoding="utf-8"))
    if type(document) is not dict or type(document.get("entries")) is not list:
        raise ValueError("diagnostic registry is not an object with an entries array")
    return document


def catalog_from_registry(document: dict[str, object]) -> dict[str, object]:
    if set(document) != {"schema", "version", "contract_epoch", "entries"}:
        raise ValueError("diagnostic registry has unknown or missing fields")
    if document["schema"] != "agentic-circuit-diagnostic-registry":
        raise ValueError("diagnostic registry identity is invalid")
    raw_entries = document["entries"]
    if type(raw_entries) is not list:
        raise ValueError("diagnostic registry entries must be an array")
    entry_keys = {
        "causes",
        "code",
        "examples",
        "owner",
        "repairs",
        "rule",
        "sources",
        "stage",
        "status",
        "title",
    }
    by_code: dict[str, dict[str, object]] = {}
    implementation = implementation_sites()
    for raw in raw_entries:
        if type(raw) is not dict or type(raw.get("code")) is not str:
            raise ValueError("diagnostic registry entry is invalid")
        if set(raw) != entry_keys:
            raise ValueError("diagnostic registry entry has unknown or missing fields")
        code = raw["code"]
        if code in by_code:
            raise ValueError(f"duplicate diagnostic registry code: {code}")
        prefix = code.split("-", 1)[0]
        if prefix not in OWNERS:
            raise ValueError(f"diagnostic registry code has no owner: {code}")
        if raw["owner"] != OWNERS[prefix]:
            raise ValueError(f"diagnostic registry owner mismatch: {code}")
        if raw["status"] not in {"active", "reserved", "retired"}:
            raise ValueError(f"diagnostic registry status is invalid: {code}")
        for field in ("owner", "stage", "title", "rule"):
            if type(raw[field]) is not str or not raw[field]:
                raise ValueError(f"diagnostic registry {field} is invalid: {code}")
        for field in ("causes", "examples", "repairs"):
            values = raw[field]
            if (
                type(values) is not list
                or not values
                or any(type(value) is not str or not value for value in values)
            ):
                raise ValueError(f"diagnostic registry {field} is invalid: {code}")
        rendered_entry = json.dumps(raw, ensure_ascii=False)
        title = raw["title"]
        invalid_title_fragments = (
            ");",
            "<<",
            "llvm::",
            "Case CompilerStage",
            "return system_",
            "(*",
            " + token",
        )
        if raw["status"] == "active" and (
            "contract associated with" in rendered_entry
            or "rejected the reported contract condition" in rendered_entry
            or re.fullmatch(r"[A-Z ]+ diagnostic [A-Z0-9]+", title)
            or len(title) < 8
            or any(fragment in title for fragment in invalid_title_fragments)
        ):
            raise ValueError(f"active diagnostic meaning is a placeholder: {code}")
        sources = raw["sources"]
        if type(sources) is not list or sources != sorted(set(sources)):
            raise ValueError(f"diagnostic registry sources are invalid: {code}")
        actual_sources = sorted(implementation.get(code, set()))
        if sources != actual_sources:
            raise ValueError(f"diagnostic registry source coverage mismatch: {code}")
        if raw["status"] == "active" and not sources:
            raise ValueError(f"active diagnostic code has no implementation use: {code}")
        if raw["status"] != "active" and sources:
            raise ValueError(f"inactive diagnostic code is used by implementation: {code}")
        by_code[code] = {key: value for key, value in raw.items() if key != "sources"}
    missing = sorted(set(implementation) - set(by_code))
    if missing:
        raise ValueError(f"implementation diagnostic code is unregistered: {missing[0]}")
    stdout_uses = native_stdout_code_uses()
    if stdout_uses:
        raise ValueError(
            "diagnostic code is used on a native success-output path: "
            + stdout_uses[0]
        )
    return {
        "schema": "agentic-circuit-diagnostic-catalog",
        "version": document["version"],
        "contract_epoch": document["contract_epoch"],
        "entries": [by_code[code] for code in sorted(by_code)],
    }


def rendered(document: dict[str, object]) -> str:
    entries = document["entries"]
    if type(entries) is not list:
        raise ValueError("diagnostic catalog entries must be an array")
    lines = [
        "{",
        f'  "schema": {json.dumps(document["schema"], ensure_ascii=False)},',
        f'  "version": {json.dumps(document["version"], ensure_ascii=False)},',
        "  \"contract_epoch\": "
        + json.dumps(document["contract_epoch"], ensure_ascii=False)
        + ",",
        '  "entries": [',
    ]
    for index, entry in enumerate(entries):
        suffix = "," if index + 1 < len(entries) else ""
        lines.append("    " + json.dumps(entry, ensure_ascii=False) + suffix)
    lines.extend(("  ]", "}"))
    return "\n".join(lines) + "\n"


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", action="store_true")
    arguments = parser.parse_args()

    try:
        current = CATALOG.read_text(encoding="utf-8")
        expected = rendered(catalog_from_registry(load_registry()))
    except (OSError, UnicodeError, ValueError, json.JSONDecodeError) as error:
        print(f"error: cannot generate diagnostic catalog: {error}", file=sys.stderr)
        return 1

    if arguments.check:
        if current != expected:
            print(
                "error: diagnostic catalog is stale; run "
                "tools/agentic-circuit/generate-diagnostic-catalog.py",
                file=sys.stderr,
            )
            return 1
        return 0

    CATALOG.write_text(expected, encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
