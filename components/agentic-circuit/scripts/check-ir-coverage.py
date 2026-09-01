#!/usr/bin/env python3
"""Enforce complete public IR coverage for the current ACIR contract.

The contracts/*.yaml manifests are the normative source of truth for the
public IR surface. This checker requires that

- the ODS definitions match the manifests exactly (any added, renamed,
  removed, or implementation-only public operation or type must be reflected
  in the manifest in the same commit),
- both manifests declare the current contract epoch,
- every manifest entry resolves to a source symbol in the ODS definitions,
- every manifest entry has observable positive and negative lit coverage,
- the dialect registration tables register the generated op/type lists, and
- docs/spec/50-verification/ir-coverage.md is the up-to-date generated ledger.

Run with --write-ledger to regenerate the ledger after a deliberate surface
change. The default mode is read-only and exits non-zero on any gap.
"""

import argparse
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
LEDGER_PATH = "docs/spec/50-verification/ir-coverage.md"

DIALECTS = {
    "acir": {
        "manifest": "contracts/acir.yaml",
        "contract_epoch": "0.4",
        "ops_td": "include/acir/Dialect/ACIR/ACIROps.td",
        "types_td": "include/acir/Dialect/ACIR/ACIRTypes.td",
        "registration": "lib/Dialect/ACIR/ACIRTypes.cpp",
        "registration_ops_inc": "acir/Dialect/ACIR/ACIROps.cpp.inc",
        "registration_types_inc": "acir/Dialect/ACIR/ACIRTypes.cpp.inc",
        "op_prefix": "ac.",
        "type_prefix": "!ac.",
    },
    "acsim": {
        "manifest": "contracts/acsim.yaml",
        "contract_epoch": "0.4",
        "ops_td": "include/acir/Dialect/ACSim/ACSimOps.td",
        "types_td": "include/acir/Dialect/ACSim/ACSimTypes.td",
        "registration": "lib/Dialect/ACSim/ACSimTypes.cpp",
        "registration_ops_inc": "acir/Dialect/ACSim/ACSimOps.cpp.inc",
        "registration_types_inc": "acir/Dialect/ACSim/ACSimTypes.cpp.inc",
        "op_prefix": "acsim.",
        "type_prefix": "!acsim.",
    },
}

MANIFEST_SCHEMA = "acir-ir-inventory"

OP_DEF_PATTERN = re.compile(
    r"def\s+(\w+Op)\s*:\s*\w+<\"([^\"]+)\"", re.MULTILINE
)
TYPE_DEF_PATTERN = re.compile(
    r"def\s+(\w+)\s*:\s*\w+<\"[^\"]+\",\s*\"([^\"]+)\"", re.MULTILINE
)


def load_manifest(root, dialect, errors):
    """Parse and validate one normative inventory manifest."""
    spec = DIALECTS[dialect]
    path = root / spec["manifest"]
    if not path.is_file():
        errors.append(f"missing normative inventory manifest: {spec['manifest']}")
        return None
    try:
        import yaml
    except ImportError:
        errors.append("pyyaml is unavailable; install requirements-dev.lock")
        return None
    try:
        document = yaml.safe_load(path.read_text())
    except yaml.YAMLError as exc:
        errors.append(f"{spec['manifest']} does not parse: {exc}")
        return None
    if not isinstance(document, dict):
        errors.append(f"{spec['manifest']} must be a mapping")
        return None
    if document.get("schema") != MANIFEST_SCHEMA:
        errors.append(
            f"{spec['manifest']} must declare schema: {MANIFEST_SCHEMA}"
        )
    epoch = document.get("contract_epoch")
    expected_epoch = spec["contract_epoch"]
    if epoch != expected_epoch:
        errors.append(
            f"{spec['manifest']} declares stale contract_epoch {epoch!r}; "
            f"expected {expected_epoch!r}"
        )
    if document.get("dialect") != dialect:
        errors.append(
            f"{spec['manifest']} declares dialect {document.get('dialect')!r}; "
            f"expected {dialect!r}"
        )
    result = {}
    for field in ("operations", "types"):
        entries = document.get(field)
        if not isinstance(entries, list) or not all(
            isinstance(entry, str) for entry in entries
        ):
            errors.append(f"{spec['manifest']} field {field} must be a list of strings")
            return None
        if len(entries) != len(set(entries)):
            errors.append(f"{spec['manifest']} field {field} contains duplicates")
        result[field] = entries
    return result


