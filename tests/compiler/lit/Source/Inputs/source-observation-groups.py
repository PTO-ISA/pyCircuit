"""Independent typed payload and instance oracles for generated observations."""

import argparse
import hashlib
import json
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path


def value(item):
    return (
        {"kind": "bool", "value": item}
        if isinstance(item, bool)
        else {"kind": "integer", "value": str(item)}
    )


def items(*sequence):
    result, ordinal = [], 0
    for item in sequence:
        if isinstance(item, str):
            result.append({"kind": "literal", "text": item})
        else:
            result.append({"kind": "value", "ordinal": ordinal})
            ordinal += 1
    return result


def log_spec(event, *sequence):
    return {"event": event, "items": items(*sequence), "level": "info"}


# Literal arrays are independent of compiler results and hardware source state.
ROWS = (
    (0, (False, 17, 51, 93, 119), 119, (0, 17, True), (0, 51, False)),
    (1, (True, 18, 52, 93, 119), 119, (17, 18, False), (51, 52, False)),
    (2, (False, 19, 53, 93, 119), 119, (18, 19, False), (52, 53, False)),
)
ARITHMETIC_ROWS = (
    (17, 17, 99, 0, False, 93, 119),
    (18, 18, 98, 2, False, 93, 119),
    (116, 116, 0, 198, True, 93, 119),
    (117, 117, 255, 200, False, 93, 119),
    (0, 0, 116, 222, False, 93, 119),
    (1, 1, 115, 224, False, 93, 119),
    (16, 16, 100, 254, False, 93, 119),
)


