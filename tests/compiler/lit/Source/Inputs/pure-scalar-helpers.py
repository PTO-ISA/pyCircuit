"""Scalar pure helper admission and shared native/RTL execution."""

import argparse
import ast
import hashlib
import json
import os
import re
import runpy
import subprocess
import sys
import tempfile
from pathlib import Path

from enum_source_execution import execution_tools

parser = argparse.ArgumentParser()
for option in (
    "repo",
    "source-compiler",
    "linker",
    "optimizer",
    "emitter",
    "cxx",
    "verilator",
    "iverilog",
    "vvp",
    "scratch",
):
    parser.add_argument("--" + option, required=True)
parser.add_argument("--prepare-only", action="store_true")
args = parser.parse_args()
repo = Path(args.repo).resolve()
fixtures = Path(__file__).resolve().parent
evidence = Path(args.scratch).resolve()
evidence.mkdir(parents=True, exist_ok=True)
build = Path(tempfile.mkdtemp(prefix="run-", dir=evidence))
source = build / "source"
source.mkdir()
env = dict(
    os.environ,
    PYTHONPATH=str(repo / "python/pycircuit/src"),
    PYTHONDONTWRITEBYTECODE="1",
    PYCIRCUIT_SOURCE_COMPILER=args.source_compiler,
    PYCIRCUIT_LINKER=args.linker,
    PYCIRCUIT_EMITTER=args.emitter,
)
commands = []
source_stages = []


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run(command, code=0, diagnostic=None):
    command = list(map(str, command))
    result = subprocess.run(
        command, cwd=repo, env=env, text=True, capture_output=True, timeout=120
    )
    row = {
        "command": command,
        "exit_status": result.returncode,
        "stdout": result.stdout,
        "stderr": result.stderr,
    }
    commands.append(row)
    (evidence / "commands.json").write_text(json.dumps(commands, indent=2) + "\n")
    assert result.returncode == code, row
    assert (
        "Assertion failed" not in result.stderr and "Traceback" not in result.stderr
    ), row
    if diagnostic:
        assert diagnostic in result.stderr, row
    return result


def cli(*arguments, code=0, diagnostic=None):
    return run([sys.executable, "-m", "pycircuit.cli", *arguments], code, diagnostic)


def compile_unit(filename, output, imports=(), replace=False, code=0, diagnostic=None):
    command = [
        "compile",
        "-c",
        source / filename,
        "--source-root",
        source,
        "--package-prefix",
        "enums",
        "-o",
        output,
    ]
    assert not imports
    if replace:
        command.append("--replace")
    result = cli(*command, code=code, diagnostic=diagnostic)
    if code == 0:
        retain_source_stages(filename, Path(output))
    return result


def retain_source_stages(filename, unit):
    # Reproduce the same successful source owner with the existing capture and
    # native compiler. This diagnostic is not a published-unit sidecar.
    sys.path.insert(0, str(repo / "python/pycircuit/src"))
    from pycircuit._source_capture import _capture_source_file
    from pycircuit._source_transport import _emit_source_transport

    stages = build / "native-reproduction" / Path(filename).stem
    stages.mkdir(parents=True)
    capture = stages / "capture.transport.mlir"
    capture.write_text(
        _emit_source_transport(
            _capture_source_file(source / filename, source_root=source)
        ),
        encoding="utf-8",
    )
    imported = stages / "source-import.ac"
    run(
        [
            args.source_compiler,
            "--capture",
            capture,
            "--package",
            "enums",
            "--path",
            filename,
            "--body-out",
            stages / "body.ac",
            "--interface-out",
            stages / "interface.ac",
            "--deps-out",
            stages / "deps.json",
            "--source-import-out",
            imported,
        ]
    )
    for kind in ("body", "interface"):
        assert (stages / (kind + ".ac")).read_bytes() == payload(
            unit, kind
        ).read_bytes()
    run([args.optimizer, imported, "-o", stages / "parsed-source-import.ac"])
    source_stages.append(
        {
            "filename": filename,
            "owner": {"package": "enums", "path": filename},
            "source_sha256": digest(source / filename),
            "headers": [],
            "capture": str(capture),
            "source_import": str(imported),
            "body": str(stages / "body.ac"),
            "interface": str(stages / "interface.ac"),
            "published_body_interface_byte_equal": True,
            "full_source_import_parsed_verified": True,
        }
    )


def payload(unit, kind):
    return unit / json.loads((unit / "unit.json").read_text())["files"][kind]


def snapshot(path):
    if path.is_file():
        return path.read_bytes()
    return {
        p.relative_to(path).as_posix(): p.read_bytes()
        for p in path.rglob("*")
        if p.is_file()
    }


def managed_snapshot(path):
    control = path.parent / ("." + path.name + ".pycircuit-publication")
    return {"payload": snapshot(path), "publication": snapshot(control)}