def extract_ops(td_path):
    """Extract {mnemonic: def name} for every op definition in a .td file."""
    if not td_path.is_file():
        return None
    text = td_path.read_text()
    return {match.group(2): match.group(1) for match in OP_DEF_PATTERN.finditer(text)}


def extract_types(td_path):
    """Extract {mnemonic: def name} for every type definition in a .td file."""
    if not td_path.is_file():
        return None
    text = td_path.read_text()
    return {
        match.group(2): match.group(1)
        for match in TYPE_DEF_PATTERN.finditer(text)
        if match.group(1).endswith("Type")
    }


def check_ods_surface(root, errors):
    """Require exact set equality between manifests and ODS definitions."""
    surface = {}
    for dialect, spec in DIALECTS.items():
        manifest = load_manifest(root, dialect, errors)
        ops = extract_ops(root / spec["ops_td"])
        types = extract_types(root / spec["types_td"])
        if ops is None:
            errors.append(f"missing ODS file: {spec['ops_td']}")
            ops = {}
        if types is None:
            errors.append(f"missing ODS file: {spec['types_td']}")
            types = {}
        if manifest is None:
            continue

        op_prefix = spec["op_prefix"]
        manifest_ops = {
            name[len(op_prefix):]
            for name in manifest["operations"]
            if name.startswith(op_prefix)
        }
        malformed = [
            name for name in manifest["operations"] if not name.startswith(op_prefix)
        ]
        for name in malformed:
            errors.append(
                f"{spec['manifest']} operation {name!r} lacks the {op_prefix!r} prefix"
            )
        missing_ops = sorted(manifest_ops - set(ops))
        extra_ops = sorted(set(ops) - manifest_ops)
        for name in missing_ops:
            errors.append(
                f"{dialect} operation {op_prefix}{name} has no source symbol "
                f"in {spec['ops_td']}"
            )
        for name in extra_ops:
            errors.append(
                f"{dialect} ODS operation {ops[name]} ({op_prefix}{name}) is an "
                f"implementation-only public alias missing from {spec['manifest']}"
            )

        manifest_types = set(manifest["types"])
        missing_types = sorted(manifest_types - set(types))
        extra_types = sorted(set(types) - manifest_types)
        for name in missing_types:
            errors.append(
                f"{dialect} type {spec['type_prefix']}{name} has no source "
                f"symbol in {spec['types_td']}"
            )
        for name in extra_types:
            errors.append(
                f"{dialect} ODS type {types[name]} ({spec['type_prefix']}{name}) "
                f"is an implementation-only public alias missing from "
                f"{spec['manifest']}"
            )

        surface[dialect] = {
            "manifest": manifest,
            "ops": ops,
            "types": types,
        }
    return surface


def check_registration_tables(root, errors):
    """Require the dialect initialize() tables to register generated lists."""
    for dialect, spec in DIALECTS.items():
        path = root / spec["registration"]
        if not path.is_file():
            errors.append(f"missing registration table: {spec['registration']}")
            continue
        text = path.read_text()
        for inc, kind in (
            (spec["registration_ops_inc"], "operations"),
            (spec["registration_types_inc"], "types"),
        ):
            if inc not in text:
                errors.append(
                    f"{spec['registration']} does not register the generated "
                    f"{kind} list ({inc})"
                )


def lit_test_files(root):
    test_dir = root / "test"
    positive = []
    negative = []
    if test_dir.is_dir():
        for path in sorted(test_dir.rglob("*.mlir")):
            relative = path.relative_to(root).as_posix()
            if "invalid" in path.name:
                negative.append((relative, path))
            else:
                positive.append((relative, path))
    return positive, negative


def coverage_pattern(needle):
    return re.compile(rf"(?<![\w.!]){re.escape(needle)}(?![\w.])")


def find_coverage(needle, files):
    pattern = coverage_pattern(needle)
    matches = []
    for relative, path in files:
        try:
            text = path.read_text()
        except UnicodeDecodeError:
            continue
        if pattern.search(text):
            matches.append(relative)
    return matches