def expected_epoch(epoch):
    phase, mixed, after, left, right = ROWS[epoch // 2]
    return [
        ("root", "log", log_spec("before", 0), [value(phase)]),
        (
            "root",
            "log",
            log_spec("mixed", "begin", False, 0, "middle", 0, 0, 0, "end"),
            [value(item) for item in mixed],
        ),
        ("root", "log", log_spec("literal", "words"), []),
        ("root", "log", log_spec("after", 0), [value(after)]),
        ("root", "report", {"name": "ticks"}, [value(phase)]),
        (
            "root/left",
            "log",
            log_spec("leaf", "old", 0, "next", 0, False),
            [value(item) for item in left],
        ),
        (
            "root/right",
            "log",
            log_spec("leaf", "old", 0, "next", 0, False),
            [value(item) for item in right],
        ),
    ]


def observations(stdout, epochs):
    records = [json.loads(line) for line in stdout.splitlines() if line.startswith("{")]
    events = [record for record in records if record["kind"] != "result"]
    assert len(events) == epochs * 7, events
    for epoch in range(epochs):
        actual = events[epoch * 7 : (epoch + 1) * 7]
        assert [
            (record["instance"], record["kind"], record["spec"], record["values"])
            for record in actual
        ] == expected_epoch(epoch), (epoch, actual)
        for record in actual:
            assert int(record["evaluation_epoch"]) == epoch
            assert int(record["commit_epoch"]) == epoch + 1
            for field in ("registration", "site"):
                identity = json.loads(record[field])
                assert identity["site"]["definition"].startswith(
                    "observation_groups.source_observation_groups."
                ), identity
                assert identity["site"]["ast_path"], identity
                assert "expansion" in identity, identity
    return records, events


def arithmetic_observations(stdout, instance):
    records = [json.loads(line) for line in stdout.splitlines() if line.startswith("{")]
    events = [record for record in records if record["kind"] != "result"]
    assert len(events) == 14 * 3, events
    spec = log_spec(
        "arith",
        "sum_left",
        0,
        "sum_right",
        0,
        "subtract",
        0,
        "product",
        0,
        "equal",
        False,
        "constants",
        0,
        0,
    )
    for epoch in range(14):
        row = ARITHMETIC_ROWS[epoch // 2]
        expected = [
            ("log", spec, [value(item) for item in row]),
            ("log", log_spec("arithmetic_scalar", 0), [value(row[0])]),
            ("report", {"name": "arithmetic_progress"}, [value(row[0])]),
        ]
        actual = events[epoch * 3 : (epoch + 1) * 3]
        assert [
            (item["kind"], item["spec"], item["values"]) for item in actual
        ] == expected, (epoch, actual)
        for item in actual:
            assert item["instance"] == instance
            assert int(item["evaluation_epoch"]) == epoch
            assert int(item["commit_epoch"]) == epoch + 1
            assert json.loads(item["site"])["site"]["definition"].startswith(
                "observation_groups.source_observation_groups."
            )
    return records, events


def main():
    parser = argparse.ArgumentParser()
    for name in (
        "repo",
        "source-compiler",
        "linker",
        "emitter",
        "cxx",
        "verilator",
        "scratch",
    ):
        parser.add_argument("--" + name, required=True)
    args = parser.parse_args()
    repo = Path(args.repo).resolve()
    scratch = Path(args.scratch).resolve()
    scratch.mkdir(parents=True, exist_ok=True)
    work = Path(tempfile.mkdtemp(prefix="observation-groups-", dir=scratch))
    source = work / "source"
    source.mkdir()
    fixture = Path(__file__).with_name("source_observation_groups.py")
    shutil.copy2(fixture, source)
    prefix = Path(args.source_compiler).resolve().parent.parent
    runtime = next(
        path
        for path in (
            prefix / "simulator/gfsim/libpyc6_runtime.a",
            prefix / "lib/libpyc6_runtime.a",
        )
        if path.is_file()
    )
    env = dict(
        os.environ,
        PYTHONPATH=str(repo / "python/pycircuit/src"),
        PYCIRCUIT_SOURCE_COMPILER=args.source_compiler,
        PYCIRCUIT_LINKER=args.linker,
        PYCIRCUIT_EMITTER=args.emitter,
    )
    inputs = [
        Path(__file__),
        fixture,
        Path(args.source_compiler),
        Path(args.linker),
        Path(args.emitter),
        runtime,
        repo / "cmake/verify_example.py",
    ]

    def digest(path):
        return hashlib.sha256(path.read_bytes()).hexdigest()

    input_hashes = {str(path): digest(path) for path in inputs}
    (work / "input-hashes.json").write_text(json.dumps(input_hashes, indent=2) + "\n")
    commands = []

    def run(label, command, failure=False):
        command = list(map(str, command))
        result = subprocess.run(
            command, cwd=repo, env=env, capture_output=True, text=True, timeout=120
        )
        (work / (label + ".stdout")).write_text(result.stdout)
        (work / (label + ".stderr")).write_text(result.stderr)
        commands.append(
            {"command": command, "exit_status": result.returncode, "label": label}
        )
        (work / "commands.json").write_text(json.dumps(commands, indent=2) + "\n")
        assert (result.returncode != 0) if failure else (result.returncode == 0), (
            command,
            result.stdout,
            result.stderr,
        )
        return result

    def cli(label, *arguments):
        return run(label, [sys.executable, "-m", "pycircuit.cli", *arguments])

    unit = work / "unit"
    cli(
        "compile",
        "compile",
        "-c",
        source / fixture.name,
        "--source-root",
        source,
        "--package-prefix",
        "observation_groups",
        "-o",
        unit,
    )
    cases = []
    successful_prefix = None
    for symbol, failing, check_count, cycles, arithmetic in (
        ("ObservationGroups", False, 2, 3, False),
        ("ObservationGroupsFailure", True, 3, 3, False),
        ("ArithmeticObservationGroups", False, 1, 7, True),
        ("CapturedArithmeticObservationGroups", False, 1, 7, True),
    ):
        output = work / symbol
        output.mkdir()
        entry = "observation_groups.source_observation_groups." + symbol
        final = output / "system.ac"
        cli(symbol + "-link", "link", unit, "--top", entry, "-o", final)
        transcripts = []
        for target in ("cpp", "verilog"):
            generated = output / target
            cli(
                symbol + "-emit-" + target,
                "emit",
                final,
                "--target",
                target,
                "-o",
                generated,
            )
            assert json.loads(
                (generated / "simulation_verification.json").read_text()
            ) == {
                "entry": {"definition": f'@"{entry}"', "arguments": []},
                "entry_source": {
                    "package": "observation_groups",
                    "path": fixture.name,
                },
                "source_checks": check_count,
            }
            binary = output / ("simulation-" + target)
            if target == "cpp":
                manifest = json.loads((generated / "generated.json").read_text())
                run(
                    symbol + "-build-cpp",
                    [
                        args.cxx,
                        "-std=c++20",
                        "-pthread",
                        "-I" + str(repo / "include"),
                        "-I" + str(generated),
                        *[
                            generated / item["path"]
                            for item in manifest["files"]
                            if item["path"].endswith(".cpp")
                        ],
                        runtime,
                        "-o",
                        binary,
                    ],
                )
            else:
                run(
                    symbol + "-build-rtl",
                    [
                        sys.executable,
                        repo / "cmake/verify_example.py",
                        "--compile-system",
                        generated,
                        "--output",
                        binary,
                        "--include",
                        repo / "include",
                        "--verilator",
                        args.verilator,
                    ],
                )
            for workers in (1, 2) if target == "cpp" else (1,):
                result = run(
                    f"{symbol}-{target}-{workers}",
                    (
                        [binary, "--cycles", str(cycles), "--workers", str(workers)]
                        if target == "cpp"
                        else [binary, f"+cycles={cycles}"]
                    ),
                    failure=failing,
                )
                records, events = (
                    arithmetic_observations(
                        result.stdout,
                        "root" if symbol.startswith("Captured") else "root/dut",
                    )
                    if arithmetic
                    else observations(result.stdout, 4 if failing else cycles * 2)
                )
                terminal = [record for record in records if record["kind"] == "result"]
                if target == "cpp" or not failing:
                    assert len(terminal) == 1
                    assert terminal[0]["status"] == (
                        "FAILED" if failing else "TERMINATED"
                    )
                    assert int(terminal[0]["epoch_time"]) == (
                        4 if failing else cycles * 2
                    )
                    if failing:
                        assert terminal[0]["error"]["phase"] == "check"
                        assert terminal[0]["error"]["code"] == "source_check_failed"
                        assert terminal[0]["error"]["message"] == "grouped failure"
                    else:
                        gauge = [
                            item
                            for item in terminal[0]["statistics"]
                            if item["name"]
                            == ("arithmetic_progress" if arithmetic else "ticks")
                        ]
                        assert len(gauge) == 1 and int(gauge[0]["value"]) == (
                            16 if arithmetic else 2
                        )
                else:
                    assert "check failed at epoch 4" in result.stdout + result.stderr
                transcripts.append(events)
        assert transcripts[0] == transcripts[1] == transcripts[2]
        semantic_prefix = [
            {
                key: record[key]
                for key in (
                    "kind",
                    "instance",
                    "evaluation_epoch",
                    "commit_epoch",
                    "spec",
                    "values",
                )
            }
            for record in transcripts[0][:28]
        ]
        if failing:
            assert semantic_prefix == successful_prefix
        elif not arithmetic:
            successful_prefix = semantic_prefix
        cases.append(
            {
                "entry": entry,
                "source_checks": check_count,
                "failed_work": failing,
                "epochs": 4 if failing else cycles * 2,
                "native_workers": [1, 2],
                "rtl": "pass",
                "complete_observations_equal": True,
            }
        )
    protected_paths = [
        path
        for folder in (
            unit,
            *[work / case["entry"].rsplit(".", 1)[-1] for case in cases],
        )
        for path in folder.rglob("*")
        if path.is_file()
    ]
    published = {str(path): digest(path) for path in protected_paths}
    source_path = source / fixture.name
    original_source = source_path.read_text()
    rejected = []
    for label, expression, diagnostic in (
        (
            "boolean-plus-byte",
            "True + x",
            "Boolean literal cannot be implicitly converted",
        ),
        (
            "unequal-fixed-widths",
            "wide + x",
            "unsigned arithmetic operands require equal widths",
        ),
        ("oversize-fixed-peer", "256 + x", "width"),
    ):
        # Mutate the captured-register expression, retaining the separately
        # checked module-input arithmetic expression and all published products.
        prefix, suffix = original_source.rsplit("17 + x", 1)
        source_path.write_text(prefix + expression + suffix)
        try:
            result = run(
                "reject-" + label,
                [
                    sys.executable,
                    "-m",
                    "pycircuit.cli",
                    "compile",
                    "-c",
                    source_path,
                    "--source-root",
                    source,
                    "--package-prefix",
                    "observation_groups",
                    "-o",
                    unit,
                    "--replace",
                ],
                failure=True,
            )
            assert diagnostic in result.stderr, (label, result.stderr)
            assert published == {str(path): digest(path) for path in protected_paths}
            rejected.append(
                {
                    "case": label,
                    "diagnostic": result.stderr,
                    "publication_preserved": True,
                }
            )
        finally:
            source_path.write_text(original_source)
    assert input_hashes == {str(path): digest(path) for path in inputs}
    (work / "evidence.json").write_text(
        json.dumps(
            {
                "role": "independent generated observation tests",
                "model": "gpt-6.1-sol",
                "effort": "high",
                "cases": cases,
                "negative_cases": rejected,
                "input_sha256": input_hashes,
                "artifact_sha256": {
                    str(path): digest(path)
                    for path in work.rglob("*")
                    if path.is_file()
                },
                "scope": "Supported unconditional observations only. Partial/inactive publication groups are checked through actual Runtime ObservationSlots, without inventing source conditional-observation admission.",
            },
            indent=2,
        )
        + "\n"
    )
    print(
        "PASS: typed distinct grouped values, separate child occurrences, full failed-Work prefix, arithmetic boundaries and protected refusals; native workers1/2 and RTL"
    )  # noqa: T201 - standalone gate receipt


if __name__ == "__main__":
    main()
