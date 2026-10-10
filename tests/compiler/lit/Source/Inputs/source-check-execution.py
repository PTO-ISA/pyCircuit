"""Native development gates on real source-linked/common-IR generated DUTs."""

import argparse
import ast
import hashlib
import importlib.util
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def decode(value):
    return "" if value == "-" else bytes.fromhex(value).decode()


def inverse(value):
    return "1" if value == "0" else "0" if value == "1" else "x"


def conjunction(a, b):
    return "0" if "0" in (a, b) else "1" if a == b == "1" else "x"


def main():
    parser = argparse.ArgumentParser()
    for name in ("repo", "source-compiler", "linker", "cxx", "scratch"):
        parser.add_argument("--" + name, required=True)
    parser.add_argument("--phase", choices=("prepare", "verify"), required=True)
    args = parser.parse_args()
    repo, evidence = Path(args.repo).resolve(), Path(args.scratch).resolve()
    evidence.mkdir(parents=True, exist_ok=True)
    fixtures = Path(__file__).resolve().parent / "source_check_execution"
    prepared_path = evidence / "prepared.json"
    if args.phase == "prepare":
        assert (
            not prepared_path.exists()
        ), "preparation cannot overwrite an existing receipt"
        root = Path(tempfile.mkdtemp(prefix="native-checks-", dir=evidence))
        source, units = root / "source", root / "units"
        source.mkdir()
        units.mkdir()
        for path in fixtures.glob("*.py"):
            shutil.copyfile(path, source / path.name)
    else:
        prepared = json.loads(prepared_path.read_text())
        root = Path(prepared["artifact_directory"])
        source, units = root / "source", root / "units"
    env = dict(
        os.environ,
        PYTHONPATH=str(repo / "python"),
        PYTHONDONTWRITEBYTECODE="1",
        PYCIRCUIT_SOURCE_COMPILER=args.source_compiler,
        PYCIRCUIT_LINKER=args.linker,
    )
    commands_path = evidence / "commands.json"
    commands = json.loads(commands_path.read_text()) if args.phase == "verify" else []
    products, completion_variants = [], []
    spec = importlib.util.spec_from_file_location(
        "source_check_oracles", Path(__file__).with_name("source-checks.py")
    )
    oracles = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(oracles)
    runtime_root = Path(args.source_compiler).resolve().parent.parent
    runtime = next(
        path
        for path in (
            runtime_root / "runtime/libpyc6_runtime.a",
            runtime_root / "lib/libpyc6_runtime.a",
        )
        if path.is_file()
    )

    def run(command):
        command = list(map(str, command))
        result = subprocess.run(
            command, cwd=repo, env=env, text=True, capture_output=True, timeout=90
        )
        row = {
            "command": command,
            "exit_status": result.returncode,
            "stdout": result.stdout,
            "stderr": result.stderr,
        }
        commands.append(row)
        (evidence / "commands.json").write_text(json.dumps(commands, indent=2) + "\n")
        assert result.returncode == 0, row
        assert (
            "Traceback" not in result.stderr and "Assertion failed" not in result.stderr
        ), row
        return result

    def cli(*arguments):
        return run([sys.executable, "-m", "pycircuit.cli", *arguments])

    def compile_unit(name, imports=()):
        unit = units / name
        command = [
            "compile",
            "-c",
            source / (name + ".py"),
            "--source-root",
            source,
            "--package-prefix",
            "execution",
            "-o",
            unit,
        ]
        for imported in imports:
            command.extend(("-I", imported))
        cli(*command)
        return unit

    def occurrence(definition, path):
        parts = [
            (
                {"kind": "index", "value": component}
                if isinstance(component, int)
                else {"kind": "field", "name": component}
            )
            for component in path
        ]
        return {"site": {"definition": definition, "ast_path": parts}, "expansion": []}

    def source_metadata(name, module_name):
        # Expected registration/check/spans come from Python AST declarations,
        # independently of the helper's analysis manifest and generated strings.
        path = fixtures / (name + ".py")
        syntax = ast.parse(path.read_text())
        nodes = list(oracles.ast_paths(syntax))
        functions = {
            node.name: node for node, _ in nodes if isinstance(node, ast.FunctionDef)
        }
        owner = functions[module_name]
        expected = []
        for call, registration in nodes:
            if not isinstance(call, ast.Call) or not isinstance(call.func, ast.Name):
                continue
            if call not in list(ast.walk(owner)) or call.func.id not in functions:
                continue
            rule = functions[call.func.id]
            if not any(
                (isinstance(d, ast.Name) and d.id == "rule")
                or (isinstance(d, ast.Attribute) and d.attr == "rule")
                for d in rule.decorator_list
            ):
                continue
            symbol = "execution." + name + "." + module_name
            for check, site in nodes:
                if not isinstance(check, ast.Assert) or check not in list(
                    ast.walk(rule)
                ):
                    continue
                location = {
                    "path": path.name,
                    "line": check.lineno,
                    "column": check.col_offset + 1,
                    "end_line": check.end_lineno,
                    "end_column": check.end_col_offset + 1,
                }
                identity = {
                    "registration": occurrence(symbol, registration),
                    "check": occurrence(symbol, site),
                    "obligation": 0,
                }
                expected.append(
                    {
                        "id": identity,
                        "location": location,
                        "message": (
                            check.msg.value
                            if check.msg
                            else "source assert check failed"
                        ),
                    }
                )
        assert expected, name
        return expected

    def fixed_metadata(definition, filename, obligation=0, expansion=None):
        registration = occurrence(definition, (7,))
        check = occurrence(definition, (13,))
        if expansion:
            check["expansion"] = expansion
        return {
            "id": {
                "registration": registration,
                "check": check,
                "obligation": obligation,
            },
            "location": {
                "path": filename,
                "line": 1,
                "column": 1,
                "end_line": 1,
                "end_column": 2,
            },
        }

    if args.phase == "prepare":
        source_cases = []
        for name in ("matrix", "snapshots"):
            unit = compile_unit(name)
            final = root / (name + ".ac")
            cli("link", unit, "--top", "execution." + name + ".Top", "-o", final)
            source_cases.append((name, final, [source_metadata(name, "Top")]))
        guard = compile_unit("guard")
        cell = compile_unit("cell", [guard])
        # Parent consumes authoritative interfaces after both provider sources vanish.
        (source / "guard.py").unlink()
        (source / "cell.py").unlink()
        hierarchy = compile_unit("hierarchy", [guard, cell])
        final = root / "hierarchy.ac"
        cli(
            "link",
            guard,
            cell,
            hierarchy,
            "--top",
            "execution.hierarchy.Top",
            "-o",
            final,
        )
        source_cases.append(
            (
                "hierarchy",
                final,
                [source_metadata("cell", "Cell"), source_metadata("guard", "Guard")],
            )
        )

        frames = [
            {
                "kind": "iteration",
                "site": occurrence("execution.collections.Ordinary", (19,))["site"],
                "ordinal": 4,
                "value": {
                    "kind": "integer",
                    "value": "-170141183460469231731687303715884105733",
                },
            }
        ]
        family = fixed_metadata("execution.collections.Child", "collections.py")
        family["message"] = "family"
        ordinary = fixed_metadata(
            "execution.collections.Ordinary", "collections.py", 3, frames
        )
        ordinary["message"] = 'quote" backslash\\ newline\n nul\0 tail'
        ordinary11 = fixed_metadata(
            "execution.collections.Ordinary", "collections.py", 11, frames
        )
        ordinary11["message"] = "ordinary 11"
        leaf_metadata = {
            "id": {
                "registration": occurrence("execution.leaves.Top", (40,)),
                "check": occurrence("execution.leaves.Top", (41,)),
                "obligation": 0,
            },
            "location": {
                "path": "leaves.py",
                "line": 1,
                "column": 1,
                "end_line": 1,
                "end_column": 2,
            },
            "message": "all-leaf barrier",
        }
        cases = [
            *source_cases,
            (
                "collections",
                fixtures / "collections.mlir",
                [[family, ordinary, ordinary11]],
            ),
            ("leaves", fixtures / "leaves.mlir", [[leaf_metadata]]),
            (
                "domains",
                fixtures / "domains.mlir",
                [
                    [
                        dict(
                            fixed_metadata("execution.domains.Cell", "domains.py"),
                            message="cell",
                        ),
                        dict(
                            fixed_metadata("execution.domains.Pure", "domains.py"),
                            message="grandchild",
                        ),
                    ]
                ],
            ),
        ]

        address = compile_unit("address_grants")
        address_final = root / "address_grants.ac"
        cli(
            "link",
            address,
            "--top",
            "execution.address_grants.Top",
            "-o",
            address_final,
        )
        cases.append(
            (
                "address_grants",
                address_final,
                [source_metadata("address_grants", "Top")],
            )
        )

        prepared_path.write_text(
            json.dumps(
                {
                    "scope": "native development; GTest owns in-process prepared emission",
                    "artifact_directory": str(root),
                    "cases": [
                        {"case": name, "final": str(final), "expected_groups": groups}
                        for name, final, groups in cases
                    ],
                },
                indent=2,
            )
            + "\n"
        )
        print(
            "PREPARED: seven source-linked/common-IR cases; no DUT emitted"
        )  # noqa: T201 - standalone gate receipt
        return
    cases = [
        (entry["case"], Path(entry["final"]), entry["expected_groups"])
        for entry in prepared["cases"]
    ]
    assert [name for name, _, _ in cases] == [
        "matrix",
        "snapshots",
        "hierarchy",
        "collections",
        "leaves",
        "domains",
        "address_grants",
    ]

    def parse_rows(output):
        rows = {}
        for line in output.splitlines():
            if not line.startswith("ROW "):
                continue
            parts = line.split()
            assert len(parts) == 12, line
            label = parts[1]
            assert label not in rows, label
            rows[label] = {
                "status": int(parts[2]),
                "epoch": int(parts[3]),
                "phase": int(parts[4]),
                "code": decode(parts[5]),
                "message": decode(parts[6]),
                "instance": decode(parts[7]),
                "source": json.loads(decode(parts[8])) if parts[8] != "-" else None,
                "id": json.loads(decode(parts[9])) if parts[9] != "-" else None,
                "available": bool(int(parts[10])),
                "output": parts[11],
            }
        return rows

    def success(row, epoch=None, output=None):
        assert (
            row["status"] == 1
            and row["phase"] == 0
            and row["code"] == ""
            and row["available"]
        ), row
        if epoch is not None:
            assert row["epoch"] == epoch, row
        if output is not None:
            assert row["output"] == output, row

    def failure(row, metadata, instance, epoch=0):
        assert row["status"] == 3 and row["phase"] == 3 and not row["available"], row
        assert row["epoch"] == epoch and row["code"] == "source_check_failed", row
        assert (
            row["message"] == metadata["message"] and row["instance"] == instance
        ), row
        assert (
            row["source"] == metadata["location"] and row["id"] == metadata["id"]
        ), row

    for case_index, (name, final, expected_groups) in enumerate(cases):
        hierarchy_names = []
        if name == "hierarchy":
            syntax = ast.parse((fixtures / "hierarchy.py").read_text())
            sites = [
                path
                for node, path in oracles.ast_paths(syntax)
                if isinstance(node, ast.Call)
                and isinstance(node.func, ast.Name)
                and node.func.id == "Cell"
            ]
            root_ir = next(
                module
                for module in oracles.parse_ir(final.read_text())
                if 'sym_name = "execution.hierarchy.Top"' in module["text"]
            )
            for site in sites:
                allocations = [
                    line
                    for op, _, line in root_ir["ops"].values()
                    if op == "ac.instance"
                    and oracles.occurrence(oracles.attribute(line, "occurrence"))
                    == ("execution.hierarchy.Top", site)
                ]
                assert len(allocations) == 1, (site, allocations)
                hierarchy_names.append(
                    re.search(r'instance_name = "([^"]+)"', allocations[0])[1]
                )
        generated = root / ("generated-" + name)
        emission = json.loads((generated / "native-emission.json").read_text())
        assert emission == {
            "case": name,
            "route": "in-process GTest",
            "status": "success",
        }, emission
        manifest = json.loads((generated / "native-checks.json").read_text())
        expected = [entry for group in expected_groups for entry in group]
        for check in manifest["checks"]:
            assert any(
                check["id"] == entry["id"] and check["location"] == entry["location"]
                for entry in expected
            ), check
        if name == "matrix":
            assert (
                len(manifest["checks"]) == 1
                and not manifest["checks"][0]["reset"]["physical"]
            )
        elif name == "snapshots":
            assert len(manifest["checks"]) == 3
            assert all(
                c["reset"] == {"physical": True, "owner": 0, "input": 4}
                for c in manifest["checks"]
            )
        elif name == "hierarchy":
            assert len(manifest["checks"]) == 4
            assert all(c["reset"]["physical"] for c in manifest["checks"])
            physical_owners = {c["reset"]["owner"] for c in manifest["checks"]}
            assert len(physical_owners) == 2
        elif name == "collections":
            family, ordinary, ordinary11 = expected
            assert len(manifest["owners"]) == 8 and len(manifest["checks"]) == 8
            for lane in range(6):
                owner = manifest["owners"][lane + 1]
                assert owner["path"][0]["coordinates"] == [lane // 3, lane % 3]
                assert manifest["checks"][lane]["reset"] == {
                    "physical": True,
                    "owner": lane + 1,
                    "input": 2,
                }
            assert [c["id"]["obligation"] for c in manifest["checks"][-2:]] == [3, 11]
            assert all(not c["reset"]["physical"] for c in manifest["checks"][-2:])
        elif name == "address_grants":
            assert len(manifest["checks"]) == 2
            assert all(
                c["reset"] == {"physical": True, "owner": 0, "input": 8}
                for c in manifest["checks"]
            )
            assert len(manifest["commits"]) >= 3 and all(
                c["kind"] == "dffe" for c in manifest["commits"]
            )
        elif name == "domains":
            assert len(manifest["checks"]) == 4
            assert sorted(c["kind"] for c in manifest["commits"]) == ["dffe", "dffe"]
            assert len({c["reset"]["owner"] for c in manifest["checks"]}) == 2
            assert all(
                c["reset"]["physical"] and c["reset"]["input"] == 3
                for c in manifest["checks"]
            )
        else:
            assert sorted(c["kind"] for c in manifest["commits"]) == [
                "byte_mem",
                "dff",
                "dffe",
                "fifo",
                "fifo",
                "sync_mem",
                "sync_mem_dp",
            ]
        cpp = [generated / path for path in manifest["files"] if path.endswith(".cpp")]
        runner = root / ("runner-" + name)
        run(
            [
                args.cxx,
                "-std=c++20",
                "-O0",
                "-g",
                "-pthread",
                "-DT3_CASE=" + str(case_index),
                "-I" + str(repo / "include"),
                "-I" + str(generated),
                Path(__file__).with_suffix(".cpp"),
                *cpp,
                runtime,
                "-o",
                runner,
            ]
        )
        traces = []
        for workers in (1, 2):
            output = run([runner, workers]).stdout
            (root / (name + f"-workers-{workers}.stdout")).write_text(output)
            traces.append(output)
            rows = parse_rows(output)
            if name == "matrix":
                for p in "01xz":
                    for c in "01xz":
                        label = "matrix-" + p + c
                        row = rows[label]
                        if conjunction(p, inverse(c)) == "0":
                            success(row, 1, c)
                        else:
                            failure(row, expected[0], "root")
                            failure(rows[label + "-retry"], expected[0], "root")
                            success(rows[label + "-reset"], 1, "1")
            elif name == "snapshots":
                for label, epoch, value in (
                    ("state-low", 1, 3),
                    ("state-rise", 2, 3),
                    ("state-held", 3, 9),
                    ("state-fall", 4, 9),
                    ("state-host-reset", 1, 3),
                ):
                    success(rows[label], epoch, format(value, "08b"))
                messages = {item["message"]: item for item in expected}
                failure(rows["state-fail-between"], messages["between"], "root", 4)
                failure(
                    rows["state-physical-reset-retry"], messages["between"], "root", 4
                )
                failure(rows["state-fail-after-held"], messages["after"], "root", 1)
                failure(rows["state-fail-allow-falling"], messages["allow"], "root")
                primitive = rows["state-primitive-precedence"]
                assert (
                    primitive["phase"] == 2
                    and primitive["code"] == "runtime_failure"
                    and primitive["id"] is None
                ), primitive
                assert "DIRECT state-discard-clock-rollback" in output
            elif name == "hierarchy":
                success(rows["hierarchy-0"], 1, "00")
                success(rows["hierarchy-1"], 1, "00")
                failure(
                    rows["hierarchy-2"],
                    expected_groups[0][0],
                    "root." + hierarchy_names[1],
                )
                failure(
                    rows["hierarchy-3"],
                    expected_groups[0][0],
                    "root." + hierarchy_names[0],
                )
            elif name == "collections":
                failure(rows["collection-0"], family, "root.cells[1][2]")
                failure(rows["collection-1"], ordinary, "root.tail")
                success(rows["collection-2"], 1, "1")
                for c in "01xz":
                    for r in "01xz":
                        row = rows["collection-matrix-" + c + r]
                        if conjunction(inverse(r), inverse(c)) == "0":
                            success(row, 1, "1")
                        else:
                            failure(row, family, "root.cells[1][2]")
            elif name == "address_grants":

                def address_bits(*values):
                    return "".join(format(value, "08b") for value in values)

                for label, epoch, value in (
                    ("address-low", 1, address_bits(0, 0, 0, 0, 0, 0, 17, 34)),
                    ("address-rise", 2, address_bits(0, 0, 0, 0, 0, 0, 17, 34)),
                    (
                        "address-observe",
                        3,
                        address_bits(17, 238, 34, 221, 17, 34, 51, 68),
                    ),
                    ("address-host-reset", 1, address_bits(0, 0, 0, 0, 0, 0, 51, 68)),
                    (
                        "address-observe-reset",
                        2,
                        address_bits(51, 204, 68, 187, 51, 68, 51, 68),
                    ),
                ):
                    success(rows[label], epoch, value)
                failure(rows["address-failure"], expected[0], "root", 3)
                failure(rows["address-terminal"], expected[0], "root", 3)
                assert (
                    "DIRECT address-domains-owner-check-suffix-discard-clock-rollback"
                    in output
                )
            elif name == "domains":
                for label, epoch, value in (
                    ("domains-low", 1, "00"),
                    ("domains-only-b", 2, "00"),
                    ("domains-observe-b", 3, "01"),
                    ("domains-rise-a", 4, "01"),
                    ("domains-observe-both", 5, "11"),
                    ("domains-host-reset", 1, "00"),
                ):
                    success(rows[label], epoch, value)
                failure(
                    rows["domains-reset-candidate-failure"], expected[0], "root.aaa", 5
                )
                failure(rows["domains-terminal-reset"], expected[0], "root.aaa", 5)
                assert (
                    "DIRECT distinct-domains-reset-candidate-and-clock-rollback"
                    in output
                )
            else:
                assert "DIRECT all-seven-leaves-barrier-reset-and-delayed-age" in output
        if name == "matrix":
            config = root / "shared-runner-config.json"
            config.write_text(
                json.dumps(
                    {
                        "schema": "pycircuit-model-config",
                        "version": "1",
                        "max_ticks": 8,
                        "max_domain_cycles": {},
                        "deadlock_window": None,
                    },
                    sort_keys=True,
                    separators=(",", ":"),
                )
                + "\n"
            )
            integration_traces = []
            for workers in (1, 2):
                events = root / (f"shared-runner-workers-{workers}.jsonl")
                output = run([runner, workers, config, events]).stdout
                integration_traces.append(output)
                rows = parse_rows(output)
                failure(rows["runner-failure"], expected[0], "root", 1)
                failure(rows["runner-retry"], expected[0], "root", 1)
                assert "RUNNER initialized=1 drives=2 samples=1 bounded=8" in output
                records = [json.loads(line) for line in events.read_text().splitlines()]
                assert len(records) == 1, records
                result = records[0]
                assert (
                    result["kind"] == "result"
                    and result["status"] == "FAILED"
                    and result["epoch_time"] == "1"
                ), result
                error = result["error"]
                assert (
                    error["code"] == "source_check_failed" and error["phase"] == "check"
                ), error
                assert (
                    error["message"] == expected[0]["message"]
                    and error["instance"] == "root"
                ), error
                assert (
                    error["source"] == expected[0]["location"]
                    and error["check_id"] == expected[0]["id"]
                ), error
            assert (
                integration_traces[0] == integration_traces[1]
            ), "shared runner worker-dependent diagnostics"
        assert traces[0] == traces[1], (
            name,
            "worker-dependent diagnostics or behavior",
        )
        if name in {"snapshots", "domains"}:
            # Preserve the normal seven DUTs and run this narrow access-only copy
            # separately. The existing family capture methods are unchanged.
            before = {path: digest(generated / path) for path in manifest["files"]}
            variant = root / ("completion-access-" + name)
            shutil.copytree(generated, variant)
            header_path = "sources/execution/" + name + ".hpp"
            header = variant / header_path
            pristine = header.read_bytes()
            count = len(manifest["checks"])
            private_block = (
                "private:\n  std::array<std::string, "
                + str(count)
                + "> pyc_check_instances_;\n  std::shared_ptr<pyc_family_Top<1>> pyc_implementation;"
            )
            text = pristine.decode()
            assert (
                text.count(private_block) == 1
            ), "root scalar access block must be unique"
            public_block = private_block.replace("private:", "public:", 1)
            access = text.replace(private_block, public_block, 1).encode()
            assert (
                access != pristine
                and access.replace(public_block.encode(), private_block.encode(), 1)
                == pristine
            )
            header.write_bytes(access)
            assert all(
                digest(variant / path) == old
                for path, old in before.items()
                if path != header_path
            )
            completion_runner = root / ("completion-runner-" + name)
            variant_cpp = [
                variant / path for path in manifest["files"] if path.endswith(".cpp")
            ]
            run(
                [
                    args.cxx,
                    "-std=c++20",
                    "-O0",
                    "-g",
                    "-pthread",
                    "-DT3_COMPLETION=" + str(case_index),
                    "-I" + str(repo / "include"),
                    "-I" + str(variant),
                    Path(__file__).with_name("source-check-completion.cpp"),
                    *variant_cpp,
                    runtime,
                    "-o",
                    completion_runner,
                ]
            )
            variant_traces = []
            for workers in (1, 2):
                output = run([completion_runner, workers]).stdout
                variant_traces.append(output)
                actual = {}
                for line in output.splitlines():
                    fields = line.split()
                    assert len(fields) == 9 and fields[0] == "COMPLETION", line
                    label = fields[1]
                    assert label not in actual
                    actual[label] = {
                        "accepted": bool(int(fields[2])),
                        "phase": int(fields[3]),
                        "code": decode(fields[4]),
                        "message": decode(fields[5]),
                        "instance": decode(fields[6]),
                        "source": (
                            json.loads(decode(fields[7])) if fields[7] != "-" else None
                        ),
                        "id": (
                            json.loads(decode(fields[8])) if fields[8] != "-" else None
                        ),
                    }
                runtime_labels = (
                    [
                        "snapshots-later-check-missing",
                        "snapshots-earlier-false-later-missing",
                        "snapshots-reset-only-missing",
                    ]
                    if name == "snapshots"
                    else [
                        "domains-earlier-false-later-reset-missing",
                        "domains-grandchild-authoritative-reset-missing",
                    ]
                )
                for label in runtime_labels:
                    row = actual[label]
                    assert row == {
                        "accepted": False,
                        "phase": 3,
                        "code": "runtime_failure",
                        "message": "model check failed",
                        "instance": "",
                        "source": None,
                        "id": None,
                    }, row
                source_labels = (
                    {"snapshots-restored-check": (expected[0], "root")}
                    if name == "snapshots"
                    else {
                        "domains-restored-later-reset": (expected[0], "root.zzz"),
                        "domains-restored-grandchild-reset": (
                            expected[1],
                            "root.aaa.pure",
                        ),
                    }
                )
                for label, (metadata, instance) in source_labels.items():
                    row = actual[label]
                    assert row == {
                        "accepted": False,
                        "phase": 3,
                        "code": "source_check_failed",
                        "message": metadata["message"],
                        "instance": instance,
                        "source": metadata["location"],
                        "id": metadata["id"],
                    }, row
                if name == "snapshots":
                    assert actual["snapshots-restored-reset"] == {
                        "accepted": True,
                        "phase": 0,
                        "code": "",
                        "message": "",
                        "instance": "",
                        "source": None,
                        "id": None,
                    }
                assert len(actual) == (5 if name == "snapshots" else 4)
            assert (
                variant_traces[0] == variant_traces[1]
            ), "completion classification differs by workers"
            assert {
                path: digest(generated / path) for path in manifest["files"]
            } == before, "pristine generated DUT drift"
            completion_variants.append(
                {
                    "case": name,
                    "access_only": True,
                    "changed_path": header_path,
                    "old_access": "private:",
                    "new_access": "public:",
                    "pristine_sha256": hashlib.sha256(pristine).hexdigest(),
                    "variant_sha256": hashlib.sha256(access).hexdigest(),
                    "all_other_generated_files_unchanged": True,
                    "workers": [1, 2],
                    "runner_sha256": digest(completion_runner),
                    "output": variant_traces[0],
                }
            )
        products.append(
            {
                "case": name,
                "input_sha256": digest(final),
                "runner_sha256": digest(runner),
                "route": (
                    "public source compile/link + prepared native emitter"
                    if name in {"matrix", "snapshots", "hierarchy", "address_grants"}
                    else "declared common final IR + same prepared native emitter"
                ),
                "workers": [1, 2],
                "native_emission": emission,
                "generated": {
                    path: digest(generated / path) for path in manifest["files"]
                },
            }
        )
    report = {
        "scope": "native development only; public checked emission and RTL acceptance remain open",
        "artifact_directory": str(root),
        "products": products,
        "completion_access_variants": completion_variants,
        "shared_SystemRunner": {
            "workers": [1, 2],
            "max_ticks": 8,
            "drives": 2,
            "successful_samples": 1,
            "failure_epoch": 1,
            "terminal_retry": True,
            "exact_error_result_record": True,
        },
        "fixtures": {
            path.name: digest(path) for path in fixtures.iterdir() if path.is_file()
        },
        "remaining": [
            "genuine RTL/synthesis safety and wrapper lifecycle",
            "live external frame mutation",
            "RTL deep zero-delay settlement",
            "native FIFO full replacement and masked multi-byte tails beyond this barrier slice",
        ],
    }
    (evidence / "results.json").write_text(json.dumps(report, indent=2) + "\n")
    print(
        "PASS: actual native checked DUTs, workers1/2, complete metadata, reset/discard and seven-leaf barrier; RTL remains open"
    )  # noqa: T201 - standalone gate receipt


if __name__ == "__main__":
    main()
