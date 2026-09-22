#!/usr/bin/env python3
"""Keep the Agentic Circuit diagnostic catalog aligned with implementation codes."""

from __future__ import annotations

import argparse
import ast
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
    r"\bAC(?:PY|ELAB|IR-[A-Z]+|LOWER|BUILD|TRACE|RUN|SDK-[A-Z]+)"
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
    "ACTRACE": "agentic-trace",
}

# A code carries many distinct messages (ACPY-TYPE-006 had 70 at the time this
# inventory was added), so one hand-written paragraph cannot describe it. The
# catalog therefore carries the exact message templates extracted from the
# implementation, and `explain` lists them. Interpolated values render as
# `{expression}` so a template still reads as the template it is.
MESSAGE_FIELD = "messages"
_ESCAPED_QUOTED = re.compile(r'"((?:[^"\\\n]|\\.)*)"')
# Prose that substitutes a placeholder for the interpolated value reads as a
# concrete-but-wrong description ("Repeated the reported value"), and the generic
# per-stage repair line is not a repair at all. Both are rejected so a summary
# that cannot be trusted is replaced by the exact message inventory instead.
PLACEHOLDER_PATTERNS = (
    "contract associated with",
    "rejected the reported contract condition",
    "the reported value",
    "the reported contract",
    "correct the reported",
)
PLACEHOLDER_TITLE_FRAGMENTS = (
    ");",
    "<<",
    "llvm::",
    "Case CompilerStage",
    "return system_",
    "(*",
    " + token",
)


class _LiteralCollector(ast.NodeVisitor):
    """Collect string literals without splitting f-string constant parts."""

    def __init__(self) -> None:
        self.values: list[str] = []
        self.pairs: list[tuple[str, str]] = []

    @staticmethod
    def _render(node: ast.expr) -> str | None:
        if isinstance(node, ast.Constant) and isinstance(node.value, str):
            return node.value
        if isinstance(node, ast.JoinedStr):
            parts: list[str] = []
            for value in node.values:
                if isinstance(value, ast.Constant) and isinstance(value.value, str):
                    parts.append(value.value)
                elif isinstance(value, ast.FormattedValue):
                    parts.append("{" + ast.unparse(value.value) + "}")
                else:
                    parts.append("{expression}")
            return "".join(parts)
        return None

    def visit_JoinedStr(self, node: ast.JoinedStr) -> None:
        rendered = self._render(node)
        if rendered is not None:
            self.values.append(rendered)
        # Do not descend: an f-string's constant parts are not standalone
        # messages, and descending would record truncated duplicates.

    def visit_Constant(self, node: ast.Constant) -> None:
        if isinstance(node.value, str):
            self.values.append(node.value)

    def visit_Call(self, node: ast.Call) -> None:
        # Several helpers pass the code and the message as sibling arguments, for
        # example `_fail("ACPY-CONFIG-001", f"workspace manifest not found: ...")`
        # or `Diagnostic(code="...", message="...")`. The message is not part of a
        # `CODE: text` literal, so pair each bare-code argument with the first
        # sibling that carries text.
        arguments: list[ast.expr] = [*node.args]
        arguments.extend(
            keyword.value for keyword in node.keywords if keyword.value is not None
        )
        bare = [self._render(argument) for argument in arguments]
        codes = [
            text
            for text in bare
            if text is not None and CODE.fullmatch(text.strip())
        ]
        if codes:
            for text in bare:
                if text is None or CODE.fullmatch(text.strip()):
                    continue
                for code in codes:
                    self.pairs.append((code, text))
        self.generic_visit(node)


def _python_literals(text: str) -> tuple[list[str], list[tuple[str, str]]]:
    try:
        tree = ast.parse(text)
    except SyntaxError:
        return [], []
    collector = _LiteralCollector()
    collector.visit(tree)
    return collector.values, collector.pairs


def _native_literals(text: str) -> tuple[list[str], list[tuple[str, str]]]:
    literals = [
        match.group(1).encode("utf-8").decode("unicode_escape")
        for match in _ESCAPED_QUOTED.finditer(text)
    ]
    return literals, []


def _message_templates(literals: list[str]) -> dict[str, set[str]]:
    found: dict[str, set[str]] = {}
    for literal in literals:
        for code in set(CODE.findall(literal)):
            marker = f"{code}:"
            if marker not in literal:
                continue
            template = literal.split(marker, 1)[1].strip()
            if not template:
                continue
            found.setdefault(code, set()).add(template)
    return found


