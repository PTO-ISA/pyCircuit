"""Actual registered extraction versus public authority and protected publication."""

import argparse
import ast
import hashlib
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

parser = argparse.ArgumentParser()
for name in ("repo", "source-compiler", "linker", "emitter", "optimizer", "scratch"):
    parser.add_argument("--" + name, required=True)
parser.add_argument("--invalid", action="store_true")
parser.add_argument("--query-checks", choices=("syntax", "admission", "resources"))
args = parser.parse_args()
repo = Path(args.repo).resolve()
scratch = Path(args.scratch).resolve()
scratch.mkdir(parents=True, exist_ok=True)
private = tempfile.TemporaryDirectory(prefix="interface-", dir=scratch)
root = Path(private.name)
source = root / "source"
source.mkdir()
fixtures = Path(__file__).resolve().parent
for name in ("source_interface_child.py", "source_interface_parent.py"):
    shutil.copyfile(fixtures / name, source / name)
env = dict(
    os.environ,
    PYTHONPATH=str(repo / "python"),
    PYTHONDONTWRITEBYTECODE="1",
    PYCIRCUIT_SOURCE_COMPILER=args.source_compiler,
    PYCIRCUIT_LINKER=args.linker,
    PYCIRCUIT_EMITTER=args.emitter,
)
commands = []


def run(command, accepted=True):
    result = subprocess.run(
        list(map(str, command)),
        cwd=repo,
        env=env,
        capture_output=True,
        text=True,
        timeout=30,
    )
    row = {
        "command": list(map(str, command)),
        "exit_status": result.returncode,
        "stdout": result.stdout,
        "stderr": result.stderr,
    }
    commands.append(row)
    (scratch / "commands.json").write_text(json.dumps(commands, indent=2) + "\n")
    assert (
        "Assertion failed" not in result.stderr and "Traceback" not in result.stderr
    ), row
    assert result.returncode == (0 if accepted else 1), row
    if not accepted:
        assert result.stderr.strip(), row
    return result


def cli(*arguments, accepted=True):
    return run([sys.executable, "-m", "pycircuit.cli", *arguments], accepted)


def compile_unit(name, output, imports=(), replace=False, accepted=True):
    command = [
        "compile",
        "-c",
        source / name,
        "--source-root",
        source,
        "--package-prefix",
        "interfaces",
        "-o",
        output,
    ]
    for unit in imports:
        command.extend(("-I", unit))
    if replace:
        command.append("--replace")
    return cli(*command, accepted=accepted)


def payload(unit, kind):
    receipt = json.loads((unit / "unit.json").read_text())
    return unit / receipt["files"][kind]


def snapshot(directory):
    return {
        path.relative_to(directory).as_posix(): path.read_bytes()
        for path in directory.rglob("*")
        if path.is_file()
    }


def normalize(path, output):
    run([args.optimizer, path, "--mlir-print-debuginfo", "-o", output])
    return output.read_text()


def extract(path, output, accepted=True):
    return run(
        [
            args.optimizer,
            path,
            "--ac-extract-source-interface",
            "--mlir-print-debuginfo",
            "-o",
            output,
        ],
        accepted,
    )


child, parent = root / "child", root / "parent"
compile_unit("source_interface_child.py", child)
compile_unit("source_interface_parent.py", parent, [child])
for unit in (child, parent):
    extracted = root / (unit.name + "-extracted.ac")
    extract(payload(unit, "body"), extracted)
    actual = normalize(extracted, root / (unit.name + "-actual.ac"))
    expected = normalize(
        payload(unit, "interface"), root / (unit.name + "-expected.ac")
    )
    assert actual == expected, (actual, expected)
    assert '"ac.module"' not in actual and '"ac.system"' not in actual
    assert 'ac.unit_kind = "interface"' in actual

