"""Table methods through public source units and the existing execution owner."""

import argparse
import hashlib
import json
import os
import runpy
import subprocess
import sys
import tempfile
from pathlib import Path

from enum_source_execution import execution_tools

parser = argparse.ArgumentParser()
for option in ("repo", "source-compiler", "linker", "optimizer", "scratch"):
    parser.add_argument("--" + option, required=True)
for option in ("emitter", "cxx", "verilator", "iverilog", "vvp"):
    parser.add_argument("--" + option)
parser.add_argument("--prepare-only", action="store_true")
parser.add_argument("--admission-only", action="store_true")
parser.add_argument("--execution-only", action="store_true")
parser.add_argument("--case", action="append", default=[])
args = parser.parse_args()
assert not (args.admission_only and args.execution_only)
repo = Path(args.repo).resolve()
fixtures = Path(__file__).resolve().parent
evidence = Path(args.scratch).resolve()
evidence.mkdir(parents=True, exist_ok=True)
build = Path(tempfile.mkdtemp(prefix="run-", dir=evidence))
source = build / "source"
source.mkdir()
env = dict(
    os.environ,
    PYTHONPATH=str(repo / "python"),
    PYTHONDONTWRITEBYTECODE="1",
    PYCIRCUIT_SOURCE_COMPILER=args.source_compiler,
    PYCIRCUIT_LINKER=args.linker,
)
if args.emitter:
    env["PYCIRCUIT_EMITTER"] = args.emitter
commands = []


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
    for unit in imports:
        command.extend(("-I", unit))
    if replace:
        command.append("--replace")
    return cli(*command, code=code, diagnostic=diagnostic)


def payload(unit, kind):
    receipt = json.loads((unit / "unit.json").read_text())
    return unit / receipt["files"][kind]


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


oracles = runpy.run_path(str(fixtures / "table-methods-oracles.py"))
admission = runpy.run_path(str(fixtures / "table-methods-admission.py"))
oracles["self_check"]()
cases = oracles["reduction_cases"]((fixtures / "table-methods-design.py").read_text())
cases.extend(
    (
        oracles["construction_case"](),
        oracles["gather_case"](),
        oracles["count_lookup_case"](),
        oracles["nominal_case"](),
    )
)
if args.case:
    selected = set(args.case)
    assert selected <= {case["name"] for case in cases}, selected
    cases = [case for case in cases if case["name"] in selected]
negative_cases = admission["admission_cases"]()
negative_directory = source / "negative"
negative_directory.mkdir()
for case in cases:
    (source / (case["name"] + ".py")).write_text(case["text"])
for case in negative_cases:
    (negative_directory / (case["name"] + ".py")).write_text(case["text"])
provider_filename = "table_methods_provider.py"
(source / provider_filename).write_text(
    (fixtures / "table-methods-provider.py").read_text()
)
prepared = {
    "artifact_directory": str(build),
    "execution_cases": cases,
    "negative_cases": negative_cases,
    "fixtures": {
        str(p): digest(p)
        for p in sorted(fixtures.glob("table-methods*"))
        if p.is_file()
    },
    "oracle_contract": "MSB-first Table/Struct layout; adjacent balanced pairs with odd-tail carry; unsigned wraparound and four-state bit symbols",
}
(evidence / "prepared.json").write_text(json.dumps(prepared, indent=2) + "\n")
if args.prepare_only:
    print(build)
    sys.exit(0)
assert all(
    (args.emitter, args.cxx, args.verilator, args.iverilog, args.vvp)
), "execution requires native and RTL tools"

