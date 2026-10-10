#!/usr/bin/env python3
"""Fail-closed structural audit for generated pyCircuit Verilog/SystemVerilog."""

from __future__ import annotations

import argparse
import difflib
import html
import json
import re
from pathlib import Path

MODULE_RE = re.compile(r"(?m)^module\s+([A-Za-z_][A-Za-z0-9_$]*)\b")
WIRE_RE = re.compile(r"(?m)^wire(?:\s+\[[^\]]+\])?\s+([A-Za-z_][A-Za-z0-9_$]*)\s*;")
INSTANCE_RE = re.compile(
    r"(?m)^([A-Za-z_][A-Za-z0-9_$]*)"
    r"(?:\s+#\([^;]*?\))?\s+([A-Za-z_][A-Za-z0-9_$]*)\s*\("
)
ASSERTION_RE = re.compile(r"(?m)^([A-Za-z_][A-Za-z0-9_$]*):\s+assert\s+property\b")
COVERAGE_RE = re.compile(
    r"(?m)^([A-Za-z_][A-Za-z0-9_$]*)_coverage:\s+cover\s+property\b"
)
PORT_CONNECTION_RE = re.compile(r"\.([A-Za-z_][A-Za-z0-9_$]*)\(([^()]*)\)")
ASSIGN_RE = re.compile(r"(?m)^assign\s+([A-Za-z_][A-Za-z0-9_$]*)\s*=\s*(.*);$")
LOWER_SNAKE_RE = re.compile(r"^[a-z][a-z0-9]*(?:_[a-z0-9]+)*$")
SIMPLE_CONNECTION_RE = re.compile(r"^(?:[A-Za-z_][A-Za-z0-9_$]*|\d+)$")
SIZED_LITERAL_RE = re.compile(r"\b\d+'[sS]?[bBoOdDhH][0-9a-fA-F_xXzZ?]+\b")
UNSIZED_VALUE_RE = re.compile(r"(?<![A-Za-z0-9_$'])\d+(?![A-Za-z0-9_$'{])")


def _module_bodies(text: str) -> list[tuple[str, str]]:
    modules: list[tuple[str, str]] = []
    matches = list(MODULE_RE.finditer(text))
    for index, match in enumerate(matches):
        end = text.find("endmodule", match.end())
        if end < 0:
            end = matches[index + 1].start() if index + 1 < len(matches) else len(text)
        modules.append((match.group(1), text[match.end() : end]))
    return modules


def _unsized_assignment_literals(rhs: str) -> list[str]:
    without_sized = SIZED_LITERAL_RE.sub("", rhs)
    without_indices = re.sub(r"\[[^\]]+\]", "", without_sized)
    return UNSIZED_VALUE_RE.findall(without_indices)


