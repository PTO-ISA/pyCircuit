"""Literal historical driver oracles against source-unit generated hardware."""

import argparse
import ast
import hashlib
import json
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

BASELINE = "8887e6dec7b4cc530a9967c860dc6a224d79a4ab"
CASES = (
    ("reset_invalidate_order_smoke", "ResetInvalidateOrder", "y", 0),
    ("trace_dsl_smoke", "TraceDsl", "y0", 1),
    ("xz_value_model_smoke", "XzValueModel", "y", 2),
)


def original_rows(oracles, name, first_port):
    """Read the retained literal drive/pre/post calls without executing old APIs."""
    directory = oracles / name
    tree = ast.parse((directory / ("tb_" + name + ".py.txt")).read_text())
    function = next(
        node
        for node in tree.body
        if isinstance(node, ast.FunctionDef) and node.name == "tb"
    )
    config = ast.parse((directory / (name + "_config.py.txt")).read_text())
    presets = next(
        ast.literal_eval(node.value)
        for node in config.body
        if isinstance(node, ast.Assign)
        and any(
            isinstance(target, ast.Name) and target.id == "TB_PRESETS"
            for target in node.targets
        )
    )
    cycles = [{}]
    reset_cycles = None
    for node in function.body:
        if not isinstance(node, ast.Expr) or not isinstance(node.value, ast.Call):
            continue
        call = node.value
        if not isinstance(call.func, ast.Attribute):
            continue
        action = call.func.attr
        if action == "reset":
            options = {item.arg: ast.literal_eval(item.value) for item in call.keywords}
            assert ast.literal_eval(call.args[0]) == "rst"
            reset_cycles = options["cycles_asserted"]
            assert options["cycles_deasserted"] == 0
        elif action == "next":
            cycles.append({})
        elif action == "drive":
            assert "stimulus" not in cycles[-1]
            cycles[-1]["stimulus"] = ast.literal_eval(call.args[1])
        elif action == "expect":
            port, value = map(ast.literal_eval, call.args)
            phase = ast.literal_eval(
                next(item.value for item in call.keywords if item.arg == "phase")
            )
            key = (port, phase)
            assert key not in cycles[-1]
            cycles[-1][key] = value
    assert reset_cycles == 2
    assert len(cycles) == presets["smoke"]["finish"]
    rows = []
    for cycle in cycles:
        second_port = "y1" if first_port == "y0" else first_port
        rows.append(
            (
                cycle["stimulus"],
                cycle[first_port, "pre"],
                cycle[first_port, "post"],
                cycle[second_port, "pre"],
                cycle[second_port, "post"],
            )
        )
    return rows


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
    evidence = Path(args.scratch).resolve()
    evidence.mkdir(parents=True, exist_ok=True)
    work = Path(tempfile.mkdtemp(prefix="history-features-", dir=evidence))
    (work / "units").mkdir()
    fixtures = Path(__file__).resolve().parent
    owned = fixtures / "history-features"
    source = work / "source"
    source.mkdir()
    for path in owned.glob("*.py"):
        shutil.copy2(path, source)
    env = dict(
        os.environ,
        PYTHONPATH=str(repo / "python"),
        PYTHONDONTWRITEBYTECODE="1",
        PYCIRCUIT_SOURCE_COMPILER=args.source_compiler,
        PYCIRCUIT_LINKER=args.linker,
        PYCIRCUIT_EMITTER=args.emitter,
    )
    commands = []

    def run(command):
        command = list(map(str, command))
        result = subprocess.run(
            command, cwd=repo, env=env, text=True, capture_output=True, timeout=120
        )
        commands.append(
            {
                "command": command,
                "exit_status": result.returncode,
                "stdout": result.stdout,
                "stderr": result.stderr,
            }
        )
        (work / "commands.json").write_text(json.dumps(commands, indent=2) + "\n")
        assert result.returncode == 0, commands[-1]
        return result.stdout

    def cli(*command):
        return run([sys.executable, "-m", "pycircuit.cli", *command])

    def compile_unit(name, providers=()):
        unit = work / "units" / name
        command = [
            "compile",
            "-c",
            source / (name + ".py"),
            "--source-root",
            source,
            "--package-prefix",
            "history_features",
            "-o",
            unit,
        ]
        for provider in providers:
            command.extend(["-I", provider])
        cli(*command)
        return unit

    runtime_root = Path(args.source_compiler).resolve().parent.parent
    runtime = next(
        path
        for path in (
            runtime_root / "runtime/libpyc6_runtime.a",
            runtime_root / "lib/libpyc6_runtime.a",
        )
        if path.is_file()
    )

    def digest(path):
        return hashlib.sha256(path.read_bytes()).hexdigest()

    inputs = [
        Path(args.source_compiler),
        Path(args.linker),
        Path(args.emitter),
        runtime,
        Path(__file__),
        fixtures / "source-historical-features.cpp",
        fixtures / "source-historical-features.sv",
        repo / "cmake/verify_example.py",
        *[path for path in owned.rglob("*") if path.is_file()],
    ]
    input_hashes = {str(path): digest(path) for path in inputs}
    (work / "input-hashes.json").write_text(json.dumps(input_hashes, indent=2) + "\n")

    leaf = compile_unit("trace_leaf")
    results = []
    feature_units = {}
    for name, symbol, port, kind in CASES:
        providers = (leaf,) if kind == 1 else ()
        unit = compile_unit(name, providers)
        feature_units[name] = unit
        output = work / name
        output.mkdir()
        rows = original_rows(owned / "oracles", name, port)
        rowfile = output / "original-rows.txt"
        rowfile.write_text("".join(" ".join(map(str, row)) + "\n" for row in rows))
        final = output / "design.ac"
        entry = f"history_features.{name}.{symbol}"
        cli("link", *providers, unit, "--top", entry, "-o", final)
        for target in ("cpp", "verilog"):
            cli("emit", final, "--target", target, "-o", output / target)
        cpp = output / "cpp"
        receipt = json.loads((cpp / "generated.json").read_text())
        sources = [
            cpp / row["path"]
            for row in receipt["files"]
            if row["path"].endswith(".cpp")
        ]
        binary = output / "native"
        run(
            [
                args.cxx,
                "-std=c++20",
                "-pthread",
                f"-DFEATURE_KIND={kind}",
                f"-DFEATURE_CYCLES={len(rows)}",
                "-I" + str(repo / "include"),
                "-I" + str(cpp),
                fixtures / "source-historical-features.cpp",
                *sources,
                runtime,
                "-o",
                binary,
            ]
        )
        expected = [
            "CYCLE " + str(index) + " " + " ".join(map(str, row[1:]))
            for index, row in enumerate(rows)
        ]
        for workers in (1, 2):
            trace = run([binary, workers, rowfile])
            (output / f"native-{workers}.stdout").write_text(trace)
            assert trace.splitlines()[-1] == "PASS"
            assert [
                line for line in trace.splitlines() if line.startswith("CYCLE ")
            ] == expected
            if kind == 2:
                assert "HOST X Z mixed-mask capture" in trace
        rtl = output / "verilog"
        receipt = json.loads((rtl / "generated.json").read_text())
        sources = [
            rtl / row["path"] for row in receipt["files"] if row["role"] == "rtl"
        ]
        defines = [f"+define+FEATURE_CYCLES={len(rows)}"]
        if kind == 0:
            defines.append("+define+FEATURE_COUNTER")
        elif kind == 1:
            defines.append("+define+FEATURE_TRACE")
        rtl_build = output / "verilated"
        run(
            [
                args.verilator,
                "--binary",
                "--timing",
                "--top-module",
                "tb",
                "--Mdir",
                rtl_build,
                "-j",
                "2",
                "-Wno-fatal",
                "-CFLAGS",
                "-std=c++20",
                *defines,
                *sorted((repo / "include/verilog").glob("*.v")),
                *sources,
                fixtures / "source-historical-features.sv",
            ]
        )
        trace = run([rtl_build / "Vtb", "+rows=" + str(rowfile)])
        (output / "rtl.stdout").write_text(trace)
        assert "PASS" in trace.splitlines()
        assert [
            line for line in trace.splitlines() if line.startswith("CYCLE ")
        ] == expected
        results.append(
            {
                "name": name,
                "entry": entry,
                "original_cycles": len(rows),
                "workers": [1, 2],
                "native": "pass",
                "rtl_known_pre_post": "pass",
                "native_host_four_state": kind == 2,
            }
        )

    providers = (leaf, *feature_units.values())
    bench = compile_unit("bench", providers)
    closed_results = []
    for name, symbol, event, check_count in (
        ("reset_invalidate_order_smoke", "ResetInvalidateOrderSystem", "counter", 1),
        ("trace_dsl_smoke", "TraceDslSystem", "trace", 2),
        ("xz_value_model_smoke", "XzValueModelSystem", "capture", 1),
    ):
        port = "y0" if event == "trace" else "y"
        rows = original_rows(owned / "oracles", name, port)
        # A held-input drain cycle exposes the final original post edge through
        # the closed system's old-Q Work observations.
        cycles = len(rows) + 1
        expected = [[row[1]] + ([row[3]] if event == "trace" else []) for row in rows]
        expected.append([rows[-1][2]] + ([rows[-1][4]] if event == "trace" else []))
        expected = [values for values in expected for _ in range(2)]
        output = work / symbol
        output.mkdir()
        final = output / "system.ac"
        entry = f"history_features.bench.{symbol}"
        cli("link", *providers, bench, "--top", entry, "-o", final)
        transcripts = []
        for target in ("cpp", "verilog"):
            generated = output / target
            cli("emit", final, "--target", target, "-o", generated)
            metadata = json.loads(
                (generated / "simulation_verification.json").read_text()
            )
            assert metadata == {
                "entry": {"definition": f'@"{entry}"', "arguments": []},
                "entry_source": {"package": "history_features", "path": "bench.py"},
                "source_checks": check_count,
            }
            executable = output / ("simulation-" + target)
            if target == "cpp":
                manifest = json.loads((generated / "generated.json").read_text())
                sources = [
                    generated / item["path"]
                    for item in manifest["files"]
                    if item["path"].endswith(".cpp")
                ]
                run(
                    [
                        args.cxx,
                        "-std=c++20",
                        "-pthread",
                        "-I" + str(repo / "include"),
                        "-I" + str(generated),
                        *sources,
                        runtime,
                        "-o",
                        executable,
                    ]
                )
            else:
                run(
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
                    ]
                )
            for workers in (1, 2) if target == "cpp" else (1,):
                arguments = (
                    ["--cycles", str(cycles), "--workers", str(workers)]
                    if target == "cpp"
                    else [f"+cycles={cycles}"]
                )
                stdout = run([executable, *arguments])
                (output / f"{target}-{workers}.stdout").write_text(stdout)
                records = [
                    json.loads(line)
                    for line in stdout.splitlines()
                    if line.startswith("{")
                ]
                observations = [record for record in records if record["kind"] == "log"]
                assert len(observations) == cycles * 2
                assert [record["spec"]["event"] for record in observations] == [
                    event
                ] * (cycles * 2)
                assert all(
                    record["spec"]["items"]
                    == [
                        {"kind": "value", "ordinal": index}
                        for index in range(len(expected[0]))
                    ]
                    for record in observations
                )
                assert [
                    int(record["evaluation_epoch"]) for record in observations
                ] == list(range(cycles * 2))
                assert [int(record["commit_epoch"]) for record in observations] == list(
                    range(1, cycles * 2 + 1)
                )
                assert [
                    [int(value["value"]) for value in record["values"]]
                    for record in observations
                ] == expected
                results_record = [
                    record for record in records if record["kind"] == "result"
                ]
                assert (
                    len(results_record) == 1
                    and results_record[0]["status"] == "TERMINATED"
                )
                assert int(results_record[0]["epoch_time"]) == cycles * 2
                transcripts.append(observations)
        assert transcripts[0] == transcripts[1] == transcripts[2]
        closed_results.append(
            {
                "entry": entry,
                "original_cycles": len(rows),
                "drain_cycles": 1,
                "run_cycles": cycles,
                "source_checks": check_count,
                "native_workers": [1, 2],
                "rtl": "pass",
                "complete_observations_equal": True,
            }
        )

    products = [
        path
        for path in work.rglob("*")
        if path.is_file()
        and (
            path.suffix in {".ac", ".cpp", ".hpp", ".v", ".sv"}
            or path.name
            in {
                "unit.json",
                "generated.json",
                "native",
                "Vtb",
                "original-rows.txt",
                "simulation-cpp",
                "simulation-verilog",
                "simulation_verification.json",
            }
        )
    ]
    assert input_hashes == {str(path): digest(path) for path in inputs}
    (work / "evidence.json").write_text(
        json.dumps(
            {
                "baseline_revision": BASELINE,
                "role": "independent literal hardware translation and tests",
                "model": "gpt-6.1-sol",
                "effort": "high",
                "cases": results,
                "closed_systems": closed_results,
                "input_sha256": input_hashes,
                "artifact_sha256": {str(path): digest(path) for path in products},
                "remaining_scope": [
                    "Legacy ProbeBuilder/ProbeView tagged probe DSL and trace selector/window JSON are retained historical assets, not established by these numerical tests.",
                    "Known physical clock/reset levels and two asserted reset cycles are covered; unknown clocks/resets and automatic domain scheduling are not asserted here.",
                    "Historical XZ driver uses only known values; additional X/Z/mixed masks test native host I/O and capture, not source-level four-state constructors or Verilator four-state behavior.",
                    "The original trace driver feeds both leaf instances identical inputs. Distinct generated allocations are retained; dynamic instance isolation is covered by tests/system/test_source_system_execution.py::test_closed_system_generated_cpp_and_verilator_observations with independently enabled left/right accumulators, not inferred from identical trace outputs.",
                ],
            },
            indent=2,
        )
        + "\n"
    )
    print(
        "PASS: three historical module roots and closed systems; complete original pre/post drivers, native workers1/2 and RTL, native host X/Z capture"
    )  # noqa: T201 - standalone gate receipt


if __name__ == "__main__":
    main()