def collect_coverage(root, surface, errors=None):
    """Map every manifest entry to its positive/negative lit coverage."""
    positive, negative = lit_test_files(root)
    ledger = {}
    for dialect, spec in DIALECTS.items():
        data = surface.get(dialect)
        if data is None:
            continue
        entries = []
        for full_name in data["manifest"]["operations"]:
            mnemonic = full_name[len(spec["op_prefix"]):]
            symbol = data["ops"].get(mnemonic)
            pos = find_coverage(full_name, positive)
            neg = find_coverage(full_name, negative)
            if errors is not None:
                if not pos:
                    errors.append(
                        f"{full_name} has no positive lit test coverage"
                    )
                if not neg:
                    errors.append(
                        f"{full_name} has no negative lit test coverage"
                    )
            entries.append(
                {
                    "kind": "operation",
                    "name": full_name,
                    "symbol": symbol or "(missing)",
                    "positive": pos,
                    "negative": neg,
                }
            )
        for name in data["manifest"]["types"]:
            needle = f"{spec['type_prefix']}{name}"
            symbol = data["types"].get(name)
            pos = find_coverage(needle, positive)
            neg = find_coverage(needle, negative)
            if errors is not None:
                if not pos:
                    errors.append(f"{needle} has no positive lit test coverage")
                if not neg:
                    errors.append(f"{needle} has no negative lit test coverage")
            entries.append(
                {
                    "kind": "type",
                    "name": needle,
                    "symbol": symbol or "(missing)",
                    "positive": pos,
                    "negative": neg,
                }
            )
        ledger[dialect] = entries
    return ledger


def render_ledger(root, surface):
    """Render the deterministic generated coverage ledger."""
    coverage = collect_coverage(root, surface)
    lines = [
        "# ACIR / ACSim coverage ledger",
        "",
        "<!-- Generated by scripts/check-ir-coverage.py --write-ledger.",
        "     Do not edit by hand; regenerate after any deliberate change to",
        "     the public IR surface, the normative manifests, or lit tests. -->",
        "",
    ]
    for dialect, spec in DIALECTS.items():
        lines.append(f"{dialect}_contract_epoch: {spec['contract_epoch']}")
    lines.append("")
    for dialect, entries in coverage.items():
        operations = [entry for entry in entries if entry["kind"] == "operation"]
        types = [entry for entry in entries if entry["kind"] == "type"]
        lines.append(f"## {dialect} operations ({len(operations)})")
        lines.append("")
        lines.append("| operation | source symbol | positive coverage | negative coverage |")
        lines.append("| --- | --- | --- | --- |")
        for entry in operations:
            lines.append(_ledger_row(entry))
        lines.append("")
        lines.append(f"## {dialect} types ({len(types)})")
        lines.append("")
        lines.append("| type | source symbol | positive coverage | negative coverage |")
        lines.append("| --- | --- | --- | --- |")
        for entry in types:
            lines.append(_ledger_row(entry))
        lines.append("")
    return "\n".join(lines)


def _ledger_row(entry):
    positive = "<br>".join(entry["positive"]) if entry["positive"] else "(none)"
    negative = "<br>".join(entry["negative"]) if entry["negative"] else "(none)"
    return (
        f"| {entry['name']} | {entry['symbol']} | {positive} | {negative} |"
    )


def check_ledger(root, surface, errors):
    """Require the committed ledger to match the freshly generated one."""
    path = root / LEDGER_PATH
    if not path.is_file():
        errors.append(
            f"missing coverage ledger: {LEDGER_PATH} "
            f"(run scripts/check-ir-coverage.py --write-ledger)"
        )
        return
    expected = render_ledger(root, surface)
    if path.read_text() != expected:
        errors.append(
            f"{LEDGER_PATH} is stale; regenerate with "
            f"scripts/check-ir-coverage.py --write-ledger"
        )


def run_checks(root):
    errors = []
    surface = check_ods_surface(root, errors)
    check_registration_tables(root, errors)
    collect_coverage(root, surface, errors)
    check_ledger(root, surface, errors)
    return errors


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--write-ledger",
        action="store_true",
        help=f"regenerate {LEDGER_PATH} instead of only verifying it",
    )
    args = parser.parse_args(argv)

    if args.write_ledger:
        errors = []
        surface = check_ods_surface(ROOT, errors)
        if errors:
            for error in errors:
                print(f"error: {error}", file=sys.stderr)
            return 1
        path = ROOT / LEDGER_PATH
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(render_ledger(ROOT, surface))
        print(f"wrote {LEDGER_PATH}")
        return 0

    errors = run_checks(ROOT)
    if errors:
        for error in errors:
            print(f"error: {error}", file=sys.stderr)
        return 1
    print(
        "IR coverage: OK (ACIR and ACSim manifests match ODS, "
        "positive+negative lit coverage complete, ledger current)"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
