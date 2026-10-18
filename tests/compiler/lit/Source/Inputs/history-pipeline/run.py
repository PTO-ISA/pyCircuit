"""Compile historical source systems and compare public observations to host oracles."""

import argparse
import ast
import hashlib
import importlib.util
import json
import os
import subprocess
import sys
from pathlib import Path


def load(path):
    spec = importlib.util.spec_from_file_location(path.stem, path)
    result = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(result)
    return result


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def source_for_root(source_text, root):
    """Keep one root's declaration closure verbatim at its original line numbers."""
    tree = ast.parse(source_text)
    declarations = {
        node.name: node
        for node in tree.body
        if isinstance(node, (ast.FunctionDef, ast.ClassDef))
    }
    assert root in declarations, root
    needed = set()
    pending = [root]
    while pending:
        name = pending.pop()
        if name in needed:
            continue
        needed.add(name)
        pending.extend(
            node.id
            for node in ast.walk(declarations[name])
            if isinstance(node, ast.Name)
            and node.id in declarations
            and node.id not in needed
        )
    lines = source_text.splitlines(keepends=True)
    for name, node in declarations.items():
        if name not in needed:
            first = min([node.lineno, *[item.lineno for item in node.decorator_list]])
            lines[first - 1 : node.end_lineno] = ["\n"] * (node.end_lineno - first + 1)
    result = "".join(lines)
    assert len(result.splitlines()) == len(source_text.splitlines())
    filtered = ast.parse(result)
    retained = {
        node.name: node.lineno
        for node in filtered.body
        if isinstance(node, (ast.FunctionDef, ast.ClassDef))
    }
    assert retained == {name: declarations[name].lineno for name in needed}
    return result


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
    fixtures = Path(__file__).resolve().parent
    scratch = Path(args.scratch).resolve()
    scratch.mkdir(parents=True, exist_ok=True)
    (scratch / "receipt.json").unlink(missing_ok=True)
    prefix = Path(args.source_compiler).resolve().parent.parent
    runtime = next(
        path
        for path in (
            prefix / "runtime/libpyc6_runtime.a",
            prefix / "lib/libpyc6_runtime.a",
        )
        if path.is_file()
    )
    runtime_hash = digest(runtime)
    env = dict(
        os.environ,
        PYTHONPATH=str(repo / "python"),
        PYCIRCUIT_SOURCE_COMPILER=args.source_compiler,
        PYCIRCUIT_LINKER=args.linker,
        PYCIRCUIT_EMITTER=args.emitter,
        MAKEFLAGS="CFG_CXXFLAGS_PCH_I=-include",
    )
    commands = []

    def run(label, command, failure=False):
        command = list(map(str, command))
        result = subprocess.run(
            command, cwd=repo, env=env, capture_output=True, text=True, timeout=240
        )
        (scratch / (label + ".stdout")).write_text(result.stdout)
        (scratch / (label + ".stderr")).write_text(result.stderr)
        commands.append(
            {"label": label, "command": command, "exit_status": result.returncode}
        )
        (scratch / "commands.json").write_text(json.dumps(commands, indent=2) + "\n")
        assert (result.returncode != 0) if failure else (result.returncode == 0), (
            label,
            result.stdout[-1000:],
            result.stderr[-1000:],
        )
        return result

    def cli(label, *arguments):
        return run(label, [sys.executable, "-m", "pycircuit.cli", *arguments])

    provider = fixtures.parent / "queue-source/pyc_credit_pipeline.py"
    oracle_path = (
        repo / "tests/compiler/oracles/queue_source/credit_independent_check.py"
    )
    models_path = repo / "tests/compiler/oracles/queue_source/models.py"
    input_paths = (
        Path(__file__),
        fixtures / "gfsim_expect_pipeline.py",
        fixtures / "pyc_credit_pipeline.py",
        provider,
        oracle_path,
        models_path,
        repo / "cmake/verify_example.py",
    )
    input_hashes = {str(path.relative_to(repo)): digest(path) for path in input_paths}
    helper_paths = (Path(args.source_compiler), Path(args.linker), Path(args.emitter))
    helper_hashes = {str(path): digest(path) for path in helper_paths}

    def compile_unit(name, source, package, output, imports=()):
        cli(
            "compile-" + name,
            "compile",
            "-c",
            source,
            "--source-root",
            source.parent,
            "--package-prefix",
            package,
            *imports,
            "-o",
            output,
            "--replace",
        )
        return output

    provider_unit = compile_unit("provider", provider, "q4_queue", scratch / "provider")
    expect_unit = compile_unit(
        "expect",
        fixtures / "gfsim_expect_pipeline.py",
        "history_pipeline",
        scratch / "expect",
    )
    credit_source = fixtures / "pyc_credit_pipeline.py"
    credit_text = credit_source.read_text()
    credit_units = {}
    split_hashes = {}
    for symbol in (
        "pyc_credit_pipeline",
        "credit_c4",
        "credit_c5",
        "credit_c7",
        "credit_c8",
    ):
        source_directory = scratch / "credit-sources" / symbol
        source_directory.mkdir(parents=True, exist_ok=True)
        split_source = source_directory / "pyc_credit_pipeline.py"
        split_source.write_text(source_for_root(credit_text, symbol))
        split_hashes[symbol] = digest(split_source)
        credit_units[symbol] = compile_unit(
            "credit-" + symbol,
            split_source,
            "history_pipeline",
            scratch / ("credit-" + symbol),
            ("-I", provider_unit),
        )

    oracle = load(oracle_path)
    expectations = load(models_path).historical_expect_contracts()
    cases = []
    for mode, symbol, cycles in (
        ("positive", "gfsim_expect_pipeline", 70),
        ("idle", "gfsim_expect_invalid_idle", 8),
        ("fault", "gfsim_expect_failure", 8),
    ):
        rows = expectations[mode]["rows"]
        fault = expectations[mode]["first_failure"]
        cases.append(
            ("gfsim_expect_pipeline", symbol, cycles, rows, fault, [expect_unit])
        )
    for section in oracle.program():
        if section["name"] not in ("C1", "C4", "C5", "C7", "C8"):
            continue
        rows, trace, _ = oracle.run_section(section, oracle.knobs())
        assert not any(row["expected_failure"] or row["rst"] for row in rows)
        expected = []
        for epoch, row in enumerate(rows):
            packed = row["expected"]
            expected.append(
                {
                    "epoch": epoch // 2,
                    "ready": (packed >> 25) & 1,
                    "available": (packed >> 24) & 1,
                    "sequence": oracle.sequence_of(packed),
                    "cycles": oracle.cycles_of(packed),
                    "value": oracle.value_of(packed),
                }
            )
        assert trace["events"][-1]["issued_count"] == 0
        assert trace["events"][-1]["completed_count"] == 0
        symbol = (
            "pyc_credit_pipeline"
            if section["name"] == "C1"
            else "credit_" + section["name"].lower()
        )
        cases.append(
            (
                "pyc_credit_pipeline",
                symbol,
                section["epochs"],
                expected,
                None,
                [provider_unit, credit_units[symbol]],
            )
        )

    receipts = []
    for source, symbol, cycles, expected, fault, closure in cases:
        folder = scratch / symbol
        folder.mkdir(exist_ok=True)
        top = f"history_pipeline.{source}.{symbol}"
        cli(
            symbol + "-link",
            "link",
            *closure,
            "--top",
            top,
            "-o",
            folder / "final.ac",
            "--replace",
        )
        traces = []
        for backend in ("cpp", "verilog"):
            generated = folder / backend
            cli(
                symbol + "-emit-" + backend,
                "emit",
                folder / "final.ac",
                "--target",
                backend,
                "-o",
                generated,
                "--replace",
            )
            executable = folder / ("simulation-" + backend)
            if backend == "cpp":
                manifest = json.loads((generated / "generated.json").read_text())
                sources = [
                    generated / item["path"]
                    for item in manifest["files"]
                    if item["path"].endswith(".cpp")
                ]
                run(
                    symbol + "-build-cpp",
                    [
                        args.cxx,
                        "-O2",
                        "-std=c++20",
                        "-pthread",
                        "-I" + str(repo / "include"),
                        "-I" + str(generated),
                        *sources,
                        runtime,
                        "-o",
                        executable,
                    ],
                )
            else:
                run(
                    symbol + "-build-verilog",
                    [
                        sys.executable,
                        repo / "cmake/verify_example.py",
                        "--compile-system",
                        generated,
                        "--output",
                        executable,
                        "--include",
                        repo / "include",
                        "--verilator",
                        args.verilator,
                    ],
                )
            for workers in (1, 2) if backend == "cpp" else (1,):
                label = f"{symbol}-{backend}-workers{workers}"
                options = (
                    ["--cycles", cycles, "--workers", workers]
                    if backend == "cpp"
                    else ["+cycles=" + str(cycles)]
                )
                result = run(label, [executable, *options], failure=fault is not None)
                records = [
                    json.loads(line)
                    for line in result.stdout.splitlines()
                    if line.startswith("{")
                ]
                observations = {}
                for record in records:
                    if record["kind"] == "log":
                        epoch = int(record["evaluation_epoch"])
                        observations.setdefault(epoch, {})[record["spec"]["event"]] = (
                            int(record["values"][0]["value"])
                        )
                assert sorted(observations) == list(range(len(expected))), label
                assert list(observations.values()) == expected, label
                if fault is None:
                    assert (
                        records[-1]["status"] == "TERMINATED"
                        and int(records[-1]["epoch_time"]) == cycles * 2
                    )
                elif backend == "cpp":
                    assert records[-1]["status"] == "FAILED"
                    assert int(records[-1]["epoch_time"]) == fault
                    assert records[-1]["error"]["code"] == "source_check_failed"
                    assert records[-1]["error"]["message"] == "value must be positive"
                else:
                    assert f"failed at epoch {fault}" in result.stdout + result.stderr
                traces.append(list(observations.values()))
        assert all(trace == traces[0] for trace in traces)
        receipts.append(
            {
                "top": top,
                "cycles": cycles,
                "fault_epoch": fault,
                "observations": len(expected),
                "workers": [1, 2],
                "final_sha256": digest(folder / "final.ac"),
            }
        )
        print(top + " passed", flush=True)
    assert all(
        digest(path) == input_hashes[str(path.relative_to(repo))]
        for path in input_paths
    )
    assert all(digest(path) == helper_hashes[str(path)] for path in helper_paths)
    assert digest(runtime) == runtime_hash
    receipt = {
        "cases": receipts,
        "candidate_inputs": input_hashes,
        "native_helpers": helper_hashes,
        "runtime_archive": {"path": str(runtime), "sha256": runtime_hash},
        "credit_source_closures": split_hashes,
        "expectation_ledgers": {
            mode: {key: value for key, value in contract.items() if key != "rows"}
            for mode, contract in expectations.items()
        },
        "limits": [
            "Fresh independent systems do not claim physical reset or sticky failure recovery.",
            "Credit zero-cost and deferred failures remain in the independent queue-source fault gate.",
            "Historical expectation_failed maps to generic source_check_failed with unchanged message.",
            "Repeated pure expectation checking on a held head has no last_ marker; no externally visible mutation is added.",
        ],
    }
    (scratch / "receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")


if __name__ == "__main__":
    main()