def implementation_messages() -> dict[str, list[str]]:
    """Return the exact, deterministic message templates per diagnostic code."""

    collected: dict[str, set[str]] = {}
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
            if CODE.search(text) is None:
                continue
            literals, pairs = (
                _python_literals(text)
                if path.suffix == ".py"
                else _native_literals(text)
            )
            for code, templates in _message_templates(literals).items():
                collected.setdefault(code, set()).update(templates)
            for code, message in pairs:
                rendered = message.strip()
                if rendered:
                    collected.setdefault(code, set()).add(rendered)
    return {code: sorted(templates) for code, templates in collected.items()}


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
    if set(document) != {"schema", "version", "entries"}:
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
    prose_fields = ("title", "rule", "causes", "examples", "repairs")
    by_code: dict[str, dict[str, object]] = {}
    implementation = implementation_sites()
    messages_by_code = implementation_messages()
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
        for field in ("owner", "stage"):
            if type(raw[field]) is not str or not raw[field]:
                raise ValueError(f"diagnostic registry {field} is invalid: {code}")
        # Hand-written prose is optional: a code whose meaning is only the set of
        # conditions it reports is described by `messages` instead of a summary
        # that paraphrases them. Prose that is present must be real prose.
        for field in ("title", "rule"):
            value = raw[field]
            if value is not None and (type(value) is not str or not value):
                raise ValueError(f"diagnostic registry {field} is invalid: {code}")
        for field in ("causes", "examples", "repairs"):
            values = raw[field]
            if values is None:
                continue
            if (
                type(values) is not list
                or not values
                or any(type(value) is not str or not value for value in values)
            ):
                raise ValueError(f"diagnostic registry {field} is invalid: {code}")
        templates = messages_by_code.get(code, [])
        if raw["status"] == "active" and not templates and raw["title"] is None:
            raise ValueError(f"active diagnostic code has no description: {code}")
        rendered_prose = json.dumps(
            {field: raw[field] for field in prose_fields}, ensure_ascii=False
        ).lower()
        title = raw["title"]
        if raw["status"] == "active" and (
            any(pattern in rendered_prose for pattern in PLACEHOLDER_PATTERNS)
            or (
                type(title) is str
                and (
                    re.fullmatch(r"[A-Z ]+ diagnostic [A-Z0-9]+", title)
                    or len(title) < 8
                    or any(
                        fragment in title for fragment in PLACEHOLDER_TITLE_FRAGMENTS
                    )
                )
            )
        ):
            raise ValueError(f"active diagnostic meaning is a placeholder: {code}")
        sources = raw["sources"]
        if type(sources) is not list or sources != sorted(set(sources)):
            raise ValueError(f"diagnostic registry sources are invalid: {code}")
        actual_sources = sorted(implementation.get(code, set()))
        if sources != actual_sources:
            raise ValueError(f"diagnostic registry source coverage mismatch: {code}")
        if raw["status"] == "active" and not sources:
            raise ValueError(
                f"active diagnostic code has no implementation use: {code}"
            )
        if raw["status"] != "active" and sources:
            raise ValueError(
                f"inactive diagnostic code is used by implementation: {code}"
            )
        entry = {key: value for key, value in raw.items() if key != "sources"}
        if templates:
            entry[MESSAGE_FIELD] = templates
        by_code[code] = entry
    missing = sorted(set(implementation) - set(by_code))
    if missing:
        raise ValueError(
            f"implementation diagnostic code is unregistered: {missing[0]}"
        )
    stdout_uses = native_stdout_code_uses()
    if stdout_uses:
        raise ValueError(
            "diagnostic code is used on a native success-output path: " + stdout_uses[0]
        )
    return {
        "schema": "agentic-circuit-diagnostic-catalog",
        "version": document["version"],
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
        '  "entries": [',
    ]
    for index, entry in enumerate(entries):
        suffix = "," if index + 1 < len(entries) else ""
        lines.append("    " + json.dumps(entry, ensure_ascii=False) + suffix)
    lines.extend(("  ]", "}"))
    return "\n".join(lines) + "\n"


def synchronized_registry(document: dict[str, object]) -> dict[str, object]:
    """Drop removed diagnostics and refresh source ownership deterministically."""

    implementation = implementation_sites()
    entries = document.get("entries")
    if type(entries) is not list:
        raise ValueError("diagnostic registry entries must be an array")
    synchronized = []
    for raw in entries:
        if type(raw) is not dict or type(raw.get("code")) is not str:
            raise ValueError("diagnostic registry entry is invalid")
        code = raw["code"]
        if code not in implementation:
            continue
        updated = dict(raw)
        updated["status"] = "active"
        updated["sources"] = sorted(implementation[code])
        synchronized.append(updated)
    synchronized.sort(key=lambda entry: str(entry["code"]))
    return {**document, "entries": synchronized}


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", action="store_true")
    arguments = parser.parse_args()

    try:
        current = CATALOG.read_text(encoding="utf-8")
        registry = load_registry()
        if not arguments.check:
            registry = synchronized_registry(registry)
            REGISTRY.write_text(rendered(registry), encoding="utf-8")
        expected = rendered(catalog_from_registry(registry))
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