provider = build / "provider.unit"
compile_unit(provider_filename, provider)
provider_header = payload(provider, "interface")
assert all(name in provider_header.read_text() for name in ("Item", "Tag", "Parcel"))
# No provider source fallback is possible during every consumer compile below.
(source / provider_filename).unlink()
units = {"provider": provider}
runtime_root = Path(args.source_compiler).resolve().parent.parent
runtime = next(
    p
    for p in (
        runtime_root / "runtime/libpyc6_runtime.a",
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
    units=units,
    declaration_sources=("provider",),
    runtime=runtime,
    compile_unit=compile_unit,
    cli=cli,
    run=run,
    payload=payload,
    digest=digest,
    clock=oracles["CLOCK"],
)
receipts = [] if args.admission_only else [execute(**case) for case in cases]

guard_text = """import pycircuit as ac
@ac.struct
class Result:
    value: ac.bits[9]
@ac.module
def Top(value: ac.bits[9]) -> Result:
    return Result(value=value)
"""
(source / "guard.py").write_text(guard_text)
guard = build / "guard.unit"
compile_unit("guard.py", guard)
guard_final = build / "guard.ac"
cli("link", guard, "--top", "enums.guard.Top", "-o", guard_final)
run(
    [
        args.optimizer,
        guard_final,
        "--ac-verify-hardware",
        "-o",
        build / "guard.verified.ac",
    ]
)
for target in ("cpp", "verilog"):
    cli("emit", guard_final, "--target", target, "-o", build / ("guard-" + target))
protected = [guard, provider, guard_final, build / "guard-cpp", build / "guard-verilog"]
for receipt in receipts:
    output = Path(receipt["output"])
    protected.extend(
        (output / "unit", output / "design.ac", output / "cpp", output / "verilog")
    )
protected_before = {str(path): managed_snapshot(path) for path in protected}
negative_receipts = []
for case in () if args.execution_only else negative_cases:
    filename = "negative/" + case["name"] + ".py"
    fresh = build / ("rejected-" + case["name"] + ".unit")
    fresh_result = compile_unit(filename, fresh, (provider,), code=1)
    fresh_diagnostic = fresh_result.stderr.rsplit("error:", 1)[-1].lower()
    assert "error:" in fresh_result.stderr.lower() and any(
        fragment in fresh_diagnostic for fragment in case["categories"]
    ), (case, fresh_result.stderr)
    assert not fresh.exists(), fresh
    replacement_result = compile_unit(
        filename, guard, (provider,), replace=True, code=1
    )
    replacement_diagnostic = replacement_result.stderr.rsplit("error:", 1)[-1].lower()
    assert "error:" in replacement_result.stderr.lower() and any(
        fragment in replacement_diagnostic for fragment in case["categories"]
    ), (case, replacement_result.stderr)
    assert {
        str(path): managed_snapshot(path) for path in protected
    } == protected_before, case["name"]
    negative_receipts.append(
        {
            "name": case["name"],
            "source_sha256": digest(source / filename),
            "fresh_rejected": True,
            "replacement_rejected": True,
            "fresh_diagnostic": fresh_result.stderr,
            "replacement_diagnostic": replacement_result.stderr,
            "protected_publications_unchanged": True,
        }
    )

# Imported identity cannot be substituted with a local, same-width nominal Item.
nominal_bad = (
    oracles["NOMINAL_DESIGN"]
    .replace(
        "@ac.struct\nclass Result:",
        "@ac.struct\nclass PeerItem:\n    tag: Tag\n    data: ac.bits[9]\n@ac.struct\nclass Result:",
    )
    .replace("identity: ac.table[3, Item]", "identity: ac.table[3, PeerItem]")
)
(source / "nominal_wrong.py").write_text(nominal_bad)
for replacement in () if args.execution_only else (False, True):
    destination = guard if replacement else build / "nominal-wrong.unit"
    result = compile_unit(
        "nominal_wrong.py", destination, (provider,), replace=replacement, code=1
    )
    assert "error" in result.stderr.lower(), result.stderr
    if not replacement:
        assert not destination.exists()
    assert {str(path): managed_snapshot(path) for path in protected} == protected_before

files = [
    Path(__file__),
    *sorted(fixtures.glob("table-methods-*.py")),
    fixtures / "enum_source_execution.py",
    fixtures / "enum-source.cpp",
    fixtures / "enum-source.sv",
]
result = {
    "artifact_directory": str(build),
    "cases": receipts,
    "negatives": negative_receipts,
    "imported_same_width_nominal_rejected": not args.execution_only,
    "provider_python_absent": True,
    "protected_publications_unchanged": True,
    "fixtures": {str(p): digest(p) for p in files},
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
    "scope": "Table expression lambdas and existing closed reductions; no helpers or scan",
}
(evidence / "candidate.json").write_text(json.dumps(result, indent=2) + "\n")
print(
    json.dumps(
        {
            "cases": len(receipts),
            "negatives": len(negative_receipts) + (not args.execution_only),
            "evidence": str(evidence),
        }
    )
)
