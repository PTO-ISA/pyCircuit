"""Own the full routed graph gate, with independent state/transaction oracles."""

import argparse
import hashlib
import importlib.util
import json
import os
import shutil
import signal
import subprocess
import sys
from pathlib import Path

import drivers
import observations

SOURCES = (
    "records",
    "scheduler",
    "merge",
    "reorder",
    "routed_dependency_pipeline",
    "routed_systems",
)
SYSTEMS = {
    "four_routes_live_dependencies": "routed_dependency_pipeline",
    "full_topology_backpressure": "routed_full_topology_backpressure",
}


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def load_checker(path):
    sys.path.insert(0, str(path.parent))
    spec = importlib.util.spec_from_file_location("routed_independent_check", path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module.check_case


def system_trace(output, case):
    records = [json.loads(line) for line in output.splitlines() if line.startswith("{")]
    phases = {}
    for record in records:
        if record["kind"] == "log":
            phase = int(record["evaluation_epoch"])
            event = record["spec"]["event"]
            assert event not in phases.setdefault(phase, {}), (phase, event)
            phases[phase][event] = int(record["values"][0]["value"])
    count = case["executed_epochs"]
    assert list(phases) == list(
        range(count * 2)
    ), "complete ordered low/high logs required"
    actual = list(phases.values())
    for epoch, row in enumerate(case["rows"]):
        pub = row["public_expected"]
        expected = {"epoch": epoch, "ready": int(pub["ready"])}
        for view in observations.PUBLIC:
            available = pub[view]["available"]
            expected[view + "_available"] = int(available)
            for field in observations.FIELDS:
                expected[view + "_" + field] = (
                    pub[view]["head"][field] if available else 0
                )
        assert actual[epoch * 2] == expected, (epoch, "low", row["label"])
        assert actual[epoch * 2 + 1] == expected, (epoch, "high old Q", row["label"])
    assert records[-1]["kind"] == "result"
    assert records[-1]["status"] == "TERMINATED"
    assert int(records[-1]["epoch_time"]) == count * 2
    return actual


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
    fixture = Path(__file__).resolve().parent
    scratch = Path(args.scratch).resolve()
    scratch.mkdir(parents=True, exist_ok=True)
    (scratch / "receipt.json").unlink(missing_ok=True)
    oracle = repo / "tests/compiler/oracles/history_routed/vectors.json"
    vectors = json.loads(oracle.read_text())
    check_case = load_checker(oracle.parent / "check.py")
    assert len(vectors["executable"]) == 11
    assert (
        sum(case["executed_epochs"] for case in vectors["executable"].values()) == 1792
    )
    inputs = [
        *fixture.glob("*.py"),
        fixture / "README.md",
        *fixture.glob("baseline/*"),
        *oracle.parent.glob("*.py"),
        oracle,
        *repo.glob("python/pycircuit/src/pycircuit/**/*.py"),
        repo / "include/verilog/fifo.v",
        repo / "include/verilog/dffe.v",
        repo / "cmake/verify_example.py",
        repo / "tests/compiler/lit/Source/source-historical-routed.test",
    ]
    input_hashes = {
        str(path.relative_to(repo)): digest(path) for path in inputs if path.is_file()
    }
    helpers = [Path(args.source_compiler), Path(args.linker), Path(args.emitter)]
    opt = helpers[0].parent / "pycircuit-opt"
    if opt.is_file():
        helpers.append(opt)
    helper_hashes = {str(path): digest(path) for path in helpers}
    prefix = helpers[0].resolve().parent.parent
    runtime = next(
        path
        for path in (
            prefix / "simulator/gfsim/libpyc6_runtime.a",
            prefix / "lib/libpyc6_runtime.a",
        )
        if path.is_file()
    )
    runtime_hash = digest(runtime)
    baseline = json.loads((fixture / "baseline/manifest.json").read_text())
    for asset in baseline["assets"]:
        assert digest(fixture / "baseline" / asset["asset"]) == asset["sha256"]
    env = dict(
        os.environ,
        PYTHONPATH=str(repo / "python/pycircuit/src"),
        PYCIRCUIT_SOURCE_COMPILER=args.source_compiler,
        PYCIRCUIT_LINKER=args.linker,
        PYCIRCUIT_EMITTER=args.emitter,
        MAKEFLAGS="CFG_CXXFLAGS_PCH_I=-include",
    )
    commands, receipts, visibility, stages = [], [], [], []

    def run(label, command, text_input=None):
        command = list(map(str, command))
        process = subprocess.Popen(
            command,
            cwd=repo,
            env=env,
            text=True,
            stdin=subprocess.PIPE if text_input is not None else None,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            start_new_session=True,
        )
        timed_out = False
        try:
            stdout, stderr = process.communicate(text_input, timeout=900)
        except subprocess.TimeoutExpired:
            timed_out = True
            os.killpg(process.pid, signal.SIGKILL)
            stdout, stderr = process.communicate()
        result = subprocess.CompletedProcess(
            command, process.returncode, stdout, stderr
        )
        (scratch / (label + ".stdout")).write_text(result.stdout)
        (scratch / (label + ".stderr")).write_text(result.stderr)
        commands.append(
            {
                "label": label,
                "command": command,
                "exit_status": result.returncode,
                "timed_out": timed_out,
            }
        )
        (scratch / "commands.json").write_text(json.dumps(commands, indent=2) + "\n")
        assert result.returncode == 0, (
            label,
            result.stdout[-2000:],
            result.stderr[-2000:],
        )
        return result.stdout

    def cli(label, *arguments):
        return run(label, [sys.executable, "-m", "pycircuit.cli", *arguments])

    units = []
    for name in SOURCES:
        unit = scratch / (name + "-unit")
        imports = [arg for previous in units for arg in ("-I", previous)]
        cli(
            "compile-" + name,
            "compile",
            "-c",
            fixture / (name + ".py"),
            "--source-root",
            fixture,
            "--package-prefix",
            "history_routed",
            *imports,
            "-o",
            unit,
            "--replace",
        )
        units.append(unit)
        stages.extend(
            {"path": str(path), "sha256": digest(path), "stage": "source-unit"}
            for path in unit.rglob("*")
            if path.is_file()
        )

    traces = {}
    roots = [("module", "pipeline", None)] + [
        ("system", root, name) for name, root in SYSTEMS.items()
    ]
    for profile, root, selected_case in roots:
        top = "history_routed." + (
            "routed_dependency_pipeline.pipeline"
            if profile == "module"
            else "routed_systems." + root
        )
        closure = units[:5] if profile == "module" else units
        final = scratch / (root + ".ac")
        cli(root + "-link", "link", *closure, "--top", top, "-o", final, "--replace")
        stages.append(
            {"path": str(final), "sha256": digest(final), "stage": "verified-final"}
        )
        for backend in ("cpp", "verilog"):
            label = root + "-" + backend
            generated = scratch / label
            cli(
                label + "-emit",
                "emit",
                final,
                "--target",
                backend,
                "-o",
                generated,
                "--replace",
            )
            generated_manifest = json.loads((generated / "generated.json").read_text())
            assert generated_manifest["entry"]["definition"].strip('@"') == top
            stages.extend(
                {"path": str(path), "sha256": digest(path), "stage": backend}
                for path in generated.rglob("*")
                if path.is_file()
            )
            executable = scratch / (label + "-driver")
            if backend == "cpp":
                native_sources = [
                    generated / entry["path"]
                    for entry in generated_manifest["files"]
                    if entry["path"].endswith(".cpp")
                ]
                if profile == "module":
                    probe = scratch / (label + "-visibility")
                    shutil.copytree(generated, probe, dirs_exist_ok=True)
                    for header in probe.rglob("*.hpp"):
                        original = generated / header.relative_to(probe)
                        content = original.read_text()
                        header.write_text(content.replace("private:", "public:"))
                        assert header.read_text().replace(
                            "public:", "private:"
                        ) == content.replace("public:", "private:")
                        visibility.append(
                            {
                                "original": str(original),
                                "sha256": digest(original),
                                "copy": str(header),
                                "copy_sha256": digest(header),
                                "change": "private: to public: only",
                            }
                        )
                    graph_layout = drivers.layout(
                        (
                            probe
                            / "sources/history_routed/routed_dependency_pipeline.hpp"
                        ).read_text()
                    )
                    driver = scratch / (label + "-driver.cpp")
                    driver.write_text(drivers.cpp(graph_layout))
                    native_sources = [
                        probe / path.relative_to(generated) for path in native_sources
                    ] + [driver]
                    includes = probe
                else:
                    includes = generated
                    metadata = json.loads(
                        (generated / "simulation_verification.json").read_text()
                    )
                    assert metadata["source_checks"] == 46
                run(
                    label + "-build",
                    [
                        args.cxx,
                        "-O0" if profile == "system" else "-O1",
                        "-std=c++20",
                        "-pthread",
                        "-I" + str(repo / "include"),
                        "-I" + str(includes),
                        *native_sources,
                        runtime,
                        "-o",
                        executable,
                    ],
                )
            elif profile == "module":
                driver = scratch / (label + "-driver.sv")
                driver.write_text(drivers.verilog(graph_layout))
                run(
                    label + "-build",
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
                        scratch / (label + "-build"),
                        "-o",
                        executable,
                        generated / "design_top.sv",
                        *generated.rglob("*.v"),
                        repo / "include/verilog/fifo.v",
                        repo / "include/verilog/dffe.v",
                        driver,
                    ],
                )
            else:
                run(
                    label + "-build",
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
            cases = (
                vectors["executable"]
                if profile == "module"
                else {selected_case: vectors["executable"][selected_case]}
            )
            for name, case in cases.items():
                for workers in (1, 2) if backend == "cpp" else (1,):
                    case_label = label + "-" + name + f"-workers{workers}"
                    if profile == "module":
                        text = observations.stimulus(case["rows"])
                        input_file = scratch / (name + ".stimulus")
                        input_file.write_text(text)
                        output = run(
                            case_label,
                            (
                                [executable, workers]
                                if backend == "cpp"
                                else [executable, "+stimulus=" + str(input_file)]
                            ),
                            text if backend == "cpp" else None,
                        )
                        actual = observations.decode(
                            output, [row["frame"] for row in case["rows"]]
                        )
                        check_case(case, actual)
                        if (
                            backend == "cpp"
                            and case["first_failure_attempt"] is not None
                        ):
                            fault = case["first_failure_attempt"]
                            actual_lines = [
                                line.split()
                                for line in output.splitlines()
                                if line.startswith("ROW ")
                            ]
                            assert (
                                actual_lines[fault][9]
                                == case["rows"][fault]["errors"][0]
                            )
                        checked_path = scratch / (case_label + ".observed.json")
                        checked_path.write_text(
                            json.dumps(actual, separators=(",", ":")) + "\n"
                        )
                    else:
                        options = (
                            ["--cycles", case["executed_epochs"], "--workers", workers]
                            if backend == "cpp"
                            else ["+cycles=" + str(case["executed_epochs"])]
                        )
                        output = run(case_label, [executable, *options])
                        actual = system_trace(output, case)
                    key = profile + ":" + name
                    if key in traces:
                        assert actual == traces[key], (
                            case_label,
                            "worker/backend observations differ",
                        )
                    traces[key] = actual
                    receipts.append(
                        {
                            "top": top,
                            "profile": profile,
                            "case": name,
                            "backend": backend,
                            "workers": workers,
                            "attempts": case["executed_epochs"],
                            "first_failure_attempt": case["first_failure_attempt"],
                            "host_reset_attempts": case["host_reset_attempts"],
                            "state_bits": 21934 if profile == "module" else None,
                            "final_sha256": digest(final),
                            "stdout_sha256": digest(scratch / (case_label + ".stdout")),
                        }
                    )
                    print(case_label + " passed", flush=True)
    assert all(digest(repo / path) == value for path, value in input_hashes.items())
    assert all(digest(Path(path)) == value for path, value in helper_hashes.items())
    assert digest(runtime) == runtime_hash
    receipt = {
        "cases": receipts,
        "candidate_inputs": input_hashes,
        "native_helpers": helper_hashes,
        "runtime_archive": {"path": str(runtime), "sha256": runtime_hash},
        "read_only_visibility": visibility,
        "stages": stages,
        "baseline": baseline,
        "source_order": list(SOURCES),
        "limits": [
            "The two original full-root tests were build/topology tests, not runtime histories; these runtime vectors are new independent coverage.",
            "Native/spec live-Done, lowest-key, earliest-deadline/key, u64-next-key contract selected; no equivalence to retired PYC first-slot/u8-key policy.",
            "Retired countdown-at-one timing is faithful away overflow; no off-by-one discrepancy is claimed.",
            "All eleven complete module histories execute the same fourteen actual queues; only the two named positive histories execute as closed @system roots.",
            "Seeded overflow/control reference cases are not hardware executions; original u2 route cannot express known invalid route four.",
            "Known-state native mask planes and RTL values checked; no complete X/Z or physical-clock-control matrix claim.",
        ],
    }
    (scratch / "receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")


if __name__ == "__main__":
    main()