if args.query_checks:
    sys.path.insert(0, str(repo / "python"))
    sys.path.insert(0, str(fixtures / "table-queries"))
    from pycircuit._source_capture import _capture_source_file
    from pycircuit._source_transport import _emit_source_transport
    from table_query_admission import (
        CONTROL,
        admission_cases,
        resource_cases,
        structure_cases,
        syntax_cases,
    )

    receipts = []
    archive = root / ("query-" + args.query_checks)
    archive.mkdir(exist_ok=True)
    if args.query_checks == "syntax":
        positives, negatives = syntax_cases()
        # Capture transports ordinary lambda parameter lists independently of a
        # typed receiver. first/argmin/map enforce their arity during compile.
        for name in ("zero-args", "two-args"):
            positives[name] = negatives.pop(name)
        positives["multi-input-map"] = positives["ordinary"].replace(
            "entries.first(where=lambda row: Write(state))",
            "entries.map(lambda lane, other: lane, entries)",
        )
        for accepted, cases in ((True, positives), (False, negatives)):
            for name, text in cases.items():
                filename = "query_" + name.replace("-", "_") + ".py"
                path = source / filename
                path.write_text(text)
                capture = archive / (name + ".capture.mlir")
                capture.write_text(
                    _emit_source_transport(
                        _capture_source_file(path, source_root=source)
                    )
                )
                before = capture.read_bytes()
                output = archive / (name + ".analyzed.mlir")
                result = run(
                    [args.optimizer, capture, "--ac-analyze-rule-writes", "-o", output],
                    accepted,
                )
                assert capture.read_bytes() == before
                if accepted:
                    assert (
                        "overlapping writer pairs" not in result.stderr
                    ), result.stderr
                else:
                    assert (
                        "Lambda" in result.stderr or "captured" in result.stderr
                    ), result.stderr
                    assert not output.exists()
                receipts.append(
                    {
                        "case": name,
                        "accepted": accepted,
                        "capture_sha256": hashlib.sha256(before).hexdigest(),
                    }
                )
    else:
        path = source / "query_control.py"
        path.write_text(CONTROL)
        unit = archive / "control-unit"
        compile_unit(path.name, unit)
        final = archive / "control.ac"
        cli("link", unit, "--top", "interfaces.query_control.Top", "-o", final)
        protected_outputs = {}
        for target in ("cpp", "verilog"):
            output = archive / target
            cli("emit", final, "--target", target, "-o", output)
            protected_outputs[target] = snapshot(output)
        before_unit, before_final = snapshot(unit), final.read_bytes()
        cases = (
            admission_cases() if args.query_checks == "admission" else resource_cases()
        )
        if args.query_checks == "admission":
            original = "entries.first(where=lambda row: row.valid)"
            for label, predicate, key in (
                ("zero", "lambda: True", "lambda: 0"),
                ("two", "lambda row, other: row.valid", "lambda row, other: row.key"),
                ("default", "lambda row=0: row.valid", "lambda row=0: row.key"),
                ("varargs", "lambda *row: True", "lambda *row: 0"),
            ):
                cases["typed-first-" + label] = CONTROL.replace(
                    original, f"entries.first(where={predicate})"
                )
                cases["typed-argmin-where-" + label] = CONTROL.replace(
                    original,
                    f"entries.argmin(where={predicate}, key=lambda row: row.key)",
                )
                cases["typed-argmin-key-" + label] = CONTROL.replace(
                    original, f"entries.argmin(where=lambda row: row.valid, key={key})"
                )
            map_control = CONTROL.replace(
                "class Result:\n    index: ac.u3\n    valid: ac.u1",
                "class Result:\n    value: ac.table[3, Entry]",
            ).replace(
                "    index, valid = "
                + original
                + "\n    return Result(index=index, valid=valid)",
                "    return Result(value=entries.map(lambda row: row))",
            )
            for label, call in (
                ("zero", "entries.map(lambda: 0)"),
                ("two", "entries.map(lambda row, other: row)"),
                ("missing-zip-parameter", "entries.map(lambda row: row, entries)"),
            ):
                cases["typed-map-" + label] = map_control.replace(
                    "entries.map(lambda row: row)", call
                )
        for name, text in cases.items():
            path.write_text(text)
            (archive / (name + ".py")).write_text(text)
            absent = archive / ("invalid-" + name)
            result = compile_unit(path.name, absent, accepted=False)
            assert not absent.exists()
            if args.query_checks == "resources":
                assert "budget" in result.stderr.lower(), result.stderr
            if name.startswith("typed-"):
                assert (
                    "Lambda" in result.stderr or "captured" in result.stderr
                ), result.stderr
            compile_unit(path.name, unit, replace=True, accepted=False)
            assert snapshot(unit) == before_unit and final.read_bytes() == before_final
            assert all(
                snapshot(archive / target) == expected
                for target, expected in protected_outputs.items()
            )
            receipts.append(
                {
                    "case": name,
                    "fresh_exit_status": 1,
                    "replacement_exit_status": 1,
                    "publication_unchanged": True,
                    "source_sha256": hashlib.sha256(text.encode()).hexdigest(),
                }
            )
        if args.query_checks == "resources":
            for name, text in structure_cases().items():
                path.write_text(text)
                output = archive / ("structure-" + name)
                compile_unit(path.name, output)
                body = payload(output, "body").read_text()
                locations = {
                    alias: (int(line), int(column))
                    for alias, line, column in re.findall(
                        r'(#loc\d+) = loc\("[^\"]+":(\d+):(\d+)\)', body
                    )
                }
                callbacks = [
                    node
                    for node in ast.walk(ast.parse(text))
                    if isinstance(node, ast.Lambda)
                ]
                witness = []
                for callback in callbacks:
                    for attribute in (
                        node
                        for node in ast.walk(callback.body)
                        if isinstance(node, ast.Attribute)
                    ):
                        site = attribute.lineno, attribute.col_offset + 1
                        actual = sum(
                            "ac.struct.get" in line
                            and any(
                                f"loc({alias})" in line
                                for alias, location in locations.items()
                                if location == site
                            )
                            for line in body.splitlines()
                        )
                        assert actual == 1, (
                            name,
                            site,
                            actual,
                            "callback projection was cloned or lost",
                        )
                        witness.append(
                            {
                                "line": site[0],
                                "column": site[1],
                                "logical_copies": actual,
                            }
                        )
                receipts.append(
                    {
                        "case": name,
                        "structure": "one actual callback projection per source expression",
                        "callback_witnesses": witness,
                        "body_sha256": hashlib.sha256(body.encode()).hexdigest(),
                    }
                )
    (archive / "receipt.json").write_text(
        json.dumps(
            {
                "scope": args.query_checks,
                "cases": receipts,
                "source_profile": (
                    "AST/scope only"
                    if args.query_checks == "syntax"
                    else "public compile/link/emit protected query publication"
                ),
                "tools": {
                    str(Path(tool).resolve()): hashlib.sha256(
                        Path(tool).read_bytes()
                    ).hexdigest()
                    for tool in (
                        args.source_compiler,
                        args.linker,
                        args.emitter,
                        args.optimizer,
                    )
                },
            },
            indent=2,
        )
        + "\n"
    )
    print(
        "Table query " + args.query_checks + " checks passed"
    )  # noqa: T201 - standalone gate evidence
    sys.exit(0)