def audit(path: Path) -> tuple[list[str], dict[str, object]]:
    raw = path.read_bytes()
    issues: list[str] = []
    try:
        text = raw.decode("ascii")
    except UnicodeDecodeError as exc:
        return [f"{path}: generated RTL is not ASCII at byte {exc.start}"], {}

    modules = _module_bodies(text)
    if not modules:
        issues.append(f"{path}: generated RTL contains no module")
    module_names = [name for name, _ in modules]
    if len(set(module_names)) != len(module_names):
        issues.append(f"{path}: generated RTL contains duplicate module names")
    if module_names != sorted(module_names):
        issues.append(
            f"{path}: generated RTL modules are not deterministically ordered"
        )

    reports: list[dict[str, object]] = []
    for module, body in modules:
        if not LOWER_SNAKE_RE.fullmatch(module) or "__" in module:
            issues.append(f"{path}: module {module!r} is not readable lower-snake")

        wires = WIRE_RE.findall(body)
        instances = [name for _, name in INSTANCE_RE.findall(body)]
        assertions = ASSERTION_RE.findall(body)
        coverage = COVERAGE_RE.findall(body)
        if wires != sorted(wires):
            issues.append(f"{path}: module {module!r} wire declarations are unstable")
        if instances != sorted(instances):
            issues.append(f"{path}: module {module!r} instances are unstable")
        if assertions != sorted(assertions):
            issues.append(f"{path}: module {module!r} assertions are unstable")
        if coverage != assertions:
            issues.append(
                f"{path}: module {module!r} assertion/coverage IDs do not match"
            )
        for category, names in (
            ("wire", wires),
            ("instance", instances),
            ("assertion", assertions),
        ):
            for name in names:
                if "__" in name:
                    issues.append(
                        f"{path}: {category} {name!r} contains a double underscore"
                    )

        for port, connection in PORT_CONNECTION_RE.findall(body):
            value = connection.strip()
            if value and not SIMPLE_CONNECTION_RE.fullmatch(value):
                issues.append(f"{path}: port .{port}({value}) contains an expression")

        assignments = ASSIGN_RE.findall(body)
        for target, rhs in assignments:
            literals = _unsized_assignment_literals(rhs)
            if literals:
                issues.append(
                    f"{path}: assignment to {target!r} uses unsized value literal(s): "
                    + ", ".join(literals)
                )

        used = set(
            re.findall(
                r"\b[A-Za-z_][A-Za-z0-9_$]*\b", "\n".join(rhs for _, rhs in assignments)
            )
        )
        used.update(
            connection.strip() for _, connection in PORT_CONNECTION_RE.findall(body)
        )
        used.update(
            re.findall(
                r"\b[A-Za-z_][A-Za-z0-9_$]*\b",
                "\n".join(
                    match.group(0)
                    for match in re.finditer(r"(?:assert|cover)\s+property[^;]+", body)
                ),
            )
        )
        dead_wires = sorted(name for name in wires if name not in used)
        if dead_wires:
            issues.append(
                f"{path}: module {module!r} has dead internal nets: "
                + ", ".join(dead_wires)
            )
        expensive = {
            "multiply": sum(rhs.count("*") for _, rhs in assignments),
            "divide": sum(rhs.count("/") for _, rhs in assignments),
            "modulo": sum(rhs.count("%") for _, rhs in assignments),
        }
        reports.append(
            {
                "module": module,
                "wire_count": len(wires),
                "instance_count": len(instances),
                "assertion_count": len(assertions),
                "coverage_count": len(coverage),
                "dead_internal_nets": dead_wires,
                "expensive_operations": expensive,
            }
        )

    return issues, {"path": str(path), "ascii": True, "modules": reports}


def _write_html(first: Path, second: Path, output: Path, identical: bool) -> None:
    left = first.read_text(encoding="ascii").splitlines()
    right = second.read_text(encoding="ascii").splitlines()
    table = difflib.HtmlDiff(wrapcolumn=120).make_table(
        left, right, fromdesc=html.escape(str(first)), todesc=html.escape(str(second))
    )
    verdict = "byte-identical" if identical else "different"
    color = "#15803d" if identical else "#b91c1c"
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(
        "<!doctype html><meta charset='utf-8'><title>RTL deterministic diff</title>"
        f"<style>body{{font-family:system-ui;background:#111827;color:#e5e7eb}}"
        f"h1{{color:{color}}}table.diff{{font-family:ui-monospace,monospace;"
        "font-size:12px;background:#f8fafc;color:#111827}}</style>"
        f"<h1>Generated RTL: {verdict}</h1>{table}",
        encoding="utf-8",
    )


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("rtl", type=Path)
    parser.add_argument("--compare", type=Path)
    parser.add_argument("--json-out", type=Path)
    parser.add_argument("--html-out", type=Path)
    parser.add_argument("--require-synthesis-admissible", action="store_true")
    args = parser.parse_args()

    issues, report = audit(args.rtl)
    assertion_count = sum(
        int(module["assertion_count"]) for module in report.get("modules", [])
    )
    report["synthesis_admissible"] = assertion_count == 0
    if args.require_synthesis_admissible and assertion_count:
        issues.append(
            f"{args.rtl}: runtime-checked obligations cannot authorize synthesis"
        )
    if args.compare:
        other_issues, other_report = audit(args.compare)
        issues.extend(other_issues)
        identical = args.rtl.read_bytes() == args.compare.read_bytes()
        if not identical:
            issues.append(f"{args.rtl} and {args.compare} are not byte-identical")
        report["comparison"] = {
            "path": str(args.compare),
            "byte_identical": identical,
            "audit": other_report,
        }
        if args.html_out:
            _write_html(args.rtl, args.compare, args.html_out, identical)
    elif args.html_out:
        issues.append("--html-out requires --compare")

    report["issues"] = issues
    if args.json_out:
        args.json_out.parent.mkdir(parents=True, exist_ok=True)
        args.json_out.write_text(
            json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8"
        )
    if issues:
        for issue in issues:
            print(f"error: {issue}")
        return 1
    print(f"generated RTL audit: PASS ({args.rtl})")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