oracles = runpy.run_path(str(fixtures / "pure-scalar-helpers-oracles.py"))
admission = runpy.run_path(str(fixtures / "pure-scalar-helpers-admission.py"))
oracles["self_check"]()
cases = [oracles["scalar_case"](), oracles["count_case"](), oracles["named_map_case"]()]
negatives = admission["admission_cases"]()
controls = admission["positive_controls"]()
for case in cases + controls + negatives:
    (source / (case["name"] + ".py")).write_text(case["text"])
# Source occurrence accounting is independent of output coincidence. Count is
# written three times; nested actual calls are finite syntax, not body recursion.
count_tree = ast.parse(oracles["COUNT_DESIGN"])
count_sites = [
    node
    for node in ast.walk(count_tree)
    if isinstance(node, ast.Call)
    and isinstance(node.func, ast.Attribute)
    and node.func.attr == "count"
]
assert len(count_sites) == 3
files = [
    Path(__file__),
    fixtures / "pure-scalar-helpers-oracles.py",
    fixtures / "pure-scalar-helpers-admission.py",
    fixtures / "enum_source_execution.py",
    fixtures / "enum-source.cpp",
    fixtures / "enum-source.sv",
]
prepared = {
    "artifact_directory": str(build),
    "execution_cases": cases,
    "negative_cases": negatives,
    "positive_controls": controls,
    "source_count_sites": [
        {"line": n.lineno, "column": n.col_offset} for n in count_sites
    ],
    "fixtures": {str(p): digest(p) for p in files},
    "oracle_contract": "unsigned modular arithmetic; full bit-symbol known/X/Z oracles; fresh/replacement publication protection",
}
(evidence / "prepared.json").write_text(json.dumps(prepared, indent=2) + "\n")
if args.prepare_only:
    print(build)
    sys.exit(0)

runtime_root = Path(args.source_compiler).resolve().parent.parent
runtime = next(
    p
    for p in (
        runtime_root / "simulator/gfsim/libpyc6_runtime.a",
        runtime_root / "lib/libpyc6_runtime.a",
    )
    if p.is_file()
)
_, execute = execution_tools(
    args=args,
    repo=repo,
    fixtures=fixtures,
    build=build,
    source=source,
    units={},
    declaration_sources=(),
    runtime=runtime,
    compile_unit=compile_unit,
    cli=cli,
    run=run,
    payload=payload,
    digest=digest,
    clock=oracles["CLOCK"],
)
receipts = [execute(**case) for case in cases]
# Count is a canonical query operation in the untransformed source body. Match
# source locations as well as occurrence count; coincident runtime values do
# not establish once-only actual evaluation.
count_receipt = next(row for row in receipts if row["name"] == "pure_count")
count_body = payload(Path(count_receipt["output"]) / "unit", "body")
folds = [
    line for line in count_body.read_text().splitlines() if '"ac.table.fold"' in line
]
expected_sites = {(node.lineno, node.col_offset + 1) for node in count_sites}
observed_sites = []
for line in folds:
    metadata = re.findall(
        r'location = \{column = (\d+) : i64, end_column = \d+ : i64, end_line = \d+ : i64, line = (\d+) : i64, path = "pure_count.py"',
        line,
    )
    matching = {
        (int(source_line), int(column)) for column, source_line in metadata
    } & expected_sites
    assert len(matching) == 1, line
    observed_sites.append(next(iter(matching)))
assert (
    len(observed_sites) == len(expected_sites) and set(observed_sites) == expected_sites
)
occurrence_receipt = {
    "source_count_calls": len(count_sites),
    "published_body_queries": len(folds),
    "sites": sorted(observed_sites),
    "body_sha256": digest(count_body),
}
map_tree = ast.parse(oracles["NAMED_MAP_DESIGN"])
ordered = next(
    node
    for node in ast.walk(map_tree)
    if isinstance(node, ast.Assign)
    and any(
        isinstance(target, ast.Name) and target.id == "ordered"
        for target in node.targets
    )
)
actual_calls = [ordered.value.func.value, ordered.value.args[1], ordered.value]
expected_map_sites = [
    (node.lineno, node.col_offset + 1, node.end_lineno, node.end_col_offset + 1)
    for node in actual_calls
]
map_stage = next(
    row for row in source_stages if row["filename"] == "named_fixed_map.py"
)
map_lines = [
    line
    for line in Path(map_stage["source_import"]).read_text().splitlines()
    if '"ac.table.map"' in line
]
observed_map_sites = []
for line in map_lines:
    positions = re.findall(
        r'location = \{column = (\d+) : i64, end_column = (\d+) : i64, end_line = (\d+) : i64, line = (\d+) : i64, path = "named_fixed_map.py"',
        line,
    )
    matches = {
        (int(first_line), int(column), int(last_line), int(end_column))
        for column, end_column, last_line, first_line in positions
    } & set(expected_map_sites)
    if matches:
        assert len(matches) == 1, line
        observed_map_sites.append(next(iter(matches)))