if args.invalid:
    body = payload(parent, "body").read_text()
    malformed = root / "malformed.ac"
    mutations = {
        "stage": body.replace('ac.stage = "source"', 'ac.stage = "final"', 1),
        "kind": body.replace(
            'ac.unit_kind = "implementation"', 'ac.unit_kind = "interface"', 1
        ),
        "missing_exports": body.replace("ac.exports =", "ac.other_exports =", 1),
    }
    for name, text in mutations.items():
        assert text != body, name
        malformed.write_text(text)
        absent = root / ("invalid-" + name + ".ac")
        extract(malformed, absent, accepted=False)
        assert not absent.exists(), name
    # A valid final design and both managed emitted bundles are protected when
    # a later source/interface pass rejects malformed replacement source.
    final = root / "parent.ac"
    cli(
        "link",
        child,
        parent,
        "--top",
        "interfaces.source_interface_parent.Parent",
        "-o",
        final,
    )
    outputs = {}
    for target in ("cpp", "verilog"):
        directory = root / target
        cli("emit", final, "--target", target, "-o", directory)
        outputs[target] = snapshot(directory)
    before_unit, before_final = snapshot(parent), final.read_bytes()
    bad = source / "source_interface_parent.py"
    # The passthrough child feeds its own x from y. Capture/import can form
    # valid graph SSA; actual HardwareAnalysis must reject this combination
    # cycle in the registered source-interface extraction pass.
    bad.write_text(bad.read_text().replace("child(x=x)", "child(x=child.y)"))
    fresh = compile_unit(
        "source_interface_parent.py", root / "absent-unit", [child], accepted=False
    )
    assert any(
        word in fresh.stderr.lower() for word in ("cycle", "cyclic")
    ), fresh.stderr
    assert not (root / "absent-unit").exists()
    replaced = compile_unit(
        "source_interface_parent.py", parent, [child], replace=True, accepted=False
    )
    assert any(
        word in replaced.stderr.lower() for word in ("cycle", "cyclic")
    ), replaced.stderr
    assert snapshot(parent) == before_unit and final.read_bytes() == before_final
    for target in outputs:
        assert snapshot(root / target) == outputs[target]
