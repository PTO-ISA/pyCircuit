"""Compile actual slot/issue environments and compare every independent epoch."""

import argparse
import copy
import hashlib
import importlib.util
import json
import os
import shutil
import subprocess
import sys
from pathlib import Path

import drivers


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def observed_rows(kind, expected, output):
    names = (
        ("left_input", "right_input", "left_output", "right_output")
        if kind == "mailbox"
        else ("wakeups", "allocations", "output")
    )
    counters = {
        "pops": dict.fromkeys(names, 0),
        "pushes": dict.fromkeys(names, 0),
        "received": {name: [] for name in names if name.endswith("output")},
    }
    actual = []
    lines = [line.split() for line in output.splitlines() if line.startswith("ROW ")]
    assert len(lines) == len(expected["rows"]), "full history required"
    for line, row in zip(lines, expected["rows"], strict=True):
        assert len(line) == 6
        controls = drivers.parse_controls(line[5], kind)
        before = copy.deepcopy(counters)
        actions = row["actions"]
        for name, values in actions.get("offer", {}).items():
            prefix = (
                name.removesuffix("_input") + "_in"
                if kind == "mailbox"
                else {"wakeups": "wakeup", "allocations": "allocation"}[name]
            )
            if controls[prefix + "_ready"]:
                counters["pushes"][name] += len(values)
        if kind == "mailbox":
            for side in ("left", "right"):
                if controls[side + "_capture"] and controls[side + "_in_available"]:
                    counters["pops"][side + "_input"] += 1
                if controls[side + "_publish"] and controls[side + "_out_ready"]:
                    counters["pushes"][side + "_output"] += 1
        else:
            for name, prefix in (("wakeups", "wakeup"), ("allocations", "allocation")):
                if controls[prefix + "_capture"] and controls[prefix + "_available"]:
                    counters["pops"][name] += 1
            if controls["publish"] and controls["issued_ready"]:
                counters["pushes"]["output"] += 1
        for name in (*actions.get("take", []), *actions.get("sink", [])):
            prefix = (
                name.removesuffix("_output") + "_out" if kind == "mailbox" else "issued"
            )
            if controls[prefix + "_available"]:
                counters["pops"][name] += 1
                counters["received"][name].append(controls[prefix + "_head"])
        actual.append(
            {
                "epoch": int(line[1]),
                "packed_before": str(int(line[2], 2)),
                "packed_work": str(int(line[3], 2)),
                "packed_after": str(int(line[4], 2)),
                "before_counters": before,
                "work_counters": before,
                "after_counters": copy.deepcopy(counters),
                "controls": controls,
            }
        )
    return {"rows": actual}


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
        p
        for p in (
            prefix / "lib/libpyc6_runtime.a",
            prefix / "runtime/libpyc6_runtime.a",
        )
        if p.is_file()
    )
    env = dict(
        os.environ,
        PYTHONPATH=str(repo / "python"),
        PYCIRCUIT_SOURCE_COMPILER=args.source_compiler,
        PYCIRCUIT_LINKER=args.linker,
        PYCIRCUIT_EMITTER=args.emitter,
        MAKEFLAGS="CFG_CXXFLAGS_PCH_I=-include",
    )
    oracle = repo / "tests/compiler/oracles/history_slot_issue"
    inputs = [*fixtures.rglob("*"), *oracle.glob("*")]
    inputs = [p for p in inputs if p.is_file() and "__pycache__" not in p.parts]
    product_sources = [
        *(repo / "python/pycircuit").glob("*.py"),
        *(repo / "include/gfsim").glob("*.h"),
        *(repo / "include/verilog").glob("*.v"),
    ]
    paths = [
        *inputs,
        *product_sources,
        Path(args.source_compiler),
        Path(args.linker),
        Path(args.emitter),
        runtime,
    ]
    before = {str(p): digest(p) for p in paths}
    commands = []

    def run(label, command):
        command = list(map(str, command))
        result = subprocess.run(
            command, cwd=repo, env=env, capture_output=True, text=True, timeout=300
        )
        (scratch / (label + ".stdout")).write_text(result.stdout)
        (scratch / (label + ".stderr")).write_text(result.stderr)
        commands.append(
            {"label": label, "command": command, "exit_status": result.returncode}
        )
        (scratch / "commands.json").write_text(json.dumps(commands, indent=2) + "\n")
        assert result.returncode == 0, (
            label,
            result.stdout[-2000:],
            result.stderr[-2000:],
        )
        return result.stdout

    def cli(label, *args):
        return run(label, [sys.executable, "-m", "pycircuit.cli", *args])

    sys.path.insert(0, str(oracle))
    spec = importlib.util.spec_from_file_location(
        "slot_issue_checker", oracle / "check.py"
    )
    checker = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(checker)
    expected_data = checker.data()
    literal = json.loads((oracle / "vectors.json").read_text())
    manifest = json.loads((fixtures / "baseline/manifest.json").read_text())
    for asset in manifest["assets"]:
        assert digest(fixtures / "baseline" / asset["asset"]) == asset["sha256"]
    assert literal["schema"] == 1
    common_units = []
    for module_name in ("payload_types", "valid_cell", "payload_cell", "providers"):
        unit = scratch / (module_name + "-unit")
        imports = [value for published in common_units for value in ("-I", published)]
        cli(
            "compile-" + module_name,
            "compile",
            "-c",
            fixtures / (module_name + ".py"),
            "--source-root",
            fixtures,
            "--package-prefix",
            "history_slot_issue",
            *imports,
            "-o",
            unit,
            "--replace",
        )
        common_units.append(unit)
    records = []
    for name, case in literal["executable"].items():
        kind = case["kind"]
        directory = scratch / name
        directory.mkdir(exist_ok=True)
        source_dir = directory / "source"
        source_dir.mkdir(exist_ok=True)
        source = source_dir / "systems.py"
        source.write_text(
            drivers.scheduled_source((fixtures / "systems.py").read_text(), case)
        )
        unit = directory / "system-unit"
        cli(
            name + "-compile",
            "compile",
            "-c",
            source,
            "--source-root",
            source_dir,
            "--package-prefix",
            "history_slot_issue",
            *[value for published in common_units for value in ("-I", published)],
            "-o",
            unit,
            "--replace",
        )
        final = directory / "final.ac"
        symbol = "slot_rule_mailbox" if kind == "mailbox" else "resident_issue"
        cli(
            name + "-link",
            "link",
            *common_units,
            unit,
            "--top",
            "history_slot_issue.systems." + symbol,
            "-o",
            final,
            "--replace",
        )
        for backend in ("cpp", "verilog"):
            cli(
                name + "-" + backend + "-emit",
                "emit",
                final,
                "--target",
                backend,
                "-o",
                directory / backend,
                "--replace",
            )
        cpp = directory / "cpp"
        rtl = directory / "verilog"
        provider_header = (cpp / "sources/history_slot_issue/providers.hpp").read_text()
        system_header = (cpp / "sources/history_slot_issue/systems.hpp").read_text()
        visible = directory / "read-only-visibility"
        shutil.copytree(cpp, visible, dirs_exist_ok=True)
        visibility = []
        for path in visible.rglob("*.hpp"):
            original = path.read_text()
            transformed = original.replace("private:", "public:").replace(
                "protected:", "public:"
            )
            path.write_text(transformed)
            visibility.append(
                {
                    "path": str(path.relative_to(visible)),
                    "original_sha256": hashlib.sha256(original.encode()).hexdigest(),
                    "instrumented_sha256": digest(path),
                    "only_access_labels": transformed
                    == original.replace("private:", "public:").replace(
                        "protected:", "public:"
                    ),
                }
            )
        (directory / "read-only-visibility.json").write_text(
            json.dumps(visibility, indent=2) + "\n"
        )
        native_driver = directory / "driver.cpp"
        native_driver.write_text(
            (fixtures / "module_harness.cpp")
            .read_text()
            .replace(
                "@CONFIG@", drivers.cpp_config(system_header, provider_header, kind)
            )
            .replace("@EPOCHS@", str(len(case["rows"])))
        )
        executable = directory / "native"
        run(
            name + "-native-build",
            [
                args.cxx,
                "-O2",
                "-std=c++20",
                "-pthread",
                "-I" + str(repo / "include"),
                "-I" + str(visible),
                native_driver,
                *(visible / "sources").rglob("*.cpp"),
                runtime,
                "-o",
                executable,
            ],
        )
        sv_driver = directory / "driver.sv"
        sv_driver.write_text(
            (fixtures / "module_harness.sv")
            .read_text()
            .replace(
                "@CONFIG@", drivers.verilog_config(system_header, provider_header, kind)
            )
            .replace("@BITS_MINUS_ONE@", str(case["snapshot_bits"] - 1))
            .replace("@EPOCHS@", str(len(case["rows"])))
        )
        rtl_executable = directory / "rtl-run"
        run(
            name + "-rtl-build",
            [
                args.verilator,
                "--binary",
                "--timing",
                "-CFLAGS",
                "-std=c++20",
                "--top-module",
                "tb",
                "-j",
                "2",
                "-Wno-fatal",
                "--Mdir",
                directory / "rtl-build",
                "-o",
                rtl_executable,
                rtl / "design_top.sv",
                *rtl.rglob("*.v"),
                repo / "include/verilog/dffe.v",
                repo / "include/verilog/fifo.v",
                sv_driver,
            ],
        )
        for label, command in (
            ("workers1", [executable, "--workers", "1", "--events", "-"]),
            ("workers2", [executable, "--workers", "2", "--events", "-"]),
            ("rtl", [rtl_executable]),
        ):
            output = run(name + "-" + label, command)
            actual = observed_rows(kind, case, output)
            observed = directory / (label + "-observations.json")
            observed.write_text(json.dumps(actual, indent=2) + "\n")
            checker.check_case(expected_data["executable"][name], actual)
            records.append(
                {
                    "case": name,
                    "backend": label,
                    "epochs": len(case["rows"]),
                    "full_snapshot_bits": case["snapshot_bits"],
                    "final_sha256": digest(final),
                    "input_source_sha256": digest(source),
                    "observed_sha256": digest(observed),
                    "checked": True,
                }
            )
    after = {str(p): digest(p) for p in paths}
    assert before == after, "candidate inputs changed during the gate"
    (scratch / "receipt.json").write_text(
        json.dumps(
            {
                "before": before,
                "after": after,
                "unchanged": True,
                "records": records,
                "original_reference_only": list(literal["original"]),
                "scope": "All reset-reachable literal histories; exact historical exceptional host setup remains separate reference evidence.",
            },
            indent=2,
        )
        + "\n"
    )
    print(f"historical slot/issue gate passed: {len(records)} native/RTL executions")


if __name__ == "__main__":
    main()