assert observed_map_sites == expected_map_sites, (
    observed_map_sites,
    expected_map_sites,
)
map_occurrences = {
    "receiver_then_additional_actual_then_callback": observed_map_sites,
    "source_import_sha256": digest(Path(map_stage["source_import"])),
    "scope": "one actual Table map operation per written actual occurrence, in source order; output coincidence is not an evaluation-count oracle",
}
control_receipts = []
for control in controls:
    unit = build / (control["name"] + ".unit")
    compile_unit(control["name"] + ".py", unit)
    final = build / (control["name"] + ".ac")
    cli("link", unit, "--top", "enums." + control["name"] + ".Top", "-o", final)
    run(
        [
            args.optimizer,
            final,
            "--ac-verify-hardware",
            "-o",
            build / (control["name"] + ".verified.ac"),
        ]
    )
    control_receipts.append(
        {
            "name": control["name"],
            "unit": str(unit),
            "final": str(final),
            "final_sha256": digest(final),
        }
    )
protected = []
for receipt in receipts:
    output = Path(receipt["output"])
    protected.extend(
        (output / "unit", output / "design.ac", output / "cpp", output / "verilog")
    )
for receipt in control_receipts:
    protected.extend((Path(receipt["unit"]), Path(receipt["final"])))
protected_before = {str(p): managed_snapshot(p) for p in protected}
guard = Path(receipts[0]["output"]) / "unit"
negative_receipts = []
for case in negatives:
    filename = case["name"] + ".py"
    diagnostics = []
    expected_binding_site = None
    if "diagnostic_actual" in case:
        calls = [
            node
            for node in ast.walk(ast.parse(case["text"]))
            if isinstance(node, ast.Call)
            and isinstance(node.func, ast.Attribute)
            and node.func.attr == "map"
            and node.args
            and isinstance(node.args[0], ast.Name)
            and node.args[0].id == "helper"
        ]
        assert len(calls) == 1, case["name"]
        call = calls[0]
        actual = (
            call.func.value
            if case["diagnostic_actual"] == 0
            else call.args[case["diagnostic_actual"]]
        )
        expected_binding_site = (actual.lineno, actual.col_offset + 1)
    for replacement in (False, True):
        destination = (
            guard if replacement else build / ("rejected-" + case["name"] + ".unit")
        )
        result = compile_unit(filename, destination, replace=replacement, code=1)
        # The first semantic diagnostic is the sequencing oracle. Later errors
        # must not make a wrong first diagnostic appear to pass.
        first = next(
            line for line in result.stderr.splitlines() if "error:" in line
        ).lower()
        assert any(fragment in first for fragment in case["categories"]), (
            case,
            result.stderr,
        )
        if expected_binding_site:
            line, column = expected_binding_site
            assert re.search(
                r'loc\("(?:[^"\n]*/)?' + re.escape(filename) + rf'":{line}:{column}\)',
                first,
            ), (case, first, expected_binding_site)
        if not replacement:
            assert not destination.exists(), destination
        assert {
            str(p): managed_snapshot(p) for p in protected
        } == protected_before, case["name"]
        diagnostics.append(result.stderr)
    negative_receipts.append(
        {
            "name": case["name"],
            "source_sha256": digest(source / filename),
            "fresh_diagnostic": diagnostics[0],
            "replacement_diagnostic": diagnostics[1],
            "protected_publications_unchanged": True,
            "expected_binding_actual_site": expected_binding_site,
        }
    )
artifacts = {str(p): digest(p) for p in sorted(build.rglob("*")) if p.is_file()}
result = {
    "artifact_directory": str(build),
    "cases": receipts,
    "negatives": negative_receipts,
    "controls": control_receipts,
    "source_occurrences": occurrence_receipt,
    "named_map_actual_occurrences": map_occurrences,
    "source_stages": source_stages,
    "protected_publications_unchanged": True,
    "fixtures": {str(p): digest(p) for p in files},
    "artifacts": artifacts,
    "runtime": {str(runtime): digest(runtime)},
    "tools": {
        str(Path(p).resolve()): digest(Path(p))
        for p in (
            args.source_compiler,
            args.linker,
            args.optimizer,
            args.emitter,
            args.cxx,
            args.verilator,
            args.iverilog,
            args.vvp,
        )
    },
    "scope": "fixed scalar expression helpers and named fixed Table maps; three runtime roots; no effects or helper calls inside arbitrary Lambdas",
}
(evidence / "candidate.json").write_text(json.dumps(result, indent=2) + "\n")
print(
    json.dumps(
        {
            "cases": len(receipts),
            "negatives": len(negative_receipts),
            "evidence": str(evidence),
        }
    )
)
